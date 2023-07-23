#pragma once

#include "aui/models/menu_model_delegate.h"
#include "aui/models/menu_separator_types.h"

#include <string>

namespace aui {

// An interface implemented by an object that provides the content of a menu.
class MenuModel {
 public:
  // The type of item.
  enum ItemType {
    TYPE_COMMAND,
    TYPE_CHECK,
    TYPE_RADIO,
    TYPE_SEPARATOR,
    TYPE_BUTTON_ITEM,
    TYPE_SUBMENU,
    TYPE_INPLACE_MENU
  };

  virtual ~MenuModel() {}

  // Returns the number of items in the menu.
  virtual int GetItemCount() const = 0;

  // Returns the type of item at the specified index.
  virtual ItemType GetTypeAt(int index) const = 0;

  // Returns the separator type at the specified index.
  virtual MenuSeparatorType GetSeparatorTypeAt(int index) const = 0;

  // Returns the command id of the item at the specified index.
  virtual int GetCommandIdAt(int index) const = 0;

  // Returns the label of the item at the specified index.
  virtual std::u16string GetLabelAt(int index) const = 0;

  // Returns true if the menu item (label/icon) at the specified index can
  // change over the course of the menu's lifetime. If this function returns
  // true, the label and icon of the menu item will be updated each time the
  // menu is shown.
  virtual bool IsItemDynamicAt(int index) const = 0;

  // Returns the checked state of the item at the specified index.
  virtual bool IsItemCheckedAt(int index) const = 0;

  // Returns the id of the group of radio items that the item at the specified
  // index belongs to.
  virtual int GetGroupIdAt(int index) const = 0;

  // Returns the enabled state of the item at the specified index.
  virtual bool IsEnabledAt(int index) const = 0;

  // Returns true if the menu item is visible.
  virtual bool IsVisibleAt(int index) const;

  // Returns the model for the submenu at the specified index.
  virtual MenuModel* GetSubmenuModelAt(int index) const = 0;

  // Called when the highlighted menu item changes to the item at the specified
  // index.
  virtual void HighlightChangedTo(int index) = 0;

  // Called when the item at the specified index has been activated.
  virtual void ActivatedAt(int index) = 0;

  // Called when the item has been activated with given event flags.
  // (for the case where the activation involves a navigation).
  // |event_flags| is a bit mask of EventFlags.
  virtual void ActivatedAt(int index, int event_flags);

  // Returns index from which menu items are justified to the right. For
  // top-level menu only.
  virtual int GetJustifyIndex() const;

  // Called when the menu is about to be shown.
  virtual void MenuWillShow() {}

  // Called when the menu has been closed.
  virtual void MenuClosed() {}

  // Set the MenuModelDelegate. Owned by the caller of this function.
  virtual void SetMenuModelDelegate(MenuModelDelegate* delegate) = 0;

  // Gets the MenuModelDelegate.
  virtual MenuModelDelegate* GetMenuModelDelegate() const = 0;

  // Retrieves the model and index that contains a specific command id. Returns
  // true if an item with the specified command id is found. |model| is inout,
  // and specifies the model to start searching from.
  static bool GetModelAndIndexForCommandId(int command_id,
                                           MenuModel** model,
                                           int* index);
};

}  // namespace aui
