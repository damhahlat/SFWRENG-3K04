#pragma once

#include <QMainWindow>
#include <QString>

class ParameterForm;
class EgramWidget;
class SerialManager;
class SerialTestDialog;

namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(int userId, const QString& username, QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    // File menu actions
    void onNewPatient();
    void onSetClock();
    void onExportBradyParams();
    void onExportTemporaryParams();
    void onQuit();

    // Help menu
    void onAbout();

    // Tools / buttons
    void onOpenDbFolder();

    // Tools → Serial Test... (actionSerialTest in mainwindow.ui)
    void on_actionSerialTest_triggered();

    // Egram tab buttons (startBtn / stopBtn in mainwindow.ui)
    void on_startBtn_clicked();   // send parameters to device
    void on_stopBtn_clicked();    // close serial port

private:
    Ui::MainWindow* ui;
    int     userId_;
    QString username_;

    ParameterForm* form_{nullptr};
    EgramWidget*   egram_{nullptr};
    SerialManager* serial_{nullptr};

    QString lastClockSet_;  // most recent "device clock" time

    // Metadata used in About/report generation
    QString appModel() const { return "DCM-APP-001"; }
    QString appVersion() const { return "1.0.0"; }
    QString dcmSerial() const { return "DCM-SN-0001"; }
    QString institution() const { return "McMaster University"; }

    void buildMenus();
    QString buildReportHtml(const QString& reportName) const;
};
