#pragma once

#include "properties/property_definition.h"

using PropertyValue = std::pair<std::u16string, bool /*read_only*/>;

class ReferencePropertyDefinition : public PropertyDefinition {
 public:
  ReferencePropertyDefinition()
      : PropertyDefinition(scada::aui::TableColumn::LEFT) {}

  // PropertyDefinition
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
  virtual bool IsReadOnly(const NodeRef& node,
                          const scada::NodeId& prop_decl_id) const override;
};

class BoolPropertyDefinition : public PropertyDefinition {
 public:
  BoolPropertyDefinition()
      : PropertyDefinition(scada::aui::TableColumn::CENTER) {}

  // PropertyDefinition
  virtual std::u16string GetText(
      const PropertyContext& context,
      const NodeRef& node,
      const scada::NodeId& prop_decl_id) const override;
  virtual scada::aui::EditData GetPropertyEditor(
      const PropertyContext& context,
      const NodeRef& node,
      const scada::NodeId& prop_decl_id) const override;
};

class EnumPropertyDefinition : public PropertyDefinition {
 public:
  EnumPropertyDefinition()
      : PropertyDefinition(scada::aui::TableColumn::LEFT) {}

  // PropertyDefinition
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
};

class TransportPropertyDefinition : public PropertyDefinition {
 public:
  TransportPropertyDefinition()
      : PropertyDefinition(scada::aui::TableColumn::LEFT) {}

  // PropertyDefinition
  virtual scada::aui::EditData GetPropertyEditor(
      const PropertyContext& context,
      const NodeRef& node,
      const scada::NodeId& prop_decl_id) const override;
  virtual void HandleEditButton(
      const PropertyContext& context,
      const NodeRef& node,
      const scada::NodeId& prop_decl_id) const override;
};

class ColorPropertyDefinition : public PropertyDefinition {
 public:
  ColorPropertyDefinition()
      : PropertyDefinition(scada::aui::TableColumn::LEFT) {}

  // PropertyDefinition
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
};
