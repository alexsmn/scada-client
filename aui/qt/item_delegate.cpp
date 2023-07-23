#include "aui/qt/item_delegate.h"

#include <QAction>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <cassert>

namespace aui {

QWidget* ItemDelegate::createEditor(QWidget* parent,
                                    const QStyleOptionViewItem& option,
                                    const QModelIndex& index) const {
  if (!edit_data_provider_)
    return nullptr;

  auto edit_data = edit_data_provider_(index);
  switch (edit_data.editor_type) {
    case EditData::EditorType::NONE:
      return nullptr;

    case EditData::EditorType::TEXT: {
      auto* line_edit = new QLineEdit{parent};
      line_edit->setFrame(false);
      return line_edit;
    }

    case EditData::EditorType::BUTTON: {
      auto* line_edit = new QLineEdit{parent};
      line_edit->setFrame(false);
      QIcon icon{":/device.png"};
      auto* action = line_edit->addAction(icon, QLineEdit::TrailingPosition);
      if (button_handler_) {
        connect(action, &QAction::triggered,
                [index, handler = button_handler_] { handler(index); });
      }
      return line_edit;
    }

    case EditData::EditorType::DROPDOWN:
      return CreateDropDown(parent, edit_data);

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

QComboBox* ItemDelegate::CreateDropDown(QWidget* parent,
                                        const EditData& edit_data) const {
  auto* combo_box = new QComboBox{parent};
  combo_box->setFocusPolicy(Qt::WheelFocus);
  combo_box->setEditable(true);
  combo_box->setFrame(false);
  combo_box->setInsertPolicy(QComboBox::InsertPolicy::NoInsert);

  connect(combo_box, QOverload<int>::of(&QComboBox::activated), this,
          [this] { const_cast<ItemDelegate*>(this)->CommitAndCloseEditor(); });

  connect(combo_box->lineEdit(), &QLineEdit::returnPressed, this,
          [this] { const_cast<ItemDelegate*>(this)->CommitAndCloseEditor(); });

  if (edit_data.async_choice_handler) {
    combo_box->addItem(tr("Loading..."));

    auto canceled = std::make_shared<bool>(false);
    connect(combo_box, &QObject::destroyed, [canceled] { *canceled = true; });

    edit_data.async_choice_handler(
        [combo_box, canceled](const std::vector<std::u16string>& choices,
                              bool last) {
          if (*canceled)
            return;

          QStringList items;
          items.reserve(choices.size());
          for (const auto& choice : choices)
            items.push_back(QString::fromStdU16String(choice));

          // Insert before loading.
          combo_box->insertItems(combo_box->count() - 1, items);

          if (last)
            combo_box->removeItem(combo_box->count() - 1);
        });

  } else {
    for (const auto& choice : edit_data.choices)
      combo_box->addItem(QString::fromStdU16String(choice));
  }

  return combo_box;
}

}  // namespace aui
