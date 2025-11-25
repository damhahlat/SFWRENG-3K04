#pragma once

#include <QString>
#include <optional>
#include <QMap>

namespace Database {

// Represents a single mode profile for a given user.
struct ModeProfile {
    int userId{};
    QString mode; // "AOO", "VOO", "AAI", "VVI", "AOOR", "VOOR", "AAIR", "VVIR"

    // Rate limits and refractory periods
    std::optional<int> lrl;   // Lower Rate Limit (ppm)
    std::optional<int> url;   // Upper Rate Limit (ppm)
    std::optional<int> arp;   // Atrial Refractory Period (ms)
    std::optional<int> vrp;   // Ventricular Refractory Period (ms)

    // Amplitudes and PW
    std::optional<double> aAmp; // Atrial Amplitude (V)
    std::optional<double> aPw;  // Atrial Pulse Width (ms)
    std::optional<double> vAmp; // Ventricular Amplitude (V)
    std::optional<double> vPw;  // Ventricular Pulse Width (ms)

    // Deliverable 2: Sensitivity
    std::optional<double> aSens; // Atrial Sensitivity (V)
    std::optional<double> vSens; // Ventricular Sensitivity (V)
};

// Path to SQLite pacemaker database
QString path();

// Basic DB init
bool init(QString* err = nullptr);

// User management
bool registerUser(const QString& username, const QString& password, QString* err = nullptr);
bool loginUser(const QString& username, const QString& password, QString* err = nullptr);
int  userId(const QString& username);

// Profiles
bool upsertProfile(const ModeProfile& p, QString* err = nullptr);
std::optional<ModeProfile> getProfile(int userId, const QString& mode, QString* err = nullptr);

} // namespace Database
