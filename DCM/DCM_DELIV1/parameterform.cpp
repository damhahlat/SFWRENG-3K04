// Import necessary libraries and header files.
#include "parameterform.h"
#include "ui_parameterform.h"
#include "database.h"

#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QFormLayout>
#include <QtMath>

// Constructor. Partly written by Generative AI due to unfamiliarity with syntax and framework.
ParameterForm::ParameterForm(int userId, QWidget* parent)
    : QWidget(parent),
    ui(new Ui::ParameterForm),
    userId_(userId)
{
    ui->setupUi(this);

    // Create necessary buttons and text fields on the page.
    connect(ui->saveBtn,  &QPushButton::clicked, this, &ParameterForm::onSave);
    connect(ui->loadBtn,  &QPushButton::clicked, this, &ParameterForm::onLoad);
    connect(ui->clearBtn, &QPushButton::clicked, this, &ParameterForm::onClear);

    auto hook = [this](){ onFieldChanged(); };
    connect(ui->modeCombo, &QComboBox::currentTextChanged, this, hook);
    connect(ui->lrlSpin,   QOverload<int>::of(&QSpinBox::valueChanged), this, hook);
    connect(ui->urlSpin,   QOverload<int>::of(&QSpinBox::valueChanged), this, hook);
    connect(ui->aAmpSpin,  QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, hook);
    connect(ui->aPwSpin,   QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, hook);
    connect(ui->vAmpSpin,  QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, hook);
    connect(ui->vPwSpin,   QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, hook);
    connect(ui->arpSpin,   QOverload<int>::of(&QSpinBox::valueChanged), this, hook);
    connect(ui->vrpSpin,   QOverload<int>::of(&QSpinBox::valueChanged), this, hook);

    // Set defaults in the text fields.
    ui->lrlSpin->setRange(30, 175);
    ui->urlSpin->setRange(50, 175);
    ui->aAmpSpin->setRange(0.0, 7.0);
    ui->vAmpSpin->setRange(0.0, 7.0);
    ui->aPwSpin->setRange(0.05, 1.9);
    ui->vPwSpin->setRange(0.05, 1.9);
    ui->arpSpin->setRange(150, 500);
    ui->vrpSpin->setRange(150, 500);

    applyDefaults();
    restoreMode();
    onFieldChanged();
    emit statusMessage("Ready.");
}

// Destructor just deletes the UI itself.
ParameterForm::~ParameterForm() { delete ui; }

// This function returns the current mode that the user is on.
QString ParameterForm::mode() const { return ui->modeCombo->currentText(); }

// Due to unfamiliarity with the hardware, the code was written manually but extensive Generative AI usage was done to find appropriate values.
bool ParameterForm::checkAmplitudeStep(double v, QString* why) const {
    if (qFuzzyIsNull(v)) return true; // Off
    if (v >= 0.5 && v <= 3.2) {
        double nearest = qRound((v - 0.5) / 0.1) * 0.1 + 0.5;
        if (qFuzzyCompare(nearest + 1.0, v + 1.0)) return true;
        if (why) *why = "Amplitude must step by 0.1 V between 0.5 and 3.2 V.";
        return false;
    }
    if (v >= 3.5 && v <= 7.0) {
        double nearest = qRound((v - 3.5) / 0.5) * 0.5 + 3.5;
        if (qFuzzyCompare(nearest + 1.0, v + 1.0)) return true;
        if (why) *why = "Amplitude must step by 0.5 V between 3.5 and 7.0 V.";
        return false;
    }
    if (why) *why = "Amplitude must be Off (0), 0.5–3.2 V, or 3.5–7.0 V.";
    return false;
}

