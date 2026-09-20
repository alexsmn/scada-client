#include "modus/qt/modus_controller.h"

#include "controller/selection_model.h"
#include "display_view/qt/display_widget.h"
#include "filesystem/file_util.h"
#include "modus/modus_component.h"
#include "modus/modus_util.h"
#include "modus/modus_view_wrapper.h"
#include "profile/window_definition.h"
#include "services/display_selection_registry.h"

#include <QScrollArea>

#include <exception>
#include <optional>
#include <string>
#include <utility>

namespace {

// The production display view: one object that is both the Qt widget the
// window embeds and the `ModusViewWrapper` the controller drives.
class ModusDisplayView final : public DisplayWidget, public ModusViewWrapper {
 public:
  explicit ModusDisplayView(QWidget* parent = nullptr)
      : DisplayWidget{parent} {}

  void Open(const WindowDefinition& definition) override {
    path_ = GetPublicFilePath(definition.path);
    DisplayWidget::Open(path_, scada::display::view::DocumentKind::kModus);
  }

  void Save(WindowDefinition&) override {}

  std::filesystem::path GetPath() const override { return path_; }

  bool ShowContainedItem(const scada::NodeId&) override { return false; }

 private:
  std::filesystem::path path_;
};

}  // namespace

ModusController::ModusController(const ControllerContext& context,
                                 DisplayViewFactory display_view_factory)
    : ControllerContext{context},
      display_view_factory_{std::move(display_view_factory)} {}

ModusController::~ModusController() {
  // Only if the strip is still showing this display's selection; see
  // `DisplaySelectionRegistry`.
  display_selection_registry_.ClearSelection(this);
}

ModusController::DisplayView ModusController::CreateDisplayView() {
  auto* display_view = new ModusDisplayView;

  display_view->set_selection_callback(
      [this](const std::optional<scada::display::view::ShapeHit>& hit) {
        if (!hit) {
          selection_.Clear();
          display_selection_registry_.ClearSelection(this);
          return;
        }

        display_selection_registry_.SetSelection(
            this, DisplayShapeLabel(*hit).toStdU16String());
        SelectDataSource(hit->data_source);
      });

  display_view->set_double_click_callback([this] { AcknowledgeSelection(); });

  auto* scroll_area = new QScrollArea;
  scroll_area->setWidget(display_view);
  scroll_area->setStyleSheet("background-color: white;");

  return {.widget = scroll_area, .wrapper = display_view};
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
  const DisplayView display_view = display_view_factory_
                                       ? display_view_factory_(*this)
                                       : CreateDisplayView();

  wrapper_ = display_view.wrapper;

  std::unique_ptr<UiView> result;
  result.reset(display_view.widget);

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
