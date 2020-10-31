#include "components/main/actions.h"

#include "base/strings/utf_string_conversions.h"
#include "node_service/node_observer.h"
#include "node_service/node_service.h"
#include "common_resources.h"
#include "components/main/action.h"
#include "components/main/action_manager.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/filesystem_node_ids.h"
#include "model/history_node_ids.h"
#include "model/scada_node_ids.h"
#include "model/security_node_ids.h"

namespace {

// TODO(semenov): Refactor to avoid listing the types.
const scada::NodeId kNewCommandTypeIds[] = {
    data_items::id::DataGroupType,       data_items::id::DiscreteItemType,
    data_items::id::AnalogItemType,      security::id::UserType,
    history::id::HistoricalDatabaseType, data_items::id::SimulationSignalType,
    devices::id::Iec60870DeviceType,     devices::id::Iec61850DeviceType,
    devices::id::Iec61850RcbType,        devices::id::ModbusLinkType,
    devices::id::ModbusDeviceType,       data_items::id::TsFormatType,
    devices::id::TransmissionItemType,
};

class NodeAction : private NodeRefObserver, public Action {
 public:
  NodeAction(ActionManager& action_manager,
             unsigned command_id,
             CommandCategory category,
             const NodeRef& node)
      : Action(command_id, category, {}),
        action_manager_(action_manager),
        node_(std::move(node)) {
    node_.Subscribe(*this);
  }

  ~NodeAction() { node_.Unsubscribe(*this); }

