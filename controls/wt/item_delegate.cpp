#include "controls/wt/item_delegate.h"

#include <cassert>

#pragma warning(push)
#pragma warning(disable : 4251 4275)
#include <Wt/WAbstractItemModel.h>
#include <Wt/WComboBox.h>
#include <Wt/WLineEdit.h>
#include <Wt/WPushButton.h>
#pragma warning(pop)

std::unique_ptr<Wt::WWidget> ItemDelegate::createEditor(
    const Wt::WModelIndex& index,
    Wt::WFlags<Wt::ViewItemRenderFlag> flags) const {
  auto edit_data = edit_data_provider_(index);
  switch (edit_data.editor_type) {
    case ui::EditData::EditorType::NONE:
      return nullptr;

    case ui::EditData::EditorType::TEXT: {
      auto line_edit = std::make_unique<Wt::WLineEdit>();
      // line_edit->setFrame(false);
      return line_edit;
    }

    case ui::EditData::EditorType::BUTTON: {
      auto line_edit = std::make_unique<Wt::WLineEdit>();
      // line_edit->setFrame(false);
      // Wt::WIcon icon{":/device.png"};
      /*auto* action = line_edit->addAction(icon, QLineEdit::TrailingPosition);
      connect(action, &QAction::triggered,
              [index, line_edit, handler = edit_data.action_handler] {
                auto text = line_edit->text().toStdWString();
                if (handler(text)) {
                  const_cast<QAbstractItemModel*>(index.model())
                      ->setData(index, QString::fromStdWString(text),
                                Qt::EditRole);
                }
              });*/
      return line_edit;
    }

    case ui::EditData::EditorType::DROPDOWN: {
      auto combo_box = std::make_unique<Wt::WComboBox>();
      // combo_box->setFocusPolicy(Qt::WheelFocus);
      // combo_box->setEditable(true);
      // combo_box->setFrame(false);
      // combo_box->setInsertPolicy(QComboBox::InsertPolicy::NoInsert);
      for (const auto& choice : edit_data.choices)
        combo_box->addItem(choice);
      combo_box->activated().connect(
          [this] { const_cast<ItemDelegate*>(this)->CommitAndCloseEditor(); });
      /*connect(combo_box->lineEdit(), &QLineEdit::returnPressed, this, [this] {
        const_cast<ItemDelegate*>(this)->CommitAndCloseEditor();
      });*/
      return combo_box;
    }

    default:
      assert(false);
      return nullptr;
  }
}

void ItemDelegate::setEditState(Wt::WWidget* editor,
                                const Wt::WModelIndex& index,
                                const Wt::cpp17::any& value) const {
  auto text = Wt::asString(index.model()->data(index, Wt::ItemDataRole::Edit));
  if (auto* line_edit = static_cast<Wt::WLineEdit*>(editor))
    line_edit->setText(text);
  else if (auto* combo_box = static_cast<Wt::WComboBox*>(editor)) {
    if (int combo_index = combo_box->findText(text); combo_index != -1)
      combo_box->setCurrentIndex(combo_index);
    else
      combo_box->setValueText(text);
    // combo_box->showPopup();
  } else
    Wt::WItemDelegate::setEditState(editor, index, value);
}

void ItemDelegate::setModelData(const Wt::cpp17::any& editState,
                                Wt::WAbstractItemModel* model,
                                const Wt::WModelIndex& index) const {
  /*if (auto* line_edit = static_cast<Wt::WLineEdit*>(editor)) {
    if (line_edit->isModified())
      model->setData(index, line_edit->text(), Qt::EditRole);
  } else if (auto* combo_box = static_cast<Wt::WComboBox*>(editor)) {
    model->setData(index, combo_box->currentText(), Qt::EditRole);
  } else {
    Wt::WItemDelegate::setModelData(editor, model, index);
  }*/
  Wt::WItemDelegate::setModelData(editState, model, index);
}

void ItemDelegate::CommitAndCloseEditor() {
  /*QWidget* editor = qobject_cast<QWidget*>(sender());
  emit commitData(editor);
  emit closeEditor(editor);*/
}
