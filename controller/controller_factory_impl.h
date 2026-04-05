#pragma once

#include "controller/controller.h"
#include "controller/controller_context.h"
#include "controller/controller_registry.h"

#include <memory>

class BlinkerManager;
class CreateTree;
class Controller;
class ControllerDelegate;
class DialogService;
class Executor;
class FileCache;
class NodeEventProvider;
class NodeService;
class Profile;
class PropertyService;
class TaskManager;
class TimedDataService;

struct ControllerFactoryImpl {
  std::unique_ptr<Controller> CreateController(unsigned command_id,
                                               ControllerDelegate& delegate,
                                               DialogService& dialog_service);

  std::shared_ptr<Executor> executor_;
  Profile& profile_;
  scada::services scada_services_;
  TaskManager& task_manager_;
  NodeEventProvider& node_event_provider_;
  TimedDataService& timed_data_service_;
  NodeService& node_service_;
  FileCache& file_cache_;
  BlinkerManager& blinker_manager_;
  PropertyService& property_service_;
  CreateTree& create_tree_;
};

inline std::unique_ptr<Controller> ControllerFactoryImpl::CreateController(
    unsigned command_id,
    ControllerDelegate& delegate,
    DialogService& dialog_service) {
  assert(scada_services_.session_service);

  auto* registrar = GetControllerRegistrar(command_id);
  if (!registrar) {
    return nullptr;
  }

  if (registrar->window_info().requires_admin_rights() &&
      !scada_services_.session_service->HasPrivilege(
          scada::Privilege::Configure)) {
    return nullptr;
  }

  return registrar->CreateController(ControllerContext{
      executor_, delegate, task_manager_, *scada_services_.session_service,
      node_event_provider_, *scada_services_.history_service,
      *scada_services_.monitored_item_service, timed_data_service_,
      node_service_, file_cache_, profile_, dialog_service, blinker_manager_,
      create_tree_, property_service_});
}
