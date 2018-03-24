#include "components/main/views/status_bar_controller.h"

#include <algorithm>

using std::max;
using std::min;

#include <atlbase.h>

#include <atlapp.h>
#include <atlctrls.h>

#include "base/strings/sys_string_conversions.h"
#include "common/event_manager.h"
#include "common/node_service.h"
#include "common/node_util.h"
#include "core/monitored_item_service.h"
#include "core/session_service.h"
#include "remote/session_proxy.h"
#include "views/client_utils_views.h"

StatusBarController::StatusBarController(
    const StatusBarControllerContext& context)
    : StatusBarControllerContext{context} {
}

void StatusBarController::Init(HWND hwnd) {
  hwnd_ = hwnd;

  Layout();
  Update();

  update_timer_.Start(
      FROM_HERE, base::TimeDelta::FromMilliseconds(500),
      base::Bind(&StatusBarController::Update, base::Unretained(this)));
}

void StatusBarController::Layout() {
  WTL::CStatusBarCtrl status_bar(hwnd_);
  RECT rect;
  status_bar.GetClientRect(&rect);

  int parts[] = {rect.right - rect.left, 100, 100, 100, 100, 120};
  for (int i = 1; i < _countof(parts); i++)
    parts[0] -= parts[i];
  for (int i = 1; i < _countof(parts); i++)
    parts[i] += parts[i - 1];
  status_bar.SetParts(_countof(parts), parts);
}

void StatusBarController::Update() {
  size_t unacked_event_count = event_manager_.unacked_events().size();
  base::string16 event_status =
      unacked_event_count
          ? base::StringPrintf(L"Событий: %u", unacked_event_count)
          : L"Нет событий";
  SetPaneText(1, event_status);

  base::string16 event_severity_min =
      base::StringPrintf(L"Важность: %u", event_manager_.severity_min());
  SetPaneText(2, event_severity_min);

  auto& user_id = session_service_.GetUserId();
  base::string16 user_status = GetDisplayName(node_service_, user_id);
  SetPaneText(3, user_status);

  base::TimeDelta ping_delay;
  auto connected = session_service_.IsConnected(&ping_delay);

  SetPaneText(4, connected ? L"Подключен" : L"Отключен");
  SetPaneText(5, connected
                     ? base::StringPrintf(
                           L"Отклик: %u мс",
                           static_cast<unsigned>(ping_delay.InMilliseconds()))
                     : L"Нет отклика");
}

void StatusBarController::SetPaneText(int pane, const base::string16& text) {
  WTL::CStatusBarCtrl status_bar(hwnd_);
  base::char16 buffer[256] = L"";
  status_bar.GetText(pane, buffer);
  if (text != buffer)
    status_bar.SetText(pane, text.c_str());
}
