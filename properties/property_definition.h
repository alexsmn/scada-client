#pragma once

#include "aui/models/edit_data.h"
#include "aui/models/table_column.h"
#include "base/lifetime.h"

namespace scada {
class NodeId;
}

class NodeRef;
struct PropertyContext;

class HierachicalPropertyDefinition;

class PropertyDefinition {
 public:
  explicit PropertyDefinition(scada::aui::TableColumn::Alignment alignment,
                              int width = 0);
  virtual ~PropertyDefinition() = default;

  scada::aui::TableColumn::Alignment alignment() const { return alignment_; }
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

  virtual scada::aui::EditData GetPropertyEditor(
      const PropertyContext& context,
      const NodeRef& node,
      const scada::NodeId& prop_decl_id) const;

  // Triggered when `GetPropertyEditor` returns button editor and the button is
  // clicked.
  virtual void HandleEditButton(const PropertyContext& context,
                                const NodeRef& node,
                                const scada::NodeId& prop_decl_id) const;

  virtual void GetAdditionalTargets(const NodeRef& node,
                                    const scada::NodeId& prop_decl_id,
                                    std::vector<scada::NodeId>& targets) const;

 private:
  const scada::aui::TableColumn::Alignment alignment_;
  const int width_;
};

class HierachicalPropertyDefinition : public PropertyDefinition {
 public:
  using Children = std::vector<const PropertyDefinition*>;

  explicit HierachicalPropertyDefinition(Children children)
      : PropertyDefinition(scada::aui::TableColumn::LEFT),
        children_(std::move(children)) {}

  const Children& children() const SCADA_LIFETIME_BOUND { return children_; }

  virtual const HierachicalPropertyDefinition* AsHierarchical() const {
    return this;
  }

 private:
  Children children_;
};
