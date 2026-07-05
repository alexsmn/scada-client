#pragma once

#include "aui/models/status_bar_model.h"

#include <boost/signals2/signal.hpp>
#include <functional>
#include <memory>

class StatusBarModelImpl final : public aui::StatusBarModel {
 public:
  StatusBarModelImpl();

  using StatusTextProvider = std::function<std::u16string()>;

  struct StatusPane {
    StatusTextProvider text_provider;
    int size = -1;
  };

  // Returns pane index that can be used when calling `NotifyPaneChanged`.
  int AddPane(const StatusPane& pane);

  void NotifyPanesChanged(int index, int count = 1);

  // StatusBarModel
  virtual int GetPaneCount() const override;
  virtual std::u16string GetPaneText(int index) const override;
  virtual int GetPaneSize(int index) const override;
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribePanesChanged(const PanesChangedCallback& callback) override;

 private:
  std::vector<StatusPane> panes_;

  boost::signals2::signal<void(int, int)> panes_changed_signal_;
};
