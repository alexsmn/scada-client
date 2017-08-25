#include "client/components/main/views/main_menu_model2.h"

#include "client/components/main/views/main_window_views.h"
#include "client/components/main/main_commands.h"
#include "base/strings/string16.h"
#include "client/base/color.h"
#include "client/client_application.h"
#include "client/client_utils.h"
#include "client/components/main/opened_view.h"
#include "client/command_handler.h"
#include "client/services/favourites.h"
#include "client/services/file_cache.h"
#include "client/components/main/view_manager.h"
#include "client/services/profile.h"
#include "client/window_definition.h"
#include "client/window_info.h"

namespace {

class DisplayMenu : public ui::MenuModel {
 public:
  explicit DisplayMenu(FileCache& file_cache) : file_cache_(file_cache) {}

  virtual bool HasIcons() const override { return false; }
  virtual ItemType GetTypeAt(int index) const override { return TYPE_COMMAND; }
  virtual ui::MenuSeparatorType GetSeparatorTypeAt(int index) const override { return ui::NORMAL_SEPARATOR; }
  virtual int GetCommandIdAt(int index) const override { return 0; }
  virtual bool IsItemDynamicAt(int index) const override { return false; }
  virtual bool GetAcceleratorAt(int index, ui::Accelerator* accelerator) const override { return false; }
  virtual bool IsItemCheckedAt(int index) const override { return false; }
  virtual int GetGroupIdAt(int index) const override { return 0; }
  virtual bool GetIconAt(int index, gfx::Image* icon) override { return false; }
  virtual ui::ButtonMenuItemModel* GetButtonMenuItemAt(int index) const override { return nullptr; }
  virtual bool IsEnabledAt(int index) const override { return true; }
  virtual ui::MenuModel* GetSubmenuModelAt(int index) const override { return nullptr; }
  virtual void HighlightChangedTo(int index) override {}
  virtual void SetMenuModelDelegate(ui::MenuModelDelegate* delegate) override {}
  virtual ui::MenuModelDelegate* GetMenuModelDelegate() const override { return nullptr; }

  virtual int GetItemCount() const override {
    return file_cache_.GetList(VIEW_TYPE_MODUS).size() +
           file_cache_.GetList(VIEW_TYPE_VIDICON_DISPLAY).size();
  }

  virtual base::string16 GetLabelAt(int index) const override {
    if (index < file_cache_.GetList(VIEW_TYPE_MODUS).size())
      return file_cache_.GetList(VIEW_TYPE_MODUS)[index].title;
    else
      return file_cache_.GetList(VIEW_TYPE_VIDICON_DISPLAY)[index].title;
  }

  virtual void ActivatedAt(int index) override {
  }

 private:
  FileCache& file_cache_;
};

} // namespace

MainMenuModel2::MainMenuModel2(MainMenuModel2Context&& context)
    : ui::SimpleMenuModel(this),
      MainMenuModel2Context(std::move(context)) {
  display_menu_.reset(new DisplayMenu(main_view_.file_cache_));

  AddSubMenu(0, L"Схема", display_menu_.get());
}

bool MainMenuModel2::IsCommandIdChecked(int command_id) const {
  auto* handler = main_view_.main_commands().GetCommandHandler(command_id);
  return handler && handler->IsCommandChecked(command_id);
}

bool MainMenuModel2::IsCommandIdEnabled(int command_id) const {
  auto* handler = main_view_.main_commands().GetCommandHandler(command_id);
  return handler && handler->IsCommandEnabled(command_id);
}

bool MainMenuModel2::GetAcceleratorForCommandId(int command_id, ui::Accelerator* accelerator) {
  return false;
}

void MainMenuModel2::ExecuteCommand(int command_id) {
  auto* handler = main_view_.main_commands_->GetCommandHandler(command_id);
  if (handler)
    handler->ExecuteCommand(command_id);
}

