#include "components/main/views/status_bar_controller.h"

#include <algorithm>

using std::min;
using std::max;

#include <atlbase.h>
#include <wtl/atlapp.h>
#include <wtl/atlctrls.h>

#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "common/event_manager.h"
#include "core/session_service.h"
#include "common/node_ref_service.h"

StatusBarController::StatusBarController(HWND hwnd, NodeRefService& node_service,
    events::EventManager& event_manager, scada::SessionService& session_state_notifier)
    : hwnd_(hwnd),
      node_service_(node_service),
      event_manager_(event_manager),
      session_service_(session_state_notifier) {
  Layout();
  Update();

  update_timer_.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(500),
      base::Bind(&StatusBarController::Update, base::Unretained(this)));
}

void StatusBarController::Layout() {
  WTL::CStatusBarCtrl status_bar(hwnd_);
  RECT rect;
  status_bar.GetClientRect(&rect);

  int parts[] = { rect.right - rect.left, 100, 100, 100, 100 };
  for (int i = 1; i < _countof(parts); i++)
    parts[0] -= parts[i];
  for (int i = 1; i < _countof(parts); i++)
    parts[i] += parts[i - 1];
  status_bar.SetParts(_countof(parts), parts);
}

void StatusBarController::Update() {
  size_t unacked_event_count = event_manager_.unacked_events().size();
  base::string16 event_status = unacked_event_count ?
      base::StringPrintf(L"Событий: %u", unacked_event_count) :
      L"Нет событий";
  SetPaneText(1, event_status);

  base::string16 event_severity_min =
      base::StringPrintf(L"Важность: %u", event_manager_.severity_min());
  SetPaneText(2, event_severity_min);

  auto& user_id = session_service_.GetUserId();
  base::string16 user_status = base::SysNativeMBToWide(node_service_.GetNode(user_id).display_name().text());
  SetPaneText(3, user_status);

  auto* connect_string = session_service_.IsConnected() ? L"Подключен" : L"Отключен";
  SetPaneText(4, connect_string);
}

void StatusBarController::SetPaneText(int pane, const base::string16& text) {
  WTL::CStatusBarCtrl status_bar(hwnd_);
  base::char16 buffer[256] = L"";
  status_bar.GetText(pane, buffer);
  if (text != buffer)
    status_bar.SetText(pane, text.c_str());
}
