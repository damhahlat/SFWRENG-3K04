#pragma once

// Include necessary libraries
#include <QString>
#include <optional>
#include <QMap>

// A structure to hold different modes
struct ModeProfile {
    int userId;
    QString mode; // options will be "AOO","VOO","AAI","VVI"
    std::optional<int> lrl, url, arp, vrp;
    std::optional<double> aAmp, aPw, vAmp, vPw;
};

/* The following code is primarily a suggestion and implementation by Generative AI (ChatGPT).
 * The purpose is to have all Database related functions live in their own namespace,
 * which improves cohesion while minimizing coupling.
 */
namespace Database {

// Open DB and create tables if needed
bool init(QString* err = nullptr);

// Stores the DB path, which is displayed in the About tab
QString path();

// Functions for managing users.
bool registerUser(const QString& username, const QString& password, QString* err = nullptr);
bool loginUser(const QString& username, const QString& password, QString* err = nullptr);
int  userId(const QString& username); // -1 if not found

// Update and insert the updates made to the database.
bool upsertProfile(const ModeProfile& p, QString* err = nullptr);

// Retrive the profile from the database.
std::optional<ModeProfile> getProfile(int userId, const QString& mode, QString* err = nullptr);
}
