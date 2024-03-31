#pragma once

#include "aui/key_codes.h"

#include <functional>
#include <memory>

class Executor;
class DialogService;
class NodeRef;
class MainWindowInterface;

struct NodeCommandContext {
  MainWindowInterface* main_window;
  DialogService& dialog_service;
  const NodeRef& node;
  aui::KeyModifiers key_modifiers;
};

using NodeCommandHandler =
    std::function<void(const NodeCommandContext& context)>;
