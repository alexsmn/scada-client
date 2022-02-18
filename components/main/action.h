#pragma once

#include "base/strings/string16.h"
#include "controls/key_codes.h"

#include <optional>
#include <string>

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

class Action {
 public:
  enum Flag {
    ENABLED = 0x0001,
    CHECKED = 0x0002,
    VISIBLE = 0x0004,
    ALWAYS_VISIBLE = 0x0008,
    CHECKABLE = 0x0016,
  };

  Action(unsigned command_id,
         CommandCategory category,
         std::u16string title,
         std::u16string short_title = std::u16string(),
         int image_id = 0,
         unsigned flags = 0);
  virtual ~Action() {}

  Action(const Action&) = delete;
  Action& operator=(const Action&) = delete;

  void set_enabled(bool enabled) { SetFlag(ENABLED, enabled); }
  void set_checkable(bool checkable) { SetFlag(CHECKABLE, checkable); }
  void set_checked(bool checked) { SetFlag(CHECKED, checked); }
  void set_visible(bool visible) { SetFlag(VISIBLE, visible); }

  virtual std::u16string GetTitle() const { return title_; }

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

  unsigned command_id_;
  CommandCategory category_;
  std::optional<Shortcut> shortcut_;

 private:
  bool GetFlag(Flag flag) const { return (flags_ & flag) != 0; }
  void SetFlag(Flag flag, bool state) {
    if (state)
      flags_ |= flag;
    else
      flags_ &= ~flag;
  }

  std::u16string title_;
  std::u16string short_title_;
  unsigned flags_;
  int image_id_;
};

inline Action::Action(unsigned command,
                      CommandCategory category,
                      std::u16string title,
                      std::u16string short_title,
                      int image_id,
                      unsigned flags)
    : command_id_(command),
      category_(category),
      title_(std::move(title)),
      short_title_(std::move(short_title)),
      image_id_(image_id),
      flags_(flags) {}

class ActionBuilder {
 public:
  ActionBuilder(unsigned command_id,
                CommandCategory category,
                std::u16string title)
      : command_id_{command_id},
        category_{category},
        title_{std::move(title)} {}

  ActionBuilder& image_id(int image_id) {
    image_id_ = image_id;
    return *this;
  }

  ActionBuilder& shortcut(Shortcut shortcut) {
    shortcut_ = std::move(shortcut);
    return *this;
  }

  Action* Build() {
    auto* action =
        new Action(command_id_, category_, std::move(title_), {}, image_id_);
    action->shortcut_ = std::move(shortcut_);
    return action;
  }

 private:
  unsigned command_id_;
  CommandCategory category_;
  std::u16string title_;
  std::optional<Shortcut> shortcut_;
  int image_id_ = 0;
};