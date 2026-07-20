// Smoke test for the scada.client.clipboard module facade: names come from
// `import scada.client.clipboard;` only, including the surfaces re-exported
// through `export import scada.node_service` (and transitively scada.common
// / scada.core / scada.base).

#include <gtest/gtest.h>

#include <type_traits>

// Import after the textual includes (AppleClang 21 libc++ merging bug).
import scada.client.clipboard;

namespace scada_client_clipboard_module {
namespace {

TEST(ScadaClientClipboardModuleSmoke, SerializationSurface) {
  static_assert(
      std::is_function_v<std::remove_pointer_t<decltype(&NodeToData)>>);
  static_assert(std::is_function_v<
                std::remove_pointer_t<decltype(&PasteNodesFromClipboard)>>);
}

TEST(ScadaClientClipboardModuleSmoke, TransitiveSurfaces) {
  // scada.common / scada.core / scada.base via the export import chain.
  scada::NodeState state;
  state.node_id = scada::NodeId{42, 7};
  EXPECT_FALSE(state.node_id.is_null());
  scada::base::Check(true, "client clipboard module smoke");
}

}  // namespace
}  // namespace scada_client_clipboard_module
