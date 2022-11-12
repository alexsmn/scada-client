#pragma once

#include "controls/models/table_model.h"
#include "controls/models/table_model_observer.h"

namespace aui {

class MirrorTableModel : public TableModel, private TableModelObserver {
 public:
  explicit MirrorTableModel(TableModel& source_model);
  ~MirrorTableModel();

  bool mirrored() const { return mirrored_; }
  void SetMirrored(bool mirrored);

  int MapFromSource(int row) const;
  int MapToSource(int row) const;

  // TableModel
  virtual int GetRowCount() override { return source_model_.GetRowCount(); }
  virtual void GetCell(TableCell& cell) override;
  virtual std::u16string GetTooltip(int row, int column_id) override;
  virtual bool SetCellText(int row,
                           int column_id,
                           const std::u16string& text) override;
  virtual bool IsEditable(int row, int column_id) override;
  virtual void Sort(int column_id, bool ascending) override;
  virtual int CompareCells(int row1, int row2, int column_id) override;

 private:
  int MirrorRow(int row) const;

  // TableModelObserver
  virtual void OnModelChanged() override;
  virtual void OnItemsChanged(int first, int count) override;
  virtual void OnItemsAdding(int first, int count) override;
  virtual void OnItemsAdded(int first, int count) override;
  virtual void OnItemsRemoving(int first, int count) override;
  virtual void OnItemsRemoved(int first, int count) override;

  TableModel& source_model_;

  bool mirrored_ = false;
};

inline MirrorTableModel::MirrorTableModel(TableModel& source_model)
    : source_model_{source_model} {
  source_model_.observers().AddObserver(this);
}

inline MirrorTableModel::~MirrorTableModel() {
  source_model_.observers().RemoveObserver(this);
}

inline void MirrorTableModel::SetMirrored(bool mirrored) {
  if (mirrored_ == mirrored)
    return;

  mirrored_ = mirrored;
  NotifyModelChanged();
}

inline void MirrorTableModel::GetCell(TableCell& cell) {
  const int mirrored_row = cell.row;
  cell.row = MapToSource(mirrored_row);
  source_model_.GetCell(cell);
  cell.row = mirrored_row;
}

inline std::u16string MirrorTableModel::GetTooltip(int row, int column_id) {
  const int source_row = MapToSource(row);
  return source_model_.GetTooltip(source_row, column_id);
}

inline bool MirrorTableModel::SetCellText(int row,
                                          int column_id,
                                          const std::u16string& text) {
  const int source_row = MapToSource(row);
  return source_model_.SetCellText(source_row, column_id, text);
}

inline bool MirrorTableModel::IsEditable(int row, int column_id) {
  const int source_row = MapToSource(row);
  return source_model_.IsEditable(source_row, column_id);
}

inline void MirrorTableModel::Sort(int column_id, bool ascending) {
  if (column_id == 0)
    SetMirrored(!ascending);
}

inline int MirrorTableModel::CompareCells(int row1, int row2, int column_id) {
  const int source_row1 = MapToSource(row1);
  const int source_row2 = MapToSource(row2);
  return source_model_.CompareCells(source_row1, source_row2, column_id);
}

inline int MirrorTableModel::MirrorRow(int row) const {
  if (!mirrored_)
    return row;

  const int row_count = source_model_.GetRowCount();
  return row_count - row - 1;
}

inline int MirrorTableModel::MapFromSource(int row) const {
  return MirrorRow(row);
}

inline int MirrorTableModel::MapToSource(int row) const {
  return MirrorRow(row);
}

inline void MirrorTableModel::OnModelChanged() {
  NotifyModelChanged();
}

inline void MirrorTableModel::OnItemsChanged(int first, int count) {
  const int mirrored_first = MapToSource(first);
  NotifyItemsChanged(mirrored_first, count);
}

inline void MirrorTableModel::OnItemsAdding(int first, int count) {
  const int mirrored_first =
      mirrored_ ? source_model_.GetRowCount() - first : first;
  NotifyItemsAdding(mirrored_first, count);
}

inline void MirrorTableModel::OnItemsAdded(int first, int count) {
  const int mirrored_first =
      mirrored_ ? source_model_.GetRowCount() - first - count : first;
  NotifyItemsAdded(mirrored_first, count);
}

inline void MirrorTableModel::OnItemsRemoving(int first, int count) {
  const int mirrored_first =
      mirrored_ ? source_model_.GetRowCount() - first - count - 1 : first;
  NotifyItemsRemoving(mirrored_first, count);
}

inline void MirrorTableModel::OnItemsRemoved(int first, int count) {
  const int mirrored_first =
      mirrored_ ? source_model_.GetRowCount() - first - 1 : first;
  NotifyItemsRemoved(mirrored_first, count);
}

}  // namespace aui
