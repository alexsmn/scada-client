#include "components/main/views/status_bar_controller.h"

#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "controls/status_bar_model.h"

#include <algorithm>

using std::max;
using std::min;

#include <atlbase.h>

#include <atlapp.h>
#include <atlctrls.h>

StatusBarController::StatusBarController(std::shared_ptr<StatusBarModel> model,
                                         HWND hwnd)
    : model_{std::move(model)}, hwnd_{hwnd} {
  model_->AddObserver(*this);

  Layout();

  for (int i = 0; i < model_->GetPaneCount(); ++i)
    SetPaneText(i, model_->GetPaneText(i));
}

StatusBarController::~StatusBarController() {
  model_->RemoveObserver(*this);
}

void StatusBarController::Layout() {
  WTL::CStatusBarCtrl status_bar(hwnd_);

  RECT rect;
  status_bar.GetClientRect(&rect);

  std::vector<int> parts(model_->GetPaneCount());

  parts[0] = rect.right - rect.left;
  for (int i = 1; i < model_->GetPaneCount(); ++i)
    parts[i] = model_->GetPaneSize(i);

  for (int i = 1; i < static_cast<int>(parts.size()); i++)
    parts[0] -= parts[i];
  for (int i = 1; i < static_cast<int>(parts.size()); i++)
    parts[i] += parts[i - 1];

  status_bar.SetParts(static_cast<int>(parts.size()), parts.data());
}

void StatusBarController::SetPaneText(int pane, const std::u16string& text) {
  auto text_wstring = base::AsWString(text);
  WTL::CStatusBarCtrl status_bar(hwnd_);
  wchar_t buffer[256] = L"";
  status_bar.GetText(pane, buffer);
  if (text_wstring != buffer)
    status_bar.SetText(pane, text_wstring.c_str());
}

void StatusBarController::OnPanesChanged(int index, int count) {
  wchar_t buffer[256] = L"";
  for (int i = 0; i < count; ++i)
    SetPaneText(index + i, model_->GetPaneText(index + i));
}

void StatusBarController::OnProgressChanged() {}
