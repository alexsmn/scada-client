#include "modus/qt/modus_controller.h"

#include "controller/selection_model.h"
#include "filesystem/file_util.h"
#include "modus/modus_component.h"
#include "modus/modus_view_wrapper.h"
#include "profile/window_definition.h"
#include "vds_runtime/qt/vds_runtime_widget.h"

#include <QScrollArea>

#include <exception>

namespace {

class ModusVdsRuntimeView final : public VdsRuntimeWidget,
                                  public ModusViewWrapper {
 public:
  explicit ModusVdsRuntimeView(QWidget* parent = nullptr)
      : VdsRuntimeWidget{parent} {}

  void Open(const WindowDefinition& definition) override {
    path_ = GetPublicFilePath(definition.path);
    VdsRuntimeWidget::Open(path_, TC_VDS_RUNTIME_DOCUMENT_KIND_AUTO);
  }

  void Save(WindowDefinition&) override {}

  std::filesystem::path GetPath() const override { return path_; }

  bool ShowContainedItem(const scada::NodeId&) override { return false; }

 private:
  std::filesystem::path path_;
};

}  // namespace

ModusController::ModusController(const ControllerContext& context)
    : ControllerContext{context} {}

ModusController::~ModusController() = default;

QWidget* ModusController::CreateRuntimeView() {
  auto* runtime_view = new ModusVdsRuntimeView;

  runtime_view->set_selection_callback([this](const QString& data_source) {
    if (data_source.isEmpty())
      return;

    try {
      selection_.SelectTimedData(
          TimedDataSpec{timed_data_service_,
                        scada::NodeId::FromString(data_source.toStdString())});
    } catch (const std::exception&) {
      selection_.Clear();
    }
  });

  runtime_view->set_double_click_callback(
      [this] { selection_.timed_data().Acknowledge(); });

  wrapper_ = runtime_view;

  auto* scroll_area = new QScrollArea;
  scroll_area->setWidget(runtime_view);
  scroll_area->setStyleSheet("background-color: white;");

  return scroll_area;
}

std::unique_ptr<UiView> ModusController::Init(
    const WindowDefinition& definition) {
  std::unique_ptr<QWidget> result;
  result.reset(CreateRuntimeView());

  wrapper_->Open(definition);

  return result;
}

void ModusController::Save(WindowDefinition& definition) {
  definition.path = FullFilePathToPublic(wrapper_->GetPath());

  wrapper_->Save(definition);
}

bool ModusController::ShowContainedItem(const scada::NodeId& item_id) {
  return wrapper_->ShowContainedItem(item_id);
}

CommandHandler* ModusController::GetCommandHandler(unsigned command_id) {
  return command_registry_.GetCommandHandler(command_id);
}