  virtual base::string16 GetTitle() const override {
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
  //  AddAction(*new Action(ID_GRAPH_VIEW, CATEGORY_NEW, "Новый график", NULL,
  //  kGraphImageIndex)); AddAction(*new Action(ID_TABLE_VIEW, CATEGORY_NEW,
  //  "Новая таблица", NULL, kTableImageIndex));

  action_manager.AddAction(*new Action(ID_OPEN_GRAPH, CATEGORY_OPEN, L"График",
                                       base::string16(), ID_GRAPH_VIEW,
                                       Action::ALWAYS_VISIBLE));
  action_manager.AddAction(*new Action(ID_TIMED_DATA_VIEW, CATEGORY_OPEN,
                                       L"Данные", base::string16(),
                                       IDB_TIMED_DATA, Action::ALWAYS_VISIBLE));
  action_manager.AddAction(*new Action(ID_OPEN_DISPLAY, CATEGORY_OPEN, L"Схема",
                                       base::string16(), ID_MODUS_VIEW,
                                       Action::ALWAYS_VISIBLE));
  action_manager.AddAction(*new Action(ID_OPEN_TABLE, CATEGORY_OPEN, L"Таблица",
                                       base::string16(), ID_TABLE_VIEW,
                                       Action::ALWAYS_VISIBLE));
  action_manager.AddAction(*new Action(ID_OPEN_SUMMARY, CATEGORY_OPEN,
                                       L"Сводка", base::string16(), IDB_SUMMARY,
                                       Action::ALWAYS_VISIBLE));
  action_manager.AddAction(
      *new Action(ID_HISTORICAL_EVENTS, CATEGORY_OPEN, L"События",
                  base::string16(), IDB_OPEN_EVENTS, Action::ALWAYS_VISIBLE));

  action_manager.AddAction(*new Action(ID_OPEN_GROUP_TABLE, CATEGORY_OPEN,
                                       L"Таблица группы", base::string16(), 0,
                                       Action::VISIBLE));

  action_manager.AddAction(
      *new Action(ID_ACKNOWLEDGE_CURRENT, CATEGORY_ITEM, L"Квитировать"));
  action_manager.AddAction(*new Action(ID_UNLOCK_ITEM, CATEGORY_ITEM,
                                       L"Снять блокировку", base::string16(),
                                       IDB_UNLOCK));
  action_manager.AddAction(*new Action(
      ID_WRITE, CATEGORY_ITEM, L"Управление...", L"Управление", IDB_WRITE));
  action_manager.AddAction(*new Action(ID_WRITE_MANUAL, CATEGORY_ITEM,
                                       L"Ручной ввод...", L"Ручной ввод",
                                       IDB_WRITE_MANUAL));
  action_manager.AddAction(
      *new Action(ID_EDIT_LIMITS, CATEGORY_ITEM, L"Уставки...", L"Уставки"));

  action_manager.AddAction(
      *new Action(ID_DEV1_REFR, CATEGORY_DEVICE, L"Опросить устройство"));
  action_manager.AddAction(
      *new Action(ID_DEV1_SYNC, CATEGORY_DEVICE, L"Синхронизировать часы"));

  action_manager.AddAction(*new Action(ID_SETUP, CATEGORY_SETUP, L"Опции"));
  action_manager.AddAction(*new Action(ID_PRINT, CATEGORY_SETUP, L"Печать",
                                       base::string16(), IDB_PRINTER));
  action_manager.AddAction(*new Action(ID_EDIT, CATEGORY_SETUP, L"Правка"));

  action_manager.AddAction(
      *new Action(ID_EXPORT_CSV, CATEGORY_EXPORT, L"Экспорт в CSV"));
  action_manager.AddAction(
      *new Action(ID_EXPORT_EXCEL, CATEGORY_EXPORT, L"Экспорт в Excel"));

  action_manager.AddAction(
      *new Action(ID_OPEN_WATCH, CATEGORY_SPECIFIC, L"Наблюдение"));
  action_manager.AddAction(
      *new Action(ID_OPEN_DEVICE_METRICS, CATEGORY_SPECIFIC, L"Метрики"));
  action_manager.AddAction(*new Action(ID_CHANGE_PASSWORD, CATEGORY_SPECIFIC,
                                       L"Задать пароль...", L"Пароль"));
  action_manager.AddAction(
      *new Action(ID_ITEM_ENABLE, CATEGORY_SPECIFIC, L"Включить"));
  action_manager.AddAction(
      *new Action(ID_ITEM_DISABLE, CATEGORY_SPECIFIC, L"Отключить"));
  action_manager.AddAction(*new Action(ID_PAUSE, CATEGORY_SPECIFIC, L"Пауза"));

  action_manager.AddAction(*new Action(ID_ACKNOWLEDGE_ALL, CATEGORY_VIEW,
                                       L"Квитировать все", base::string16(),
                                       IDB_ACKNOWLEDGE_ALL));
  action_manager.AddAction(*new Action(ID_SEVERITY_CUSTOM, CATEGORY_VIEW,
                                       L"Важность...", L"Важность"));
  action_manager.AddAction(*new Action(ID_FAVOURITES_ADD_URL, CATEGORY_EDIT,
                                       L"Добавить Web-страницу...",
                                       L"Добавить Web-страницу"));
  action_manager.AddAction(
      *new Action(ID_MODUS_TOOLBAR, CATEGORY_VIEW, L"Панель инструментов"));
  action_manager.AddAction(
      *new Action(ID_MODUS_STATUSBAR, CATEGORY_VIEW, L"Строка состояния"));
  action_manager.AddAction(*new Action(ID_EVENT_VIEW, CATEGORY_VIEW,
                                       L"Панель событий", base::string16(),
                                       ID_EVENT_VIEW));
  action_manager.AddAction(*new Action(ID_SAVE, CATEGORY_VIEW, L"Сохранить"));
  action_manager.AddAction(*new Action(ID_SAVE_AS, CATEGORY_VIEW,
                                       L"Сохранить как...", L"Сохранить"));

  action_manager
      .AddAction(*new Action(ID_CURRENT_EVENTS, CATEGORY_PERIOD, L"Текущие"))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_TIME_RANGE_DAY, CATEGORY_PERIOD, L"День"))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_TIME_RANGE_WEEK, CATEGORY_PERIOD, L"Неделя"))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_TIME_RANGE_MONTH, CATEGORY_PERIOD, L"Месяц"))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_TIME_RANGE_CUSTOM, CATEGORY_PERIOD,
                             L"Другой...", L"Другой"))
      .set_checkable(true);

  action_manager
      .AddAction(*new Action(ID_INTERVAL_1M, CATEGORY_INTERVAL, L"Минутный"))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_INTERVAL_5M, CATEGORY_INTERVAL, L"5 минут"))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_INTERVAL_15M, CATEGORY_INTERVAL, L"15 минут"))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_INTERVAL_30M, CATEGORY_INTERVAL, L"30 минут"))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_INTERVAL_1H, CATEGORY_INTERVAL, L"Часовой"))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_INTERVAL_12H, CATEGORY_INTERVAL, L"12 часов"))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_INTERVAL_1D, CATEGORY_INTERVAL, L"Суточный"))
      .set_checkable(true);

  action_manager
      .AddAction(
          *new Action(ID_AGGREGATION_START, CATEGORY_AGGREGATION, L"Первое"))
      .set_checkable(true);
  action_manager
      .AddAction(
          *new Action(ID_AGGREGATION_END, CATEGORY_AGGREGATION, L"Последнее"))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_AGGREGATION_COUNT, CATEGORY_AGGREGATION,
                             L"Количество"))
      .set_checkable(true);
  action_manager
      .AddAction(
          *new Action(ID_AGGREGATION_MIN, CATEGORY_AGGREGATION, L"Минимум"))
      .set_checkable(true);
  action_manager
      .AddAction(
          *new Action(ID_AGGREGATION_MAX, CATEGORY_AGGREGATION, L"Максимум"))
      .set_checkable(true);
  action_manager
      .AddAction(
          *new Action(ID_AGGREGATION_SUM, CATEGORY_AGGREGATION, L"Сумма"))
      .set_checkable(true);
  action_manager
      .AddAction(
          *new Action(ID_AGGREGATION_AVG, CATEGORY_AGGREGATION, L"Среднее"))
      .set_checkable(true);

  action_manager.AddAction(*new Action(ID_ITEM_PARAMS, CATEGORY_EDIT,
                                       L"Параметры", base::string16(),
                                       IDB_RECORD_EDITOR));
  action_manager.AddAction(*new Action(ID_TABLE_CONFIG, CATEGORY_EDIT,
                                       L"Параметры элементов", L"Элементы"));
  action_manager.AddAction(*new Action(ID_TRANSMISSION_VIEW, CATEGORY_EDIT,
                                       L"Таблица ретрансляции",
                                       L"Ретрансляция"));
  action_manager.AddAction(
      *new Action(ID_NEW_PORTFOLIO, CATEGORY_EDIT, L"Создать портфолио"));
  action_manager.AddAction(*new Action(ID_ADD_ITEMS, CATEGORY_EDIT,
                                       L"Добавить объекты...",
                                       L"Добавить объекты"));
  action_manager.AddAction(
      *new Action(ID_RENAME, CATEGORY_EDIT, L"Переименовать"));
  action_manager.AddAction(*new Action(ID_COPY, CATEGORY_EDIT, L"Копировать",
                                       base::string16(), IDB_COPY));
  action_manager.AddAction(*new Action(ID_PASTE, CATEGORY_EDIT, L"Вставить",
                                       base::string16(), IDB_PASTE));
  action_manager.AddAction(*new Action(ID_DELETE, CATEGORY_EDIT, L"Удалить",
                                       base::string16(), IDB_DELETE));
  action_manager.AddAction(
      *new Action(ID_CLEAR_ALL, CATEGORY_EDIT, L"Очистить"));

  action_manager.AddAction(*new Action(ID_ADD_MULTIPLE_ITEMS, CATEGORY_CREATE,
                                       L"Серия объектов..."));
  action_manager.AddAction(*new Action(ID_NEW_SERVICE_ITEMS, CATEGORY_CREATE,
                                       L"Сервисные объекты..."));
  action_manager.AddAction(*new Action(ID_NEW_IEC60870_LINK101, CATEGORY_CREATE,
                                       L"Направление МЭК-60870-101"));
  action_manager.AddAction(*new Action(ID_NEW_IEC60870_LINK104, CATEGORY_CREATE,
                                       L"Направление МЭК-60870-104"));
  for (size_t i = 0; i < _countof(kNewCommandTypeIds); ++i) {
    action_manager.AddAction(
        *new NodeAction(action_manager, ID_NEW + i, CATEGORY_CREATE,
                        node_service.GetNode(kNewCommandTypeIds[i])));
  }
}
