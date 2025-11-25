#pragma once

#include <QDialog>
#include <QString>
#include "serialmanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class SerialTestDialog; }
QT_END_NAMESPACE

class SerialTestDialog : public QDialog {
    Q_OBJECT

public:
    explicit SerialTestDialog(QWidget* parent = nullptr);
    ~SerialTestDialog() override;

private slots:
    void onRefreshPorts();
    void onConnectClicked();
    void onDisconnectClicked();
    void onSendClicked();
    void onErrorMessage(const QString& msg);

private:
    Ui::SerialTestDialog* ui;
    SerialManager manager_;
};
