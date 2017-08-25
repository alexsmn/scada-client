#include "views/framework/dialog.h"
#include "common_resources.h"
#include "services/favourites.h"

#include <atlbase.h>
#include <wtl/atlapp.h>
#include <wtl/atlctrls.h>
#include <string>

class AddFavouritesDialog : public framework::Dialog {
 public:
  explicit AddFavouritesDialog(Favourites& favourites)
      : Dialog(IDD_ADD_FAVOURITES),
        favourites_(favourites) {
  }

  void set_name(const base::string16& name) { name_ = name; }

  const base::string16& name() const { return name_; }
  const base::string16& folder() const { return folder_; }

 protected:
  virtual void OnInitDialog() {
    SetItemText(IDC_NAME, name_);
    
    folder_combo_ = GetItem(IDC_FOLDER);

    for (auto& folder : favourites_.folders())
      folder_combo_.AddString(folder.GetTitle().c_str());

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

bool ShowAddFavouritesDialog(base::string16& title, base::string16& folder_name, Favourites& favourites) {
  AddFavouritesDialog dlg(favourites);
  dlg.set_name(title);
  if (dlg.Execute() != IDOK)
    return false;
  title = dlg.name();
  folder_name = dlg.folder();
  return true;
}