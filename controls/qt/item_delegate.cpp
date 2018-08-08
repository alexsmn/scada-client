#include "item_delegate.h"

#include <cassert>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>

QWidget* ItemDelegate::createEditor(QWidget* parent,
                                    const QStyleOptionViewItem& option,
                                    const QModelIndex& index) const {
  auto edit_data = edit_data_provider_(index);
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
          combo_box, QOverload<int>::of(&QComboBox::activated), this,
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

void ItemDelegate::setEditorData(QWidget* editor,
                                 const QModelIndex& index) const {
  auto text = index.model()->data(index, Qt::EditRole).toString();
  if (auto* line_edit = qobject_cast<QLineEdit*>(editor))
    line_edit->setText(text);
  else if (auto* combo_box = qobject_cast<QComboBox*>(editor)) {
    if (int index = combo_box->findText(text); index != -1)
      combo_box->setCurrentIndex(index);
    else
      combo_box->setCurrentText(text);
    combo_box->showPopup();
  } else
    QItemDelegate::setEditorData(editor, index);
}

void ItemDelegate::setModelData(QWidget* editor,
                                QAbstractItemModel* model,
                                const QModelIndex& index) const {
  if (auto* line_edit = qobject_cast<QLineEdit*>(editor))
    model->setData(index, line_edit->text(), Qt::EditRole);
  else if (auto* combo_box = qobject_cast<QComboBox*>(editor))
    model->setData(index, combo_box->currentText(), Qt::EditRole);
  else
    QItemDelegate::setModelData(editor, model, index);
}

void ItemDelegate::CommitAndCloseEditor() {
  QWidget* editor = qobject_cast<QWidget*>(sender());
  emit commitData(editor);
  emit closeEditor(editor);
}
