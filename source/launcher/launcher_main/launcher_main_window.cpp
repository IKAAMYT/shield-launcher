#include <winsock2.h>
#include <ws2tcpip.h>
#include "std_include.hpp"
#include "launcher_main_window.hpp"
#include "style.hpp"
#include "settings_dialog.hpp"
#include "window_utils.hpp"
#include "utils.hpp"
#include "../launcher_funcs/dll_loading.hpp"
#include "../launcher_funcs/json_utils.hpp"
#include "../launcher_funcs/auto_update.hpp"
#include <mmsystem.h>
#include <fstream>
#include <QFrame>
#include <QComboBox>
#include <wininet.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wininet.lib")
#include <tlhelp32.h>
#include <regex>
#include <QMetaObject>
#include <QDialog>
#include <QScrollBar>
#include <QLineEdit>
#include <QTextEdit>
#include <QAbstractAnimation>
#include <QFontDatabase>
#include <QFont>
#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include "embedded_fonts.hpp"
#include <QPainter>
#include <QPolygonF>
#include <QWidget>

namespace fs = std::filesystem;

// ============================================================
//  FOND ANIMÉ (classe interne, sans Q_OBJECT donc sans moc)
//  Hexagones qui pulsent + particules dorées. Léger (20 FPS).
// ============================================================
class AnimatedBackground : public QWidget {
public:
    AnimatedBackground(QWidget* parent = nullptr) : QWidget(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        for (int i = 0; i < 26; ++i) {
            Particle p;
            p.x = (float)(rand() % 800);
            p.y = (float)(rand() % 600);
            p.speed = 0.2f + (rand() % 60) / 100.0f;
            p.size = 1.0f + (rand() % 25) / 10.0f;
            p.alpha = 0.05f + (rand() % 30) / 100.0f;
            particles.push_back(p);
        }
        QTimer* t = new QTimer(this);
        QObject::connect(t, &QTimer::timeout, this, [this]() { phase += 0.04f; tick(); update(); });
        t->start(50);
    }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const QColor gold(242, 196, 17);
        float r = 32.0f, dx = r * 1.732f, dy = r * 1.5f;
        int row = 0;
        for (float y = -r; y < height() + r; y += dy, ++row) {
            float off = (row % 2) ? dx / 2 : 0;
            for (float x = -r + off; x < width() + r; x += dx) {
                float pulse = 0.5f + 0.5f * std::sin(phase + (x + y) * 0.01f);
                QColor col = gold; col.setAlphaF(0.03f + pulse * 0.045f);
                QPen pen(col); pen.setWidthF(1.0);
                painter.setPen(pen); painter.setBrush(Qt::NoBrush);
                painter.drawPolygon(hexagon(x, y, r - 3));
            }
        }
        painter.setPen(Qt::NoPen);
        for (auto& p : particles) {
            QColor col = gold; col.setAlphaF(p.alpha);
            painter.setBrush(col);
            painter.drawEllipse(QPointF(p.x, p.y), p.size, p.size);
        }
    }
