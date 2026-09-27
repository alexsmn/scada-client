#include "device_diagnostics/qt/device_diagnostics_selection_panel.h"

#include "aui/test/app_environment.h"
#include "controller/qt/selection_panel_registry.h"

#include <QWidget>

#include <memory>

#include <gtest/gtest.h>

namespace {

// The dock's object name is persisted in every page's saved dock state, so it
// must not change when the panel moves between owners: a renamed dock is one
// every existing layout loses track of.
TEST(DeviceDiagnosticsSelectionPanelTest,
     RegistersOneDockUnderItsPersistedName) {
  AppEnvironment app_env;
  SelectionPanelRegistry registry;
  RegisterDeviceDiagnosticsSelectionPanel(registry);
  ASSERT_EQ(registry.factories().size(), 1u);

  std::unique_ptr<SelectionPanel> panel =
      registry.factories().front()(SelectionPanelContext{});
  ASSERT_NE(panel, nullptr);
  EXPECT_EQ(panel->object_name(), "DeviceDiagnosticsDock");
  EXPECT_EQ(panel->title(), u"Device diagnostics");

  // No selection clears rather than crashing on the absent services.
  panel->ShowSelection(nullptr);
  delete &panel->widget();
}

}  // namespace
