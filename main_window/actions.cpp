#include "main_window/actions.h"

#include "aui/key_codes.h"
#include "aui/translation.h"
#include "resources/common_resources.h"
#include "controller/action.h"
#include "controller/action_manager.h"
#include "controller/command_ui_registry.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/filesystem_node_ids.h"
#include "model/history_node_ids.h"
#include "model/security_node_ids.h"
#include "node_service/node_observer.h"
#include "node_service/node_service.h"

namespace {

// TODO(semenov): Refactor to avoid listing the types.
const scada::NodeId kNewCommandTypeIds[] = {
    data_items::id::DataGroupType,
    data_items::id::DiscreteItemType,
    data_items::id::AnalogItemType,
    security::id::UserType,
    history::id::HistoricalDatabaseType,
    data_items::id::SimulationSignalType,
    devices::id::Iec60870DeviceType,
    devices::id::Iec61850DeviceType,
    devices::id::Iec61850RcbType,
    devices::id::ModbusLinkType,
    devices::id::ModbusDeviceType,
    data_items::id::TsFormatType,
    devices::id::ModbusTransmissionItemType,
    devices::id::Iec60870TransmissionItemType,
    devices::id::Iec61850TransmissionItemType,
};

class NodeActionTitle : private NodeRefObserver {
 public:
  NodeActionTitle(ActionManager& action_manager,
                  unsigned command_id,
                  NodeRef node)
      : action_manager_(action_manager),
        command_id_(command_id),
        node_(std::move(node)) {
    node_.Subscribe(*this);
  }

  ~NodeActionTitle() { node_.Unsubscribe(*this); }

  std::u16string GetTitle() const {
    return ToString16(node_.display_name());
  }

 private:
  // NodeRefObserver
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override {
    action_manager_.NotifyActionChanged(command_id_, ActionChangeMask::Title);
  }

  ActionManager& action_manager_;
  const unsigned command_id_;

  const NodeRef node_;

  const std::shared_ptr<NodeActionTitle*> weak_ptr_factory_ =
      std::make_shared<NodeActionTitle*>(this);
};

Action MakeNodeAction(ActionManager& action_manager,
                      unsigned command_id,
                      CommandCategory category,
                      NodeRef node) {
  auto title = std::make_shared<NodeActionTitle>(action_manager, command_id,
                                                std::move(node));
  return Action{
      .command_id_ = command_id,
      .category_ = category,
      .title_provider_ = [title] { return title->GetTitle(); },
  };
}

void RegisterCreateActions(ActionManager& action_manager,
                           NodeService& node_service) {
  action_manager.AddAction(Action{.command_id_ = ID_ADD_MULTIPLE_ITEMS,
                                       .category_ = CATEGORY_CREATE,
                                       .title_ = Translate("Multiple Create...")});
  action_manager.AddAction(Action{.command_id_ = ID_NEW_SERVICE_ITEMS,
                                       .category_ = CATEGORY_CREATE,
                                       .title_ = Translate("Service Items...")});
  action_manager.AddAction(Action{.command_id_ = ID_NEW_IEC60870_LINK101,
                                       .category_ = CATEGORY_CREATE,
                                       .title_ = Translate("IEC 60870-101 Link")});
  action_manager.AddAction(Action{.command_id_ = ID_NEW_IEC60870_LINK104,
                                       .category_ = CATEGORY_CREATE,
                                       .title_ = Translate("IEC 60870-104 Link")});

  for (size_t i = 0; i < std::size(kNewCommandTypeIds); ++i) {
    action_manager.AddAction(MakeNodeAction(
        action_manager, ID_NEW + i, CATEGORY_CREATE,
        node_service.GetNode(kNewCommandTypeIds[i])));
  }
}

void RegisterFileSystemActions(ActionManager& action_manager) {
  action_manager.AddAction(Action{.command_id_ = ID_CREATE_FILE_DIRECTORY,
                                       .category_ = CATEGORY_CREATE,
                                       .title_ = Translate("Folder"),
                                       .short_title_ =
                                           Translate("Create Folder...")});
  action_manager.AddAction(Action{.command_id_ = ID_ADD_FILE,
                                       .category_ = CATEGORY_CREATE,
                                       .title_ = Translate("File"),
                                       .short_title_ = Translate("Add File...")});
}

}  // namespace

scada::NodeId GetNewCommandTypeId(unsigned command_id) {
  if (command_id < ID_NEW)
    return scada::NodeId();

  auto index = command_id - ID_NEW;
  if (index >= std::size(kNewCommandTypeIds))
    return scada::NodeId();

  return kNewCommandTypeIds[index];
}