private:
    struct Particle { float x, y, speed, size, alpha; };
    std::vector<Particle> particles;
    float phase = 0.0f;
    void tick() {
        for (auto& p : particles) {
            p.y -= p.speed;
            p.x += std::sin(phase + p.y * 0.02f) * 0.2f;
            if (p.y < -5) { p.y = (float)height() + 5; p.x = (float)(rand() % (width() > 0 ? width() : 800)); }
        }
    }
    QPolygonF hexagon(float cx, float cy, float r) {
        QPolygonF poly;
        for (int i = 0; i < 6; ++i) {
            float a = 3.14159265f / 180.0f * (60 * i - 30);
            poly << QPointF(cx + r * std::cos(a), cy + r * std::sin(a));
        }
        return poly;
    }
};

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent),
launcherDir(QCoreApplication::applicationDirPath().toStdWString()),
serverIpFile((fs::path(launcherDir) / L"servers.txt").wstring()),
reshadeEnabled(false),
volume(100),
soundPath((fs::path(launcherDir) / L"sounds" / L"startup_sound.mp3").wstring()),
vanillaButton(nullptr),
onlineButton(nullptr),
offlineButton(nullptr),
nameButton(nullptr),
ipButton(nullptr),
discordButton(nullptr),
wikiButton(nullptr),
settingsButton(nullptr),
closeLauncherCheckbox(nullptr),
reshadeCheckbox(nullptr),
volumeLabel(nullptr),
volumeSlider(nullptr),
progressBar(nullptr)
{
    fs::create_directories(launcherDir);
    fs::create_directories(fs::path(launcherDir) / "sounds");

    // ===== Police Rajdhani embarquee (DA AlterCOD) =====
    {
        QFontDatabase::addApplicationFontFromData(
            QByteArray(reinterpret_cast<const char*>(rajdhani_medium_data), rajdhani_medium_size));
        QFontDatabase::addApplicationFontFromData(
            QByteArray(reinterpret_cast<const char*>(rajdhani_semibold_data), rajdhani_semibold_size));
        QFontDatabase::addApplicationFontFromData(
            QByteArray(reinterpret_cast<const char*>(rajdhani_bold_data), rajdhani_bold_size));
        QFont appFont("Rajdhani", 10);
        QApplication::setFont(appFont);
    }

    setWindowTitle("AlterBO4 Launcher");
    setFixedSize(800, 600);

    HINSTANCE hInstance = GetModuleHandle(NULL);
    HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(101));
    if (hIcon) {
        SendMessage((HWND)winId(), WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SendMessage((HWND)winId(), WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    }
    else {
        fs::path iconPath = fs::path(launcherDir) / "images" / "icon.ico";
        if (fs::exists(iconPath)) {
            HICON fileIcon = (HICON)LoadImageW(NULL, std::wstring(iconPath.wstring()).c_str(),
                IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
            if (fileIcon) {
                SendMessage((HWND)winId(), WM_SETICON, ICON_SMALL, (LPARAM)fileIcon);
                SendMessage((HWND)winId(), WM_SETICON, ICON_BIG, (LPARAM)fileIcon);
            }
            QIcon icon(QString::fromStdString(iconPath.string()));
            if (!icon.isNull()) { setWindowIcon(icon); QApplication::setWindowIcon(icon); }
        }
    }

    // ============ FOND ============
    QLabel* backgroundLabel = new QLabel(this);
    backgroundLabel->setFixedSize(800, 600);
    backgroundLabel->move(0, 0);
    backgroundLabel->setStyleSheet("background-color: #08080a;");
    fs::path imagePath = fs::path(launcherDir) / "images" / "launcher.png";
    if (fs::exists(imagePath)) {
        QPixmap background(QString::fromStdString(imagePath.string()));
        if (!background.isNull()) {
            background = background.scaled(800, 600, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            backgroundLabel->setPixmap(background);
            backgroundLabel->setAlignment(Qt::AlignCenter);
        }
    }
    // Fond animé (hexagones + particules)
    AnimatedBackground* animBg = new AnimatedBackground(this);
    animBg->setFixedSize(800, 600);
    animBg->move(0, 0);

    // voile sombre par-dessus le fond pour la lisibilité
    QLabel* overlay = new QLabel(this);
    overlay->setFixedSize(800, 600);
    overlay->move(0, 0);
    overlay->setStyleSheet("background-color: rgba(8, 8, 10, 180);");

    QWidget* container = new QWidget(this);
    container->setFixedSize(800, 600);
    container->setStyleSheet("background: transparent;");
    setCentralWidget(container);

    // styles réutilisables
    const QString sideBtn =
        "QPushButton { background: transparent; color: #8a8a82; border: none;"
        " border-left: 3px solid transparent; text-align: left; padding: 12px 18px;"
        " font-size: 15px; font-weight: bold; letter-spacing: 1px; }"
        "QPushButton:hover { color: #f2c411; border-left: 3px solid #f2c411;"
        " background: rgba(242,196,17,0.08); }";

    const QString goldBtn =
        "QPushButton { background: rgba(10,10,8,200); color: #f2c411;"
        " border: 1px solid #f2c411; border-radius: 5px; padding: 8px 14px;"
        " font-size: 13px; font-weight: bold; letter-spacing: 1px; }"
        "QPushButton:hover { background: #f2c411; color: #08080a; }";

    // ============ LAYOUT GLOBAL : sidebar | contenu ============
    QHBoxLayout* rootLayout = new QHBoxLayout(container);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ----- SIDEBAR GAUCHE -----
    QWidget* sidebar = new QWidget(this);
    sidebar->setFixedWidth(180);
    sidebar->setStyleSheet("background-color: rgba(6,6,8,235); border-right: 1px solid rgba(242,196,17,0.30);");
    QVBoxLayout* sideLayout = new QVBoxLayout(sidebar);
    sideLayout->setContentsMargins(0, 22, 0, 18);
    sideLayout->setSpacing(2);

    // logo + titre en haut de la sidebar
    QLabel* brand = new QLabel("ALTERCOD", this);
    brand->setStyleSheet("color: #f2c411; font-size: 20px; font-weight: bold; letter-spacing: 3px; padding: 0 0 18px 18px; background: transparent;");
    sideLayout->addWidget(brand);

    // bouton JOUER (scroll vers le bas n'a pas de sens ici : il déclenche online aussi possible,
    // mais on le laisse décoratif d'ancrage. Les vrais lancements sont en bas à droite.)
    QPushButton* navPlay = new QPushButton("  JOUER", this);
    navPlay->setStyleSheet(
        "QPushButton { background: rgba(242,196,17,0.10); color: #f2c411; border: none;"
        " border-left: 3px solid #f2c411; text-align: left; padding: 12px 18px;"
        " font-size: 15px; font-weight: bold; letter-spacing: 1px; }");
    sideLayout->addWidget(navPlay);

    // PROFIL -> ouvre le changement de pseudo (réutilise setName)
    nameButton = new QPushButton("  PROFIL", this);
    nameButton->setStyleSheet(sideBtn);
    sideLayout->addWidget(nameButton);

    // SERVEUR -> ouvre le changement d'IP (réutilise setIp)
    ipButton = new QPushButton("  SERVEUR", this);
    ipButton->setStyleSheet(sideBtn);
    sideLayout->addWidget(ipButton);

    // REGLAGES -> ouvre les settings client (réutilise openSettings)
    settingsButton = new QPushButton("  REGLAGES", this);
    settingsButton->setStyleSheet(sideBtn);
    sideLayout->addWidget(settingsButton);

    // Bouton : ouvrir le dossier du jeu
    QPushButton* folderButton = new QPushButton("  DOSSIER DU JEU", this);
    folderButton->setStyleSheet(sideBtn);
    sideLayout->addWidget(folderButton);
    connect(folderButton, &QPushButton::clicked, this, [this]() {
        // Ouvre le dossier parent (le dossier du jeu) dans l'explorateur
        fs::path gameDir = fs::path(launcherDir).parent_path();
        std::string path = gameDir.string();
        ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
    });

    // Bouton : signaler un bug (ouvre le Discord)
    QPushButton* bugButton = new QPushButton("  SIGNALER UN BUG", this);
    bugButton->setStyleSheet(sideBtn);
    sideLayout->addWidget(bugButton);
    connect(bugButton, &QPushButton::clicked, this, [this]() {
        ShellExecuteA(NULL, "open", "https://discord.gg/v9v5DHPYBR", NULL, NULL, SW_SHOWNORMAL);
    });

    // Bouton : chat communautaire
    QPushButton* chatButton = new QPushButton("  CHAT", this);
    chatButton->setStyleSheet(sideBtn);
    sideLayout->addWidget(chatButton);
    connect(chatButton, &QPushButton::clicked, this, &MainWindow::openChat);

    sideLayout->addStretch();

    // ===== Statut serveur compact (sidebar) =====
    serverStatusLabel = new QLabel("  Connexion serveur...", this);
    serverStatusLabel->setStyleSheet("color: #8a8a82; font-size: 11px; font-weight: bold; letter-spacing: 1px; padding: 4px 18px; background: transparent;");
    sideLayout->addWidget(serverStatusLabel);

    // ===== Compteur de temps de jeu total (en bas de la sidebar) =====
    QLabel* playtimeLabel = new QLabel(this);
    playtimeLabel->setObjectName("playtimeLabel");
    playtimeLabel->setStyleSheet("color: #f2c411; font-size: 11px; font-weight: bold; letter-spacing: 1px; padding: 4px 18px; background: transparent;");
    {
        // Charger le total sauvegardé (en secondes) depuis playtime.txt
        long long total = 0;
        fs::path ptFile = fs::path(launcherDir) / "playtime.txt";
        if (fs::exists(ptFile)) {
            std::ifstream in(ptFile);
            in >> total;
        }
        long long h = total / 3600;
        long long m = (total % 3600) / 60;
        playtimeLabel->setText(QString("TEMPS DE JEU\n%1h %2min").arg(h).arg(m));
    }
    sideLayout->addWidget(playtimeLabel);

    // ===== Succès basés sur le temps de jeu =====
    {
        long long total = 0;
        fs::path ptFile = fs::path(launcherDir) / "playtime.txt";
        if (fs::exists(ptFile)) { std::ifstream in(ptFile); if (in) in >> total; }
        long long h = total / 3600;
        QString rank, icon;
        if (h >= 100)      { rank = "LEGENDE";    icon = "\xE2\x98\x85"; }   // ★
        else if (h >= 50)  { rank = "VETERAN";    icon = "\xE2\x98\x85"; }
        else if (h >= 25)  { rank = "EXPERT";     icon = "\xE2\x97\x86"; }   // ◆
        else if (h >= 10)  { rank = "CONFIRME";   icon = "\xE2\x97\x86"; }
        else if (h >= 1)   { rank = "RECRUE";     icon = "\xE2\x96\xB2"; }   // ▲
        else               { rank = "BLEU";       icon = "\xE2\x96\xB2"; }
        QLabel* rankLabel = new QLabel(QString::fromUtf8("%1  RANG : %2").arg(icon).arg(rank), this);
        rankLabel->setStyleSheet("color: #ffd633; font-size: 11px; font-weight: bold; letter-spacing: 1px; padding: 2px 18px 6px 18px; background: transparent;");
        sideLayout->addWidget(rankLabel);
    }

    // ===== Compteur de lancements =====
    {
        long long count = 0;
        fs::path lc = fs::path(launcherDir) / "launches.txt";
        if (fs::exists(lc)) { std::ifstream in(lc); if (in) in >> count; }
        QLabel* launchCountLabel = new QLabel(QString("LANCEMENTS : %1").arg(count), this);
        launchCountLabel->setObjectName("launchCountLabel");
        launchCountLabel->setStyleSheet("color: #8a8a82; font-size: 11px; font-weight: bold; letter-spacing: 1px; padding: 0 18px 6px 18px; background: transparent;");
        sideLayout->addWidget(launchCountLabel);
    }

    QFrame* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: rgba(242,196,17,0.2);");
    sideLayout->addWidget(sep);

    // ===== Réseaux avec compteurs (bas de sidebar) =====
    const QString netBtn =
        "QPushButton { background: transparent; color: #c8c6ba; border: none;"
        " border-left: 3px solid transparent; text-align: left; padding: 8px 18px;"
        " font-size: 12px; font-weight: bold; letter-spacing: 1px; }"
        "QPushButton:hover { color: #f2c411; border-left: 3px solid #f2c411;"
        " background: rgba(242,196,17,0.08); }";

    discordButton = new QPushButton("  DISCORD   7 761", this);
    discordButton->setStyleSheet(netBtn);
    sideLayout->addWidget(discordButton);

    QPushButton* ytButton = new QPushButton("  YOUTUBE   10 010", this);
    ytButton->setStyleSheet(netBtn);
    sideLayout->addWidget(ytButton);

    QPushButton* tiktokButton = new QPushButton("  TIKTOK   3 690", this);
    tiktokButton->setStyleSheet(netBtn);
    sideLayout->addWidget(tiktokButton);

    wikiButton = new QPushButton("  SITE / DOCS", this);
    wikiButton->setStyleSheet(netBtn);
    sideLayout->addWidget(wikiButton);

    // liens réseaux (remplace TIKTOK_URL et YOUTUBE_URL par tes vraies chaines)
    connect(ytButton, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl("https://www.youtube.com/@IKAAM"));
    });
    connect(tiktokButton, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl("https://www.tiktok.com/@ikaam"));
    });

    rootLayout->addWidget(sidebar);

    // ----- ZONE CONTENU DROITE -----
    QWidget* content = new QWidget(this);
    content->setStyleSheet("background: transparent;");
    QVBoxLayout* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(30, 26, 30, 24);
    contentLayout->setSpacing(0);

    // statut serveur (statique, indicateur visuel)
    QLabel* statusLabel = new QLabel(QString::fromUtf8("\xE2\x97\x8F  CLIENT PRET"), this);
    statusLabel->setStyleSheet("color: #39d98a; font-size: 12px; font-weight: bold; letter-spacing: 2px; background: transparent;");
    // Animation : point vert qui clignote (pulsation douce de l'opacite)
    {
        QGraphicsOpacityEffect* statusFx = new QGraphicsOpacityEffect(statusLabel);
        statusLabel->setGraphicsEffect(statusFx);
        QPropertyAnimation* statusAnim = new QPropertyAnimation(statusFx, "opacity", this);
        statusAnim->setDuration(1100);
        statusAnim->setStartValue(1.0);
        statusAnim->setKeyValueAt(0.5, 0.35);
        statusAnim->setEndValue(1.0);
        statusAnim->setLoopCount(-1);
        statusAnim->setEasingCurve(QEasingCurve::InOutSine);
        statusAnim->start();
    }
    contentLayout->addWidget(statusLabel);

    contentLayout->addSpacing(10);

    QLabel* subTitle = new QLabel("BLACK OPS 4  -  CLIENT", this);
    subTitle->setStyleSheet("color: #c8c6ba; font-size: 13px; font-weight: bold; letter-spacing: 3px; background: transparent;");
    contentLayout->addWidget(subTitle);

    QLabel* bigTitle = new QLabel("ALTERCOD", this);
    bigTitle->setStyleSheet("color: #f2c411; font-size: 52px; font-weight: bold; letter-spacing: 4px; background: transparent;");
    contentLayout->addWidget(bigTitle);

    contentLayout->addSpacing(14);

    // badges (version + à jour)
    QHBoxLayout* badgeRow = new QHBoxLayout();
    badgeRow->setSpacing(8);
    QLabel* verBadge = new QLabel(QString("v%1").arg(QString::fromStdString(updater::get_server_version())), this);
    verBadge->setStyleSheet("color: #f2c411; border: 1px solid rgba(242,196,17,0.5); border-radius: 4px; padding: 4px 10px; font-size: 11px; font-weight: bold; letter-spacing: 1px; background: transparent;");
    badgeRow->addWidget(verBadge);
    QLabel* upToDate = new QLabel("A JOUR", this);
    upToDate->setStyleSheet("color: #39d98a; border: 1px solid rgba(57,217,138,0.5); border-radius: 4px; padding: 4px 10px; font-size: 11px; font-weight: bold; letter-spacing: 1px; background: transparent;");
    badgeRow->addWidget(upToDate);
    badgeRow->addStretch();
    contentLayout->addLayout(badgeRow);

    contentLayout->addSpacing(16);

    // ===== Sélecteur de serveur =====
    QLabel* srvLabel = new QLabel("SERVEUR", this);
    srvLabel->setStyleSheet("color: #8a8a82; font-size: 11px; font-weight: bold; letter-spacing: 2px; background: transparent;");
    contentLayout->addWidget(srvLabel);

    serverCombo = new QComboBox(this);
    serverCombo->addItem("Serveur Principal  -  70.55.126.7", "70.55.126.7");
    // Ajoute d'autres serveurs ici : serverCombo->addItem("Nom  -  IP", "IP");
    serverCombo->setStyleSheet(
        "QComboBox { background: rgba(10,10,8,200); color: #f5f3ea; border: 1px solid #f2c411;"
        " border-radius: 4px; padding: 7px 10px; font-size: 13px; font-weight: bold; }"
        "QComboBox::drop-down { border: none; width: 22px; }"
        "QComboBox QAbstractItemView { background: #14140f; color: #f5f3ea;"
        " border: 1px solid #f2c411; selection-background-color: #f2c411; selection-color: #08080a; }");
    contentLayout->addWidget(serverCombo);
    // quand on change de serveur, on écrit l'IP dans la config
    connect(serverCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        QString ip = serverCombo->currentData().toString();
        if (!ip.isEmpty()) saveServerIp(ip.toStdString());
    });

    contentLayout->addSpacing(14);

    // ===== Panneau NEWS (chargé depuis ikaam.fr) =====
    QLabel* newsTitle = new QLabel("LOBBIES EN DIRECT", this);
    newsTitle->setStyleSheet("color: #f2c411; font-size: 12px; font-weight: bold; letter-spacing: 2px; background: transparent;");
    contentLayout->addWidget(newsTitle);

    // Conteneur dynamique de lobbies (boutons Rejoindre)
    lobbyContainer = new QWidget(this);
    lobbyContainer->setStyleSheet("background: rgba(10,10,8,140); border: 1px solid rgba(242,196,17,0.3); border-radius: 4px;");
    lobbyLayout = new QVBoxLayout(lobbyContainer);
    lobbyLayout->setContentsMargins(8, 8, 8, 8);
    lobbyLayout->setSpacing(6);
    lobbyContainer->setMinimumHeight(110);
    QLabel* loadingLbl = new QLabel("Chargement des lobbies...", this);
    loadingLbl->setStyleSheet("color: #c8c6ba; font-size: 12px; background: transparent; border: none;");
    lobbyLayout->addWidget(loadingLbl);
    contentLayout->addWidget(lobbyContainer);

    // Charger les infos serveur (dashboard) en direct, rafraîchi toutes les 30s
    QTimer* serverTimer = new QTimer(this);
    connect(serverTimer, &QTimer::timeout, this, &MainWindow::refreshServerInfo);
    serverTimer->start(30000); // 30 secondes
    QTimer::singleShot(800, this, &MainWindow::refreshServerInfo);

    contentLayout->addStretch();

    // volume
    QHBoxLayout* volRow = new QHBoxLayout();
    volumeLabel = new QLabel("VOLUME", this);
    volumeLabel->setStyleSheet("color: #8a8a82; font-size: 11px; font-weight: bold; letter-spacing: 2px; background: transparent;");
    volRow->addWidget(volumeLabel);
    volumeSlider = new QSlider(Qt::Horizontal, this);
    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(volume);
    volumeSlider->setStyleSheet(
        "QSlider::groove:horizontal { height: 6px; background: rgba(242,196,17,0.12);"
        " border: 1px solid rgba(242,196,17,0.4); border-radius: 3px; }"
        "QSlider::handle:horizontal { background: #f2c411; border: 1px solid #ffd633;"
        " width: 16px; margin-top: -6px; margin-bottom: -6px; border-radius: 8px; }"
        "QSlider::sub-page:horizontal { background: #f2c411; border-radius: 3px; }");
    volRow->addWidget(volumeSlider, 1);
    contentLayout->addLayout(volRow);

    contentLayout->addSpacing(8);

    // case "fermer au lancement"
    closeLauncherCheckbox = new QCheckBox("Fermer le launcher au lancement", this);
    closeLauncherCheckbox->setStyleSheet(
        "QCheckBox { color: #c8c6ba; font-size: 12px; font-weight: bold; background: transparent; }"
        "QCheckBox::indicator { width: 15px; height: 15px; }"
        "QCheckBox::indicator:unchecked { border: 1px solid #f2c411; background: rgba(10,10,8,200); border-radius: 3px; }"
        "QCheckBox::indicator:checked { border: 1px solid #ffd633; background: #f2c411; border-radius: 3px; }");
    contentLayout->addWidget(closeLauncherCheckbox);

    contentLayout->addSpacing(14);

    // barre de progression
    progressBar = new QProgressBar(this);
    progressBar->setStyleSheet(
        "QProgressBar { border: 1px solid #f2c411; border-radius: 4px; text-align: center;"
        " color: #f5f3ea; background: rgba(10,10,8,160); font-weight: bold; }"
        "QProgressBar::chunk { background-color: #f2c411; border-radius: 3px; }");
    progressBar->setVisible(false);
    contentLayout->addWidget(progressBar);

    contentLayout->addSpacing(6);

    // ----- BOUTONS DE LANCEMENT -----
    QHBoxLayout* launchRow = new QHBoxLayout();
    launchRow->setSpacing(10);

    onlineButton = new QPushButton(QString::fromUtf8("\xE2\x96\xB6  ONLINE"), this);
    onlineButton->setStyleSheet(
        "QPushButton { background: #f2c411; color: #08080a; border: none; border-radius: 5px;"
        " padding: 15px; font-size: 17px; font-weight: bold; letter-spacing: 3px; }"
        "QPushButton:hover { background: #ffd633; }"
        "QPushButton:disabled { background: rgba(242,196,17,0.3); color: rgba(8,8,10,0.5); }");
    launchRow->addWidget(onlineButton, 2);
    // Animation : pulsation douce du bouton ONLINE (opacite qui respire)
    {
        QGraphicsOpacityEffect* onlineFx = new QGraphicsOpacityEffect(onlineButton);
        onlineButton->setGraphicsEffect(onlineFx);
        QPropertyAnimation* onlineAnim = new QPropertyAnimation(onlineFx, "opacity", this);
        onlineAnim->setDuration(1600);
        onlineAnim->setStartValue(1.0);
        onlineAnim->setKeyValueAt(0.5, 0.72);
        onlineAnim->setEndValue(1.0);
        onlineAnim->setLoopCount(-1);
        onlineAnim->setEasingCurve(QEasingCurve::InOutSine);
        onlineAnim->start();
    }

    offlineButton = new QPushButton("OFFLINE", this);
    offlineButton->setStyleSheet(
        "QPushButton { background: rgba(10,10,8,200); color: #f2c411; border: 1px solid #f2c411;"
        " border-radius: 5px; padding: 15px; font-size: 14px; font-weight: bold; letter-spacing: 2px; }"
        "QPushButton:hover { background: #f2c411; color: #08080a; }"
        "QPushButton:disabled { color: rgba(242,196,17,0.4); border-color: rgba(242,196,17,0.4); }");
    launchRow->addWidget(offlineButton, 1);

    vanillaButton = new QPushButton("VANILLA", this);
    vanillaButton->setStyleSheet(
        "QPushButton { background: rgba(10,10,8,200); color: #8a8a82; border: 1px solid rgba(242,196,17,0.3);"
        " border-radius: 5px; padding: 15px; font-size: 14px; font-weight: bold; letter-spacing: 2px; }"
        "QPushButton:hover { background: rgba(242,196,17,0.15); color: #f2c411; }"
        "QPushButton:disabled { color: rgba(138,138,130,0.4); }");
    launchRow->addWidget(vanillaButton, 1);

    contentLayout->addLayout(launchRow);

    rootLayout->addWidget(content, 1);

    // mise à jour + son au démarrage (inchangé)
    QTimer::singleShot(500, this, [this]() {
        updater::check_and_prompt_for_updates(this);
        QTimer::singleShot(1000, this, &MainWindow::playStartupSound);
        });

    // ============ CONNEXIONS (mécanique inchangée) ============
    connect(navPlay, &QPushButton::clicked, this, [this]() { startGame(true); });
    connect(vanillaButton, &QPushButton::clicked, this, [this]() { startGame(false, true); });
    connect(onlineButton, &QPushButton::clicked, this, [this]() { startGame(true); });
    connect(offlineButton, &QPushButton::clicked, this, [this]() { startGame(false); });
    connect(nameButton, &QPushButton::clicked, this, &MainWindow::setName);
    connect(ipButton, &QPushButton::clicked, this, &MainWindow::setIp);
    connect(discordButton, &QPushButton::clicked, this, &MainWindow::openDiscord);
    connect(wikiButton, &QPushButton::clicked, this, &MainWindow::openDocs);
    connect(volumeSlider, &QSlider::valueChanged, this, &MainWindow::updateVolume);
    connect(settingsButton, &QPushButton::clicked, this, &MainWindow::openSettings);
    connect(closeLauncherCheckbox, &QCheckBox::stateChanged, this, [this](int state) {
        closeLauncherOnPlay = (state == Qt::Checked);
        saveCloseLauncheronPlay();
        });

    // Charge les réglages sauvegardés
    loadVolumeSettings();
    loadCloseLauncheronPlay();

    // ===== SPLASH ANIMÉ AU DÉMARRAGE =====
    {
        QWidget* splash = new QWidget(this);
        splash->setObjectName("splashScreen");
        splash->setFixedSize(800, 600);
        splash->move(0, 0);
        splash->setStyleSheet("background-color: #050506;");
        QVBoxLayout* sl = new QVBoxLayout(splash);
        sl->setAlignment(Qt::AlignCenter);

        QLabel* splashTitle = new QLabel("ALTERBO4", splash);
        splashTitle->setAlignment(Qt::AlignCenter);
        splashTitle->setStyleSheet("color: #f2c411; font-size: 60px; font-weight: bold; letter-spacing: 10px; background: transparent;");
        sl->addWidget(splashTitle);

        QLabel* splashSub = new QLabel("LAUNCHER", splash);
        splashSub->setAlignment(Qt::AlignCenter);
        splashSub->setStyleSheet("color: #8a8a82; font-size: 16px; letter-spacing: 8px; background: transparent;");
        sl->addWidget(splashSub);

        splash->show();
        splash->raise();

        // Fondu d'apparition du titre
        QGraphicsOpacityEffect* eff = new QGraphicsOpacityEffect(splashTitle);
        splashTitle->setGraphicsEffect(eff);
        QPropertyAnimation* fadeIn = new QPropertyAnimation(eff, "opacity");
        fadeIn->setDuration(700);
        fadeIn->setStartValue(0.0);
        fadeIn->setEndValue(1.0);
        fadeIn->setEasingCurve(QEasingCurve::OutCubic);
        fadeIn->start(QAbstractAnimation::DeleteWhenStopped);

        // Disparition du splash après 1.6s (fondu de sortie)
        QTimer::singleShot(1600, this, [splash]() {
            QGraphicsOpacityEffect* outEff = new QGraphicsOpacityEffect(splash);
            splash->setGraphicsEffect(outEff);
            QPropertyAnimation* fadeOut = new QPropertyAnimation(outEff, "opacity");
            fadeOut->setDuration(500);
            fadeOut->setStartValue(1.0);
            fadeOut->setEndValue(0.0);
            QObject::connect(fadeOut, &QPropertyAnimation::finished, splash, &QWidget::deleteLater);
            fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
        });
    }
}


