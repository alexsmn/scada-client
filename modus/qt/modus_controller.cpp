#include "modus/qt/modus_controller.h"

#include "aui/dialog_service.h"
#include "base/strings/strcat.h"
#include "common_resources.h"
#include "components/web/web_component.h"
#include "controller/controller_delegate.h"
#include "controller/controller_registry.h"
#include "controller/selection_model.h"
#include "controller/window_info.h"
#include "filesystem/file_cache.h"
#include "filesystem/file_util.h"
#include "modus/modus_component.h"
#include "modus/modus_util.h"
#include "modus/qt/modus_view.h"
#include "modus/qt/modus_view2.h"
#include "modus/qt/modus_view3.h"
#include "profile/profile.h"
#include "profile/window_definition.h"
#include "web/web_util.h"

#include <QScrollArea>

ModusController::ModusController(const ControllerContext& context,
                                 AliasResolver alias_resolver)
    : ControllerContext{context}, alias_resolver_{std::move(alias_resolver)} {}

ModusController::~ModusController() = default;

QWidget* ModusController::CreateModusView() {
  auto title_callback = [this](const std::u16string& title) {
    controller_delegate_.SetTitle(title);
  };

  auto navigation_callback =
      [executor = executor_,
       weak_ptr = weak_factory_.GetWeakPtr()](std::u16string_view hyperlink) {
        // Intentionally delay open to exit from the Modus handler.
        executor->PostTask([weak_ptr, hyperlink = std::u16string{hyperlink}] {
          if (auto* ptr = weak_ptr.get()) {
            ptr->OpenHyperlink(hyperlink);
          }
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
        u"Встроенная отрисовка схем Модус включена и будет применена для далее "
        u"открытых схем. Для отключения функции используйте меню Настройки.",
        u"Встроенная отрисовка", MessageBoxMode::Info);
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
  view2_ = new ModusView2{timed_data_service_};

  view2_->set_selection_signal(
      [this](const TimedDataSpec& spec) { selection_.SelectTimedData(spec); });

  view2_->set_navigation_signal(
      [executor = executor_, weak_ptr = weak_factory_.GetWeakPtr()](
          const std::filesystem::path& path) {
        // Intentionally delay open to exit from the Modus handler.
        executor->PostTask([weak_ptr, path] {
          if (auto* ptr = weak_ptr.get()) {
            ptr->OpenPath(path);
          }
        });
      });

  view2_->set_double_click_signal(
      [this] { selection_.timed_data().Acknowledge(); });

  wrapper_ = view2_;

  auto* scroll_area = new QScrollArea;
  scroll_area->setWidget(view2_);
  scroll_area->setStyleSheet("background-color: white;");

  return scroll_area;
}

QWidget* ModusController::CreateModusView3() {
  view3_ = new ModusView3{timed_data_service_};

  wrapper_ = view3_;

  return view3_;
}

std::unique_ptr<UiView> ModusController::Init(
    const WindowDefinition& definition) {
  std::unique_ptr<QWidget> result;
  if (IsModus2(definition, profile_))
    result.reset(CreateModusView3());
  else
    result.reset(CreateModusView());

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
        base::StrCat(
            {u"Файл ", hyperlink, u" не найден или находится вне папки схем."}),
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
