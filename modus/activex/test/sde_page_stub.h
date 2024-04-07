#pragma once

#include "modus/activex/modus.h"
#include "modus/activex/test/sde_objects_stub.h"

#include <atlbase.h>

#include <atlcom.h>
#include <vector>

namespace modus {

class ATL_NO_VTABLE SdePageStub : public CComObjectRootEx<CComMultiThreadModel>,
                                  public IDispatchImpl<SDECore::ISDEPage50> {
 public:
  BEGIN_COM_MAP(SdePageStub)
  COM_INTERFACE_ENTRY(IDispatch)
  COM_INTERFACE_ENTRY(SDECore::ISDEPage50)
  END_COM_MAP()

  // SDECore::ISDEPage50

  STDMETHODIMP GetPictureByCoordinates(
      /*[in]*/ int X,
      /*[in]*/ int Y,
      /*[in]*/ int RigthX,
      /*[in]*/ int BottomY,
      /*[in]*/ BSTR FormatName,
      /*[in]*/ BSTR FileName,
      /*[out]*/ VARIANT* DataPicture) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP GetObjectByCoordinates(
      /*[in]*/ int X,
      /*[in]*/ int Y,
      /*[out,retval]*/ SDECore::ISDEObject50** __Object) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP IntersectElements(
      /*[in]*/ double X1,
      /*[in]*/ double Y1,
      /*[in]*/ double X2,
      /*[in]*/ double X3,
      /*[out,retval]*/ SDECore::ISDEObjects2** Result) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP ContainsElements(
      /*[in]*/ double X1,
      /*[in]*/ double Y1,
      /*[in]*/ double X2,
      /*[in]*/ double X3,
      /*[out,retval]*/ SDECore::ISDEObjects2** Result) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP Reconnect() override { return E_NOTIMPL; }
  STDMETHODIMP get_SizeXBase(
      /*[out,retval]*/ long* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_SizeXBase(
      /*[in]*/ long Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_SizeYBase(
      /*[out,retval]*/ long* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_SizeYBase(
      /*[in]*/ long Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_Name(
      /*[out,retval]*/ BSTR* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_Name(
      /*[in]*/ BSTR Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_SDEObjects(
      /*[out,retval]*/ SDECore::ISDEObjects2** Value) override {
    return SdeObjectsStub::Create(objects_, Value);
  }

  STDMETHODIMP get_Params(
      /*[out,retval]*/ SDECore::IParams** Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_UseTopology(
      /*[out,retval]*/ VARIANT_BOOL* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_UseTopology(
      /*[in]*/ VARIANT_BOOL Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_SizeX(
      /*[out,retval]*/ long* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_SizeX(
      /*[in]*/ long Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_SizeY(
      /*[out,retval]*/ long* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_SizeY(
      /*[in]*/ long Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_PosX(
      /*[out,retval]*/ long* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_PosX(
      /*[in]*/ long Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_PosY(
      /*[out,retval]*/ long* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_PosY(
      /*[in]*/ long Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_Scale(
      /*[out,retval]*/ double* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_Scale(
      /*[in]*/ double Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_Color(
      /*[out,retval]*/ OLE_COLOR* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_Color(
      /*[in]*/ OLE_COLOR Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_WinLeft(
      /*[out,retval]*/ long* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_WinLeft(
      /*[in]*/ long Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_WinTop(
      /*[out,retval]*/ long* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_WinTop(
      /*[in]*/ long Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_WinWidth(
      /*[out,retval]*/ long* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_WinWidth(
      /*[in]*/ long Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_WinHeight(
      /*[out,retval]*/ long* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_WinHeight(
      /*[in]*/ long Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_RTID(
      /*[out,retval]*/ long* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_NavPicture(
      /*[out,retval]*/ VARIANT* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_NavPicture(
      /*[in]*/ VARIANT Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_PosEndX(
      /*[out,retval]*/ long* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_PosEndY(
      /*[out,retval]*/ long* Value) override {
    return E_NOTIMPL;
  }

  std::vector<CComPtr<SDECore::ISDEObject50>> objects_;
};

}  // namespace modus