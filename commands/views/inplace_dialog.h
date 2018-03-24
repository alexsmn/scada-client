#pragma once

class InplaceDialog : public ATL::CDialogImpl<InplaceDialog> {
 public:
  UINT IDD;
  
  BEGIN_MSG_MAP(InplaceDialog)
    FORWARD_NOTIFICATIONS()
  END_MSG_MAP()
};
