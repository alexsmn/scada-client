#include "client/components/modus/qt/modus_controller.h"

#include <QScrollArea>

#include "base/bind.h"
#include "client/controller_factory.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/strings/string_util.h"
#include "client/common_resources.h"
#include "client/services/file_cache.h"
#include "client/components/main/main_window.h"
#include "client/components/modus/qt/modus_view2.h"
#include "client/selection_model.h"
#include "client/services/profile.h"
#include "client/window_definition.h"
#include "client/window_info.h"

REGISTER_CONTROLLER(ModusController, ID_MODUS_VIEW);

ModusController::ModusController(const ControllerContext& context)
    : Controller(context),
      weak_factory_(this) {
}

ModusController::~ModusController() {
}

QWidget* ModusController::CreateModusView2() {
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

  QObject::connect(view2_.get(), &QWidget::windowTitleChanged, [this](const QString& title) {
    controller_delegate_.SetTitle(title.toStdWString());
  });

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
//  scroll_area->setStyleSheet("background-color: white;");

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
