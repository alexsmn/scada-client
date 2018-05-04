#include "components/main/views/main_menu_model.h"

#include "command_handler.h"
#include "common_resources.h"
#include "components/main/main_window.h"
#include "components/main/main_window_manager.h"
#include "components/main/opened_view.h"
#include "components/main/view_manager.h"
#include "components/main/views/context_menu_model.h"
#include "services/favourites.h"
#include "services/file_cache.h"
#include "services/profile.h"
#include "window_definition.h"
#include "window_info.h"

#include <atlres.h>

const unsigned kTableTypes[] = {ID_TABLE_VIEW, ID_SHEET_VIEW,
                                ID_TIMED_DATA_VIEW, 0};
const unsigned kGraphTypes[] = {ID_GRAPH_VIEW, 0};

// InplaceMenuModel

InplaceMenuModel::InplaceMenuModel(ui::SimpleMenuModel::Delegate* delegate)
    : ui::SimpleMenuModel{delegate} {}

void InplaceMenuModel::AddInplaceMenu(std::shared_ptr<ui::MenuModel> model) {
  int index = GetItemCount();
  inplace_models_.emplace_back(InplaceInfo{index, std::move(model)});
}

void InplaceMenuModel::MenuWillShow() {
  for (auto& inplace_model : inplace_models_)
    DeleteItems(inplace_model.index, inplace_model.count);

  for (auto& inplace_model : inplace_models_)
    inplace_model.model->MenuWillShow();

  int offset = 0;
  for (auto& inplace_model : inplace_models_) {
    auto& model = *inplace_model.model;
    int count = model.GetItemCount();
    for (int i = 0; i < count; ++i) {
      InsertItemAt(offset + inplace_model.index + i, model.GetCommandIdAt(i),
                   model.GetLabelAt(i));
    }
    inplace_model.offset = offset;
    inplace_model.count = count;
    offset += count;
  }
}

std::pair<ui::MenuModel*, int> InplaceMenuModel::GetModelAndIndexAt(int index) {
  for (auto& inplace_model : inplace_models_) {
    if (index < inplace_model.index)
      return {this, index};
    if (inplace_model.index <= index &&
        index < inplace_model.index + inplace_model.count) {
      auto& model = *inplace_model.model;
      int model_index = index - inplace_model.index;
      return {&model, model_index};
    }
    index -= inplace_model.count;
  }
  return {this, index};
}

std::pair<const ui::MenuModel*, int> InplaceMenuModel::GetModelAndIndexAt(
    int index) const {
  return const_cast<InplaceMenuModel*>(this)->GetModelAndIndexAt(index);
}

bool InplaceMenuModel::IsEnabledAt(int index) const {
  auto [model, model_index] = GetModelAndIndexAt(index);
  if (model == this)
    return ui::SimpleMenuModel::IsEnabledAt(model_index);
  else
    return model && model->IsEnabledAt(model_index);
}

void InplaceMenuModel::ActivatedAt(int index) {
  auto [model, model_index] = GetModelAndIndexAt(index);
  if (model == this)
    return ui::SimpleMenuModel::ActivatedAt(model_index);
  else if (model)
    model->ActivatedAt(model_index);
}

// DisplayMenuModel

DisplayMenuModel::DisplayMenuModel(const MainMenuContext& context)
    : ui::SimpleMenuModel{nullptr}, MainMenuContext{std::move(context)} {}

void DisplayMenuModel::MenuWillShow() {
  Clear();
  paths_.clear();

  AddItems(VIEW_TYPE_MODUS);
  AddItems(VIEW_TYPE_VIDICON_DISPLAY);
}

void DisplayMenuModel::ActivatedAt(int index) {
  auto& path = paths_[index];
  // find existing display
  if (auto* view = main_window_manager_.FindOpenedViewByFilePath(path)) {
    view->Activate();
  } else {
    // add new window
    WindowDefinition def(GetWindowInfo(ID_MODUS_VIEW));
    def.path = path;
    main_window_.OpenView(def, true);
  }
}

bool DisplayMenuModel::IsEnabledAt(int index) const {
  return paths_.empty();
}

void DisplayMenuModel::AddItems(unsigned type) {
  for (auto& entry : file_cache_.GetList(type)) {
    AddItem(0, entry.title);
    paths_.emplace_back(entry.path);
  }

  if (paths_.empty())
    AddItem(0, L"<Нет схем>");
}

// FavouritesMenuModel

class FavouritesMenuModel : private MainMenuContext,
                            public ui::SimpleMenuModel {
 public:
  FavouritesMenuModel(ui::SimpleMenuModel::Delegate* delegate,
                      const unsigned view_types[],
                      const MainMenuContext& context);

  // views::MenuModel
  virtual void MenuWillShow() override;
  virtual void ActivatedAt(int index) override;
  virtual bool IsEnabledAt(int index) const override;

 private:
  const unsigned* view_types_;

  std::vector<const WindowDefinition*> windows_;
};

