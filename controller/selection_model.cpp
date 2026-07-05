#include "controller/selection_model.h"

#include "common/formula_util.h"
#include "controller/controller.h"
#include "model/scada_node_ids.h"
#include "node_service/node_service.h"
#include "timed_data/timed_data_property.h"

SelectionModel::SelectionModel(SelectionModelContext&& context)
    : SelectionModelContext{std::move(context)} {
  timed_data_.property_change_handler = [this](const PropertySet& properties) {
    if (properties.is_item_changed())
      Clear();
  };
}

SelectionModel::~SelectionModel() {
  Reset();
}

void SelectionModel::Clear() {
  if (type_ == EMPTY)
    return;
  Reset();
  Changed();
}

void SelectionModel::SelectNode(const NodeRef& node) {
  if (!node) {
    Clear();
    return;
  }

  if (node_ == node)
    return;

  Reset();

  type_ = NODE;
  node_ = node;
  SubscribeNode();

  if (node.node_class() == scada::NodeClass::Variable) {
    type_ = SPEC;
    timed_data_.Connect(timed_data_service_, node);
  }

  Changed();
}

void SelectionModel::SelectTimedData(const TimedDataSpec& spec) {
  if (type_ == SPEC && timed_data_ == spec)
    return;

  Reset();

  type_ = SPEC;
  timed_data_ = spec;
  timed_data_.SetCurrentOnly();
  node_ = timed_data_.node();
  if (node_)
    SubscribeNode();

  Changed();
}

void SelectionModel::SelectMultiple() {
  Reset();
  type_ = MULTI;
  Changed();
}

std::u16string SelectionModel::GetTitle() const {
  return timed_data_.GetTitle();
}

NodeIdSet SelectionModel::GetMultipleNodeIds() const {
  return multiple_handler ? multiple_handler() : NodeIdSet{};
}

void SelectionModel::Reset() {
  type_ = EMPTY;

  if (node_) {
    node_semantic_changed_connection_.disconnect();
    model_changed_connection_.disconnect();
    node_ = nullptr;
  }

  timed_data_.Reset();
}

void SelectionModel::SubscribeNode() {
  node_semantic_changed_connection_ = node_.SubscribeNodeSemanticChanged(
      [this](const scada::NodeId& node_id) { OnNodeSemanticChanged(node_id); });
  model_changed_connection_ = node_.SubscribeModelChanged(
      [this](const scada::ModelChangeEvent& event) { OnModelChanged(event); });
}

void SelectionModel::Changed() {
  if (change_handler)
    change_handler();
}

void SelectionModel::OnNodeSemanticChanged(const scada::NodeId& node_id) {
  if (node_.node_id() == node_id)
    Changed();
}

void SelectionModel::OnModelChanged(const scada::ModelChangeEvent& event) {
  if ((event.verb & scada::ModelChangeEvent::NodeDeleted) &&
      (event.node_id == node_.node_id())) {
    Clear();
  }
}
