#pragma once

#include "core/node_command_context.h"

#include <cassert>
#include <functional>
#include <vector>

// Registry of handlers for the default action on a node, such as opening a
// file or opening a data item in its default view.
class DefaultNodeCommandRegistry {
 public:
  using Handler = std::function<bool(const NodeCommandContext& context)>;

  // Registers a handler. Handlers are evaluated in registration order until
  // one returns true.
  void AddHandler(Handler handler) {
    assert(handler);
    handlers_.push_back(std::move(handler));
  }

  // Executes the first handler that accepts `context`.
  bool Execute(const NodeCommandContext& context) const {
    for (const auto& handler : handlers_) {
      if (handler(context)) {
        return true;
      }
    }
    return false;
  }

 private:
  std::vector<Handler> handlers_;
};
