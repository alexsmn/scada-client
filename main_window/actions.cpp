#include "main_window/actions.h"

#include "base/strings/utf_string_conversions.h"
#include "common_resources.h"
#include "main_window/action.h"
#include "main_window/action_manager.h"
#include "aui/key_codes.h"
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
                                       u"Серия объектов..."));
  action_manager.AddAction(*new Action(ID_NEW_SERVICE_ITEMS, CATEGORY_CREATE,
                                       u"Сервисные объекты..."));
  action_manager.AddAction(*new Action(ID_NEW_IEC60870_LINK101, CATEGORY_CREATE,
                                       u"Направление МЭК-60870-101"));
  action_manager.AddAction(*new Action(ID_NEW_IEC60870_LINK104, CATEGORY_CREATE,
                                       u"Направление МЭК-60870-104"));

  for (size_t i = 0; i < _countof(kNewCommandTypeIds); ++i) {
    action_manager.AddAction(
        *new NodeAction(action_manager, ID_NEW + i, CATEGORY_CREATE,
                        node_service.GetNode(kNewCommandTypeIds[i])));
  }
}

void RegisterFileSystemActions(ActionManager& action_manager) {
  action_manager.AddAction(*new Action(ID_CREATE_FILE_DIRECTORY,
                                       CATEGORY_CREATE, u"Папка",
                                       u"Создать папку..."));
  action_manager.AddAction(
      *new Action(ID_ADD_FILE, CATEGORY_CREATE, u"Файл", u"Добавить файл..."));
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
  action_manager.AddAction(*new Action(ID_OPEN_GRAPH, CATEGORY_OPEN, u"График",
                                       std::u16string(), ID_GRAPH_VIEW,
                                       Action::ALWAYS_VISIBLE));
  action_manager.AddAction(*new Action(ID_TIMED_DATA_VIEW, CATEGORY_OPEN,
                                       u"Данные", std::u16string(),
                                       IDB_TIMED_DATA, Action::ALWAYS_VISIBLE));
  action_manager.AddAction(*new Action(ID_OPEN_DISPLAY, CATEGORY_OPEN, u"Схема",
                                       std::u16string(), ID_MODUS_VIEW,
                                       Action::ALWAYS_VISIBLE));
  action_manager.AddAction(*new Action(ID_OPEN_TABLE, CATEGORY_OPEN, u"Таблица",
                                       std::u16string(), ID_TABLE_VIEW,
                                       Action::ALWAYS_VISIBLE));
  action_manager.AddAction(*new Action(ID_OPEN_SUMMARY, CATEGORY_OPEN,
                                       u"Сводка", std::u16string(), IDB_SUMMARY,
                                       Action::ALWAYS_VISIBLE));
  action_manager.AddAction(
      *new Action(ID_HISTORICAL_EVENTS, CATEGORY_OPEN, u"События",
                  std::u16string(), IDB_OPEN_EVENTS, Action::ALWAYS_VISIBLE));

  action_manager.AddAction(*new Action(ID_OPEN_GROUP_TABLE, CATEGORY_OPEN,
                                       u"Таблица группы", std::u16string(), 0,
                                       Action::VISIBLE));

  action_manager.AddAction(
      *new Action(ID_ACKNOWLEDGE_CURRENT, CATEGORY_ITEM, u"Квитировать"));
  action_manager.AddAction(*new Action(ID_UNLOCK_ITEM, CATEGORY_ITEM,
                                       u"Снять блокировку", std::u16string(),
                                       IDB_UNLOCK));
  action_manager.AddAction(*new Action(
      ID_WRITE, CATEGORY_ITEM, u"Управление...", u"Управление", IDB_WRITE));
  action_manager.AddAction(*new Action(ID_WRITE_MANUAL, CATEGORY_ITEM,
                                       u"Ручной ввод...", u"Ручной ввод",
                                       IDB_WRITE_MANUAL));
  action_manager.AddAction(
      *new Action(ID_EDIT_LIMITS, CATEGORY_ITEM, u"Уставки...", u"Уставки"));

  action_manager.AddAction(
      *new Action(ID_DEV1_REFR, CATEGORY_DEVICE, u"Опросить устройство"));
  action_manager.AddAction(
      *new Action(ID_DEV1_SYNC, CATEGORY_DEVICE, u"Синхронизировать часы"));

  action_manager.AddAction(*new Action(ID_SETUP, CATEGORY_SETUP, u"Опции"));
  action_manager.AddAction(*new Action(ID_PRINT, CATEGORY_SETUP, u"Печать",
                                       std::u16string(), IDB_PRINTER));
  action_manager.AddAction(*new Action(ID_EDIT, CATEGORY_SETUP, u"Правка"))
      .set_checkable(true);

  action_manager.AddAction(
      *new Action(ID_EXPORT_CSV, CATEGORY_EXPORT, u"Экспорт в CSV"));
  action_manager.AddAction(
      *new Action(ID_EXPORT_EXCEL, CATEGORY_EXPORT, u"Экспорт в Excel"));

  action_manager.AddAction(
      *new Action(ID_OPEN_WATCH, CATEGORY_SPECIFIC, u"Наблюдение"));
  action_manager.AddAction(
      *new Action(ID_OPEN_DEVICE_METRICS, CATEGORY_SPECIFIC, u"Метрики"));
  action_manager.AddAction(*new Action(ID_CHANGE_PASSWORD, CATEGORY_SPECIFIC,
                                       u"Задать пароль...", u"Пароль"));
  action_manager.AddAction(
      *new Action(ID_ITEM_ENABLE, CATEGORY_SPECIFIC, u"Включить"));
  action_manager.AddAction(
      *new Action(ID_ITEM_DISABLE, CATEGORY_SPECIFIC, u"Отключить"));
  action_manager.AddAction(*new Action(ID_PAUSE, CATEGORY_SPECIFIC, u"Пауза"));

  action_manager.AddAction(*new Action(ID_ACKNOWLEDGE_ALL, CATEGORY_VIEW,
                                       u"Квитировать все", std::u16string(),
                                       IDB_ACKNOWLEDGE_ALL));
  action_manager.AddAction(*new Action(ID_SEVERITY_CUSTOM, CATEGORY_VIEW,
                                       u"Важность...", u"Важность"));
  action_manager.AddAction(*new Action(ID_FAVOURITES_ADD_URL, CATEGORY_EDIT,
                                       u"Добавить Web-страницу...",
                                       u"Добавить Web-страницу"));
  action_manager.AddAction(
      *new Action(ID_MODUS_TOOLBAR, CATEGORY_VIEW, u"Панель инструментов"));
  action_manager.AddAction(
      *new Action(ID_MODUS_STATUSBAR, CATEGORY_VIEW, u"Строка состояния"));
  action_manager.AddAction(*new Action(ID_EVENT_VIEW, CATEGORY_VIEW,
                                       u"Панель событий", std::u16string(),
                                       ID_EVENT_VIEW));
  action_manager.AddAction(*new Action(ID_SAVE, CATEGORY_VIEW, u"Сохранить"));
  action_manager.AddAction(*new Action(ID_SAVE_AS, CATEGORY_VIEW,
                                       u"Сохранить как...", u"Сохранить"));

  action_manager
      .AddAction(*new Action(ID_CURRENT_EVENTS, CATEGORY_PERIOD, u"Текущие"))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_TIME_RANGE_DAY, CATEGORY_PERIOD, u"День"))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_TIME_RANGE_WEEK, CATEGORY_PERIOD, u"Неделя"))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_TIME_RANGE_MONTH, CATEGORY_PERIOD, u"Месяц"))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_TIME_RANGE_CUSTOM, CATEGORY_PERIOD,
                             u"Другой...", u"Другой"))
      .set_checkable(true);

  action_manager
      .AddAction(*new Action(ID_INTERVAL_1M, CATEGORY_INTERVAL, u"Минутный"))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_INTERVAL_5M, CATEGORY_INTERVAL, u"5 минут"))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_INTERVAL_15M, CATEGORY_INTERVAL, u"15 минут"))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_INTERVAL_30M, CATEGORY_INTERVAL, u"30 минут"))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_INTERVAL_1H, CATEGORY_INTERVAL, u"Часовой"))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_INTERVAL_12H, CATEGORY_INTERVAL, u"12 часов"))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_INTERVAL_1D, CATEGORY_INTERVAL, u"Суточный"))
      .set_checkable(true);

  action_manager
      .AddAction(
          *new Action(ID_AGGREGATION_START, CATEGORY_AGGREGATION, u"Первое"))
      .set_checkable(true);
  action_manager
      .AddAction(
          *new Action(ID_AGGREGATION_END, CATEGORY_AGGREGATION, u"Последнее"))
      .set_checkable(true);
  action_manager
      .AddAction(*new Action(ID_AGGREGATION_COUNT, CATEGORY_AGGREGATION,
                             u"Количество"))
      .set_checkable(true);
  action_manager
      .AddAction(
          *new Action(ID_AGGREGATION_MIN, CATEGORY_AGGREGATION, u"Минимум"))
      .set_checkable(true);
  action_manager
      .AddAction(
          *new Action(ID_AGGREGATION_MAX, CATEGORY_AGGREGATION, u"Максимум"))
      .set_checkable(true);
  action_manager
      .AddAction(
          *new Action(ID_AGGREGATION_SUM, CATEGORY_AGGREGATION, u"Сумма"))
      .set_checkable(true);
  action_manager
      .AddAction(
          *new Action(ID_AGGREGATION_AVG, CATEGORY_AGGREGATION, u"Среднее"))
      .set_checkable(true);

  action_manager.AddAction(*new Action(ID_ITEM_PARAMS, CATEGORY_EDIT,
                                       u"Параметры", std::u16string(),
                                       IDB_RECORD_EDITOR));
  action_manager.AddAction(*new Action(ID_TABLE_CONFIG, CATEGORY_EDIT,
                                       u"Параметры элементов", u"Элементы"));
  action_manager.AddAction(*new Action(ID_TRANSMISSION_VIEW, CATEGORY_EDIT,
                                       u"Таблица ретрансляции",
                                       u"Ретрансляция"));
  action_manager.AddAction(
      *new Action(ID_NEW_PORTFOLIO, CATEGORY_EDIT, u"Создать портфолио"));
  action_manager.AddAction(*new Action(ID_ADD_ITEMS, CATEGORY_EDIT,
                                       u"Добавить объекты...",
                                       u"Добавить объекты"));
  action_manager.AddAction(
      *ActionBuilder(ID_RENAME, CATEGORY_EDIT, u"Переименовать")
           .shortcut(aui::KeyCode::F2)
           .Build());
  action_manager.AddAction(
      *ActionBuilder(ID_COPY, CATEGORY_EDIT, u"Копировать")
           .image_id(IDB_COPY)
           .shortcut({aui::ControlModifier, aui::KeyCode::C})
           .Build());
  action_manager.AddAction(
      *ActionBuilder(ID_PASTE, CATEGORY_EDIT, u"Вставить")
           .image_id(IDB_PASTE)
           .shortcut({aui::ControlModifier, aui::KeyCode::V})
           .Build());
  action_manager.AddAction(*ActionBuilder(ID_DELETE, CATEGORY_EDIT, u"Удалить")
                                .image_id(IDB_DELETE)
                                .shortcut(aui::KeyCode::Delete)
                                .Build());
  action_manager.AddAction(
      *new Action(ID_CLEAR_ALL, CATEGORY_EDIT, u"Очистить"));

  RegisterCreateActions(action_manager, node_service);

  RegisterFileSystemActions(action_manager);
}
