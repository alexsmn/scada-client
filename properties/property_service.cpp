#include "properties/property_service.h"

#include "base/any_executor.h"
#include "base/check.h"
#include "base/range_util.h"
#include "base/u16format.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/scada_node_ids.h"
#include "node_service/node_awaitable.h"
#include "node_service/node_util.h"
#include "properties/channel_property_definition.h"
#include "properties/property_defs.h"
#include "scada/co_result.h"

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <stdexcept>

namespace {

const PropertyDefinition kNamePropDef(scada::aui::TableColumn::LEFT, 150);
const PropertyDefinition kStringPropDef(scada::aui::TableColumn::LEFT);
const PropertyDefinition kIntPropDef(scada::aui::TableColumn::RIGHT);
const PropertyDefinition kDoublePropDef(scada::aui::TableColumn::RIGHT);
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
        {scada::data_items::id::DataItemType_Input1,
         &kObjectInput1PropTree.root},
        {scada::data_items::id::DataItemType_Input2,
         &kObjectInput2PropTree.root},
        {scada::data_items::id::DataItemType_Output,
         &kObjectOutputPropTree.root},
        {scada::devices::id::LinkType_Transport, &kLinkTransportPropDef},
        {scada::data_items::id::TsFormatType_OpenColor, &kColorPropDef},
        {scada::data_items::id::TsFormatType_CloseColor, &kColorPropDef},
};

// Returns unfetched type definitions.
std::unordered_set<NodeRef> GetChildTypeDefinitions(
    const NodeRef& parent_node) {
  scada::base::Check(parent_node.fetched());
  scada::base::Check(parent_node.type_definition().fetched());

  std::unordered_set<NodeRef> child_type_definitions;
  for (auto&& type_definition : GetCreatableChildTypes(parent_node))
    child_type_definitions.emplace(std::move(type_definition));
  return child_type_definitions;
}

// True when `type_definition` and its whole supertype chain are resident
// (fetched) right now. The v3 registry holds node models in a bounded MRU
// (NodeServiceImpl::TouchKeepAlive); a model fetched earlier in a coroutine
// can be evicted — and its fetched() state dropped — while later fetches
// suspend, so fetched-ness must be re-verified before any synchronous walk.
bool IsTypeChainResident(const NodeRef& type_definition) {
  for (auto type = type_definition; type; type = type.supertype()) {
    if (!type.fetched())
      return false;
  }
  return true;
}

// Fetches `type_definition`'s supertype chain and re-verifies it is still
// resident after the awaits (see IsTypeChainResident). On success the
// caller's synchronous walk must run with no suspension point in between —
// eviction only happens on the service executor, so verified state cannot
// thrash mid-walk.
scada::CoStatus FetchTypeChainResident(NodeRef type_definition) {
  constexpr int kMaxResidencyAttempts = 3;
  for (int attempt = 0; attempt < kMaxResidencyAttempts; ++attempt) {
    if (auto status = co_await FetchTypeChainStatus(type_definition); !status)
      co_return status;
    if (IsTypeChainResident(type_definition))
      co_return scada::StatusCode::Good;
  }
  co_return scada::Status{scada::StatusCode::Bad_ObjectIsBusy};
}

// Fetches `parent_node` plus its type chain and re-verifies both stayed
// resident, so GetChildTypeDefinitions can be called immediately after
// without tripping its residency preconditions.
scada::CoStatus FetchParentAndTypeChainResident(NodeRef parent_node) {
  constexpr int kMaxResidencyAttempts = 3;
  for (int attempt = 0; attempt < kMaxResidencyAttempts; ++attempt) {
    if (auto status = co_await FetchNodeStatus(parent_node); !status)
      co_return status;
    if (auto status =
            co_await FetchTypeChainResident(parent_node.type_definition());
        !status) {
      co_return status;
    }
    if (parent_node.fetched() && parent_node.type_definition() &&
        IsTypeChainResident(parent_node.type_definition())) {
      co_return scada::StatusCode::Good;
    }
  }
  co_return scada::Status{scada::StatusCode::Bad_ObjectIsBusy};
}

}  // namespace

