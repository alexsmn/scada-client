#include "components/modus/views/modus_controller.h"

#include "base/bind.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "client_utils.h"
#include "common_resources.h"
#include "controller_factory.h"
#include "services/file_cache.h"
#include "components/main/main_window.h"
#include "components/modus/views/modus_view.h"
#include "components/modus/views/modus_view2.h"
#include "selection_model.h"
#include "services/profile.h"
#include "window_definition.h"
#include "window_info.h"

REGISTER_CONTROLLER(ModusController, ID_MODUS_VIEW);

ModusController::ModusController(const ControllerContext& context)
    : Controller(context),
      weak_factory_(this) {
}

ModusController::~ModusController() {
}

views::View* ModusController::CreateModusView() {
  view_ = std::make_unique<ModusView>(ModusViewContext{
      node_service_,
      timed_data_service_,
      file_cache_,
  });

  view_->title_callback_ = [this] (const base::string16& title) {
    controller_delegate_.SetTitle(title);
  };

  view_->navigation_callback_ = [this] (const base::FilePath& path) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, base::Bind(
        &ModusController::OpenPath, weak_factory_.GetWeakPtr(), path));
  };

  view_->selection_callback_ = [this] (const rt::TimedDataSpec& spec) {
    selection().SelectTimedData(spec);
  };

  // TODO: Change on ContextMenu.
  view_->popup_menu_callback_ = [this] (const gfx::Point& point) {
    controller_delegate_.ShowPopupMenu(IDR_MODUS_POPUP, point, false);
  };

  wrapper_ = view_.get();

 return view_.get();
}

views::View* ModusController::CreateModusView2() {
  view2_.reset(new ModusView2(timed_data_service_));

  view2_->set_selection_signal([this] (const rt::TimedDataSpec& spec) {
    selection().SelectTimedData(spec);
  });

  view2_->set_navigation_signal([this] (const base::FilePath& path) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, base::Bind(
        &ModusController::OpenPath, weak_factory_.GetWeakPtr(), path));
  });

  view2_->set_double_click_signal([this] {
    selection().GetTimedData().Acknowledge();
  });

  view2_->title_changed_handler = [this](base::StringPiece16 new_title) {
    controller_delegate_.SetTitle(new_title);
  };

  wrapper_ = view2_.get();

  return view2_->CreateParentIfNecessary();
}

views::View* ModusController::Init(const WindowDefinition& definition) {
  bool modus2 = profile_.modus2;
  if (auto* options = definition.FindItem("Options")) {
    auto version = options->GetInt("version", 0);
    if (version != 0)
      modus2 = version >= 2;
  }

  if (!base::LowerCaseEqualsASCII(definition.path.Extension(), ".xsde"))
    modus2 = false;

  views::View* view = nullptr;
  if (modus2)
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
  switch (command_id) {
    case ID_SETUP:
    case ID_PRINT:
    case ID_MODUS_TOOLBAR:
    case ID_MODUS_STATUSBAR:
      return view_ ? this : nullptr;
  }

  return __super::GetCommandHandler(command_id);
}

void ModusController::ExecuteCommand(unsigned command) {
  switch (command) {
    case ID_SETUP:
      view_->sde_form_->ShowOptions();
      break;

    case ID_PRINT:
      view_->sde_form_->Print();
      break;
      
    case ID_MODUS_TOOLBAR: {
      VARIANT_BOOL visible = VARIANT_FALSE;
      view_->sde_form_->get_ToolbarVisible(&visible);
      view_->sde_form_->put_ToolbarVisible(!visible);
      break;
    }
                 
    case ID_MODUS_STATUSBAR: {
      VARIANT_BOOL visible = VARIANT_FALSE;
      view_->sde_form_->get_StatusVisible(&visible);
      view_->sde_form_->put_StatusVisible(!visible);
      break;
    }

    default:
      __super::ExecuteCommand(command);
      break;
  }
}

void ModusController::OpenPath(const base::FilePath& path) {
  WindowDefinition win(GetWindowInfo(ID_MODUS_VIEW));
  win.path = path;
  controller_delegate_.OpenView(win);
}
