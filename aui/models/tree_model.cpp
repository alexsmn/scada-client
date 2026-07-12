#include "aui/aui_ns_compat.h"

#include "aui/models/tree_model.h"

namespace scada::aui {

void TreeModel::TreeNodesAdding(void* parent, int start, int count) {
  nodes_adding_signal_(parent, start, count);
}

void TreeModel::TreeNodesAdded(void* parent, int start, int count) {
  nodes_added_signal_(parent, start, count);
}

void TreeModel::TreeNodesDeleting(void* parent, int start, int count) {
  nodes_deleting_signal_(parent, start, count);
}

void TreeModel::TreeNodesDeleted(void* parent, int start, int count) {
  nodes_deleted_signal_(parent, start, count);
}

void TreeModel::TreeNodeChanged(void* node) {
  node_changed_signal_(node);
}

void TreeModel::TreeModelResetting() {
  model_resetting_signal_();
}

void TreeModel::TreeModelReset() {
  model_reset_signal_();
}

}  // namespace aui
