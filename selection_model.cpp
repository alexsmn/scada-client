#include "selection_model.h"

#include "base/strings/sys_string_conversions.h"
#include "common/node_service.h"
#include "common/scada_node_ids.h"

SelectionModel::SelectionModel(NodeService& node_service,
                               TimedDataService& timed_data_service)
    : node_service_{node_service}, timed_data_service_{timed_data_service} {
  timed_data_.property_change_handler =
      [this](const rt::PropertySet& properties) {
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
  node_.Subscribe(*this);

  if (node.fetched() && node.node_class() == scada::NodeClass::Variable) {
    try {
      timed_data_.Connect(timed_data_service_, node.id());
    } catch (const std::exception&) {
      Clear();
      return;
    }

    type_ = SPEC;
  }

  Changed();
}

void SelectionModel::SelectNodeId(const scada::NodeId& node_id) {
  Clear();

  assert(!pending_request_);
  pending_request_ = std::make_shared<bool>();

  std::weak_ptr<bool> cancelation = pending_request_;
  node_service_.GetNode(node_id).Fetch(NodeFetchStatus::NodeOnly(),
                                       [=](const NodeRef& node) {
                                         if (cancelation.expired())
                                           return;
                                         pending_request_ = nullptr;
                                         if (node.status())
                                           SelectNode(node);
                                       });
}

void SelectionModel::SelectTimedData(const rt::TimedDataSpec& spec) {
  if (type_ == SPEC && timed_data_ == spec)
    return;

  Reset();

  type_ = SPEC;
  timed_data_ = spec;
  node_ = timed_data_.GetNode();
  if (node_)
    node_.Subscribe(*this);

  Changed();
}

void SelectionModel::SelectMultiple() {
  Reset();
  type_ = MULTI;
  Changed();
}

base::string16 SelectionModel::GetTitle() const {
  return timed_data_.GetTitle();
}

NodeIdSet SelectionModel::GetMultipleNodeIds() const {
  return multiple_handler ? multiple_handler() : NodeIdSet{};
}

void SelectionModel::Reset() {
  type_ = EMPTY;

  if (node_) {
    node_.Unsubscribe(*this);
    node_ = nullptr;
  }

  timed_data_.Reset();
  pending_request_ = nullptr;
}

void SelectionModel::Changed() {
  if (change_handler)
    change_handler();
}

void SelectionModel::OnNodeSemanticChanged(const scada::NodeId& node_id) {
  if (node_.id() == node_id)
    Changed();
}

void SelectionModel::OnModelChanged(const scada::ModelChangeEvent& event) {
  if ((event.verb & scada::ModelChangeEvent::NodeDeleted) &&
      (event.node_id == node_.id())) {
    Clear();
  }
}
