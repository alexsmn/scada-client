#include "aui/models/simple_menu_model.h"

#include "base/bind.h"
#include "base/threading/thread_task_runner_handle.h"

#include <cassert>

namespace aui {

const int kSeparatorId = -1;

struct SimpleMenuModel::Item {
  int command_id;
  base::string16 label;
  ItemType type;
  int group_id;
  MenuModel* submenu;
  MenuSeparatorType separator_type;
};

////////////////////////////////////////////////////////////////////////////////
// SimpleMenuModel::Delegate, public:

bool SimpleMenuModel::Delegate::IsCommandIdVisible(int command_id) const {
  return true;
}

bool SimpleMenuModel::Delegate::IsItemForCommandIdDynamic(
    int command_id) const {
  return false;
}

base::string16 SimpleMenuModel::Delegate::GetLabelForCommandId(
    int command_id) const {
  return base::string16();
}

void SimpleMenuModel::Delegate::CommandIdHighlighted(int command_id) {}

void SimpleMenuModel::Delegate::ExecuteCommand(int command_id,
                                               int event_flags) {
  ExecuteCommand(command_id);
}

void SimpleMenuModel::Delegate::MenuWillShow(SimpleMenuModel* /*source*/) {}

void SimpleMenuModel::Delegate::MenuClosed(SimpleMenuModel* /*source*/) {}

////////////////////////////////////////////////////////////////////////////////
// SimpleMenuModel, public:

SimpleMenuModel::SimpleMenuModel(Delegate* delegate)
    : delegate_(delegate),
      menu_model_delegate_(nullptr),
      method_factory_(this) {}

SimpleMenuModel::~SimpleMenuModel() {}

void SimpleMenuModel::AddItem(int command_id, const base::string16& label) {
  Item item = {command_id, label, TYPE_COMMAND, -1, nullptr, NORMAL_SEPARATOR};
  AppendItem(item);
}

void SimpleMenuModel::AddSeparator(MenuSeparatorType separator_type) {
#if !defined(USE_AURA)
  if (separator_type != NORMAL_SEPARATOR) {
    assert(false && "Not implemented");
  }
#endif
  // DCHECK(items_.empty() || items_.back().type != TYPE_SEPARATOR);
  Item item = {kSeparatorId, base::string16(), TYPE_SEPARATOR,
               -1,           nullptr,          separator_type};
  AppendItem(item);
}

void SimpleMenuModel::AddCheckItem(int command_id,
                                   const base::string16& label) {
  Item item = {command_id, label, TYPE_CHECK, -1, nullptr, NORMAL_SEPARATOR};
  AppendItem(item);
}

void SimpleMenuModel::AddRadioItem(int command_id,
                                   const base::string16& label,
                                   int group_id) {
  Item item = {command_id, label,   TYPE_RADIO,
               group_id,   nullptr, NORMAL_SEPARATOR};
  AppendItem(item);
}

void SimpleMenuModel::AddSeparatorIfNecessary(
    MenuSeparatorType separator_type) {
  if (!items_.empty() && items_.back().type != TYPE_SEPARATOR)
    AddSeparator(separator_type);
}

void SimpleMenuModel::AddSubMenu(int command_id,
                                 const base::string16& label,
                                 MenuModel* model) {
  Item item = {command_id, label, TYPE_SUBMENU, -1, model, NORMAL_SEPARATOR};
  AppendItem(item);
}

void SimpleMenuModel::AddInplaceMenu(MenuModel* model) {
  Item item = {0, {}, TYPE_INPLACE_MENU, -1, model, NORMAL_SEPARATOR};
  AppendItem(item);
}

void SimpleMenuModel::InsertItemAt(int index,
                                   int command_id,
                                   const base::string16& label) {
  Item item = {command_id, label, TYPE_COMMAND, -1, nullptr, NORMAL_SEPARATOR};
  InsertItemAtIndex(item, index);
}

void SimpleMenuModel::InsertSeparatorAt(int index,
                                        MenuSeparatorType separator_type) {
#if !defined(USE_AURA)
  if (separator_type != NORMAL_SEPARATOR) {
    assert(false && "Not implemented");
  }
#endif
  Item item = {kSeparatorId, base::string16(), TYPE_SEPARATOR,
               -1,           nullptr,          separator_type};
  InsertItemAtIndex(item, index);
}

void SimpleMenuModel::InsertCheckItemAt(int index,
                                        int command_id,
                                        const base::string16& label) {
  Item item = {command_id, label, TYPE_CHECK, -1, nullptr, NORMAL_SEPARATOR};
  InsertItemAtIndex(item, index);
}

void SimpleMenuModel::InsertRadioItemAt(int index,
                                        int command_id,
                                        const base::string16& label,
                                        int group_id) {
  Item item = {command_id, label,   TYPE_RADIO,
               group_id,   nullptr, NORMAL_SEPARATOR};
  InsertItemAtIndex(item, index);
}

void SimpleMenuModel::InsertSubMenuAt(int index,
                                      int command_id,
                                      const base::string16& label,
                                      MenuModel* model) {
  Item item = {command_id, label, TYPE_SUBMENU, -1, model, NORMAL_SEPARATOR};
  InsertItemAtIndex(item, index);
}

void SimpleMenuModel::DeleteItems(int index, int count) {
  items_.erase(items_.begin() + index, items_.begin() + index + count);
}

void SimpleMenuModel::Clear() {
  items_.clear();
}

int SimpleMenuModel::GetIndexOfCommandId(int command_id) {
  for (ItemVector::iterator i = items_.begin(); i != items_.end(); ++i) {
    if (i->command_id == command_id) {
      return FlipIndex(static_cast<int>(std::distance(items_.begin(), i)));
    }
  }
  return -1;
}

////////////////////////////////////////////////////////////////////////////////
// SimpleMenuModel, MenuModel implementation:

int SimpleMenuModel::GetItemCount() const {
  return static_cast<int>(items_.size());
}

MenuModel::ItemType SimpleMenuModel::GetTypeAt(int index) const {
  return items_[ValidateItemIndex(FlipIndex(index))].type;
}

MenuSeparatorType SimpleMenuModel::GetSeparatorTypeAt(int index) const {
  return items_[ValidateItemIndex(FlipIndex(index))].separator_type;
}

int SimpleMenuModel::GetCommandIdAt(int index) const {
  return items_[ValidateItemIndex(FlipIndex(index))].command_id;
}

base::string16 SimpleMenuModel::GetLabelAt(int index) const {
  if (IsItemDynamicAt(index))
    return delegate_->GetLabelForCommandId(GetCommandIdAt(index));
  return items_[ValidateItemIndex(FlipIndex(index))].label;
}

bool SimpleMenuModel::IsItemDynamicAt(int index) const {
  if (delegate_)
    return delegate_->IsItemForCommandIdDynamic(GetCommandIdAt(index));
  return false;
}

bool SimpleMenuModel::IsItemCheckedAt(int index) const {
  if (!delegate_)
    return false;
  MenuModel::ItemType item_type = GetTypeAt(index);
  return (item_type == TYPE_CHECK || item_type == TYPE_RADIO)
             ? delegate_->IsCommandIdChecked(GetCommandIdAt(index))
             : false;
}

int SimpleMenuModel::GetGroupIdAt(int index) const {
  return items_[ValidateItemIndex(FlipIndex(index))].group_id;
}

bool SimpleMenuModel::IsEnabledAt(int index) const {
  int command_id = GetCommandIdAt(index);
  if (!delegate_ || command_id == kSeparatorId)
    return true;
  return delegate_->IsCommandIdEnabled(command_id);
}

bool SimpleMenuModel::IsVisibleAt(int index) const {
  int command_id = GetCommandIdAt(index);
  if (!delegate_ || command_id == kSeparatorId)
    return true;
  return delegate_->IsCommandIdVisible(command_id);
}

void SimpleMenuModel::HighlightChangedTo(int index) {
  if (delegate_)
    delegate_->CommandIdHighlighted(GetCommandIdAt(index));
}

void SimpleMenuModel::ActivatedAt(int index) {
  if (delegate_)
    delegate_->ExecuteCommand(GetCommandIdAt(index));
}

void SimpleMenuModel::ActivatedAt(int index, int event_flags) {
  if (delegate_)
    delegate_->ExecuteCommand(GetCommandIdAt(index), event_flags);
}

MenuModel* SimpleMenuModel::GetSubmenuModelAt(int index) const {
  return items_[ValidateItemIndex(FlipIndex(index))].submenu;
}

void SimpleMenuModel::MenuWillShow() {
  if (delegate_)
    delegate_->MenuWillShow(this);
}

int SimpleMenuModel::GetJustifyIndex() const {
  return justify_index;
}

void SimpleMenuModel::MenuClosed() {
  // Due to how menus work on the different platforms, ActivatedAt will be
  // called after this.  It's more convenient for the delegate to be called
  // afterwards though, so post a task.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&SimpleMenuModel::OnMenuClosed, method_factory_.GetWeakPtr()));
}

void SimpleMenuModel::SetMenuModelDelegate(
    MenuModelDelegate* menu_model_delegate) {
  menu_model_delegate_ = menu_model_delegate;
}

MenuModelDelegate* SimpleMenuModel::GetMenuModelDelegate() const {
  return menu_model_delegate_;
}

void SimpleMenuModel::OnMenuClosed() {
  if (delegate_)
    delegate_->MenuClosed(this);
}

int SimpleMenuModel::FlipIndex(int index) const {
  return index;
}

////////////////////////////////////////////////////////////////////////////////
// SimpleMenuModel, Private:

int SimpleMenuModel::ValidateItemIndex(int index) const {
  CHECK_GE(index, 0);
  CHECK_LT(static_cast<size_t>(index), items_.size());
  return index;
}

void SimpleMenuModel::AppendItem(const Item& item) {
  ValidateItem(item);
  items_.push_back(item);
}

void SimpleMenuModel::InsertItemAtIndex(const Item& item, int index) {
  ValidateItem(item);
  items_.insert(items_.begin() + FlipIndex(index), item);
}

void SimpleMenuModel::ValidateItem(const Item& item) {
#ifndef NDEBUG
  if (item.type == TYPE_SEPARATOR) {
    assert(item.command_id == kSeparatorId);
  } else {
    assert(item.command_id >= 0);
  }
#endif  // NDEBUG
}

}  // namespace aui
