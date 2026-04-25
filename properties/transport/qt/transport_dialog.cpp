#include "properties/transport/transport_dialog.h"

#include "aui/dialog_service.h"
#include "aui/qt/dialog_util.h"
#include "properties/transport/transport_dialog_model.h"
#include "ui_transport_dialog.h"

#include <transport/transport_string.h>

class TransportDialog : public QDialog {
  Q_OBJECT

 public:
  explicit TransportDialog(std::unique_ptr<TransportDialogModel> model,
                           QWidget* parent = nullptr);

  const transport::TransportString& transport_string() const {
    return model_->transport_string_;
  }

 public Q_SLOTS:
  virtual void accept() override;

 private:
  void SetTypeIndex(int index);

  Ui::TransportDialog ui;

  std::unique_ptr<TransportDialogModel> model_;
};

#include "transport_dialog.moc"

TransportDialog::TransportDialog(std::unique_ptr<TransportDialogModel> model,
                                 QWidget* parent)
    : QDialog{parent}, model_{std::move(model)} {
  ui.setupUi(this);

  ui.networkHostLineEdit->setText(
      QString::fromStdU16String(model_->network_host));
  ui.networkPortLineEdit->setText(QString::number(model_->network_port));

  for (auto& item : model_->type_items)
    ui.typeComboBox->addItem(QString::fromStdU16String(item));
  ui.typeComboBox->setCurrentIndex(model_->type_index);

  for (auto& item : model_->serial_port_items)
    ui.serialPortComboBox->addItem(QString::fromStdU16String(item));
  ui.serialPortComboBox->setCurrentIndex(model_->serial_port_index);

  for (auto& item : model_->baud_rate_items)
    ui.baudRateComboBox->addItem(QString::fromStdU16String(item));
  ui.baudRateComboBox->setCurrentIndex(model_->baud_rate_index);

  for (auto& item : model_->bit_count_items)
    ui.bitCountComboBox->addItem(QString::fromStdU16String(item));
  ui.bitCountComboBox->setCurrentIndex(model_->bit_count_index);

  for (auto& item : model_->parity_items)
    ui.parityComboBox->addItem(QString::fromStdU16String(item));
  ui.parityComboBox->setCurrentIndex(model_->parity_index);

  for (auto& item : model_->flow_control_items)
    ui.flowControlComboBox->addItem(QString::fromStdU16String(item));
  ui.flowControlComboBox->setCurrentIndex(model_->flow_control_index);

  for (auto& item : model_->stop_bits_items)
    ui.stopBitsComboBox->addItem(QString::fromStdU16String(item));
  ui.stopBitsComboBox->setCurrentIndex(model_->stop_bits_index);

  ui.typeComboBox->setCurrentIndex(model_->type_index);
  SetTypeIndex(model_->type_index);
  connect(ui.typeComboBox, QOverload<int>::of(&QComboBox::activated),
          [this](int index) { SetTypeIndex(index); });
}

void TransportDialog::accept() {
  model_->type_index = ui.typeComboBox->currentIndex();

  model_->network_host = ui.networkHostLineEdit->text().toStdU16String();
  model_->network_port = ui.networkPortLineEdit->text().toInt();

  model_->serial_port_index = ui.serialPortComboBox->currentIndex();
  model_->baud_rate_index = ui.baudRateComboBox->currentIndex();
  model_->bit_count_index = ui.bitCountComboBox->currentIndex();
  model_->parity_index = ui.parityComboBox->currentIndex();
  model_->flow_control_index = ui.flowControlComboBox->currentIndex();
  model_->stop_bits_index = ui.stopBitsComboBox->currentIndex();

  model_->Save();

  QDialog::accept();
}

void TransportDialog::SetTypeIndex(int index) {
  bool serial_port = model_->IsSerialPortType(index);
  ui.stackedWidget->setCurrentIndex(serial_port ? 1 : 0);
}

promise<transport::TransportString> ShowTransportDialog(
    DialogService& dialog_service,
    const transport::TransportString& transport_string) {
  auto model = std::make_unique<TransportDialogModel>(transport_string);
  auto dialog = std::make_unique<TransportDialog>(
      std::move(model), dialog_service.GetParentWidget());
  return StartMappedModalDialog(std::move(dialog), [](TransportDialog& dialog) {
    return dialog.transport_string();
  });
}
