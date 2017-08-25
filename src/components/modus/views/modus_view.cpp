#include "client/components/modus/views/modus_view.h"

#include "base/logging.h"
#include "base/win/scoped_bstr.h"
#include "client/views/ambient_props.h"
#include "client/views/client_application_views.h"
#include "client/components/modus/views/modus_element.h"
#include "client/components/modus/views/modus_loader.h"
#include "client/components/modus/views/modus_object.h"

#include <atlwin.h>

// ModusView

ModusView::ModusView(ModusViewContext&& context)
    : ModusViewContext(std::move(context)),
      ActiveXControl(*g_application_views),
      selected_item_(scada::NodeId()) {
  set_controller(this);
}

ModusView::~ModusView() {
  DeleteObjects();
}

void ModusView::Open(const base::FilePath& path) {
  path_ = path;
}

base::FilePath ModusView::GetPath() const {
  return path_;
}

bool ModusView::ShowContainedItem(const scada::NodeId& item_id) {
  auto* object = FindObject(item_id);
  if (!object)
    return false;
    
  sde_document_->DocHighLight(&object->sde_object(), RGB(0, 255, 0), false);
  
  if (!object->elements().empty())
    selection_callback_(object->elements()[0]->timed_data());
  
  return true;
}

void ModusView::DeleteObjects() {
  for (Objects::iterator i = objects_.begin(); i != objects_.end(); ++i)
    delete *i;
  objects_.clear();

  sde_document_.Release();
  if (sde_form_)
    DispEventUnadvise(sde_form_.get());
  sde_form_.Release();
}

void ModusView::OpenInternal(const base::FilePath& path) {
  if (!sde_form_)
    return;

  assert(!sde_document_);
  assert(objects_.empty());
  
  sde_form_->Open(base::win::ScopedBstr(path.value().c_str()));

  sde_form_->get_Document(sde_document_.Receive());
  if (!sde_document_)
    return;

  {
    modus::ModusLoader loader({node_service_, timed_data_service_, file_cache_});
    loader.Load(*sde_document_, path, this);
    title_ = loader.title();
  }

  path_ = path;

  title_callback_(title_);
}

modus::ModusObject* ModusView::FindObject(scada::NodeId trid) {
  for (Objects::iterator i = objects_.begin(); i != objects_.end(); ++i) {
    modus::ModusObject& object = **i;
    for (auto* element : object.elements()) {
      if (element->timed_data().trid() == trid)
        return &object;
    }
  }
  return NULL;
}

STDMETHODIMP_(void) ModusView::OnDocPopup(ISDEDocument50* doc, VARIANT_BOOL* popup) {
  assert(doc);
  assert(popup);
  *popup = FALSE;
}

STDMETHODIMP_(void) ModusView::OnDocDblClick(ISDEDocument50* doc,
                                             SDECore::IUIEventInfo* info) {
  assert(doc);
  assert(info);
  
  modus::ModusObject* object = NULL;

  base::win::ScopedComPtr<SDECore::ISDEObject50> sde_object;
  info->get_Touched(sde_object.Receive());
  if (sde_object) {
    long id = -1;
    sde_object->get_RTID(&id);
    if (id != -1) {
      ObjectMap::iterator i = object_map_.find(id);
      if (i != object_map_.end())
        object = i->second;
    }
  }
   
  bool acked = false;
  if (object) {
    for (auto* element : object->elements()) {
      if (element->timed_data().alerting()) {
        element->timed_data().Acknowledge();
        acked = true;
      }
    }
  }

  if (sde_object && !acked) {
    base::string16 hyperlink = modus::GetHyperlink(*sde_object);
    if (!hyperlink.empty())
      navigation_callback_(base::FilePath(hyperlink));
  }
}

STDMETHODIMP_(void) ModusView::OnDocClick(ISDEDocument50* doc,
                                          SDECore::IUIEventInfo* info) {
  assert(doc);
  assert(info);

  long button = 0;
  info->get_Button(&button);

  static const long LeftBtn = 0;
  static const long RightBtn = 2;

/*	long x = 0, y = 0;
  info->get_X(&x);
  info->get_Y(&y);*/

  modus::ModusObject* object = NULL;
  
  base::win::ScopedComPtr<SDECore::ISDEObject50> sde_object;
  info->get_Touched(sde_object.Receive());
  if (sde_object) {
    long id = -1;
    sde_object->get_RTID(&id);
    
    if (id != -1) {
      ObjectMap::iterator i = object_map_.find(id);
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
    popup_menu_callback_(gfx::Point(pt));
  }
}

void ModusView::OnControlCreated(views::ActiveXControl& sender) {
  // set ambient properties
  {
    base::win::ScopedComPtr<IAxWinAmbientDispatchEx> ambientEx;
    QueryHost(IID_IAxWinAmbientDispatchEx, ambientEx.ReceiveVoid());
    assert(ambientEx);
    CComObject<AmbientProps>* ambient = NULL;
    CComObject<AmbientProps>::CreateInstance(&ambient);
    assert(ambient);
    ambient->display_name = OLESTR("Ñץולא");
    ambientEx->SetAmbientDispatch(ambient);
    ambientEx->put_MessageReflect(ATL_VARIANT_TRUE);
  }

  if (!CreateControl(L"{001F373C-29D3-5C7E-A000-A0FC803D82EE}"))
    CreateControl(L"{001F373C-29D3-5F7E-A000-A0FC803D82EE}");

  QueryControl(__uuidof(htsde2::IHTSDEForm2), sde_form_.ReceiveVoid());
  if (sde_form_) {
    DispEventAdvise(sde_form_.get());
    sde_form_->put_StatusVisible(VARIANT_FALSE);
    sde_form_->put_ToolbarVisible(VARIANT_FALSE);
    //sde_form_->put_PagesVisible(SDECore::txPagesHidden);
    sde_form_->put_AxBorderStyle(htsde2::afbNone);
  }

  OpenInternal(path_);
}

void ModusView::OnContractDestroyed(views::ActiveXControl& sender) {
  DeleteObjects();
}