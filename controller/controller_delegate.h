#pragma once

#include "aui/point.h"
#include "controller/node_id_set.h"

#include <boost/signals2/connection.hpp>
#include <functional>
#include <string_view>

namespace scada::aui {
class MenuModel;
}

class CommandHandler;
class ContentsModel;
class NodeRef;
class WindowDefinition;

class ControllerDelegate {
 public:
  virtual void SetTitle(std::u16string_view title) = 0;

  // Resolves a command against the full command surface the shell offers this
  // view — selection-scoped commands, opened-view commands, and globally
  // registered actions — with the same resolution the main-window toolbar and
  // command palette use. Returns null when nothing currently handles the
  // command, so a view-embedded control can hide or disable itself. The
  // default (headless/test hosts) offers no shell commands.
  virtual CommandHandler* ResolveViewCommand(unsigned command_id) {
    return nullptr;
  }

  // * `merge_menu` is an additional menu to prepend the result menu. It can be
  // null.
  // * `point` is in _screen_ coordinates.
  // * `right_click` should be set if popup is initated by right-click.
  virtual void ShowPopupMenu(scada::aui::MenuModel* merge_menu,
                             const scada::aui::Point& point,
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
