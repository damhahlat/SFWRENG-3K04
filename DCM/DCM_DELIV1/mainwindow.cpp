#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "database.h"
#include "parameterform.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QUrl>
#include <QPushButton>
#include <QVBoxLayout>

MainWindow::MainWindow(int userId, const QString& username, QWidget* parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    userId_(userId),
    username_(username)
{
    ui->setupUi(this);

    // App styling that won't fight AutoUIC
    setStyleSheet(R"(
    QTabWidget::pane {
        border: 1px solid rgba(0,0,0,35);
        border-radius: 10px;
        margin-top: 6px;
    }
    QTabBar::tab {
        color: black;
        padding: 8px 16px;
        margin-right: 4px;
        border: 1px solid rgba(0,0,0,20);
        border-bottom: none;
        border-top-left-radius: 8px;
        border-top-right-radius: 8px;
        background: transparent;
    }
    QTabBar::tab:selected { background: transparent; }

    QPushButton {
        padding: 6px 12px;
        border: 1px solid rgba(0,0,0,25);
        border-radius: 8px;
        background: palette(button);
    }
    QPushButton:hover { border-color: rgba(0,0,0,45); }

    #paramHeader, #egramHeader, #aboutHeader {
        font-weight: 600;
        font-size: 14pt;
    }
    #paramStatus { color: #666; }
    )");

    // About tab info
    setWindowTitle(QString("DCM — User: %1").arg(username_));
    ui->usernameLabel->setText(username_);
    ui->dbPathLabel->setText(Database::path());
    ui->paramHeader->setText("Parameters (AOO, VOO, AAI, VVI)");
    ui->paramStatus->setText(" ");

    connect(ui->openDbFolderBtn, &QPushButton::clicked,
            this, &MainWindow::onOpenDbFolder);

    // Parameters tab: embed the form
    auto* form = new ParameterForm(userId_, this);

    if (auto* paramsLayout = ui->parametersPage->findChild<QVBoxLayout*>("parametersLayout")) {
        paramsLayout->insertWidget(1, form);  // between header and status label
    }

    connect(form, &ParameterForm::statusMessage,
            ui->paramStatus, &QLabel::setText);

    ui->paramStatus->setText("Ready.");
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::onOpenDbFolder() {
    const QString folder = QFileInfo(Database::path()).absolutePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
}
