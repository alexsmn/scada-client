#include "controls/models/tree_model.h"

namespace aui {

void TreeModel::TreeNodesAdding(void* parent, int start, int count) {
  for (ObserverList::iterator i = observers_.begin(); i != observers_.end();)
    (*i++)->OnTreeNodesAdding(parent, start, count);
}

void TreeModel::TreeNodesAdded(void* parent, int start, int count) {
  for (ObserverList::iterator i = observers_.begin(); i != observers_.end();)
    (*i++)->OnTreeNodesAdded(parent, start, count);
}

void TreeModel::TreeNodesDeleting(void* parent, int start, int count) {
  for (ObserverList::iterator i = observers_.begin(); i != observers_.end();)
    (*i++)->OnTreeNodesDeleting(parent, start, count);
}

void TreeModel::TreeNodesDeleted(void* parent, int start, int count) {
  for (ObserverList::iterator i = observers_.begin(); i != observers_.end();)
    (*i++)->OnTreeNodesDeleted(parent, start, count);
}

void TreeModel::TreeNodeChanged(void* node) {
  for (ObserverList::iterator i = observers_.begin(); i != observers_.end();)
    (*i++)->OnTreeNodeChanged(node);
}

void TreeModel::TreeModelResetting() {
  for (ObserverList::iterator i = observers_.begin(); i != observers_.end();)
    (*i++)->OnTreeModelResetting();
}

void TreeModel::TreeModelReset() {
  for (ObserverList::iterator i = observers_.begin(); i != observers_.end();)
    (*i++)->OnTreeModelReset();
}

}  // namespace aui
