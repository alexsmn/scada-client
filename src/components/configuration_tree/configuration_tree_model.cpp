#include "components/configuration_tree/configuration_tree_model.h"

#include "base/strings/sys_string_conversions.h"
#include "common/browse_util.h"
#include "common/node_service.h"
#include "translation.h"

namespace {

// Callback = void(const scada::NodeId& parent_id)
template <class Callback>
void BrowseMultipleParentIdsHelper(
    scada::ViewService& view_service,
    const scada::NodeId& node_id,
    const std::shared_ptr<const std::vector<scada::NodeId>>&
        shared_reference_type_ids,
    size_t from_index,
    const Callback& callback) {
  BrowseParentId(view_service, node_id,
                 shared_reference_type_ids->at(from_index),
                 [=, &view_service](const scada::NodeId& parent_id) {
                   if (!parent_id.is_null())
                     callback(parent_id);
                   else
                     BrowseMultipleParentIdsHelper(view_service, node_id,
                                                   shared_reference_type_ids,
                                                   from_index + 1, callback);
                 });
}

// Callback = void(const scada::NodeId& parent_id)
template <class Callback>
void BrowseMultipleParentIds(
    scada::ViewService& view_service,
    const scada::NodeId& node_id,
    const std::vector<scada::NodeId>& reference_type_ids,
    const Callback& callback) {
  BrowseMultipleParentIdsHelper(
      view_service, node_id,
      std::make_shared<const std::vector<scada::NodeId>>(reference_type_ids), 0,
      callback);
}

}  // namespace

// ConfigurationTreeNode

ConfigurationTreeNode::ConfigurationTreeNode(ConfigurationTreeModel& model,
                                             NodeRef data_node)
    : model_(model) {
  SetDataNode(std::move(data_node));
}

ConfigurationTreeNode::~ConfigurationTreeNode() {
  SetDataNode(nullptr);
}

void ConfigurationTreeNode::SetDataNode(NodeRef data_node) {
  if (data_node_ == data_node)
    return;

  if (data_node_) {
    // TODO: Remove all.
    model_.node_map_.erase(data_node_.id());
  }

  data_node_ = std::move(data_node);

  if (data_node_) {
    assert(model_.node_map_.find(data_node_.id()) == model_.node_map_.end());
    model_.node_map_[data_node_.id()] = this;
  }
}

void ConfigurationTreeNode::BrowseChildren() {
  if (!data_node_)
    return;
  if (children_)
    return;

  children_ = true;

  model_.Browse(data_node_);
}

int ConfigurationTreeNode::GetChildCount() const {
  const_cast<ConfigurationTreeNode*>(this)->BrowseChildren();
  return ui::TreeNode<ConfigurationTreeNode>::GetChildCount();
}

base::string16 ConfigurationTreeNode::GetText(int column_id) const {
  return ToString16(data_node_.display_name());
}

int ConfigurationTreeNode::GetIcon() const {
  return (data_node_ && data_node_.fetched() &&
          data_node_.node_class() == scada::NodeClass::Variable)
             ? IMAGE_ITEM
             : IMAGE_FOLDER;
}

void ConfigurationTreeNode::OnNodeSemanticChanged() {
  Changed();
}

void ConfigurationTreeNode::Changed() {
  model().TreeNodeChanged(this);
}

// ConfigurationTreeModel

ConfigurationTreeModel::ConfigurationTreeModel(
    scada::ViewService& view_service,
    NodeService& node_service,
    const scada::NodeId& root_id,
    std::vector<scada::NodeId> reference_type_ids)
    : view_service_{view_service},
      node_service_{node_service},
      reference_type_ids_{std::move(reference_type_ids)} {
  node_service_.Subscribe(*this);

  set_root(std::make_unique<ConfigurationTreeNode>(
      *this, node_service.GetNode(root_id)));
}

ConfigurationTreeModel::~ConfigurationTreeModel() {
  set_root(nullptr);

  node_service_.Unsubscribe(*this);
}

std::unique_ptr<ConfigurationTreeNode> ConfigurationTreeModel::CreateNode(
    const NodeRef& data_node) {
  return std::make_unique<ConfigurationTreeNode>(*this, data_node);
}

