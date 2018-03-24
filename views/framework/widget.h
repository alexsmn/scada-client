#pragma once

#include <atlbase.h>

#include <atlapp.h>
#include <atlctrls.h>
#include <atlwin.h>

namespace framework {

class Widget {
 protected:
  friend class DialogImplWtl;

  virtual void OnCommand(UINT notification_code) {}
  virtual LRESULT OnNotify(int w_param, LPNMHDR l_param) { return 0; }
};

}  // namespace framework
