#include "serialtestdialog.h"
#include "ui_serialtestdialog.h"

#include <QTimer>
#include <QMessageBox>

// -------------------------------------------------------------
// Constructor
// -------------------------------------------------------------
SerialTestDialog::SerialTestDialog(QWidget* parent)
    : QDialog(parent),
    ui(new Ui::SerialTestDialog)
{
    ui->setupUi(this);

    // Connect UI buttons
    connect(ui->btnRefresh,  &QPushButton::clicked,
            this,            &SerialTestDialog::onRefreshPorts);
    connect(ui->btnConnect,  &QPushButton::clicked,
            this,            &SerialTestDialog::onConnectClicked);
    connect(ui->btnSend,     &QPushButton::clicked,
            this,            &SerialTestDialog::onSendClicked);

    // SerialManager error messages
    connect(&manager_, &SerialManager::errorOccurred,
            this,      &SerialTestDialog::onErrorMessage);

    // Initial UI state
    ui->lblStatus->setText("Disconnected");
    onRefreshPorts();

    // Poll for incoming bytes periodically and append to txtReceive
    QTimer* t = new QTimer(this);
    connect(t, &QTimer::timeout, [this]() {
        if (!manager_.isOpen())
            return;
        QByteArray data = manager_.readBytes();
        if (!data.isEmpty()) {
            ui->txtReceive->append(QString::fromUtf8(data));
        }
    });
    t->start(100);
}

// -------------------------------------------------------------
// Destructor
// -------------------------------------------------------------
SerialTestDialog::~SerialTestDialog()
{
    delete ui;
}

// -------------------------------------------------------------
// Refresh available ports
// -------------------------------------------------------------
void SerialTestDialog::onRefreshPorts()
{
    ui->cbPort->clear();
    ui->cbPort->addItems(manager_.availablePorts());

    if (ui->cbPort->count() == 0) {
        ui->lblStatus->setText("No COM ports found.");
    } else {
        ui->lblStatus->setText("Select a port and connect.");
    }
}

// -------------------------------------------------------------
// Connect / Disconnect
// -------------------------------------------------------------
void SerialTestDialog::onConnectClicked()
{
    if (!manager_.isOpen()) {
        // Attempt to connect
        QString portName = ui->cbPort->currentText();
        bool ok = false;
        int baud = ui->cbBaud->currentText().toInt(&ok);
        if (!ok || baud <= 0) {
            baud = 115200;
        }

        QString err;
        if (!manager_.openPort(portName, baud, &err)) {
            ui->lblStatus->setText("Failed to connect: " + err);
            return;
        }

        ui->lblStatus->setText(
            QString("Connected to %1 @ %2 baud").arg(portName).arg(baud));
        ui->btnConnect->setText("Disconnect");
    } else {
        // Disconnect
        manager_.closePort();
        ui->lblStatus->setText("Disconnected");
        ui->btnConnect->setText("Connect");
    }
}

// Dummy slot to satisfy the header; not used (no disconnect button in UI)
void SerialTestDialog::onDisconnectClicked()
{
    // Not used; kept only to match header and avoid linker errors.
}

// -------------------------------------------------------------
// Send data
// -------------------------------------------------------------
void SerialTestDialog::onSendClicked()
{
    if (!manager_.isOpen()) {
        ui->lblStatus->setText("Not connected.");
        return;
    }

    QString text = ui->txtSend->text();
    if (text.isEmpty()) {
        ui->lblStatus->setText("Nothing to send.");
        return;
    }

    if (!manager_.writeBytes(text.toUtf8())) {
        ui->lblStatus->setText("Write failed.");
        return;
    }

    ui->lblStatus->setText("Sent.");
}

// -------------------------------------------------------------
// Error handling
// -------------------------------------------------------------
void SerialTestDialog::onErrorMessage(const QString& msg)
{
    ui->lblStatus->setText("Error: " + msg);
}
