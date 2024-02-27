#pragma once

#include "aui/key_codes.h"

#include <functional>
#include <memory>

class Executor;
class NodeRef;
class MainWindow;

struct NodeCommandContext {
  MainWindow* main_window;
  const NodeRef& node;
  aui::KeyModifiers key_modifiers;
};

using NodeCommandHandler =
    std::function<void(const NodeCommandContext& context)>;
