// scada.client.properties — named C++20 module facade over the
// client_properties_qt headers.
//
// Same design and rules as scada.base (see core/base/scada_base.cppm and
// core/docs/cxx-modules.md): headers stay the source of truth, the global
// module fragment includes them, the purview re-exports names with
// `export using`. The `export import`s of scada.client.aui /
// scada.node_service mirror client_properties' PUBLIC links.
//
// Deliberately NOT in this facade:
//  - properties/transport/*: a separate, PRIVATE-linked library
//    (client_properties_transport); its headers are not part of this
//    facade's surface.
//
// Not exported (owned by other facades / other libraries):
//  - aui::TableColumn, aui::EditData (aui/models/*.h) and DialogService
//    (aui/dialog_service.h, forward-declared here): owned and exported by
//    scada.client.aui;
//  - NodeRef and the other node_service names: owned and exported by
//    scada.node_service (its keep-alive covers std::hash<NodeRef>);
//  - TaskManager (forward-declared in property_context.h): owned by
//    scada.client.services.
//
// ChoiceNone() (property_util.h) is a function, so it has external linkage and
// IS exported. It used to be an `extern const` for the same reason; it became a
// function when its text moved into the translation catalog, which a
// namespace-scope const cannot reach (no QApplication at static-init time).

module;

// ---- Global module fragment: headers stay the source of truth ----
#include "properties/channel_property_definition.h"
#include "properties/property_context.h"
#include "properties/property_definition.h"
#include "properties/property_defs.h"
#include "properties/property_service.h"
#include "properties/property_util.h"

export module scada.client.properties;

// Mirror client_properties' PUBLIC link transitivity.
export import scada.client.aui;
export import scada.node_service;

export {
  // channel_property_definition.h
  using ::ChannelPropertyDefinition;

  // property_context.h
  using ::PropertyContext;

  // property_definition.h
  using ::HierachicalPropertyDefinition;
  using ::PropertyDefinition;

  // property_defs.h
  using ::BoolPropertyDefinition;
  using ::ColorPropertyDefinition;
  using ::EnumPropertyDefinition;
  using ::PropertyValue;
  using ::ReferencePropertyDefinition;
  using ::TransportPropertyDefinition;

  // property_service.h
  using ::PropertyDefs;
  using ::PropertyService;

  // property_util.h
  using ::FindNodeByNameAndType;
  using ::ChoiceNone;
  using ::MakeAsyncChoiceHandler;
  using ::SetTextHelper;
}  // export
