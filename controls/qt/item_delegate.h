#pragma once

#include "controls/models/edit_data.h"

#include <QItemDelegate>
#include <functional>

class QComboBox;

namespace aui {

class ItemDelegate final : public QItemDelegate {
  Q_OBJECT

 public:
  using EditDataProvider = std::function<EditData(const QModelIndex& index)>;
  using ButtonHandler = std::function<void(const QModelIndex& index)>;

  void set_edit_data_provider(EditDataProvider edit_data_provider) {
    edit_data_provider_ = std::move(edit_data_provider);
  }

  void set_button_handler(ButtonHandler button_handler) {
    button_handler_ = std::move(button_handler);
  }

  // QItemDelegate
  virtual QWidget* createEditor(QWidget* parent,
                                const QStyleOptionViewItem& option,
                                const QModelIndex& index) const override;
  virtual void setEditorData(QWidget* editor,
                             const QModelIndex& index) const override;
  virtual void setModelData(QWidget* editor,
                            QAbstractItemModel* model,
                            const QModelIndex& index) const override;

 private:
  void CommitAndCloseEditor();

  QComboBox* CreateDropDown(QWidget* parent, const EditData& edit_data) const;

  EditDataProvider edit_data_provider_;
  ButtonHandler button_handler_;
};

}  // namespace aui
