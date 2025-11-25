#include "serialmanager.h"
#include <QSerialPortInfo>
#include <QDebug>

SerialManager::SerialManager(QObject* parent)
    : QObject(parent)
{
    connect(&port_, &QSerialPort::readyRead,
            this,   &SerialManager::onReadyRead);

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(&port_, &QSerialPort::errorOccurred,
            this,   &SerialManager::onError);
#else
    connect(&port_,
            static_cast<void(QSerialPort::*)(QSerialPort::SerialPortError)>(&QSerialPort::error),
            this,   &SerialManager::onError);
#endif
}

SerialManager::~SerialManager()
{
    if (port_.isOpen())
        port_.close();
}

QStringList SerialManager::availablePorts() const
{
    QStringList list;
    for (const QSerialPortInfo& info : QSerialPortInfo::availablePorts()) {
        list << info.portName();
    }
    return list;
}

bool SerialManager::openPort(const QString& portName, qint32 baudRate, QString* err)
{
    if (port_.isOpen())
        port_.close();

    port_.setPortName(portName);
    port_.setBaudRate(baudRate);
    port_.setDataBits(QSerialPort::Data8);
    port_.setParity(QSerialPort::NoParity);
    port_.setStopBits(QSerialPort::OneStop);
    port_.setFlowControl(QSerialPort::NoFlowControl);

    if (!port_.open(QIODevice::ReadWrite)) {
        if (err) *err = port_.errorString();
        emit errorOccurred(port_.errorString());
        return false;
    }
    return true;
}

void SerialManager::closePort()
{
    if (port_.isOpen())
        port_.close();
}

bool SerialManager::writeBytes(const QByteArray& data)
{
    if (!port_.isOpen()) {
        emit errorOccurred("Port not open");
        return false;
    }

    qint64 written = port_.write(data);
    if (written == -1) {
        emit errorOccurred(port_.errorString());
        return false;
    }

    port_.flush();
    return true;
}

QByteArray SerialManager::readBytes()
{
    QByteArray out = rxBuffer_;
    rxBuffer_.clear();
    return out;
}

void SerialManager::onReadyRead()
{
    rxBuffer_.append(port_.readAll());
}

void SerialManager::onError(QSerialPort::SerialPortError e)
{
    if (e == QSerialPort::NoError)
        return;

    emit errorOccurred(QStringLiteral("Serial error: %1").arg(port_.errorString()));
}
