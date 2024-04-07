#pragma once

#include "modus/activex/modus.h"

#include <atlbase.h>

#include <atlcom.h>

namespace modus {

class ATL_NO_VTABLE SdeFormStub
    : public CComObjectRootEx<CComMultiThreadModel>,
      public CComCoClass<SdeFormStub, &__uuidof(htsde2::HTSDEForm2)>,
      public IDispatchImpl<htsde2::IHTSDEForm2>,
      public IPersistStreamInitImpl<SdeFormStub> {
 public:
  BEGIN_COM_MAP(SdeFormStub)
  COM_INTERFACE_ENTRY(IDispatch)
  COM_INTERFACE_ENTRY(htsde2::IHTSDEForm2)
  COM_INTERFACE_ENTRY(IPersistStreamInit)
  END_COM_MAP()

  BEGIN_PROP_MAP(SdeFormStub)
  // Property map entries for persistent data members (if any)
  // PROP_DATA_ENTRY("Data1", m_data1, VT_I4)
  // PROP_DATA_ENTRY("Data2", m_data2, VT_BSTR)
  END_PROP_MAP()

  // htsde2::IHTSDEForm2

  STDMETHODIMP get_AutoScroll(VARIANT_BOOL* AutoScroll) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP put_AutoScroll(VARIANT_BOOL AutoScroll) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_AxBorderStyle(
      htsde2::TxActiveFormBorderStyle* AxBorderStyle) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP put_AxBorderStyle(
      htsde2::TxActiveFormBorderStyle AxBorderStyle) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_Caption(BSTR* Caption) override { return E_NOTIMPL; }

  STDMETHODIMP put_Caption(BSTR Caption) override { return E_NOTIMPL; }

  STDMETHODIMP get_Color(OLE_COLOR* Color) override { return E_NOTIMPL; }

  STDMETHODIMP put_Color(OLE_COLOR Color) override { return E_NOTIMPL; }

  STDMETHODIMP get_Font(struct Font** Font) override { return E_NOTIMPL; }

  STDMETHODIMP put_Font(struct Font* Font) override { return E_NOTIMPL; }

  STDMETHODIMP get_KeyPreview(VARIANT_BOOL* KeyPreview) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP put_KeyPreview(VARIANT_BOOL KeyPreview) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_PixelsPerInch(long* PixelsPerInch) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP put_PixelsPerInch(long PixelsPerInch) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_PrintScale(htsde2::TxPrintScale* PrintScale) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP put_PrintScale(htsde2::TxPrintScale PrintScale) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_Scaled(VARIANT_BOOL* Scaled) override { return E_NOTIMPL; }

  STDMETHODIMP put_Scaled(VARIANT_BOOL Scaled) override { return E_NOTIMPL; }

  STDMETHODIMP get_Active(VARIANT_BOOL* Active) override { return E_NOTIMPL; }

  STDMETHODIMP get_DropTarget(VARIANT_BOOL* DropTarget) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP put_DropTarget(VARIANT_BOOL DropTarget) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_HelpFile(BSTR* HelpFile) override { return E_NOTIMPL; }

  STDMETHODIMP put_HelpFile(BSTR HelpFile) override { return E_NOTIMPL; }

  STDMETHODIMP get_WindowState(
      enum SDECore::TxWindowState* WindowState) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP put_WindowState(
      enum SDECore::TxWindowState WindowState) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_Visible(VARIANT_BOOL* Visible) override { return E_NOTIMPL; }

  STDMETHODIMP put_Visible(VARIANT_BOOL Visible) override { return E_NOTIMPL; }

  STDMETHODIMP get_Enabled(VARIANT_BOOL* Enabled) override { return E_NOTIMPL; }

  STDMETHODIMP put_Enabled(VARIANT_BOOL Enabled) override { return E_NOTIMPL; }

  STDMETHODIMP get_Cursor(short* Cursor) override { return E_NOTIMPL; }

  STDMETHODIMP put_Cursor(short Cursor) override { return E_NOTIMPL; }

  STDMETHODIMP AboutBox() override { return E_NOTIMPL; }

  STDMETHODIMP get_Navigator(htsde2::INavigator** Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_FileName(BSTR* pbstrFileName) override { return E_NOTIMPL; }

  STDMETHODIMP get_ToolbarVisible(VARIANT_BOOL* pVisible) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_StatusVisible(VARIANT_BOOL* pVisible) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_Scale(long* pScale) override { return E_NOTIMPL; }

  STDMETHODIMP get_NavigatorVisible(VARIANT_BOOL* pVisible) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_PagesVisible(SDECore::TxPagesMode* pMode) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_ScrollVisible(VARIANT_BOOL* pVisible) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_Document(SDECore::ISDEDocument50** ppDocument) override {
    return sde_document_.CopyTo(ppDocument);
  }

  STDMETHODIMP get_ElectricModel(IDispatch** ppDispatch) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_Loader(SDECore::ISDELoader** ppLoader) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_AppHandle(OLE_HANDLE* phAppHandle) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP put_FileName(BSTR bstrFileName) override { return E_NOTIMPL; }

  STDMETHODIMP put_ToolbarVisible(VARIANT_BOOL vbVisible) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP put_StatusVisible(VARIANT_BOOL vbVisible) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP put_Scale(long lScale) override { return E_NOTIMPL; }

  STDMETHODIMP put_NavigatorVisible(VARIANT_BOOL vbVisible) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP put_PagesVisible(SDECore::TxPagesMode mode) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP put_ScrollVisible(VARIANT_BOOL vbVisible) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP put_AppHandle(OLE_HANDLE hAppHandle) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP FindView(BSTR bstrViewName, long* plViewIndex) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP GotoView(BSTR bstrViewName, long* plViewIndex) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP Print() override { return E_NOTIMPL; }

  STDMETHODIMP ShowOptions() override { return E_NOTIMPL; }

  STDMETHODIMP Open(BSTR bstrFilename) override { return E_NOTIMPL; }

  STDMETHODIMP FitToWindow() override { return E_NOTIMPL; }

  CComPtr<SDECore::ISDEDocument50> sde_document_;

  BOOL m_bRequiresSave = FALSE;
};

}  // namespace modus