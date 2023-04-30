#pragma once

#include "base/promise.h"
#include "common/node_state.h"
#include "controls/models/grid_model.h"
#include "controls/models/table_column.h"
#include "node_service/node_ref.h"

#include <string>
#include <unordered_set>
#include <vector>

class DialogService;
class NodeService;
class HierachicalPropertyDefinition;
class TaskManager;

typedef std::pair<std::u16string, bool /*read_only*/> PropertyValue;

struct PropertyContext {
  NodeService& node_service_;
  TaskManager& task_manager_;
  DialogService& dialog_service_;
};

class PropertyDefinition {
 public:
  explicit PropertyDefinition(aui::TableColumn::Alignment alignment,
                              int width = 0);

  aui::TableColumn::Alignment alignment() const { return alignment_; }
  int width() const { return width_; }

  virtual bool IsReadOnly(const NodeRef& node,
                          const scada::NodeId& prop_decl_id) const;

  virtual std::u16string GetTitle(const PropertyContext& context,
                                  const NodeRef& property_declaration) const;

  virtual const HierachicalPropertyDefinition* AsHierarchical() const {
    return nullptr;
  }

  virtual std::u16string GetText(const PropertyContext& context,
                                 const NodeRef& node,
                                 const scada::NodeId& prop_decl_id) const;
  virtual void SetText(const PropertyContext& context,
                       const NodeRef& node,
                       const scada::NodeId& prop_decl_id,
                       const std::u16string& text) const;

  virtual aui::EditData GetPropertyEditor(
      const PropertyContext& context,
      const NodeRef& node,
      const scada::NodeId& prop_decl_id) const;

  // Triggered when `GetPropertyEditor` returns button editor and the button is
  // clicked.
  virtual void HandleEditButton(const PropertyContext& context,
                                const NodeRef& node,
                                const scada::NodeId& prop_decl_id) const;

 private:
  aui::TableColumn::Alignment alignment_;
  int width_;
};

class HierachicalPropertyDefinition : public PropertyDefinition {
 public:
  typedef std::vector<const PropertyDefinition*> Children;

  explicit HierachicalPropertyDefinition(Children children)
      : children_(std::move(children)),
        PropertyDefinition(aui::TableColumn::LEFT) {}

  const Children& children() const { return children_; }

  virtual const HierachicalPropertyDefinition* AsHierarchical() const {
    return this;
  }

 private:
  Children children_;
};

class ReferencePropertyDefinition : public PropertyDefinition {
 public:
  ReferencePropertyDefinition() : PropertyDefinition(aui::TableColumn::LEFT) {}

  // PropertyDefinition
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
  virtual bool IsReadOnly(const NodeRef& node,
                          const scada::NodeId& prop_decl_id) const override;
};

class BoolPropertyDefinition : public PropertyDefinition {
 public:
  BoolPropertyDefinition() : PropertyDefinition(aui::TableColumn::CENTER) {}

  // PropertyDefinition
  virtual std::u16string GetText(
      const PropertyContext& context,
      const NodeRef& node,
      const scada::NodeId& prop_decl_id) const override;
  virtual aui::EditData GetPropertyEditor(
      const PropertyContext& context,
      const NodeRef& node,
      const scada::NodeId& prop_decl_id) const override;
};

class EnumPropertyDefinition : public PropertyDefinition {
 public:
  EnumPropertyDefinition() : PropertyDefinition(aui::TableColumn::LEFT) {}

  // PropertyDefinition
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
};

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

  static const char16_t kParentGroupDevice[];

 private:
  const std::u16string title_;
  const bool device_;
};

class TransportPropertyDefinition : public PropertyDefinition {
 public:
  TransportPropertyDefinition() : PropertyDefinition(aui::TableColumn::LEFT) {}

  // PropertyDefinition
  virtual aui::EditData GetPropertyEditor(
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
  ColorPropertyDefinition() : PropertyDefinition(aui::TableColumn::LEFT) {}

  // PropertyDefinition
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
};

typedef std::vector<std::pair<NodeRef /*prop_decl*/, const PropertyDefinition*>>
    PropertyDefs;

const PropertyDefinition* GetPropertyDef(const NodeRef& prop_decl);

PropertyDefs GetTypePropertyDefs(const NodeRef& type_definition);
promise<PropertyDefs> GetChildPropertyDefs(const NodeRef& parent_node);

void GetTypeProperties(const NodeRef& type_definition,
                       std::unordered_set<NodeRef>& property_declarations);
