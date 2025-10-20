#pragma once
#include <QString>
#include <optional>

struct ModeProfile {
    int userId;
    QString mode; // "AOO","VOO","AAI","VVI"
    std::optional<int> lrl, url, arp, vrp;
    std::optional<double> aAmp, aPw, vAmp, vPw;
};

namespace Database {
// Open DB and create tables if needed
bool init(QString* err = nullptr);

// Path for your About tab
QString path();

// Auth
bool registerUser(const QString& username, const QString& password, QString* err = nullptr);
bool loginUser(const QString& username, const QString& password, QString* err = nullptr);
int  userId(const QString& username); // -1 if not found

// Profiles (we'll use these when we build the Parameters tab)
bool upsertProfile(const ModeProfile& p, QString* err = nullptr);
std::optional<ModeProfile> getProfile(int userId, const QString& mode, QString* err = nullptr);
}
