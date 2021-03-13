#include "components/configuration_tree/node_service_tree_impl.h"

#include "base/bind_util.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "core/event.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"

NodeServiceTreeImpl::NodeServiceTreeImpl(NodeServiceTreeImplContext&& context)
    : NodeServiceTreeImplContext{std::move(context)} {
  node_service_.Subscribe(*this);
}

NodeServiceTreeImpl ::~NodeServiceTreeImpl() {
  node_service_.Unsubscribe(*this);
}

NodeRef NodeServiceTreeImpl::GetRoot() const {
  return root_node_;
}

bool NodeServiceTreeImpl::HasChildren(const NodeRef& node) const {
  return true;
}

std::vector<NodeServiceTreeImpl::ChildRef> NodeServiceTreeImpl::GetChildren(
    const NodeRef& node) const {
  std::vector<ChildRef> children;

  for (const auto& [reference_type_id, forward] : reference_filter_) {
    const auto& targets = forward ? node.targets(reference_type_id)
                                  : node.inverse_targets(reference_type_id);
    for (const auto& child_node : targets) {
      if (IsMatchingNode(child_node))
        children.emplace_back(ChildRef{reference_type_id, forward, child_node});
    }
  }

  return children;
}

bool NodeServiceTreeImpl::IsMatchingNode(const NodeRef& node) const {
  assert(node);

  if (!type_definition_ids_.empty()) {
    bool matches = false;
    const auto& type_definition = node.type_definition();
    for (auto& filter_type_definition_id : type_definition_ids_) {
      if (IsSubtypeOf(type_definition, filter_type_definition_id)) {
        matches = true;
        break;
      }
    }
    if (!matches)
      return false;
  }

  return true;
}

void NodeServiceTreeImpl::SetObserver(Observer* observer) {
  observer_ = observer;
}

void NodeServiceTreeImpl::OnModelChanged(const scada::ModelChangeEvent& event) {
  auto task_runner = base::SequencedTaskRunnerHandle::Get();
  task_runner->PostTask(
      FROM_HERE, BindLambda([this, event] {
        if (!observer_)
          return;

        if (event.verb & scada::ModelChangeEvent::NodeDeleted) {
          observer_->OnNodeDeleted(event.node_id);

        } else {
          if (event.verb & (scada::ModelChangeEvent::ReferenceAdded |
                            scada::ModelChangeEvent::ReferenceDeleted))
            observer_->OnNodeChildrenChanged(event.node_id);
        }

        observer_->OnNodeModelChanged(event.node_id);
      }));
}

void NodeServiceTreeImpl::OnNodeSemanticChanged(const scada::NodeId& node_id) {
  auto task_runner = base::SequencedTaskRunnerHandle::Get();
  task_runner->PostTask(FROM_HERE, BindLambda([this, node_id] {
                          if (observer_)
                            observer_->OnNodeSemanticsChanged(node_id);
                        }));
}
