#pragma once

#include <functional>
#include <string>
#include <vector>

namespace aui {

struct EditData {
  enum class EditorType { NONE, TEXT, DROPDOWN, BUTTON };
  EditorType editor_type = EditorType::TEXT;

  std::vector<std::u16string> choices;

  using AsyncChoiceCallback =
      std::function<void(const std::vector<std::u16string>& choices,
                         bool last)>;
  using AsyncChoiceHandler =
      std::function<void(const AsyncChoiceCallback& callback)>;
  AsyncChoiceHandler async_choice_handler;
};

}  // namespace aui
