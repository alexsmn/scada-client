#pragma once

#include "modus/activex/modus.h"
#include "modus/activex/test/named_pbs_stub.h"
#include "modus/activex/test/sde_objects_stub.h"

#include <atlbase.h>

#include <atlcom.h>

namespace modus {

class ATL_NO_VTABLE SdeObjectStub
    : public CComObjectRootEx<CComMultiThreadModel>,
      public IDispatchImpl<SDECore::ISDEObject50> {
 public:
  BEGIN_COM_MAP(SdeObjectStub)
  COM_INTERFACE_ENTRY(IDispatch)
  COM_INTERFACE_ENTRY(SDECore::ISDEObject50)
  END_COM_MAP()

  // SDECore::ISDEObject50

  STDMETHODIMP get_SValue(
      /*[in]*/ BSTR ParamNm,
      /*[out,retval]*/ BSTR* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_SValue(
      /*[in]*/ BSTR ParamNm,
      /*[in]*/ BSTR Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_IsContainer(
      /*[out,retval]*/ VARIANT_BOOL* Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP Clone(
      /*[in]*/ SDECore::ISDEDocument* DestDoc,
      /*[in]*/ SDECore::ISDEPage* DestPage,
      /*[in]*/ double ShiftX,
      /*[in]*/ double ShiftY,
      /*[in]*/ VARIANT_BOOL internal,
      /*[out,retval]*/ SDECore::ISDEObject42** Result) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP Shift(
      /*[in]*/ double X,
      /*[in]*/ double Y) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP ChangeType(
      /*[in]*/ VARIANT NewType) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP MinimalDistance(
      /*[in]*/ SDECore::ISDEObject42* V,
      /*[out,retval]*/ long* Result) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_Typ(
      /*[out,retval]*/ long* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP VinheritsFrom(
      /*[in]*/ BSTR AClassName,
      /*[out,retval]*/ VARIANT_BOOL* Result) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP Connect(
      /*[in]*/ VARIANT_BOOL act) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_OwnerList(
      /*[out,retval]*/ SDECore::ISDEObjects2** Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_KeyRoleLink(
      /*[out,retval]*/ BSTR* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_KeyLink(
      /*[out,retval]*/ BSTR* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_RoleLink(
      /*[out,retval]*/ BSTR* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_DispName(
      /*[out,retval]*/ BSTR* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_EquipType(
      /*[out,retval]*/ BSTR* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_DOCRTID(
      /*[out,retval]*/ long* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_Bounds(
      /*[out,retval]*/ SDECore::SDERect* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_Bounds(
      /*[in]*/ SDECore::SDERect Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_SNIdent(
      /*[out,retval]*/ BSTR* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_TypeName(
      /*[out,retval]*/ BSTR* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_Nodes(
      /*[out,retval]*/ SDECore::INodes42** Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_Elements(
      /*[out,retval]*/ SDECore::ISDEObjects2** Value) override {
    return SdeObjectsStub::Create(elements_, Value);
  }

  STDMETHODIMP get_Owner(
      /*[out,retval]*/ SDECore::ISDEObject50** Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_Params(
      /*[out,retval]*/ SDECore::IParams** Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_VisibleExt(
      /*[out,retval]*/ SDECore::TxVisibilityStatus* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_ShortPath(
      /*[out,retval]*/ BSTR* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_Tag(
      /*[out,retval]*/ long* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_OwnerPage(
      /*[out,retval]*/ SDECore::ISDEPage50** Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_OwnerDoc(
      /*[out,retval]*/ SDECore::ISDEDocument50** Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_X(
      /*[in]*/ long Index,
      /*[out,retval]*/ double* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_X(
      /*[in]*/ long Index,
      /*[in]*/ double Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_Y(
      /*[in]*/ long Index,
      /*[out,retval]*/ double* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_Y(
      /*[in]*/ long Index,
      /*[in]*/ double Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_NoOfNodes(
      /*[out,retval]*/ long* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_Radius(
      /*[out,retval]*/ double* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_Radius(
      /*[in]*/ double Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_Orient90(
      /*[out,retval]*/ long* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP put_Orient90(
      /*[in]*/ long Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_Document(
      /*[out,retval]*/ SDECore::ISDEDocument50** Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_Page(
      /*[out,retval]*/ SDECore::ISDEPage50** Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_Container(
      /*[out,retval]*/ SDECore::ISDEObject50** Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP SaveStream(
      /*[in]*/ VARIANT Destination,
      /*[in]*/ SDECore::TxPersister Format) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP LoadStream(
      /*[in]*/ VARIANT Source,
      /*[in]*/ SDECore::TxPersister Format) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_RTID(
      /*[out,retval]*/ long* Value) override {
    return E_NOTIMPL;
  }
  STDMETHODIMP get_Tech(
      /*[out,retval]*/ SDECore::ITechObject** Value) override {
    return E_NOTIMPL;
  }

  STDMETHODIMP get_Techs(
      /*[out,retval]*/ SDECore::INamedPBs** Value) override {
    return NamedPbsStub::Create(techs_, Value);
  }

  STDMETHODIMP get_ViewParts(
      /*[out,retval]*/ SDECore::INamedPBs** Value) override {
    return E_NOTIMPL;
  }

  std::vector<CComPtr<SDECore::INamedPB>> techs_;
  std::vector<CComPtr<SDECore::ISDEObject50>> elements_;
};

}  // namespace modus