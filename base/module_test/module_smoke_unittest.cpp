// Smoke test for the scada.client.base module facade: names come from
// `import scada.client.base;` only, including the surfaces re-exported
// through `export import scada.base`.

#include <gtest/gtest.h>

#include <type_traits>

// Import after the textual includes (AppleClang 21 libc++ merging bug).
import scada.client.base;

namespace scada_client_base_module {
namespace {

TEST(ScadaClientBaseModuleSmoke, SettingsStoresAndUrls) {
  MemorySettingsStore store;
  store.Clear();
  EXPECT_FALSE(IsWebUrl(u""));

  static_assert(std::is_class_v<FileSettingsStore>);
  static_assert(std::is_class_v<Blinker>);
  static_assert(std::is_class_v<TimeRange>);
}

TEST(ScadaClientBaseModuleSmoke, TransitiveSurfaces) {
  // scada.base via the export import chain.
  EXPECT_EQ(Format(3), "3");
  base::Check(true, "client base module smoke");
}

}  // namespace
}  // namespace scada_client_base_module
