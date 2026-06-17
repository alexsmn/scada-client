#include "modus/qt/modus_controller.h"

#include "aui/dialog_service.h"
#include "aui/translation.h"
#include "base/awaitable.h"
#include "base/u16format.h"
#include "base/web_util.h"
#include "controller/controller_delegate.h"
#include "controller/controller_registry.h"
#include "controller/selection_model.h"
#include "controller/window_info.h"
#include "filesystem/file_cache.h"
#include "filesystem/file_util.h"
#include "modules/web/web_component.h"
#include "modus/modus_component.h"
#include "modus/modus_util.h"
#include "modus/qt/modus_view.h"
#include "modus/qt/modus_view2.h"
#include "profile/profile.h"
#include "profile/window_definition.h"
#include "resources/common_resources.h"
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

ModusController::ModusController(const ControllerContext& context,
                                 AliasResolver alias_resolver)
    : ControllerContext{context}, alias_resolver_{std::move(alias_resolver)} {}

ModusController::~ModusController() = default;

QWidget* ModusController::CreateModusView() {
  auto title_callback = [this](const std::u16string& title) {
    controller_delegate_.SetTitle(title);
  };

  auto navigation_callback = [executor = executor_,
                              cancelation = cancelation_.weak_ptr(),
                              this](std::u16string_view hyperlink) {
    // Intentionally delay open to exit from the Modus handler.
    CoSpawn(executor, cancelation,
            [this, hyperlink = std::u16string{hyperlink}]() -> Awaitable<void> {
              OpenHyperlink(hyperlink);
              co_return;
            });
  };

  auto selection_callback = [this](const TimedDataSpec& spec) {
    selection_.SelectTimedData(spec);
  };

  // TODO: Change on ContextMenu.
  auto context_menu_handler = [this](const aui::Point& point) {
    controller_delegate_.ShowPopupMenu(nullptr, IDR_MODUS_POPUP, point, false);
  };

  auto enable_internal_render_callback = [this] {
    profile_.modus.modus2 = true;
    profile_.NotifyChange();

    dialog_service_.RunMessageBox(
        Translate("Built-in Modus diagram rendering is enabled and will be "
                  "applied to subsequently opened diagrams. To disable, use "
                  "the Settings menu."),
        Translate("Built-in rendering"), MessageBoxMode::Info);
    controller_delegate_.Close();
  };

  view_ = new ModusView{modus::ModusDocumentContext{
      executor_, alias_resolver_, timed_data_service_, file_cache_, profile_,
      title_callback, navigation_callback, selection_callback,
      context_menu_handler, enable_internal_render_callback}};

  wrapper_ = view_;

  command_registry_.AddCommand(Command{ID_SETUP}.set_execute_handler(
      [this] { view_->ShowSetupDialog(); }));

  return view_;
}

QWidget* ModusController::CreateModusView2() {
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
  result.reset(CreateModusView2());

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

void ModusController::OpenHyperlink(std::u16string_view hyperlink) {
  if (IsWebUrl(hyperlink)) {
    WindowDefinition win(kWebWindowInfo);
    win.path = hyperlink;
    controller_delegate_.OpenView(win);
    return;
  }

  auto path = MakeModusFilePath(hyperlink, wrapper_->GetPath());
  if (!path.has_value()) {
    dialog_service_.RunMessageBox(
        u16format(L"File {} not found or located outside the diagrams folder.",
                  hyperlink),
        {}, MessageBoxMode::Error);
    return;
  }

  if (!IsModusFilePath(*path)) {
    WindowDefinition win(kWebWindowInfo);
    win.path = std::move(*path);
    controller_delegate_.OpenView(win);
    return;
  }

  OpenPath(std::move(*path));
}

void ModusController::OpenPath(const std::filesystem::path& path) {
  WindowDefinition win(kModusWindowInfo);
  win.path = path;
  controller_delegate_.OpenView(win);
}
