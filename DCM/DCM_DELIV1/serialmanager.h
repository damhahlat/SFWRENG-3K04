#pragma once

#include <QObject>
#include <QSerialPort>
#include <QStringList>

class SerialManager : public QObject
{
    Q_OBJECT

public:
    explicit SerialManager(QObject *parent = nullptr);
    ~SerialManager();

    QStringList availablePorts() const;

    bool openPort(const QString &portName, qint32 baudRate);
    void closePort();

    bool isOpen() const;
    qint32 currentBaudRate() const;
    QString currentPortName() const;

public slots:
    void sendData(const QByteArray &data);

signals:
    void dataReceived(const QByteArray &data);
    void errorOccurred(const QString &message);
    void connected(const QString &portName, qint32 baudRate);
    void disconnected();

private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);

private:
    QSerialPort m_port;
};
