#include "components/limits/limit_dialog.h"

#include "components/limits/limit_model.h"
#include "services/dialog_service.h"
#include "ui_limit_dialog.h"

class LimitDialog : public QDialog {
  Q_OBJECT

 public:
  explicit LimitDialog(LimitModel& model, QWidget* parent = nullptr);

 public Q_SLOTS:
  virtual void accept() override;

 private:
  Ui::LimitDialog ui;

  LimitModel& model_;
};

#include "limit_dialog.moc"

LimitDialog::LimitDialog(LimitModel& model, QWidget* parent)
    : QDialog{parent}, model_{model} {
  ui.setupUi(this);

  ui.descriptionLabel->setText(
      QString::fromStdU16String(model_.GetSourceTitle()));

  auto limits = model_.GetLimits();
  ui.loEdit->setText(QString::fromStdU16String(limits.lo));
  ui.hiEdit->setText(QString::fromStdU16String(limits.hi));
  ui.loLoEdit->setText(QString::fromStdU16String(limits.lolo));
  ui.hiHiEdit->setText(QString::fromStdU16String(limits.hihi));
}

void LimitDialog::accept() {
  LimitModel::Limits limits = {};
  limits.lo = ui.loEdit->text().toStdU16String();
  limits.hi = ui.hiEdit->text().toStdU16String();
  limits.lolo = ui.loLoEdit->text().toStdU16String();
  limits.hihi = ui.hiHiEdit->text().toStdU16String();
  model_.WriteLimits(limits);

  QDialog::accept();
}

void ShowLimitsDialog(DialogService& dialog_service,
                      LimitDialogContext&& context) {
  LimitModel model{std::move(context)};
  LimitDialog dialog{model, dialog_service.GetParentWidget()};
  dialog.exec();
}
