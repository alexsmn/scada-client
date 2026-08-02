#pragma once

#include "controller/controller.h"
#include "controller/controller_context.h"

#include <memory>

class WindowDefinition;

// The Administration explorer pane's controller.
//
// It navigates and nothing else: each section resolves to a global command the
// shell already owns, so opening "Users" here is the same code path as the
// menu entry. That is what keeps the rail's promise — a mode switches panes,
// and the pane's rows open views — without this module reimplementing any of
// the admin surfaces it lists.
class AdministrationView final : protected ControllerContext,
                                 public Controller {
 public:
  explicit AdministrationView(const ControllerContext& context);
  ~AdministrationView() override;

  // Controller
  std::unique_ptr<UiView> Init(const WindowDefinition& definition) override;
};
