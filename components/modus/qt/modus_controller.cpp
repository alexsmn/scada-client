#include "components/modus/qt/modus_controller.h"

#include "base/bind.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "client_utils.h"
#include "common_resources.h"
#include "components/modus/qt/modus_view2.h"
#include "controller_factory.h"
#include "selection_model.h"
#include "services/file_cache.h"
#include "services/profile.h"
#include "window_definition.h"
#include "window_info.h"

#include <qscrollarea.h>

REGISTER_CONTROLLER(ModusController, ID_MODUS_VIEW);

ModusController::ModusController(const ControllerContext& context)
    : Controller{context}, wrapper_(nullptr) {}

ModusController::~ModusController() {}

QWidget* ModusController::CreateModusView2() {
  view2_ = std::make_unique<ModusView2>(timed_data_service_);

  view2_->set_selection_signal([this](const rt::TimedDataSpec& spec) {
    selection().SelectTimedData(spec);
  });

  view2_->set_navigation_signal([this](const base::FilePath& path) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&ModusController::OpenPath,
                              weak_factory_.GetWeakPtr(), path));
  });

  view2_->set_double_click_signal(
      [this] { selection().GetTimedData().Acknowledge(); });

  wrapper_ = view2_.get();

  return view2_.get();
}

QWidget* ModusController::Init(const WindowDefinition& definition) {
  bool modus2 = profile_.modus2;
  if (auto* options = definition.FindItem("Options")) {
    auto version = options->GetInt("version", 0);
    if (version != 0)
      modus2 = version >= 2;
  }

  if (!base::LowerCaseEqualsASCII(definition.path.Extension(), ".xsde"))
    modus2 = false;

  auto* view = CreateModusView2();
  wrapper_->Open(GetPublicFilePath(definition.path));

  auto* scroll_area = new QScrollArea;
  scroll_area->setWidget(view);
  scroll_area->setStyleSheet("background-color: white;");

  return scroll_area;
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

  return __super::GetCommandHandler(command_id);
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

void ModusController::OpenPath(const base::FilePath& path) {
  WindowDefinition win(GetWindowInfo(ID_MODUS_VIEW));
  win.path = path;
  controller_delegate_.OpenView(win);
}
