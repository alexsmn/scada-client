#pragma once

#include "base/macros.h"
#include "base/win/win_util2.h"

#include <map>
#include <memory>

namespace framework {

class Dialog;
class Widget;

class DialogImpl {
 public:
  explicit DialogImpl(Dialog& dialog)
      : dialog_(dialog),
        window_handle_(NULL) { }
  virtual ~DialogImpl() { }
  
  HWND window_handle() const { return window_handle_; }
  
  virtual unsigned Execute(HWND parent) = 0;
  virtual void EndDialog(unsigned modal_result) = 0;

 protected:
  Dialog& dialog() const { return dialog_; }
 
  void set_window_handle(HWND handle) { window_handle_ = handle; }
  
 private:
  Dialog& dialog_;
  HWND window_handle_;

  DISALLOW_COPY_AND_ASSIGN(DialogImpl);
};

class Dialog {
public:
  explicit Dialog(unsigned resource_id);

  unsigned resource_id() const { return resource_id_; }
  HWND window_handle() const { return impl_->window_handle(); }

  base::string16 GetWindowText() const {
    return win_util::GetWindowText(window_handle());
  }

  unsigned Execute(HWND parent = ::GetActiveWindow()) {
    return impl_->Execute(parent);
  }

 protected:
  friend class DialogImplWtl;
 
  HWND GetItem(unsigned item_id) const {
    return ::GetDlgItem(window_handle(), item_id);
  }
  int GetItemInt(unsigned item_id) const {
    return ::GetDlgItemInt(window_handle(), item_id, NULL, TRUE);
  }
  base::string16 GetItemText(unsigned item_id) const {
    return win_util::GetWindowText(GetItem(item_id));
  }

  void SetWindowText(const base::string16& text) {
    ::SetWindowText(window_handle(), text.c_str());
  }
  void SetItemText(unsigned item_id, const base::string16& text) {
    ::SetDlgItemText(window_handle(), item_id, text.c_str());
  }
  void SetItemInt(unsigned item_id, int value) {
    ::SetDlgItemInt(window_handle(), item_id, value, TRUE);
  }
  
  void AttachView(Widget& view, unsigned item_id) {
    view_map_[item_id] = &view;
  }

  Widget* GetView(unsigned id) const {
    ViewMap::const_iterator i = view_map_.find(id);
    return i != view_map_.end() ? i->second : NULL;
  }

  void EndDialog(unsigned modal_result) {
    impl_->EndDialog(modal_result);
  }

  virtual void OnInitDialog() { }
  virtual void OnOK() { EndDialog(IDOK); }
  virtual void OnCancel() { EndDialog(IDCANCEL); }

 private:
  typedef std::map<unsigned, Widget*> ViewMap;
  
  unsigned resource_id_;
  std::unique_ptr<DialogImpl> impl_;
  
  ViewMap view_map_;

  DISALLOW_COPY_AND_ASSIGN(Dialog);
};

} // namespace framework