bool MainMenuModel2::GetMenuItems(UINT id, MenuItems& items) {
  MenuItem item;
  item.radio = false;
  item.checked = false;
  item.enabled = true;

  LPCTSTR empty = NULL;

  switch (id) {
    case ID_PAGE_0: {
      empty = _T("Нет листов");
      Profile::PageMap& pages = profile_.pages();
      for (Profile::PageMap::iterator i = pages.begin(); i != pages.end(); ++i) {
        Page& page = i->second;
        item.text = page.id == main_view_.current_page().id ?
            main_view_.current_page().GetTitle() : page.GetTitle();
        item.radio = true;
        item.checked = main_view_.current_page().id == page.id;
        items.push_back(item);
      }
      break;
    }
    
    case ID_WIN_0: {
      empty = _T("Нет окон");
      for (auto& p : main_view_.view_manager().views()) {
        item.text = p.view->GetWindowTitle();
        item.radio = true;
        item.checked = main_view_.active_view() == p.view;
        items.push_back(item);
      }
      break;
    }

    case ID_TRASH_0: {
      empty = _T("Корзина пуста");
      Page& trash = profile_.trash();
      for (int i = 0; i < trash.GetWindowCount(); ++i) {
        WindowDefinition& win = trash.GetWindow(i);
        item.text = win.GetTitle();
        item.text.insert(0, L"Восстановить ");
        items.push_back(item);
      }
      break;
    }

    case ID_NEW_DISPLAY_0: {
      empty = _T("Нет мнемосхем");
      GetFileCacheItems(VIEW_TYPE_MODUS, items);
      GetFileCacheItems(VIEW_TYPE_VIDICON_DISPLAY, items);
      break;
    }

    case ID_NEW_REPORT_0: {
      empty = _T("Нет отчетов");
      const auto& cache = file_cache_.GetList(VIEW_TYPE_EXCEL_REPORT);
      for (size_t i = 0; i < cache.size(); ++i) {
        item.text = cache[i].title;
        items.push_back(item);
      }
      break;
    }
      
    case ID_FAV_TABLE_0: {
      empty = _T("Избранное");
      unsigned types[] = { ID_TABLE_VIEW, ID_SHEET_VIEW };
      GetFavouritesMenuItems(items, _countof(types), types);
      break;
    }
      
    case ID_FAV_GRAPH_0: {
      empty = _T("Избранное");
      unsigned types[] = { ID_GRAPH_VIEW };
      GetFavouritesMenuItems(items, _countof(types), types);
      break;
    }
      
    default:
      return false;
  }

  if (items.empty()) {
    assert(empty);
    item.text = empty;
    item.radio = false;
    item.checked = false;
    item.enabled = false;
    items.push_back(item);
  }
  return true;
}

void MainMenuModel2::GetFavouritesMenuItems(MenuItems& items, int type_count,
                                            const unsigned* types) {
  if (const Page* favourites_folder = favourites_.GetFolder()) {
    for (int i = 0; i < favourites_folder->GetWindowCount(); ++i) {
      const WindowDefinition& win = favourites_folder->GetWindow(i);
      if (!win.window_info())
        continue;
      if (std::find(types, types + type_count, win.window_info()->command_id) == types + type_count)
        continue;
        
      MenuItem item;
      item.text = win.GetTitle();
      items.push_back(item);
    }
  }
}

LRESULT MainMenuModel2::OnNewDisplay(WORD /*wNotifyCode*/, WORD wID,
                                 HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  assert(wID >= ID_NEW_DISPLAY_0);
  
  int index = wID - ID_NEW_DISPLAY_0;

  unsigned window_type = ID_MODUS_VIEW;
  const auto* cache = &file_cache_.GetList(VIEW_TYPE_MODUS);
  if (index >= static_cast<int>(cache->size())) {
      index -= cache->size();
      window_type = ID_VIDICON_DISPLAY_VIEW;
      cache = &file_cache_.GetList(VIEW_TYPE_VIDICON_DISPLAY);
  }

  const base::FilePath& path = (*cache)[index].path;

  // find existing display
  auto view = main_view_.find_opened_view_(path);
  if (view) {
    view->Activate();
  } else {
    // add new window
    WindowDefinition def(GetWindowInfo(window_type));
    def.path = path;
    main_view_.OpenView(def, true);
  }

  return 0;
}

LRESULT MainMenuModel2::OnNewReport(WORD /*wNotifyCode*/, WORD wID,
                                 HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  assert(wID >= ID_NEW_REPORT_0);
  
  const auto& cache = file_cache_.GetList(VIEW_TYPE_EXCEL_REPORT);
  int index = wID - ID_NEW_REPORT_0;
  const base::FilePath& path = cache[index].path;

  // TODO: find existing display

  // Add new window.
  WindowDefinition def(GetWindowInfo(ID_EXCEL_REPORT_VIEW));
  def.path = path;
  main_view_.OpenView(def, true);

  return 0;
}

