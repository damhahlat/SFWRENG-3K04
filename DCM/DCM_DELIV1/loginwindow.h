#pragma once

// Include the dialog library
#include <QDialog>

// Suggestion by Generative AI to promote cohesion and good modularization
namespace Ui { class LoginWindow; }

// Creating new class LoginWindow, which is based on QDialog. We wants the login window to behave very similarly with a popup window.
class LoginWindow : public QDialog {
    Q_OBJECT
public:
    explicit LoginWindow(QWidget* parent = nullptr);
    ~LoginWindow() override; // Struggled with this, but apparently QDialog's virtual destructor needs to be overwridden in order to implement our own.

    QString username() const { return username_; }

// Behaves similarly to Qt signals.
private slots:
    void onRegisterClicked();
    void onLoginClicked();

private:
    Ui::LoginWindow* ui{nullptr};
    QString username_; // Stores username locally.
};
