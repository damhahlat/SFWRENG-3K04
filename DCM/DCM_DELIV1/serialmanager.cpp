#include "serialmanager.h"
#include <QSerialPortInfo>

SerialManager::SerialManager(QObject *parent)
    : QObject(parent)
{
    connect(&m_port, &QSerialPort::readyRead,
            this, &SerialManager::handleReadyRead);

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(&m_port, &QSerialPort::errorOccurred,
            this, &SerialManager::handleError);
#else
    connect(&m_port, static_cast<void (QSerialPort::*)(QSerialPort::SerialPortError)>(&QSerialPort::error),
            this, &SerialManager::handleError);
#endif
}

SerialManager::~SerialManager()
{
    if (m_port.isOpen()) {
        m_port.close();
    }
}

QStringList SerialManager::availablePorts() const
{
    QStringList ports;
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        ports << info.portName();
    }
    return ports;
}

bool SerialManager::openPort(const QString &portName, qint32 baudRate)
{
    if (m_port.isOpen()) {
        m_port.close();
    }

    m_port.setPortName(portName);
    m_port.setBaudRate(baudRate);
    m_port.setDataBits(QSerialPort::Data8);
    m_port.setParity(QSerialPort::NoParity);
    m_port.setStopBits(QSerialPort::OneStop);
    m_port.setFlowControl(QSerialPort::NoFlowControl);

    if (!m_port.open(QIODevice::ReadWrite)) {
        emit errorOccurred(QStringLiteral("Failed to open port %1: %2")
                               .arg(portName, m_port.errorString()));
        return false;
    }

    emit connected(portName, baudRate);
    return true;
}

void SerialManager::closePort()
{
    if (m_port.isOpen()) {
        m_port.close();
        emit disconnected();
    }
}

bool SerialManager::isOpen() const
{
    return m_port.isOpen();
}

qint32 SerialManager::currentBaudRate() const
{
    return m_port.baudRate();
}

QString SerialManager::currentPortName() const
{
    return m_port.portName();
}

void SerialManager::sendData(const QByteArray &data)
{
    if (!m_port.isOpen()) {
        emit errorOccurred(QStringLiteral("Port is not open"));
        return;
    }

    qint64 written = m_port.write(data);
    if (written == -1) {
        emit errorOccurred(QStringLiteral("Failed to write data: %1").arg(m_port.errorString()));
    }
}

void SerialManager::handleReadyRead()
{
    const QByteArray data = m_port.readAll();
    if (!data.isEmpty()) {
        emit dataReceived(data);
    }
}

void SerialManager::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError)
        return;

    emit errorOccurred(QStringLiteral("Serial error: %1").arg(m_port.errorString()));

    // You can decide whether to auto close on some errors
    // if (error == QSerialPort::ResourceError) {
    //     closePort();
    // }
}
