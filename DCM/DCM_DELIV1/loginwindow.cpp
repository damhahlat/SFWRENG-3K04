#include "loginwindow.h"
#include "ui_loginwindow.h"
#include "database.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QLineEdit>

LoginWindow::LoginWindow(QWidget* parent)
    : QDialog(parent), ui(new Ui::LoginWindow) {
    ui->setupUi(this);

    auto* btnRegister = ui->buttonBox->addButton("Register", QDialogButtonBox::ActionRole);
    auto* btnLogin    = ui->buttonBox->addButton("Login",    QDialogButtonBox::AcceptRole);

    connect(btnRegister, &QPushButton::clicked, this, &LoginWindow::onRegisterClicked);
    connect(btnLogin,    &QPushButton::clicked, this, &LoginWindow::onLoginClicked);
    connect(ui->passEdit, &QLineEdit::returnPressed, btnLogin, &QPushButton::click);
}

LoginWindow::~LoginWindow() {                 // <-- DEFINED here
    delete ui;
}

void LoginWindow::onRegisterClicked() {
    QString err;
    const auto u = ui->userEdit->text().trimmed();
    const auto p = ui->passEdit->text();
    if (u.isEmpty() || p.isEmpty()) { ui->statusLabel->setText("Enter username and password."); return; }
    if (Database::registerUser(u, p, &err))
        ui->statusLabel->setText("Registered. You can log in.");
    else
        ui->statusLabel->setText("Error: " + err);
}

void LoginWindow::onLoginClicked() {
    QString err;
    const auto u = ui->userEdit->text().trimmed();
    const auto p = ui->passEdit->text();
    if (u.isEmpty() || p.isEmpty()) { ui->statusLabel->setText("Enter username and password."); return; }
    if (!Database::loginUser(u, p, &err)) { ui->statusLabel->setText(err.isEmpty() ? "Invalid credentials." : err); return; }
    username_ = u;
    accept();
}