void MainWindow::addPlaytimeAndRefresh() {
    if (!gameRunning) return;
    gameRunning = false;
    time_t now = time(nullptr);
    long long elapsed = (long long)(now - gameStartTime);
    if (elapsed < 0) elapsed = 0;

    // Charger l'ancien total
    long long total = 0;
    std::filesystem::path ptFile = std::filesystem::path(launcherDir) / "playtime.txt";
    if (std::filesystem::exists(ptFile)) {
        std::ifstream in(ptFile);
        in >> total;
    }
    total += elapsed;
    // Sauver
    std::ofstream out(ptFile);
    out << total;
    out.close();

    // Rafraîchir le label
    QLabel* lbl = this->findChild<QLabel*>("playtimeLabel");
    if (lbl) {
        long long h = total / 3600;
        long long m = (total % 3600) / 60;
        lbl->setText(QString("TEMPS DE JEU\n%1h %2min").arg(h).arg(m));
    }
}

void MainWindow::startGame(bool isOnline, bool isVanilla) {

    // ===== Détecteur : BlackOps4 déjà lancé ? =====
    {
        bool alreadyRunning = false;
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32 pe; pe.dwSize = sizeof(pe);
            if (Process32First(snap, &pe)) {
                do {
                    if (_stricmp(pe.szExeFile, "BlackOps4.exe") == 0) { alreadyRunning = true; break; }
                } while (Process32Next(snap, &pe));
            }
            CloseHandle(snap);
        }
        if (alreadyRunning) {
            QMessageBox msgBox(QMessageBox::Warning, "Jeu deja lance",
                QString::fromUtf8("Black Ops 4 est deja en cours d'execution.\nFerme le jeu avant d'en relancer un."),
                QMessageBox::Ok, this);
            msgBox.exec();
            return;
        }
    }

    // Marque le début de session de jeu (pour le compteur de temps)
    gameStartTime = time(nullptr);
    gameRunning = true;

    // ===== Compteur de lancements (+1) =====
    {
        fs::path lc = fs::path(launcherDir) / "launches.txt";
        long long count = 0;
        if (fs::exists(lc)) { std::ifstream in(lc); if (in) in >> count; }
        count++;
        { std::ofstream out(lc); out << count; }
        QLabel* ll = this->findChild<QLabel*>("launchCountLabel");
        if (ll) ll->setText(QString("LANCEMENTS : %1").arg(count));
    }

    // ===== Écran de chargement cinématique =====
    {
        QWidget* loadingScreen = new QWidget(this);
        loadingScreen->setObjectName("loadingScreen");
        loadingScreen->setFixedSize(800, 600);
        loadingScreen->move(0, 0);
        loadingScreen->setStyleSheet("background-color: rgba(5, 5, 6, 240);");
        QVBoxLayout* ll = new QVBoxLayout(loadingScreen);
        ll->setAlignment(Qt::AlignCenter);

        QLabel* logo = new QLabel("ALTERCOD", loadingScreen);
        logo->setAlignment(Qt::AlignCenter);
        logo->setStyleSheet("color: #f2c411; font-size: 46px; font-weight: bold; letter-spacing: 6px; background: transparent;");
        ll->addWidget(logo);

        QLabel* status = new QLabel("Initialisation...", loadingScreen);
        status->setObjectName("loadingStatus");
        status->setAlignment(Qt::AlignCenter);
        status->setStyleSheet("color: #c8c6ba; font-size: 14px; letter-spacing: 2px; background: transparent;");
        ll->addWidget(status);

        QProgressBar* lbar = new QProgressBar(loadingScreen);
        lbar->setFixedWidth(360);
        lbar->setTextVisible(false);
        lbar->setRange(0, 100);
        lbar->setValue(0);
        lbar->setStyleSheet(
            "QProgressBar { border: 1px solid #f2c411; border-radius: 4px; background: rgba(10,10,8,160); height: 10px; }"
            "QProgressBar::chunk { background-color: #f2c411; border-radius: 3px; }");
        ll->addWidget(lbar, 0, Qt::AlignCenter);

        loadingScreen->show();
        loadingScreen->raise();

        // Messages cinématiques qui défilent
        static const char* msgs[] = {
            "Connexion au serveur AlterBO4...",
            "Verification des fichiers...",
            "Chargement des ressources...",
            "Preparation du client...",
            "Lancement du jeu..."
        };
        for (int i = 0; i <= 100; i += 4) {
            lbar->setValue(i);
            int idx = (i * 5) / 101;
            if (idx < 5) status->setText(QString::fromUtf8(msgs[idx]));
            QApplication::processEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(45));
        }
        // L'écran sera retiré à la fin de startGame
    }

    vanillaButton->setEnabled(false);
    onlineButton->setEnabled(false);
    offlineButton->setEnabled(false);

    progressBar->setVisible(true);
    progressBar->setValue(0);

    // not really needed for multiple instances
    /*
    if (DllLoading::isGameRunning()) {
        showMessageBox(QMessageBox::Warning, "Warning", "Game is already running!");
        progressBar->setVisible(false);

		vanillaButton->setEnabled(true);
        onlineButton->setEnabled(true);
        offlineButton->setEnabled(true);
        return;
    }
    */

    progressBar->setValue(15);

    copyLPCFolder();

    progressBar->setValue(25);

    std::string currentDir = QCoreApplication::applicationDirPath().toStdString();

    if (!isVanilla)
    {
        auto result = DllLoading::extractDlls(currentDir, isOnline, reshadeEnabled);
        if(result != DllLoading::Result::Success) {
            QString errorMsg;
            switch(result) {
            case DllLoading::Result::FileNotFound:
                errorMsg = QString("Required files not found! Make sure %1 exists in the launcher directory.")
                    .arg(isOnline ? "mp.zip" : "solo.zip");
                break;
            case DllLoading::Result::InvalidGamePath:
                errorMsg = "BlackOps4.exe not found! Make sure the launcher shortcut is in the game directory.";
                break;
            case DllLoading::Result::ZipError:
                errorMsg = "Failed to extract files!";
                break;
            default:
                errorMsg = "Unknown error occurred while extracting files.";
            }
            showMessageBox(QMessageBox::Critical, "Error", errorMsg);
            progressBar->setVisible(false);

            vanillaButton->setEnabled(true);
            onlineButton->setEnabled(true);
            offlineButton->setEnabled(true);
            return;
        }
    }

    progressBar->setValue(75);

	std::string game_exe = "BlackOps4.exe";

    // bnet's
    if (isVanilla)
    {
        game_exe = "Black Ops 4 Launcher.exe";

		// delete our dll if it exist.
        fs::path xinputPath = fs::path(currentDir) / "XInput9_1_0.dll";
        if (fs::exists(xinputPath)) {
            fs::remove(xinputPath);
		}
    }

    fs::path currentPath = fs::path(currentDir);
    fs::path gameExePath;

    if (fs::exists(currentPath / game_exe)) {
        gameExePath = currentPath / game_exe;
    }
    else if (fs::exists(currentPath.parent_path().parent_path() / game_exe)) {
        gameExePath = currentPath.parent_path().parent_path() / game_exe;
    }
    else {
        showMessageBox(QMessageBox::Critical, "Error", "Could not locate BlackOps4.exe!");
        progressBar->setVisible(false);

		vanillaButton->setEnabled(true);
        onlineButton->setEnabled(true);
        offlineButton->setEnabled(true);
        return;
    }

    if (!DllLoading::launchGame(gameExePath.string(), isOnline)) {
        showMessageBox(QMessageBox::Critical, "Error", "Failed to start the game!");
        progressBar->setVisible(false);

		vanillaButton->setEnabled(true);
        onlineButton->setEnabled(true);
        offlineButton->setEnabled(true);
        return;
    }

    progressBar->setValue(100);
    progressBar->setVisible(false);

	vanillaButton->setEnabled(true);
    onlineButton->setEnabled(true);
    offlineButton->setEnabled(true);

    // ===== Comptage du temps de jeu (thread, marche même si launcher fermé) =====
    {
        std::string ldir = std::filesystem::path(launcherDir).string();
        std::thread([ldir]() {
            time_t start = time(nullptr);
            // Attendre que BlackOps4.exe se ferme
            bool wasRunning = false;
            for (;;) {
                bool running = false;
                HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
                if (snap != INVALID_HANDLE_VALUE) {
                    PROCESSENTRY32 pe; pe.dwSize = sizeof(pe);
                    if (Process32First(snap, &pe)) {
                        do {
                            if (_stricmp(pe.szExeFile, "BlackOps4.exe") == 0) { running = true; break; }
                        } while (Process32Next(snap, &pe));
                    }
                    CloseHandle(snap);
                }
                if (running) wasRunning = true;
                if (wasRunning && !running) break;   // le jeu s'est fermé
                // sécurité : si le jeu n'a jamais démarré après 60s, on arrête
                if (!wasRunning && (time(nullptr) - start) > 60) return;
                std::this_thread::sleep_for(std::chrono::seconds(3));
            }
            // Ajouter le temps écoulé au total
            time_t end = time(nullptr);
            long long elapsed = (long long)(end - start);
            if (elapsed < 0) elapsed = 0;
            std::filesystem::path ptFile = std::filesystem::path(ldir) / "playtime.txt";
            long long total = 0;
            { std::ifstream in(ptFile); if (in) in >> total; }
            total += elapsed;
            { std::ofstream out(ptFile); out << total; }
        }).detach();
    }

    // Retirer l'écran de chargement
    {
        QWidget* ls = this->findChild<QWidget*>("loadingScreen");
        if (ls) ls->deleteLater();
    }

	// close launcher after starting game
    if (closeLauncherOnPlay)
	    QTimer::singleShot(1000, this, &MainWindow::close);
}

