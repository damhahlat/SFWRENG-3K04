// Import necessary header files and libraries
#include "loginwindow.h"
#include "ui_loginwindow.h"
#include "database.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QLineEdit>

// Constructor for the LoginWindow.
// Generative AI was used to implement some of this due to unfamiliarity with Qt and consistent bugs.
LoginWindow::LoginWindow(QWidget* parent)
    : QDialog(parent), ui(new Ui::LoginWindow) {
    ui->setupUi(this);

    // Creates the login and register buttons on the actual page itself and connects them to related functions.
    auto* btnRegister = ui->buttonBox->addButton("Register", QDialogButtonBox::ActionRole);
    auto* btnLogin    = ui->buttonBox->addButton("Login",    QDialogButtonBox::AcceptRole);
    connect(btnRegister, &QPushButton::clicked, this, &LoginWindow::onRegisterClicked);
    connect(btnLogin,    &QPushButton::clicked, this, &LoginWindow::onLoginClicked);

    ui->passEdit->setEchoMode(QLineEdit::Password);

    QString err;
    if (!Database::init(&err)) {
        ui->statusLabel->setText("Database error: " + err);
        ui->buttonBox->setEnabled(false);
    }
}

// Destructor just deletes the ui window.
LoginWindow::~LoginWindow() { delete ui; }

// When "register" is clicked, the username and password are retrieved and the registerUser function is run from the database namespace.
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

// If "login" is clicked, the username and password and matched with the credentials.
void LoginWindow::onLoginClicked() {
    QString err;
    const auto u = ui->userEdit->text().trimmed();
    const auto p = ui->passEdit->text();
    if (u.isEmpty() || p.isEmpty()) { ui->statusLabel->setText("Enter username and password."); return; }
    if (!Database::loginUser(u, p, &err)) { ui->statusLabel->setText(err.isEmpty() ? "Invalid credentials." : err); return; }
    username_ = u;
    accept();
}
