#include "favorites/add_favourites_dialog.h"

#include "aui/dialog_service.h"
#include "aui/qt/dialog_util.h"
#include "favorites/favourites.h"
#include "ui_add_favourites_dialog.h"

#include <QListWidget>
#include <QPushButton>

#include <QDialogButtonBox>

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

  // Name the action rather than the assent (docs/client/ux/dialogs.md §3).
  ui.buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Add"));

  ui.nameLineEdit->setText(QString::fromStdU16String(window_def_.title));

  ui.folderListWidget->addItem(empty_folder_title_);
  for (auto& folder : favourites_.folders())
    ui.folderListWidget->addItem(QString::fromStdU16String(folder.GetTitle()));
  ui.folderListWidget->setCurrentRow(0);

  // Picking an existing group fills the «Group» field, which is what accept()
  // reads; the first row means no group. Connected after the initial
  // setCurrentRow so the field opens empty.
  connect(ui.folderListWidget, &QListWidget::currentRowChanged, this,
          [this](int row) {
            ui.folderLineEdit->setText(
                row <= 0 ? QString{} : ui.folderListWidget->item(row)->text());
          });
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

Awaitable<void> ShowAddFavouritesDialog(DialogService& dialog_service,
                                        AddFavouritesContext context) {
  auto dialog = std::make_unique<AddFavouritesDialog>(
      std::move(context), dialog_service.GetParentWidget());
  co_await StartModalDialog(std::move(dialog));
  co_return;
}
