#pragma once

#include "command_handler.h"

#include <functional>
#include <unordered_map>

class Command {
 public:
  using ExecuteHandler = std::function<void()>;
  using EnabledHandler = std::function<bool()>;
  using CheckedHandler = std::function<bool()>;
  using AvailableHandler = std::function<bool()>;

  Command(unsigned command_id) : command_id{command_id} {}

  Command& set_execute_handler(ExecuteHandler handler) {
    execute_handler = std::move(handler);
    return *this;
  }

  Command& set_enabled_handler(EnabledHandler handler) {
    enabled_handler = std::move(handler);
    return *this;
  }

  Command& set_checked_handler(CheckedHandler handler) {
    checked_handler = std::move(handler);
    return *this;
  }

  Command& set_available_handler(AvailableHandler handler) {
    available_handler = std::move(handler);
    return *this;
  }

  const unsigned command_id;

  ExecuteHandler execute_handler;
  EnabledHandler enabled_handler;
  CheckedHandler checked_handler;
  AvailableHandler available_handler;
};

class CommandRegistry : private CommandHandler {
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

inline Command& CommandRegistry::AddCommand(Command command) {
  auto command_id = command.command_id;
  return command_map_.try_emplace(command_id, std::move(command)).first->second;
}

inline const Command* CommandRegistry::FindCommand(unsigned command_id) const {
  auto i = command_map_.find(command_id);
  return i != command_map_.end() ? &i->second : nullptr;
}

inline CommandHandler* CommandRegistry::GetCommandHandler(unsigned command_id) {
  auto* command = FindCommand(command_id);
  if (!command)
    return nullptr;
  bool available =
      command->available_handler ? command->available_handler() : true;
  return available ? this : nullptr;
}

inline bool CommandRegistry::IsCommandEnabled(unsigned command_id) const {
  auto* command = FindCommand(command_id);
  if (!command)
    return false;
  return command->enabled_handler ? command->enabled_handler() : true;
}

inline bool CommandRegistry::IsCommandChecked(unsigned command_id) const {
  auto* command = FindCommand(command_id);
  if (!command)
    return false;
  return command->checked_handler && command->checked_handler();
}

inline void CommandRegistry::ExecuteCommand(unsigned command_id) {
  auto* command = FindCommand(command_id);
  if (command)
    command->execute_handler();
}

inline Command* CommandRegistry::FindCommand(unsigned command_id) {
  auto i = command_map_.find(command_id);
  return i != command_map_.end() ? &i->second : nullptr;
}
