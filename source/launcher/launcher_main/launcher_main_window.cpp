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

namespace fs = std::filesystem;

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

    setWindowTitle("AlterCOD Launcher");
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

    sideLayout->addStretch();

    // liens bas de sidebar : Discord + Docs
    discordButton = new QPushButton("  DISCORD", this);
    discordButton->setStyleSheet(sideBtn);
    sideLayout->addWidget(discordButton);

    wikiButton = new QPushButton("  SITE / DOCS", this);
    wikiButton->setStyleSheet(sideBtn);
    sideLayout->addWidget(wikiButton);

    rootLayout->addWidget(sidebar);

    // ----- ZONE CONTENU DROITE -----
    QWidget* content = new QWidget(this);
    content->setStyleSheet("background: transparent;");
    QVBoxLayout* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(30, 26, 30, 24);
    contentLayout->setSpacing(0);

    // statut serveur (statique, indicateur visuel)
    QLabel* statusLabel = new QLabel("\u25CF  CLIENT PRET", this);
    statusLabel->setStyleSheet("color: #39d98a; font-size: 12px; font-weight: bold; letter-spacing: 2px; background: transparent;");
    contentLayout->addWidget(statusLabel);

    contentLayout->addSpacing(10);

    QLabel* subTitle = new QLabel("BLACK OPS 4 \u00B7 CLIENT", this);
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
    QLabel* upToDate = new QLabel("\u00C0 JOUR", this);
    upToDate->setStyleSheet("color: #39d98a; border: 1px solid rgba(57,217,138,0.5); border-radius: 4px; padding: 4px 10px; font-size: 11px; font-weight: bold; letter-spacing: 1px; background: transparent;");
    badgeRow->addWidget(upToDate);
    badgeRow->addStretch();
    contentLayout->addLayout(badgeRow);

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

    onlineButton = new QPushButton("\u25B6  ONLINE", this);
    onlineButton->setStyleSheet(
        "QPushButton { background: #f2c411; color: #08080a; border: none; border-radius: 5px;"
        " padding: 15px; font-size: 17px; font-weight: bold; letter-spacing: 3px; }"
        "QPushButton:hover { background: #ffd633; }"
        "QPushButton:disabled { background: rgba(242,196,17,0.3); color: rgba(8,8,10,0.5); }");
    launchRow->addWidget(onlineButton, 2);

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
}


void MainWindow::startGame(bool isOnline, bool isVanilla) {

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

    QDesktopServices::openUrl(QUrl("https://leikaam.fr/AlterCOD"));
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
