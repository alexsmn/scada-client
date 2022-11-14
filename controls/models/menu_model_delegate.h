#pragma once

namespace aui {

class MenuModelDelegate {
 public:
  // Invoked when an icon has been loaded from history.
  virtual void OnIconChanged(int index) = 0;

 protected:
  virtual ~MenuModelDelegate() {}
};

}  // namespace aui
