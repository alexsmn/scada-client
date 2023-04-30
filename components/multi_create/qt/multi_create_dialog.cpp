#include "components/multi_create/multi_create_dialog.h"

#include "components/multi_create/multi_create_model.h"
#include "qt/dialog_util.h"
#include "services/dialog_service.h"
#include "ui_multi_create_dialog.h"

class MultiCreateDialog final : public QDialog {
  Q_OBJECT

 public:
  explicit MultiCreateDialog(std::unique_ptr<MultiCreateModel> model,
                             QWidget* parent = nullptr);
 public Q_SLOTS:
  virtual void accept() override;

 private:
  void SetAutoName();

  Ui::MultiCreateDialog ui;

  std::unique_ptr<MultiCreateModel> model_;

  bool auto_name_ = true;
};

#include "multi_create_dialog.moc"

MultiCreateDialog::MultiCreateDialog(std::unique_ptr<MultiCreateModel> model,
                                     QWidget* parent)
    : QDialog{parent}, model_{std::move(model)} {
  ui.setupUi(this);

  SetAutoName();
  connect(ui.namePrefixLineEdit, &QLineEdit::textEdited,
          [this] { auto_name_ = false; });
  connect(ui.discreteTypeRadioButton, &QRadioButton::toggled,
          [this] { SetAutoName(); });

  QStringList devices;
  devices.reserve(model_->devices().size());
  for (const auto& p : model_->devices())
    devices.push_back(QString::fromStdU16String(p.first));
  ui.deviceComboBox->addItems(devices);
}

void MultiCreateDialog::accept() {
  MultiCreateModel::RunParams params = {};
  params.device = ui.deviceComboBox->currentText().toStdU16String();
  params.count = ui.countSpinBox->value();
  params.ts = ui.discreteTypeRadioButton->isChecked();
  params.starting_number = ui.startingNumberSpinBox->value();
  params.starting_address = ui.startingAddressSpinBox->value();
  params.name_prefix = ui.namePrefixLineEdit->text().toStdU16String();
  params.path_prefix = ui.addressPrefixLineEdit->text().toStdString();
  model_->Run(params);

  QDialog::accept();
}

void MultiCreateDialog::SetAutoName() {
  if (auto_name_) {
    bool ts = ui.discreteTypeRadioButton->isChecked();
    ui.namePrefixLineEdit->setText(
        QString::fromStdU16String(model_->GetAutoName(ts)));
  }
}

void ShowMultiCreateDialog(DialogService& dialog_service,
                           MultiCreateContext&& context) {
  auto model = std::make_unique<MultiCreateModel>(std::move(context));
  auto dialog = std::make_unique<MultiCreateDialog>(
      std::move(model), dialog_service.GetParentWidget());
  StartModalDialog(std::move(dialog));
}
