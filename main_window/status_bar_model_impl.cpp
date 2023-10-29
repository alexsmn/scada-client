#include "main_window/status_bar_model_impl.h"

#include "base/executor.h"
#include "base/strings/stringprintf.h"
#include "events/node_event_provider.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "scada/session_service.h"

// StatusBarModelImpl

StatusBarModelImpl::StatusBarModelImpl(StatusBarModelImplContext&& context)
    : StatusBarModelImplContext{std::move(context)} {
  progress_host_connection_ = progress_host_.Subscribe(
      [this](const ProgressStatus& status) { OnProgressStatus(status); });

  // TODO: weak_ptr.
  session_state_changed_connection_ =
      session_service_.SubscribeSessionStateChanged(BindExecutor(
          executor_, [this](bool connected, const scada::Status& status) {
            UpdateUser();
          }));

  UpdateUser();
}

StatusBarModelImpl::~StatusBarModelImpl() {
  user_node_.Unsubscribe(*this);
}

int StatusBarModelImpl::GetPaneCount() const {
  return 6;
}

std::u16string StatusBarModelImpl::GetPaneText(int index) const {
  switch (index) {
    case 1: {
      size_t unacked_event_count = node_event_provider_.unacked_events().size();
      return unacked_event_count
                 ? base::StringPrintf(u"Событий: %u", unacked_event_count)
                 : u"Нет событий";
    }

    case 2:
      return base::StringPrintf(u"Важность: %u",
                                node_event_provider_.severity_min());

    case 3:
      return user_node_.display_name();

    case 4: {
      base::TimeDelta ping_delay;
      auto connected = session_service_.IsConnected(&ping_delay);
      return connected ? u"Подключен" : u"Отключен";
    }

    case 5: {
      base::TimeDelta ping_delay;
      auto connected = session_service_.IsConnected(&ping_delay);
      return connected ? base::StringPrintf(
                             u"Отклик: %u мс",
                             static_cast<unsigned>(ping_delay.InMilliseconds()))
                       : u"Нет отклика";
    }

    default:
      return {};
  }
}

int StatusBarModelImpl::GetPaneSize(int index) const {
  const int kSizes[] = {-1, 100, 100, 100, 100, 120};
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

  for (auto& o : observers_)
    o.OnPanesChanged(kUserPaneIndex, 1);
}

void StatusBarModelImpl::OnNodeSemanticChanged(const scada::NodeId& node_id) {
  for (auto& o : observers_)
    o.OnPanesChanged(kUserPaneIndex, 1);
}
