#pragma once

// Include Qt's MainWindow library, which will be built upon.
#include <QMainWindow>

// Include the serial test class we wrote
#include "serialtestdialog.h"

// Declarations for the parameter form and the egram, which is present in D1 but hasn't been implemented yet.
class ParameterForm;
class EgramWidget;

// Store the MainWindow class in the UI namespace, which is another Generative AI suggestion.
namespace Ui { class MainWindow; }

// Implementation of the MainWindow class which is built upon Qt's own MainWindow class.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(int userId, const QString& username, QWidget* parent = nullptr);
    ~MainWindow() override; // Override Qt's own destructor in favour of ours, which is in the cpp file.

// These are options that are present in the browser at the top of the design.
private slots:

    // Serial testing
    void on_actionSerialTest_triggered();

    // File menu
    void onNewPatient();
    void onSetClock();
    void onExportBradyParams();
    void onExportTemporaryParams();
    void onQuit();

    // Help
    void onAbout();
    void onOpenDbFolder();

// Bunch of information that is or will eventually displayed on the GUI.
private:
    Ui::MainWindow* ui;
    int userId_{-1};
    QString username_;
    ParameterForm* form_{nullptr};
    EgramWidget* egram_{nullptr};
    QString lastClockSet_;  // store most recent device time

    // Generative AI said having these in the about page is good practice for software design.
    QString appModel() const { return "DCM-APP-001"; }
    QString appVersion() const { return "1.0.0"; }
    QString dcmSerial() const { return "DCM-SN-0001"; }
    QString institution() const { return "McMaster University"; }

    void buildMenus();

    // These functions take the mode profiles and generate reports in HTML files.
    QString buildReportHtml(const QString& reportName) const;
    bool exportHtmlToPdf(const QString& html, const QString& defaultName);
};
