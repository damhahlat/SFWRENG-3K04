#pragma once

// Include necessary libraries.
#include <QWidget>
#include <QSettings>
#include <QMap>
#include <QString>

// Suggestion by Generative AI to improve cohesion.
QT_BEGIN_NAMESPACE
namespace Ui { class ParameterForm; }
QT_END_NAMESPACE

// The Parameter Form class is built on the QWidget class, since it behaves similarly.
class ParameterForm : public QWidget {
    Q_OBJECT
public:

    // Constructor and destructor.
    explicit ParameterForm(int userId, QWidget* parent = nullptr);
    ~ParameterForm() override;

    // The values for the parameter are stored as text (ex. AOO and corresponding value are both text instead of text and a double).
    // This is to account for bad user input.
    QMap<QString, QString> currentValuesAsText() const;

// Suggestion by Generative AI. This is how status messages can be written.
signals:
    void statusMessage(const QString& msg);

// Just clears and resets all the defaults.
public slots:
    void clearAll();

// If the user changes a field, saves, loads, or clears, perform corresponding actions.
private slots:
    void onFieldChanged();
    void onSave();
    void onLoad();
    void onClear();

// Just necessary values that might be helpful later on.
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
    void applyModeVisibility(const QString& m);
    bool checkAmplitudeStep(double v, QString* why) const;

    // helper to show/hide a field and its label regardless of object names
    void setFieldAndLabelVisible(QWidget* field, bool visible) const;
};
