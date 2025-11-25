#pragma once

#include <QObject>
#include <QSerialPort>
#include <QStringList>
#include <QVector>

#include "database.h"  // Database::ModeProfile

class PacemakerLink : public QObject {
    Q_OBJECT

public:
    explicit PacemakerLink(QObject* parent = nullptr);
    ~PacemakerLink() override;

    // Device / COM management
    QStringList availablePorts() const;
    bool connectToDevice(const QString& portName, qint32 baudRate, QString* errorMessage = nullptr);
    void disconnectFromDevice();
    bool isConnected() const { return m_port.isOpen(); }

    // Deliverable 2 features
    void sendParameters(const Database::ModeProfile& profile);
    void requestParameters();

    // Egram (NOT IMPLEMENTED AS REQUESTED)
    void startEgramStream(quint8 mask);
    void stopEgramStream();

signals:
    // Connection status
    void connected(const QString& port, qint32 baud);
    void disconnected();
    void errorOccurred(const QString& msg);

    // Parameter interaction
    void parametersWritten();
    void parametersReadBack(const Database::ModeProfile& profile);

    // Egram data (ignored)
    void egramSamplesReceived(const QVector<double>& atrial,
                              const QVector<double>& ventricular);

private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError err);

private:
    // Frame builders (32-byte frames)
    QByteArray buildSetParametersFrame(const Database::ModeProfile& p) const;
    QByteArray buildRequestParametersFrame() const;
    QByteArray buildStartEgramFrame(quint8 mask) const;
    QByteArray buildStopEgramFrame() const;

    // Frame processing
    void processIncomingBytes();
    void handleFrame(const QByteArray& frame);
    void handleParametersFrame(const QByteArray& frame);
    void handleEgramFrame(const QByteArray& frame);

    // Utilities
    static quint16 readUInt16LE(const QByteArray& data, int offset);
    static void writeUInt16LE(QByteArray& data, int offset, quint16 value);

    static float readFloat32LE(const QByteArray& data, int offset);
    static void writeFloat32LE(QByteArray& data, int offset, float value);

    // Mode mapping
    static quint8  modeToCode(const QString& mode);
    static QString codeToMode(quint8 code);

private:
    QSerialPort m_port;
    QByteArray m_rxBuffer;
};
