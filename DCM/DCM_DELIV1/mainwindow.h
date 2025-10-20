#pragma once
#include <QMainWindow>

namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(int userId, const QString& username, QWidget* parent = nullptr);
    ~MainWindow() override;

private:
    Ui::MainWindow* ui;
    int userId_;
    QString username_;

private slots:
    void onOpenDbFolder();
};
