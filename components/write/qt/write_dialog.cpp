#include "components/write/write_dialog.h"

#include "components/write/write_model.h"
#include "qt/dialog_service_impl_qt.h"
#include "qt/dialog_util.h"
#include "ui_write_dialog.h"

class WriteDialog : public QDialog {
  Q_OBJECT

 public:
  explicit WriteDialog(std::shared_ptr<WriteModel> model,
                       QWidget* parent = nullptr);

 public Q_SLOTS:
  virtual void accept() override;

 private:
  void UpdateCurrent();
  void UpdateCondition();
  void UpdateStatus();

  Ui::WriteDialog ui;

  const std::shared_ptr<WriteModel> model_;
  DialogServiceImplQt dialog_service_;
};

#include "write_dialog.moc"

WriteDialog::WriteDialog(std::shared_ptr<WriteModel> model, QWidget* parent)
    : QDialog{parent}, model_{std::move(model)} {
  ui.setupUi(this);

  dialog_service_.parent_widget = this;
  model_->set_dialog_service(&dialog_service_);

  setWindowTitle(QString::fromStdU16String(model_->GetWindowTitle()));

  ui.descriptionLabel->setText(
      QString::fromStdU16String(model_->GetSourceTitle()));
  ui.currentValueLabel->setText(
      QString::fromStdU16String(model_->GetCurrentValue(true)));

  ui.valueComboBox->setEditable(!model_->discrete());

  ui.unitLabel->setText({});
  ui.conditionLabel->setText({});
  ui.statusLabel->setText({});
  ui.lockLabel->setVisible(model_->lock_allowed());
  ui.lockCheckBox->setVisible(model_->lock_allowed());
  ui.lockCheckBox->setChecked(model_->locked());
  ui.conditionHeaderLabel->setVisible(model_->has_condition());
  ui.conditionLabel->setVisible(model_->has_condition());

  if (model_->discrete()) {
    for (const auto& state : model_->GetDiscreteStates())
      ui.valueComboBox->addItem(QString::fromStdU16String(state));
    ui.valueComboBox->setCurrentIndex(model_->GetCurrentDiscreteState());

  } else {
    ui.valueComboBox->setCurrentText(
        QString::fromStdU16String(model_->GetCurrentValue(false)));
    ui.unitLabel->setText(QString::fromStdU16String(model_->GetAnalogUnits()));
  }

  model_->current_change_handler = [this] { UpdateCurrent(); };
  model_->condition_change_handler = [this] { UpdateCondition(); };
  model_->status_change_handler = [this] { UpdateStatus(); };

  model_->completion_handler = [this](bool ok) {
    ui.statusLabel->setText({});
    if (ok)
      QDialog::accept();
  };

  UpdateCurrent();
  UpdateCondition();
  UpdateStatus();
}

void WriteDialog::UpdateCurrent() {
  ui.currentValueLabel->setText(
      QString::fromStdU16String(model_->GetCurrentValue(true)));
}

void WriteDialog::UpdateCondition() {
  ui.conditionLabel->setText(model_->IsConditionOk() ? tr("Satisfied")
                                                     : tr("Unsatisfied"));
  ui.okButton->setEnabled(model_->IsConditionOk());
}

void WriteDialog::UpdateStatus() {
  ui.statusLabel->setText(QString::fromStdU16String(model_->GetStatusText()));
}

void WriteDialog::accept() {
  double value = 0.0;
  if (model_->discrete()) {
    auto index = ui.valueComboBox->currentIndex();
    value = index ? 1.0 : 0.0;

  } else {
    bool ok = false;
    value = ui.valueComboBox->currentText().toDouble(&ok);
    if (!ok) {
      dialog_service_.RunMessageBox(
          tr("Incorrect floating point value.").toStdU16String(), {},
          MessageBoxMode::Error);
      return;
    }
  }

  bool lock = false;
  if (model_->lock_allowed())
    lock = ui.lockCheckBox->isChecked();

  model_->Write(value, lock);
}

void ExecuteWriteDialog(DialogService& dialog_service, WriteContext&& context) {
  auto model = std::make_shared<WriteModel>(std::move(context));
  auto dialog =
      std::make_unique<WriteDialog>(model, dialog_service.GetParentWidget());
  StartModalDialog(std::move(dialog));
}
