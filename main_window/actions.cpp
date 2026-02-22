#include "main_window/actions.h"

#include "aui/key_codes.h"
#include "aui/translation.h"
#include "common_resources.h"
#include "main_window/action.h"
#include "main_window/action_manager.h"
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

class NodeAction : private NodeRefObserver, public Action {
 public:
  NodeAction(ActionManager& action_manager,
             unsigned command_id,
             CommandCategory category,
             NodeRef node)
      : Action(command_id, category, {}),
        action_manager_(action_manager),
        node_(std::move(node)) {
    node_.Subscribe(*this);
  }

  ~NodeAction() { node_.Unsubscribe(*this); }

  virtual std::u16string GetTitle() const override {
    return ToString16(node_.display_name());
  }

 private:
  // NodeRefObserver
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override {
    action_manager_.NotifyActionChanged(command_id_, ActionChangeMask::Title);
  }

  ActionManager& action_manager_;

  const NodeRef node_;

  const std::shared_ptr<NodeAction*> weak_ptr_factory_ =
      std::make_shared<NodeAction*>(this);
};

void RegisterCreateActions(ActionManager& action_manager,
                           NodeService& node_service) {
  action_manager.AddAction(*new Action(ID_ADD_MULTIPLE_ITEMS, CATEGORY_CREATE,
                                       Translate("Multiple Create...")));
  action_manager.AddAction(*new Action(ID_NEW_SERVICE_ITEMS, CATEGORY_CREATE,
                                       Translate("Service Items...")));
  action_manager.AddAction(*new Action(ID_NEW_IEC60870_LINK101, CATEGORY_CREATE,
                                       Translate("IEC 60870-101 Link")));
  action_manager.AddAction(*new Action(ID_NEW_IEC60870_LINK104, CATEGORY_CREATE,
                                       Translate("IEC 60870-104 Link")));

  for (size_t i = 0; i < _countof(kNewCommandTypeIds); ++i) {
    action_manager.AddAction(
        *new NodeAction(action_manager, ID_NEW + i, CATEGORY_CREATE,
                        node_service.GetNode(kNewCommandTypeIds[i])));
  }
}

void RegisterFileSystemActions(ActionManager& action_manager) {
  action_manager.AddAction(*new Action(ID_CREATE_FILE_DIRECTORY,
                                       CATEGORY_CREATE, Translate("Folder"),
                                       Translate("Create Folder...")));
  action_manager.AddAction(
      *new Action(ID_ADD_FILE, CATEGORY_CREATE, Translate("File"), Translate("Add File...")));
}

}  // namespace

scada::NodeId GetNewCommandTypeId(unsigned command_id) {
  if (command_id < ID_NEW)
    return scada::NodeId();

  auto index = command_id - ID_NEW;
  if (index >= _countof(kNewCommandTypeIds))
    return scada::NodeId();

  return kNewCommandTypeIds[index];
}

