#pragma once
#include <QDialog>

namespace Ui { class LoginWindow; }

class LoginWindow : public QDialog {
    Q_OBJECT
public:
    explicit LoginWindow(QWidget* parent = nullptr);
    ~LoginWindow() override;                 // DECLARED (must be DEFINED in .cpp)

    QString username() const { return username_; }

private slots:
    void onRegisterClicked();
    void onLoginClicked();

private:
    Ui::LoginWindow* ui{nullptr};
    QString username_;
};
