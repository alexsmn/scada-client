#pragma once

#include "controller/command_handler.h"

#include <functional>
#include <optional>
#include <ranges>
#include <string_view>
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

enum class MenuGroup { DISPLAY_SETTINGS };

// `C` represents the context type.
template <typename C>
class BasicCommand {
 public:
  using ExecuteHandler = UnaryFunction<void, C>;
  using EnabledHandler = UnaryFunction<bool, C>;
  using CheckedHandler = UnaryFunction<bool, C>;
  using AvailableHandler = UnaryFunction<bool, C>;

  BasicCommand() {}

  // Allow implicit conversion from `unsigned` to support
  // `registry.AddCommand(ID_XXX)`.
  BasicCommand(unsigned command_id) : command_id{command_id} {}

  BasicCommand& set_title(std::u16string_view title) {
    this->title = title;
    return *this;
  }

  BasicCommand& set_menu_group(MenuGroup group) {
    this->menu_group = group;
    return *this;
  }

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

  const unsigned command_id = 0;

  std::u16string title;
  std::optional<MenuGroup> menu_group;

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

  auto commands() { return command_map_ | std::views::values; }

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

inline unsigned CreateUniqueCommandId() {
  static unsigned next_command_id = 0xFFFF;
  return next_command_id++;
}

template <typename C>
inline BasicCommand<C>& BasicCommandRegistry<C>::AddCommand(
    BasicCommand<C> command) {
  auto command_id =
      command.command_id ? command.command_id : CreateUniqueCommandId();
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
