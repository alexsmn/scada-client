#pragma once

#include "modus/activex/modus.h"

#include <atlbase.h>

#include <atlcom.h>
#include <wrl/client.h>

namespace modus {

class ModusEventSink
    : public CComObjectRootEx<CComSingleThreadModel>,
      public ATL::IDispEventImpl<1,
                                 ModusEventSink,
                                 &__uuidof(htsde2::IHTSDEForm2Events),
                                 &__uuidof(htsde2::__htsde2),
                                 0xFFFF,
                                 0xFFFF> {
 public:
  static Microsoft::WRL::ComPtr<ModusEventSink> Create(
      ModusDocument& document) {
    CComObject<ModusEventSink>* event_sink = nullptr;
    if (HRESULT hr = CComObject<ModusEventSink>::CreateInstance(&event_sink);
        FAILED(hr)) {
      throw std::runtime_error("Cannot create event sink");
    }

    event_sink->document_ = &document;
    return event_sink;
  }

  void DetachDocument() { document_ = nullptr; }

  BEGIN_COM_MAP(ModusEventSink)
  END_COM_MAP()

  BEGIN_SINK_MAP(ModusEventSink)
  SINK_ENTRY_EX(1, __uuidof(htsde2::IHTSDEForm2Events), 0x0000000b, OnDocPopup)
  SINK_ENTRY_EX(1,
                __uuidof(htsde2::IHTSDEForm2Events),
                0x0000001a,
                OnDocClick)  // click
  SINK_ENTRY_EX(1,
                __uuidof(htsde2::IHTSDEForm2Events),
                0x00000012,
                OnDocDblClick)  // double-click
  SINK_ENTRY_EX(1,
                __uuidof(htsde2::IHTSDEForm2Events),
                0x00000019,
                OnDocRightClick)  // right-click
  END_SINK_MAP()

  STDMETHOD_(void, OnDocPopup)(ISDEDocument50* doc, VARIANT_BOOL* popup);
  STDMETHOD_(void, OnDocClick)
  (ISDEDocument50* doc, SDECore::IUIEventInfo* info);
  STDMETHOD_(void, OnDocRightClick)
  (ISDEDocument50* doc, SDECore::IUIEventInfo* info);
  STDMETHOD_(void, OnDocDblClick)
  (ISDEDocument50* doc, SDECore::IUIEventInfo* info);

 private:
  ModusDocument* document_ = nullptr;
};

STDMETHODIMP_(void)
ModusEventSink::OnDocPopup(ISDEDocument50* sde_document, VARIANT_BOOL* popup) {
  assert(popup);

  *popup = FALSE;

  if (!document_)
    return;

  bool bool_popup = *popup != VARIANT_FALSE;
  document_->OnDocPopup(bool_popup);
  *popup = bool_popup ? VARIANT_TRUE : VARIANT_FALSE;
}

STDMETHODIMP_(void)
ModusEventSink::OnDocDblClick(ISDEDocument50* sde_document,
                              SDECore::IUIEventInfo* ui_event_info) {
  assert(ui_event_info);

  if (document_)
    document_->OnDocDblClick(*ui_event_info);
}

STDMETHODIMP_(void)
ModusEventSink::OnDocClick(ISDEDocument50* sde_document,
                           SDECore::IUIEventInfo* ui_event_info) {
  assert(ui_event_info);

  // WARNING: |info->get_Button()| doesn't always give the right button.

  if (document_)
    document_->OnDocClick(ModusDocument::MouseButton::Left, *ui_event_info);
}

STDMETHODIMP_(void)
ModusEventSink::OnDocRightClick(ISDEDocument50* sde_document,
                                SDECore::IUIEventInfo* ui_event_info) {
  assert(ui_event_info);

  if (document_)
    document_->OnDocClick(ModusDocument::MouseButton::Right, *ui_event_info);
}

}  // namespace modus