FavouritesMenuModel::FavouritesMenuModel(
    ui::SimpleMenuModel::Delegate* delegate,
    const unsigned view_types[],
    const MainMenuContext& context)
    : MainMenuContext{std::move(context)},
      ui::SimpleMenuModel{delegate},
      view_types_{view_types} {}

void FavouritesMenuModel::MenuWillShow() {
  Clear();
  windows_.clear();

  if (const Page* favourites_folder = favourites_.GetFolder()) {
    for (int i = 0; i != favourites_folder->GetWindowCount(); ++i) {
      const WindowDefinition& win = favourites_folder->GetWindow(i);

      bool found = false;
      for (int i = 0; view_types_[i]; ++i) {
        if (win.window_info().command_id == view_types_[i]) {
          found = true;
          break;
        }
      }

      if (found) {
        AddItem(0, win.GetTitle());
        windows_.push_back(&win);
      }
    }
  }

  if (windows_.empty())
    AddItem(0, L"<Нет избранного>");
}

void FavouritesMenuModel::ActivatedAt(int index) {
  main_window_.OpenView(*windows_[index], true);
}

bool FavouritesMenuModel::IsEnabledAt(int index) const {
  return !windows_.empty();
}

// PageMenuModel

class PageMenuModel : private MainMenuContext, public ui::SimpleMenuModel {
 public:
  explicit PageMenuModel(const MainMenuContext& context);

  // views::MenuModel
  virtual void MenuWillShow() override;
  virtual void ActivatedAt(int index) override;
};

PageMenuModel::PageMenuModel(const MainMenuContext& context)
    : MainMenuContext{context}, ui::SimpleMenuModel{nullptr} {}

void PageMenuModel::MenuWillShow() {
  Clear();

  for (auto& p : profile_.pages)
    AddItem(0, p.second.title);
}

void PageMenuModel::ActivatedAt(int index) {
  auto p = profile_.pages.begin();
  std::advance(p, index);
  // p->second
}

// WindowMenuModel

class WindowMenuModel : private MainMenuContext, public ui::SimpleMenuModel {
 public:
  explicit WindowMenuModel(const MainMenuContext& context)
      : MainMenuContext{context}, ui::SimpleMenuModel{nullptr} {}

  // views::MenuModel
  virtual void MenuWillShow() override;
  virtual void ActivatedAt(int index) override;
};

void WindowMenuModel::MenuWillShow() {
  Clear();

  for (auto* opened_view : view_manager_.views())
    AddItem(0, opened_view->GetWindowTitle());
}

void WindowMenuModel::ActivatedAt(int index) {}

// TrashMenuModel

class TrashMenuModel : private MainMenuContext, public ui::SimpleMenuModel {
 public:
  explicit TrashMenuModel(const MainMenuContext& context)
      : MainMenuContext{context}, ui::SimpleMenuModel{nullptr} {}

  // views::MenuModel
  virtual void MenuWillShow() override;
  virtual void ActivatedAt(int index) override;
};

void TrashMenuModel::MenuWillShow() {
  if (GetItemCount() == 0)
    AddItem(0, L"<Корзина пуста>");
}

void TrashMenuModel::ActivatedAt(int index) {}

// MainMenuModel

MainMenuModel::MainMenuModel(const MainMenuContext& context)
    : MainMenuContext{std::move(context)},
      ui::SimpleMenuModel{this},
      display_menu_model_{context},
      table_submenu_{this},
      graph_submenu_{this},
      more_submenu_{this},
      page_submenu_{this},
      window_submenu_{this},
      settings_submenu_{this},
      help_submenu_{this} {
  Rebuild();
}

