#include "modus/qt/modus_controller.h"

#include "controller/selection_model.h"
#include "filesystem/file_util.h"
#include "modus/modus_component.h"
#include "modus/modus_util.h"
#include "modus/modus_view_wrapper.h"
#include "profile/window_definition.h"
#include "vds_runtime/qt/vds_runtime_widget.h"

#include <QScrollArea>

#include <exception>
#include <string>
#include <utility>

namespace {

// The production runtime view: one object that is both the Qt widget the
// window embeds and the `ModusViewWrapper` the controller drives.
class ModusVdsRuntimeView final : public VdsRuntimeWidget,
                                  public ModusViewWrapper {
 public:
  explicit ModusVdsRuntimeView(QWidget* parent = nullptr)
      : VdsRuntimeWidget{parent} {}

  void Open(const WindowDefinition& definition,
            int32_t document_kind) override {
    path_ = GetPublicFilePath(definition.path);
    VdsRuntimeWidget::Open(path_, document_kind);
  }

  void Save(WindowDefinition&) override {}

  std::filesystem::path GetPath() const override { return path_; }

  bool ShowContainedItem(const scada::NodeId&) override { return false; }

 private:
  std::filesystem::path path_;
};

}  // namespace

ModusController::ModusController(const ControllerContext& context,
                                 RuntimeViewFactory runtime_view_factory)
    : ControllerContext{context},
      runtime_view_factory_{std::move(runtime_view_factory)} {}

ModusController::~ModusController() = default;

ModusController::RuntimeView ModusController::CreateVdsRuntimeView() {
  auto* runtime_view = new ModusVdsRuntimeView;

  runtime_view->set_selection_callback([this](const QString& data_source) {
    SelectDataSource(data_source.toStdString());
  });

  runtime_view->set_double_click_callback([this] { AcknowledgeSelection(); });

  auto* scroll_area = new QScrollArea;
  scroll_area->setWidget(runtime_view);
  scroll_area->setStyleSheet("background-color: white;");

  return {.widget = scroll_area, .wrapper = runtime_view};
}

void ModusController::SelectDataSource(std::string_view data_source) {
  if (data_source.empty())
    return;

  try {
    selection_.SelectTimedData(
        TimedDataSpec{timed_data_service_,
                      scada::NodeId::FromString(std::string{data_source})});
  } catch (const std::exception&) {
    selection_.Clear();
  }
}

void ModusController::AcknowledgeSelection() {
  selection_.timed_data().Acknowledge();
}

std::unique_ptr<UiView> ModusController::Init(
    const WindowDefinition& definition) {
  const RuntimeView runtime_view = runtime_view_factory_
                                       ? runtime_view_factory_(*this)
                                       : CreateVdsRuntimeView();

  wrapper_ = runtime_view.wrapper;

  std::unique_ptr<UiView> result;
  result.reset(runtime_view.widget);

  wrapper_->Open(definition, DocumentKindFor(definition, profile_));

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