// Code was mostly written manually, but Generative AI was used to figure out what counts as "valid" in the context of the hardware, which is the DCM team is mostly unfamiliar with.
bool ParameterForm::validate(QString* err) const {
    const int lrl = ui->lrlSpin->value();
    const int url = ui->urlSpin->value();
    if (lrl >= url) { if (err) *err = "LRL must be less than URL."; return false; }

    auto stepOk = [&](int v)->bool{
        if (v >= 30 && v < 50) return (v - 30) % 5 == 0;
        if (v >= 50 && v <= 90) return true;
        if (v > 90 && v <= 175) return (v - 90) % 5 == 0;
        return false;
    };
    if (!stepOk(lrl)) { if (err) *err = "LRL step invalid: use 5 bpm in 30–50, 1 bpm in 50–90, 5 bpm in 90–175."; return false; }
    if ((url - 50) % 5 != 0) { if (err) *err = "URL must step by 5 bpm."; return false; }

    auto pwOk = [](double v)->bool{
        if (qFuzzyCompare(v + 1.0, 0.05 + 1.0)) return true;
        double nearest = qRound((v - 0.1) / 0.1) * 0.1 + 0.1;
        return qFuzzyCompare(nearest + 1.0, v + 1.0) && v >= 0.1 && v <= 1.9;
    };
    if (!pwOk(ui->aPwSpin->value())) { if (err) *err = "Atrial PW must be 0.05 ms or 0.1–1.9 ms in 0.1 ms steps."; return false; }
    if (!pwOk(ui->vPwSpin->value())) { if (err) *err = "Ventricular PW must be 0.05 ms or 0.1–1.9 ms in 0.1 ms steps."; return false; }

    QString why;
    if (!checkAmplitudeStep(ui->aAmpSpin->value(), &why)) { if (err) *err = "Atrial amplitude: " + why; return false; }
    if (!checkAmplitudeStep(ui->vAmpSpin->value(), &why)) { if (err) *err = "Ventricular amplitude: " + why; return false; }

    if (ui->arpSpin->value() % 10 != 0) { if (err) *err = "ARP must step by 10 ms."; return false; }
    if (ui->vrpSpin->value() % 10 != 0) { if (err) *err = "VRP must step by 10 ms."; return false; }

    return true;
}

// Hides or shows the field and its label. This keeps the layout tidy when modes are switched. This was a suggestion by Generative AI to clean the UI.
void ParameterForm::setFieldAndLabelVisible(QWidget* field, bool visible) const {
    if (field) field->setVisible(visible);
    // Try to find a QFormLayout that owns this field and toggle its label too
    const auto layouts = this->findChildren<QFormLayout*>();
    for (auto* fl : layouts) {
        if (auto* lbl = fl->labelForField(field)) {
            lbl->setVisible(visible);
        }
    }
}

// Shows controls for AOO, AAI, VOO, and VII. Filters between atrial and ventricular depending on mode.
void ParameterForm::applyModeVisibility(const QString& m) {
    bool atrial = (m == "AOO" || m == "AAI");
    bool vent   = (m == "VOO" || m == "VVI");

    setFieldAndLabelVisible(ui->aAmpSpin, atrial);
    setFieldAndLabelVisible(ui->aPwSpin,  atrial);
    setFieldAndLabelVisible(ui->vAmpSpin, vent);
    setFieldAndLabelVisible(ui->vPwSpin,  vent);
}

// Ensures that code is valid and therefore the user can save the current settings. Otherwise user can save invalid values.
void ParameterForm::reflectValidity() {
    QString err;
    const bool ok = validate(&err);
    ui->saveBtn->setEnabled(ok);
    emit statusMessage(ok ? QString{} : err);
}

// If a field is changed, ensure that the appropriate labels and visible and ensure that values are valid before letting the user save.
void ParameterForm::onFieldChanged() {
    applyModeVisibility(mode());
    reflectValidity();
}

// If the user wants to save the fields for a given mode, get the values and insert or update the database.
void ParameterForm::onSave() {
    QString err;
    if (!validate(&err)) { emit statusMessage(err); return; }

    ModeProfile p;
    p.userId = userId_;
    p.mode = mode();
    p.lrl = ui->lrlSpin->value();
    p.url = ui->urlSpin->value();
    p.aAmp = ui->aAmpSpin->value();
    p.aPw = ui->aPwSpin->value();
    p.vAmp= ui->vAmpSpin->value();
    p.vPw = ui->vPwSpin->value();
    p.arp = ui->arpSpin->value();
    p.vrp = ui->vrpSpin->value();

    if (Database::upsertProfile(p, &err)) {
        rememberMode(p.mode);
        emit statusMessage("Saved.");
    } else {
        emit statusMessage("Save failed: " + err);
    }
    reflectValidity();
}

