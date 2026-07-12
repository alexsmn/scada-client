#include "main_window/status_bar/status_bar_model_impl.h"

StatusBarModelImpl::StatusBarModelImpl() {
  panes_.emplace_back();
}

int StatusBarModelImpl::AddPane(const StatusPane& pane) {
  panes_.emplace_back(pane);
  return static_cast<int>(panes_.size()) - 1;
}

void StatusBarModelImpl::SetAlarmCountProvider(AlarmCountProvider provider) {
  alarm_count_provider_ = std::move(provider);
}

int StatusBarModelImpl::GetPaneCount() const {
  return static_cast<int>(panes_.size());
}

std::u16string StatusBarModelImpl::GetPaneText(int index) const {
  auto text_provider = panes_[index].text_provider;
  return text_provider ? text_provider() : std::u16string{};
}

int StatusBarModelImpl::GetPaneSize(int index) const {
  return panes_[index].size;
}

std::optional<aui::Color> StatusBarModelImpl::GetPaneColor(int index) const {
  auto color_provider = panes_[index].color_provider;
  return color_provider ? color_provider() : std::nullopt;
}

int StatusBarModelImpl::GetAlarmCount() const {
  return alarm_count_provider_ ? alarm_count_provider_() : 0;
}

boost::signals2::scoped_connection StatusBarModelImpl::SubscribePanesChanged(
    const PanesChangedCallback& callback) {
  return panes_changed_signal_.connect(callback);
}

void StatusBarModelImpl::NotifyPanesChanged(int index, int count) {
  panes_changed_signal_(index, count);
}
