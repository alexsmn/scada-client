#include "components/modus/activex/modus_document.h"

#include "base/memory_istream.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/scoped_bstr.h"
#include "components/modus/activex/modus.h"
#include "components/modus/activex/modus_element.h"
#include "components/modus/activex/modus_loader.h"
#include "components/modus/activex/modus_object.h"

#include <atlbase.h>

#include <atlcom.h>
#include <filesystem>
#include <functional>

namespace modus {

// ModusDocument::EventSink

class ModusDocument::EventSink
    : public CComObjectRootEx<CComSingleThreadModel>,
      public ATL::IDispEventImpl<1,
                                 EventSink,
                                 &__uuidof(htsde2::IHTSDEForm2Events),
                                 &__uuidof(htsde2::__htsde2),
                                 0xFFFF,
                                 0xFFFF> {
 public:
  void set_document(ModusDocument* document) { document_ = document; }

  BEGIN_COM_MAP(EventSink)
  END_COM_MAP()

  BEGIN_SINK_MAP(EventSink)
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
ModusDocument::EventSink::OnDocPopup(ISDEDocument50* sde_document,
                                     VARIANT_BOOL* popup) {
  assert(popup);

  *popup = FALSE;

  if (!document_)
    return;

  bool bool_popup = *popup != VARIANT_FALSE;
  document_->OnDocPopup(bool_popup);
  *popup = bool_popup ? VARIANT_TRUE : VARIANT_FALSE;
}

STDMETHODIMP_(void)
ModusDocument::EventSink::OnDocDblClick(ISDEDocument50* sde_document,
                                        SDECore::IUIEventInfo* ui_event_info) {
  assert(ui_event_info);

  if (document_)
    document_->OnDocDblClick(*ui_event_info);
}

STDMETHODIMP_(void)
ModusDocument::EventSink::OnDocClick(ISDEDocument50* sde_document,
                                     SDECore::IUIEventInfo* ui_event_info) {
  assert(ui_event_info);

  // WARNING: |info->get_Button()| doesn't always give the right button.

  if (document_)
    document_->OnDocClick(ModusDocument::MouseButton::Left, *ui_event_info);
}

STDMETHODIMP_(void)
ModusDocument::EventSink::OnDocRightClick(
    ISDEDocument50* sde_document,
    SDECore::IUIEventInfo* ui_event_info) {
  assert(ui_event_info);

  if (document_)
    document_->OnDocClick(ModusDocument::MouseButton::Right, *ui_event_info);
}

// ModusDocument

ModusDocument::ModusDocument(ModusDocumentContext&& context,
                             htsde2::IHTSDEForm2& sde_form)
    : ModusDocumentContext{std::move(context)}, sde_form_{&sde_form} {
  // WARNING: The initialization order is important for Modus.

  CreateEventSink();

  if (event_sink_) {
    event_sink_->DispEventAdvise(sde_form_.Get());
  }

  sde_form_->put_StatusVisible(VARIANT_FALSE);
  sde_form_->put_ToolbarVisible(VARIANT_FALSE);
  // sde_form_->put_PagesVisible(SDECore::txPagesHidden);
  sde_form_->put_AxBorderStyle(htsde2::afbNone);
}

ModusDocument::~ModusDocument() {
  object_map_.clear();
  objects_.clear();

  // WARNING: The cleanup order is important for Modus.

  sde_document_.Reset();

  if (event_sink_) {
    if (sde_form_)
      event_sink_->DispEventUnadvise(sde_form_.Get());
    event_sink_->set_document(nullptr);
  }

  sde_form_.Reset();
}

void ModusDocument::InitFromFilePath(const std::filesystem::path& path) {
  sde_form_->Open(base::win::ScopedBstr(path.wstring()));

  sde_form_->get_Document(sde_document_.ReleaseAndGetAddressOf());
  if (!sde_document_)
    return;

  {
    modus::ModusLoader loader{modus::ModusLoaderContext{
        alias_resolver_, timed_data_service_, file_cache_}};
    loader.Load(*sde_document_.Get(), path,
                [this](long object_id, std::unique_ptr<ModusObject> object) {
                  auto& added_object = objects_.emplace_back(std::move(object));
                  if (object_id != -1)
                    object_map_[object_id] = added_object.get();
                });
    title_ = base::WideToUTF16(loader.title());
  }

  PostInit();
}

void ModusDocument::InitFromState(std::string_view state) {
  Microsoft::WRL::ComPtr<IPersistStreamInit> psi;
  sde_form_.As(&psi);

  if (!psi) {
    return;
  }

  MemoryIStream stream{reinterpret_cast<BYTE*>(const_cast<char*>(state.data())),
                       state.size()};
  if (FAILED(psi->Load(&stream))) {
    return;
  }

  sde_form_->get_Document(sde_document_.ReleaseAndGetAddressOf());
  if (!sde_document_)
    return;

  PostInit();
}

void ModusDocument::PostInit() {}

void ModusDocument::CreateEventSink() {
  HRESULT res;
  CComObject<EventSink>* event_sink = nullptr;
  if (FAILED(res = CComObject<EventSink>::CreateInstance(&event_sink)))
    throw std::runtime_error("Cannot create event sink");

  event_sink_ = event_sink;
  event_sink_->set_document(this);
}

bool ModusDocument::ShowContainedItem(const scada::NodeId& node_id) {
  auto* object = FindObject(node_id);
  if (!object)
    return false;

  sde_document_->DocHighLight(&object->sde_object(), RGB(0, 255, 0), false);

  if (!object->elements().empty())
    selection_callback_(object->elements()[0]->timed_data());

  return true;
}

modus::ModusObject* ModusDocument::FindObject(const scada::NodeId& node_id) {
  for (auto& object : objects_) {
    for (auto& element : object->elements()) {
      if (element->timed_data().GetNode().node_id() == node_id)
        return object.get();
    }
  }
  return nullptr;
}

void ModusDocument::OnDocPopup(bool& popup) {
  // WARNING: We can't use this event to show context menu, as it happens before
  // OnDocClick() where we update selection.

  popup = false;
}

void ModusDocument::OnDocDblClick(SDECore::IUIEventInfo& ui_event_info) {
  modus::ModusObject* object = nullptr;

  Microsoft::WRL::ComPtr<SDECore::ISDEObject50> sde_object;
  ui_event_info.get_Touched(sde_object.ReleaseAndGetAddressOf());
  if (sde_object) {
    ObjectId id = -1;
    sde_object->get_RTID(&id);
    if (id != -1) {
      auto i = object_map_.find(id);
      if (i != object_map_.end())
        object = i->second;
    }
  }

  bool acked = false;
  if (object) {
    for (auto& element : object->elements()) {
      if (element->timed_data().alerting()) {
        element->timed_data().Acknowledge();
        acked = true;
      }
    }
  }

  if (sde_object && !acked) {
    auto hyperlink = modus::GetHyperlink(*sde_object.Get());
    if (!hyperlink.empty())
      navigation_callback_(base::AsString16(hyperlink));
  }
}

void ModusDocument::OnDocClick(MouseButton button,
                               SDECore::IUIEventInfo& ui_event_info) {
  modus::ModusObject* object = nullptr;

  Microsoft::WRL::ComPtr<SDECore::ISDEObject50> sde_object;
  ui_event_info.get_Touched(sde_object.ReleaseAndGetAddressOf());
  if (sde_object) {
    ObjectId id = -1;
    sde_object->get_RTID(&id);

    if (id != -1) {
      auto i = object_map_.find(id);
      if (i != object_map_.end())
        object = i->second;
    }
  }

  modus::ModusElement* element = nullptr;
  if (object && !object->elements().empty())
    element = object->elements().front().get();

  if (!element) {
    selection_callback_(TimedDataSpec());
    return;
  }

  selection_callback_(element->timed_data());

  if (button == MouseButton::Right) {
    POINT pt;
    GetCursorPos(&pt);
    context_menu_callback_(aui::Point{pt.x, pt.y});
  }
}

std::string ModusDocument::SaveState() const {
  Microsoft::WRL::ComPtr<IPersistStreamInit> psi;
  sde_form_.As(&psi);

  if (!psi) {
    return {};
  }

  std::string data(1024 * 1024, ' ');
  MemoryIStream stream{reinterpret_cast<BYTE*>(data.data()), 0, data.size()};
  if (FAILED(psi->Save(&stream, TRUE))) {
    return {};
  }

  data.resize(stream.size());
  return data;
}

}  // namespace modus
