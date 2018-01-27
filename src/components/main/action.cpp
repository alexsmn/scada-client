#include "components/main/action.h"

#include "base/at_exit.h"
#include "base/strings/sys_string_conversions.h"
#include "common/scada_node_ids.h"
#include "common_resources.h"
#include "common/node_ref_service.h"
#include "translation.h"

GroupedActions GroupCommands(ActionManager& action_manager, const std::vector<unsigned>& commands) {
  GroupedActions grouped_commands;
  for (std::vector<unsigned>::const_iterator i = commands.begin();
       i != commands.end(); ++i) {
    Action* action = action_manager.FindAction(*i);
    if (!action)
      continue;
    grouped_commands[action->category_].push_back(action);
  }
  return grouped_commands;
}

const base::char16* GetCommandCategoryTitle(CommandCategory category) {
  const base::char16* titles[] = { L"Новый",      // CATEGORY_NEW
                             L"Открыть",    // CATEGORY_OPEN
                             L"Объект",     // CATEGORY_ITEM
                             L"Устройство", // CATEGORY_DEVICE
                             L"Опции",      // CATEGORY_SETUP
                             L"Разное",     // CATEGORY_SPECIFIC
                             L"Окно",       // CATEGORY_VIEW
                             L"Период",     // CATEGORY_PERIOD
                             L"Создать",    // CATEGORY_CREATE
                             L"Правка" };   // CATEGORY_EDIT,
  assert(category >= 0 && category < _countof(titles));
  return titles[category];
}

bool CanExpandCommandCategory(CommandCategory category) {
  return category != CATEGORY_CREATE &&
         category != CATEGORY_DEVICE &&
         category != CATEGORY_PERIOD &&
         category != CATEGORY_NEW;
}

namespace {

const scada::NodeId kNewCommandTypeIds[] = {
    id::DataGroupType,
    id::DiscreteItemType,
    id::AnalogItemType,
    id::UserType,
    id::HistoricalDatabaseType,
    id::SimulationSignalType,
    id::Iec60870DeviceType,
    id::ModbusLinkType,
    id::ModbusDeviceType,
    id::TsFormatType,
    id::TransmissionItemType
};

} // namespace

scada::NodeId GetNewCommandTypeId(unsigned command_id) {
  if (command_id < ID_NEW)
    return scada::NodeId();

  auto index = command_id - ID_NEW;
  if (index >= _countof(kNewCommandTypeIds))
    return scada::NodeId();

  return kNewCommandTypeIds[index];
}

// Action

Action::Action(unsigned command, CommandCategory category, base::string16 title,
               base::string16 short_title, int image_id, unsigned flags)
    : command_id_(command),
      category_(category),
      title_(std::move(title)),
      short_title_(std::move(short_title)),
      image_id_(image_id),
      flags_(flags) {
}

class NodeAction : public Action {
 public:
  NodeAction(unsigned command_id, CommandCategory category, const NodeRef& node)
      : Action(command_id, category, {}),
        node_(std::move(node)) {
  }

  virtual base::string16 GetTitle() const override {
    return ToString16(node_.display_name());
  }

 private:
  const NodeRef node_;
};

// ActionManager

