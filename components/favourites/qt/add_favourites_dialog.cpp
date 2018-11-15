#include "components/favourites/add_favourites_dialog.h"

#include "services/dialog_service.h"
#include "services/favourites.h"
#include "ui_add_favourites_dialog.h"

class AddFavouritesDialog : public QDialog, private AddFavouritesContext {
  Q_OBJECT

 public:
  explicit AddFavouritesDialog(AddFavouritesContext&& context,
                               QWidget* parent = nullptr);

 public Q_SLOTS:
  virtual void accept() override;

 private:
  Ui::AddFavouritesDialog ui;

  QListWidgetItem* empty_folder_item_ = nullptr;
};

#include "add_favourites_dialog.moc"

AddFavouritesDialog::AddFavouritesDialog(AddFavouritesContext&& context,
                                         QWidget* parent)
    : QDialog{parent}, AddFavouritesContext{std::move(context)} {
  ui.setupUi(this);

  ui.nameLineEdit->setText(QString::fromStdWString(window_def_.title));

  empty_folder_item_ =
      new QListWidgetItem{tr("(No Folder)"), ui.folderListWidget};
  ui.folderListWidget->addItem(empty_folder_item_);
  for (auto& folder : favourites_.folders())
    ui.folderListWidget->addItem(QString::fromStdWString(folder.GetTitle()));
  ui.folderListWidget->setCurrentRow(0);
}

void AddFavouritesDialog::accept() {
  window_def_.title = ui.nameLineEdit->text().toStdWString();
  auto* item = ui.folderListWidget->currentItem();
  auto folder_name =
      item && item != empty_folder_item_ ? item->text() : nullptr;
  const Page& folder =
      favourites_.GetOrAddFolder(folder_name.toStdWString().c_str());
  favourites_.Add(window_def_, folder);

  QDialog::accept();
}

bool ShowAddFavouritesDialog(DialogService& dialog_service,
                             AddFavouritesContext&& context) {
  AddFavouritesDialog dialog{std::move(context),
                             dialog_service.GetParentWidget()};
  return dialog.exec() == AddFavouritesDialog::Accepted;
}
