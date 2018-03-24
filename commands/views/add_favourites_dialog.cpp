#include "commands/add_favourites_dialog.h"

#include "common_resources.h"
#include "services/favourites.h"
#include "views/framework/dialog.h"

#include <atlbase.h>

#include <atlapp.h>
#include <atlctrls.h>
#include <string>

class AddFavouritesDialog : public framework::Dialog {
 public:
  explicit AddFavouritesDialog(Favourites& favourites)
      : Dialog{IDD_ADD_FAVOURITES}, favourites_{favourites} {}

  void set_name(const base::string16& name) { name_ = name; }

  const base::string16& name() const { return name_; }
  const base::string16& folder() const { return folder_; }

 protected:
  virtual void OnInitDialog() {
    SetItemText(IDC_NAME, name_);

    folder_combo_ = GetItem(IDC_FOLDER);

    const Favourites::Folders& folders = favourites_.folders();
    for (Favourites::Folders::const_iterator i = folders.begin();
         i != folders.end(); ++i) {
      const Page& folder = *i;
      folder_combo_.AddString(folder.GetTitle().c_str());
    }

    folder_combo_.SetCurSel(0);
  }

  virtual void OnOK() {
    name_ = GetItemText(IDC_NAME);
    folder_ = win_util::GetWindowText(folder_combo_);

    Dialog::OnOK();
  }

 private:
  Favourites& favourites_;

  base::string16 name_;
  base::string16 folder_;

  WTL::CComboBox folder_combo_;
};

bool ShowAddFavouritesDialog(Favourites& favourites,
                             base::string16& title,
                             base::string16& folder_name) {
  AddFavouritesDialog dlg{favourites};
  dlg.set_name(title);
  if (dlg.Execute() != IDOK)
    return false;
  title = dlg.name();
  folder_name = dlg.folder();
  return true;
}
