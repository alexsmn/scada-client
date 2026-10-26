#include "modus/qt/modus_controller.h"

#include "controller/selection_model.h"
#include "display_view/qt/display_widget.h"
#include "filesystem/file_cache.h"
#include "filesystem/file_util.h"
#include "modus/modus_component.h"
#include "modus/modus_live_data.h"
#include "modus/modus_util.h"
#include "modus/modus_view_wrapper.h"
#include "profile/window_definition.h"
#include "services/display_selection_registry.h"
#include "timed_data/timed_data_spec.h"

#include "base/utf_convert.h"

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
    DisplayWidget::Open(path_, DisplayDocumentKind::kModus);
  }

  void Save(WindowDefinition&) override {}

  std::filesystem::path GetPath() const override { return path_; }

  std::u16string GetTitle() const override { return title().toStdU16String(); }

  bool ShowContainedItem(const scada::NodeId&) override { return false; }

  std::vector<std::string> GetDataSources() const override {
    return DisplayWidget::DataSources();
  }

  void SetDataSourceReading(const ModusDataSourceReading& reading) override {
    DisplayWidget::SetDataSourceReading(reading.data_source, reading.state,
                                        reading.text, reading.quality);
  }

  bool SelectDataSourceElement(std::string_view data_source) override {
    return DisplayWidget::SelectDataSourceElement(data_source);
  }

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
      [this](const std::optional<DisplayShapeHit>& hit) {
        if (!hit) {
          selection_.Clear();
          display_selection_registry_.ClearSelection(this);
          return;
        }

        SelectDataSource(hit->data_source);

        // A Modus element has no authored name, and a bound symbol often no
        // text either, so the selected object's own title names it.
        std::u16string label = DisplayShapeLabel(*hit).toStdU16String();
        if (label.empty() && !selection_.empty())
          label = ToString16(selection_.timed_data().GetTitle());
        display_selection_registry_.SetSelection(this, label);
      });

  display_view->set_double_click_callback([this] { AcknowledgeSelection(); });

  auto* scroll_area = new QScrollArea;
  scroll_area->setWidget(display_view);
  scroll_area->setStyleSheet("background-color: white;");

  return {.widget = scroll_area, .wrapper = display_view};
}

void ModusController::SelectDataSource(std::string_view data_source) {
  // An empty data source is a click on bare page or on a shape bound to no
  // object. The display moves (or drops) its highlight either way, so the
  // selection must follow: leaving the previous object selected let
  // «Управление…», the Inspector and a double-click acknowledgement act on an
  // object the operator no longer sees highlighted (backlog 907).
  if (data_source.empty()) {
    selection_.Clear();
    return;
  }

  // Through the formula path, as the schematic subscribes and as 2.5.7 did:
  // a binding key is an alias, a node id or an expression, and only the
  // formula service resolves all three (backlog 915).
  try {
    selection_.SelectTimedData(
        TimedDataSpec{timed_data_service_, std::string_view{data_source}});
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
  widget_ = display_view.widget;

  std::unique_ptr<UiView> result;
  result.reset(display_view.widget);

  wrapper_->Open(definition);

  const std::vector<std::string> data_sources = wrapper_->GetDataSources();
  live_data_ = std::make_unique<ModusLiveData>(
      timed_data_service_, data_sources,
      [this](const ModusDataSourceReading& reading) {
        // The widget tree belongs to the window, which may be torn down
        // before this controller is.
        if (!widget_)
          return;
        wrapper_->SetDataSourceReading(reading);
        // A subscription resolving to its object is what the file index
        // waits for; null while the constructor below is still running.
        if (live_data_)
          UpdateFileCache();
      });
  UpdateFileCache();

  return result;
}

void ModusController::Save(WindowDefinition& definition) {
  definition.path = FullFilePathToPublic(wrapper_->GetPath());

  wrapper_->Save(definition);
}

void ModusController::UpdateFileCache() {
  const std::filesystem::path path = wrapper_->GetPath();
  if (path.empty())
    return;

  // The schematic's «Заголовок» names it in the display menu; one without a
  // title keeps its file name, as the cache's own scan names it.
  std::u16string title = wrapper_->GetTitle();
  if (title.empty())
    title = path.stem().u16string();

  FileCache::ItemMap items;
  for (const scada::NodeId& node_id : live_data_->ResolvedNodes())
    items.emplace(node_id, 0);

  if (title == indexed_title_ && items == indexed_items_)
    return;
  indexed_title_ = title;
  indexed_items_ = items;

  // What 2.5.7's ActiveX loader did on every load (`FileCacheUpdater`), and
  // what nothing in the built-in path did: the menu kept file names and
  // «Мнемосхема» for an object found no display (backlogs 915, 910).
  file_cache_.Update(ID_MODUS_VIEW, FullFilePathToPublic(path), title, items);
}

bool ModusController::ShowContainedItem(const scada::NodeId& item_id) {
  // The element bound to the object, found through the bindings the schematic
  // is subscribed to (backlog 910); a renderer that resolves objects itself
  // is asked otherwise.
  if (live_data_ && widget_) {
    if (std::optional<std::string> data_source =
            live_data_->FindDataSourceForNode(item_id);
        data_source && wrapper_->SelectDataSourceElement(*data_source)) {
      return true;
    }
  }
  return wrapper_->ShowContainedItem(item_id);
}

CommandHandler* ModusController::GetCommandHandler(unsigned command_id) {
  return command_registry_.GetCommandHandler(command_id);
}
