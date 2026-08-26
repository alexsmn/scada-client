#include "modules/limits/limit_dialog.h"

#include "aui/dialog_service.h"
#include "aui/qt/dialog_service_impl_qt.h"
#include "aui/qt/dialog_util.h"
#include "modules/limits/limit_model.h"
#include "ui_limit_dialog.h"

#include <QPushButton>

#include <QDialogButtonBox>

class LimitDialog : public QDialog {
  Q_OBJECT

 public:
  explicit LimitDialog(std::shared_ptr<LimitModel> model,
                       QWidget* parent = nullptr);

 public Q_SLOTS:
  virtual void accept() override;

 private:
  Ui::LimitDialog ui;

  const std::shared_ptr<LimitModel> model_;
  DialogServiceImplQt dialog_service_;
};

#include "limit_dialog.moc"

LimitDialog::LimitDialog(std::shared_ptr<LimitModel> model, QWidget* parent)
    : QDialog{parent}, model_{std::move(model)} {
  ui.setupUi(this);

  // The error box belongs to this dialog, which is still on screen when a
  // write is refused.
  dialog_service_.parent_widget = this;
  model_->set_dialog_service(&dialog_service_);

  // The write decides when the dialog closes. Apply is re-enabled either way,
  // so a refused edit can be corrected and retried on the spot.
  model_->completion_handler = [this](bool ok) {
    ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    if (ok)
      QDialog::accept();
  };

  // Name the action rather than the assent (docs/client/ux/dialogs.md §3).
  ui.buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Apply"));

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
  // Deliberately not QDialog::accept(): the post completes a turn or more
  // later, and closing here is what made a refused write indistinguishable
  // from a successful one. `completion_handler` closes the dialog once the
  // write has actually succeeded.
  ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
  model_->WriteLimits(limits);
}

Awaitable<void> ShowLimitsDialog(DialogService& dialog_service,
                                 LimitDialogContext context) {
  auto model = std::make_shared<LimitModel>(std::move(context));
  auto dialog = std::make_unique<LimitDialog>(std::move(model),
                                              dialog_service.GetParentWidget());
  return StartOwnedModalDialog(std::move(dialog));
}
