#pragma once

#include "controller/action.h"
#include "controller/command_handler.h"

#include <map>
#include <optional>
#include <span>
#include <vector>

enum class CommandContextId {
  Global,
  Selection,
  OpenedView,
  Controller,
};

struct CommandDescriptor {
  unsigned command_id = 0;
  CommandCategory category = CATEGORY_SPECIFIC;
  std::u16string title;
  std::u16string short_title;
  int image_id = 0;
  unsigned flags = 0;
  std::optional<Shortcut> shortcut;
  std::function<std::u16string()> title_provider;
  // Controls whether generic toolbar builders include this command.
  bool show_in_toolbar = true;
  // Controls whether generic context menu builders include this command.
  bool show_in_context_menu = true;

  std::u16string GetTitle() const {
    return title_provider ? title_provider() : title;
  }

  std::u16string GetShortTitle() const {
    return short_title.empty() ? title : short_title;
  }

  bool checkable() const { return (flags & Action::CHECKABLE) != 0; }
};

struct CommandRegistration {
  CommandContextId context_id = CommandContextId::Global;
  CommandHandler* handler = nullptr;
};

class CommandManager {
 public:
  using CommandMap = std::map<unsigned, CommandDescriptor>;
  using CommandList = std::vector<CommandDescriptor*>;

  CommandManager();
  ~CommandManager();

  CommandManager(const CommandManager&) = delete;
  CommandManager& operator=(const CommandManager&) = delete;

  const CommandList& commands() const { return commands_; }

  CommandDescriptor& RegisterCommand(CommandDescriptor descriptor);
  CommandDescriptor* FindCommand(unsigned command_id) const;

  void RegisterHandler(unsigned command_id,
                       CommandContextId context_id,
                       CommandHandler& handler);

  CommandHandler* ResolveHandler(
      unsigned command_id,
      std::span<const CommandContextId> active_contexts) const;

 private:
  CommandMap command_map_;
  CommandList commands_;
  std::multimap<unsigned, CommandRegistration> registrations_;
};

CommandDescriptor ToCommandDescriptor(const Action& action);

// Resolves a command handler through registered contexts, falling back to an
// aggregate command router while legacy command registrations are still used.
CommandHandler* ResolveCommandHandler(
    const CommandManager& command_manager,
    unsigned command_id,
    std::span<const CommandContextId> active_contexts,
    CommandHandler& fallback_handler);

using CommandDescriptorList = std::vector<CommandDescriptor*>;
using GroupedCommandDescriptors =
    std::map<CommandCategory, CommandDescriptorList>;

GroupedCommandDescriptors GroupCommands(CommandManager& command_manager,
                                        const std::vector<unsigned>& commands);
