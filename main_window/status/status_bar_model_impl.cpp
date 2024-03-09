#include "main_window/status/status_bar_model_impl.h"

StatusBarModelImpl::StatusBarModelImpl() {
  panes_.emplace_back();
}

int StatusBarModelImpl::AddPane(const StatusPane& pane) {
  panes_.emplace_back(pane);
  return static_cast<int>(panes_.size()) - 1;
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

void StatusBarModelImpl::AddObserver(aui::StatusBarModelObserver& observer) {
  observers_.AddObserver(&observer);
}

void StatusBarModelImpl::RemoveObserver(aui::StatusBarModelObserver& observer) {
  observers_.RemoveObserver(&observer);
}
void StatusBarModelImpl::NotifyPanesChanged(int index, int count) {
  for (auto& o : observers_) {
    o.OnPanesChanged(index, count);
  }
}
