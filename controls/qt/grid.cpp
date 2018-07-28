#include "grid.h"

#include <QComboBox>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>

Grid::Grid(ui::GridModel& model,
           ui::HeaderModel& row_model,
           ui::HeaderModel& column_model)
    : model_{model}, model_adapter_{model, row_model, column_model} {
  horizontalHeader()->setHighlightSections(false);
  verticalHeader()->setDefaultSectionSize(19);
  setModel(&model_adapter_);
  resizeColumnsToContents();
  setItemDelegate(&item_delegate_);
}

Grid::~Grid() {
  setItemDelegate(nullptr);
}

void Grid::SetContextMenuHandler(ContextMenuHandler handler) {
  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, &QWidget::customContextMenuRequested,
          [this, handler](const QPoint& pos) {
            handler(viewport()->mapToGlobal(pos));
          });
}

QWidget* Grid::ItemDelegate::createEditor(QWidget* parent,
                                          const QStyleOptionViewItem& option,
                                          const QModelIndex& index) const {
  auto edit_data = grid_.model_.GetEditData(index.row(), index.column());
  switch (edit_data.editor_type) {
    case ui::EditData::EditorType::NONE:
      return nullptr;

    case ui::EditData::EditorType::TEXT: {
      auto* line_edit = new QLineEdit{parent};
      line_edit->setFrame(false);
      return line_edit;
    }

    case ui::EditData::EditorType::BUTTON:
      return new QPushButton{parent};

    case ui::EditData::EditorType::DROPDOWN: {
      auto* combo_box = new QComboBox{parent};
      combo_box->setFocusPolicy(Qt::WheelFocus);
      combo_box->setEditable(true);
      combo_box->setFrame(false);
      combo_box->setInsertPolicy(QComboBox::InsertPolicy::NoInsert);
      for (const auto& choice : edit_data.choices)
        combo_box->addItem(QString::fromStdWString(choice));
      connect(
          combo_box, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          [this] { const_cast<ItemDelegate*>(this)->CommitAndCloseEditor(); });
      connect(combo_box->lineEdit(), &QLineEdit::returnPressed, this, [this] {
        const_cast<ItemDelegate*>(this)->CommitAndCloseEditor();
      });
      return combo_box;
    }

    default:
      assert(false);
      return nullptr;
  }
}

void Grid::ItemDelegate::setEditorData(QWidget* editor,
                                       const QModelIndex& index) const {
  auto text = index.model()->data(index, Qt::EditRole).toString();
  if (auto* line_edit = qobject_cast<QLineEdit*>(editor))
    line_edit->setText(text);
  else if (auto* combo_box = qobject_cast<QComboBox*>(editor)) {
    combo_box->lineEdit()->blockSignals(true);
    combo_box->lineEdit()->setText(text);
    combo_box->lineEdit()->blockSignals(false);
    combo_box->showPopup();
  } else
    QItemDelegate::setEditorData(editor, index);
}

void Grid::ItemDelegate::setModelData(QWidget* editor,
                                      QAbstractItemModel* model,
                                      const QModelIndex& index) const {
  if (auto* line_edit = qobject_cast<QLineEdit*>(editor))
    model->setData(index, line_edit->text(), Qt::EditRole);
  else if (auto* combo_box = qobject_cast<QComboBox*>(editor))
    model->setData(index, combo_box->currentText(), Qt::EditRole);
  else
    QItemDelegate::setModelData(editor, model, index);
}

void Grid::ItemDelegate::CommitAndCloseEditor() {
  QWidget* editor = qobject_cast<QWidget*>(sender());
  emit commitData(editor);
  emit closeEditor(editor);
}
