#include "components/limits/limit_dialog.h"

#include "components/limits/limit_model.h"
#include "qt/dialog_util.h"
#include "services/dialog_service.h"
#include "ui_limit_dialog.h"

class LimitDialog : public QDialog {
  Q_OBJECT

 public:
  explicit LimitDialog(std::unique_ptr<LimitModel> model,
                       QWidget* parent = nullptr);

 public Q_SLOTS:
  virtual void accept() override;

 private:
  Ui::LimitDialog ui;

  std::unique_ptr<LimitModel> model_;
};

#include "limit_dialog.moc"

LimitDialog::LimitDialog(std::unique_ptr<LimitModel> model, QWidget* parent)
    : QDialog{parent}, model_{std::move(model)} {
  ui.setupUi(this);

  ui.descriptionLabel->setText(
      QString::fromStdU16String(model_->GetSourceTitle()));

  auto limits = model_->GetLimits();
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
  model_->WriteLimits(limits);

  QDialog::accept();
}

void ShowLimitsDialog(DialogService& dialog_service,
                      LimitDialogContext&& context) {
  auto model = std::make_unique<LimitModel>(std::move(context));
  auto dialog = std::make_unique<LimitDialog>(std::move(model),
                                              dialog_service.GetParentWidget());
  StartModalDialog(std::move(dialog));
}
