#pragma once

#include "aui/color.h"
#include "aui/handlers.h"
#include "aui/models/tree_model.h"

#include "aui/os_exchange_data.h"
#include <boost/signals2/connection.hpp>

#include <QAbstractitemmodel>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

class QIcon;

namespace scada::aui {

class TreeModel;

class TreeModelAdapter : public QAbstractItemModel {
 public:
  explicit TreeModelAdapter(std::shared_ptr<TreeModel> model);
  virtual ~TreeModelAdapter();

  void SetCheckable(bool checkable) { checkable_ = checkable; }

  using CheckedHandler = std::function<void(void* node, bool checked)>;
  void SetCheckedHandler(CheckedHandler handler) {
    checked_handler_ = std::move(handler);
  }

  bool IsChecked(void* node) const;
  void SetChecked(void* node, bool checked);
  void SetCheckedNodes(std::set<void*> nodes);

  void LoadIcons(std::string_view resource_path, int width, Color mask_color);

  // Loads row glyphs from SVG resources, tinted with `tint`, keeping index
  // order so it is a drop-in for the sliced-strip form above. The paths are
  // retained so RetintGlyphs() can re-render them when the theme changes.
  void LoadGlyphs(std::span<const std::string_view> resource_paths,
                  int size,
                  Color tint,
                  qreal device_pixel_ratio);

  // Re-renders the glyphs last passed to LoadGlyphs() in `tint`. No-op when
  // the adapter carries a bitmap strip instead, which cannot be recoloured.
  void RetintGlyphs(Color tint, qreal device_pixel_ratio);

  void* GetNode(const QModelIndex& index) const;
  QModelIndex GetNodeIndex(void* node, int column) const;

  void SetDragHandler(std::vector<std::string> supported_formats,
                      DragHandler handler);

  // QAbstractItemModel
  virtual QVariant headerData(int section,
                              Qt::Orientation orientation,
                              int role = Qt::DisplayRole) const override;
  virtual QModelIndex index(
      int row,
      int column,
      const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& child) const override;
  virtual int rowCount(
      const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(
      const QModelIndex& parent = QModelIndex()) const override;
  virtual QVariant data(const QModelIndex& index,
                        int role = Qt::DisplayRole) const override;
  virtual bool setData(const QModelIndex& index,
                       const QVariant& value,
                       int role = Qt::EditRole) override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual bool hasChildren(const QModelIndex& parent) const override;
  virtual bool canFetchMore(const QModelIndex& parent) const override;
  virtual void fetchMore(const QModelIndex& parent) override;
  virtual QStringList mimeTypes() const override;
  virtual QMimeData* mimeData(const QModelIndexList& indexes) const override;
  virtual bool canDropMimeData(const QMimeData* data,
                               Qt::DropAction action,
                               int row,
                               int column,
                               const QModelIndex& parent) const override;
  virtual bool dropMimeData(const QMimeData* data,
                            Qt::DropAction action,
                            int row,
                            int column,
                            const QModelIndex& parent) override;

  int row_height = 18;

  DropHandler drop_handler;

 private:
  int GetIndexOf(void* node) const;

  DropAction GetDropAction(const QMimeData* data,
                           Qt::DropAction action,
                           int row,
                           int column,
                           const QModelIndex& parent) const;

  void ConnectModel();

  void OnTreeNodesAdding(void* parent, int start, int count);
  void OnTreeNodesAdded(void* parent, int start, int count);
  void OnTreeNodesDeleting(void* parent, int start, int count);
  void OnTreeNodesDeleted(void* parent, int start, int count);
  void OnTreeNodeChanged(void* node);
  void OnTreeModelResetting();
  void OnTreeModelReset();

  const std::shared_ptr<TreeModel> model_;

  std::vector<boost::signals2::scoped_connection> model_connections_;

  std::vector<QIcon> icons_;

  // Retained so a theme change can re-render the glyphs in the new tint; empty
  // when the adapter carries a bitmap strip, which cannot be recoloured.
  std::vector<std::string> glyph_paths_;
  int glyph_size_ = 0;

  bool checkable_ = false;
  CheckedHandler checked_handler_;
  std::set<void*> checked_nodes_;

  QStringList supported_mime_types_;
  DragHandler drag_handler_;
};

}  // namespace scada::aui
