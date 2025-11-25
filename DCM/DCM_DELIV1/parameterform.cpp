#include "parameterform.h"
#include "ui_parameterform.h"
#include "serialmanager.h"

#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QFormLayout>
#include <QMessageBox>
#include <QInputDialog>
#include <QtMath>
#include <QtEndian>

// Constructor
ParameterForm::ParameterForm(int userId, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::ParameterForm)
    , userId_(userId)
    , settings_("McMaster", "DCM")
{
    ui->setupUi(this);

    // Create serial manager
    serial_ = new SerialManager(this);
    connect(serial_, &SerialManager::errorOccurred,
            this, &ParameterForm::onSerialError);

    // Wire buttons
    connect(ui->saveBtn,  &QPushButton::clicked, this, &ParameterForm::onSave);
    connect(ui->loadBtn,  &QPushButton::clicked, this, &ParameterForm::onLoad);
    connect(ui->clearBtn, &QPushButton::clicked, this, &ParameterForm::onClear);
    connect(ui->sendBtn,  &QPushButton::clicked, this, &ParameterForm::onSend);
    connect(ui->stopBtn,  &QPushButton::clicked, this, &ParameterForm::onStop);

    // Track field changes for live validation
    auto notifyChange = [this]() { onFieldChanged(); };
    connect(ui->modeCombo, &QComboBox::currentTextChanged, this, notifyChange);
    connect(ui->lrlSpin,   qOverload<int>(&QSpinBox::valueChanged), this, notifyChange);
    connect(ui->urlSpin,   qOverload<int>(&QSpinBox::valueChanged), this, notifyChange);
    connect(ui->arpSpin,   qOverload<int>(&QSpinBox::valueChanged), this, notifyChange);
    connect(ui->vrpSpin,   qOverload<int>(&QSpinBox::valueChanged), this, notifyChange);
    connect(ui->aAmpSpin,  qOverload<double>(&QDoubleSpinBox::valueChanged), this, notifyChange);
    connect(ui->aPwSpin,   qOverload<double>(&QDoubleSpinBox::valueChanged), this, notifyChange);
    connect(ui->vAmpSpin,  qOverload<double>(&QDoubleSpinBox::valueChanged), this, notifyChange);
    connect(ui->vPwSpin,   qOverload<double>(&QDoubleSpinBox::valueChanged), this, notifyChange);

    applyDefaults();
    restoreMode();
    reflectValidity();
    updateConnectionStatus();
}

ParameterForm::~ParameterForm()
{
    if (serial_ && serial_->isOpen()) {
        serial_->closePort();
    }
    delete ui;
}

// ---------- small helpers ----------

QString ParameterForm::mode() const
{
    return ui->modeCombo->currentText();
}

void ParameterForm::applyDefaults()
{
    // Reasonable defaults; adjust if your spec says otherwise
    ui->lrlSpin->setValue(60);
    ui->urlSpin->setValue(120);

    ui->arpSpin->setValue(250);
    ui->vrpSpin->setValue(320);

    ui->aAmpSpin->setValue(3.5);
    ui->aPwSpin->setValue(0.4);

    ui->vAmpSpin->setValue(3.5);
    ui->vPwSpin->setValue(0.4);
}

void ParameterForm::rememberMode(const QString& m)
{
    settings_.setValue("lastMode", m);
}

void ParameterForm::restoreMode()
{
    const QString m = settings_.value("lastMode").toString();
    if (m.isEmpty())
        return;

    int idx = ui->modeCombo->findText(m);
    if (idx >= 0)
        ui->modeCombo->setCurrentIndex(idx);
}

bool ParameterForm::checkAmplitudeStep(double v, QString* why) const
{
    // Amplitude must be a multiple of 0.5 V between 0 and 7.5 V
    if (v < 0.0 || v > 7.5) {
        if (why) *why = "Amplitude must be between 0 and 7.5 V.";
        return false;
    }
    double steps = v / 0.5;
    if (qFabs(steps - qRound(steps)) > 1e-6) {
        if (why) *why = "Amplitude must be in 0.5 V steps.";
        return false;
    }
    return true;
}