// PropertyService

Awaitable<void> PropertyService::GetAllSubtypesPropertiesAsync(
    AnyExecutor executor,
    const NodeRef& type_definition,
    const std::shared_ptr<std::unordered_set<NodeRef>>& property_decls) {
  // Skip on failure rather than walking: GetTypeProperties fail-stops on an
  // unfetched (possibly keep-alive-evicted) chain.
  if (auto status = co_await FetchTypeChainResident(type_definition); !status)
    co_return;

  GetTypeProperties(type_definition, *property_decls);

  for (const auto& subtype : type_definition.targets(scada::id::HasSubtype)) {
    co_await GetAllSubtypesPropertiesAsync(executor, subtype, property_decls);
  }
}

scada::CoStatus PropertyService::GetAllSubtypesPropertiesStatusAsync(
    AnyExecutor executor,
    const NodeRef& type_definition,
    const std::shared_ptr<std::unordered_set<NodeRef>>& property_decls) {
  auto status = co_await FetchTypeChainResident(type_definition);
  if (!status) {
    co_return status;
  }

  GetTypeProperties(type_definition, *property_decls);

  for (const auto& subtype : type_definition.targets(scada::id::HasSubtype)) {
    status = co_await GetAllSubtypesPropertiesStatusAsync(executor, subtype,
                                                          property_decls);
    if (!status) {
      co_return status;
    }
  }
  co_return scada::StatusCode::Good;
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
  scada::base::Check(type_definition.fetched());
  for (auto supertype_definition = type_definition; supertype_definition;
       supertype_definition = supertype_definition.supertype()) {
    for (const auto& p : supertype_definition.targets(scada::id::HasProperty))
      property_decls.emplace(p);
    for (const auto& r : supertype_definition.references(
             scada::id::NonHierarchicalReferences)) {
      if (!IsSubtypeOf(r.reference_type, scada::id::HasSubtype)) {
        property_decls.emplace(r.reference_type);
      }
    }
  }
}

PropertyDefs PropertyService::GetTypePropertyDefs(
    const NodeRef& type_definition) {
  scada::base::Check(type_definition.fetched());

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

Awaitable<PropertyDefs> PropertyService::GetChildPropertyDefsAsync(
    AnyExecutor executor,
    const NodeRef& parent_node) {
  auto property_decls = std::make_shared<std::unordered_set<NodeRef>>();

  // Bail out empty on failure rather than walking: GetChildTypeDefinitions
  // fail-stops on an unfetched (possibly keep-alive-evicted) parent or chain.
  if (auto status = co_await FetchParentAndTypeChainResident(parent_node);
      !status) {
    co_return PropertyDefs{};
  }
  auto child_type_definitions = GetChildTypeDefinitions(parent_node);

  for (const auto& child_type_definition : child_type_definitions) {
    co_await GetAllSubtypesPropertiesAsync(executor, child_type_definition,
                                           property_decls);
  }

  co_return GetPropertyDefs(*property_decls);
}

scada::CoStatusOr<PropertyDefs>
PropertyService::GetChildPropertyDefsStatusAsync(AnyExecutor executor,
                                                 const NodeRef& parent_node) {
  auto property_decls = std::make_shared<std::unordered_set<NodeRef>>();

  auto status = co_await FetchParentAndTypeChainResident(parent_node);
  if (!status) {
    co_return status;
  }
  auto child_type_definitions = GetChildTypeDefinitions(parent_node);

  for (const auto& child_type_definition : child_type_definitions) {
    status = co_await GetAllSubtypesPropertiesStatusAsync(
        executor, child_type_definition, property_decls);
    if (!status) {
      co_return status;
    }
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
