#include "components/main/status_bar_model_impl.h"

#include "base/strings/stringprintf.h"
#include "common/event_fetcher.h"
#include "core/session_service.h"
#include "node_service/node_util.h"

namespace {

void MergeProgress(const StatusBarModel::Progress& from,
                   StatusBarModel::Progress& to) {
  if (!from.active)
    return;

  to.active = true;
  to.range += from.range;
  to.current += from.current;
}

}  // namespace

// StatusBarModelImpl

StatusBarModelImpl::StatusBarModelImpl(StatusBarModelImplContext&& context)
    : StatusBarModelImplContext{std::move(context)} {
  task_manager_.AddObserver(*this);

  update_timer_.Start(
      FROM_HERE, base::TimeDelta::FromMilliseconds(500),
      base::Bind(&StatusBarModelImpl::OnTimer, base::Unretained(this)));
}

StatusBarModelImpl::~StatusBarModelImpl() {
  task_manager_.RemoveObserver(*this);
}

int StatusBarModelImpl::GetPaneCount() {
  return 6;
}

std::wstring StatusBarModelImpl::GetPaneText(int index) {
  switch (index) {
    case 1: {
      size_t unacked_event_count = event_fetcher_.unacked_events().size();
      return unacked_event_count
                 ? base::StringPrintf(L"Событий: %u", unacked_event_count)
                 : L"Нет событий";
    }

    case 2:
      return base::StringPrintf(L"Важность: %u", event_fetcher_.severity_min());

    case 3: {
      const auto& user_id = session_service_.GetUserId();
      return GetDisplayName(node_service_, user_id);
    }

    case 4: {
      base::TimeDelta ping_delay;
      auto connected = session_service_.IsConnected(&ping_delay);
      return connected ? L"Подключен" : L"Отключен";
    }

    case 5: {
      base::TimeDelta ping_delay;
      auto connected = session_service_.IsConnected(&ping_delay);
      return connected ? base::StringPrintf(
                             L"Отклик: %u мс",
                             static_cast<unsigned>(ping_delay.InMilliseconds()))
                       : L"Нет отклика";
    }

    default:
      return {};
  }
}

int StatusBarModelImpl::GetPaneSize(int index) {
  const int kSizes[] = {-1, 100, 100, 100, 100, 120};
  return kSizes[index];
}

StatusBarModel::Progress StatusBarModelImpl::GetProgress() const {
  return progress_;
}

void StatusBarModelImpl::AddObserver(StatusBarModelObserver& observer) {
  observers_.AddObserver(&observer);
}

void StatusBarModelImpl::RemoveObserver(StatusBarModelObserver& observer) {
  observers_.RemoveObserver(&observer);
}

void StatusBarModelImpl::OnTimer() {
  const int pending_task_count = pending_task_provider_();
  if (pending_task_count != pending_task_count_) {
    pending_task_count_ = pending_task_count;
    max_pending_task_count_ =
        pending_task_count_ != 0
            ? std ::max(max_pending_task_count_, pending_task_count_)
            : 0;
    UpdateProgress();
  }

  for (auto& o : observers_)
    o.OnPanesChanged(0, 6);
}

void StatusBarModelImpl::UpdateProgress() {
  progress_ = {};
  MergeProgress({task_manager_status_.active, task_manager_status_.range,
                 task_manager_status_.current},
                progress_);
  MergeProgress({pending_task_count_ != 0, max_pending_task_count_,
                 max_pending_task_count_ - pending_task_count_},
                progress_);

  for (auto& o : observers_)
    o.OnProgressChanged();
}

void StatusBarModelImpl::OnTaskManagerStatus(
    const TaskManagerObserver::Status& status) {
  task_manager_status_ = status;
  UpdateProgress();
}
