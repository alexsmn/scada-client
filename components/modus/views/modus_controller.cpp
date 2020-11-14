#include "components/modus/views/modus_controller.h"

#include "base/bind.h"
#include "base/strings/strcat.h"
#include "base/threading/thread_task_runner_handle.h"
#include "client_utils.h"
#include "common_resources.h"
#include "components/modus/modus_util.h"
#include "components/modus/views/modus_view.h"
#include "components/modus/views/modus_view2.h"
#include "controller_factory.h"
#include "selection_model.h"
#include "services/dialog_service.h"
#include "services/file_cache.h"
#include "window_definition.h"
#include "window_info.h"

ModusController::ModusController(const ControllerContext& context)
    : Controller{context}, wrapper_(nullptr) {}

ModusController::~ModusController() {}

views::View* ModusController::CreateModusView() {
  auto title_callback = [this](const std::wstring& title) {
    controller_delegate_.SetTitle(title);
  };

  auto navigation_callback = [this](base::StringPiece16 hyperlink) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::Bind(&ModusController::OpenHyperlink, weak_factory_.GetWeakPtr(),
                   hyperlink.as_string()));
  };

  auto selection_callback = [this](const TimedDataSpec& spec) {
    selection().SelectTimedData(spec);
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

  return view_.get();
}

views::View* ModusController::CreateModusView2() {
  view2_ = std::make_unique<ModusView2>(ModusView2Context{timed_data_service_});

  view2_->set_selection_signal(
      [this](const TimedDataSpec& spec) { selection().SelectTimedData(spec); });

  view2_->set_navigation_signal([this](const base::FilePath& path) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&ModusController::OpenPath,
                              weak_factory_.GetWeakPtr(), path));
  });

  view2_->set_double_click_signal(
      [this] { selection().timed_data().Acknowledge(); });

  view2_->title_changed_handler = [this](base::StringPiece16 new_title) {
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
  if (view_) {
    switch (command_id) {
      case ID_SETUP:
      case ID_PRINT:
      case ID_MODUS_TOOLBAR:
      case ID_MODUS_STATUSBAR:
        return this;
    }
  }

  return Controller::GetCommandHandler(command_id);
}

void ModusController::ExecuteCommand(unsigned command) {
  if (auto* sde_form = wrapper_->GetSdeForm()) {
    switch (command) {
      case ID_SETUP:
        sde_form->ShowOptions();
        return;

      case ID_PRINT:
        sde_form->Print();
        return;

      case ID_MODUS_TOOLBAR: {
        VARIANT_BOOL visible = VARIANT_FALSE;
        sde_form->get_ToolbarVisible(&visible);
        sde_form->put_ToolbarVisible(!visible);
        return;
      }

      case ID_MODUS_STATUSBAR: {
        VARIANT_BOOL visible = VARIANT_FALSE;
        sde_form->get_StatusVisible(&visible);
        sde_form->put_StatusVisible(!visible);
        return;
      }
    }
  }

  __super::ExecuteCommand(command);
}

void ModusController::OpenHyperlink(base::StringPiece16 hyperlink) {
  if (IsWebUrl(hyperlink)) {
    WindowDefinition win(GetWindowInfo(ID_WEB_VIEW));
    win.path = base::FilePath{hyperlink};
    controller_delegate_.OpenView(win);
    return;
  }

  auto path = MakeModusFilePath(base::FilePath{hyperlink}, wrapper_->GetPath());
  if (!path.has_value()) {
    dialog_service_.RunMessageBox(
        base::StrCat(
            {L"Файл ", hyperlink, L" не найден или находится вне папки схем."}),
        {}, MessageBoxMode::Error);
    return;
  }

  if (!IsModusFilePath(*path)) {
    WindowDefinition win(GetWindowInfo(ID_WEB_VIEW));
    win.path = std::move(*path);
    controller_delegate_.OpenView(win);
    return;
  }

  OpenPath(std::move(*path));
}

void ModusController::OpenPath(const base::FilePath& path) {
  WindowDefinition win(GetWindowInfo(ID_MODUS_VIEW));
  win.path = path;
  controller_delegate_.OpenView(win);
}

void ModusController::ShowPopupMenu(const gfx::Point& point) {
  controller_delegate_.ShowPopupMenu(IDR_MODUS_POPUP, point, false);
}
