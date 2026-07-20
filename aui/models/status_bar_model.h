#pragma once

#include "aui/color.h"
#include "aui/severity_colors.h"

#include <boost/signals2/connection.hpp>
#include <functional>
#include <optional>
#include <string>

namespace scada::aui {

class StatusBarModel {
 public:
  using PanesChangedCallback = std::function<void(int index, int count)>;

  virtual ~StatusBarModel() = default;

  virtual int GetPaneCount() const = 0;
  virtual std::u16string GetPaneText(int index) const = 0;
  virtual int GetPaneSize(int index) const = 0;

  // Optional foreground colour for a pane's text (e.g. a severity indicator).
  // Default: no colour, i.e. the pane uses the status bar's normal text colour.
  virtual std::optional<Color> GetPaneColor(int index) const {
    return std::nullopt;
  }

  // Count of currently unacknowledged alarms, for chrome that shows an unread
  // badge (e.g. the activity rail). Refreshed together with the panes, so
  // observers read it on SubscribePanesChanged. Default 0 for models that do
  // not track alarms.
  virtual int GetAlarmCount() const { return 0; }

  // Whether pane |index| belongs in the top context bar's curated cluster (the
  // who/where context — user, connection, server), as opposed to the full
  // status strip. Default false, so a plain status bar contributes no cluster.
  virtual bool IsContextBarPane(int index) const { return false; }

  // Count of currently unacknowledged alarms at `level`, for the live severity
  // KPI tiles. Refreshed with the panes; default 0 for models without alarms.
  virtual int GetSeverityCount(SeverityLevel level) const { return 0; }

  // Notifies after the text of |count| panes starting at |index| changed.
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribePanesChanged(const PanesChangedCallback& callback) = 0;
};

}  // namespace scada::aui
