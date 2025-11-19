// Include necessary header files and libraries
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "database.h"
#include "parameterform.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QUrl>
#include <QPushButton>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QAction>
#include <QDateTime>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QTimer>
#include <QPainter>
#include <QPainterPath>

// The egram hasn't been implemented yet since that's a D2 project, but we still have it present with dummy values running.
// The dummy graph was implemented using Generative AI (paintEvent).
class EgramWidget : public QWidget {
public:
    explicit EgramWidget(QWidget* parent=nullptr): QWidget(parent) {
        setMinimumHeight(200);
        timer_ = new QTimer(this);
        connect(timer_, &QTimer::timeout, this, [this](){
            phase_ += 0.15;
            if (phase_ > 10000) phase_ = 0;
            update();
        });
        timer_->start(30);
    }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.fillRect(rect(), Qt::white);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(QPen(Qt::gray, 1, Qt::DashLine));
        p.drawLine(10, height()/2, width()-10, height()/2);

        p.setPen(QPen(Qt::black, 2));
        const int L = width()-20;
        QPainterPath path(QPointF(10, height()/2));
        for (int i=0;i<=L;++i){
            double t = (phase_ + i)/20.0;
            double v = std::sin(t)*30.0 + (std::sin(t*0.23)*5.0);
            path.lineTo(10+i, height()/2 - v);
        }
        p.drawPath(path);
    }
private:
    QTimer* timer_{nullptr};
    double phase_{0.0};
};

