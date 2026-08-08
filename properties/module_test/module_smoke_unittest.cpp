// Smoke test for the scada.client.properties module facade: names come from
// `import scada.client.properties;` only, including the surfaces re-exported
// through `export import scada.client.aui` / `export import
// scada.node_service`.

#include <gtest/gtest.h>

#include <type_traits>

// Import after the textual includes (AppleClang 21 libc++ merging bug).
import scada.client.properties;

namespace scada_client_properties_module {
namespace {

TEST(ScadaClientPropertiesModuleSmoke, PropertiesSurface) {
  EXPECT_FALSE(ChoiceNone().empty());

  static_assert(std::is_class_v<PropertyService>);
  static_assert(std::is_class_v<PropertyContext>);
  static_assert(
      std::is_base_of_v<PropertyDefinition, ChannelPropertyDefinition>);
}

TEST(ScadaClientPropertiesModuleSmoke, TransitiveSurfaces) {
  // scada.client.aui / scada.core / scada.base via the export import chain.
  static_assert(std::is_class_v<scada::aui::TableColumn>);
  EXPECT_FALSE(scada::NodeId(42, 7).is_null());
  scada::base::Check(true, "client properties module smoke");
}

}  // namespace
}  // namespace scada_client_properties_module
