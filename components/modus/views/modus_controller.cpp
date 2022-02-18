#include "components/modus/views/modus_controller.h"

#include "base/bind.h"
#include "base/string_piece_util.h"
#include "base/strings/strcat.h"
#include "base/threading/thread_task_runner_handle.h"
#include "client_utils.h"
#include "common_resources.h"
#include "components/modus/activex/modus.h"
#include "components/modus/modus_component.h"
#include "components/modus/modus_util.h"
#include "components/modus/views/modus_view.h"
#include "components/modus/views/modus_view2.h"
#include "components/web/web_component.h"
#include "controller_delegate.h"
#include "controller_registry.h"
#include "selection_model.h"
#include "services/dialog_service.h"
#include "services/file_cache.h"
#include "window_definition.h"
#include "window_info.h"

#undef StrCat

ModusController::ModusController(const ControllerContext& context)
    : ControllerContext{context}, wrapper_(nullptr) {}

ModusController::~ModusController() {}

views::View* ModusController::CreateModusView() {
  auto title_callback = [this](const std::u16string& title) {
    controller_delegate_.SetTitle(title);
  };

  auto navigation_callback = [this](std::u16string_view hyperlink) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::Bind(&ModusController::OpenHyperlink, weak_factory_.GetWeakPtr(),
                   std::u16string{hyperlink}));
  };

  auto selection_callback = [this](const TimedDataSpec& spec) {
    selection_.SelectTimedData(spec);
  };

  // TODO: Change on ContextMenu.
  auto context_menu_callback = [this](const gfx::Point& point) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&ModusController::ShowPopupMenu,
                              weak_factory_.GetWeakPtr(), point));
  };

  view_ = std::make_unique<ModusView>(modus::ModusDocumentContext{
      alias_resolver_, timed_data_service_, file_cache_, title_callback,
      navigation_callback, selection_callback, context_menu_callback});

  wrapper_ = view_.get();

  command_registry_.AddCommand(Command{ID_SETUP}.set_execute_handler([this] {
    if (auto* sde_form = wrapper_->GetSdeForm())
      sde_form->ShowOptions();
  }));

  command_registry_.AddCommand(Command{ID_PRINT}.set_execute_handler([this] {
    if (auto* sde_form = wrapper_->GetSdeForm())
      sde_form->Print();
  }));

  command_registry_.AddCommand(
      Command{ID_MODUS_TOOLBAR}.set_execute_handler([this] {
        if (auto* sde_form = wrapper_->GetSdeForm()) {
          VARIANT_BOOL visible = VARIANT_FALSE;
          sde_form->get_ToolbarVisible(&visible);
          sde_form->put_ToolbarVisible(!visible);
        }
      }));

  command_registry_.AddCommand(
      Command{ID_MODUS_STATUSBAR}.set_execute_handler([this] {
        if (auto* sde_form = wrapper_->GetSdeForm()) {
          VARIANT_BOOL visible = VARIANT_FALSE;
          sde_form->get_StatusVisible(&visible);
          sde_form->put_StatusVisible(!visible);
        }
      }));

  return view_.get();
}

views::View* ModusController::CreateModusView2() {
  view2_ = std::make_unique<ModusView2>(ModusView2Context{timed_data_service_});

  view2_->set_selection_signal(
      [this](const TimedDataSpec& spec) { selection_.SelectTimedData(spec); });

  view2_->set_navigation_signal([this](const std::filesystem::path& path) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&ModusController::OpenPath,
                              weak_factory_.GetWeakPtr(), path));
  });

  view2_->set_double_click_signal(
      [this] { selection_.timed_data().Acknowledge(); });

  view2_->title_changed_handler = [this](std::u16string_view new_title) {
    controller_delegate_.SetTitle(new_title);
  };

  wrapper_ = view2_.get();

  return view2_->CreateParentIfNecessary();
}

views::View* ModusController::Init(const WindowDefinition& definition) {
  views::View* view = nullptr;
  if (IsModus2(definition, profile_))
    view = CreateModusView2();
  else
    view = CreateModusView();

  wrapper_->Open(GetPublicFilePath(definition.path));

  return view;
}

void ModusController::Save(WindowDefinition& definition) {
  definition.path = FullFilePathToPublic(wrapper_->GetPath());
  definition.AddItem("Options").SetInt("version", view_ ? 1 : 2);
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
        base::StrCat({u"Файл ", AsStringPiece(hyperlink),
                      u" не найден или находится вне папки схем."}),
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

void ModusController::ShowPopupMenu(const gfx::Point& point) {
  controller_delegate_.ShowPopupMenu(IDR_MODUS_POPUP, point, false);
}
