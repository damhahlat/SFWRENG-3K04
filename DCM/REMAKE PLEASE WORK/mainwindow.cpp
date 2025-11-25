#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "database.h"
#include "parameterform.h"
#include "serialmanager.h"
#include "serialtestdialog.h"

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
#include <QStatusBar>
#include <QLabel>

// ------------------------------------------------------------------
// Simple dummy Egram widget (visual only, no real signal yet).
// ------------------------------------------------------------------
class EgramWidget : public QWidget {
    Q_OBJECT
public:
    explicit EgramWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setMinimumHeight(200);
        timer_ = new QTimer(this);
        connect(timer_, &QTimer::timeout, this, [this]() {
            phase_ += 0.15;
            if (phase_ > 10000.0)
                phase_ = 0.0;
            update();
        });
        timer_->start(30);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.fillRect(rect(), Qt::white);
        p.setRenderHint(QPainter::Antialiasing, true);

        // Baseline
        p.setPen(QPen(Qt::gray, 1, Qt::DashLine));
        p.drawLine(10, height() / 2, width() - 10, height() / 2);

        // Fake waveform
        p.setPen(QPen(Qt::blue, 2));
        QPainterPath path;
        path.moveTo(10, height() / 2);
        for (int x = 0; x < width() - 20; ++x) {
            double t = (phase_ + x) / 20.0;
            double v = std::sin(t) * 30.0 + std::sin(t * 0.23) * 5.0;
            path.lineTo(10 + x, height() / 2 - v);
        }
        p.drawPath(path);
    }

private:
    QTimer* timer_{nullptr};
    double  phase_{0.0};
};

#include "mainwindow.moc"

// ------------------------------------------------------------------
// MainWindow
// ------------------------------------------------------------------

MainWindow::MainWindow(int userId, const QString& username, QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , userId_(userId)
    , username_(username)
{
    ui->setupUi(this);

    // Set basic labels
    setWindowTitle(QString("DCM — User: %1").arg(username_));
    ui->usernameLabel->setText(username_);
    ui->dbPathLabel->setText(Database::path());
    ui->clockLabel->setText("Device Clock: (not set)");

    // Parameter page
    form_ = new ParameterForm(userId_, this);
    if (auto* layout = ui->parametersPage->findChild<QVBoxLayout*>("parametersLayout")) {
        layout->insertWidget(1, form_);
    } else {
        form_->setParent(ui->parametersPage);
        form_->show();
    }
    connect(form_, &ParameterForm::statusMessage,
            ui->paramStatus, &QLabel::setText);
    ui->paramStatus->setText("Ready.");

    // Egram page
    if (auto* eLayout = ui->egramPage->findChild<QVBoxLayout*>("egramLayout")) {
        egram_ = new EgramWidget(this);
        eLayout->insertWidget(0, egram_);
    }

    // Open DB folder button on About tab
    connect(ui->openDbFolderBtn, &QPushButton::clicked,
            this, &MainWindow::onOpenDbFolder);

    // Serial manager
    serial_ = new SerialManager(this);

    // Build File / Help menus (Tools menu is from .ui)
    buildMenus();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ------------------------------------------------------------------
// Menus & helpers
// ------------------------------------------------------------------

void MainWindow::buildMenus()
{
    // File
    auto fileMenu = menuBar()->addMenu("&File");
    auto actNew   = fileMenu->addAction("New Patient");
    auto actClock = fileMenu->addAction("Set Clock...");
    auto actBrady = fileMenu->addAction("Export Bradycardia Parameters (HTML)...");
    auto actTemp  = fileMenu->addAction("Export Temporary Parameters (HTML)...");
    fileMenu->addSeparator();
    auto actQuit  = fileMenu->addAction("Quit");

    connect(actNew,   &QAction::triggered, this, &MainWindow::onNewPatient);
    connect(actClock, &QAction::triggered, this, &MainWindow::onSetClock);
    connect(actBrady, &QAction::triggered, this, &MainWindow::onExportBradyParams);
    connect(actTemp,  &QAction::triggered, this, &MainWindow::onExportTemporaryParams);
    connect(actQuit,  &QAction::triggered, this, &MainWindow::onQuit);

    // Help
    auto helpMenu = menuBar()->addMenu("&Help");
    auto actAbout = helpMenu->addAction("About");
    connect(actAbout, &QAction::triggered, this, &MainWindow::onAbout);
}

// Build a simple HTML report from current parameters
QString MainWindow::buildReportHtml(const QString& reportName) const
{
    QMap<QString, QString> kv;
    if (form_)
        kv = form_->currentValuesAsText();

    QString html;
    html += "<html><head><meta charset=\"utf-8\"/>";
    html += "<style>body{font-family:Arial;} table{border-collapse:collapse;} ";
    html += "td,th{border:1px solid #ccc;padding:4px;}</style></head><body>";
    html += QString("<h1>%1</h1>").arg(reportName);
    html += QString("<p>Institution: %1<br/>Model: %2<br/>App Version: %3<br/>"
                    "DCM Serial: %4<br/>DB Path: %5</p>")
                .arg(institution(),
                     appModel(),
                     appVersion(),
                     dcmSerial(),
                     Database::path());

    html += "<table><tr><th>Parameter</th><th>Value</th></tr>";
    for (auto it = kv.cbegin(); it != kv.cend(); ++it) {
        html += QString("<tr><td>%1</td><td>%2</td></tr>")
        .arg(it.key(), it.value());
    }
    html += "</table></body></html>";
    return html;
}

// ------------------------------------------------------------------
// File menu slots
// ------------------------------------------------------------------

void MainWindow::onNewPatient()
{
    if (form_)
        form_->clearAll();
    statusBar()->showMessage("Started new patient session.", 3000);
}

void MainWindow::onSetClock()
{
    QDateTime dt = QDateTime::currentDateTime();
    bool ok = false;
    QString text = QInputDialog::getText(this, "Set Clock",
                                         "Enter device time (yyyy-MM-dd hh:mm:ss), "
                                         "or leave as-is for current:",
                                         QLineEdit::Normal,
                                         dt.toString("yyyy-MM-dd hh:mm:ss"),
                                         &ok);
    if (ok && !text.trimmed().isEmpty()) {
        auto parsed = QDateTime::fromString(text.trimmed(), "yyyy-MM-dd hh:mm:ss");
        if (parsed.isValid())
            dt = parsed;
    }

    lastClockSet_ = dt.toString(Qt::ISODate);
    ui->clockLabel->setText("Device Clock: " + lastClockSet_);
    statusBar()->showMessage("Clock set: " + lastClockSet_, 3000);
}

void MainWindow::onExportBradyParams()
{
    const QString html = buildReportHtml("Bradycardia Parameters Report");
    const QString out = QFileDialog::getSaveFileName(
        this, "Export Brady Parameters", "brady_params.html", "HTML Files (*.html)");
    if (out.isEmpty())
        return;

    QFile f(out);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Export", "Cannot write file.");
        return;
    }
    QTextStream ts(&f);
    ts << html;
    f.close();

    QDesktopServices::openUrl(QUrl::fromLocalFile(out));
    statusBar()->showMessage("Brady report exported.", 3000);
}

void MainWindow::onExportTemporaryParams()
{
    const QString html = buildReportHtml("Temporary Parameters Report");
    const QString out = QFileDialog::getSaveFileName(
        this, "Export Temporary Parameters", "temporary_params.html", "HTML Files (*.html)");
    if (out.isEmpty())
        return;

    QFile f(out);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Export", "Cannot write file.");
        return;
    }
    QTextStream ts(&f);
    ts << html;
    f.close();

    QDesktopServices::openUrl(QUrl::fromLocalFile(out));
    statusBar()->showMessage("Temporary report exported.", 3000);
}

