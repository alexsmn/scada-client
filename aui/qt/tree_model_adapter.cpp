
#include "aui/qt/tree_model_adapter.h"

#include "aui/color.h"
#include "aui/drag_drop_types.h"
#include "aui/models/tree_model.h"
#include "aui/qt/image_util.h"
#include "aui/qt/theme_qt.h"
#include "base/check.h"

#include <QApplication>
#include <QIcon>
#include <QMimeData>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QSize>

namespace scada::aui {

namespace {

bool IsTransparent(Color color) {
  return color.rgba().a == 0;
}

// Resolves a model's `ColorRole` into the platform's own colours.
//
// This is the seam the toolkit-free models cannot cross for themselves: they
// can say *disabled* or *header*, and the palette -- which follows the OS
// light/dark theme and the user's accessibility settings -- decides what that
// looks like. Returns an unset `QVariant` for `Default`, leaving the view's
// own colour alone.
QVariant RoleForeground(ColorRole role) {
  const QPalette& palette = QApplication::palette();
  switch (role) {
    case ColorRole::Disabled:
      return palette.color(QPalette::Disabled, QPalette::Text);
    case ColorRole::Header:
      return palette.color(QPalette::Normal, QPalette::ButtonText);
    case ColorRole::Default:
      return QVariant{};
  }
  return QVariant{};
}

QVariant RoleBackground(ColorRole role) {
  const QPalette& palette = QApplication::palette();
  switch (role) {
    case ColorRole::Header:
      return palette.color(QPalette::Normal, QPalette::Button);
    // A disabled cell is greyed by its text colour alone; tinting the row
    // behind it as well would read as a selection.
    case ColorRole::Disabled:
    case ColorRole::Default:
      return QVariant{};
  }
  return QVariant{};
}

// The pixmap for a tree icon at its loaded size (icons are loaded at a single
// size; fall back to 16 px if the icon reports none).
QPixmap IconPixmap(const QIcon& icon) {
  const QList<QSize> sizes = icon.availableSizes();
  return icon.pixmap(sizes.isEmpty() ? QSize{16, 16} : sizes.first());
}

// Returns `base` with a small filled quality dot drawn to its left (a status
// badge before the node icon), preserving the base pixmap's device-pixel ratio
// so it stays crisp on high-DPI displays.
QPixmap WithStatusDot(const QPixmap& base, const QColor& color) {
  const qreal dpr = base.isNull() ? 1.0 : base.devicePixelRatio();
  const int dot = 8;  // logical px
  const int gap = 3;  // logical px between dot and icon
  const int base_w = base.isNull() ? 0 : static_cast<int>(base.width() / dpr);
  const int base_h =
      base.isNull() ? dot : static_cast<int>(base.height() / dpr);
  const int width = base_w + (base_w ? gap : 0) + dot;
  const int height = std::max(base_h, dot);

  QPixmap result(QSize{width, height} * dpr);
  result.setDevicePixelRatio(dpr);
  result.fill(Qt::transparent);

  QPainter painter{&result};
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(Qt::NoPen);
  painter.setBrush(color);
  painter.drawEllipse(QRectF{0.0, (height - dot) / 2.0, static_cast<qreal>(dot),
                             static_cast<qreal>(dot)});
  if (!base.isNull())
    painter.drawPixmap(dot + gap, (height - base_h) / 2, base);
  return result;
}

std::unique_ptr<QMimeData> CreateMimeData(const DragData& drag_data) {
  if (drag_data.empty())
    return nullptr;

  auto mime_data = std::make_unique<QMimeData>();
  for (auto& [mime_type, buffer] : drag_data) {
    // WARNING: |QByteArray::fromRawData| doesn't take ownership of the buffer.
    mime_data->setData(
        QString::fromLocal8Bit(mime_type.data(), mime_type.size()),
        QByteArray{buffer.data(), static_cast<int>(buffer.size())});
  }
  return mime_data;
}

DragData MakeDragData(const QMimeData& mime_data) {
  DragData drag_data;
  for (const auto& mime_type : mime_data.formats()) {
    const auto& data = mime_data.data(mime_type);
    std::vector<char> buffer{data.data(), data.data() + data.size()};
    drag_data.emplace(mime_type.toStdString(), std::move(buffer));
  }
  return drag_data;
}

int ConvertDropAction(Qt::DropAction action) {
  switch (action) {
    case Qt::DropAction::CopyAction:
      return aui::DragDropTypes::DRAG_COPY;
    case Qt::DropAction::MoveAction:
      return aui::DragDropTypes::DRAG_MOVE;
    case Qt::DropAction::LinkAction:
      return aui::DragDropTypes::DRAG_LINK;
    default:
      return aui::DragDropTypes::DRAG_NONE;
  }
}

}  // namespace

// TreeModelAdapter

TreeModelAdapter::TreeModelAdapter(std::shared_ptr<aui::TreeModel> model)
    : model_{std::move(model)} {
  ConnectModel();
}

TreeModelAdapter::~TreeModelAdapter() = default;

void TreeModelAdapter::ConnectModel() {
  model_connections_.push_back(
      model_->SubscribeNodesAdding([this](void* parent, int start, int count) {
        OnTreeNodesAdding(parent, start, count);
      }));
  model_connections_.push_back(
      model_->SubscribeNodesAdded([this](void* parent, int start, int count) {
        OnTreeNodesAdded(parent, start, count);
      }));
  model_connections_.push_back(model_->SubscribeNodesDeleting(
      [this](void* parent, int start, int count) {
        OnTreeNodesDeleting(parent, start, count);
      }));
  model_connections_.push_back(
      model_->SubscribeNodesDeleted([this](void* parent, int start, int count) {
        OnTreeNodesDeleted(parent, start, count);
      }));
  model_connections_.push_back(model_->SubscribeNodeChanged(
      [this](void* node) { OnTreeNodeChanged(node); }));
  model_connections_.push_back(
      model_->SubscribeModelResetting([this] { OnTreeModelResetting(); }));
  model_connections_.push_back(
      model_->SubscribeModelReset([this] { OnTreeModelReset(); }));
}

void TreeModelAdapter::LoadGlyphs(
    std::span<const std::string_view> resource_paths,
    int size,
    Color tint,
    qreal device_pixel_ratio) {
  glyph_paths_.assign(resource_paths.begin(), resource_paths.end());
  glyph_size_ = size;
  icons_ = ::LoadTintedGlyphs(resource_paths, size, tint.qcolor(),
                              device_pixel_ratio);
}

void TreeModelAdapter::RetintGlyphs(Color tint, qreal device_pixel_ratio) {
  if (glyph_paths_.empty())
    return;

  std::vector<std::string_view> paths;
  paths.reserve(glyph_paths_.size());
  for (const std::string& path : glyph_paths_)
    paths.emplace_back(path);
  icons_ =
      ::LoadTintedGlyphs(paths, glyph_size_, tint.qcolor(), device_pixel_ratio);
}

void* TreeModelAdapter::GetNode(const QModelIndex& index) const {
  return index.isValid() ? index.internalPointer() : model_->GetRoot();
}

QModelIndex TreeModelAdapter::GetNodeIndex(void* node, int column) const {
  int row = GetIndexOf(node);
  return createIndex(row, column, node);
}

QVariant TreeModelAdapter::headerData(int section,
                                      Qt::Orientation orientation,
                                      int role) const {
  if (orientation != Qt::Horizontal)
    return QAbstractItemModel::headerData(section, orientation, role);

  switch (role) {
    case Qt::DisplayRole:
      return QString::fromStdU16String(model_->GetColumnText(section));
    case Qt::SizeHintRole: {
      auto size =
          QAbstractItemModel::headerData(section, orientation, role).toSize();
      size.setHeight(20);
      int peferred_width = model_->GetColumnPreferredSize(section);
      if (peferred_width != 0)
        size.setWidth(peferred_width);
      return size;
    }
    default:
      return QAbstractItemModel::headerData(section, orientation, role);
  }
}

QModelIndex TreeModelAdapter::index(int row,
                                    int column,
                                    const QModelIndex& parent) const {
  if (!parent.isValid()) {
    base::Check(row == 0);
    return GetNodeIndex(model_->GetRoot(), column);
  }

  void* parent_node = GetNode(parent);
  if (row >= model_->GetChildCount(parent_node))
    return {};

  void* child_node = model_->GetChild(parent_node, row);
  return createIndex(row, column, child_node);
}

QModelIndex TreeModelAdapter::parent(const QModelIndex& child) const {
  base::Check(child.isValid());

  void* child_node = GetNode(child);

  if (child_node == model_->GetRoot())
    return QModelIndex();

  void* parent_node = model_->GetParent(child_node);
  return createIndex(GetIndexOf(parent_node), 0, parent_node);
}

int TreeModelAdapter::rowCount(const QModelIndex& parent) const {
  if (parent.column() > 0)
    return 0;

  if (!parent.isValid())
    return 1;  // root only

  return model_->GetChildCount(GetNode(parent));
}

int TreeModelAdapter::columnCount(const QModelIndex& parent) const {
  return model_->GetColumnCount();
}

QVariant TreeModelAdapter::data(const QModelIndex& index, int role) const {
  base::Check(index.isValid());

  void* node = GetNode(index);

  switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
      return QString::fromStdU16String(model_->GetText(node, index.column()));
    case Qt::ForegroundRole: {
      // A model answers with a role or with a literal colour, never both, so
      // the order here only decides which question is asked first. The models
      // that still name a literal colour are asserting a process semantic
      // (alarm state, data quality) with a fixed value that must not follow
      // the platform theme; they leave the role at `Default` and fall through.
      const ColorRole role = model_->GetColorRole(node, index.column());
      if (role != ColorRole::Default)
        return RoleForeground(role);
      auto color = model_->GetTextColor(node, index.column());
      return IsTransparent(color) ? QVariant{} : color.qcolor();
    }
    case Qt::BackgroundRole: {
      const ColorRole role = model_->GetColorRole(node, index.column());
      if (role != ColorRole::Default)
        return RoleBackground(role);
      auto color = model_->GetBackgroundColor(node, index.column());
      return IsTransparent(color) ? QVariant{} : color.qcolor();
    }
    case Qt::DecorationRole: {
      if (index.column() != 0)
        return QVariant();
      const int icon_index = model_->GetIcon(node);
      const bool has_icon =
          icon_index >= 0 && icon_index < static_cast<int>(icons_.size());
      // A quality status dot precedes the node icon when the model supplies
      // one.
      const std::optional<Color> status = model_->GetStatusColor(node);
      if (!status)
        return has_icon ? QVariant(icons_[icon_index]) : QVariant();
      const QPixmap base =
          has_icon ? IconPixmap(icons_[icon_index]) : QPixmap{};
      return QVariant(WithStatusDot(base, status->qcolor()));
    }
    case Qt::FontRole:
      // Value/timestamp columns render in the design-system monospace font so
      // digits stay tabular as they update (design-language.md §3).
      // `MonoValueFont` is empty under the legacy theme, keeping the default.
      if (model_->IsMonospaceColumn(index.column())) {
        if (std::optional<QFont> font = MonoValueFont())
          return *font;
      }
      return QVariant();
    case Qt::SizeHintRole:
      return QSize{-1, row_height};
    case Qt::CheckStateRole:
      if (!checkable_ || index.column() != 0 || node == model_->GetRoot())
        return QVariant();
      return checked_nodes_.find(node) == checked_nodes_.end() ? Qt::Unchecked
                                                               : Qt::Checked;
    default:
      return QVariant();
  }
}

