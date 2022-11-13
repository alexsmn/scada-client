#pragma once

#include "controls/models/header_model.h"

namespace aui {

class FixedRowModel : public HeaderModel {
 public:
  class Delegate {
   public:
    virtual int GetRowCount() = 0;
    virtual base::string16 GetRowTitle(int index);
  };

  explicit FixedRowModel(Delegate& delegate);

  void set_row_height(int height) { row_height_ = height; }

  void NotifyModelChanged() { aui::HeaderModel::NotifyModelChanged(); }

  // aui::HeaderModel
  virtual int GetCount() const override;
  virtual int GetSize(int index) const override;
  virtual base::string16 GetTitle(int index) const override;

 private:
  Delegate& delegate_;

  int row_height_ = 17;
};

}  // namespace aui
