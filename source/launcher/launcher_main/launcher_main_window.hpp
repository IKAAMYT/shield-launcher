#pragma once

#include "std_include.hpp"
#include <string>
#include <Windows.h>
#include <QSystemTrayIcon>
#include <QCloseEvent>
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
    void closeEvent(QCloseEvent* event) override;

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
    QSystemTrayIcon* trayIcon;
};