bool TreeModelAdapter::setData(const QModelIndex& index,
                               const QVariant& value,
                               int role) {
  base::Check(index.isValid());

  void* node = GetNode(index);

  if (role == Qt::EditRole) {
    model_->SetText(node, index.column(), value.toString().toStdU16String());
    return true;

  } else if (role == Qt::CheckStateRole) {
    bool checked = value == Qt::Checked;
    if (checked_handler_)
      checked_handler_(node, checked);
    return true;

  } else {
    return false;
  }
}

Qt::ItemFlags TreeModelAdapter::flags(const QModelIndex& index) const {
  base::Check(index.isValid());

  auto flags = QAbstractItemModel::flags(index);

  void* node = GetNode(index);

  bool selectable = model_->IsSelectable(node, index.column());
  flags.setFlag(Qt::ItemIsSelectable, selectable);

  bool checkable =
      checkable_ && index.column() == 0 && node != model_->GetRoot();
  flags.setFlag(Qt::ItemIsUserCheckable, checkable);

  bool editable = model_->IsEditable(node, index.column());
  flags.setFlag(Qt::ItemIsEditable, editable);

  flags.setFlag(Qt::ItemIsDragEnabled,
                !supported_mime_types_.empty() && drag_handler_);

  flags.setFlag(Qt::ItemIsDropEnabled,
                !supported_mime_types_.empty() && drop_handler);

  return flags;
}

