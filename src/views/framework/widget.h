#pragma once

#include <atlbase.h>
#include <wtl/atlapp.h>
#include <atlwin.h>
#include <wtl/atlctrls.h>

namespace framework {

class Widget {
 protected:
  friend class DialogImplWtl;
 
  virtual void OnCommand(UINT notification_code) { }
  virtual LRESULT OnNotify(int w_param, LPNMHDR l_param) { return 0; }
};

} // namespace framework
