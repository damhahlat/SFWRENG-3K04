#include "pacemakerlink.h"
#include <QSerialPortInfo>
#include <QDebug>
#include <QtMath>

// -------------------------------------------------------------
// Message definitions for 32-byte protocol
// -------------------------------------------------------------
namespace {
constexpr int FRAME_SIZE = 32;

constexpr quint8 MSG_SET_PARAMS      = 0x01;
constexpr quint8 MSG_REQUEST_PARAMS  = 0x02;
constexpr quint8 MSG_PARAMS_RESPONSE = 0x03;

constexpr quint8 MSG_EGRAM_SAMPLES   = 0x04; // unused (stubbed)
constexpr quint8 MSG_EGRAM_START     = 0x07;
constexpr quint8 MSG_EGRAM_STOP      = 0x08;
}

// -------------------------------------------------------------
// Constructor / Destructor
// -------------------------------------------------------------
PacemakerLink::PacemakerLink(QObject* parent)
    : QObject(parent)
{
    connect(&m_port, &QSerialPort::readyRead,
            this, &PacemakerLink::handleReadyRead);

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(&m_port, &QSerialPort::errorOccurred,
            this, &PacemakerLink::handleError);
#else
    connect(&m_port,
            static_cast<void (QSerialPort::*)(QSerialPort::SerialPortError)>(&QSerialPort::error),
            this, &PacemakerLink::handleError);
#endif
}

PacemakerLink::~PacemakerLink()
{
    if (m_port.isOpen())
        m_port.close();
}

// -------------------------------------------------------------
// Port enumeration
// -------------------------------------------------------------
QStringList PacemakerLink::availablePorts() const
{
    QStringList list;
    for (const QSerialPortInfo& info : QSerialPortInfo::availablePorts()) {
        list << info.portName();
    }
    return list;
}

// -------------------------------------------------------------
// Connect / Disconnect
// -------------------------------------------------------------
bool PacemakerLink::connectToDevice(const QString& portName, qint32 baudRate, QString* errorMessage)
{
    if (m_port.isOpen())
        m_port.close();

    m_port.setPortName(portName);
    m_port.setBaudRate(baudRate);
    m_port.setDataBits(QSerialPort::Data8);
    m_port.setParity(QSerialPort::NoParity);
    m_port.setStopBits(QSerialPort::OneStop);
    m_port.setFlowControl(QSerialPort::NoFlowControl);

    if (!m_port.open(QIODevice::ReadWrite)) {
        if (errorMessage)
            *errorMessage = m_port.errorString();
        emit errorOccurred(m_port.errorString());
        return false;
    }

    emit connected(portName, baudRate);
    return true;
}

void PacemakerLink::disconnectFromDevice()
{
    if (m_port.isOpen()) {
        m_port.close();
        emit disconnected();
    }
}

// -------------------------------------------------------------
// Outgoing commands
// -------------------------------------------------------------
void PacemakerLink::sendParameters(const Database::ModeProfile& p)
{
    if (!m_port.isOpen()) {
        emit errorOccurred("Port not open.");
        return;
    }

    QByteArray frame = buildSetParametersFrame(p);
    m_port.write(frame);
    m_port.flush();
    emit parametersWritten();
}

void PacemakerLink::requestParameters()
{
    if (!m_port.isOpen()) {
        emit errorOccurred("Port not open.");
        return;
    }

    QByteArray frame = buildRequestParametersFrame();
    m_port.write(frame);
    m_port.flush();
}

void PacemakerLink::startEgramStream(quint8 mask)
{
    if (!m_port.isOpen()) return;

    QByteArray frame = buildStartEgramFrame(mask);
    m_port.write(frame);
    m_port.flush();
}

void PacemakerLink::stopEgramStream()
{
    if (!m_port.isOpen()) return;

    QByteArray frame = buildStopEgramFrame();
    m_port.write(frame);
    m_port.flush();
}

// -------------------------------------------------------------
// Incoming serial data
// -------------------------------------------------------------
void PacemakerLink::handleReadyRead()
{
    m_rxBuffer.append(m_port.readAll());

    while (m_rxBuffer.size() >= FRAME_SIZE) {
        QByteArray frame = m_rxBuffer.left(FRAME_SIZE);
        m_rxBuffer.remove(0, FRAME_SIZE);
        handleFrame(frame);
    }
}

void PacemakerLink::handleError(QSerialPort::SerialPortError e)
{
    if (e == QSerialPort::NoError)
        return;

    emit errorOccurred(m_port.errorString());
}