int TreeModelAdapter::GetIndexOf(void* node) const {
  if (node == model_->GetRoot())
    return 0;

  void* parent_node = model_->GetParent(node);
  for (int i = 0; i < model_->GetChildCount(parent_node); ++i) {
    if (model_->GetChild(parent_node, i) == node)
      return i;
  }

  return -1;
}

void TreeModelAdapter::OnTreeNodesAdding(void* parent, int start, int count) {
  auto parent_index = GetNodeIndex(parent, 0);
  beginInsertRows(parent_index, start, start + count - 1);
}

void TreeModelAdapter::OnTreeNodesAdded(void* parent, int start, int count) {
  endInsertRows();
}

void TreeModelAdapter::OnTreeNodesDeleting(void* parent, int start, int count) {
  auto parent_index = GetNodeIndex(parent, 0);
  beginRemoveRows(parent_index, start, start + count - 1);
}

void TreeModelAdapter::OnTreeNodesDeleted(void* parent, int start, int count) {
  endRemoveRows();
}

void TreeModelAdapter::OnTreeNodeChanged(void* node) {
  dataChanged(GetNodeIndex(node, 0),
              GetNodeIndex(node, model_->GetColumnCount() - 1));
}

bool TreeModelAdapter::IsChecked(void* node) const {
  return checked_nodes_.find(node) != checked_nodes_.end();
}

