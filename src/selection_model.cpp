#include "client/selection_model.h"

#include "base/strings/sys_string_conversions.h"
#include "common/scada_node_ids.h"
#include "common/node_ref_service.h"

SelectionModel::SelectionModel(NodeRefService& node_service, TimedDataService& timed_data_service)
    : node_service_{node_service},
      timed_data_service_{timed_data_service} {
  timed_data_.set_delegate(this);
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

void SelectionModel::SelectNode(NodeRef node) {
  if (!node) {
    Clear();
    return;
  }

  if (node_ == node)
    return;

  Reset();

	type_ = NODE;
	node_ = node;
  node_service_.AddObserver(*this);

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
  node_service_.RequestNode(node_id, [=](const scada::Status& status, const NodeRef& node) {
    if (cancelation.expired())
      return;
    pending_request_ = nullptr;
    if (node)
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
    node_service_.AddObserver(*this);

	Changed();
}

void SelectionModel::SelectMultiple() {
  Reset();
	type_ = MULTI;
	Changed();
}

void SelectionModel::OnPropertyChanged(rt::TimedDataSpec& spec,
                                  const rt::PropertySet& properties) {
  assert(&spec == &timed_data_);
  
  if (properties.is_item_changed())
    Clear();
}

base::string16 SelectionModel::GetTitle() const {
  return timed_data_.GetTitle();
}

NodeIdSet SelectionModel::GetMultipleNodeIds() const {
  return multiple_handler_ ? multiple_handler_() : NodeIdSet();
}

void SelectionModel::Reset() {
	type_ = EMPTY;

  if (node_) {
    node_service_.RemoveObserver(*this);
	  node_ = nullptr;
  }

	timed_data_.Reset();
  pending_request_ = nullptr;
}

void SelectionModel::Changed() {
  if (change_handler_)
    change_handler_();
}

void SelectionModel::OnNodeSemanticChanged(const scada::NodeId& node_id) {
  if (node_.id() == node_id)
    Changed();
}

void SelectionModel::OnNodeDeleted(const scada::NodeId& node_id) {
  if (node_.id() == node_id)
    Clear();
}
