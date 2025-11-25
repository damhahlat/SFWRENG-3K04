#include "database.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

namespace Database {

// ------------------------------------------------------------
// Internal helper: get DB instance (singleton pattern)
// ------------------------------------------------------------
static QSqlDatabase& db()
{
    static bool initialized = false;
    static QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");

    if (!initialized) {
        db.setDatabaseName(path());
        initialized = true;
    }
    return db;
}

// ------------------------------------------------------------
// Path
// ------------------------------------------------------------
QString path()
{
    // Store DB in user's Documents folder
    QString base = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QDir dir(base);
    if (!dir.exists("dcm"))
        dir.mkdir("dcm");

    return dir.filePath("dcm/pacemaker.db");
}

// ------------------------------------------------------------
// Initialize database
// ------------------------------------------------------------
bool init(QString* err)
{
    auto& connection = db();

    if (!connection.open()) {
        if (err)
            *err = "Failed to open database: " + connection.lastError().text();
        return false;
    }

    QSqlQuery q;

    // Create users table
    if (!q.exec(
            "CREATE TABLE IF NOT EXISTS users ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  username TEXT UNIQUE,"
            "  password TEXT"
            ");"
            )) {
        if (err) *err = q.lastError().text();
        return false;
    }

    // Create mode profiles table
    if (!q.exec(
            "CREATE TABLE IF NOT EXISTS profiles ("
            "  userId INTEGER,"
            "  mode TEXT,"
            "  lrl INTEGER,"
            "  url INTEGER,"
            "  arp INTEGER,"
            "  vrp INTEGER,"
            "  aAmp REAL,"
            "  aPw REAL,"
            "  vAmp REAL,"
            "  vPw REAL,"
            "  aSens REAL,"
            "  vSens REAL,"
            "  PRIMARY KEY (userId, mode)"
            ");"
            )) {
        if (err) *err = q.lastError().text();
        return false;
    }

    return true;
}

// ------------------------------------------------------------
// User Management
// ------------------------------------------------------------
bool registerUser(const QString& username, const QString& password, QString* err)
{
    QSqlQuery q;

    q.prepare("INSERT INTO users (username, password) VALUES (?, ?)");
    q.addBindValue(username);
    q.addBindValue(password);

    if (!q.exec()) {
        if (err) *err = "Registration error: " + q.lastError().text();
        return false;
    }
    return true;
}

bool loginUser(const QString& username, const QString& password, QString* err)
{
    QSqlQuery q;

    q.prepare("SELECT id FROM users WHERE username=? AND password=?");
    q.addBindValue(username);
    q.addBindValue(password);

    if (!q.exec()) {
        if (err) *err = "Database error: " + q.lastError().text();
        return false;
    }

    return q.next();
}

int userId(const QString& username)
{
    QSqlQuery q;

    q.prepare("SELECT id FROM users WHERE username=?");
    q.addBindValue(username);

    if (!q.exec()) return -1;
    if (!q.next()) return -1;

    return q.value(0).toInt();
}

// ------------------------------------------------------------
// Profile Management
// ------------------------------------------------------------
bool upsertProfile(const ModeProfile& p, QString* err)
{
    QSqlQuery q;

    q.prepare(
        "INSERT INTO profiles ("
        "  userId, mode, lrl, url, arp, vrp, "
        "  aAmp, aPw, vAmp, vPw, aSens, vSens"
        ") VALUES (?,?,?,?,?,?,?,?,?,?,?,?) "
        "ON CONFLICT(userId, mode) DO UPDATE SET "
        "  lrl=excluded.lrl,"
        "  url=excluded.url,"
        "  arp=excluded.arp,"
        "  vrp=excluded.vrp,"
        "  aAmp=excluded.aAmp,"
        "  aPw=excluded.aPw,"
        "  vAmp=excluded.vAmp,"
        "  vPw=excluded.vPw,"
        "  aSens=excluded.aSens,"
        "  vSens=excluded.vSens;"
        );

    q.addBindValue(p.userId);
    q.addBindValue(p.mode);
    q.addBindValue(p.lrl.value_or(0));
    q.addBindValue(p.url.value_or(0));
    q.addBindValue(p.arp.value_or(0));
    q.addBindValue(p.vrp.value_or(0));
    q.addBindValue(p.aAmp.value_or(0.0));
    q.addBindValue(p.aPw.value_or(0.0));
    q.addBindValue(p.vAmp.value_or(0.0));
    q.addBindValue(p.vPw.value_or(0.0));
    q.addBindValue(p.aSens.value_or(0.0));
    q.addBindValue(p.vSens.value_or(0.0));

    if (!q.exec()) {
        if (err) *err = "Failed to save profile: " + q.lastError().text();
        return false;
    }

    return true;
}

std::optional<ModeProfile> getProfile(int uid, const QString& mode, QString* err)
{
    QSqlQuery q;

    q.prepare(
        "SELECT "
        "  lrl, url, arp, vrp, "
        "  aAmp, aPw, vAmp, vPw, "
        "  aSens, vSens "
        "FROM profiles WHERE userId=? AND mode=?"
        );
    q.addBindValue(uid);
    q.addBindValue(mode);

    if (!q.exec()) {
        if (err) *err = q.lastError().text();
        return {};
    }

    if (!q.next())
        return {};

    ModeProfile p;
    p.userId = uid;
    p.mode   = mode;

    auto getOptInt = [&](int idx) -> std::optional<int> {
        int val = q.value(idx).toInt();
        return (val == 0 ? std::optional<int>() : val);
    };

    auto getOptDouble = [&](int idx) -> std::optional<double> {
        double val = q.value(idx).toDouble();
        return (val == 0.0 ? std::optional<double>() : val);
    };

    p.lrl   = getOptInt(0);
    p.url   = getOptInt(1);
    p.arp   = getOptInt(2);
    p.vrp   = getOptInt(3);
    p.aAmp  = getOptDouble(4);
    p.aPw   = getOptDouble(5);
    p.vAmp  = getOptDouble(6);
    p.vPw   = getOptDouble(7);
    p.aSens = getOptDouble(8);
    p.vSens = getOptDouble(9);

    return p;
}

} // namespace Database
