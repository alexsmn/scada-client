#pragma once

#include "aui/aui_ns_compat.h"

#include "aui/handlers.h"
#include "aui/tree.h"
#include "base/check.h"

#include <Wt/WSortFilterProxyModel.h>

namespace scada::aui {

class TreeProxyModel : public Wt::WSortFilterProxyModel {
 public:
  explicit TreeProxyModel(Tree& tree) : tree_{tree} {}

  void SetCompareHandler(TreeCompareHandler handler);

 protected:
  // QSortFilterProxyModel
  virtual bool lessThan(const Wt::WModelIndex& source_left,
                        const Wt::WModelIndex& source_right) const override;

 private:
  Tree& tree_;
  TreeCompareHandler compare_handler_;
};

// TreeProxyModel

inline void TreeProxyModel::SetCompareHandler(TreeCompareHandler handler) {
  compare_handler_ = std::move(handler);
  invalidate();
}

inline bool TreeProxyModel::lessThan(
    const Wt::WModelIndex& source_left,
    const Wt::WModelIndex& source_right) const {
  base::Check(source_left.column() == source_right.column());

  if (compare_handler_ && source_left.column() == 0) {
    return compare_handler_(tree_.model_adapter_->GetNode(source_left),
                            tree_.model_adapter_->GetNode(source_right)) < 0;
  }

  return WSortFilterProxyModel::lessThan(source_left, source_right);
}

}  // namespace aui