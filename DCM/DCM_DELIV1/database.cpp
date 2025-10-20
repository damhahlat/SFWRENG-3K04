#include "database.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QDir>

static QSqlDatabase db;

static QString hashPw(const QString& pw) {
    return QString::fromUtf8(QCryptographicHash::hash(pw.toUtf8(), QCryptographicHash::Sha256).toHex());
}

QString Database::path() {
    auto dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/dcm.db";
}

bool Database::init(QString* err) {
    if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
        if (err) *err = "QSQLITE driver not available";
        return false;
    }

    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(path());
    if (!db.open()) {
        if (err) *err = db.lastError().text();
        return false;
    }

    // Enable FK behavior (habit, even if not used yet)
    {
        QSqlQuery pragma("PRAGMA foreign_keys = ON;");
        if (pragma.lastError().isValid()) {
            if (err) *err = "Failed to enable foreign_keys: " + pragma.lastError().text();
            return false;
        }
    }

    // Run DDL statements one-by-one
    const QStringList ddl = {
        // users
        "CREATE TABLE IF NOT EXISTS users ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " username TEXT UNIQUE NOT NULL,"
        " password_hash TEXT NOT NULL,"
        " created_at TEXT NOT NULL DEFAULT (datetime('now'))"
        ")",

        // mode_profiles
        "CREATE TABLE IF NOT EXISTS mode_profiles ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " user_id INTEGER NOT NULL,"
        " mode TEXT NOT NULL,"
        " lrl INTEGER, url INTEGER,"
        " atrial_amplitude REAL, atrial_pulse_width REAL,"
        " ventricular_amplitude REAL, ventricular_pulse_width REAL,"
        " arp INTEGER, vrp INTEGER,"
        " updated_at TEXT NOT NULL DEFAULT (datetime('now')),"
        " UNIQUE(user_id, mode)"
        ")"
    };

    for (const auto& sql : ddl) {
        QSqlQuery q;
        if (!q.exec(sql)) {
            if (err) *err = q.lastError().text();
            return false;
        }
    }
    return true;
}

bool Database::registerUser(const QString& u, const QString& p, QString* err) {
    // enforce 10-user cap
    QSqlQuery count("SELECT COUNT(*) FROM users");
    if (!count.next()) { if (err) *err = count.lastError().text(); return false; }
    if (count.value(0).toInt() >= 10) { if (err) *err = "User limit (10) reached"; return false; }

    QSqlQuery dupe;
    dupe.prepare("SELECT 1 FROM users WHERE username=?");
    dupe.addBindValue(u);
    if (!dupe.exec()) { if (err) *err = dupe.lastError().text(); return false; }
    if (dupe.next()) { if (err) *err = "Username already exists"; return false; }

    QSqlQuery ins;
    ins.prepare("INSERT INTO users(username, password_hash) VALUES(?, ?)");
    ins.addBindValue(u);
    ins.addBindValue(hashPw(p));
    if (!ins.exec()) { if (err) *err = ins.lastError().text(); return false; }
    return true;
}

bool Database::loginUser(const QString& u, const QString& p, QString* err) {
    QSqlQuery q;
    q.prepare("SELECT password_hash FROM users WHERE username=?");
    q.addBindValue(u);
    if (!q.exec()) { if (err) *err = q.lastError().text(); return false; }
    if (!q.next()) { if (err) *err = "User not found"; return false; }
    return q.value(0).toString() == hashPw(p);
}

int Database::userId(const QString& u) {
    QSqlQuery q;
    q.prepare("SELECT id FROM users WHERE username=?");
    q.addBindValue(u);
    if (!q.exec() || !q.next()) return -1;
    return q.value(0).toInt();
}

// Stubs for later steps (we’ll fill when we add the Parameters tab)
bool Database::upsertProfile(const ModeProfile& p, QString* err) {
    QSqlQuery q;
    q.prepare(
        "INSERT INTO mode_profiles(user_id,mode,lrl,url,atrial_amplitude,atrial_pulse_width,"
        "ventricular_amplitude,ventricular_pulse_width,arp,vrp)"
        "VALUES(?,?,?,?,?,?,?,?,?,?) "
        "ON CONFLICT(user_id,mode) DO UPDATE SET "
        "lrl=excluded.lrl, url=excluded.url, atrial_amplitude=excluded.atrial_amplitude, "
        "atrial_pulse_width=excluded.atrial_pulse_width, ventricular_amplitude=excluded.ventricular_amplitude, "
        "ventricular_pulse_width=excluded.ventricular_pulse_width, arp=excluded.arp, vrp=excluded.vrp, "
        "updated_at=datetime('now')"
        );
    q.addBindValue(p.userId);
    q.addBindValue(p.mode);
    q.addBindValue(p.lrl ? QVariant(*p.lrl) : QVariant());
    q.addBindValue(p.url ? QVariant(*p.url) : QVariant());
    q.addBindValue(p.aAmp ? QVariant(*p.aAmp) : QVariant());
    q.addBindValue(p.aPw  ? QVariant(*p.aPw)  : QVariant());
    q.addBindValue(p.vAmp ? QVariant(*p.vAmp) : QVariant());
    q.addBindValue(p.vPw  ? QVariant(*p.vPw)  : QVariant());
    q.addBindValue(p.arp  ? QVariant(*p.arp)  : QVariant());
    q.addBindValue(p.vrp  ? QVariant(*p.vrp)  : QVariant());
    if (!q.exec()) { if (err) *err = q.lastError().text(); return false; }
    return true;
}

std::optional<ModeProfile> Database::getProfile(int userId, const QString& mode, QString* err) {
    QSqlQuery q;
    q.prepare("SELECT lrl,url,atrial_amplitude,atrial_pulse_width,ventricular_amplitude,"
              "ventricular_pulse_width,arp,vrp FROM mode_profiles WHERE user_id=? AND mode=?");
    q.addBindValue(userId);
    q.addBindValue(mode);
    if (!q.exec()) { if (err) *err = q.lastError().text(); return std::nullopt; }
    if (!q.next()) return std::nullopt;

    ModeProfile p{userId, mode};
    auto getOptI = [&](int i)->std::optional<int>{ return q.value(i).isNull()?std::nullopt:std::make_optional(q.value(i).toInt()); };
    auto getOptD = [&](int i)->std::optional<double>{ return q.value(i).isNull()?std::nullopt:std::make_optional(q.value(i).toDouble()); };
    p.lrl = getOptI(0); p.url = getOptI(1);
    p.aAmp = getOptD(2); p.aPw = getOptD(3);
    p.vAmp = getOptD(4); p.vPw = getOptD(5);
    p.arp = getOptI(6);  p.vrp = getOptI(7);
    return p;
}
