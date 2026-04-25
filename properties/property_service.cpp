#include "properties/property_service.h"

#include "base/awaitable_promise.h"
#include "base/executor_conversions.h"
#include "base/range_util.h"
#include "base/u16format.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/scada_node_ids.h"
#include "node_service/node_promises.h"
#include "node_service/node_util.h"
#include "properties/channel_property_definition.h"
#include "properties/property_defs.h"

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <stdexcept>

namespace {

const PropertyDefinition kNamePropDef(aui::TableColumn::LEFT, 150);
const PropertyDefinition kStringPropDef(aui::TableColumn::LEFT);
const PropertyDefinition kIntPropDef(aui::TableColumn::RIGHT);
const PropertyDefinition kDoublePropDef(aui::TableColumn::RIGHT);
const BoolPropertyDefinition kBoolPropDef;
const ReferencePropertyDefinition kRefPropDef;
const ColorPropertyDefinition kColorPropDef;
const EnumPropertyDefinition kEnumPropDef;

struct ChannelPropertyTree {
  explicit ChannelPropertyTree(std::u16string_view suffix)
      : device{u16format(L"Device{}", suffix), true},
        channel{u16format(L"Channel{}", suffix), false} {}

  ChannelPropertyDefinition device;
  ChannelPropertyDefinition channel;

  HierachicalPropertyDefinition root{{&device, &channel}};
};

const ChannelPropertyTree kObjectInput1PropTree{u""};
const ChannelPropertyTree kObjectInput2PropTree{u" (backup)"};
const ChannelPropertyTree kObjectOutputPropTree{u" (control)"};

const TransportPropertyDefinition kLinkTransportPropDef;

const std::unordered_map<scada::NodeId, const PropertyDefinition*>
    kPropertyDefinitionMap = {
        {data_items::id::DataItemType_Input1, &kObjectInput1PropTree.root},
        {data_items::id::DataItemType_Input2, &kObjectInput2PropTree.root},
        {data_items::id::DataItemType_Output, &kObjectOutputPropTree.root},
        {devices::id::LinkType_Transport, &kLinkTransportPropDef},
        {data_items::id::TsFormatType_OpenColor, &kColorPropDef},
        {data_items::id::TsFormatType_CloseColor, &kColorPropDef},
};

void CollectCreates(const NodeRef& node,
                    std::unordered_set<NodeRef>& child_type_definitions) {
  for (auto&& creates : node.targets(scada::id::Creates))
    child_type_definitions.emplace(std::move(creates));
}

// Returns unfetched type definitions.
std::unordered_set<NodeRef> GetChildTypeDefinitions(
    const NodeRef& parent_node) {
  assert(parent_node.fetched());
  assert(parent_node.type_definition().fetched());

  std::unordered_set<NodeRef> child_type_definitions;

  CollectCreates(parent_node, child_type_definitions);

  for (auto node_type = parent_node.type_definition(); node_type;
       node_type = node_type.supertype()) {
    CollectCreates(node_type, child_type_definitions);
  }

  return child_type_definitions;
}

Awaitable<PropertyDefs> GetChildPropertyDefsCompatAsync(
    std::shared_ptr<PropertyService> service,
    std::optional<AnyExecutor> configured_executor,
    NodeRef parent_node) {
  if (!configured_executor) {
    throw std::logic_error{"PropertyService executor is not configured"};
  }

  co_return co_await service->GetChildPropertyDefsAsync(*configured_executor,
                                                        parent_node);
}

}  // namespace

// PropertyService

PropertyService::PropertyService(AnyExecutor executor)
    : executor_{std::move(executor)} {}

Awaitable<void> PropertyService::GetAllSubtypesPropertiesAsync(
    AnyExecutor executor,
    const NodeRef& type_definition,
    const std::shared_ptr<std::unordered_set<NodeRef>>& property_decls) {
  co_await AwaitPromise(executor, FetchNode(type_definition));

  GetTypeProperties(type_definition, *property_decls);

  for (const auto& subtype : type_definition.targets(scada::id::HasSubtype)) {
    co_await GetAllSubtypesPropertiesAsync(executor, subtype, property_decls);
  }
}

