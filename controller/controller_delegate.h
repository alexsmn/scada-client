#pragma once

#include "aui/aui_ns_compat.h"

#include "aui/point.h"
#include "controller/node_id_set.h"

#include <boost/signals2/connection.hpp>
#include <functional>
#include <string_view>

namespace scada::aui {
class MenuModel;
}

class ContentsModel;
class NodeRef;
class WindowDefinition;

class ControllerDelegate {
 public:
  virtual void SetTitle(std::u16string_view title) = 0;

  // * `merge_menu` is an additional menu to prepend the result menu. It can be
  // null.
  // * `point` is in _screen_ coordinates.
  // * `right_click` should be set if popup is initated by right-click.
  virtual void ShowPopupMenu(aui::MenuModel* merge_menu,
                             unsigned resource_id,
                             const aui::Point& point,
                             bool right_click) = 0;

  virtual void SetModified(bool modified) = 0;

  virtual void Close() = 0;

  virtual void OpenView(const WindowDefinition& def) = 0;

  virtual void ExecuteDefaultNodeCommand(const NodeRef& node) = 0;

  using ContentsChangedCallback =
      std::function<void(const NodeIdSet& node_ids)>;
  using ContainedItemChangedCallback =
      std::function<void(const scada::NodeId& node_id, bool added)>;

  virtual ContentsModel* GetActiveContentsModel() = 0;
  // Notifies after the contained-item set of the active view changed
  // wholesale.
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeContentsChanged(const ContentsChangedCallback& callback) = 0;
  // Notifies after a single contained item was added or removed.
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeContainedItemChanged(
      const ContainedItemChangedCallback& callback) = 0;

  virtual void Focus() = 0;
};
