#include "main_window/status_bar_model_impl.h"

#include "base/executor.h"
#include "base/executor_task_runner.h"
#include "base/strings/stringprintf.h"
#include "events/node_event_provider.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "profile/profile.h"
#include "scada/session_service.h"

using namespace std::chrono_literals;

// StatusBarModelImpl

StatusBarModelImpl::StatusBarModelImpl(StatusBarModelImplContext&& context)
    : StatusBarModelImplContext{std::move(context)},
      session_poll_timer_{executor_} {
  connections_.emplace_back(progress_host_.Subscribe(
      [this](const ProgressStatus& status) { OnProgressStatus(status); }));

  // TODO: weak_ptr.
  connections_.emplace_back(
      session_service_.SubscribeSessionStateChanged(BindExecutor(
          executor_, [this](bool connected, const scada::Status& status) {
            UpdateUser();
          })));

  connections_.emplace_back(profile_.AddChangeObserver(
      [this] { NotifyPanelsChanged(kSeverityPaneIndex); }));

  UpdateUser();

  session_poll_timer_.StartRepeating(1s, [this] {
    NotifyPanelsChanged(kSessionFirstPaneIndex, kSessionPaneCount);
  });
}

StatusBarModelImpl::~StatusBarModelImpl() {
  user_node_.Unsubscribe(*this);
}

int StatusBarModelImpl::GetPaneCount() const {
  return kPaneCount;
}

std::u16string StatusBarModelImpl::GetEventPaneText() const {
  size_t unacked_event_count = node_event_provider_.unacked_events().size();
  return unacked_event_count
             ? base::StringPrintf(u"Событий: %u", unacked_event_count)
             : u"Нет событий";
}

std::u16string StatusBarModelImpl::GetSessionPaneText() const {
  base::TimeDelta ping_delay;
  auto connected = session_service_.IsConnected(&ping_delay);
  return connected ? u"Подключен" : u"Отключен";
}

std::u16string StatusBarModelImpl::GetPingPaneText() const {
  base::TimeDelta ping_delay;
  auto connected = session_service_.IsConnected(&ping_delay);
  return connected ? base::StringPrintf(
                         u"Отклик: %u мс",
                         static_cast<unsigned>(ping_delay.InMilliseconds()))
                   : u"Нет отклика";
}

std::u16string StatusBarModelImpl::GetPaneText(int index) const {
  switch (index) {
    case kEventPaneIndex:
      return GetEventPaneText();

    case kSeverityPaneIndex:
      return base::StringPrintf(u"Важность: %u",
                                node_event_provider_.severity_min());

    case kUserPaneIndex:
      return user_node_.display_name();

    case kSessionFirstPaneIndex:
      return GetSessionPaneText();

    case kSessionFirstPaneIndex + 1:
      return GetPingPaneText();

    default:
      return {};
  }
}

int StatusBarModelImpl::GetPaneSize(int index) const {
  const int kSizes[] = {-1, 100, 100, 100, 100, 120};
  static_assert(arraysize(kSizes) == kPaneCount, "Invalid kSizes size");
  return kSizes[index];
}

aui::StatusBarModel::Progress StatusBarModelImpl::GetProgress() const {
  return progress_;
}

void StatusBarModelImpl::AddObserver(aui::StatusBarModelObserver& observer) {
  observers_.AddObserver(&observer);
}

void StatusBarModelImpl::RemoveObserver(aui::StatusBarModelObserver& observer) {
  observers_.RemoveObserver(&observer);
}

void StatusBarModelImpl::OnProgressStatus(const ProgressStatus& status) {
  progress_ = {status.active, status.range, status.current};

  for (auto& o : observers_)
    o.OnProgressChanged();
}

void StatusBarModelImpl::UpdateUser() {
  auto user_id = session_service_.GetUserId();
  if (user_id == user_node_.node_id())
    return;

  user_node_.Unsubscribe(*this);

  user_node_ = node_service_.GetNode(user_id);
  user_node_.Fetch(NodeFetchStatus::NodeOnly());
  user_node_.Subscribe(*this);

  NotifyPanelsChanged(kUserPaneIndex);
}

void StatusBarModelImpl::OnNodeSemanticChanged(const scada::NodeId& node_id) {
  NotifyPanelsChanged(kUserPaneIndex);
}

void StatusBarModelImpl::NotifyPanelsChanged(int index, int count) {
  for (auto& o : observers_) {
    o.OnPanesChanged(index, count);
  }
}
