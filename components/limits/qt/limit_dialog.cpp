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
      QString::fromStdWString(model_.GetSourceTitle()));

  auto limits = model_.GetLimits();
  ui.loEdit->setText(QString::fromStdWString(limits.lo));
  ui.hiEdit->setText(QString::fromStdWString(limits.hi));
  ui.loLoEdit->setText(QString::fromStdWString(limits.lolo));
  ui.hiHiEdit->setText(QString::fromStdWString(limits.hihi));
}

void LimitDialog::accept() {
  LimitModel::Limits limits = {};
  limits.lo = ui.loEdit->text().toStdWString();
  limits.hi = ui.hiEdit->text().toStdWString();
  limits.lolo = ui.loLoEdit->text().toStdWString();
  limits.hihi = ui.hiHiEdit->text().toStdWString();
  model_.WriteLimits(limits);

  QDialog::accept();
}

void ShowLimitsDialog(DialogService& dialog_service,
                      LimitDialogContext&& context) {
  LimitModel model{std::move(context)};
  LimitDialog dialog{model, dialog_service.GetParentWidget()};
  dialog.exec();
}
