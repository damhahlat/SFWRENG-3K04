#include "parameterform.h"
#include "ui_parameterform.h"
#include "database.h"

#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>

ParameterForm::ParameterForm(int userId, QWidget* parent)
    : QWidget(parent),
    ui(new Ui::ParameterForm),
    userId_(userId)
{
    ui->setupUi(this);

    // Buttons
    connect(ui->saveBtn,  &QPushButton::clicked, this, &ParameterForm::onSave);
    connect(ui->loadBtn,  &QPushButton::clicked, this, &ParameterForm::onLoad);
    connect(ui->clearBtn, &QPushButton::clicked, this, &ParameterForm::onClear);

    // Live validation
    auto hook = [this](){ onFieldChanged(); };
    connect(ui->modeCombo, &QComboBox::currentTextChanged, this, hook);
    connect(ui->lrlSpin,   &QSpinBox::valueChanged,        this, hook);
    connect(ui->urlSpin,   &QSpinBox::valueChanged,        this, hook);
    connect(ui->aAmpSpin,  QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, hook);
    connect(ui->aPwSpin,   QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, hook);
    connect(ui->vAmpSpin,  QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, hook);
    connect(ui->vPwSpin,   QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, hook);
    connect(ui->arpSpin,   &QSpinBox::valueChanged,        this, hook);
    connect(ui->vrpSpin,   &QSpinBox::valueChanged,        this, hook);

    applyDefaults();
    restoreMode();
    onFieldChanged();   // initial validity
    emit statusMessage("Ready.");
}

ParameterForm::~ParameterForm() { delete ui; }

QString ParameterForm::mode() const { return ui->modeCombo->currentText(); }

bool ParameterForm::validate(QString* err) const {
    const int lrl = ui->lrlSpin->value();
    const int url = ui->urlSpin->value();
    if (lrl >= url) { if (err) *err = "LRL must be less than URL."; return false; }

    const QString m = mode();
    if ((m == "AOO" || m == "AAI") && ui->aPwSpin->value() < 0.05) {
        if (err) *err = "Atrial PW must be ≥ 0.05 ms."; return false;
    }
    if ((m == "VOO" || m == "VVI") && ui->vPwSpin->value() < 0.05) {
        if (err) *err = "Ventricular PW must be ≥ 0.05 ms."; return false;
    }
    return true;
}

void ParameterForm::reflectValidity() {
    QString err;
    const bool ok = validate(&err);
    ui->saveBtn->setEnabled(ok);
    emit statusMessage(ok ? QString{} : err);
}

void ParameterForm::applyDefaults() {
    ui->modeCombo->setCurrentIndex(0);
    ui->lrlSpin->setValue(60);
    ui->urlSpin->setValue(120);
    ui->aAmpSpin->setValue(3.5);
    ui->aPwSpin->setValue(0.4);
    ui->vAmpSpin->setValue(3.5);
    ui->vPwSpin->setValue(0.4);
    ui->arpSpin->setValue(250);
    ui->vrpSpin->setValue(250);
}

void ParameterForm::rememberMode(const QString& m) {
    settings_.setValue("lastMode", m);
}

void ParameterForm::restoreMode() {
    const QString m = settings_.value("lastMode").toString();
    if (!m.isEmpty()) {
        const int idx = ui->modeCombo->findText(m);
        if (idx >= 0) ui->modeCombo->setCurrentIndex(idx);
    }
}

void ParameterForm::onFieldChanged() {
    reflectValidity();
}

void ParameterForm::onSave() {
    QString err;
    if (!validate(&err)) { emit statusMessage(err); return; }

    ModeProfile p;
    p.userId = userId_;
    p.mode = mode();
    p.lrl = ui->lrlSpin->value();
    p.url = ui->urlSpin->value();
    p.aAmp = ui->aAmpSpin->value();
    p.aPw  = ui->aPwSpin->value();
    p.vAmp = ui->vAmpSpin->value();
    p.vPw  = ui->vPwSpin->value();
    p.arp  = ui->arpSpin->value();
    p.vrp  = ui->vrpSpin->value();

    if (Database::upsertProfile(p, &err)) {
        rememberMode(p.mode);
        emit statusMessage("Saved.");
    } else {
        emit statusMessage("Save failed: " + err);
    }
    reflectValidity();
}

void ParameterForm::onLoad() {
    QString err;
    auto res = Database::getProfile(userId_, mode(), &err);
    if (!err.isEmpty()) { emit statusMessage("Load failed: " + err); return; }
    if (!res) { emit statusMessage("No saved profile for this mode."); return; }

    const ModeProfile& p = *res;
    if (p.lrl) ui->lrlSpin->setValue(*p.lrl);
    if (p.url) ui->urlSpin->setValue(*p.url);
    if (p.aAmp) ui->aAmpSpin->setValue(*p.aAmp);
    if (p.aPw)  ui->aPwSpin->setValue(*p.aPw);
    if (p.vAmp) ui->vAmpSpin->setValue(*p.vAmp);
    if (p.vPw)  ui->vPwSpin->setValue(*p.vPw);
    if (p.arp)  ui->arpSpin->setValue(*p.arp);
    if (p.vrp)  ui->vrpSpin->setValue(*p.vrp);

    rememberMode(mode());
    emit statusMessage("Loaded.");
    reflectValidity();
}

void ParameterForm::onClear() {
    applyDefaults();
    emit statusMessage("Cleared (unsaved).");
    reflectValidity();
}
