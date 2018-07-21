#include "components/modus/activex/modus_document.h"

#include "base/win/scoped_bstr.h"
#include "components/modus/activex/modus_element.h"
#include "components/modus/activex/modus_loader.h"
#include "components/modus/activex/modus_object.h"

namespace modus {

ModusDocument::ModusDocument(ModusDocumentContext&& context,
                             htsde2::IHTSDEForm2& sde_form,
                             const base::FilePath& path)
    : ModusDocumentContext{std::move(context)}, sde_form_{&sde_form} {
  DispEventAdvise(sde_form_.Get());
  sde_form_->put_StatusVisible(VARIANT_FALSE);
  sde_form_->put_ToolbarVisible(VARIANT_FALSE);
  // sde_form_->put_PagesVisible(SDECore::txPagesHidden);
  sde_form_->put_AxBorderStyle(htsde2::afbNone);

  sde_form_->Open(base::win::ScopedBstr(path.value().c_str()));

  sde_form_->get_Document(sde_document_.GetAddressOf());
  if (!sde_document_)
    return;

  {
    modus::ModusLoader loader{modus::ModusLoaderContext{
        alias_resolver_, timed_data_service_, file_cache_}};
    loader.Load(*sde_document_.Get(), path, this);
    title_ = loader.title();
  }
}

ModusDocument::~ModusDocument() {
  object_map_.clear();
  objects_.clear();

  sde_document_.Reset();
  if (sde_form_)
    DispEventUnadvise(sde_form_.Get());
  sde_form_.Reset();
}

bool ModusDocument::ShowContainedItem(const scada::NodeId& item_id) {
  auto* object = FindObject(item_id);
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
  return NULL;
}

STDMETHODIMP_(void)
ModusDocument::OnDocPopup(ISDEDocument50* doc, VARIANT_BOOL* popup) {
  assert(doc);
  assert(popup);
  *popup = FALSE;
}

STDMETHODIMP_(void)
ModusDocument::OnDocDblClick(ISDEDocument50* doc, SDECore::IUIEventInfo* info) {
  assert(doc);
  assert(info);

  modus::ModusObject* object = NULL;

  Microsoft::WRL::ComPtr<SDECore::ISDEObject50> sde_object;
  info->get_Touched(sde_object.GetAddressOf());
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
    base::string16 hyperlink = modus::GetHyperlink(*sde_object.Get());
    if (!hyperlink.empty())
      navigation_callback_(base::FilePath(hyperlink));
  }
}

STDMETHODIMP_(void)
ModusDocument::OnDocClick(ISDEDocument50* doc, SDECore::IUIEventInfo* info) {
  assert(doc);
  assert(info);

  long button = 0;
  info->get_Button(&button);

  static const long LeftBtn = 0;
  static const long RightBtn = 2;

  modus::ModusObject* object = NULL;

  Microsoft::WRL::ComPtr<SDECore::ISDEObject50> sde_object;
  info->get_Touched(sde_object.GetAddressOf());
  if (sde_object) {
    ObjectId id = -1;
    sde_object->get_RTID(&id);

    if (id != -1) {
      auto i = object_map_.find(id);
      if (i != object_map_.end())
        object = i->second;
    }
  }

  if (!object) {
    selection_callback_(rt::TimedDataSpec());
    return;
  }

  if (!object->elements().empty())
    selection_callback_(object->elements()[0]->timed_data());

  if (button == RightBtn) {
    POINT pt;
    GetCursorPos(&pt);
    context_menu_callback_(ToUiPoint(pt));
  }
}

}  // namespace modus