void TreeModelAdapter::SetChecked(void* node, bool checked) {
  bool changed = false;

  if (checked)
    changed = checked_nodes_.emplace(node).second;
  else
    changed = checked_nodes_.erase(node) != 0;

  if (changed) {
    auto index = GetNodeIndex(node, 0);
    dataChanged(index, index, {Qt::CheckStateRole});
  }
}

void TreeModelAdapter::SetCheckedNodes(std::set<void*> nodes) {
  std::vector<void*> changed_nodes;
  if (nodes.empty())
    changed_nodes.reserve(checked_nodes_.size());
  else if (checked_nodes_.empty())
    changed_nodes.reserve(nodes.size());
  std::set_symmetric_difference(nodes.begin(), nodes.end(),
                                checked_nodes_.begin(), checked_nodes_.end(),
                                std::back_inserter(changed_nodes));

  checked_nodes_ = std::move(nodes);

  const QVector<int> roles = {Qt::CheckStateRole};
  for (auto* node : changed_nodes) {
    auto index = GetNodeIndex(node, 0);
    dataChanged(index, index, roles);
  }
}

void TreeModelAdapter::OnTreeModelResetting() {
  beginResetModel();
}

void TreeModelAdapter::OnTreeModelReset() {
  endResetModel();
}

bool TreeModelAdapter::hasChildren(const QModelIndex& parent) const {
  void* node = parent.isValid() ? GetNode(parent) : model_->GetRoot();
  return model_->HasChildren(node);
}

bool TreeModelAdapter::canFetchMore(const QModelIndex& parent) const {
  void* node = parent.isValid() ? GetNode(parent) : model_->GetRoot();
  return model_->CanFetchMore(node);
}

void TreeModelAdapter::fetchMore(const QModelIndex& parent) {
  void* node = parent.isValid() ? GetNode(parent) : model_->GetRoot();
  model_->FetchMore(node);
}

void TreeModelAdapter::SetDragHandler(std::vector<std::string> mime_types,
                                      DragHandler handler) {
  std::transform(
      mime_types.begin(), mime_types.end(),
      std::back_inserter(supported_mime_types_),
      [](const std::string& str) { return QString::fromStdString(str); });

  drag_handler_ = std::move(handler);
}

QStringList TreeModelAdapter::mimeTypes() const {
  return supported_mime_types_;
}

QMimeData* TreeModelAdapter::mimeData(const QModelIndexList& indexes) const {
  std::vector<void*> nodes;
  nodes.reserve(indexes.size());
  std::transform(indexes.begin(), indexes.end(), std::back_inserter(nodes),
                 [&](const QModelIndex& index) { return GetNode(index); });

  auto drag_data = drag_handler_(nodes);
  return CreateMimeData(drag_data).release();
}

bool TreeModelAdapter::canDropMimeData(const QMimeData* data,
                                       Qt::DropAction action,
                                       int row,
                                       int column,
                                       const QModelIndex& parent) const {
  return !!GetDropAction(data, action, row, column, parent);
}

bool TreeModelAdapter::dropMimeData(const QMimeData* data,
                                    Qt::DropAction action,
                                    int row,
                                    int column,
                                    const QModelIndex& parent) {
  auto drop_action = GetDropAction(data, action, row, column, parent);
  if (!drop_action)
    return false;

  int drop_result = drop_action();
  return drop_result != aui::DragDropTypes::DRAG_NONE;
}

DropAction TreeModelAdapter::GetDropAction(const QMimeData* data,
                                           Qt::DropAction action,
                                           int row,
                                           int column,
                                           const QModelIndex& parent) const {
  if (!data || !drop_handler)
    return nullptr;

  auto drag_data = MakeDragData(*data);
  if (drag_data.empty())
    return nullptr;

  const int drop_action = ConvertDropAction(action);
  auto* node = GetNode(parent);

  return drop_handler(drop_action, drag_data, node);
}

}  // namespace scada::aui