LRESULT MainMenuModel2::OnPage(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  int ix = wID - ID_PAGE_0;
  assert(ix >= 0 && ix <= 100);

  Profile::PageMap& pages = profile_.pages();
  Page* page = NULL;
  for (Profile::PageMap::iterator i = pages.begin(); i != pages.end(); i++) {
    if (!ix--) {
      page = &i->second;
      break;
    }
  }

  DCHECK(page);

  // check revert page
  bool revert = page->id == main_view_.current_page().id;
  if (revert) {
    base::string16 title = main_view_.current_page().GetTitle();
    base::string16 message = base::StringPrintf(
        L"Вернуться к сохраненному листу %ls?", title.c_str());
    if (ShowMessageBox(main_view_, message.c_str(),
                      (LPCTSTR)NULL, MB_YESNO | MB_ICONQUESTION) == IDNO) {
      return 0;
    }
  }

  // Don't allow to open same page in different windows.
  if (!revert && main_view_.page_opened_(page->id)) {
    ShowMessageBox(main_view_,
        L"Указанный лист открыт в другом окне.",
        (LPCTSTR)NULL, MB_ICONEXCLAMATION | MB_OK);
    return 0;
  }

  if (!revert)
    main_view_.SavePage();

  main_view_.OpenPage(*page);

  return 0;
}

LRESULT MainMenuModel2::OnWin(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
  int ix = wID - ID_WIN_0;
  assert(ix >= 0 && ix <= 100);

  OpenedView* view = NULL;
  for (auto& p : main_view_.view_manager().views()) {
    if (!ix--) {
      view = p.view;
      break;
    }
  }
  
  assert(view);
  main_view_.ActivateView(*view);

  return 0;
}

LRESULT MainMenuModel2::OnTrash(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  int n = (int)wID - (int)ID_TRASH_0;
  assert(n >= 0 && n < 100);

  Page& trash = profile_.trash();

  int i = 0;
  while (n-- && i < trash.GetWindowCount())
    i++;
  if (i < trash.GetWindowCount()) {
    main_view_.OpenView(trash.GetWindow(i), true);
    trash.DeleteWindow(i);
  }

  return 0;
}

LRESULT MainMenuModel2::OnFavTable(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                               BOOL& bHandled) {
  int n = wID - ID_FAV_TABLE_0;
  assert(n >= 0 && n < 100);

  const WindowDefinition* _win = NULL;
  if (const Page* favourites_folder = favourites_.GetFolder()) {
    for (int i = 0; i < favourites_folder->GetWindowCount(); ++i) {
        const WindowDefinition& win = favourites_folder->GetWindow(i);
        if (!win.window_info())
          continue;
        if (win.window_info()->command_id != ID_TABLE_VIEW &&
            win.window_info()->command_id != ID_SHEET_VIEW)
          continue;
        if (!n--) {
          _win = &win;
          break;
        }
    }
  }

  if (_win)
    main_view_.OpenView(*_win, true);

  return 0;
}

LRESULT MainMenuModel2::OnFavGraph(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  int n = wID - ID_FAV_GRAPH_0;
  assert(n >= 0 && n < 100);

  const WindowDefinition* _win = NULL;
  if (const Page* favourites_folder = favourites_.GetFolder()) {
    for (int i = 0; i < favourites_folder->GetWindowCount(); ++i) {
        const WindowDefinition& win = favourites_folder->GetWindow(i);
        if (!win.window_info())
          continue;
        if (win.window_info()->command_id != ID_GRAPH_VIEW)
          continue;
        if (!n--) {
          _win = &win;
          break;
        }
    }
  }

  if (_win)
    main_view_.OpenView(*_win, true);

  return 0;
}

void MainMenuModel2::GetFileCacheItems(int type_id, MenuItems& items) {
  const auto& cache = file_cache_.GetList(type_id);
  for (size_t i = 0; i < cache.size(); ++i) {
    MenuItem item;
    item.text = cache[i].title;
    items.push_back(item);
  }
}
