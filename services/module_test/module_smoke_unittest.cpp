// Smoke test for the scada.client.services module facade: names come from
// `import scada.client.services;` only, including the surfaces re-exported
// through `export import scada.client.aui` / `export import scada.common`.

#include <gtest/gtest.h>

#include <type_traits>

// Import after the textual includes (AppleClang 21 libc++ merging bug).
import scada.client.services;

namespace scada_client_services_module {
namespace {

TEST(ScadaClientServicesModuleSmoke, ServicesSurface) {
  DeviceState state = DeviceState::Online;
  EXPECT_NE(state, DeviceState::Offline);

  static_assert(std::is_class_v<TaskManagerImpl>);
  static_assert(std::is_class_v<Speech>);
  static_assert(std::is_class_v<TelemetryClient>);
  static_assert(std::is_base_of_v<TaskManager, TaskManagerImpl>);
}

TEST(ScadaClientServicesModuleSmoke, TransitiveSurfaces) {
  // scada.common / scada.client.aui via the export import chain.
  scada::NodeProperties properties;
  EXPECT_EQ(scada::FindProperty(properties, scada::NodeId{1, 0}), nullptr);
  static_assert(std::is_class_v<aui::SimpleMenuModel>);
  base::Check(true, "client services module smoke");
}

}  // namespace
}  // namespace scada_client_services_module
