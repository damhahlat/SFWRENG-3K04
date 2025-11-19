#pragma once

#include <QDialog>

class SerialManager;

namespace Ui {
class SerialTestDialog;
}

class SerialTestDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SerialTestDialog(QWidget *parent = nullptr);
    ~SerialTestDialog();

private slots:
    void on_btnRefresh_clicked();
    void on_btnConnect_clicked();
    void on_btnSend_clicked();

    void handleDataReceived(const QByteArray &data);
    void handleError(const QString &message);
    void handleConnected(const QString &portName, qint32 baudRate);
    void handleDisconnected();

private:
    void populatePorts();
    void setupBaudRates();

private:
    Ui::SerialTestDialog *ui;
    SerialManager *m_serial;
    bool m_connected;
};
