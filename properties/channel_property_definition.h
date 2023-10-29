#pragma once

#include "scada/node_id.h"
#include "properties/property_definition.h"

class ChannelPropertyDefinition : public PropertyDefinition {
 public:
  ChannelPropertyDefinition(std::u16string title, bool device)
      : PropertyDefinition(aui::TableColumn::LEFT),
        title_(std::move(title)),
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
  virtual aui::EditData GetPropertyEditor(
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

  const std::u16string title_;
  const bool device_;
};