bool ParameterForm::checkPulseWidth(double v, QString* why) const
{
    // Pulse width typically 0.1 to 1.9 ms (adjust per spec)
    if (v < 0.1 || v > 1.9) {
        if (why) *why = "Pulse width must be between 0.1 and 1.9 ms.";
        return false;
    }
    return true;
}

void ParameterForm::updateConnectionStatus()
{
    if (serial_ && serial_->isOpen()) {
        emit statusMessage("✓ Serial connected");
    } else {
        emit statusMessage("Serial disconnected");
    }
}

// ---------- validation ----------

bool ParameterForm::validate(QString* err) const
{
    // Rate limits
    if (ui->lrlSpin->value() >= ui->urlSpin->value()) {
        if (err) *err = "LRL must be less than URL.";
        return false;
    }

    if (ui->lrlSpin->value() < 30 || ui->lrlSpin->value() > 175) {
        if (err) *err = "LRL must be between 30 and 175 ppm.";
        return false;
    }

    if (ui->urlSpin->value() < 50 || ui->urlSpin->value() > 175) {
        if (err) *err = "URL must be between 50 and 175 ppm.";
        return false;
    }

    // Refractory periods
    if (ui->arpSpin->value() < 150 || ui->arpSpin->value() > 500) {
        if (err) *err = "ARP must be between 150 and 500 ms.";
        return false;
    }

    if (ui->vrpSpin->value() < 150 || ui->vrpSpin->value() > 500) {
        if (err) *err = "VRP must be between 150 and 500 ms.";
        return false;
    }

    // Atrial amplitude
    QString why;
    if (!checkAmplitudeStep(ui->aAmpSpin->value(), &why)) {
        if (err) *err = "Atrial amplitude invalid: " + why;
        return false;
    }

    // Ventricular amplitude
    if (!checkAmplitudeStep(ui->vAmpSpin->value(), &why)) {
        if (err) *err = "Ventricular amplitude invalid: " + why;
        return false;
    }

    // Atrial pulse width
    if (!checkPulseWidth(ui->aPwSpin->value(), &why)) {
        if (err) *err = "Atrial pulse width invalid: " + why;
        return false;
    }

    // Ventricular pulse width
    if (!checkPulseWidth(ui->vPwSpin->value(), &why)) {
        if (err) *err = "Ventricular pulse width invalid: " + why;
        return false;
    }

    return true;
}

void ParameterForm::reflectValidity()
{
    QString err;
    const bool ok = validate(&err);

    // Update status message with color coding
    if (ok) {
        emit statusMessage("✓ Parameters are valid.");
    } else {
        emit statusMessage("✗ " + err);
    }
}

// ---------- slots ----------

void ParameterForm::onFieldChanged()
{
    reflectValidity();
    rememberMode(mode());
}

void ParameterForm::onSave()
{
    Database::ModeProfile p;
    QString err;
    if (!tryBuildProfile(&p, &err)) {
        QMessageBox::warning(this, "Cannot Save", err);
        return;
    }

    QString dbErr;
    if (!Database::upsertProfile(p, &dbErr)) {
        QMessageBox::critical(this, "Database Error",
                              "Failed to save profile:\n" + dbErr);
        return;
    }

    QMessageBox::information(this, "Saved",
                             QString("Profile saved for mode %1").arg(p.mode));
    emit statusMessage("✓ Profile saved for mode " + p.mode);
}

void ParameterForm::onLoad()
{
    const QString m = mode();
    QString dbErr;
    auto opt = Database::getProfile(userId_, m, &dbErr);

    if (!opt) {
        QString msg = dbErr.isEmpty()
        ? QString("No saved profile found for mode %1").arg(m)
        : "Database error: " + dbErr;
        QMessageBox::information(this, "Load Profile", msg);
        return;
    }

    const Database::ModeProfile& p = *opt;

    // Load values into UI
    ui->lrlSpin->setValue(p.lrl.value_or(60));
    ui->urlSpin->setValue(p.url.value_or(120));
    ui->arpSpin->setValue(p.arp.value_or(250));
    ui->vrpSpin->setValue(p.vrp.value_or(320));

    ui->aAmpSpin->setValue(p.aAmp.value_or(3.5));
    ui->aPwSpin->setValue(p.aPw.value_or(0.4));
    ui->vAmpSpin->setValue(p.vAmp.value_or(3.5));
    ui->vPwSpin->setValue(p.vPw.value_or(0.4));

    QMessageBox::information(this, "Loaded",
                             QString("Profile loaded for mode %1").arg(m));
    emit statusMessage("✓ Profile loaded for mode " + m);
    reflectValidity();
}

