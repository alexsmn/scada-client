// Smoke test for the scada.client.core module facade: names come from
// `import scada.client.core;` only, including the surfaces re-exported
// through `export import scada.base`.

#include <gtest/gtest.h>

#include <type_traits>

// Import after the textual includes (AppleClang 21 libc++ merging bug).
import scada.client.core;

namespace scada_client_core_module {
namespace {

TEST(ScadaClientCoreModuleSmoke, ProgressAndCommandContexts) {
  ProgressStatus a;
  ProgressStatus b;
  EXPECT_TRUE(a == b);

  static_assert(std::is_class_v<CoreModule>);
  static_assert(std::is_class_v<GlobalCommandContext>);
  static_assert(std::is_class_v<NodeCommandContext>);
  static_assert(std::is_base_of_v<ProgressHost, ProgressHostImpl>);
}

TEST(ScadaClientCoreModuleSmoke, TransitiveSurfaces) {
  // scada.base via the export import chain.
  EXPECT_EQ(Format(7), "7");
  scada::base::Check(true, "client core module smoke");
}

}  // namespace
}  // namespace scada_client_core_module
