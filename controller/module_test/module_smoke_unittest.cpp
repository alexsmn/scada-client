// Smoke test for the scada.client.controller module facade: names come from
// `import scada.client.controller;` only, including the surfaces re-exported
// through the five export imports (scada.client.aui, scada.client.base,
// scada.client.profile, scada.node_service, scada.timed_data).

#include <gtest/gtest.h>

#include <type_traits>

// Import after the textual includes (AppleClang 21 libc++ merging bug).
import scada.client.controller;

namespace scada_client_controller_module {
namespace {

TEST(ScadaClientControllerModuleSmoke, ControllerSurface) {
  NodeIdSet ids = MakeNodeIdSet(scada::NodeId{42, 7});
  EXPECT_EQ(ids.size(), 1u);

  static_assert(std::is_class_v<Controller>);
  static_assert(std::is_class_v<CommandRegistry>);
  static_assert(std::is_class_v<ControllerRegistry>);
  static_assert(std::is_class_v<WindowInfo>);
}

TEST(ScadaClientControllerModuleSmoke, TransitiveSurfaces) {
  // scada.client.profile / scada.client.base / scada.client.aui via the
  // export import chain.
  static_assert(std::is_class_v<WindowDefinition>);
  static_assert(std::is_class_v<scada::RelativeTimeRange>);
  static_assert(std::is_class_v<scada::aui::SimpleMenuModel>);
  scada::base::Check(true, "client controller module smoke");
}

}  // namespace
}  // namespace scada_client_controller_module
