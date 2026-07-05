#include "controller/command_manager.h"

#include "base/check.h"

#include <algorithm>
#include <utility>

CommandManager::CommandManager() = default;

CommandManager::~CommandManager() = default;

CommandDescriptor& CommandManager::RegisterCommand(
    CommandDescriptor descriptor) {
  base::Check(descriptor.command_id != 0);

  const auto command_id = descriptor.command_id;
  auto [it, inserted] = command_map_.emplace(command_id, std::move(descriptor));
  if (!inserted) {
    return it->second;
  }

  commands_.push_back(&it->second);
  return it->second;
}

CommandDescriptor* CommandManager::FindCommand(unsigned command_id) const {
  auto it = command_map_.find(command_id);
  return it != command_map_.end() ? const_cast<CommandDescriptor*>(&it->second)
                                  : nullptr;
}

void CommandManager::RegisterHandler(unsigned command_id,
                                     CommandContextId context_id,
                                     CommandHandler& handler) {
  base::Check(FindCommand(command_id));
  registrations_.emplace(
      command_id,
      CommandRegistration{.context_id = context_id, .handler = &handler});
}

CommandHandler* CommandManager::ResolveHandler(
    unsigned command_id,
    std::span<const CommandContextId> active_contexts) const {
  auto registrations = registrations_.equal_range(command_id);
  for (auto context = active_contexts.rbegin();
       context != active_contexts.rend(); ++context) {
    auto it =
        std::find_if(registrations.first, registrations.second,
                     [context](const auto& registration) {
                       return registration.second.context_id == *context &&
                              registration.second.handler &&
                              registration.second.handler->GetCommandHandler(
                                  registration.first);
                     });
    if (it != registrations.second) {
      return it->second.handler->GetCommandHandler(command_id);
    }
  }
  return nullptr;
}

CommandDescriptor ToCommandDescriptor(const Action& action) {
  return CommandDescriptor{
      .command_id = action.command_id_,
      .category = action.category_,
      .title = action.title_,
      .short_title = action.short_title_,
      .image_id = action.image_id_,
      .flags = action.flags_,
      .shortcut = action.shortcut_,
      .title_provider = action.title_provider_,
  };
}

CommandHandler* ResolveCommandHandler(
    const CommandManager& command_manager,
    unsigned command_id,
    std::span<const CommandContextId> active_contexts,
    CommandHandler& fallback_handler) {
  if (auto* handler =
          command_manager.ResolveHandler(command_id, active_contexts)) {
    return handler;
  }
  return fallback_handler.GetCommandHandler(command_id);
}

GroupedCommandDescriptors GroupCommands(CommandManager& command_manager,
                                        const std::vector<unsigned>& commands) {
  GroupedCommandDescriptors grouped_commands;
  for (unsigned command_id : commands) {
    if (auto* command = command_manager.FindCommand(command_id)) {
      grouped_commands[command->category].push_back(command);
    }
  }
  return grouped_commands;
}
