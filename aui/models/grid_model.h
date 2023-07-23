#pragma once

#include "base/observer_list.h"
#include "aui/models/edit_data.h"
#include "aui/models/header_model.h"
#include "aui/models/table_column.h"

#include <cassert>
#include <vector>

namespace aui {

class GridRange;

struct GridModelIndex {
  int row = -1;
  int column = -1;

  bool is_valid() const { return row >= 0 && column >= 0; }
};

class GridModel {
 public:
  class Observer {
   public:
    // Range has been changed.
    virtual void OnGridModelChanged(GridModel& model) {}
    virtual void OnGridRowsAdded(GridModel& model, int first, int count) {}
    virtual void OnGridRowsRemoved(GridModel& model, int first, int count) {}

    // Data in specified range has been changed.
    virtual void OnGridRangeChanged(GridModel& model, const GridRange& range) {}
  };

  GridModel();
  virtual ~GridModel();

  std::u16string GetCellText(int row, int column);

  virtual void GetCell(GridCell& cell) = 0;
  virtual std::u16string GetHint(int row, int column);

  virtual bool IsEditable(int row, int column);
  virtual bool SetCellText(int row, int column, const std::u16string& text);
  virtual EditData GetEditData(int row, int column);
  // Invoked when `EditData.editor_type == BUTTON` and the button is clicked.
  virtual void HandleEditButton(int row, int column);

  base::ObserverList<Observer>& observers() { return observers_; }

 protected:
  void NotifyModelChanged();
  void NotifyRowsAdded(int first, int count);
  void NotifyRowsRemoved(int first, int count);

  void NotifyRangeChanged(const GridRange& range);
  void NotifyRowsChanged(int first, int count);

  base::ObserverList<Observer> observers_;
};

}  // namespace aui
