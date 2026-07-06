// Smoke test for the scada.client.aui module facade: names come from
// `import scada.client.aui;` only, including the surfaces re-exported
// through `export import scada.base` / `export import scada.core`.

#include <gtest/gtest.h>

#include <type_traits>

// Import after the textual includes (AppleClang 21 libc++ merging bug).
import scada.client.aui;

namespace scada_client_aui_module {
namespace {

TEST(ScadaClientAuiModuleSmoke, GeometryAndColors) {
  aui::Rgba black{0, 0, 0};
  EXPECT_EQ(black, aui::Rgba(0, 0, 0));
  EXPECT_GT(aui::GetColorCount(), 0u);

  static_assert(std::is_class_v<aui::Size>);
  static_assert(std::is_class_v<aui::GridModel>);
  static_assert(std::is_class_v<aui::SimpleMenuModel>);
  static_assert(std::is_class_v<DialogService>);
}

TEST(ScadaClientAuiModuleSmoke, TransitiveSurfaces) {
  // scada.core / scada.base via the export import chain.
  EXPECT_FALSE(scada::NodeId(42, 7).is_null());
  base::Check(true, "aui module smoke");
}

}  // namespace
}  // namespace scada_client_aui_module
