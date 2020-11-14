#pragma once

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
         std::wstring title,
         std::wstring short_title = std::wstring(),
         int image_id = 0,
         unsigned flags = 0);
  virtual ~Action() {}

  Action(const Action&) = delete;
  Action& operator=(const Action&) = delete;

  void set_enabled(bool enabled) { SetFlag(ENABLED, enabled); }
  void set_checkable(bool checkable) { SetFlag(CHECKABLE, checkable); }
  void set_checked(bool checked) { SetFlag(CHECKED, checked); }
  void set_visible(bool visible) { SetFlag(VISIBLE, visible); }

  virtual std::wstring GetTitle() const { return title_; }

  unsigned command_id() const { return command_id_; }

  std::wstring GetShortTitle() const {
    return short_title_.empty() ? title_ : short_title_;
  }
  void SetTitle(std::wstring title) { title_ = std::move(title); }

  int image_id() const { return image_id_; }
  bool enabled() const { return GetFlag(ENABLED); }
  bool checkable() const { return GetFlag(CHECKABLE); }
  bool checked() const { return GetFlag(CHECKED); }
  bool visible() const { return GetFlag(VISIBLE); }
  bool always_visible() const { return GetFlag(ALWAYS_VISIBLE); }

  unsigned command_id_;
  CommandCategory category_;

 private:
  bool GetFlag(Flag flag) const { return (flags_ & flag) != 0; }
  void SetFlag(Flag flag, bool state) {
    if (state)
      flags_ |= flag;
    else
      flags_ &= ~flag;
  }

  std::wstring title_;
  std::wstring short_title_;
  unsigned flags_;
  int image_id_;
};

inline Action::Action(unsigned command,
                      CommandCategory category,
                      std::wstring title,
                      std::wstring short_title,
                      int image_id,
                      unsigned flags)
    : command_id_(command),
      category_(category),
      title_(std::move(title)),
      short_title_(std::move(short_title)),
      image_id_(image_id),
      flags_(flags) {}