// If the user wants to load the previous values, refer to the database and get those values.
// Due to bugs, development was partly done by Generative AI.
void ParameterForm::onLoad() {
    QString err;
    const auto m = mode();
    auto opt = Database::getProfile(userId_, m, &err);
    if (!opt.has_value()) {
        emit statusMessage(err.isEmpty() ? "No saved values for this mode." : "Load failed: " + err);
        return;
    }
    const ModeProfile& p = *opt;
    if (p.lrl)ui->lrlSpin->setValue(*p.lrl);
    if (p.url) ui->urlSpin->setValue(*p.url);
    if (p.aAmp) ui->aAmpSpin->setValue(*p.aAmp);
    if (p.aPw) ui->aPwSpin->setValue(*p.aPw);
    if (p.vAmp) ui->vAmpSpin->setValue(*p.vAmp);
    if (p.vPw) ui->vPwSpin->setValue(*p.vPw);
    if (p.arp) ui->arpSpin->setValue(*p.arp);
    if (p.vrp)  ui->vrpSpin->setValue(*p.vrp);

    rememberMode(mode());
    emit statusMessage("Loaded.");
    reflectValidity();
}

// If user wants to clear, just apply the defaults and also ensure that the values are valid (paranoia but also maybe good practice?).
void ParameterForm::onClear() {
    applyDefaults();
    emit statusMessage("Cleared (unsaved).");
    reflectValidity();
}

// Apparently code fails if we don't have this I have no clue myself :(
void ParameterForm::clearAll() { onClear(); }

// Just the default values.
void ParameterForm::applyDefaults() {
    ui->modeCombo->setCurrentText("VVI");
    ui->lrlSpin->setValue(60);
    ui->urlSpin->setValue(120);
    ui->aAmpSpin->setValue(3.5);
    ui->aPwSpin->setValue(0.4);
    ui->vAmpSpin->setValue(3.5);
    ui->vPwSpin->setValue(0.4);
    ui->arpSpin->setValue(250);
    ui->vrpSpin->setValue(320);
}

// If you're reading this, enjoy this epic ASCII duck (4 am debugging session i'm going insane):
//        _
//        .__(.)<
//         \___)
//  ~~~~~~~~~~~~~~~~~~

void ParameterForm::rememberMode(const QString& m) { settings_.setValue("lastMode", m); }

void ParameterForm::restoreMode() {
    const QString m = settings_.value("lastMode").toString();
    if (!m.isEmpty()) ui->modeCombo->setCurrentText(m);
}

// This just makes a hashmap of current UI values which is used in buildReportHTML. Yet another suggestion and partial implementation by Generative AI.
QMap<QString, QString> ParameterForm::currentValuesAsText() const {
    QMap<QString, QString> kv;
    kv["Mode"] = mode();
    kv["LRL (bpm)"] = QString::number(ui->lrlSpin->value());
    kv["URL (bpm)"] = QString::number(ui->urlSpin->value());
    kv["Atrial Amplitude (V)"] = QString::number(ui->aAmpSpin->value(), 'f', 2);
    kv["Atrial Pulse Width (ms)"] = QString::number(ui->aPwSpin->value(), 'f', 2);
    kv["Ventricular Amplitude (V)"] = QString::number(ui->vAmpSpin->value(), 'f', 2);
    kv["Ventricular Pulse Width (ms)"] = QString::number(ui->vPwSpin->value(), 'f', 2);
    kv["ARP (ms)"] = QString::number(ui->arpSpin->value());
    kv["VRP (ms)"] = QString::number(ui->vrpSpin->value());
    return kv;
}