// The main window constructor.
// This was coded manually, but due to complexity Generative AI helped guide our process.
MainWindow::MainWindow(int userId, const QString& username, QWidget* parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    userId_(userId),
    username_(username)
{
    ui->setupUi(this);

    // Write in GUI elements based on user data.
    setWindowTitle(QString("DCM — User: %1").arg(username_));
    if (auto l = ui->usernameLabel) l->setText(username_);
    if (auto l = ui->dbPathLabel)   l->setText(Database::path());
    if (auto l = ui->paramHeader)   l->setText("Parameters (AOO, VOO, AAI, VVI)");
    if (auto l = ui->paramStatus)   l->setText(" ");

    connect(ui->openDbFolderBtn, &QPushButton::clicked,
            this, &MainWindow::onOpenDbFolder);

    form_ = new ParameterForm(userId_, this);
    if (auto* layout = ui->parametersPage->findChild<QVBoxLayout*>("parametersLayout")) {
        layout->insertWidget(1, form_);
    } else {
        form_->setParent(ui->parametersPage);
        form_->show();
    }
    connect(form_, &ParameterForm::statusMessage, ui->paramStatus, &QLabel::setText);
    ui->paramStatus->setText("Ready.");

    if (auto* egramLayout = ui->egramPage->findChild<QVBoxLayout*>("egramLayout")) {
        egram_ = new EgramWidget(this);
        egramLayout->addWidget(egram_);
    }

    buildMenus();
}

// Destructor.
MainWindow::~MainWindow() { delete ui; }

// Builds the menus at the top of the program. Generative AI helped with this process due to unfamiliarity with the system.
void MainWindow::buildMenus() {
    auto fileMenu = menuBar()->addMenu("&File");
    auto actNew   = fileMenu->addAction("New Patient");
    auto actClock = fileMenu->addAction("Set Clock...");
    auto actBrady = fileMenu->addAction("Export Bradycardia Parameters (HTML)...");
    auto actTemp  = fileMenu->addAction("Export Temporary Parameters (HTML)...");
    fileMenu->addSeparator();
    auto actQuit  = fileMenu->addAction("Quit");

    connect(actNew,  &QAction::triggered, this, &MainWindow::onNewPatient);
    connect(actClock,&QAction::triggered, this, &MainWindow::onSetClock);
    connect(actBrady,&QAction::triggered, this, &MainWindow::onExportBradyParams);
    connect(actTemp, &QAction::triggered, this, &MainWindow::onExportTemporaryParams);
    connect(actQuit, &QAction::triggered, this, &MainWindow::onQuit);

    auto helpMenu = menuBar()->addMenu("&Help");
    auto actAbout = helpMenu->addAction("About");
    connect(actAbout, &QAction::triggered, this, &MainWindow::onAbout);
}

// Just clears the session.
void MainWindow::onNewPatient() {
    if (form_) form_->clearAll();
    statusBar()->showMessage("Started new patient session.", 3000);
}

// Either takes current device time, or uses user input to set the clock.
void MainWindow::onSetClock() {
    QDateTime dt = QDateTime::currentDateTime();
    bool ok = false;
    QString text = QInputDialog::getText(this, "Set Clock",
                                         "Device date/time (leave blank for now):\nYYYY-MM-DD hh:mm:ss",
                                         QLineEdit::Normal, "", &ok);
    if (ok && !text.trimmed().isEmpty()) {
        auto parsed = QDateTime::fromString(text.trimmed(), "yyyy-MM-dd hh:mm:ss");
        if (parsed.isValid()) dt = parsed;
    }

    lastClockSet_ = dt.toString(Qt::ISODate);

    // Update About tab if label exists
    if (auto label = ui->aboutPage->findChild<QLabel*>("clockLabel"))
        label->setText("Device Clock: " + lastClockSet_);
    else
        QMessageBox::information(this, "Set Clock", "Device clock set to: " + lastClockSet_);
}

// This takes the current data in the parameter form and stores it as an HTML file.
// Due to unfamiliarity with the system, a large portion of this was implemented through Generative AI.
QString MainWindow::buildReportHtml(const QString& reportName) const {
    QMap<QString, QString> kv;
    if (form_) kv = form_->currentValuesAsText();

    auto html = QString(
                    "<html><head><meta charset='utf-8'>"
                    "<style>"
                    "body{font-family:'Segoe UI',sans-serif;}"
                    "h1{font-size:18px;margin:0 0 8px 0;}"
                    "table{border-collapse:collapse;width:100%%;}"
                    "td,th{border:1px solid #ccc;padding:6px;}"
                    ".hdr td{border:none;padding:2px 0;}"
                    "</style></head><body>"
                    "<h1>%1</h1>"
                    "<table class='hdr'>"
                    "<tr><td><b>Institution</b></td><td>%2</td></tr>"
                    "<tr><td><b>Print Date/Time</b></td><td>%3</td></tr>"
                    "<tr><td><b>Device Model</b></td><td>PG-001</td></tr>"
                    "<tr><td><b>Device Serial</b></td><td>PG-SN-0001</td></tr>"
                    "<tr><td><b>DCM Serial</b></td><td>%4</td></tr>"
                    "<tr><td><b>Application Model</b></td><td>%5</td></tr>"
                    "<tr><td><b>Application Version</b></td><td>%6</td></tr>"
                    "<tr><td><b>Report</b></td><td>%1</td></tr>"
                    "</table><br/>"
                    "<table>"
                    "<tr><th>Parameter</th><th>Value</th></tr>")
                    .arg(reportName)
                    .arg(institution())
                    .arg(QDateTime::currentDateTime().toString(Qt::ISODate))
                    .arg(dcmSerial())
                    .arg(appModel())
                    .arg(appVersion());

    for (auto it = kv.cbegin(); it != kv.cend(); ++it) {
        html += QString("<tr><td>%1</td><td>%2</td></tr>").arg(it.key(), it.value());
    }
    html += "</table></body></html>";
    return html;
}

// Thsi function saves the HTML file but it also opens it using whatever program the user wants.
// Again, due to unfamiliarity with this system Generative AI was extensively used.
static bool saveHtmlAndOpen(QWidget* parent, const QString& html, const QString& suggestedName) {
    const QString out = QFileDialog::getSaveFileName(parent, "Export HTML", suggestedName, "HTML Files (*.html)");
    if (out.isEmpty()) return false;

    QFile f(out);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(parent, "Export", "Cannot write file.");
        return false;
    }
    QTextStream ts(&f);
    ts << html;
    f.close();

    QDesktopServices::openUrl(QUrl::fromLocalFile(out));
    QMessageBox::information(parent, "Export", "Saved: " + out);
    return true;
}

// If the user wants to export the Bradycardia Parameters
void MainWindow::onExportBradyParams() {
    saveHtmlAndOpen(this, buildReportHtml("Bradycardia Parameters Report"), "brady_params.html");
}

// If the user wants to export the Temporary Parameters
void MainWindow::onExportTemporaryParams() {
    saveHtmlAndOpen(this, buildReportHtml("Temporary Parameters Report"), "temporary_params.html");
}

// If the user selects quitting from the menu.
void MainWindow::onQuit() { close(); }

// If the user clicks on about from the menu.
void MainWindow::onAbout() {
    QString msg =
        QString("Institution: %1\nApplication Model: %2\nVersion: %3\nDCM Serial: %4\nDatabase: %5")
            .arg(institution(), appModel(), appVersion(), dcmSerial(), Database::path());
    QMessageBox::information(this, "About DCM", msg);
}

// This is primarily for debugging purposes. Users should not have access to the database for... obvious reason.
void MainWindow::onOpenDbFolder() {
    const QString folder = QFileInfo(Database::path()).absolutePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
}

void MainWindow::on_actionSerialTest_triggered() {
    SerialTestDialog dlg(this);
    dlg.exec();
}
