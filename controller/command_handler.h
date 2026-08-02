#pragma once

#include "base/check.h"

#include <string>

class CommandHandler {
 public:
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
