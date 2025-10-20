#pragma once
#include <QWidget>
#include <QSettings>

QT_BEGIN_NAMESPACE
namespace Ui { class ParameterForm; }
QT_END_NAMESPACE

class ParameterForm : public QWidget {
    Q_OBJECT
public:
    explicit ParameterForm(int userId, QWidget* parent = nullptr);
    ~ParameterForm() override;

signals:
    void statusMessage(const QString& msg);

private slots:
    void onSave();
    void onLoad();
    void onClear();
    void onFieldChanged();                 // live validation

private:
    Ui::ParameterForm* ui;
    int userId_;
    QSettings settings_{"McMaster", "PacemakerDCM"};

    bool validate(QString* err) const;
    void reflectValidity();
    QString mode() const;
    void applyDefaults();
    void rememberMode(const QString& m);
    void restoreMode();
};
