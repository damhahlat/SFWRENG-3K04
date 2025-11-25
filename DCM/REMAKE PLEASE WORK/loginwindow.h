#pragma once

#include <QDialog>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui { class LoginWindow; }
QT_END_NAMESPACE

class LoginWindow : public QDialog {
    Q_OBJECT

public:
    explicit LoginWindow(QWidget* parent = nullptr);
    ~LoginWindow() override;

    // Used by main.cpp after exec()
    QString username() const;

signals:
    void loginSuccess(int userId, const QString& username);

private slots:
    void onLogin();      // OK button
    void onRegister();   // Cancel button

private:
    Ui::LoginWindow* ui;
};
