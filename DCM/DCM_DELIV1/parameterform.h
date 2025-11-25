#pragma once

#include <QWidget>
#include <QSettings>
#include <QMap>
#include <QString>
#include <QByteArray>

#include "database.h"   // Database::ModeProfile

class SerialManager;

QT_BEGIN_NAMESPACE
namespace Ui { class ParameterForm; }
QT_END_NAMESPACE

class ParameterForm : public QWidget {
    Q_OBJECT

public:
    explicit ParameterForm(int userId, QWidget* parent = nullptr);
    ~ParameterForm() override;

    // Build a profile from UI. Returns false if validation fails.
    bool tryBuildProfile(Database::ModeProfile* out, QString* errMsg = nullptr) const;

    // Used by report builder to get "LRL: 60 ppm", etc.
    QMap<QString, QString> currentValuesAsText() const;

    // === Frame encode/decode for serial link ===
    // Build the 32-byte frame that the Simulink model expects.
    bool buildTxFrame(QByteArray* outFrame, QString* errMsg = nullptr) const;

    // Parse a 32-byte frame from the device and update the UI.
    bool applyFromRxFrame(const QByteArray& frame, QString* errMsg = nullptr);

signals:
    void statusMessage(const QString& msg);

public slots:
    void clearAll();

private slots:
    void onFieldChanged();
    void onSave();
    void onLoad();
    void onClear();

    // Serial communication slots
    void onSend();
    void onStop();
    void onSerialError(const QString& msg);

private:
    Ui::ParameterForm* ui;
    int userId_;
    QSettings settings_;
    SerialManager* serial_{nullptr};

    bool validate(QString* err) const;
    void reflectValidity();
    QString mode() const;

    void applyDefaults();
    void rememberMode(const QString& m);
    void restoreMode();

    bool checkAmplitudeStep(double v, QString* why) const;
    bool checkPulseWidth(double v, QString* why) const;

    void updateConnectionStatus();

    // Helpers for frame encode/decode
    static quint8 modeToCode(const QString& mode);
    static QString codeToMode(quint8 code);
};
