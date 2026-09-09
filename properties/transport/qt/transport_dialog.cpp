#include "properties/transport/transport_dialog.h"

#include "aui/dialog_service.h"
#include "aui/qt/dialog_util.h"
#include "aui/translation.h"
#include "base/any_executor.h"
#include "base/awaitable.h"
#include "properties/transport/transport_dialog_model.h"
#include "ui_transport_dialog.h"

#include <transport/transport_string.h>

#include <QIntValidator>

#include <utility>

namespace {

// The TCP/UDP port range. 0 is not a port an operator can connect to, and it
// is what an empty or non-numeric field used to save.
constexpr int kMinPort = 1;
constexpr int kMaxPort = 65535;

}  // namespace

class TransportDialog : public QDialog {
  Q_OBJECT

 public:
  TransportDialog(std::unique_ptr<TransportDialogModel> model,
                  DialogService& dialog_service,
                  AnyExecutor executor);

  const transport::TransportString& transport_string() const {
    return model_->transport_string_;
  }

 public Q_SLOTS:
  virtual void accept() override;

 private:
  void SetTypeIndex(int index);
  // Shows `message` as an error box owned by this dialog and keeps the
  // dialog open. Spawned, not awaited: RunMessageBox is a lazy awaitable and
  // accept() is a Qt slot with nothing to hand it to.
  void ReportInputError(std::u16string message);

  Ui::TransportDialog ui;

  std::unique_ptr<TransportDialogModel> model_;
  DialogService& dialog_service_;
  AnyExecutor executor_;
};

#include "transport_dialog.moc"

TransportDialog::TransportDialog(std::unique_ptr<TransportDialogModel> model,
                                 DialogService& dialog_service,
                                 AnyExecutor executor)
    : QDialog{dialog_service.GetParentWidget()},
      model_{std::move(model)},
      dialog_service_{dialog_service},
      executor_{std::move(executor)} {
  ui.setupUi(this);

  // Keeps out non-digits and anything longer than a port; a value that is
  // numerically out of range can still be typed (QIntValidator reports it as
  // Intermediate), which is why accept() checks the range as well.
  ui.networkPortLineEdit->setValidator(
      new QIntValidator{kMinPort, kMaxPort, ui.networkPortLineEdit});

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
  const int type_index = ui.typeComboBox->currentIndex();

  // The port field only means something for a network transport. Parsed with
  // the flag and range-checked: toInt() alone answered 0 for a typo, and
  // 70000 or -5 went into the persisted transport string as written.
  int network_port = model_->network_port;
  if (!model_->IsSerialPortType(type_index)) {
    bool ok = false;
    network_port = ui.networkPortLineEdit->text().toInt(&ok);
    if (!ok || network_port < kMinPort || network_port > kMaxPort) {
      ReportInputError(Translate("The port must be a number from 1 to 65535."));
      return;
    }
  }

  model_->type_index = type_index;

  model_->network_host = ui.networkHostLineEdit->text().toStdU16String();
  model_->network_port = network_port;

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

void TransportDialog::ReportInputError(std::u16string message) {
  // The dialog service outlives this modal (it belongs to the property
  // context that opened it), so the reference is safe to carry.
  CoSpawn(executor_,
          [&dialog_service = dialog_service_, message = std::move(message),
           title = windowTitle().toStdU16String()]() -> Awaitable<void> {
            (void)co_await dialog_service.RunMessageBox(message, title,
                                                        MessageBoxMode::Error);
          });
}

Awaitable<transport::TransportString> ShowTransportDialog(
    DialogService& dialog_service,
    const transport::TransportString& transport_string) {
  // The awaiting coroutine's executor is where the dialog reports input
  // errors from; it has no other way to reach one from a Qt slot.
  AnyExecutor executor = co_await boost::asio::this_coro::executor;
  auto model = std::make_unique<TransportDialogModel>(transport_string);
  auto dialog = std::make_unique<TransportDialog>(
      std::move(model), dialog_service, std::move(executor));
  co_return co_await StartMappedModalDialog(
      std::move(dialog),
      [](TransportDialog& dialog) { return dialog.transport_string(); });
}
