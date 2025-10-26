// Include the header file and some necessary libraries
#include "database.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QDir>

// Stores the database
static QSqlDatabase db;

// This function just hashes the password. Suggestion by Generative AI (ChatGPT).
static QString hashPw(const QString& pw) {
    return QString::fromUtf8(QCryptographicHash::hash(pw.toUtf8(), QCryptographicHash::Sha256).toHex());
}

// This function automatically chooses a path for the database depending on the OS.
// This was another recommendation by Generative AI (ChatGPT) instead of locally storing it within the same directory, which leads to security vulnerabilities.
QString Database::path() {
    auto dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/dcm.db";
}

// Creates the database by connecting to the actual path.
bool Database::init(QString* err) {

    // Double check to see if SQLite is available.
    if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
        if (err) *err = "QSQLITE driver not available";
        return false;
    }

    // Opens the database, and returns an error if not possible.
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(path());
    if (!db.open()) {
        if (err) *err = db.lastError().text();
        return false;
    }

    // Run DDL statements to create the necessary tables in the database
    const QStringList ddl = {

        // Table that stores the necessary users.
        "CREATE TABLE IF NOT EXISTS users ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " username TEXT UNIQUE NOT NULL,"
        " password_hash TEXT NOT NULL,"
        " created_at TEXT NOT NULL DEFAULT (datetime('now'))"
        ")",

        // Table to store mode profiles for each user.
        // The DDL for this table was created in conjunction with Generative AI to account for best practices.
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

    // Iterate through the entire DDL and run it. If there's an error return false.
    for (const auto& sql : ddl) {
        QSqlQuery q;
        if (!q.exec(sql)) {
            if (err) *err = q.lastError().text();
            return false;
        }
    }

    // Database is successfully created.
    return true;
}

// Function to register the user.
bool Database::registerUser(const QString& u, const QString& p, QString* err) {

    // Ensure that the user count doesn't exceed 10.
    QSqlQuery count("SELECT COUNT(*) FROM users");
    if (!count.next()) { if (err) *err = count.lastError().text(); return false; } // This line is a recommendation by ChatGPT to ensure that data is actually retrieved
    if (count.value(0).toInt() >= 10) { if (err) *err = "User limit (10) reached"; return false; }

    // Ensure that a duplicate user doesn't exist.
    QSqlQuery dupe;
    dupe.prepare("SELECT 1 FROM users WHERE username=?");
    dupe.addBindValue(u);
    if (!dupe.exec()) { if (err) *err = dupe.lastError().text(); return false; }
    if (dupe.next()) { if (err) *err = "Username already exists"; return false; }

    // Inserts new user into the database.
    QSqlQuery ins;
    ins.prepare("INSERT INTO users(username, password_hash) VALUES(?, ?)");
    ins.addBindValue(u);
    ins.addBindValue(hashPw(p));
    if (!ins.exec()) { if (err) *err = ins.lastError().text(); return false; }
    return true;
}

// Function to login. Finds user, ensure password hatches match, and returns whether or not credentials work.
bool Database::loginUser(const QString& u, const QString& p, QString* err) {
    QSqlQuery q;
    q.prepare("SELECT password_hash FROM users WHERE username=?");
    q.addBindValue(u);
    if (!q.exec()) { if (err) *err = q.lastError().text(); return false; }
    if (!q.next()) { if (err) *err = "User not found"; return false; }
    return q.value(0).toString() == hashPw(p);
}

// Function to find the userID based on the username.
int Database::userId(const QString& u) {
    QSqlQuery q;
    q.prepare("SELECT id FROM users WHERE username=?");
    q.addBindValue(u);
    if (!q.exec() || !q.next()) return -1; // If not found return -1 instead.
    return q.value(0).toInt();
}

// Stubs for later steps (we’ll fill when we add the Parameters tab)
// We struggled with the syntax to get this to work, so this was written in conjunction with Generative AI.
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

// Just retrives the mode profile, depending on the user and the mode selected.
std::optional<ModeProfile> Database::getProfile(int userId, const QString& mode, QString* err) {
    QSqlQuery q;
    q.prepare("SELECT lrl,url,atrial_amplitude,atrial_pulse_width,ventricular_amplitude,"
              "ventricular_pulse_width,arp,vrp FROM mode_profiles WHERE user_id=? AND mode=?");
    q.addBindValue(userId);
    q.addBindValue(mode);
    if (!q.exec()) { if (err) *err = q.lastError().text(); return std::nullopt; }
    if (!q.next()) return std::nullopt;

    // Mode Profile structure which is updated based on stores data.
    ModeProfile p{userId, mode};
    auto getOptI = [&](int i)->std::optional<int>{ return q.value(i).isNull()?std::nullopt:std::make_optional(q.value(i).toInt()); }; // Due to bugs this line was written with Generative AI
    auto getOptD = [&](int i)->std::optional<double>{ return q.value(i).isNull()?std::nullopt:std::make_optional(q.value(i).toDouble()); }; // Due to bugs Generative AI.

    // Update local data with now retrieved database data.
    p.lrl = getOptI(0); p.url = getOptI(1);
    p.aAmp = getOptD(2); p.aPw = getOptD(3);
    p.vAmp = getOptD(4); p.vPw = getOptD(5);
    p.arp = getOptI(6);  p.vrp = getOptI(7);
    return p;
}
