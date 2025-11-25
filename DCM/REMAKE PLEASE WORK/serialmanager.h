#pragma once

#include <QObject>
#include <QSerialPort>
#include <QStringList>

// Lightweight helper around QSerialPort for the Serial Test dialog.
class SerialManager : public QObject {
    Q_OBJECT

public:
    explicit SerialManager(QObject* parent = nullptr);
    ~SerialManager() override;

    QStringList availablePorts() const;
    bool openPort(const QString& portName, qint32 baudRate, QString* err = nullptr);
    void closePort();

    bool isOpen() const { return port_.isOpen(); }

    bool writeBytes(const QByteArray& data);
    QByteArray readBytes();

signals:
    void errorOccurred(const QString& message);

private slots:
    void onReadyRead();
    void onError(QSerialPort::SerialPortError e);

private:
    QSerialPort port_;
    QByteArray  rxBuffer_;
};
