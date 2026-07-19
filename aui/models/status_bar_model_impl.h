#pragma once

#include "aui/aui_ns_compat.h"

#include "aui/models/status_bar_model.h"

#include <boost/signals2/signal.hpp>
#include <functional>
#include <memory>
#include <vector>

namespace scada::aui {

// Provider-driven StatusBarModel: the application registers panes whose text,
// colour and counters are pulled from callbacks, and pushes refreshes via
// NotifyPanesChanged(). Keeps the model free of any domain coupling — the
// providers live with the consumer.
class StatusBarModelImpl final : public StatusBarModel {
 public:
  StatusBarModelImpl();

  using StatusTextProvider = std::function<std::u16string()>;
  using StatusColorProvider = std::function<std::optional<Color>()>;

  struct StatusPane {
    StatusTextProvider text_provider;
    // Optional: supplies the pane's text colour (e.g. a severity indicator).
    StatusColorProvider color_provider;
    int size = -1;
    // Also surface this pane in the top context bar's curated cluster.
    bool in_context_bar = false;
  };

  using AlarmCountProvider = std::function<int()>;
  using SeverityCountProvider = std::function<int(SeverityLevel)>;

  // Returns pane index that can be used when calling `NotifyPaneChanged`.
  int AddPane(const StatusPane& pane);

  // Supplies the unacknowledged-alarm count surfaced via GetAlarmCount().
  void SetAlarmCountProvider(AlarmCountProvider provider);

  // Supplies the per-severity alarm counts surfaced via GetSeverityCount().
  void SetSeverityCountProvider(SeverityCountProvider provider);

  void NotifyPanesChanged(int index, int count = 1);

  // StatusBarModel
  virtual int GetPaneCount() const override;
  virtual std::u16string GetPaneText(int index) const override;
  virtual int GetPaneSize(int index) const override;
  virtual std::optional<Color> GetPaneColor(int index) const override;
  virtual int GetAlarmCount() const override;
  virtual bool IsContextBarPane(int index) const override;
  virtual int GetSeverityCount(SeverityLevel level) const override;
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribePanesChanged(const PanesChangedCallback& callback) override;

 private:
  std::vector<StatusPane> panes_;
  AlarmCountProvider alarm_count_provider_;
  SeverityCountProvider severity_count_provider_;

  boost::signals2::signal<void(int, int)> panes_changed_signal_;
};

}  // namespace scada::aui
