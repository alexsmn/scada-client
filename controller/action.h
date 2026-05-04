#pragma once

#include "aui/key_codes.h"

#include <functional>
#include <optional>
#include <string>
#include <utility>

enum CommandCategory {
  CATEGORY_NEW,
  CATEGORY_OPEN,
  CATEGORY_ITEM,
  CATEGORY_DEVICE,
  CATEGORY_SETUP,
  CATEGORY_EXPORT,
  CATEGORY_SPECIFIC,
  CATEGORY_VIEW,
  CATEGORY_PERIOD,
  CATEGORY_CREATE,
  CATEGORY_EDIT,
  CATEGORY_AGGREGATION,
  CATEGORY_INTERVAL,
  CATEGORY_COUNT,
};

class Shortcut {
 public:
  Shortcut(aui::KeyCode key_code) : key_code_{key_code} {}
  Shortcut(aui::KeyModifier key_modifier, aui::KeyCode key_code)
      : modifiers_{static_cast<aui::KeyModifiers>(key_modifier)},
        key_code_{key_code} {}

  aui::KeyCode key_code() const { return key_code_; }
  aui::KeyModifiers modifiers() const { return modifiers_; }

 private:
  aui::KeyCode key_code_ = aui::KeyCode::Unknown;
  aui::KeyModifiers modifiers_{};
};

struct Action {
  enum Flag {
    ENABLED = 0x0001,
    CHECKED = 0x0002,
    VISIBLE = 0x0004,
    ALWAYS_VISIBLE = 0x0008,
    CHECKABLE = 0x0016,
  };

  unsigned command_id_ = 0;
  CommandCategory category_ = CATEGORY_SPECIFIC;
  std::u16string title_;
  std::u16string short_title_;
  int image_id_ = 0;
  unsigned flags_ = 0;
  std::optional<Shortcut> shortcut_;
  std::function<std::u16string()> title_provider_;

  void set_enabled(bool enabled) { SetFlag(ENABLED, enabled); }
  void set_checkable(bool checkable) { SetFlag(CHECKABLE, checkable); }
  void set_checked(bool checked) { SetFlag(CHECKED, checked); }
  void set_visible(bool visible) { SetFlag(VISIBLE, visible); }

  std::u16string GetTitle() const {
    return title_provider_ ? title_provider_() : title_;
  }

  unsigned command_id() const { return command_id_; }

  std::u16string GetShortTitle() const {
    return short_title_.empty() ? title_ : short_title_;
  }
  void SetTitle(std::u16string title) { title_ = std::move(title); }

  int image_id() const { return image_id_; }
  bool enabled() const { return GetFlag(ENABLED); }
  bool checkable() const { return GetFlag(CHECKABLE); }
  bool checked() const { return GetFlag(CHECKED); }
  bool visible() const { return GetFlag(VISIBLE); }
  bool always_visible() const { return GetFlag(ALWAYS_VISIBLE); }

  bool GetFlag(Flag flag) const { return (flags_ & flag) != 0; }
  void SetFlag(Flag flag, bool state) {
    if (state)
      flags_ |= flag;
    else
      flags_ &= ~flag;
  }
};