void MainWindow::setName() {

    QDialog dialog(this);
    dialog.setWindowTitle("Set Name");
    dialog.setFixedWidth(400);

    WindowUtils::setWindowIcon(&dialog);

    dialog.setStyleSheet(Style::getDarkStyleSheet());

    QVBoxLayout layout(&dialog);

    std::string currentDir = QCoreApplication::applicationDirPath().toStdString();
    fs::path gamePath = fs::path(currentDir);
    fs::path gameExePath = gamePath / "BlackOps4.exe";

    if (!fs::exists(gameExePath)) {

        fs::path parentPath = gamePath.parent_path().parent_path();
        gameExePath = parentPath / "BlackOps4.exe";

        if (fs::exists(gameExePath)) {
            gamePath = parentPath;
        }
    }

    std::string jsonPath = (gamePath / "project-bo4.json").string();
    std::string currentName = JsonUtils::getJsonItem(jsonPath, "identity", "name");

    QLabel label("Enter your name:");
    layout.addWidget(&label);

    QLineEdit lineEdit;
    lineEdit.setText(QString::fromStdString(currentName));
    layout.addWidget(&lineEdit);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout.addWidget(&buttonBox);

    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        std::string newName = lineEdit.text().toStdString();
        if (newName.empty()) {
            QMessageBox msgBox(QMessageBox::Warning, "Warning", "Name cannot be empty!", QMessageBox::Ok, this);
            WindowUtils::setWindowIcon(&msgBox);
            msgBox.exec();
            return;
        }

        if (!JsonUtils::replaceJsonValue(jsonPath, newName, "identity", "name")) {
            QMessageBox msgBox(QMessageBox::Critical, "Error", "Failed to save name!", QMessageBox::Ok, this);
            WindowUtils::setWindowIcon(&msgBox);
            msgBox.exec();
        }
    }
}