void ParameterForm::onClear()
{
    auto reply = QMessageBox::question(
        this,
        "Clear Parameters",
        "Reset all parameters to default values?",
        QMessageBox::Yes | QMessageBox::No
        );

    if (reply == QMessageBox::Yes) {
        clearAll();
    }
}

void ParameterForm::clearAll()
{
    applyDefaults();
    reflectValidity();
    emit statusMessage("Fields reset to defaults.");
}

// ---------- SERIAL COMMUNICATION ----------

void ParameterForm::onSend()
{
    // Build the frame first
    QByteArray frame;
    QString err;
    if (!buildTxFrame(&frame, &err)) {
        QMessageBox::warning(this, "Invalid Parameters",
                             "Cannot send parameters:\n" + err);
        return;
    }

    // Check if serial port is open
    if (!serial_->isOpen()) {
        // Get available ports
        QStringList ports = serial_->availablePorts();
        if (ports.isEmpty()) {
            QMessageBox::warning(this, "No Ports",
                                 "No serial ports available.\n"
                                 "Please connect a device and try again.");
            return;
        }

        // Ask user to select port
        bool ok = false;
        QString port = QInputDialog::getItem(
            this,
            "Select Serial Port",
            "Choose the pacemaker COM port:",
            ports,
            0,
            false,
            &ok
            );

        if (!ok || port.isEmpty()) {
            return;
        }

        // Ask for baud rate
        QStringList baudRates = {"9600", "19200", "38400", "57600", "115200"};
        QString baudStr = QInputDialog::getItem(
            this,
            "Select Baud Rate",
            "Choose the baud rate:",
            baudRates,
            4, // default to 115200
            false,
            &ok
            );

        if (!ok || baudStr.isEmpty()) {
            return;
        }

        int baud = baudStr.toInt();

        // Try to open the port
        QString serr;
        if (!serial_->openPort(port, baud, &serr)) {
            QMessageBox::critical(this, "Connection Failed",
                                  QString("Failed to open %1:\n%2").arg(port, serr));
            return;
        }

        updateConnectionStatus();
        QMessageBox::information(this, "Connected",
                                 QString("Successfully connected to %1 @ %2 baud").arg(port).arg(baud));
    }

    // Send the frame
    if (!serial_->writeBytes(frame)) {
        QMessageBox::critical(this, "Send Failed",
                              "Failed to write data to serial port.\n"
                              "The device may have been disconnected.");
        updateConnectionStatus();
        return;
    }

    // Success!
    emit statusMessage(QString("✓ Sent %1-byte frame to pacemaker").arg(frame.size()));

    // Log frame for debugging
    qDebug() << "TX Frame [" << frame.size() << "bytes]:" << frame.toHex(' ').toUpper();

    QMessageBox::information(this, "Sent",
                             QString("Parameters sent successfully!\n"
                                     "Mode: %1\n"
                                     "LRL: %2 ppm\n"
                                     "URL: %3 ppm")
                                 .arg(mode())
                                 .arg(ui->lrlSpin->value())
                                 .arg(ui->urlSpin->value()));
}

void ParameterForm::onStop()
{
    if (!serial_->isOpen()) {
        QMessageBox::information(this, "Not Connected",
                                 "Serial port is not open.");
        return;
    }

    auto reply = QMessageBox::question(
        this,
        "Disconnect",
        "Close the serial port connection?",
        QMessageBox::Yes | QMessageBox::No
        );

    if (reply == QMessageBox::Yes) {
        serial_->closePort();
        updateConnectionStatus();
        emit statusMessage("Serial port closed.");
        QMessageBox::information(this, "Disconnected",
                                 "Serial port closed successfully.");
    }
}

