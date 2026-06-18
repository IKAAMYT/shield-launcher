#include "std_include.hpp"
#include "style.hpp"

//me love qt due to using css for design :D
// === AlterCOD theme (IKAAM) : fond noir #0a0a08 / accent or #f2c411 ===
namespace Style {

    QString getLightStyleSheet() {
        return QString(R"(
            QMainWindow, QDialog { background-color: #f4f3ee; color: #1a1a14; }
            QLabel { color: #1a1a14; }
            QLineEdit {
                background-color: #ffffff; color: #1a1a14;
                border: 1px solid #f2c411; padding: 4px; border-radius: 4px;
            }
            QPushButton {
                background-color: #ffffff; color: #1a1a14;
                border: 1px solid #f2c411; border-radius: 4px;
                padding: 6px 12px; min-width: 80px;
            }
            QPushButton:hover { background-color: #f2c411; color: #0a0a08; }
            QPushButton:pressed { background-color: #d9af0e; color: #0a0a08; }
            QComboBox {
                background-color: #ffffff; color: #1a1a14;
                border: 1px solid #f2c411; border-radius: 4px; padding: 4px;
            }
            QComboBox::drop-down { border: none; }
            QComboBox::down-arrow { background-color: #f2c411; width: 12px; height: 12px; }
            QComboBox QAbstractItemView {
                background-color: #ffffff; color: #1a1a14;
                selection-background-color: #f2c411; selection-color: #0a0a08;
            }
            QSlider::groove:horizontal {
                border: 1px solid #f2c411; height: 8px; background: #e8e6dc;
                margin: 2px 0; border-radius: 4px;
            }
            QSlider::handle:horizontal {
                background: #f2c411; border: 1px solid #d9af0e;
                width: 18px; margin: -2px 0; border-radius: 9px;
            }
            QSlider::handle:horizontal:hover { background: #ffd633; }
            QCheckBox { color: #1a1a14; }
            QCheckBox::indicator { width: 16px; height: 16px; }
            QCheckBox::indicator:unchecked {
                background-color: #ffffff; border: 1px solid #f2c411; border-radius: 3px;
            }
            QCheckBox::indicator:checked {
                background-color: #f2c411; border: 1px solid #d9af0e; border-radius: 3px;
            }
            QProgressBar {
                border: 1px solid #f2c411; border-radius: 4px;
                background-color: #ffffff; color: #1a1a14; text-align: center;
            }
            QProgressBar::chunk { background-color: #f2c411; border-radius: 3px; }
            QMessageBox { background-color: #f4f3ee; color: #1a1a14; }
            QMessageBox QLabel { color: #1a1a14; }
        )");
    }

    QString getStyleSheet() { return getStyleSheet(true); }

    QString getStyleSheet(bool darkMode) {
        return darkMode ? getDarkStyleSheet() : getLightStyleSheet();
    }

    QString getDarkStyleSheet() {
        // === AlterCOD dark : noir profond + or #f2c411 ===
        return QString(R"(
            QMainWindow { background-color: transparent; }
            QWidget { background-color: transparent; color: #f5f3ea; }
            QPushButton {
                background-color: rgba(10, 10, 8, 200); color: #f2c411;
                border: 1px solid #f2c411; border-radius: 4px;
                padding: 5px; min-width: 80px; font-weight: bold;
            }
            QPushButton:hover {
                background-color: #f2c411; color: #0a0a08; border: 1px solid #ffd633;
            }
            QPushButton:pressed {
                background-color: #d9af0e; color: #0a0a08; border: 1px solid #d9af0e;
            }
            QLabel { color: #f5f3ea; }
            QLineEdit {
                background-color: rgba(10, 10, 8, 200); color: #f5f3ea;
                border: 1px solid #f2c411; border-radius: 4px; padding: 5px;
            }
            QProgressBar {
                border: 1px solid #f2c411; border-radius: 4px; text-align: center;
                background: rgba(10, 10, 8, 160); color: #f5f3ea;
            }
            QProgressBar::chunk { background-color: #f2c411; border-radius: 3px; }
            QSlider::groove:horizontal {
                border: 1px solid #f2c411; height: 8px;
                background: rgba(10, 10, 8, 200); margin: 2px 0; border-radius: 4px;
            }
            QSlider::sub-page:horizontal {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #d9af0e, stop:1 #f2c411);
                border-radius: 4px;
            }
            QSlider::handle:horizontal {
                background: #f2c411; border: 1px solid #ffd633;
                width: 18px; margin: -2px 0; border-radius: 9px;
            }
            QSlider::handle:horizontal:hover { background: #ffd633; }
            QCheckBox { color: #f5f3ea; spacing: 5px; }
            QCheckBox::indicator { width: 15px; height: 15px; }
            QCheckBox::indicator:unchecked {
                border: 1px solid #f2c411; background: rgba(10, 10, 8, 200); border-radius: 3px;
            }
            QCheckBox::indicator:checked {
                border: 1px solid #ffd633; background: #f2c411; border-radius: 3px;
            }
            QComboBox {
                background-color: rgba(10, 10, 8, 200); color: #f5f3ea;
                border: 1px solid #f2c411; border-radius: 4px; padding: 5px;
            }
            QComboBox::drop-down { border: none; }
            QComboBox::down-arrow { background-color: #f2c411; width: 12px; height: 12px; }
            QComboBox QAbstractItemView {
                background-color: rgba(20, 20, 16, 235); color: #f5f3ea;
                border: 1px solid #f2c411;
                selection-background-color: #f2c411; selection-color: #0a0a08;
            }
            QMessageBox { background-color: rgba(20, 20, 16, 235); color: #f5f3ea; }
            QMessageBox QLabel { color: #f5f3ea; }
            QDialog { background-color: rgba(20, 20, 16, 235); color: #f5f3ea; }
        )");
    }
}
