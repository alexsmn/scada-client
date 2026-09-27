#pragma once

#include "base/any_executor.h"
#include "base/lifetime.h"

#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>

class CommandHandler;
class NodeRef;
class NodeService;
class QWidget;
class SelectionModel;

namespace scada {
class AttributeService;
class NodeId;
}  // namespace scada

// What a main window hands each selection panel it creates. Every field but
// `executor` and `resolve_command` is optional and null in minimal and test
// contexts; a panel that needs one it did not get degrades as its module
// decides, never by dereferencing it.
struct SelectionPanelContext {
  AnyExecutor executor;
  // The handler the window would run `command_id` through against the active
  // view's selection, or null. Resolve at the moment of use: the active view
  // can change or close between a panel being built and a button being
  // clicked.
  std::function<CommandHandler*(unsigned command_id)> resolve_command;
  NodeService* node_service = nullptr;
  // Raw attribute reads, for attributes NodeService does not fetch.
  scada::AttributeService* attribute_service = nullptr;
  // Calls an OPC UA Method on a node, reporting through the task manager.
  std::function<void(const NodeRef& node, const scada::NodeId& method_id)>
      call_node_method;
  // Whether the session holds the OPC UA Call permission; null means "assume
  // it does".
  std::function<bool()> has_call_permission;
};

// A specialist panel in the right dock that follows the active view's
// selection: it fills for the selections its module understands and clears
// for everything else. The module that owns the panel decides which those
// are, which is the whole point — the main window only docks it and forwards
// the selection.
class SelectionPanel {
 public:
  virtual ~SelectionPanel() = default;

  // The dock's object name. It is persisted in each page's saved dock state,
  // so it must never change once shipped.
  virtual std::string object_name() const = 0;
  // The dock's translated title.
  virtual std::u16string title() const = 0;
  // The panel's widget. The window docks it, and Qt parenting then owns it;
  // this object must not delete it.
  virtual QWidget& widget() = 0;
  // Shows `selection`, or clears for a null one (no active view, or a view
  // with no selection model). Called on every selection change.
  virtual void ShowSelection(const SelectionModel* selection) = 0;
};

// Builds one panel for one main window, or returns null to contribute none.
using SelectionPanelFactory = std::function<std::unique_ptr<SelectionPanel>(
    const SelectionPanelContext& context)>;

// The selection panels feature modules contribute to the main window's right
// dock, in registration order — which is the order their tabs appear after
// the Inspector. The sibling of ControllerRegistry (workspace views) and
// UiCommandRegistry (commands): a module registers what it contributes, and
// the shell builds it without naming the module.
class SelectionPanelRegistry {
 public:
  void Register(SelectionPanelFactory factory) {
    factories_.push_back(std::move(factory));
  }

  std::span<const SelectionPanelFactory> factories() const
      SCADA_LIFETIME_BOUND {
    return factories_;
  }

 private:
  std::vector<SelectionPanelFactory> factories_;
};
