#include "serialtestdialog.h"
#include "ui_serialtestdialog.h"

#include "serialmanager.h"

#include <QMessageBox>
#include <QDateTime>

SerialTestDialog::SerialTestDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SerialTestDialog)
    , m_serial(new SerialManager(this))
    , m_connected(false)
{
    ui->setupUi(this);
    setupBaudRates();
    populatePorts();

    connect(m_serial, &SerialManager::dataReceived,
            this, &SerialTestDialog::handleDataReceived);
    connect(m_serial, &SerialManager::errorOccurred,
            this, &SerialTestDialog::handleError);
    connect(m_serial, &SerialManager::connected,
            this, &SerialTestDialog::handleConnected);
    connect(m_serial, &SerialManager::disconnected,
            this, &SerialTestDialog::handleDisconnected);
}

SerialTestDialog::~SerialTestDialog()
{
    delete ui;
}

void SerialTestDialog::setupBaudRates()
{
    ui->cbBaud->clear();
    QList<int> bauds = {9600, 19200, 38400, 57600, 115200};
    for (int b : bauds) {
        ui->cbBaud->addItem(QString::number(b), b);
    }
    int index = ui->cbBaud->findData(115200);
    if (index >= 0) {
        ui->cbBaud->setCurrentIndex(index);
    }
}

void SerialTestDialog::populatePorts()
{
    ui->cbPort->clear();
    const QStringList ports = m_serial->availablePorts();
    ui->cbPort->addItems(ports);
}

void SerialTestDialog::on_btnRefresh_clicked()
{
    populatePorts();
}

void SerialTestDialog::on_btnConnect_clicked()
{
    if (!m_connected) {
        const QString portName = ui->cbPort->currentText();
        if (portName.isEmpty()) {
            QMessageBox::warning(this, tr("Serial"), tr("No port selected."));
            return;
        }

        bool ok = false;
        qint32 baud = ui->cbBaud->currentData().toInt(&ok);
        if (!ok) {
            baud = 115200;
        }

        if (!m_serial->openPort(portName, baud)) {
            // error message already sent via signal
            return;
        }
        // handleConnected slot will be called
    } else {
        m_serial->closePort();
        // handleDisconnected slot will be called
    }
}

void SerialTestDialog::on_btnSend_clicked()
{
    const QString text = ui->txtSend->text();
    if (text.isEmpty())
        return;

    QByteArray data = text.toUtf8();
    data.append('\n');  // simple newline protocol
    m_serial->sendData(data);

    const QString ts = QDateTime::currentDateTime().toString("HH:mm:ss");
    ui->txtReceive->append(QStringLiteral("[%1] Sent: %2").arg(ts, text));

    ui->txtSend->clear();
}

void SerialTestDialog::handleDataReceived(const QByteArray &data)
{
    const QString ts = QDateTime::currentDateTime().toString("HH:mm:ss");
    QString str = QString::fromUtf8(data);
    // Remove any trailing newlines just for display cleanliness
    str.replace('\r', ' ');
    str.replace('\n', ' ');
    ui->txtReceive->append(QStringLiteral("[%1] Received: %2").arg(ts, str));
}

void SerialTestDialog::handleError(const QString &message)
{
    const QString ts = QDateTime::currentDateTime().toString("HH:mm:ss");
    ui->txtReceive->append(QStringLiteral("[%1] Error: %2").arg(ts, message));
    ui->lblStatus->setText(QStringLiteral("Status: Error - %1").arg(message));
}

void SerialTestDialog::handleConnected(const QString &portName, qint32 baudRate)
{
    m_connected = true;
    ui->btnConnect->setText(tr("Disconnect"));
    ui->lblStatus->setText(QStringLiteral("Status: Connected to %1 at %2 baud")
                               .arg(portName)
                               .arg(baudRate));
}

void SerialTestDialog::handleDisconnected()
{
    m_connected = false;
    ui->btnConnect->setText(tr("Connect"));
    ui->lblStatus->setText(QStringLiteral("Status: Disconnected"));
}