void MainWindow::onQuit()
{
    close();
}

// ------------------------------------------------------------------
// Help
// ------------------------------------------------------------------

void MainWindow::onAbout()
{
    const QString msg = QString(
                            "DCM Application\n"
                            "Institution: %1\n"
                            "Model: %2\n"
                            "Version: %3\n"
                            "DCM Serial: %4\n"
                            "Database Path: %5\n")
                            .arg(institution(),
                                 appModel(),
                                 appVersion(),
                                 dcmSerial(),
                                 Database::path());
    QMessageBox::information(this, "About DCM", msg);
}

// ------------------------------------------------------------------
// Tools / DB folder
// ------------------------------------------------------------------

void MainWindow::onOpenDbFolder()
{
    const QString folder = QFileInfo(Database::path()).absolutePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
}

// Tools → Serial Test... (simple passthrough to existing dialog)
void MainWindow::on_actionSerialTest_triggered()
{
    SerialTestDialog dlg(this);
    dlg.exec();
}

// ------------------------------------------------------------------
// SERIAL COMMUNICATION: SEND PARAMETERS
// ------------------------------------------------------------------

// Start button: build parameter frame and send over serial
void MainWindow::on_startBtn_clicked()
{
    if (!form_) {
        QMessageBox::warning(this, "Send", "Parameter form not available.");
        return;
    }

    QByteArray frame;
    QString err;
    if (!form_->buildTxFrame(&frame, &err)) {
        QMessageBox::warning(this, "Invalid Parameters", err);
        return;
    }

    // Ensure serial is open; if not, ask for a port
    if (!serial_->isOpen()) {
        QStringList ports = serial_->availablePorts();
        if (ports.isEmpty()) {
            QMessageBox::warning(this, "Serial", "No serial ports available.");
            return;
        }

        bool ok = false;
        QString port = QInputDialog::getItem(
            this, "Select Serial Port", "Port:", ports, 0, false, &ok);
        if (!ok || port.isEmpty())
            return;

        QString serr;
        if (!serial_->openPort(port, 115200, &serr)) {
            QMessageBox::warning(this, "Serial",
                                 "Failed to open " + port + ": " + serr);
            return;
        }
    }

    if (!serial_->writeBytes(frame)) {
        QMessageBox::warning(this, "Serial", "Failed to write to serial port.");
        return;
    }

    ui->paramStatus->setText("Frame sent to pacemaker.");
    statusBar()->showMessage("Parameters sent to device.", 3000);
}

// Stop button: just closes the serial port for now
void MainWindow::on_stopBtn_clicked()
{
    if (serial_->isOpen())
        serial_->closePort();

    statusBar()->showMessage("Serial port closed.", 3000);
}