void ParameterForm::onSerialError(const QString& msg)
{
    QMessageBox::warning(this, "Serial Error", msg);
    updateConnectionStatus();
}

// ---------- profile build ----------

bool ParameterForm::tryBuildProfile(Database::ModeProfile* out, QString* errMsg) const
{
    if (!out) return false;

    QString err;
    if (!validate(&err)) {
        if (errMsg) *errMsg = err;
        return false;
    }

    Database::ModeProfile p;
    p.userId = userId_;
    p.mode   = mode();

    p.lrl = ui->lrlSpin->value();
    p.url = ui->urlSpin->value();
    p.arp = ui->arpSpin->value();
    p.vrp = ui->vrpSpin->value();

    p.aAmp = ui->aAmpSpin->value();
    p.aPw  = ui->aPwSpin->value();
    p.vAmp = ui->vAmpSpin->value();
    p.vPw  = ui->vPwSpin->value();

    // Sensitivities (if present in your DB struct but not in UI) can stay empty:
    p.aSens = std::nullopt;
    p.vSens = std::nullopt;

    *out = p;
    if (errMsg) errMsg->clear();
    return true;
}

// ---------- current values as text ----------

QMap<QString, QString> ParameterForm::currentValuesAsText() const
{
    QMap<QString, QString> kv;
    kv["Mode"] = mode();
    kv["LRL (bpm)"] = QString::number(ui->lrlSpin->value());
    kv["URL (bpm)"] = QString::number(ui->urlSpin->value());
    kv["Atrial Amplitude (V)"] = QString::number(ui->aAmpSpin->value(), 'f', 2);
    kv["Atrial Pulse Width (ms)"] = QString::number(ui->aPwSpin->value(), 'f', 2);
    kv["Ventricular Amplitude (V)"] = QString::number(ui->vAmpSpin->value(), 'f', 2);
    kv["Ventricular Pulse Width (ms)"] = QString::number(ui->vPwSpin->value(), 'f', 2);
    kv["ARP (ms)"] = QString::number(ui->arpSpin->value());
    kv["VRP (ms)"] = QString::number(ui->vrpSpin->value());
    return kv;
}

// ---------- mode <-> code mapping ----------

quint8 ParameterForm::modeToCode(const QString& mode)
{
    const QString m = mode.toUpper();
    if (m == "AOO")  return 0;
    if (m == "VOO")  return 1;
    if (m == "AAI")  return 2;
    if (m == "VVI")  return 3;
    if (m == "AOOR") return 4;
    if (m == "VOOR") return 5;
    if (m == "AAIR") return 6;
    if (m == "VVIR") return 7;
    return 0; // default to AOO
}

QString ParameterForm::codeToMode(quint8 code)
{
    switch (code) {
    case 0:  return "AOO";
    case 1:  return "VOO";
    case 2:  return "AAI";
    case 3:  return "VVI";
    case 4:  return "AOOR";
    case 5:  return "VOOR";
    case 6:  return "AAIR";
    case 7:  return "VVIR";
    default: return "AOO";
    }
}

// ---------- frame encode / decode ----------