void AddGlobalActions(ActionManager& action_manager,
                      NodeService& node_service) {
  action_manager.AddAction(Action{.command_id_ = ID_OPEN_GRAPH,
                                       .category_ = CATEGORY_OPEN,
                                       .title_ = Translate("Graph"),
                                       .image_id_ = ID_GRAPH_VIEW,
                                       .flags_ = Action::ALWAYS_VISIBLE});
  action_manager.AddAction(Action{.command_id_ = ID_TIMED_DATA_VIEW,
                                       .category_ = CATEGORY_OPEN,
                                       .title_ = Translate("Data"),
                                       .image_id_ = IDB_TIMED_DATA,
                                       .flags_ = Action::ALWAYS_VISIBLE});
  action_manager.AddAction(Action{.command_id_ = ID_OPEN_DISPLAY,
                                       .category_ = CATEGORY_OPEN,
                                       .title_ = Translate("Display"),
                                       .image_id_ = ID_MODUS_VIEW,
                                       .flags_ = Action::ALWAYS_VISIBLE});
  action_manager.AddAction(Action{.command_id_ = ID_OPEN_TABLE,
                                       .category_ = CATEGORY_OPEN,
                                       .title_ = Translate("Table"),
                                       .image_id_ = ID_TABLE_VIEW,
                                       .flags_ = Action::ALWAYS_VISIBLE});
  action_manager.AddAction(Action{.command_id_ = ID_OPEN_SUMMARY,
                                       .category_ = CATEGORY_OPEN,
                                       .title_ = Translate("Summary"),
                                       .image_id_ = IDB_SUMMARY,
                                       .flags_ = Action::ALWAYS_VISIBLE});
  action_manager.AddAction(Action{.command_id_ = ID_HISTORICAL_EVENTS,
                                       .category_ = CATEGORY_OPEN,
                                       .title_ = Translate("Events"),
                                       .image_id_ = IDB_OPEN_EVENTS,
                                       .flags_ = Action::ALWAYS_VISIBLE});

  action_manager.AddAction(Action{.command_id_ = ID_OPEN_GROUP_TABLE,
                                       .category_ = CATEGORY_OPEN,
                                       .title_ = Translate("Group Table"),
                                       .flags_ = Action::VISIBLE});

  action_manager.AddAction(Action{.command_id_ = ID_ACKNOWLEDGE_CURRENT,
                                       .category_ = CATEGORY_ITEM,
                                       .title_ = Translate("Acknowledge")});
  action_manager.AddAction(Action{.command_id_ = ID_UNLOCK_ITEM,
                                       .category_ = CATEGORY_ITEM,
                                       .title_ = Translate("Unlock"),
                                       .image_id_ = IDB_UNLOCK});
  action_manager.AddAction(Action{.command_id_ = ID_WRITE,
                                       .category_ = CATEGORY_ITEM,
                                       .title_ = Translate("Control..."),
                                       .short_title_ = Translate("Control"),
                                       .image_id_ = IDB_WRITE});
  action_manager.AddAction(Action{.command_id_ = ID_WRITE_MANUAL,
                                       .category_ = CATEGORY_ITEM,
                                       .title_ = Translate("Manual Input..."),
                                       .short_title_ =
                                           Translate("Manual Input"),
                                       .image_id_ = IDB_WRITE_MANUAL});
  action_manager.AddAction(Action{.command_id_ = ID_EDIT_LIMITS,
                                       .category_ = CATEGORY_ITEM,
                                       .title_ = Translate("Limits..."),
                                       .short_title_ = Translate("Limits")});

  action_manager.AddAction(Action{.command_id_ = ID_DEV1_REFR,
                                       .category_ = CATEGORY_DEVICE,
                                       .title_ = Translate("Poll Device")});
  action_manager.AddAction(Action{.command_id_ = ID_DEV1_SYNC,
                                       .category_ = CATEGORY_DEVICE,
                                       .title_ =
                                           Translate("Synchronize Clock")});

  action_manager.AddAction(Action{.command_id_ = ID_SETUP,
                                       .category_ = CATEGORY_SETUP,
                                       .title_ = Translate("Options")});
  action_manager.AddAction(Action{.command_id_ = ID_PRINT,
                                       .category_ = CATEGORY_SETUP,
                                       .title_ = Translate("Print"),
                                       .image_id_ = IDB_PRINTER});
  action_manager.AddAction(Action{.command_id_ = ID_EDIT,
                                       .category_ = CATEGORY_SETUP,
                                       .title_ = Translate("Edit"),
                                       .flags_ = Action::CHECKABLE});

  action_manager.AddAction(Action{.command_id_ = ID_EXPORT_CSV,
                                       .category_ = CATEGORY_EXPORT,
                                       .title_ = Translate("Export to CSV")});
  action_manager.AddAction(Action{.command_id_ = ID_EXPORT_EXCEL,
                                       .category_ = CATEGORY_EXPORT,
                                       .title_ = Translate("Export to Excel")});

  action_manager.AddAction(Action{.command_id_ = ID_OPEN_WATCH,
                                       .category_ = CATEGORY_SPECIFIC,
                                       .title_ = Translate("Watch")});
  action_manager.AddAction(Action{.command_id_ = ID_OPEN_DEVICE_METRICS,
                                       .category_ = CATEGORY_SPECIFIC,
                                       .title_ = Translate("Metrics")});
  action_manager.AddAction(Action{.command_id_ = ID_CHANGE_PASSWORD,
                                       .category_ = CATEGORY_SPECIFIC,
                                       .title_ =
                                           Translate("Set Password..."),
                                       .short_title_ = Translate("Password")});
  action_manager.AddAction(Action{.command_id_ = ID_ITEM_ENABLE,
                                       .category_ = CATEGORY_SPECIFIC,
                                       .title_ = Translate("Enable")});
  action_manager.AddAction(Action{.command_id_ = ID_ITEM_DISABLE,
                                       .category_ = CATEGORY_SPECIFIC,
                                       .title_ = Translate("Disable")});
  action_manager.AddAction(Action{.command_id_ = ID_PAUSE,
                                       .category_ = CATEGORY_SPECIFIC,
                                       .title_ = Translate("Pause")});

  action_manager.AddAction(Action{.command_id_ = ID_ACKNOWLEDGE_ALL,
                                       .category_ = CATEGORY_VIEW,
                                       .title_ = Translate("Acknowledge All"),
                                       .image_id_ = IDB_ACKNOWLEDGE_ALL});
  action_manager.AddAction(Action{.command_id_ = ID_SEVERITY_CUSTOM,
                                       .category_ = CATEGORY_VIEW,
                                       .title_ = Translate("Severity..."),
                                       .short_title_ = Translate("Severity")});
  action_manager.AddAction(Action{.command_id_ = ID_FAVOURITES_ADD_URL,
                                       .category_ = CATEGORY_EDIT,
                                       .title_ = Translate("Add Web Page..."),
                                       .short_title_ =
                                           Translate("Add Web Page")});
  action_manager.AddAction(Action{.command_id_ = ID_MODUS_TOOLBAR,
                                       .category_ = CATEGORY_VIEW,
                                       .title_ = Translate("Toolbar")});
  action_manager.AddAction(Action{.command_id_ = ID_MODUS_STATUSBAR,
                                       .category_ = CATEGORY_VIEW,
                                       .title_ = Translate("Status Bar")});
  action_manager.AddAction(Action{.command_id_ = ID_EVENT_VIEW,
                                       .category_ = CATEGORY_VIEW,
                                       .title_ = Translate("Event Panel"),
                                       .image_id_ = ID_EVENT_VIEW});
  action_manager.AddAction(Action{.command_id_ = ID_SAVE,
                                       .category_ = CATEGORY_VIEW,
                                       .title_ = Translate("Save")});
  action_manager.AddAction(Action{.command_id_ = ID_SAVE_AS,
                                       .category_ = CATEGORY_VIEW,
                                       .title_ = Translate("Save As..."),
                                       .short_title_ = Translate("Save")});

  action_manager.AddAction(Action{.command_id_ = ID_CURRENT_EVENTS,
                                       .category_ = CATEGORY_PERIOD,
                                       .title_ = Translate("Current"),
                                       .flags_ = Action::CHECKABLE});

  action_manager.AddAction(Action{.command_id_ = ID_TIME_RANGE_15M,
                                       .category_ = CATEGORY_PERIOD,
                                       .title_ = Translate("15 min"),
                                       .flags_ = Action::CHECKABLE});
  action_manager.AddAction(Action{.command_id_ = ID_TIME_RANGE_HOUR,
                                       .category_ = CATEGORY_PERIOD,
                                       .title_ = Translate("Hour"),
                                       .flags_ = Action::CHECKABLE});
  action_manager.AddAction(Action{.command_id_ = ID_TIME_RANGE_DAY,
                                       .category_ = CATEGORY_PERIOD,
                                       .title_ = Translate("Day"),
                                       .flags_ = Action::CHECKABLE});
  action_manager.AddAction(Action{.command_id_ = ID_TIME_RANGE_WEEK,
                                       .category_ = CATEGORY_PERIOD,
                                       .title_ = Translate("Week"),
                                       .flags_ = Action::CHECKABLE});
  action_manager.AddAction(Action{.command_id_ = ID_TIME_RANGE_MONTH,
                                       .category_ = CATEGORY_PERIOD,
                                       .title_ = Translate("Month"),
                                       .flags_ = Action::CHECKABLE});
  action_manager.AddAction(Action{.command_id_ = ID_TIME_RANGE_CUSTOM,
                                       .category_ = CATEGORY_PERIOD,
                                       .title_ = Translate("Custom..."),
                                       .short_title_ = Translate("Custom"),
                                       .flags_ = Action::CHECKABLE});

  action_manager.AddAction(Action{.command_id_ = ID_INTERVAL_1M,
                                       .category_ = CATEGORY_INTERVAL,
                                       .title_ = Translate("1-Minute"),
                                       .flags_ = Action::CHECKABLE});
  action_manager.AddAction(Action{.command_id_ = ID_INTERVAL_5M,
                                       .category_ = CATEGORY_INTERVAL,
                                       .title_ = Translate("5 min"),
                                       .flags_ = Action::CHECKABLE});
  action_manager.AddAction(Action{.command_id_ = ID_INTERVAL_15M,
                                       .category_ = CATEGORY_INTERVAL,
                                       .title_ = Translate("15 min"),
                                       .flags_ = Action::CHECKABLE});
  action_manager.AddAction(Action{.command_id_ = ID_INTERVAL_30M,
                                       .category_ = CATEGORY_INTERVAL,
                                       .title_ = Translate("30 min"),
                                       .flags_ = Action::CHECKABLE});
  action_manager.AddAction(Action{.command_id_ = ID_INTERVAL_1H,
                                       .category_ = CATEGORY_INTERVAL,
                                       .title_ = Translate("1-Hour"),
                                       .flags_ = Action::CHECKABLE});
  action_manager.AddAction(Action{.command_id_ = ID_INTERVAL_12H,
                                       .category_ = CATEGORY_INTERVAL,
                                       .title_ = Translate("12 hours"),
                                       .flags_ = Action::CHECKABLE});
  action_manager.AddAction(Action{.command_id_ = ID_INTERVAL_1D,
                                       .category_ = CATEGORY_INTERVAL,
                                       .title_ = Translate("1-Day"),
                                       .flags_ = Action::CHECKABLE});

  action_manager.AddAction(Action{.command_id_ = ID_AGGREGATION_START,
                                       .category_ = CATEGORY_AGGREGATION,
                                       .title_ = Translate("First"),
                                       .flags_ = Action::CHECKABLE});
  action_manager.AddAction(Action{.command_id_ = ID_AGGREGATION_END,
                                       .category_ = CATEGORY_AGGREGATION,
                                       .title_ = Translate("Last"),
                                       .flags_ = Action::CHECKABLE});
  action_manager.AddAction(Action{.command_id_ = ID_AGGREGATION_COUNT,
                                       .category_ = CATEGORY_AGGREGATION,
                                       .title_ = Translate("Count"),
                                       .flags_ = Action::CHECKABLE});
  action_manager.AddAction(Action{.command_id_ = ID_AGGREGATION_MIN,
                                       .category_ = CATEGORY_AGGREGATION,
                                       .title_ = Translate("Minimum"),
                                       .flags_ = Action::CHECKABLE});
  action_manager.AddAction(Action{.command_id_ = ID_AGGREGATION_MAX,
                                       .category_ = CATEGORY_AGGREGATION,
                                       .title_ = Translate("Maximum"),
                                       .flags_ = Action::CHECKABLE});
  action_manager.AddAction(Action{.command_id_ = ID_AGGREGATION_SUM,
                                       .category_ = CATEGORY_AGGREGATION,
                                       .title_ = Translate("Sum"),
                                       .flags_ = Action::CHECKABLE});
  action_manager.AddAction(Action{.command_id_ = ID_AGGREGATION_AVG,
                                       .category_ = CATEGORY_AGGREGATION,
                                       .title_ = Translate("Average"),
                                       .flags_ = Action::CHECKABLE});

  action_manager.AddAction(Action{.command_id_ = ID_ITEM_PARAMS,
                                       .category_ = CATEGORY_EDIT,
                                       .title_ = Translate("Properties"),
                                       .image_id_ = IDB_RECORD_EDITOR});
  action_manager.AddAction(Action{.command_id_ = ID_TABLE_CONFIG,
                                       .category_ = CATEGORY_EDIT,
                                       .title_ =
                                           Translate("Element Properties"),
                                       .short_title_ = Translate("Elements")});
  action_manager.AddAction(Action{.command_id_ = ID_TRANSMISSION_VIEW,
                                       .category_ = CATEGORY_EDIT,
                                       .title_ =
                                           Translate("Transmission Table"),
                                       .short_title_ =
                                           Translate("Transmission")});
  action_manager.AddAction(Action{.command_id_ = ID_NEW_PORTFOLIO,
                                       .category_ = CATEGORY_EDIT,
                                       .title_ =
                                           Translate("Create Portfolio")});
  action_manager.AddAction(Action{.command_id_ = ID_ADD_ITEMS,
                                       .category_ = CATEGORY_EDIT,
                                       .title_ = Translate("Add Items..."),
                                       .short_title_ =
                                           Translate("Add Items")});
  action_manager.AddAction(Action{.command_id_ = ID_RENAME,
                                       .category_ = CATEGORY_EDIT,
                                       .title_ = Translate("Rename"),
                                       .shortcut_ =
                                           Shortcut{aui::KeyCode::F2}});
  action_manager.AddAction(Action{
      .command_id_ = ID_COPY,
      .category_ = CATEGORY_EDIT,
      .title_ = Translate("Copy"),
      .image_id_ = IDB_COPY,
      .shortcut_ = Shortcut{aui::ControlModifier, aui::KeyCode::C}});
  action_manager.AddAction(Action{
      .command_id_ = ID_PASTE,
      .category_ = CATEGORY_EDIT,
      .title_ = Translate("Paste"),
      .image_id_ = IDB_PASTE,
      .shortcut_ = Shortcut{aui::ControlModifier, aui::KeyCode::V}});
  action_manager.AddAction(Action{.command_id_ = ID_DELETE,
                                       .category_ = CATEGORY_EDIT,
                                       .title_ = Translate("Delete"),
                                       .image_id_ = IDB_DELETE,
                                       .shortcut_ =
                                           Shortcut{aui::KeyCode::Delete}});
  action_manager.AddAction(Action{.command_id_ = ID_CLEAR_ALL,
                                       .category_ = CATEGORY_EDIT,
                                       .title_ = Translate("Clear")});

  RegisterCreateActions(action_manager, node_service);

  RegisterFileSystemActions(action_manager);
}

