// Smoke test for the scada.client.profile module facade: names come from
// `import scada.client.profile;` only, including the surfaces re-exported
// through `export import scada.client.aui`.

#include <gtest/gtest.h>

#include <type_traits>

// Import after the textual includes (AppleClang 21 libc++ merging bug).
import scada.client.profile;

namespace scada_client_profile_module {
namespace {

TEST(ScadaClientProfileModuleSmoke, ProfileSurface) {
  static_assert(std::is_class_v<Profile>);
  static_assert(std::is_class_v<Page>);
  static_assert(std::is_class_v<PageLayout>);
  static_assert(std::is_class_v<WindowDefinition>);
}

TEST(ScadaClientProfileModuleSmoke, TransitiveSurfaces) {
  // scada.client.aui / scada.core / scada.base via the export import chain.
  static_assert(std::is_class_v<aui::TableColumn>);
  EXPECT_FALSE(scada::NodeId(42, 7).is_null());
  base::Check(true, "client profile module smoke");
}

}  // namespace
}  // namespace scada_client_profile_module
