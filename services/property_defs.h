#pragma once

#include <vector>

#include "base/strings/string16.h"
#include "common/node_ref.h"
#include "core/configuration_types.h"
#include "ui/base/models/grid_model.h"
#include "ui/base/models/table_column.h"

class NodeService;
class HierachicalPropertyDefinition;
class TaskManager;

typedef std::pair<base::string16, bool /*read_only*/> PropertyValue;

struct PropertyContext {
  NodeService& node_service_;
  TaskManager& task_manager_;
};

class PropertyDefinition {
 public:
  explicit PropertyDefinition(ui::TableColumn::Alignment alignment,
                              int width = 0);

  ui::TableColumn::Alignment alignment() const { return alignment_; }
  int width() const { return width_; }

  virtual bool IsReadOnly(const NodeRef& node,
                          const scada::NodeId& prop_decl_id) const;

  virtual base::string16 GetTitle(const PropertyContext& context,
                                  const NodeRef& property_declaration) const;

  virtual const HierachicalPropertyDefinition* AsHierarchical() const {
    return nullptr;
  }

  virtual base::string16 GetText(const PropertyContext& context,
                                 const NodeRef& node,
                                 const scada::NodeId& prop_decl_id) const;
  virtual void SetText(const PropertyContext& context,
                       const NodeRef& node,
                       const scada::NodeId& prop_decl_id,
                       const base::string16& text) const;
  virtual ui::EditData GetPropertyEditor(
      const PropertyContext& context,
      const NodeRef& type_definition,
      const scada::NodeId& prop_decl_id) const;

 private:
  ui::TableColumn::Alignment alignment_;
  int width_;
};

class HierachicalPropertyDefinition : public PropertyDefinition {
 public:
  typedef std::vector<const PropertyDefinition*> Children;

  explicit HierachicalPropertyDefinition(Children children)
      : children_(std::move(children)),
        PropertyDefinition(ui::TableColumn::LEFT) {}

  const Children& children() const { return children_; }

  virtual const HierachicalPropertyDefinition* AsHierarchical() const {
    return this;
  }

 private:
  Children children_;
};

class ReferencePropertyDefinition : public PropertyDefinition {
 public:
  ReferencePropertyDefinition() : PropertyDefinition(ui::TableColumn::LEFT) {}

  // PropertyDefinition
  virtual base::string16 GetText(
      const PropertyContext& context,
      const NodeRef& node,
      const scada::NodeId& prop_decl_id) const override;
  virtual void SetText(const PropertyContext& context,
                       const NodeRef& node,
                       const scada::NodeId& prop_decl_id,
                       const base::string16& text) const override;
  virtual ui::EditData GetPropertyEditor(
      const PropertyContext& context,
      const NodeRef& type_definition,
      const scada::NodeId& prop_decl_id) const override;
  virtual bool IsReadOnly(const NodeRef& node,
                          const scada::NodeId& prop_decl_id) const override;
};

class BoolPropertyDefinition : public PropertyDefinition {
 public:
  BoolPropertyDefinition() : PropertyDefinition(ui::TableColumn::CENTER) {}

  // PropertyDefinition
  virtual base::string16 GetText(
      const PropertyContext& context,
      const NodeRef& node,
      const scada::NodeId& prop_decl_id) const override;
  virtual ui::EditData GetPropertyEditor(
      const PropertyContext& context,
      const NodeRef& type_definition,
      const scada::NodeId& prop_decl_id) const override;
};

class EnumPropertyDefinition : public PropertyDefinition {
 public:
  EnumPropertyDefinition() : PropertyDefinition(ui::TableColumn::LEFT) {}

  // PropertyDefinition
  virtual base::string16 GetText(
      const PropertyContext& context,
      const NodeRef& node,
      const scada::NodeId& prop_decl_id) const override;
  virtual void SetText(const PropertyContext& context,
                       const NodeRef& node,
                       const scada::NodeId& prop_decl_id,
                       const base::string16& text) const override;
  virtual ui::EditData GetPropertyEditor(
      const PropertyContext& context,
      const NodeRef& type_definition,
      const scada::NodeId& prop_decl_id) const override;
};

class ChannelPropertyDefinition : public PropertyDefinition {
 public:
  ChannelPropertyDefinition(base::string16 title, bool device)
      : PropertyDefinition(ui::TableColumn::LEFT),
        title_(std::move(title)),
        device_(device) {}

  // PropertyDefinition
  virtual base::string16 GetTitle(
      const PropertyContext& context,
      const NodeRef& property_declaration) const override;
  virtual base::string16 GetText(
      const PropertyContext& context,
      const NodeRef& node,
      const scada::NodeId& prop_decl_id) const override;
  virtual void SetText(const PropertyContext& context,
                       const NodeRef& node,
                       const scada::NodeId& prop_decl_id,
                       const base::string16& text) const override;
  virtual ui::EditData GetPropertyEditor(
      const PropertyContext& context,
      const NodeRef& type_definition,
      const scada::NodeId& prop_decl_id) const override;

 private:
  base::string16 title_;
  bool device_;
};

class TransportPropertyDefinition : public PropertyDefinition {
 public:
  TransportPropertyDefinition() : PropertyDefinition(ui::TableColumn::LEFT) {}

  // PropertyDefinition
  virtual ui::EditData GetPropertyEditor(
      const PropertyContext& context,
      const NodeRef& type_definition,
      const scada::NodeId& prop_decl_id) const override;
};

class ColorPropertyDefinition : public PropertyDefinition {
 public:
  ColorPropertyDefinition() : PropertyDefinition(ui::TableColumn::LEFT) {}

  // PropertyDefinition
  virtual base::string16 GetText(
      const PropertyContext& context,
      const NodeRef& node,
      const scada::NodeId& prop_decl_id) const override;
  virtual void SetText(const PropertyContext& context,
                       const NodeRef& node,
                       const scada::NodeId& prop_decl_id,
                       const base::string16& text) const override;
  virtual ui::EditData GetPropertyEditor(
      const PropertyContext& context,
      const NodeRef& type_definition,
      const scada::NodeId& prop_decl_id) const override;
};

typedef std::vector<std::pair<NodeRef /*prop_decl*/, const PropertyDefinition*>>
    PropertyDefs;

const PropertyDefinition* GetPropertyDef(const scada::NodeId& prop_decl_id);
PropertyDefs GetTypeProperties(const NodeRef& type_definition);
