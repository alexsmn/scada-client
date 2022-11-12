#pragma once

#include "base/observer_list.h"
#include "base/strings/string16.h"
#include "controls/models/table_column.h"

namespace aui {

class HeaderModel {
 public:
  class Observer {
   public:
    // Columns replaced.
    virtual void OnModelChanged(HeaderModel& model) {}
    virtual void OnSizeChanged(HeaderModel& model, int index) {}
  };

  bool fixed_size() const { return fixed_size_; }
  void SetFixedSize(bool fixed) { fixed_size_ = fixed; }

  virtual int GetCount() const = 0;

  virtual int GetSize(int index) const = 0;
  virtual void SetSize(int index, int new_size) {}

  virtual base::string16 GetTitle(int index) const = 0;

  virtual TableColumn::Alignment GetAlignment(int index) const {
    return TableColumn::CENTER;
  }

  virtual TableColumn::DataType GetDataType(int index) const {
    return TableColumn::DataType::General;
  }

  base::ObserverList<Observer>& observers() { return observers_; }

 protected:
  HeaderModel() : fixed_size_(false) {}

  void NotifyModelChanged();
  void NotifySizeChanged(int index);

 private:
  bool fixed_size_;

  base::ObserverList<Observer> observers_;
};

class ColumnHeaderModel : public HeaderModel {
 public:
  typedef std::vector<TableColumn> Columns;
  const Columns& columns() const { return columns_; }
  void SetColumns(int count, const TableColumn* columns);

  void SetColumnCount(int count, int column_width);

  // HeaderModel
  virtual int GetCount() const override { return columns_.size(); }
  virtual base::string16 GetTitle(int index) const override;
  virtual int GetSize(int index) const override {
    return columns_[index].width;
  }
  virtual void SetSize(int index, int new_size) override;
  virtual TableColumn::Alignment GetAlignment(int index) const override {
    return columns_[index].alignment;
  }
  virtual TableColumn::DataType GetDataType(int index) const override {
    return columns_[index].data_type;
  }

 private:
  Columns columns_;
};

}  // namespace aui
