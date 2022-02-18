#include "components/favourites/add_favourites_dialog.h"

#include "base/strings/string_util.h"
#include "common_resources.h"
#include "components/favourites/favourites.h"
#include "services/dialog_service.h"
#include "views/framework/dialog.h"

#include <atlbase.h>

#include <atlapp.h>
#include <atlctrls.h>
#include <string>

class AddFavouritesDialog : public framework::Dialog {
 public:
  explicit AddFavouritesDialog(Favourites& favourites)
      : Dialog{IDD_ADD_FAVOURITES}, favourites_{favourites} {}

  std::wstring title;
  std::wstring folder;

 protected:
  virtual void OnInitDialog() {
    SetItemText(IDC_NAME, title);

    folder_combo_ = GetItem(IDC_FOLDER);
    for (auto& folder : favourites_.folders())
      folder_combo_.AddString(base::AsWString(folder.GetTitle()).c_str());
    folder_combo_.SetCurSel(0);
  }

  virtual void OnOK() {
    title = GetItemText(IDC_NAME);
    folder = win_util::GetWindowText(folder_combo_);

    Dialog::OnOK();
  }

 private:
  Favourites& favourites_;

  WTL::CComboBox folder_combo_;
};

bool ShowAddFavouritesDialog(DialogService& dialog_service,
                             AddFavouritesContext&& context) {
  AddFavouritesDialog dlg{context.favourites_};
  dlg.title = base::AsWString(context.window_def_.title);
  if (dlg.Execute(dialog_service.GetDialogOwningWindow()) != IDOK)
    return false;

  const Page& folder =
      context.favourites_.GetOrAddFolder(base::AsString16(dlg.folder));
  context.window_def_.title = base::AsString16(dlg.title);
  context.favourites_.Add(context.window_def_, folder);
  return true;
}
