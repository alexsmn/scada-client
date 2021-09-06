#pragma once

#include "ui/base/models/edit_data.h"

#include <Wt/WItemDelegate.h>
#include <functional>

class ItemDelegate final : public Wt::WItemDelegate {
 public:
  using EditDataProvider =
      std::function<ui::EditData(const Wt::WModelIndex& index)>;

  explicit ItemDelegate(EditDataProvider edit_data_provider)
      : edit_data_provider_{std::move(edit_data_provider)} {}

  // QItemDelegate
  virtual std::unique_ptr<Wt::WWidget> createEditor(
      const Wt::WModelIndex& index,
      Wt::WFlags<Wt::ViewItemRenderFlag> flags) const override;
  virtual void setEditState(Wt::WWidget* editor,
                            const Wt::WModelIndex& index,
                            const Wt::cpp17::any& value) const override;
  virtual void setModelData(const Wt::cpp17::any& editState,
                            Wt::WAbstractItemModel* model,
                            const Wt::WModelIndex& index) const override;

 private:
  void CommitAndCloseEditor();

  const EditDataProvider edit_data_provider_;
};
