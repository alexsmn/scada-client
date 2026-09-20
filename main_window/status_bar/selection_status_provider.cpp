#include "main_window/status_bar/selection_status_provider.h"

#include "aui/translation.h"

void SelectionStatusProvider::Init(const ChangeNotifier& change_notifier) {
  connection_ = registry_.SubscribeChanged(change_notifier);
}

std::u16string SelectionStatusProvider::GetText() const {
  const std::u16string& label = registry_.label();
  if (label.empty())
    return {};

  return Translate("Selected") + u" · " + label;
}
