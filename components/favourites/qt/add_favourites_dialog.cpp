#include "components/favourites/add_favourites_dialog.h"

#include "components/favourites/favourites.h"
#include "services/dialog_service.h"
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

  const QString empty_folder_title_ = tr("(No Folder)");
};

#include "add_favourites_dialog.moc"

AddFavouritesDialog::AddFavouritesDialog(AddFavouritesContext&& context,
                                         QWidget* parent)
    : QDialog{parent}, AddFavouritesContext{std::move(context)} {
  ui.setupUi(this);

  ui.nameLineEdit->setText(QString::fromStdU16String(window_def_.title));

  ui.folderListWidget->addItem(empty_folder_title_);
  for (auto& folder : favourites_.folders())
    ui.folderListWidget->addItem(QString::fromStdU16String(folder.GetTitle()));
  ui.folderListWidget->setCurrentRow(0);
}

void AddFavouritesDialog::accept() {
  window_def_.title = ui.nameLineEdit->text().toStdU16String();
  auto folder_name = ui.folderLineEdit->text();
  if (folder_name == empty_folder_title_)
    folder_name.clear();
  const Page& folder = favourites_.GetOrAddFolder(folder_name.toStdU16String());
  favourites_.Add(window_def_, folder);

  QDialog::accept();
}

bool ShowAddFavouritesDialog(DialogService& dialog_service,
                             AddFavouritesContext&& context) {
  AddFavouritesDialog dialog{std::move(context),
                             dialog_service.GetParentWidget()};
  return dialog.exec() == AddFavouritesDialog::Accepted;
}