bool ParameterForm::buildTxFrame(QByteArray* outFrame, QString* errMsg) const
{
    if (!outFrame) {
        if (errMsg) *errMsg = "Invalid output buffer";
        return false;
    }

    Database::ModeProfile p;
    if (!tryBuildProfile(&p, errMsg))
        return false;

    QByteArray frame(32, 0);

    // Helper lambdas for encoding
    auto putFloat = [&frame](int index0, double value) {
        float f = static_cast<float>(value);
        quint32 raw;
        static_assert(sizeof(float) == sizeof(quint32), "float must be 4 bytes");
        memcpy(&raw, &f, sizeof(float));
        // Little-endian byte order
        frame[index0 + 0] = char(raw & 0xFF);
        frame[index0 + 1] = char((raw >> 8) & 0xFF);
        frame[index0 + 2] = char((raw >> 16) & 0xFF);
        frame[index0 + 3] = char((raw >> 24) & 0xFF);
    };

    auto putU16 = [&frame](int index0, int value) {
        quint16 u = quint16(value);
        // Little-endian byte order
        frame[index0 + 0] = char(u & 0xFF);
        frame[index0 + 1] = char((u >> 8) & 0xFF);
    };

    // Build the frame (0-indexed)
    frame[0] = 0;                           // Byte 1: Reserved/Header
    frame[1] = 1;                           // Byte 2: Command (1 = SET_PARAM)
    frame[2] = char(modeToCode(p.mode));    // Byte 3: Mode code
    frame[3] = char(p.lrl.value_or(60));    // Byte 4: LRL
    frame[4] = char(p.url.value_or(120));   // Byte 5: URL

    // Bytes 6-9: Atrial Amplitude (float)
    putFloat(5, p.aAmp.value_or(3.5));

    // Bytes 10-13: Ventricular Amplitude (float)
    putFloat(9, p.vAmp.value_or(3.5));

    // Bytes 14-17: Atrial Pulse Width (float)
    putFloat(13, p.aPw.value_or(0.4));

    // Bytes 18-21: Ventricular Pulse Width (float)
    putFloat(17, p.vPw.value_or(0.4));

    // Bytes 22-23: VRP (uint16)
    putU16(21, p.vrp.value_or(320));

    // Bytes 24-25: ARP (uint16)
    putU16(23, p.arp.value_or(250));

    // Bytes 26-32: Reserved/unused parameters (all zeros)
    frame[25] = 0;      // Byte 26: HysteresisTime
    putU16(26, 0);      // Bytes 27-28: AVD
    frame[28] = 0;      // Byte 29: ReactionTime
    frame[29] = 0;      // Byte 30: ResponseFactor
    frame[30] = 0;      // Byte 31: RecoveryTime
    frame[31] = 0;      // Byte 32: Unused

    *outFrame = frame;
    if (errMsg) errMsg->clear();

    return true;
}

bool ParameterForm::applyFromRxFrame(const QByteArray& frame, QString* errMsg)
{
    if (frame.size() < 32) {
        if (errMsg) *errMsg = "Frame too short (expected 32 bytes).";
        return false;
    }

    // Helper lambdas for decoding
    auto getFloat = [&frame](int index0) -> double {
        quint32 raw = 0;
        raw |= (quint8(frame[index0 + 0]) << 0);
        raw |= (quint8(frame[index0 + 1]) << 8);
        raw |= (quint8(frame[index0 + 2]) << 16);
        raw |= (quint8(frame[index0 + 3]) << 24);
        float f;
        memcpy(&f, &raw, sizeof(float));
        return double(f);
    };

    auto getU16 = [&frame](int index0) -> int {
        quint16 u = 0;
        u |= quint8(frame[index0 + 0]);
        u |= quint8(frame[index0 + 1]) << 8;
        return int(u);
    };

    // Parse the frame
    const quint8 modeCode = quint8(frame[2]);
    const int lrl = quint8(frame[3]);
    const int url = quint8(frame[4]);

    const double aAmp = getFloat(5);
    const double vAmp = getFloat(9);
    const double aPw  = getFloat(13);
    const double vPw  = getFloat(17);

    const int vrp = getU16(21);
    const int arp = getU16(23);

    // Update UI
    const QString modeStr = codeToMode(modeCode);
    int idx = ui->modeCombo->findText(modeStr);
    if (idx >= 0)
        ui->modeCombo->setCurrentIndex(idx);

    ui->lrlSpin->setValue(lrl);
    ui->urlSpin->setValue(url);

    ui->arpSpin->setValue(arp);
    ui->vrpSpin->setValue(vrp);

    ui->aAmpSpin->setValue(aAmp);
    ui->aPwSpin->setValue(aPw);
    ui->vAmpSpin->setValue(vAmp);
    ui->vPwSpin->setValue(vPw);

    if (errMsg) errMsg->clear();
    reflectValidity();
    emit statusMessage("✓ Parameters loaded from device frame.");

    qDebug() << "RX Frame - Mode:" << modeStr
             << "LRL:" << lrl << "URL:" << url;

    return true;
}