void AddGlobalActions(ActionManager& action_manager,
                      NodeService& node_service) {
  action_manager.AddAction(*new Action(ID_OPEN_GRAPH, CATEGORY_OPEN, Translate("Graph"),
                                       std::u16string(), ID_GRAPH_VIEW,
                                       Action::ALWAYS_VISIBLE));
  action_manager.AddAction(*new Action(ID_TIMED_DATA_VIEW, CATEGORY_OPEN,
                                       Translate("Data"), std::u16string(),
                                       IDB_TIMED_DATA, Action::ALWAYS_VISIBLE));
  action_manager.AddAction(*new Action(ID_OPEN_DISPLAY, CATEGORY_OPEN, Translate("Display"),
                                       std::u16string(), ID_MODUS_VIEW,
                                       Action::ALWAYS_VISIBLE));
  action_manager.AddAction(*new Action(ID_OPEN_TABLE, CATEGORY_OPEN, Translate("Table"),
                                       std::u16string(), ID_TABLE_VIEW,
                                       Action::ALWAYS_VISIBLE));
  action_manager.AddAction(*new Action(ID_OPEN_SUMMARY, CATEGORY_OPEN,
                                       Translate("Summary"), std::u16string(), IDB_SUMMARY,
                                       Action::ALWAYS_VISIBLE));
  action_manager.AddAction(
      *new Action(ID_HISTORICAL_EVENTS, CATEGORY_OPEN, Translate("Events"),
                  std::u16string(), IDB_OPEN_EVENTS, Action::ALWAYS_VISIBLE));

  action_manager.AddAction(*new Action(ID_OPEN_GROUP_TABLE, CATEGORY_OPEN,
                                       Translate("Group Table"), std::u16string(), 0,
                                       Action::VISIBLE));

  action_manager.AddAction(
      *new Action(ID_ACKNOWLEDGE_CURRENT, CATEGORY_ITEM, Translate("Acknowledge")));
  action_manager.AddAction(*new Action(ID_UNLOCK_ITEM, CATEGORY_ITEM,
                                       Translate("Unlock"), std::u16string(),
                                       IDB_UNLOCK));
  action_manager.AddAction(*new Action(
      ID_WRITE, CATEGORY_ITEM, Translate("Control..."), Translate("Control"), IDB_WRITE));
  action_manager.AddAction(*new Action(ID_WRITE_MANUAL, CATEGORY_ITEM,
                                       Translate("Manual Input..."), Translate("Manual Input"),
                                       IDB_WRITE_MANUAL));
  action_manager.AddAction(
      *new Action(ID_EDIT_LIMITS, CATEGORY_ITEM, Translate("Limits..."), Translate("Limits")));

  action_manager.AddAction(
      *new Action(ID_DEV1_REFR, CATEGORY_DEVICE, Translate("Poll Device")));
  action_manager.AddAction(
      *new Action(ID_DEV1_SYNC, CATEGORY_DEVICE, Translate("Synchronize Clock")));

  action_manager.AddAction(*new Action(ID_SETUP, CATEGORY_SETUP, Translate("Options")));
  action_manager.AddAction(*new Action(ID_PRINT, CATEGORY_SETUP, Translate("Print"),
                                       std::u16string(), IDB_PRINTER));
  action_manager.AddAction(*new Action(ID_EDIT, CATEGORY_SETUP, Translate("Edit")))
      .set_checkable(true);

  action_manager.AddAction(
      *new Action(ID_EXPORT_CSV, CATEGORY_EXPORT, Translate("Export to CSV")));
  action_manager.AddAction(
      *new Action(ID_EXPORT_EXCEL, CATEGORY_EXPORT, Translate("Export to Excel")));

  action_manager.AddAction(
      *new Action(ID_OPEN_WATCH, CATEGORY_SPECIFIC, Translate("Watch")));
  action_manager.AddAction(
      *new Action(ID_OPEN_DEVICE_METRICS, CATEGORY_SPECIFIC, Translate("Metrics")));
  action_manager.AddAction(*new Action(ID_CHANGE_PASSWORD, CATEGORY_SPECIFIC,
                                       Translate("Set Password..."), Translate("Password")));
  action_manager.AddAction(
      *new Action(ID_ITEM_ENABLE, CATEGORY_SPECIFIC, Translate("Enable")));
  action_manager.AddAction(
      *new Action(ID_ITEM_DISABLE, CATEGORY_SPECIFIC, Translate("Disable")));
  action_manager.AddAction(*new Action(ID_PAUSE, CATEGORY_SPECIFIC, Translate("Pause")));

  action_manager.AddAction(*new Action(ID_ACKNOWLEDGE_ALL, CATEGORY_VIEW,
                                       Translate("Acknowledge All"), std::u16string(),
                                       IDB_ACKNOWLEDGE_ALL));
  action_manager.AddAction(*new Action(ID_SEVERITY_CUSTOM, CATEGORY_VIEW,
                                       Translate("Severity..."), Translate("Severity")));
  action_manager.AddAction(*new Action(ID_FAVOURITES_ADD_URL, CATEGORY_EDIT,
                                       Translate("Add Web Page..."),
                                       Translate("Add Web Page")));
  action_manager.AddAction(
      *new Action(ID_MODUS_TOOLBAR, CATEGORY_VIEW, Translate("Toolbar")));
  action_manager.AddAction(
      *new Action(ID_MODUS_STATUSBAR, CATEGORY_VIEW, Translate("Status Bar")));
  action_manager.AddAction(*new Action(ID_EVENT_VIEW, CATEGORY_VIEW,
                                       Translate("Event Panel"), std::u16string(),
                                       ID_EVENT_VIEW));
  action_manager.AddAction(*new Action(ID_SAVE, CATEGORY_VIEW, Translate("Save")));
  action_manager.AddAction(*new Action(ID_SAVE_AS, CATEGORY_VIEW,
                                       Translate("Save As..."), Translate("Save")));

  action_manager
      .AddAction(*new Action(ID_CURRENT_EVENTS, CATEGORY_PERIOD, Translate("Current")))
      .set_checkable(true);

  action_manager
      .AddAction(*new Action(ID_TIME_RANGE_15M, CATEGORY_PERIOD, Translate("15 min")))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_TIME_RANGE_HOUR, CATEGORY_PERIOD, Translate("Hour")))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_TIME_RANGE_DAY, CATEGORY_PERIOD, Translate("Day")))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_TIME_RANGE_WEEK, CATEGORY_PERIOD, Translate("Week")))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_TIME_RANGE_MONTH, CATEGORY_PERIOD, Translate("Month")))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_TIME_RANGE_CUSTOM, CATEGORY_PERIOD,
                             Translate("Custom..."), Translate("Custom")))
      .set_checkable(true);

  action_manager
      .AddAction(*new Action(ID_INTERVAL_1M, CATEGORY_INTERVAL, Translate("1-Minute")))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_INTERVAL_5M, CATEGORY_INTERVAL, Translate("5 min")))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_INTERVAL_15M, CATEGORY_INTERVAL, Translate("15 min")))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_INTERVAL_30M, CATEGORY_INTERVAL, Translate("30 min")))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_INTERVAL_1H, CATEGORY_INTERVAL, Translate("1-Hour")))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_INTERVAL_12H, CATEGORY_INTERVAL, Translate("12 hours")))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_INTERVAL_1D, CATEGORY_INTERVAL, Translate("1-Day")))
      .set_checkable(true);

  action_manager
      .AddAction(
          *new Action(ID_AGGREGATION_START, CATEGORY_AGGREGATION, Translate("First")))
      .set_checkable(true);
  action_manager
      .AddAction(
          *new Action(ID_AGGREGATION_END, CATEGORY_AGGREGATION, Translate("Last")))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_AGGREGATION_COUNT, CATEGORY_AGGREGATION,
                             Translate("Count")))
      .set_checkable(true);
  action_manager
      .AddAction(
          *new Action(ID_AGGREGATION_MIN, CATEGORY_AGGREGATION, Translate("Minimum")))
      .set_checkable(true);
  action_manager
      .AddAction(
          *new Action(ID_AGGREGATION_MAX, CATEGORY_AGGREGATION, Translate("Maximum")))
      .set_checkable(true);
  action_manager
      .AddAction(
          *new Action(ID_AGGREGATION_SUM, CATEGORY_AGGREGATION, Translate("Sum")))
      .set_checkable(true);
  action_manager
      .AddAction(
          *new Action(ID_AGGREGATION_AVG, CATEGORY_AGGREGATION, Translate("Average")))
      .set_checkable(true);

  action_manager.AddAction(*new Action(ID_ITEM_PARAMS, CATEGORY_EDIT,
                                       Translate("Properties"), std::u16string(),
                                       IDB_RECORD_EDITOR));
  action_manager.AddAction(*new Action(ID_TABLE_CONFIG, CATEGORY_EDIT,
                                       Translate("Element Properties"), Translate("Elements")));
  action_manager.AddAction(*new Action(ID_TRANSMISSION_VIEW, CATEGORY_EDIT,
                                       Translate("Transmission Table"),
                                       Translate("Transmission")));
  action_manager.AddAction(
      *new Action(ID_NEW_PORTFOLIO, CATEGORY_EDIT, Translate("Create Portfolio")));
  action_manager.AddAction(*new Action(ID_ADD_ITEMS, CATEGORY_EDIT,
                                       Translate("Add Items..."),
                                       Translate("Add Items")));
  action_manager.AddAction(
      *ActionBuilder(ID_RENAME, CATEGORY_EDIT, Translate("Rename"))
           .shortcut(aui::KeyCode::F2)
           .Build());
  action_manager.AddAction(
      *ActionBuilder(ID_COPY, CATEGORY_EDIT, Translate("Copy"))
           .image_id(IDB_COPY)
           .shortcut({aui::ControlModifier, aui::KeyCode::C})
           .Build());
  action_manager.AddAction(
      *ActionBuilder(ID_PASTE, CATEGORY_EDIT, Translate("Paste"))
           .image_id(IDB_PASTE)
           .shortcut({aui::ControlModifier, aui::KeyCode::V})
           .Build());
  action_manager.AddAction(*ActionBuilder(ID_DELETE, CATEGORY_EDIT, Translate("Delete"))
                                .image_id(IDB_DELETE)
                                .shortcut(aui::KeyCode::Delete)
                                .Build());
  action_manager.AddAction(
      *new Action(ID_CLEAR_ALL, CATEGORY_EDIT, Translate("Clear")));

  RegisterCreateActions(action_manager, node_service);

  RegisterFileSystemActions(action_manager);
}