const PropertyDefinition* PropertyService::GetPropertyDef(
    const NodeRef& prop_decl) {
  if (auto i = kPropertyDefinitionMap.find(prop_decl.node_id());
      i != kPropertyDefinitionMap.end()) {
    return i->second;
  }

  if (prop_decl.node_class() == scada::NodeClass::ReferenceType)
    return &kRefPropDef;

  static const scada::NumericId kIntDataTypeIds[] = {
      scada::id::Int8,  scada::id::UInt8,  scada::id::Int16, scada::id::UInt16,
      scada::id::Int32, scada::id::UInt32, scada::id::Int64, scada::id::UInt64};

  auto data_type = prop_decl.data_type();

  if (IsSubtypeOf(data_type, scada::id::Boolean))
    return &kBoolPropDef;
  if (IsSubtypeOf(data_type, scada::id::Double))
    return &kIntPropDef;
  if (IsSubtypeOf(data_type, scada::id::String) ||
      IsSubtypeOf(data_type, scada::id::LocalizedText))
    return &kStringPropDef;
  if (IsSubtypeOf(data_type, scada::id::Enumeration))
    return &kEnumPropDef;

  // NOTE: Enums are subtypes of INT.
  for (const auto data_type_id : kIntDataTypeIds) {
    if (IsSubtypeOf(data_type, data_type_id))
      return &kIntPropDef;
  }

  return nullptr;
}

void PropertyService::GetTypeProperties(
    const NodeRef& type_definition,
    std::unordered_set<NodeRef>& property_decls) {
  assert(type_definition.fetched());
  for (auto supertype_definition = type_definition; supertype_definition;
       supertype_definition = supertype_definition.supertype()) {
    for (const auto& p : supertype_definition.targets(scada::id::HasProperty))
      property_decls.emplace(p);
    for (const auto& r : supertype_definition.references(
             scada::id::NonHierarchicalReferences)) {
      // TODO: Introduce common base reference type.
      if (!IsSubtypeOf(r.reference_type, scada::id::Creates) &&
          !IsSubtypeOf(r.reference_type, scada::id::HasSubtype)) {
        property_decls.emplace(r.reference_type);
      }
    }
  }
}

PropertyDefs PropertyService::GetTypePropertyDefs(
    const NodeRef& type_definition) {
  assert(type_definition.fetched());

  std::unordered_set<NodeRef> prop_decls;
  GetTypeProperties(type_definition, prop_decls);

  PropertyDefs properties;
  properties.reserve(prop_decls.size());

  for (auto& prop_decl : prop_decls) {
    if (auto* def = GetPropertyDef(prop_decl))
      properties.emplace_back(prop_decl, def);
  }
  return properties;
}

promise<PropertyDefs> PropertyService::GetChildPropertyDefs(
    const NodeRef& parent_node) {
  auto configured_executor = executor_;
  auto executor =
      configured_executor ? *configured_executor : MakeThreadAnyExecutor();
  auto service = std::make_shared<PropertyService>(*this);
  return ToPromise(executor, GetChildPropertyDefsCompatAsync(
                                 std::move(service), configured_executor,
                                 parent_node));
}

Awaitable<PropertyDefs> PropertyService::GetChildPropertyDefsAsync(
    AnyExecutor executor,
    const NodeRef& parent_node) {
  auto property_decls = std::make_shared<std::unordered_set<NodeRef>>();

  co_await AwaitPromise(executor, FetchNode(parent_node));
  auto child_type_definitions = GetChildTypeDefinitions(parent_node);

  for (const auto& child_type_definition : child_type_definitions) {
    co_await GetAllSubtypesPropertiesAsync(executor, child_type_definition,
                                           property_decls);
  }

  co_return GetPropertyDefs(*property_decls);
}

PropertyDefs PropertyService::GetPropertyDefs(
    const std::unordered_set<NodeRef>& property_decls) {
  PropertyDefs property_defs =
      property_decls |
      boost::adaptors::transformed([this](const NodeRef& property_decl) {
        return std::make_pair(property_decl, GetPropertyDef(property_decl));
      }) |
      boost::adaptors::filtered([](const auto& p) { return !!p.second; }) |
      to_vector;
  std::ranges::sort(property_defs);
  return property_defs;
}
