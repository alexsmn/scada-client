#pragma once

#include "base/memory/weak_ptr.h"
#include "aui/models/menu_model.h"

#include <vector>

namespace aui {

// A simple MenuModel implementation with an imperative API for adding menu
// items. This makes it easy to construct fixed menus. Menus populated by
// dynamic data sources may be better off implementing MenuModel directly.
// The breadth of MenuModel is not exposed through this API.
class SimpleMenuModel : public MenuModel {
 public:
  class Delegate {
   public:
    // Methods for determining the state of specific command ids.
    virtual bool IsCommandIdChecked(int command_id) const = 0;
    virtual bool IsCommandIdEnabled(int command_id) const = 0;
    virtual bool IsCommandIdVisible(int command_id) const;

    // Some command ids have labels and icons that change over time.
    virtual bool IsItemForCommandIdDynamic(int command_id) const;
    virtual std::u16string GetLabelForCommandId(int command_id) const;

    // Notifies the delegate that the item with the specified command id was
    // visually highlighted within the menu.
    virtual void CommandIdHighlighted(int command_id);

    // Performs the action associated with the specified command id.
    virtual void ExecuteCommand(int command_id) = 0;
    // Performs the action associates with the specified command id
    // with |event_flags|.
    virtual void ExecuteCommand(int command_id, int event_flags);

    // Notifies the delegate that the menu is about to show.
    virtual void MenuWillShow(SimpleMenuModel* source);

    // Notifies the delegate that the menu has closed.
    virtual void MenuClosed(SimpleMenuModel* source);

   protected:
    virtual ~Delegate() {}
  };

  // The Delegate can be NULL, though if it is items can't be checked or
  // disabled.
  explicit SimpleMenuModel(Delegate* delegate);
  virtual ~SimpleMenuModel();

  void set_delegate(Delegate* delegate) { delegate_ = delegate; }
  Delegate* delegate() { return delegate_; }

  // Methods for adding items to the model.
  void AddItem(int command_id, const std::u16string& label);
  void AddSeparator(MenuSeparatorType separator_type);
  void AddCheckItem(int command_id, const std::u16string& label);
  void AddRadioItem(int command_id, const std::u16string& label, int group_id);

  // Adds a separator if the menu is empty, or the last item is not a separator.
  void AddSeparatorIfNecessary(MenuSeparatorType separator_type);

  // These three methods take pointers to various sub-models. These models
  // should be owned by the same owner of this SimpleMenuModel.
  void AddSubMenu(int command_id,
                  const std::u16string& label,
                  MenuModel* model);

  void AddInplaceMenu(MenuModel* model);

  // Methods for inserting items into the model.
  void InsertItemAt(int index, int command_id, const std::u16string& label);
  void InsertSeparatorAt(int index, MenuSeparatorType separator_type);
  void InsertCheckItemAt(int index,
                         int command_id,
                         const std::u16string& label);
  void InsertRadioItemAt(int index,
                         int command_id,
                         const std::u16string& label,
                         int group_id);
  void InsertSubMenuAt(int index,
                       int command_id,
                       const std::u16string& label,
                       MenuModel* model);

  void DeleteItems(int index, int count);

  // Clears all items. Note that it does not free MenuModel of submenu.
  void Clear();

  // Returns the index of the item that has the given |command_id|. Returns
  // -1 if not found.
  int GetIndexOfCommandId(int command_id);

  // Overridden from MenuModel:
  virtual int GetItemCount() const override;
  virtual ItemType GetTypeAt(int index) const override;
  virtual MenuSeparatorType GetSeparatorTypeAt(int index) const override;
  virtual int GetCommandIdAt(int index) const override;
  virtual std::u16string GetLabelAt(int index) const override;
  virtual bool IsItemDynamicAt(int index) const override;
  virtual bool IsItemCheckedAt(int index) const override;
  virtual int GetGroupIdAt(int index) const override;
  virtual bool IsEnabledAt(int index) const override;
  virtual bool IsVisibleAt(int index) const override;
  virtual void HighlightChangedTo(int index) override;
  virtual void ActivatedAt(int index) override;
  virtual void ActivatedAt(int index, int event_flags) override;
  virtual MenuModel* GetSubmenuModelAt(int index) const override;
  virtual int GetJustifyIndex() const override;
  virtual void MenuWillShow() override;
  virtual void MenuClosed() override;
  virtual void SetMenuModelDelegate(
      MenuModelDelegate* menu_model_delegate) override;
  virtual MenuModelDelegate* GetMenuModelDelegate() const override;

  int justify_index = -1;

 protected:
  // Some variants of this model (SystemMenuModel) relies on items to be
  // inserted backwards. This is counter-intuitive for the API, so rather than
  // forcing customers to insert things backwards, we return the indices
  // backwards instead. That's what this method is for. By default, it just
  // returns what it's passed.
  virtual int FlipIndex(int index) const;

 private:
  struct Item;

  typedef std::vector<Item> ItemVector;

  // Caller needs to call FlipIndex() if necessary. Returns |index|.
  int ValidateItemIndex(int index) const;

  // Functions for inserting items into |items_|.
  void AppendItem(const Item& item);
  void InsertItemAtIndex(const Item& item, int index);
  void ValidateItem(const Item& item);

  // Notify the delegate that the menu is closed.
  void OnMenuClosed();

  ItemVector items_;

  Delegate* delegate_;

  MenuModelDelegate* menu_model_delegate_;

  base::WeakPtrFactory<SimpleMenuModel> method_factory_;
};

}  // namespace aui
