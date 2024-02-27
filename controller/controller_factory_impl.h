#pragma once

#include "common/aliases.h"
#include "common/master_data_services.h"
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
class Favourites;
class FileCache;
class LocalEvents;
class MasterDataServices;
class NodeEventProvider;
class NodeService;
class PortfolioManager;
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
  MasterDataServices& master_data_services_;
  AliasResolver alias_resolver_;
  TaskManager& task_manager_;
  NodeEventProvider& node_event_provider_;
  TimedDataService& timed_data_service_;
  NodeService& node_service_;
  PortfolioManager& portfolio_manager_;
  LocalEvents& local_events_;
  Favourites& favourites_;
  FileCache& file_cache_;
  BlinkerManager& blinker_manager_;
  PropertyService& property_service_;
  CreateTree& create_tree_;
};

inline std::unique_ptr<Controller> ControllerFactoryImpl::CreateController(
    unsigned command_id,
    ControllerDelegate& delegate,
    DialogService& dialog_service) {
  auto* registrar = GetControllerRegistrar(command_id);
  if (!registrar) {
    return nullptr;
  }

  if (registrar->window_info().requires_admin_rights() &&
      !master_data_services_.HasPrivilege(scada::Privilege::Configure)) {
    return nullptr;
  }

  return registrar->CreateController(ControllerContext{
      executor_, delegate, alias_resolver_, task_manager_,
      master_data_services_, node_event_provider_, master_data_services_,
      master_data_services_, timed_data_service_, node_service_,
      portfolio_manager_, local_events_, favourites_, file_cache_, profile_,
      dialog_service, blinker_manager_, create_tree_, property_service_});
}
