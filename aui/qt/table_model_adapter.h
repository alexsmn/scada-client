#pragma once

#include "aui/color.h"
#include "base/lifetime.h"

#include <QAbstractItemModel>
#include <boost/signals2/connection.hpp>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <vector>

class QIcon;

namespace scada::aui {

class TableModel;
struct TableColumn;

class TableModelAdapter : public QAbstractTableModel {
 public:
  TableModelAdapter(std::shared_ptr<TableModel> model,
                    std::vector<TableColumn> columns);
  virtual ~TableModelAdapter();

  TableModel& model() SCADA_LIFETIME_BOUND { return *model_; }
  const TableModel& model() const SCADA_LIFETIME_BOUND { return *model_; }

  std::vector<TableColumn>& columns() SCADA_LIFETIME_BOUND { return columns_; }
  const std::vector<TableColumn>& columns() const SCADA_LIFETIME_BOUND {
    return columns_;
  }

  // Loads cell glyphs from SVG resources, tinted with `tint`, keeping index
  // order so it is a drop-in for the sliced-strip form above.
  void LoadGlyphs(std::span<const std::string_view> resource_paths,
                  int size,
                  Color tint,
                  qreal device_pixel_ratio);

  // Re-renders the glyphs last passed to LoadGlyphs() in `tint`. No-op when
  // the adapter carries a bitmap strip, which cannot be recoloured.
  void RetintGlyphs(Color tint, qreal device_pixel_ratio);

  // QAbstractTableModel
  virtual int rowCount(
      const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(
      const QModelIndex& parent = QModelIndex()) const override;
  virtual QVariant data(const QModelIndex& index,
                        int role = Qt::DisplayRole) const override;
  virtual bool setData(const QModelIndex& index,
                       const QVariant& value,
                       int role = Qt::EditRole) override;
  virtual QVariant headerData(int section,
                              Qt::Orientation orientation,
                              int role = Qt::DisplayRole) const override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual void sort(int column,
                    Qt::SortOrder order = Qt::AscendingOrder) override;
  virtual QStringList mimeTypes() const override;
  virtual QMimeData* mimeData(const QModelIndexList& indexes) const override;

  void OnModelChanged();
  void OnItemsChanged(int first, int count);
  void OnItemsAdding(int first, int count);
  void OnItemsAdded(int first, int count);
  void OnItemsRemoving(int first, int count);
  void OnItemsRemoved(int first, int count);

 private:
  void ConnectModel();

  const std::shared_ptr<TableModel> model_;
  std::vector<TableColumn> columns_;
  std::vector<QIcon> icons_;

  // Retained so a theme change can re-render the glyphs in the new tint.
  std::vector<std::string> glyph_paths_;
  int glyph_size_ = 0;

  std::vector<boost::signals2::scoped_connection> model_connections_;
};

}  // namespace scada::aui
