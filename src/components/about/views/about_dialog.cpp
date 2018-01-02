#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "base/win/win_util2.h"
#include "views/framework/dialog.h"
#include "common_resources.h"
#include "project.h"

#include <atlbase.h>
#include <atlapp.h>

class AboutDialog : public framework::Dialog {
 public:
  AboutDialog() : Dialog(IDD_ABOUTBOX) { }
  
 protected:
  virtual void OnInitDialog() {
    __super::OnInitDialog();
    
    base::string16 version_string = win_util::LoadResourceString(
        WTL::ModuleHelper::GetResourceInstance(), IDS_VERSION_STRING);
    base::string16 application_title = win_util::LoadResourceString(
        WTL::ModuleHelper::GetResourceInstance(), IDR_MAINFRAME);
    base::string16 text = base::StringPrintf(version_string.c_str(),
        application_title.c_str(),
        base::SysNativeMBToWide(PROJECT_VERSION_DOTTED_STRING).c_str());
        
    SetItemText(IDC_VERSION, text);
  }
};

void ShowAboutDialog() {
  AboutDialog().Execute();
}