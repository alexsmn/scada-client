#pragma once

#include "command_handler.h"

#include <functional>
#include <unordered_map>

class Command {
 public:
  using ExecuteHandler = std::function<void()>;
  using EnabledHandler = std::function<bool()>;
  using CheckedHandler = std::function<bool()>;

  Command(unsigned command_id) : command_id{command_id} {}

  const unsigned command_id;

  ExecuteHandler execute_handler;
  EnabledHandler enabled_handler;
  CheckedHandler checked_handler;
};

class CommandHandlerImpl : private CommandHandler {
 public:
  Command& AddCommand(Command command);

  Command* FindCommand(unsigned command_id);
  const Command* FindCommand(unsigned command_id) const;

  // CommandHandler
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;

 private:
  // CommandHandler
  virtual bool IsCommandEnabled(unsigned command_id) const override;
  virtual bool IsCommandChecked(unsigned command_id) const override;
  virtual void ExecuteCommand(unsigned command_id) override;

  std::unordered_map<unsigned /*command_id*/, Command> command_map_;
};

inline Command& CommandHandlerImpl::AddCommand(Command command) {
  auto command_id = command.command_id;
  return command_map_.try_emplace(command_id, std::move(command)).first->second;
}

inline const Command* CommandHandlerImpl::FindCommand(
    unsigned command_id) const {
  auto i = command_map_.find(command_id);
  return i != command_map_.end() ? &i->second : nullptr;
}

inline CommandHandler* CommandHandlerImpl::GetCommandHandler(
    unsigned command_id) {
  return FindCommand(command_id) ? this : nullptr;
}

inline bool CommandHandlerImpl::IsCommandEnabled(unsigned command_id) const {
  auto* command = FindCommand(command_id);
  return command && (!command->enabled_handler || command->enabled_handler());
}

inline bool CommandHandlerImpl::IsCommandChecked(unsigned command_id) const {
  auto* command = FindCommand(command_id);
  return command && command->checked_handler && command->checked_handler();
}

inline void CommandHandlerImpl::ExecuteCommand(unsigned command_id) {
  auto* command = FindCommand(command_id);
  if (command)
    command->execute_handler();
}

inline Command* CommandHandlerImpl::FindCommand(unsigned command_id) {
  auto i = command_map_.find(command_id);
  return i != command_map_.end() ? &i->second : nullptr;
}
