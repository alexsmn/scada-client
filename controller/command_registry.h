#pragma once

#include "command_handler.h"

#include <functional>
#include <unordered_map>

template <typename R, typename A>
struct UnaryFunctionImpl {
  using Type = std::function<R(const A&)>;
};

template <typename R>
struct UnaryFunctionImpl<R, void> {
  using Type = std::function<R()>;
};

// Provides a zero-parameter function when the argument is void.
template <typename R, typename A>
using UnaryFunction = typename UnaryFunctionImpl<R, A>::Type;

// `C` represents the context type.
template <typename C>
class BasicCommand {
 public:
  using ExecuteHandler = UnaryFunction<void, C>;
  using EnabledHandler = UnaryFunction<bool, C>;
  using CheckedHandler = UnaryFunction<bool, C>;
  using AvailableHandler = UnaryFunction<bool, C>;

  // Allow implicit conversion from `unsigned` to support
  // `registry.AddCommand(ID_XXX)`.
  BasicCommand(unsigned command_id) : command_id{command_id} {}

  BasicCommand& set_execute_handler(ExecuteHandler handler) {
    execute_handler = std::move(handler);
    return *this;
  }

  BasicCommand& set_enabled_handler(EnabledHandler handler) {
    enabled_handler = std::move(handler);
    return *this;
  }

  BasicCommand& set_checked_handler(CheckedHandler handler) {
    checked_handler = std::move(handler);
    return *this;
  }

  BasicCommand& set_available_handler(AvailableHandler handler) {
    available_handler = std::move(handler);
    return *this;
  }

  const unsigned command_id;

  ExecuteHandler execute_handler;
  EnabledHandler enabled_handler;
  CheckedHandler checked_handler;
  AvailableHandler available_handler;
};

using Command = BasicCommand<void>;

// `C` represents the context type.
template <typename C>
class BasicCommandRegistry {
 public:
  BasicCommand<C>& AddCommand(BasicCommand<C> command);

  BasicCommand<C>* FindCommand(unsigned command_id);
  const BasicCommand<C>* FindCommand(unsigned command_id) const;

 protected:
  std::unordered_map<unsigned /*command_id*/, BasicCommand<C>> command_map_;
};

// Introduce a separate class to allow forward declaration.
class CommandRegistry : public CommandHandler,
                        public BasicCommandRegistry<void> {
 public:
  // CommandHandler
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual bool IsCommandEnabled(unsigned command_id) const override;
  virtual bool IsCommandChecked(unsigned command_id) const override;
  virtual void ExecuteCommand(unsigned command_id) override;
};

template <typename C>
inline BasicCommand<C>& BasicCommandRegistry<C>::AddCommand(
    BasicCommand<C> command) {
  auto command_id = command.command_id;
  return command_map_.try_emplace(command_id, std::move(command)).first->second;
}

template <typename C>
inline const BasicCommand<C>* BasicCommandRegistry<C>::FindCommand(
    unsigned command_id) const {
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

template <typename C>
inline BasicCommand<C>* BasicCommandRegistry<C>::FindCommand(
    unsigned command_id) {
  auto i = command_map_.find(command_id);
  return i != command_map_.end() ? &i->second : nullptr;
}