// ============================================================
//  CHAT COMMUNAUTAIRE (backend chat.php sur ikaam.fr)
// ============================================================
void MainWindow::openChat() {
    QDialog* dlg = new QDialog(this);
    dlg->setWindowTitle(QString::fromUtf8("Chat AlterBO4"));
    dlg->setFixedSize(460, 560);
    dlg->setStyleSheet(
        "QDialog{background:#0a0a08;}"
        "QTextEdit{background:#111110;color:#e8e6da;border:1px solid rgba(242,196,17,.3);border-radius:6px;font-size:13px;}"
        "QLineEdit{background:#15140f;color:#fff;border:1px solid rgba(242,196,17,.4);border-radius:6px;padding:8px;font-size:13px;}"
        "QPushButton{background:#f2c411;color:#08080a;border:none;border-radius:6px;padding:8px 16px;font-weight:bold;}"
        "QPushButton:hover{background:#ffd633;}"
        "QLabel{color:#f2c411;font-weight:bold;font-size:14px;letter-spacing:1px;}");
    WindowUtils::setWindowIcon(dlg);

    QVBoxLayout* lay = new QVBoxLayout(dlg);
    QLabel* title = new QLabel(QString::fromUtf8("\xF0\x9F\x92\xAC  CHAT COMMUNAUTAIRE"), dlg);
    lay->addWidget(title);

    QTextEdit* msgArea = new QTextEdit(dlg);
    msgArea->setReadOnly(true);
    lay->addWidget(msgArea, 1);

    QHBoxLayout* inputRow = new QHBoxLayout();
    QLineEdit* input = new QLineEdit(dlg);
    input->setPlaceholderText(QString::fromUtf8("\xC3\x89cris un message..."));
    input->setMaxLength(300);
    QPushButton* sendBtn = new QPushButton("Envoyer", dlg);
    inputRow->addWidget(input, 1);
    inputRow->addWidget(sendBtn);
    lay->addLayout(inputRow);

    // Récupérer le pseudo depuis project-bo4.json (identity.name)
    auto getPseudo = [this]() -> std::string {
        std::string currentDir = QCoreApplication::applicationDirPath().toStdString();
        fs::path gamePath = fs::path(currentDir);
        if (!fs::exists(gamePath / "project-bo4.json")) {
            fs::path pp = gamePath.parent_path().parent_path();
            if (fs::exists(pp / "project-bo4.json")) gamePath = pp;
        }
        std::string jsonPath = (gamePath / "project-bo4.json").string();
        std::string n = JsonUtils::getJsonItem(jsonPath, "identity", "name");
        if (n.empty()) n = "Joueur";
        return n;
    };

    // Fonction de rafraîchissement des messages (GET chat.php)
    auto refreshMessages = [msgArea]() {
        std::thread([msgArea]() {
            std::string json;
            HINTERNET hNet = InternetOpenA("AlterBO4-Chat", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
            if (hNet) {
                HINTERNET hUrl = InternetOpenUrlA(hNet, "https://ikaam.fr/chat.php",
                    NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0);
                if (hUrl) {
                    char buf[4096]; DWORD read = 0;
                    while (InternetReadFile(hUrl, buf, sizeof(buf)-1, &read) && read > 0) { buf[read]=0; json += buf; }
                    InternetCloseHandle(hUrl);
                }
                InternetCloseHandle(hNet);
            }
            // Parsing simple des messages (name + msg)
            QString html;
            try {
                std::regex rMsg("\"name\":\"(.*?)\",\"msg\":\"(.*?)\",\"ts\"");
                auto begin = std::sregex_iterator(json.begin(), json.end(), rMsg);
                auto end = std::sregex_iterator();
                for (auto it = begin; it != end; ++it) {
                    std::string nm = (*it)[1], ms = (*it)[2];
                    // déséchapper \/ et unicode basique
                    QString qn = QString::fromUtf8(nm.c_str());
                    QString qm = QString::fromUtf8(ms.c_str());
                    qm.replace("\\/", "/").replace("\\\"", "\"");
                    qn.replace("\\/", "/");
                    html += "<b style='color:#f2c411'>" + qn.toHtmlEscaped() + "</b> : " + qm.toHtmlEscaped() + "<br>";
                }
            } catch (...) {}
            if (html.isEmpty()) html = "<i style='color:#888'>Aucun message. Sois le premier !</i>";
            QMetaObject::invokeMethod(msgArea, [msgArea, html]() {
                int sb = msgArea->verticalScrollBar()->value();
                bool atBottom = (sb >= msgArea->verticalScrollBar()->maximum() - 4);
                msgArea->setHtml(html);
                if (atBottom) msgArea->verticalScrollBar()->setValue(msgArea->verticalScrollBar()->maximum());
            }, Qt::QueuedConnection);
        }).detach();
    };

    // Envoi d'un message (POST chat.php)
    auto sendMessage = [input, getPseudo, refreshMessages]() {
        QString text = input->text().trimmed();
        if (text.isEmpty()) return;
        std::string pseudo = getPseudo();
        std::string msg = text.toStdString();
        input->clear();
        std::thread([pseudo, msg, refreshMessages]() {
            // URL-encode minimal
            auto enc = [](const std::string& s) {
                std::string out;
                char hex[4];
                for (unsigned char ch : s) {
                    if (isalnum(ch) || ch=='-'||ch=='_'||ch=='.'||ch=='~') out += ch;
                    else { sprintf(hex, "%%%02X", ch); out += hex; }
                }
                return out;
            };
            std::string post = "name=" + enc(pseudo) + "&msg=" + enc(msg);
            HINTERNET hNet = InternetOpenA("AlterBO4-Chat", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
            if (hNet) {
                HINTERNET hCon = InternetConnectA(hNet, "ikaam.fr", INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
                if (hCon) {
                    HINTERNET hReq = HttpOpenRequestA(hCon, "POST", "/chat.php", NULL, NULL, NULL, INTERNET_FLAG_SECURE, 0);
                    if (hReq) {
                        std::string headers = "Content-Type: application/x-www-form-urlencoded";
                        HttpSendRequestA(hReq, headers.c_str(), (DWORD)headers.size(), (LPVOID)post.c_str(), (DWORD)post.size());
                        InternetCloseHandle(hReq);
                    }
                    InternetCloseHandle(hCon);
                }
                InternetCloseHandle(hNet);
            }
            // Rafraîchir après envoi
            refreshMessages();
        }).detach();
    };

    QObject::connect(sendBtn, &QPushButton::clicked, dlg, [sendMessage]() { sendMessage(); });
    QObject::connect(input, &QLineEdit::returnPressed, dlg, [sendMessage]() { sendMessage(); });

    // Timer de rafraîchissement (5s)
    QTimer* chatTimer = new QTimer(dlg);
    QObject::connect(chatTimer, &QTimer::timeout, dlg, [refreshMessages]() { refreshMessages(); });
    chatTimer->start(5000);
    refreshMessages(); // premier chargement

    dlg->exec();
}

void MainWindow::refreshServerInfo() {
    std::thread([this]() {
        // --- Mesure du ping (temps de connexion TCP au serveur) ---
        long pingMs = -1;
        {
            WSADATA wsa;
            if (WSAStartup(MAKEWORD(2,2), &wsa) == 0) {
                SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                if (sock != INVALID_SOCKET) {
                    // mode non-bloquant pour timeout
                    u_long mode = 1; ioctlsocket(sock, FIONBIO, &mode);
                    sockaddr_in addr; addr.sin_family = AF_INET; addr.sin_port = htons(8080);
                    addr.sin_addr.s_addr = inet_addr("70.55.126.7");
                    auto t0 = std::chrono::steady_clock::now();
                    connect(sock, (sockaddr*)&addr, sizeof(addr));
                    fd_set wset; FD_ZERO(&wset); FD_SET(sock, &wset);
                    timeval tv; tv.tv_sec = 2; tv.tv_usec = 0;
                    if (select(0, NULL, &wset, NULL, &tv) > 0) {
                        auto t1 = std::chrono::steady_clock::now();
                        pingMs = (long)std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
                    }
                    closesocket(sock);
                }
                WSACleanup();
            }
        }

        std::string html;
        HINTERNET hNet = InternetOpenA("AlterBO4-Launcher", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (hNet) {
            HINTERNET hUrl = InternetOpenUrlA(hNet, "http://70.55.126.7:8080/",
                NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
            if (hUrl) {
                char buf[4096]; DWORD read = 0;
                while (InternetReadFile(hUrl, buf, sizeof(buf) - 1, &read) && read > 0) {
                    buf[read] = 0; html += buf;
                }
                InternetCloseHandle(hUrl);
            }
            InternetCloseHandle(hNet);
        }

        // --- Parsing ---
        std::string players = "?", lobbies = "?";
        std::string lobbyList;
        bool online = !html.empty();

        try {
            std::smatch m;
            std::regex rPlayers("PLAYERS ONLINE[\\s\\S]*?<h6[^>]*>\\s*(\\d+)");
            if (std::regex_search(html, m, rPlayers)) players = m[1];
            std::regex rLobbies("LOBBYS ACTIVE[\\s\\S]*?<h6[^>]*>\\s*(\\d+)");
            if (std::regex_search(html, m, rLobbies)) lobbies = m[1];

            // Lobbies : Hosted by X ... <td>MAP[MP/ZM] ... <td>N</td>
            std::regex rLobby("Lobby_\\d+<sup>\\[Hosted by ([^\\]]+)\\][\\s\\S]*?<td>([^<]+)\\[(?:MP|ZM)\\]<sup>[\\s\\S]*?<td>(\\d+)</td>");
            auto begin = std::sregex_iterator(html.begin(), html.end(), rLobby);
            auto end = std::sregex_iterator();
            for (auto it = begin; it != end; ++it) {
                std::string host = (*it)[1];
                std::string map = (*it)[2];
                std::string pl = (*it)[3];
                std::string id = (*it)[4];
                // format : map|host|joueurs|id séparés par des tabulations, lobbies par \n
                lobbyList += map + "\t" + host + "\t" + pl + "\t" + id + "\n";
            }
        } catch (...) {}

        if (lobbyList.empty()) lobbyList = online ? "Aucun lobby actif." : "Serveur injoignable.";

        // --- Mise à jour UI ---
        QString sideTxt;
        if (online)
            sideTxt = QString::fromUtf8("\xE2\x97\x8F  %1 EN LIGNE \xC2\xB7 %2 LOBBIES")
                        .arg(QString::fromStdString(players)).arg(QString::fromStdString(lobbies));
        else
            sideTxt = QString::fromUtf8("\xE2\x97\x8F  Serveur hors ligne");

        QString lobbyData = QString::fromUtf8(lobbyList.c_str()).trimmed();
        bool isOnline = online;
        QString pingTxt = (pingMs >= 0) ? QString("  \xE2\x80\xA2  %1 ms").arg(pingMs) : QString();
        QString sideTxtFull = sideTxt + (isOnline ? pingTxt : QString());

        QMetaObject::invokeMethod(this, [this, sideTxtFull, lobbyData, isOnline]() {
            if (serverStatusLabel) {
                serverStatusLabel->setText(sideTxtFull);
                serverStatusLabel->setStyleSheet(QString(
                    "color: %1; font-size: 11px; font-weight: bold; letter-spacing: 1px; padding: 4px 18px; background: transparent;")
                    .arg(isOnline ? "#39d98a" : "#d9534f"));
            }
            // Vider l'ancienne liste
            if (lobbyLayout) {
                QLayoutItem* item;
                while ((item = lobbyLayout->takeAt(0)) != nullptr) {
                    if (item->widget()) item->widget()->deleteLater();
                    delete item;
                }
                if (lobbyData.isEmpty() || !isOnline) {
                    QLabel* lbl = new QLabel(isOnline ? "Aucun lobby actif." : "Serveur injoignable.", lobbyContainer);
                    lbl->setStyleSheet("color: #c8c6ba; font-size: 12px; background: transparent; border: none;");
                    lobbyLayout->addWidget(lbl);
                } else {
                    QStringList rows = lobbyData.split("\n", Qt::SkipEmptyParts);
                    for (const QString& row : rows) {
                        QStringList parts = row.split("\t");
                        if (parts.size() < 3) continue;
                        QString map = parts[0].trimmed();
                        QString host = parts[1].trimmed();
                        QString pl = parts[2].trimmed();
                        QLabel* info = new QLabel(QString("\xE2\x96\xB8  %1  \xC2\xB7  %2 joueurs  (%3)").arg(map).arg(pl).arg(host), lobbyContainer);
                        info->setStyleSheet("color: #f5f3ea; font-size: 12px; font-weight: bold; background: transparent; border: none;");
                        lobbyLayout->addWidget(info);
                    }
                }
            }
        }, Qt::QueuedConnection);
    }).detach();
}

void MainWindow::saveServerIp(const std::string& ip) {
    std::string currentDir = QCoreApplication::applicationDirPath().toStdString();
    std::filesystem::path gamePath = std::filesystem::path(currentDir);
    std::string jsonPath = (gamePath / "project-bo4.json").string();
    JsonUtils::replaceJsonValue(jsonPath, ip, "demonware", "ipv4");
}

void MainWindow::setIp() {

    QDialog dialog(this);
    dialog.setWindowTitle("Set Server IP");
    dialog.setFixedWidth(400);

    WindowUtils::setWindowIcon(&dialog);

    dialog.setStyleSheet(Style::getDarkStyleSheet());

    QVBoxLayout layout(&dialog);

    std::string currentDir = QCoreApplication::applicationDirPath().toStdString();
    fs::path gamePath = fs::path(currentDir);
    fs::path gameExePath = gamePath / "BlackOps4.exe";

    if (!fs::exists(gameExePath)) {
        fs::path parentPath = gamePath.parent_path().parent_path();
        gameExePath = parentPath / "BlackOps4.exe";

        if (fs::exists(gameExePath)) {
            gamePath = parentPath;
        }
    }

    std::string jsonPath = (gamePath / "project-bo4.json").string();
    std::string currentIp = JsonUtils::getJsonItem(jsonPath, "demonware", "ipv4");

    QLabel label("Enter server IP address:");
    layout.addWidget(&label);

    QLineEdit lineEdit;
    lineEdit.setText(QString::fromStdString(currentIp));
    layout.addWidget(&lineEdit);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout.addWidget(&buttonBox);

    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        std::string newIp = lineEdit.text().toStdString();
        if (newIp.empty()) {
            QMessageBox msgBox(QMessageBox::Warning, "Warning", "IP address cannot be empty!", QMessageBox::Ok, this);
            WindowUtils::setWindowIcon(&msgBox);
            msgBox.exec();
            return;
        }

        if (!JsonUtils::replaceJsonValue(jsonPath, newIp, "demonware", "ipv4")) {
            QMessageBox msgBox(QMessageBox::Critical, "Error", "Failed to save IP address!", QMessageBox::Ok, this);
            WindowUtils::setWindowIcon(&msgBox);
            msgBox.exec();
        }
    }
}

void MainWindow::openDiscord() {

    QDesktopServices::openUrl(QUrl("https://discord.gg/v9v5DHPYBR"));
}

void MainWindow::openDocs() {

    QDesktopServices::openUrl(QUrl("https://ikaam.fr/AlterCOD"));
}

void MainWindow::updateVolume(int value) {
    // Only update the internal volume variable, don't change system volume
    volume = value;
    volumeLabel->setText(QString("Volume: %1%").arg(value));
    
    // If the MP3 is currently playing, update its volume
    std::wstring volumeCmd = L"setaudio mp3 volume to " + std::to_wstring(value * 10);
    mciSendStringW(volumeCmd.c_str(), NULL, 0, NULL);
    
    // Save the volume setting
    saveVolumeSettings();
}

void MainWindow::openSettings() {

    fs::path gameDir = fs::path(launcherDir).parent_path().parent_path();
    std::string jsonPath = (gameDir / "project-bo4.json").string();

    SettingsDialog settingsDialog(this);
    settingsDialog.setJsonPath(jsonPath);
    settingsDialog.exec();
}

void MainWindow::playStartupSound() {

    if (!fs::exists(soundPath)) {
        QMessageBox::information(this, "Debug", "Sound file not found at: " + QString::fromStdWString(soundPath));
        return;
    }

    // Close any existing MP3 playback
    std::wstring closeCmd = L"close mp3";
    mciSendStringW(closeCmd.c_str(), NULL, 0, NULL);

    // Open the MP3 file
    std::wstring openCmd = L"open \"" + soundPath + L"\" type mpegvideo alias mp3";
    MCIERROR openError = mciSendStringW(openCmd.c_str(), NULL, 0, NULL);
    
    if (openError != 0) {
        wchar_t errorText[256];
        mciGetErrorStringW(openError, errorText, 256);
        QMessageBox::warning(this, "Sound Error", "Failed to open sound file: " + 
                            QString::fromWCharArray(errorText));
        return;
    }
    
    // Set the volume for the MP3 (volume range for MCI is 0-1000, our volume is 0-100)
    std::wstring volumeCmd = L"setaudio mp3 volume to " + std::to_wstring(volume * 10);
    mciSendStringW(volumeCmd.c_str(), NULL, 0, NULL);

    // Play the MP3
    std::wstring playCmd = L"play mp3";
    MCIERROR playError = mciSendStringW(playCmd.c_str(), NULL, 0, NULL);
    
    if (playError != 0) {
        wchar_t errorText[256];
        mciGetErrorStringW(playError, errorText, 256);
        QMessageBox::warning(this, "Sound Error", "Failed to play sound file: " + 
                            QString::fromWCharArray(errorText));
    }
}

void MainWindow::showMessageBox(QMessageBox::Icon icon, const QString& title, const QString& text) {

    QMessageBox msgBox(this);
    msgBox.setIcon(icon);
    msgBox.setWindowTitle(title);
    msgBox.setText(text);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}

void MainWindow::saveCloseLauncheronPlay() {
    try {

        std::string jsonPath = (fs::path(launcherDir) / "launcher-config.json").string();

        rapidjson::Document document;
        if(fs::exists(jsonPath)) {
            std::ifstream file(jsonPath);
            std::string jsonContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();

            if(!jsonContent.empty()) {
                document.Parse(jsonContent.c_str());
            }
        }

        if(!document.IsObject()) {
            document.SetObject();
        }

        if(!document.HasMember("launcher")) {
            rapidjson::Value audioObject(rapidjson::kObjectType);
            document.AddMember("launcher", audioObject, document.GetAllocator());
        }

        rapidjson::Value& audioObject = document["launcher"];
        if(audioObject.HasMember("closeLauncherOnPlay")) {
            audioObject["closeLauncherOnPlay"] = closeLauncherOnPlay;
        }
        else {
            rapidjson::Value closeValue(closeLauncherOnPlay);
            audioObject.AddMember("closeLauncherOnPlay", closeValue, document.GetAllocator());
        }

        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        document.Accept(writer);

        std::ofstream outFile(jsonPath);
        outFile << buffer.GetString();
        outFile.close();
    }
    catch(const std::exception& e) {

    }
}

void MainWindow::loadCloseLauncheronPlay() {
    try {
        std::string jsonPath = (fs::path(launcherDir) / "launcher-config.json").string();

        if(fs::exists(jsonPath)) {
            std::ifstream file(jsonPath);
            std::string jsonContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();

            if(!jsonContent.empty()) {
                rapidjson::Document document;
                document.Parse(jsonContent.c_str());

                if(document.IsObject() && document.HasMember("launcher") &&
                    document["launcher"].IsObject() && document["launcher"].HasMember("closeLauncherOnPlay") &&
                    document["launcher"]["closeLauncherOnPlay"].IsBool()) {
                    closeLauncherOnPlay = document["launcher"]["closeLauncherOnPlay"].GetBool();

                    if(closeLauncherCheckbox) {
                        closeLauncherCheckbox->setChecked(closeLauncherOnPlay);
                    }
                }
            }
        }
    }
    catch(const std::exception& e) {
    }
}

void MainWindow::saveVolumeSettings() {
    try {

        std::string jsonPath = (fs::path(launcherDir) / "launcher-config.json").string();

        rapidjson::Document document;
        if (fs::exists(jsonPath)) {
            std::ifstream file(jsonPath);
            std::string jsonContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();

            if (!jsonContent.empty()) {
                document.Parse(jsonContent.c_str());
            }
        }

        if (!document.IsObject()) {
            document.SetObject();
        }

        if (!document.HasMember("audio")) {
            rapidjson::Value audioObject(rapidjson::kObjectType);
            document.AddMember("audio", audioObject, document.GetAllocator());
        }

        rapidjson::Value& audioObject = document["audio"];
        if (audioObject.HasMember("volume")) {
            audioObject["volume"] = volume;
        }
        else {
            rapidjson::Value volumeValue(volume);
            audioObject.AddMember("volume", volumeValue, document.GetAllocator());
        }

        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        document.Accept(writer);

        std::ofstream outFile(jsonPath);
        outFile << buffer.GetString();
        outFile.close();
    }
    catch (const std::exception& e) {
        
    }
}

void MainWindow::loadVolumeSettings() {
    try {
        std::string jsonPath = (fs::path(launcherDir) / "launcher-config.json").string();

        if (fs::exists(jsonPath)) {
            std::ifstream file(jsonPath);
            std::string jsonContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();

            if (!jsonContent.empty()) {
                rapidjson::Document document;
                document.Parse(jsonContent.c_str());

                if (document.IsObject() && document.HasMember("audio") && 
                    document["audio"].IsObject() && document["audio"].HasMember("volume") && 
                    document["audio"]["volume"].IsInt()) {
                    volume = document["audio"]["volume"].GetInt();
                    
                    if (volume < 0) volume = 0;
                    if (volume > 100) volume = 100;
                    
                    if (volumeSlider) {
                        volumeSlider->setValue(volume);
                        volumeLabel->setText(QString("Volume: %1%").arg(volume));
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
    }
}

bool MainWindow::copyLPCFolder() {

    std::string currentDir = QCoreApplication::applicationDirPath().toStdString();
    fs::path currentPath = fs::path(currentDir);
    fs::path sourceLPCPath = currentPath / "LPC";
    fs::path gameDir = currentPath.parent_path().parent_path();
    fs::path destLPCPath = gameDir / "LPC";

    if (!fs::exists(sourceLPCPath)) {
        return false;
    }

    return utils::copyDirectoryRecursive(sourceLPCPath.string(), destLPCPath.string());
}

void MainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::ActivationChange) {
        if (this->isActiveWindow()) {
            // Quand on revient sur le launcher : recharger le total depuis playtime.txt
            QLabel* lbl = this->findChild<QLabel*>("playtimeLabel");
            if (lbl) {
                long long total = 0;
                std::filesystem::path ptFile = std::filesystem::path(launcherDir) / "playtime.txt";
                { std::ifstream in(ptFile); if (in) in >> total; }
                long long h = total / 3600;
                long long m = (total % 3600) / 60;
                lbl->setText(QString("TEMPS DE JEU\n%1h %2min").arg(h).arg(m));
            }
        }
    }
    QMainWindow::changeEvent(event);
}