// -------------------------------------------------------------
// Frame builders
// -------------------------------------------------------------
QByteArray PacemakerLink::buildSetParametersFrame(const Database::ModeProfile& p) const
{
    QByteArray f(FRAME_SIZE, 0);

    // Byte 1 = message type
    f[1] = MSG_SET_PARAMS;

    // Byte 2 = mode code
    f[2] = modeToCode(p.mode);

    // Byte 3 = LRL
    f[3] = static_cast<char>(p.lrl.value_or(0));

    // Byte 4 = URL
    f[4] = static_cast<char>(p.url.value_or(0));

    // Floats for amplitude + PW
    writeFloat32LE(f, 5,  static_cast<float>(p.aAmp.value_or(0.0)));
    writeFloat32LE(f, 9,  static_cast<float>(p.vAmp.value_or(0.0)));
    writeFloat32LE(f, 13, static_cast<float>(p.aPw.value_or(0.0)));
    writeFloat32LE(f, 17, static_cast<float>(p.vPw.value_or(0.0)));

    // ARP / VRP
    writeUInt16LE(f, 21, static_cast<quint16>(p.vrp.value_or(0)));
    writeUInt16LE(f, 23, static_cast<quint16>(p.arp.value_or(0)));

    // AVD not used (Deliverable 2)
    writeUInt16LE(f, 26, 0);

    // Sensitivity fields (Deliverable 2)
    // Let’s place them in remaining bytes 28–31
    writeFloat32LE(f, 28, static_cast<float>(p.aSens.value_or(0.0))); // actually uses 4 bytes, overlaps with 31
    // but your frame is 32 bytes; sensitivity occupies final bytes safely

    // But we also need ventricular sensitivity.
    // Use offset 24/30? Instead align at 24 properly:
    writeFloat32LE(f, 24, static_cast<float>(p.vSens.value_or(0.0)));

    return f;
}

QByteArray PacemakerLink::buildRequestParametersFrame() const
{
    QByteArray f(FRAME_SIZE, 0);
    f[1] = MSG_REQUEST_PARAMS;
    return f;
}

QByteArray PacemakerLink::buildStartEgramFrame(quint8 mask) const
{
    QByteArray f(FRAME_SIZE, 0);
    f[1] = MSG_EGRAM_START;
    f[2] = mask;
    return f;
}

QByteArray PacemakerLink::buildStopEgramFrame() const
{
    QByteArray f(FRAME_SIZE, 0);
    f[1] = MSG_EGRAM_STOP;
    return f;
}

// -------------------------------------------------------------
// Incoming frame routing
// -------------------------------------------------------------
void PacemakerLink::handleFrame(const QByteArray& f)
{
    if (f.size() != FRAME_SIZE)
        return;

    const quint8 type = static_cast<quint8>(f[1]);

    switch (type) {
    case MSG_PARAMS_RESPONSE:
        handleParametersFrame(f);
        break;

    case MSG_EGRAM_SAMPLES:
        handleEgramFrame(f);
        break;

    default:
        break;
    }
}

// -------------------------------------------------------------
// Parameter response from pacemaker
// -------------------------------------------------------------
void PacemakerLink::handleParametersFrame(const QByteArray& f)
{
    Database::ModeProfile p;
    p.userId = -1;

    p.mode = codeToMode(static_cast<quint8>(f[2]));

    p.lrl = static_cast<quint8>(f[3]);
    p.url = static_cast<quint8>(f[4]);

    p.aAmp = readFloat32LE(f, 5);
    p.vAmp = readFloat32LE(f, 9);
    p.aPw  = readFloat32LE(f, 13);
    p.vPw  = readFloat32LE(f, 17);

    p.vrp = readUInt16LE(f, 21);
    p.arp = readUInt16LE(f, 23);

    p.vSens = readFloat32LE(f, 24);
    p.aSens = readFloat32LE(f, 28);

    emit parametersReadBack(p);
}

// -------------------------------------------------------------
// Egram handler (ignored as requested)
// -------------------------------------------------------------
void PacemakerLink::handleEgramFrame(const QByteArray& f)
{
    Q_UNUSED(f);
    // Deliverable 2 does NOT require waveform streaming.
}

// -------------------------------------------------------------
// Utility functions (endian)
// -------------------------------------------------------------
quint16 PacemakerLink::readUInt16LE(const QByteArray& d, int offset)
{
    quint8 lo = static_cast<quint8>(d[offset]);
    quint8 hi = static_cast<quint8>(d[offset+1]);
    return (hi << 8) | lo;
}

void PacemakerLink::writeUInt16LE(QByteArray& d, int offset, quint16 v)
{
    d[offset]   = static_cast<char>(v & 0xFF);
    d[offset+1] = static_cast<char>((v >> 8) & 0xFF);
}

float PacemakerLink::readFloat32LE(const QByteArray& d, int offset)
{
    float f;
    unsigned char* p = reinterpret_cast<unsigned char*>(&f);
    p[0] = d[offset];
    p[1] = d[offset+1];
    p[2] = d[offset+2];
    p[3] = d[offset+3];
    return f;
}

void PacemakerLink::writeFloat32LE(QByteArray& d, int offset, float v)
{
    const unsigned char* p = reinterpret_cast<const unsigned char*>(&v);
    d[offset]   = p[0];
    d[offset+1] = p[1];
    d[offset+2] = p[2];
    d[offset+3] = p[3];
}

// -------------------------------------------------------------
// Mode mapping
// -------------------------------------------------------------
quint8 PacemakerLink::modeToCode(const QString& m)
{
    if (m == "AOO")  return 0;
    if (m == "VOO")  return 1;
    if (m == "AAI")  return 2;
    if (m == "VVI")  return 3;
    if (m == "AOOR") return 4;
    if (m == "VOOR") return 5;
    if (m == "AAIR") return 6;
    if (m == "VVIR") return 7;
    return 0xFF;
}

QString PacemakerLink::codeToMode(quint8 c)
{
    switch (c) {
    case 0: return "AOO";
    case 1: return "VOO";
    case 2: return "AAI";
    case 3: return "VVI";
    case 4: return "AOOR";
    case 5: return "VOOR";
    case 6: return "AAIR";
    case 7: return "VVIR";
    default: return "AOO";
    }
}
