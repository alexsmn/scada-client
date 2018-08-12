#include "components/write/write_dialog.h"

#include "components/write/write_model.h"
#include "qt/dialog_service_impl_qt.h"
#include "ui_write_dialog.h"

class WriteDialog : public QDialog {
  Q_OBJECT

 public:
  explicit WriteDialog(WriteModel& model, QWidget* parent = nullptr);

 public Q_SLOTS:
  virtual void accept() override;

 private:
  Ui::WriteDialog ui;

  WriteModel& model_;
  DialogServiceImplQt dialog_service_;
};

#include "write_dialog.moc"

WriteDialog::WriteDialog(WriteModel& model, QWidget* parent)
    : QDialog{parent}, model_{model} {
  ui.setupUi(this);
  dialog_service_.parent_widget = this;

  setWindowTitle(QString::fromStdWString(model_.GetWindowTitle()));

  ui.descriptionLabel->setText(
      QString::fromStdWString(model_.GetSourceTitle()));
  ui.currentValueLabel->setText(
      QString::fromStdWString(model_.GetCurrentValue(true)));

  ui.valueComboBox->setEditable(!model_.discrete());

  ui.unitLabel->setText({});
  ui.conditionLabel->setText({});
  ui.statusLabel->setText({});
  ui.lockLabel->setVisible(model_.lock_allowed());
  ui.lockCheckBox->setVisible(model_.lock_allowed());

  if (model_.discrete()) {
    for (const auto& state : model_.GetDiscreteStates())
      ui.valueComboBox->addItem(QString::fromStdWString(state));
    ui.valueComboBox->setCurrentIndex(model_.GetCurrentDiscreteState());

  } else {
    ui.valueComboBox->setCurrentText(
        QString::fromStdWString(model_.GetCurrentValue(false)));
    ui.unitLabel->setText(QString::fromStdWString(model_.GetAnalogUnits()));
  }

  model_.current_change_handler = [this] {
    ui.currentValueLabel->setText(
        QString::fromStdWString(model_.GetCurrentValue(true)));
  };

  model_.condition_change_handler = [this] {
    ui.conditionLabel->setText(model_.IsConditionOk() ? tr("Satisfied")
                                                      : tr("Unsatisfied"));
  };

  model_.status_change_handler = [this] {
    ui.statusLabel->setText(QString::fromStdWString(model_.GetStatusText()));
  };

  model_.completion_handler = [this](bool ok) {
    ui.statusLabel->setText({});
    if (ok)
      QDialog::accept();
  };
}

void WriteDialog::accept() {
  double value = 0.0;
  if (model_.discrete()) {
    auto index = ui.valueComboBox->currentIndex();
    value = index ? 1.0 : 0.0;
  } else {
    ui.valueComboBox->currentText();
  }
}

void ExecuteWriteDialog(DialogService& dialog_service, WriteContext&& context) {
  WriteModel model{std::move(context)};
  WriteDialog dialog{model, dialog_service.GetParentWidget()};
  dialog.exec();
}