void MainMenuModel::Rebuild() {
  const MainMenuContext& context = *this;

  AddSubMenu(0, L"Схемы", &display_menu_model_);

  table_submenu_.AddItem(ID_TABLE_VIEW, L"Новая таблица");
  table_submenu_.AddItem(ID_SHEET_VIEW, L"Новая п-таблица");
  table_submenu_.AddItem(ID_TIMED_DATA_VIEW, L"Новая таблица значений");
  table_submenu_.AddSeparator(ui::NORMAL_SEPARATOR);
  table_submenu_.AddInplaceMenu(
      std::make_shared<FavouritesMenuModel>(delegate(), kTableTypes, context));
  AddSubMenu(0, L"Таблица", &table_submenu_);

  graph_submenu_.AddItem(ID_GRAPH_VIEW, L"Новый");
  graph_submenu_.AddSeparator(ui::NORMAL_SEPARATOR);
  graph_submenu_.AddInplaceMenu(
      std::make_shared<FavouritesMenuModel>(delegate(), kGraphTypes, context));
  AddSubMenu(0, L"График", &graph_submenu_);

  context_menu_ = std::make_unique<ContextMenuModel>(
      main_window_, action_manager_, command_handler_);
  AddSubMenu(0, L"Объект", context_menu_.get());

  more_submenu_.AddItem(ID_OBJECT_VIEW, L"Объекты");
  more_submenu_.AddItem(ID_EVENT_VIEW, L"События");
  more_submenu_.AddItem(ID_FAVOURITES_VIEW, L"Избранное");
  more_submenu_.AddItem(ID_PORTFOLIO_VIEW, L"Портфолио");
  more_submenu_.AddItem(ID_HARDWARE_VIEW, L"Оборудование");
  more_submenu_.AddItem(ID_EVENT_JOURNAL_VIEW, L"Журнал событий");
  if (admin_) {
    more_submenu_.AddSeparator(ui::NORMAL_SEPARATOR);
    more_submenu_.AddItem(ID_NODES_VIEW, L"Узлы");
    more_submenu_.AddItem(ID_TS_FORMATS_VIEW, L"Форматы");
    more_submenu_.AddItem(ID_SIMULATION_ITEMS_VIEW, L"Эмулируемые сигналы");
    more_submenu_.AddItem(ID_USERS_VIEW, L"Пользователи");
    more_submenu_.AddItem(ID_HISTORICAL_DB_VIEW, L"Базы данных");
    more_submenu_.AddSeparator(ui::NORMAL_SEPARATOR);
    more_submenu_.AddItem(ID_EXPORT_CONFIGURATION_TO_EXCEL,
                          L"Экспорт конфигурации в Excel...");
    more_submenu_.AddItem(ID_IMPORT_CONFIGURATION_FROM_EXCEL,
                          L"Импорт конфигурации из Excel...");
  }
  AddSubMenu(0, L"Далее", &more_submenu_);

  page_submenu_.AddItem(ID_PAGE_NEW, L"Новый");
  page_submenu_.AddItem(ID_PAGE_DELETE, L"Удалить");
  page_submenu_.AddItem(ID_PAGE_RENAME, L"Переименовать");
  page_submenu_.AddSeparator(ui::NORMAL_SEPARATOR);
  page_submenu_.AddInplaceMenu(std::make_shared<PageMenuModel>(context));
  AddSubMenu(0, L"Лист", &page_submenu_);

  window_submenu_.AddItem(ID_WINDOW_NEW, L"Новое");
  window_submenu_.AddSeparator(ui::NORMAL_SEPARATOR);
  window_submenu_.AddItem(ID_VIEW_CHANGE_TITLE, L"Переименовать");
  window_submenu_.AddItem(ID_VIEW_ADD_TO_FAVOURITES, L"В избранное");
  window_submenu_.AddItem(ID_VIEW_CLOSE, L"Закрыть");
  window_submenu_.AddSeparator(ui::NORMAL_SEPARATOR);
  window_submenu_.AddInplaceMenu(std::make_shared<WindowMenuModel>(context));
  window_submenu_.AddSeparator(ui::NORMAL_SEPARATOR);
  window_submenu_.AddInplaceMenu(std::make_shared<TrashMenuModel>(context));
  AddSubMenu(0, L"Окно", &window_submenu_);

  // TODO:
  settings_submenu_.AddItem(0, L"Панель инструментов");
  settings_submenu_.AddItem(ID_VIEW_STATUS_BAR, L"Строка состояния");
  settings_submenu_.AddSeparator(ui::NORMAL_SEPARATOR);
  settings_submenu_.AddItem(ID_WRITE_CONFIRMATION, L"Подтверждение управления");
  settings_submenu_.AddItem(ID_SHOW_WRITEOK,
                            L"Сообщение об успешном управлении");
  settings_submenu_.AddSeparator(ui::NORMAL_SEPARATOR);
  settings_submenu_.AddItem(ID_SHOW_EVENTS,
                            L"Показывать события при появлении");
  settings_submenu_.AddItem(ID_HIDE_EVENTS,
                            L"Скрывать события при квитировании");
  settings_submenu_.AddItem(ID_EVENT_FLASH_WINDOW,
                            L"Мигание основного окна по событию");
  settings_submenu_.AddItem(ID_EVENT_PLAY_SOUND,
                            L"Звуковая сигнализация по событию");
  settings_submenu_.AddSeparator(ui::NORMAL_SEPARATOR);
  settings_submenu_.AddItem(ID_VIEW_PUBLIC_FOLDER, L"Открыть папку схем");
  settings_submenu_.AddItem(ID_MODUS2_MODE,
                            L"Встроенный визуализатор схем MODUS");
  AddSubMenu(0, L"Настройки", &settings_submenu_);

  help_submenu_.AddItem(ID_HELP_MANUAL, L"Документация");
  help_submenu_.AddSeparator(ui::NORMAL_SEPARATOR);
  help_submenu_.AddItem(ID_APP_ABOUT, L"О программе...");
  AddSubMenu(0, L"Справка", &help_submenu_);
}

bool MainMenuModel::IsCommandIdChecked(int command_id) const {
  return false;
}

bool MainMenuModel::IsCommandIdEnabled(int command_id) const {
  return true;
}

bool MainMenuModel::GetAcceleratorForCommandId(int command_id,
                                               ui::Accelerator* accelerator) {
  return false;
}

void MainMenuModel::ExecuteCommand(int command_id) {
  if (auto* handler = command_handler_.GetCommandHandler(command_id))
    handler->ExecuteCommand(command_id);
}
