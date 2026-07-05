#pragma once

#include <boost/signals2/connection.hpp>
#include <functional>
#include <string>

namespace aui {

class StatusBarModel {
 public:
  using PanesChangedCallback = std::function<void(int index, int count)>;

  virtual ~StatusBarModel() = default;

  virtual int GetPaneCount() const = 0;
  virtual std::u16string GetPaneText(int index) const = 0;
  virtual int GetPaneSize(int index) const = 0;

  // Notifies after the text of |count| panes starting at |index| changed.
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribePanesChanged(const PanesChangedCallback& callback) = 0;
};

}  // namespace aui
