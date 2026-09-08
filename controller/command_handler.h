#pragma once

#include "base/check.h"

#include <string>

// A polymorphic command sink. Routers own their handlers through this base
// (`std::unique_ptr<CommandHandler>`), so the destructor must be virtual:
// without it every per-view command router was deleted through the base on
// tab close, its derived destructor never ran, and each command — with the
// `Cancelation` guarding its in-flight coroutines — leaked for the life of
// the process.
class CommandHandler {
 public:
  virtual ~CommandHandler() = default;

  virtual CommandHandler* GetCommandHandler(unsigned command_id) {
    return this;
  }

  virtual bool IsCommandEnabled(unsigned command_id) const { return true; }

  // Why |command_id| is currently disabled, for a greyed menu entry that would
  // otherwise leave the operator guessing. Empty when the command is enabled
  // or supplies no reason. Only commands that resolve a handler can answer —
  // a command whose availability gate fails is not offered at all.
  virtual std::u16string GetCommandDisabledReason(unsigned command_id) const {
    return {};
  }

  virtual bool IsCommandChecked(unsigned command_id) const { return false; }

  virtual void ExecuteCommand(unsigned command_id) {
    scada::base::NotReached();
  }
};
