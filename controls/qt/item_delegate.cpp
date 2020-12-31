#include "item_delegate.h"

#include <QAction>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <cassert>

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

    case ui::EditData::EditorType::BUTTON: {
      auto* line_edit = new QLineEdit{parent};
      line_edit->setFrame(false);
      QIcon icon{":/device.png"};
      auto* action = line_edit->addAction(icon, QLineEdit::TrailingPosition);
      connect(action, &QAction::triggered,
              [index, line_edit, handler = edit_data.action_handler] {
                auto text = line_edit->text().toStdWString();
                if (handler(text)) {
                  const_cast<QAbstractItemModel*>(index.model())
                      ->setData(index, QString::fromStdWString(text),
                                Qt::EditRole);
                }
              });
      return line_edit;
    }

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
    if (int list_index = combo_box->findText(text); list_index != -1)
      combo_box->setCurrentIndex(list_index);
    else
      combo_box->setCurrentText(text);
    combo_box->showPopup();
  } else
    QItemDelegate::setEditorData(editor, index);
}

void ItemDelegate::setModelData(QWidget* editor,
                                QAbstractItemModel* model,
                                const QModelIndex& index) const {
  if (auto* line_edit = qobject_cast<QLineEdit*>(editor)) {
    if (line_edit->isModified())
      model->setData(index, line_edit->text(), Qt::EditRole);
  } else if (auto* combo_box = qobject_cast<QComboBox*>(editor)) {
    model->setData(index, combo_box->currentText(), Qt::EditRole);
  } else {
    QItemDelegate::setModelData(editor, model, index);
  }
}

void ItemDelegate::CommitAndCloseEditor() {
  QWidget* editor = qobject_cast<QWidget*>(sender());
  emit commitData(editor);
  emit closeEditor(editor);
}
