#include "components/node_table/node_table_component.h"

#include "components/node_table/node_table_controller.h"
#include "controller_registry.h"
#include "model/data_items_node_ids.h"
#include "model/history_node_ids.h"
#include "model/scada_node_ids.h"
#include "model/security_node_ids.h"
#include "node_service/node_service.h"

// NodeTableControllerImpl

template <scada::NumericId kNodeId>
class NodeTableControllerImpl : public NodeTableController {
 public:
  explicit NodeTableControllerImpl(const ControllerContext& context)
      : NodeTableController(context, GetParentNode(context.node_service_)) {}

 private:
  static NodeRef GetParentNode(NodeService& node_service) {
    return kNodeId != 0 ? node_service.GetNode(
                              scada::NodeId{kNodeId, NamespaceIndexes::SCADA})
                        : nullptr;
  }
};

const WindowInfo kTableEditorWindowInfo = {
    ID_TABLE_EDITOR,
    "TableEditor",
    u"Конфигурация",
    WIN_DISALLOW_NEW | WIN_REQUIRES_ADMIN,
    0,
    0,
    IDR_GRID_POPUP};

const WindowInfo kTsFormatsWindowInfo = {
    ID_TS_FORMATS_VIEW, "Params", u"Форматы", WIN_REQUIRES_ADMIN, 0, 0,
    IDR_GRID_POPUP};

const WindowInfo kUsersWindowInfo = {
    ID_USERS_VIEW, "Users", u"Пользователи", WIN_REQUIRES_ADMIN, 0, 0,
    IDR_GRID_POPUP};

const WindowInfo kSimulationSignalsWindowInfo = {ID_SIMULATION_ITEMS_VIEW,
                                                 "SimulationItems",
                                                 u"Эмулируемые сигналы",
                                                 WIN_REQUIRES_ADMIN,
                                                 0,
                                                 0,
                                                 IDR_GRID_POPUP};

const WindowInfo kHistoricalDatabasesWindowInfo = {ID_HISTORICAL_DB_VIEW,
                                                   "HistoricalDB",
                                                   u"Базы данных",
                                                   WIN_REQUIRES_ADMIN,
                                                   0,
                                                   0,
                                                   IDR_GRID_POPUP};

REGISTER_CONTROLLER(NodeTableControllerImpl<0>, kTableEditorWindowInfo);
REGISTER_CONTROLLER(NodeTableControllerImpl<data_items::numeric_id::TsFormats>,
                    kTsFormatsWindowInfo);
REGISTER_CONTROLLER(NodeTableControllerImpl<security::numeric_id::Users>,
                    kUsersWindowInfo);
REGISTER_CONTROLLER(
    NodeTableControllerImpl<data_items::numeric_id::SimulationSignals>,
    kSimulationSignalsWindowInfo);
REGISTER_CONTROLLER(
    NodeTableControllerImpl<history::numeric_id::HistoricalDatabases>,
    kHistoricalDatabasesWindowInfo);
