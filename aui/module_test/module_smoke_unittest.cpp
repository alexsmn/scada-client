// Smoke test for the scada.client.aui module facade: names come from
// `import scada.client.aui;` only, including the surfaces re-exported
// through `export import scada.base`.

#include <gtest/gtest.h>

#include <type_traits>

// Import after the textual includes (AppleClang 21 libc++ merging bug).
import scada.client.aui;

namespace scada_client_aui_module {
namespace {

TEST(ScadaClientAuiModuleSmoke, GeometryAndColors) {
  scada::aui::Rgba black{0, 0, 0};
  EXPECT_EQ(black, scada::aui::Rgba(0, 0, 0));
  EXPECT_GT(scada::aui::GetColorCount(), 0u);

  static_assert(std::is_class_v<scada::aui::Size>);
  static_assert(std::is_class_v<scada::aui::GridModel>);
  static_assert(std::is_class_v<scada::aui::SimpleMenuModel>);
  static_assert(std::is_class_v<DialogService>);
}

TEST(ScadaClientAuiModuleSmoke, TransitiveSurfaces) {
  // scada.base via the export import chain.
  scada::base::Check(true, "aui module smoke");
}

}  // namespace
}  // namespace scada_client_aui_module
