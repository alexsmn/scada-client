#pragma once

#include "aui/models/table_column.h"
#include "base/lifetime.h"

#include <boost/signals2/connection.hpp>
#include <boost/signals2/signal.hpp>
#include <functional>
#include <string>

namespace scada::aui {

class HeaderModel {
 public:
  using ModelChangedCallback = std::function<void(HeaderModel& model)>;
  using SizeChangedCallback =
      std::function<void(HeaderModel& model, int index)>;

  bool fixed_size() const { return fixed_size_; }
  void SetFixedSize(bool fixed) { fixed_size_ = fixed; }

  virtual int GetCount() const = 0;

  virtual int GetSize(int index) const = 0;
  virtual void SetSize(int index, int new_size) {}

  virtual std::u16string GetTitle(int index) const = 0;

  virtual TableColumn::Alignment GetAlignment(int index) const {
    return TableColumn::CENTER;
  }

  virtual TableColumn::DataType GetDataType(int index) const {
    return TableColumn::DataType::General;
  }

  // Notifies after the columns have been replaced.
  [[nodiscard]] boost::signals2::scoped_connection SubscribeModelChanged(
      const ModelChangedCallback& callback);
  // Notifies after a column has been resized.
  [[nodiscard]] boost::signals2::scoped_connection SubscribeSizeChanged(
      const SizeChangedCallback& callback);

 protected:
  HeaderModel() : fixed_size_(false) {}

  void NotifyModelChanged();
  void NotifySizeChanged(int index);

 private:
  bool fixed_size_;

  boost::signals2::signal<void(HeaderModel&)> model_changed_signal_;
  boost::signals2::signal<void(HeaderModel&, int)> size_changed_signal_;
};

class ColumnHeaderModel : public HeaderModel {
 public:
  typedef std::vector<TableColumn> Columns;
  const Columns& columns() const SCADA_LIFETIME_BOUND { return columns_; }
  void SetColumns(int count, const TableColumn* columns);

  void SetColumnCount(int count, int column_width);

  // HeaderModel
  virtual int GetCount() const override { return columns_.size(); }
  virtual std::u16string GetTitle(int index) const override;
  // Out of range reads as 0, matching `SetSize`, which ignores such an index.
  virtual int GetSize(int index) const override {
    if (index < 0 || index >= static_cast<int>(columns_.size()))
      return 0;
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

}  // namespace scada::aui
