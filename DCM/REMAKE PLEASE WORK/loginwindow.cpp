#include "loginwindow.h"
#include "ui_loginwindow.h"
#include "database.h"

#include <QPushButton>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QSqlQuery>

LoginWindow::LoginWindow(QWidget* parent)
    : QDialog(parent), ui(new Ui::LoginWindow)
{
    ui->setupUi(this);
    ui->statusLabel->clear();

    // Set up buttons explicitly
    ui->buttonBox->clear();
    QPushButton* loginBtn = ui->buttonBox->addButton("Login", QDialogButtonBox::AcceptRole);
    QPushButton* registerBtn = ui->buttonBox->addButton("Register", QDialogButtonBox::ActionRole);

    connect(loginBtn, &QPushButton::clicked, this, &LoginWindow::onLogin);
    connect(registerBtn, &QPushButton::clicked, this, &LoginWindow::onRegister);
}

LoginWindow::~LoginWindow() {
    delete ui;
}

QString LoginWindow::username() const {
    return ui->userEdit->text().trimmed();
}

void LoginWindow::onLogin() {
    QString uname = ui->userEdit->text().trimmed();
    QString password = ui->passEdit->text().trimmed();

    // Validation
    if (uname.isEmpty() || password.isEmpty()) {
        ui->statusLabel->setText("Username and password cannot be empty.");
        return;
    }

    QString err;
    if (!Database::loginUser(uname, password, &err)) {
        ui->statusLabel->setText("Login failed: Invalid username or password.");
        return;
    }

    int id = Database::userId(uname);
    if (id < 0) {
        ui->statusLabel->setText("Error: Account not found.");
        return;
    }

    emit loginSuccess(id, uname);
    accept();
}

void LoginWindow::onRegister() {
    QString uname = ui->userEdit->text().trimmed();
    QString password = ui->passEdit->text().trimmed();

    // Validation
    if (uname.isEmpty() || password.isEmpty()) {
        ui->statusLabel->setText("Username and password cannot be empty.");
        return;
    }

    if (uname.length() < 3) {
        ui->statusLabel->setText("Username must be at least 3 characters.");
        return;
    }

    if (password.length() < 4) {
        ui->statusLabel->setText("Password must be at least 4 characters.");
        return;
    }

    // Check max users (10)
    QSqlQuery q;
    q.prepare("SELECT COUNT(*) FROM users");
    if (q.exec() && q.next()) {
        int userCount = q.value(0).toInt();
        if (userCount >= 10) {
            ui->statusLabel->setText("Maximum number of users (10) reached.");
            return;
        }
    }

    // Check if username already exists
    q.prepare("SELECT id FROM users WHERE username=?");
    q.addBindValue(uname);
    if (q.exec() && q.next()) {
        ui->statusLabel->setText("Username already exists. Please choose another.");
        return;
    }

    QString err;
    if (!Database::registerUser(uname, password, &err)) {
        ui->statusLabel->setText("Registration failed: " + err);
        return;
    }

    ui->statusLabel->setText("✓ Registration successful! You can now log in.");
    ui->userEdit->clear();
    ui->passEdit->clear();
}