void AddDefaultMenuContributions(UiCommandRegistry& ui_command_registry) {
  ui_command_registry.AddMenuItem(
      {.menu_id = MainMenuId::Table,
       .order = 100,
       .command_id = ID_TABLE_VIEW,
       .title = Translate("New Table")});
  ui_command_registry.AddMenuItem(
      {.menu_id = MainMenuId::Table,
       .order = 110,
       .command_id = ID_SHEET_VIEW,
       .title = Translate("New Custom Table")});
  ui_command_registry.AddMenuItem(
      {.menu_id = MainMenuId::Table,
       .order = 120,
       .command_id = ID_TIMED_DATA_VIEW,
       .title = Translate("New Data Table")});
  ui_command_registry.AddMenuItem(
      {.menu_id = MainMenuId::Table,
       .order = 200,
       .command_id = ID_OPEN_GROUP_TABLE,
       .title = Translate("Group Table"),
       .separator_before = true});

  ui_command_registry.AddMenuItem(
      {.menu_id = MainMenuId::Graph,
       .order = 100,
       .command_id = ID_GRAPH_VIEW,
       .title = Translate("New")});

  ui_command_registry.AddMenuItem(
      {.menu_id = MainMenuId::More,
       .order = 210,
       .command_id = ID_TS_FORMATS_VIEW,
       .title = Translate("Formats"),
       .checkable = true,
       .admin_only = true});
  ui_command_registry.AddMenuItem(
      {.menu_id = MainMenuId::More,
       .order = 220,
       .command_id = ID_SIMULATION_ITEMS_VIEW,
       .title = Translate("Simulated Signals"),
       .checkable = true,
       .admin_only = true});
  ui_command_registry.AddMenuItem(
      {.menu_id = MainMenuId::More,
       .order = 230,
       .command_id = ID_USERS_VIEW,
       .title = Translate("Users"),
       .checkable = true,
       .admin_only = true});
  ui_command_registry.AddMenuItem(
      {.menu_id = MainMenuId::More,
       .order = 240,
       .command_id = ID_HISTORICAL_DB_VIEW,
       .title = Translate("Databases"),
       .checkable = true,
       .admin_only = true});
}
