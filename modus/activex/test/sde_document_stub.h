#pragma once

#include "modus/activex/modus.h"

#include <atlbase.h>

#include <atlcom.h>

namespace modus {

class ATL_NO_VTABLE SdeDocumentStub
    : public CComObjectRootEx<CComMultiThreadModel>,
      public IDispatchImpl<SDECore::ISDEDocument50> {
 public:
  BEGIN_COM_MAP(SdeDocumentStub)
  COM_INTERFACE_ENTRY(IDispatch)
  COM_INTERFACE_ENTRY(SDECore::ISDEDocument50)
  END_COM_MAP()

  // SDECore::ISDEDocument50

  STDMETHODIMP get_Techs(SDECore::INamedPBs** Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_LightChanging(VARIANT_BOOL* Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP put_LightChanging(VARIANT_BOOL Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP UpdateIndexesObj(SDECore::ISDEObject50* Obj) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP ResetIndexes() override { return E_NOTIMPL; }

  STDMETHODIMP get_NeedRecreateElectricModel(VARIANT_BOOL* Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP put_NeedRecreateElectricModel(VARIANT_BOOL Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP FindDamaged(SDECore::TxFindMask Mask,
                           SDECore::TxMaskChoice Mode,
                           SDECore::ISDEObjects2** Result) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_ProtectionModel(IUnknown** Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP RefreshIndex(BSTR Index) override { return E_NOTIMPL; }

  STDMETHODIMP DeleteIndex(BSTR Index) override { return E_NOTIMPL; }

  STDMETHODIMP get_Graphics(SDECore::TxTypeFormat TypeFormat,
                            long ObjectRTID,
                            VARIANT_BOOL DefaultOrient,
                            VARIANT_BOOL DefaultSize,
                            VARIANT* Result) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_Flat(
      /*[out,retval]*/ SDECore::ISDEObjects2** Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_FlatIndex(
      /*[out,retval]*/ BSTR* Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP put_FlatIndex(
      /*[in]*/ BSTR Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_Application(
      /*[out,retval]*/ IDispatch** Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_Author(
      /*[out,retval]*/ BSTR* Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP put_Author(
      /*[in]*/ BSTR Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_Comments(
      /*[out,retval]*/ BSTR* Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP put_Comments(
      /*[in]*/ BSTR Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_FullName(
      /*[out,retval]*/ BSTR* Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_KeyWords(
      /*[out,retval]*/ BSTR* Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP put_KeyWords(
      /*[in]*/ BSTR Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_Name(
      /*[out,retval]*/ BSTR* Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_Parent(
      /*[out,retval]*/ IDispatch** Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_Path(
      /*[out,retval]*/ BSTR* Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_ReadOnly(
      /*[out,retval]*/ VARIANT_BOOL* Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_Saved(
      /*[out,retval]*/ VARIANT_BOOL* Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_Subject(
      /*[out,retval]*/ BSTR* Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP put_Subject(
      /*[in]*/ BSTR Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_Title(
      /*[out,retval]*/ BSTR* Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP put_Title(
      /*[in]*/ BSTR Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP Activate() override { return E_NOTIMPL; }

  STDMETHODIMP Close() override { return E_NOTIMPL; }

  STDMETHODIMP Print() override { return E_NOTIMPL; }

  STDMETHODIMP PrintOut() override { return E_NOTIMPL; }

  STDMETHODIMP Save(
      /*[in]*/ VARIANT FileName,
      /*[in]*/ VARIANT Version) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_Pages(
      /*[out,retval]*/ SDECore::ISDEPages** Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP DocGoToView(
      /*[in]*/ SDECore::ISDEObject50* Element) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_SourceFileName(
      /*[out,retval]*/ BSTR* Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP put_SourceFileName(
      /*[in]*/ BSTR Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_CurrentPage(
      /*[out,retval]*/ SDECore::ISDEPage50** Value) override {
    return sde_page_.CopyTo(Value);
  }

  STDMETHODIMP get_CurrentPageIndex(
      /*[out,retval]*/ VARIANT* Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP put_CurrentPageIndex(
      /*[in]*/ VARIANT Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_SelectedList(
      /*[out,retval]*/ SDECore::ISDEObjects2** Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_SelectedView(
      /*[out,retval]*/ SDECore::ISDEObject50** Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP put_SelectedView(
      /*[in]*/ SDECore::ISDEObject50* Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP FindViewIndex(
      /*[in]*/ BSTR Index,
      /*[in]*/ BSTR Value,
      /*[out,retval]*/ SDECore::ISDEObject50** Element) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_RTID(
      /*[out,retval]*/ long* Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_UserProperties(
      /*[out,retval]*/ SDECore::IUserProperties** Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_DetailLevels(
      /*[out,retval]*/ SDECore::IDetailsLevels** Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP Open(
      /*[in]*/ BSTR FileName,
      /*[out,retval]*/ VARIANT_BOOL* Result) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_Int(
      /*[out,retval]*/ long* Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP put_Int(
      /*[in]*/ long Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_Selections(
      /*[out,retval]*/ SDECore::IHighlights** Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP FindViewsIndex(
      /*[in]*/ BSTR Index,
      /*[in]*/ BSTR Value,
      /*[out,retval]*/ SDECore::ISDEObjects2** Elements) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_MultiSelect(
      /*[out,retval]*/ VARIANT_BOOL* Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP put_MultiSelect(
      /*[in]*/ VARIANT_BOOL Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_ShowRules(
      /*[out,retval]*/ SDECore::IRules** Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_CurrentEntity(
      /*[out,retval]*/ IDispatch** Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP put_CurrentEntity(
      /*[in]*/ IDispatch* Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_ParamBlock(
      /*[out,retval]*/ IDispatch** Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP put_ParamBlock(
      /*[in]*/ IDispatch* Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP BeginUpdate() override { return E_NOTIMPL; }

  STDMETHODIMP EndUpdate() override { return E_NOTIMPL; }

  STDMETHODIMP DocHighLight(
      /*[in]*/ SDECore::ISDEObject50* Element,
      /*[in]*/ long Color,
      /*[in]*/ VARIANT_BOOL Restrict) override {
    return E_NOTIMPL;
  }

  CComPtr<SDECore::ISDEPage50> sde_page_;
};

}  // namespace modus