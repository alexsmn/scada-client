#pragma once

#include <cassert>

class CommandHandler {
 public:
  virtual CommandHandler* GetCommandHandler(unsigned command_id) {
    return this;
  }

  virtual bool IsCommandEnabled(unsigned command_id) const { return true; }

  virtual bool IsCommandChecked(unsigned command_id) const { return false; }

  virtual void ExecuteCommand(unsigned command_id) { assert(false); }
};
