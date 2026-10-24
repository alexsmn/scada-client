#pragma once

#include "scada/node_id.h"
#include "properties/property_definition.h"

class ChannelPropertyDefinition : public PropertyDefinition {
 public:
  // `title` is the English source, translated when read: the definitions are
  // namespace-scope constants built before any catalog is installed, so a
  // title composed at construction stayed English (backlog 916).
  ChannelPropertyDefinition(const char* title, bool device)
      : PropertyDefinition(scada::aui::TableColumn::LEFT),
        title_(title),
        device_(device) {}

  // PropertyDefinition
  virtual std::u16string GetTitle(
      const PropertyContext& context,
      const NodeRef& property_declaration) const override;
  virtual std::u16string GetText(
      const PropertyContext& context,
      const NodeRef& node,
      const scada::NodeId& prop_decl_id) const override;
  virtual void SetText(const PropertyContext& context,
                       const NodeRef& node,
                       const scada::NodeId& prop_decl_id,
                       const std::u16string& text) const override;
  virtual scada::aui::EditData GetPropertyEditor(
      const PropertyContext& context,
      const NodeRef& node,
      const scada::NodeId& prop_decl_id) const override;
  virtual void GetAdditionalTargets(
      const NodeRef& node,
      const scada::NodeId& prop_decl_id,
      std::vector<scada::NodeId>& targets) const override;

  static const char16_t kParentGroupDevice[];

 private:
  static scada::NodeId GetDeviceId(const NodeRef& node,
                                   const scada::NodeId& prop_decl_id);

  const char* const title_;
  const bool device_;
};
