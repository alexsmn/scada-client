#include "components/main/status_bar_model_impl.h"

#include "base/strings/stringprintf.h"
#include "common/event_fetcher.h"
#include "core/session_service.h"
#include "node_service/node_util.h"

// StatusBarModelImpl

StatusBarModelImpl::StatusBarModelImpl(StatusBarModelImplContext&& context)
    : StatusBarModelImplContext{std::move(context)} {
  progress_host_connection_ = progress_host_.Subscribe(
      [this](const ProgressStatus& status) { OnProgressStatus(status); });
}

StatusBarModelImpl::~StatusBarModelImpl() {}

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

void StatusBarModelImpl::OnProgressStatus(const ProgressStatus& status) {
  progress_ = {status.active, status.range, status.current};

  for (auto& o : observers_)
    o.OnProgressChanged();
}
