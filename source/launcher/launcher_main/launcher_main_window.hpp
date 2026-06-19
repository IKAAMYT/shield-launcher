#pragma once

#include "std_include.hpp"
#include <string>
#include <Windows.h>
#include <QEvent>
#include <QComboBox>
#include <QVBoxLayout>
#include <ctime>
#pragma comment(lib, "winmm.lib")

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:

    void startGame(bool isOnline, bool isVanilla = false);
    void setName();
    void setIp();
    void openDiscord();
    void openDocs();
    void updateVolume(int value);
    void openSettings();

protected:
    void changeEvent(QEvent* event) override;

private:

    void showMessageBox(QMessageBox::Icon icon, const QString& title, const QString& text);
    bool copyLPCFolder();
    void playStartupSound();
    void saveCloseLauncheronPlay();
    void loadCloseLauncheronPlay();
    void saveVolumeSettings();
    void loadVolumeSettings();
    
    std::wstring launcherDir;
    std::wstring serverIpFile;
    bool reshadeEnabled;
	bool closeLauncherOnPlay;
    int volume;
    std::wstring soundPath;

    //gui
    QPushButton* vanillaButton;
    QPushButton* onlineButton;
    QPushButton* offlineButton;
    QPushButton* nameButton;
    QPushButton* ipButton;
    QPushButton* discordButton;
    QPushButton* wikiButton;
    QPushButton* settingsButton;
    QCheckBox* closeLauncherCheckbox;
    QCheckBox* reshadeCheckbox;
    QLabel* volumeLabel;
    QSlider* volumeSlider;
    QProgressBar* progressBar;
    time_t gameStartTime = 0;
    bool gameRunning = false;
    void addPlaytimeAndRefresh();
    void saveServerIp(const std::string& ip);
    void refreshServerInfo();
    QLabel* serverStatusLabel = nullptr;
    QWidget* lobbyContainer = nullptr;
    QVBoxLayout* lobbyLayout = nullptr;
    QComboBox* serverCombo = nullptr;
    QLabel* newsLabel = nullptr;
};