ConfigurationTreeNode* ConfigurationTreeModel::FindNode(
    const scada::NodeId& node_id) {
  auto i = node_map_.find(node_id);
  return i != node_map_.end() ? i->second : nullptr;
}

void ConfigurationTreeModel::EnsureParent(const scada::NodeId& node_id) {
  if (!FindNode(node_id))
    return;

  BrowseMultipleParentIds(
      view_service_, node_id, reference_type_ids_,
      [=](const scada::NodeId& parent_id) {
        auto* node_ptr = FindNode(node_id);
        if (!node_ptr)
          return;

        auto* supposed_parent = FindNode(parent_id);
        if (supposed_parent == node_ptr->parent())
          return;

        // Remove node from old place.
        std::unique_ptr<ConfigurationTreeNode> node;
        if (ConfigurationTreeNode* n = FindNode(node_id))
          node = Remove(*n->parent(), n->parent()->IndexOfChild(*n));

        // Insert into new place.
        if (!supposed_parent)
          return;

        if (node)
          Add(*supposed_parent, supposed_parent->GetChildCount(),
              std::move(node));
        else
          EnsureNode(node_id);
      });
}

void ConfigurationTreeModel::EnsureNode(const scada::NodeId& node_id) {
  auto weak_ptr = weak_ptr_factory_.GetWeakPtr();
  BrowseMultipleParentIds(
      view_service_, node_id, reference_type_ids_,
      [weak_ptr, this, node_id](const scada::NodeId& parent_id) {
        if (!weak_ptr.get() || parent_id.is_null())
          return;
        EnsureNode2(node_id, parent_id);
      });
}

void ConfigurationTreeModel::EnsureNode2(const scada::NodeId& node_id,
                                         const scada::NodeId& parent_id) {
  if (FindNode(node_id))
    return;

  auto node = node_service_.GetNode(node_id);
  if (!node)
    return;

  if (auto* parent_node = FindNode(parent_id)) {
    if (auto tree_node = CreateNode(node))
      Add(*parent_node, parent_node->GetChildCount(), std::move(tree_node));
  }
}

void ConfigurationTreeModel::OnModelChange(const ModelChangeEvent& event) {
  if (event.verb & ModelChangeEvent::NodeDeleted) {
    if (ConfigurationTreeNode* node = FindNode(event.node_id))
      Remove(*node->parent(), node->parent()->IndexOfChild(*node));

  } else if (event.verb & ModelChangeEvent::NodeAdded) {
    EnsureNode(event.node_id);

  } else if (event.verb & (ModelChangeEvent::ReferenceAdded |
                           ModelChangeEvent::ReferenceDeleted)) {
    EnsureParent(event.node_id);
  }
}

void ConfigurationTreeModel::OnNodeSemanticChanged(
    const scada::NodeId& node_id) {
  if (ConfigurationTreeNode* tree_node = FindNode(node_id))
    tree_node->OnNodeSemanticChanged();
}

void ConfigurationTreeModel::Browse(const NodeRef& node) {
  auto weak_ptr = weak_ptr_factory_.GetWeakPtr();
  auto node_id = node.id();

  std::vector<scada::BrowseDescription> descriptions;
  descriptions.reserve(reference_type_ids_.size());
  for (auto& reference_type_id : reference_type_ids_) {
    descriptions.push_back(
        {node_id, scada::BrowseDirection::Forward, reference_type_id, true});
  }

  view_service_.Browse(
      descriptions,
      [this, weak_ptr, node_id](const scada::Status& status,
                                std::vector<scada::BrowseResult>&& results) {
        auto ptr = weak_ptr.get();
        if (!ptr)
          return;

        for (auto& result : results)
          OnBrowseComplete(node_id, std::move(result.references));
      });
}

void ConfigurationTreeModel::OnBrowseComplete(
    const scada::NodeId& node_id,
    const scada::ReferenceDescriptions& references) {
  for (auto& reference : references)
    EnsureNode2(reference.node_id, node_id);
}