ActionManager::ActionManager(NodeRefService& node_service) {
//  AddAction(*new Action(ID_GRAPH_VIEW, CATEGORY_NEW, "Новый график", NULL, kGraphImageIndex));
//  AddAction(*new Action(ID_TABLE_VIEW, CATEGORY_NEW, "Новая таблица", NULL, kTableImageIndex));
  
  AddAction(*new Action(ID_OPEN_GRAPH, CATEGORY_OPEN, L"График", base::string16(), ID_GRAPH_VIEW, Action::ALWAYS_VISIBLE));
  AddAction(*new Action(ID_TIMED_DATA_VIEW, CATEGORY_OPEN, L"Данные", base::string16(), IDB_TIMED_DATA, Action::ALWAYS_VISIBLE));
  AddAction(*new Action(ID_OPEN_DISPLAY, CATEGORY_OPEN, L"Схема", base::string16(), ID_MODUS_VIEW, Action::ALWAYS_VISIBLE));
  AddAction(*new Action(ID_OPEN_TABLE, CATEGORY_OPEN, L"Таблица", base::string16(), ID_TABLE_VIEW, Action::ALWAYS_VISIBLE));
  AddAction(*new Action(ID_OPEN_SUMMARY, CATEGORY_OPEN, L"Сводка", base::string16(), IDB_SUMMARY, Action::ALWAYS_VISIBLE));
  AddAction(*new Action(ID_HISTORICAL_EVENTS, CATEGORY_OPEN, L"События", base::string16(), IDB_OPEN_EVENTS, Action::ALWAYS_VISIBLE));

  AddAction(*new Action(ID_OPEN_GROUP_TABLE, CATEGORY_OPEN, L"Таблица группы", base::string16(), 0, Action::VISIBLE));

  AddAction(*new Action(ID_ACKNOWLEDGE_CURRENT, CATEGORY_ITEM, L"Квитировать"));
  AddAction(*new Action(ID_UNLOCK_ITEM, CATEGORY_ITEM, L"Снять блокировку", base::string16(), IDB_UNLOCK));
  AddAction(*new Action(ID_WRITE, CATEGORY_ITEM, L"Управление...", L"Управление", IDB_WRITE));
  AddAction(*new Action(ID_WRITE_MANUAL, CATEGORY_ITEM, L"Ручной ввод...", L"Ручной ввод", IDB_WRITE_MANUAL));
  AddAction(*new Action(ID_EDIT_LIMITS, CATEGORY_ITEM, L"Уставки...", L"Уставки"));
  
  AddAction(*new Action(ID_DEV1_REFR, CATEGORY_DEVICE, L"Опросить устройство"));
  AddAction(*new Action(ID_DEV1_SYNC, CATEGORY_DEVICE, L"Синхронизировать часы"));

  AddAction(*new Action(ID_SETUP, CATEGORY_SETUP, L"Опции"));
  AddAction(*new Action(ID_PRINT, CATEGORY_SETUP, L"Печать", base::string16(), IDB_PRINTER));
  AddAction(*new Action(ID_EDIT, CATEGORY_SETUP, L"Правка"));
  AddAction(*new Action(ID_EXPORT, CATEGORY_SETUP, L"Экспорт"));
 
  AddAction(*new Action(ID_OPEN_WATCH, CATEGORY_SPECIFIC, L"Наблюдение"));
  AddAction(*new Action(ID_OPEN_DEVICE_METRICS, CATEGORY_SPECIFIC, L"Метрики"));
  AddAction(*new Action(ID_CHANGE_PASSWORD, CATEGORY_SPECIFIC, L"Задать пароль...", L"Пароль"));
  AddAction(*new Action(ID_ITEM_ENABLE, CATEGORY_SPECIFIC, L"Включить"));
  AddAction(*new Action(ID_ITEM_DISABLE, CATEGORY_SPECIFIC, L"Отключить"));
  AddAction(*new Action(ID_PAUSE, CATEGORY_SPECIFIC, L"Пауза"));

  AddAction(*new Action(ID_ACKNOWLEDGE_ALL, CATEGORY_VIEW, L"Квитировать все", base::string16(), IDB_ACKNOWLEDGE_ALL));
  AddAction(*new Action(ID_SEVERITY_CUSTOM, CATEGORY_VIEW, L"Важность...", L"Важность"));
  AddAction(*new Action(ID_MODUS_TOOLBAR, CATEGORY_VIEW, L"Панель инструментов"));
  AddAction(*new Action(ID_MODUS_STATUSBAR, CATEGORY_VIEW, L"Строка состояния"));
  AddAction(*new Action(ID_EVENT_VIEW, CATEGORY_VIEW, L"Панель событий", base::string16(), ID_EVENT_VIEW));
  AddAction(*new Action(ID_SAVE, CATEGORY_VIEW, L"Сохранить"));
  AddAction(*new Action(ID_SAVE_AS, CATEGORY_VIEW, L"Сохранить как...", L"Сохранить"));
  
  AddAction(*new Action(ID_CURRENT_EVENTS, CATEGORY_PERIOD, L"Текущие"));
  AddAction(*new Action(ID_TIME_RANGE_DAY, CATEGORY_PERIOD, L"День"));
  AddAction(*new Action(ID_TIME_RANGE_WEEK, CATEGORY_PERIOD, L"Неделя"));
  AddAction(*new Action(ID_TIME_RANGE_MONTH, CATEGORY_PERIOD, L"Месяц"));
  AddAction(*new Action(ID_TIME_RANGE_CUSTOM, CATEGORY_PERIOD, L"Другой...", L"Другой"));
  
  AddAction(*new Action(ID_ITEM_PARAMS, CATEGORY_EDIT, L"Параметры", base::string16(), IDB_RECORD_EDITOR));
  AddAction(*new Action(ID_TABLE_CONFIG, CATEGORY_EDIT, L"Параметры элементов", L"Элементы"));
  AddAction(*new Action(ID_TRANSMISSION_VIEW, CATEGORY_EDIT, L"Таблица ретрансляции", L"Ретрансляция"));
  AddAction(*new Action(ID_NEW_PORTFOLIO, CATEGORY_EDIT, L"Создать портфолио"));
  AddAction(*new Action(ID_ADD_ITEMS, CATEGORY_EDIT, L"Добавить объекты...", L"Добавить объекты"));
  AddAction(*new Action(ID_RENAME, CATEGORY_EDIT, L"Переименовать"));
  AddAction(*new Action(ID_DELETE, CATEGORY_EDIT, L"Удалить", base::string16(), IDB_DELETE));
  AddAction(*new Action(ID_CLEAR_ALL, CATEGORY_EDIT, L"Очистить"));
  
  AddAction(*new Action(ID_ADD_MULTIPLE_ITEMS, CATEGORY_CREATE, L"Серия объектов..."));
  AddAction(*new Action(ID_NEW_SERVICE_ITEMS, CATEGORY_CREATE, L"Сервисные объекты..."));
  AddAction(*new Action(ID_NEW_IEC60870_LINK101, CATEGORY_CREATE, L"Направление МЭК-60870-101"));
  AddAction(*new Action(ID_NEW_IEC60870_LINK104, CATEGORY_CREATE, L"Направление МЭК-60870-104"));
  for (size_t i = 0; i < _countof(kNewCommandTypeIds); ++i)
    AddAction(*new NodeAction(ID_NEW + i, CATEGORY_CREATE, node_service.GetNode(kNewCommandTypeIds[i])));
}

ActionManager::~ActionManager()
{
  for (ActionMap::iterator i = action_map_.begin(); i != action_map_.end(); ++i)
    delete i->second;
}

void ActionManager::AddAction(Action& action)
{
  assert(!FindAction(action.command_id()));
  action_map_[action.command_id()] = &action;
  actions_.push_back(&action);
}

Action* ActionManager::FindAction(unsigned command) const
{
  ActionMap::const_iterator i = action_map_.find(command);
  return i != action_map_.end() ? i->second : NULL;
}
