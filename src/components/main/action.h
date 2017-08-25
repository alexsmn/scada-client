#pragma once

#include "base/strings/string16.h"
#include "core/node_id.h"

#include <vector>
#include <map>

class NodeRefService;

enum CommandCategory {
  CATEGORY_NEW,
  CATEGORY_OPEN,
  CATEGORY_ITEM,
  CATEGORY_DEVICE,
  CATEGORY_SETUP,
  CATEGORY_SPECIFIC,
  CATEGORY_VIEW,
  CATEGORY_PERIOD,
  CATEGORY_CREATE,
  CATEGORY_EDIT,
};

class Action {
 public:
  enum Flag { ENABLED = 0x0001, CHECKED = 0x0002, VISIBLE = 0x0004,
              ALWAYS_VISIBLE = 0x0008 };

  Action(unsigned command_id, CommandCategory category, base::string16 title,
         base::string16 short_title = base::string16(), int image_id = 0,
         unsigned flags = 0);
  virtual ~Action() {}

  void set_enabled(bool enabled) { SetFlag(ENABLED, enabled); }
  void set_checked(bool checked) { SetFlag(CHECKED, checked); }
  void set_visible(bool visible) { SetFlag(VISIBLE, visible); }

  virtual base::string16 GetTitle() const { return title_; }

  unsigned command_id() const { return command_id_; }
  base::string16 GetShortTitle() const { return short_title_.empty() ? title_ : short_title_; }
  int image_id() const { return image_id_; }
  bool enabled() const { return GetFlag(ENABLED); }
  bool checked() const { return GetFlag(CHECKED); }
  bool visible() const { return GetFlag(VISIBLE); }
  bool always_visible() const { return GetFlag(ALWAYS_VISIBLE); }

  unsigned command_id_;
  CommandCategory		category_;

 private:
  bool GetFlag(Flag flag) const { return (flags_ & flag) != 0; }
  void SetFlag(Flag flag, bool state) { if (state) flags_ |= flag; else flags_ &= ~flag; }

  base::string16 title_;
  base::string16 short_title_;
  unsigned flags_;
  int image_id_;

  DISALLOW_COPY_AND_ASSIGN(Action);
};

typedef std::vector<Action*> ActionList;

class ActionManager {
 public:
  typedef std::map<unsigned, Action*> ActionMap;

  explicit ActionManager(NodeRefService& node_service);
  ~ActionManager();

  const ActionList& actions() const { return actions_; }

  Action* FindAction(unsigned command) const;

 private:
  void AddAction(Action& action);

  ActionMap   action_map_;
  ActionList  actions_;

  DISALLOW_COPY_AND_ASSIGN(ActionManager);
};

typedef std::map<CommandCategory, ActionList> GroupedActions;

GroupedActions GroupCommands(ActionManager& action_manager, const std::vector<unsigned>& commands);

const base::char16* GetCommandCategoryTitle(CommandCategory category);
bool CanExpandCommandCategory(CommandCategory category);

scada::NodeId GetNewCommandTypeId(unsigned command_id);
