#include "components/create_service_item/create_service_item_dialog.h"

#include "components/create_service_item/create_service_item_model.h"
#include "services/dialog_service.h"
#include "ui_create_service_item.h"

class CreateServiceItemDialog final : public QDialog {
  Q_OBJECT

 public:
  explicit CreateServiceItemDialog(CreateServiceItemModel& model,
                                   QWidget* parent = nullptr);
 public Q_SLOTS:
  virtual void accept() override;

 private:
  Ui::CreateServiceItemDialog ui;

  void UpdateComponents();

  CreateServiceItemModel& model_;
};

#include "create_service_item_dialog.moc"

CreateServiceItemDialog::CreateServiceItemDialog(CreateServiceItemModel& model,
                                                 QWidget* parent)
    : QDialog{parent}, model_{model} {
  ui.setupUi(this);

  connect(ui.deviceComboBox, QOverload<int>::of(&QComboBox::activated),
          [this](int index) {
            model_.SetDeviceIndex(index);
            UpdateComponents();
          });

  QStringList devices;
  devices.reserve(model_.devices().size());
  for (const auto& p : model_.devices())
    devices.push_back(QString::fromStdWString(p.first));
  ui.deviceComboBox->addItems(devices);

  UpdateComponents();
}

void CreateServiceItemDialog::accept() {
  CreateServiceItemModel::RunParams params = {};

  auto rows = ui.componentListWidget->selectionModel()->selectedRows();
  params.component_indexes.reserve(rows.size());
  std::transform(rows.begin(), rows.end(),
                 std::back_inserter(params.component_indexes),
                 [](const QModelIndex& index) { return index.row(); });

  model_.Run(params);

  QDialog::accept();
}

void CreateServiceItemDialog::UpdateComponents() {
  QStringList components;
  components.reserve(model_.components().size());
  for (const auto& p : model_.components())
    components.push_back(QString::fromStdWString(p.first));
  ui.componentListWidget->clear();
  ui.componentListWidget->addItems(components);
}

void ShowCreateServiceItemDialog(DialogService& dialog_service,
                                 CreateServiceItemContext&& context) {
  CreateServiceItemModel model{std::move(context)};
  CreateServiceItemDialog dialog{model, dialog_service.GetParentWidget()};
  dialog.exec();
}
