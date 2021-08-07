#include "components/modus/qt/modus_controller.h"

#include "base/bind.h"
#include "base/strings/strcat.h"
#include "base/threading/thread_task_runner_handle.h"
#include "client_utils.h"
#include "common_resources.h"
#include "components/modus/modus_component.h"
#include "components/modus/modus_util.h"
#include "components/modus/qt/modus_view.h"
#include "components/modus/qt/modus_view2.h"
#include "components/modus/qt/modus_view3.h"
#include "components/web/web_component.h"
#include "controller_delegate.h"
#include "controller_registry.h"
#include "selection_model.h"
#include "services/dialog_service.h"
#include "services/file_cache.h"
#include "services/profile.h"
#include "window_definition.h"
#include "window_info.h"

#include <QScrollArea>

ModusController::ModusController(const ControllerContext& context)
    : ControllerContext{context}, wrapper_(nullptr) {}

ModusController::~ModusController() {}

QWidget* ModusController::CreateModusView() {
  auto title_callback = [this](const std::wstring& title) {
    controller_delegate_.SetTitle(title);
  };

  auto navigation_callback = [this](std::wstring_view hyperlink) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::Bind(&ModusController::OpenHyperlink, weak_factory_.GetWeakPtr(),
                   std::wstring{hyperlink}));
  };

  auto selection_callback = [this](const TimedDataSpec& spec) {
    selection_.SelectTimedData(spec);
  };

  // TODO: Change on ContextMenu.
  auto context_menu_handler = [this](const UiPoint& point) {
    controller_delegate_.ShowPopupMenu(IDR_MODUS_POPUP, point, false);
  };

  view_ = std::make_unique<ModusView>(modus::ModusDocumentContext{
      alias_resolver_, timed_data_service_, file_cache_, title_callback,
      navigation_callback, selection_callback, context_menu_handler});

  wrapper_ = view_.get();

  return view_.get();
}

QWidget* ModusController::CreateModusView2() {
  view2_ = std::make_unique<ModusView2>(timed_data_service_);

  view2_->set_selection_signal(
      [this](const TimedDataSpec& spec) { selection_.SelectTimedData(spec); });

  view2_->set_navigation_signal([this](const base::FilePath& path) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&ModusController::OpenPath,
                              weak_factory_.GetWeakPtr(), path));
  });

  view2_->set_double_click_signal(
      [this] { selection_.timed_data().Acknowledge(); });

  wrapper_ = view2_.get();

  auto* scroll_area = new QScrollArea;
  scroll_area->setWidget(view2_.get());
  scroll_area->setStyleSheet("background-color: white;");

  return scroll_area;
}

QWidget* ModusController::CreateModusView3() {
  view3_ = std::make_unique<ModusView3>(timed_data_service_);

  wrapper_ = view3_.get();

  return view3_.get();
}

QWidget* ModusController::Init(const WindowDefinition& definition) {
  QWidget* result = nullptr;
  if (IsModus2(definition, profile_))
    result = CreateModusView3();
  else
    result = CreateModusView();

  wrapper_->Open(GetPublicFilePath(definition.path));

  return result;
}

void ModusController::Save(WindowDefinition& definition) {
  definition.path = FullFilePathToPublic(wrapper_->GetPath());
}

bool ModusController::ShowContainedItem(const scada::NodeId& item_id) {
  return wrapper_->ShowContainedItem(item_id);
}

CommandHandler* ModusController::GetCommandHandler(unsigned command_id) {
  switch (command_id) {
    case ID_SETUP:
    case ID_PRINT:
    case ID_MODUS_TOOLBAR:
    case ID_MODUS_STATUSBAR:
      return nullptr;
  }

  return Controller::GetCommandHandler(command_id);
}

void ModusController::ExecuteCommand(unsigned command) {
  switch (command) {
    case ID_SETUP:
      break;

    case ID_PRINT:
      break;

    case ID_MODUS_TOOLBAR: {
      break;
    }

    case ID_MODUS_STATUSBAR: {
      break;
    }

    default:
      __super::ExecuteCommand(command);
      break;
  }
}

void ModusController::OpenHyperlink(std::wstring_view hyperlink) {
  if (IsWebUrl(hyperlink)) {
    WindowDefinition win(kWebWindowInfo);
    win.path = base::FilePath{ToStringPiece(hyperlink)};
    controller_delegate_.OpenView(win);
    return;
  }

  auto path = MakeModusFilePath(base::FilePath{ToStringPiece(hyperlink)},
                                wrapper_->GetPath());
  if (!path.has_value()) {
    dialog_service_.RunMessageBox(
        base::StrCat({L"Файл ", ToStringPiece(hyperlink),
                      L" не найден или находится вне папки схем."}),
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

void ModusController::OpenPath(const base::FilePath& path) {
  WindowDefinition win(kModusWindowInfo);
  win.path = path;
  controller_delegate_.OpenView(win);
}
