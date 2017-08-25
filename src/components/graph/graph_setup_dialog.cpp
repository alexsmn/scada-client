#include "client/components/graph/graph_setup_dialog.h"

#include "skia/ext/skia_utils_win.h"

LRESULT GraphSetupDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	CenterWindow(GetParent());

	wnd_color = GetDlgItem(IDC_COLOR);
	for (size_t i = 1; i <= palette::GetColorCount(); i++)
		wnd_color.AddString((LPCTSTR)i);
	BYTE col = palette::FindColor(color);
	wnd_color.SetCurSel(col != 0 ? col - 1 : 0);

	WTL::CUpDownCtrl(GetDlgItem(IDC_LINE_WEIGHT_UPDOWN)).SetRange(1, 10);

	SetDlgItemInt(IDC_LINE_WEIGHT, line_weight_, FALSE);

	return TRUE;
}

LRESULT GraphSetupDialog::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if (wID == IDOK) {
		int i = wnd_color.GetCurSel();
		color = (i != -1) ? palette::GetColor(i) : SK_ColorBLACK;

		line_weight_ = GetDlgItemInt(IDC_LINE_WEIGHT, NULL, FALSE);
	}

	EndDialog(wID);
	return 0;
}

void GraphSetupDialog::DrawItem(LPDRAWITEMSTRUCT dis)
{
	_ASSERT(dis->CtlID == IDC_COLOR);
	WTL::CDCHandle dc(dis->hDC);

	BOOL sel = dis->itemState&ODS_SELECTED;
	HBRUSH brush = GetSysColorBrush(sel ? COLOR_HIGHLIGHT : COLOR_WINDOW);

	RECT rect = dis->rcItem;
	dc.FillRect(&rect, brush);

	InflateRect(&rect, -3, -2);
	LPCTSTR text = _T("");

	if (dis->itemID >= 0 && dis->itemID < palette::GetColorCount()) {
		// draw color
		RECT crect = rect;
		crect.right = crect.left + crect.bottom - crect.top;
		dc.SelectStockBrush(DC_BRUSH);
		dc.SetDCBrushColor(skia::SkColorToCOLORREF(palette::GetColor(dis->itemID)));
		dc.SelectStockPen(BLACK_PEN);
		dc.Rectangle(&crect);
		// draw text
		text = palette::GetColorName(dis->itemID);
		rect.left = crect.right + 3;
	}

	COLORREF color = GetSysColor(sel ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT);
	dc.SetTextColor(color);
	dc.SetBkMode(TRANSPARENT);
	dc.DrawText(text, -1, &rect, DT_LEFT|DT_VCENTER|DT_SINGLELINE);	
}