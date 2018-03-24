#pragma once

#include <atlbase.h>
#include <atlcom.h>
#include <cassert>

inline HRESULT ConvertVariantToColor(const VARIANT& var, COLORREF& color) {
  CComVariant v(var);
  HRESULT res = v.ChangeType(VT_UI4);
  if (FAILED(res))
    return res;
  color = v.uintVal;
  return S_OK;
}

class AmbientProps : public CComCoClass<AmbientProps, &CLSID_NULL>,
                     public CComObjectRootEx<CComSingleThreadModel>,
                     public IDispatch {
 public:
  AmbientProps()
      : back_color(RGB(255, 255, 255)),
        fore_color(RGB(0, 0, 0)) { }

  BEGIN_COM_MAP(AmbientProps)
    COM_INTERFACE_ENTRY(IDispatch)
  END_COM_MAP()

  // IDispatch
  
  STDMETHOD(GetTypeInfoCount)(UINT* pctinfo) {
    pctinfo = 0;
    return S_OK;
  }
  
  STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo) {
    return E_INVALIDARG;
  }
  
  STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
                           LCID lcid, DISPID* rgdispid) {
    return E_INVALIDARG;
  }
  
  STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
                    DISPPARAMS* pdispparams, VARIANT* pvarResult,
                    EXCEPINFO* pexcepinfo, UINT* puArgErr) {
    if (wFlags & DISPATCH_PROPERTYGET){
      if (pdispparams->cArgs)
        return E_INVALIDARG;
      return PropGet(dispidMember, pvarResult);
    
    } else if (wFlags & DISPATCH_PROPERTYPUT) {
      if (pdispparams->cArgs != 1 ||
        pdispparams->cNamedArgs != 1 ||
        pdispparams->rgdispidNamedArgs[0] != DISPID_PROPERTYPUT)
        return E_INVALIDARG;
      return PropPut(dispidMember, pdispparams->rgvarg);
    
    } else
      return E_INVALIDARG;
  }

  virtual HRESULT PropGet(DISPID dispid, VARIANT* arg) {
    assert(arg->vt == VT_EMPTY);
    switch (dispid) {
      case DISPID_AMBIENT_BACKCOLOR:
        return CComVariant(back_color).Detach(arg);
      case DISPID_AMBIENT_DISPLAYNAME:
        return CComVariant(display_name).Detach(arg);
      case DISPID_AMBIENT_FONT:
        return CComVariant(font).Detach(arg);
      case DISPID_AMBIENT_FORECOLOR:
        return CComVariant(fore_color).Detach(arg);
      default:
        return E_INVALIDARG;
    }
  }

  virtual HRESULT PropPut(DISPID dispid, VARIANT* arg) {
    switch (dispid) {
      case DISPID_AMBIENT_BACKCOLOR:
        return ConvertVariantToColor(*arg, back_color);
      case DISPID_AMBIENT_DISPLAYNAME:
        if (arg->vt != VT_BSTR)
          return DISP_E_BADVARTYPE;
        display_name = arg->bstrVal;
        return S_OK;
      case DISPID_AMBIENT_FONT: {
        if (arg->vt != VT_DISPATCH && arg->vt != VT_UNKNOWN)
          return DISP_E_BADVARTYPE;
        HRESULT res = S_OK;
        CComPtr<IFont> font;
        if (arg->pdispVal)
          res = arg->pdispVal->QueryInterface(IID_IFont, (void**)&font);
        if (SUCCEEDED(res))
          this->font.Attach(font.Detach());
        return res;
      }
      case DISPID_AMBIENT_FORECOLOR:
        return ConvertVariantToColor(*arg, fore_color);
      default:
        return E_INVALIDARG;
    }
  }

 public:
  COLORREF	back_color;
  COLORREF	fore_color;
  CComBSTR	display_name;
  CComPtr<IFont>	font;
};
