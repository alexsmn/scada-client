#include "graph/graph_setup_dialog.h"

#include "base/strings/string_util.h"

LRESULT GraphSetupDialog::OnInitDialog(UINT /*uMsg*/,
                                       WPARAM /*wParam*/,
                                       LPARAM /*lParam*/,
                                       BOOL& /*bHandled*/) {
  CenterWindow(GetParent());

  wnd_color = GetDlgItem(IDC_COLOR);
  for (size_t i = 1; i <= aui::GetColorCount(); i++)
    wnd_color.AddString((LPCTSTR)i);
  int col = aui::FindColor(color);
  wnd_color.SetCurSel(col != 0 ? col - 1 : 0);

  WTL::CUpDownCtrl(GetDlgItem(IDC_LINE_WEIGHT_UPDOWN)).SetRange(1, 10);

  SetDlgItemInt(IDC_LINE_WEIGHT, line_weight_, FALSE);

  return TRUE;
}

LRESULT GraphSetupDialog::OnCloseCmd(WORD /*wNotifyCode*/,
                                     WORD wID,
                                     HWND /*hWndCtl*/,
                                     BOOL& /*bHandled*/) {
  if (wID == IDOK) {
    int i = wnd_color.GetCurSel();
    color = i != -1 ? aui::GetColor(i) : aui::ColorCode::Black;

    line_weight_ = GetDlgItemInt(IDC_LINE_WEIGHT, NULL, FALSE);
  }

  EndDialog(wID);
  return 0;
}

void GraphSetupDialog::DrawItem(LPDRAWITEMSTRUCT dis) {
  _ASSERT(dis->CtlID == IDC_COLOR);
  WTL::CDCHandle dc(dis->hDC);

  BOOL sel = dis->itemState & ODS_SELECTED;
  HBRUSH brush = GetSysColorBrush(sel ? COLOR_HIGHLIGHT : COLOR_WINDOW);

  RECT rect = dis->rcItem;
  dc.FillRect(&rect, brush);

  InflateRect(&rect, -3, -2);
  std::wstring text;

  if (dis->itemID >= 0 && dis->itemID < aui::GetColorCount()) {
    // draw color
    RECT crect = rect;
    crect.right = crect.left + crect.bottom - crect.top;
    dc.SelectStockBrush(DC_BRUSH);
    dc.SetDCBrushColor(aui::ToCOLORREF(aui::GetColor(dis->itemID)));
    dc.SelectStockPen(BLACK_PEN);
    dc.Rectangle(&crect);
    // draw text
    text = base::AsWString(aui::GetColorName(dis->itemID));
    rect.left = crect.right + 3;
  }

  COLORREF color = GetSysColor(sel ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT);
  dc.SetTextColor(color);
  dc.SetBkMode(TRANSPARENT);
  dc.DrawText(text.data(), text.size(), &rect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}