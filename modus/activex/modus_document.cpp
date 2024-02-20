#include "modus/activex/modus_document.h"

#include "base/memory_istream.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/scoped_bstr.h"
#include "base/win/variant_util.h"
#include "modus/activex/modus_element.h"
#include "modus/activex/modus_event_sink.h"
#include "modus/activex/modus_loader.h"
#include "modus/activex/modus_object.h"
#include "profile/profile.h"

namespace modus {

ModusDocument::ModusDocument(ModusDocumentContext&& context,
                             htsde2::IHTSDEForm2& sde_form)
    : ModusDocumentContext{std::move(context)}, sde_form_{&sde_form} {
  // WARNING: The initialization order is important for Modus.

  // TODO: Event sink is not working. Instead, Qt signals are used. Investigate.
  // event_sink_ = ModusEventSink::Create(*this);

  if (event_sink_) {
    if (HRESULT hr = event_sink_->DispEventAdvise(sde_form_.Get());
        FAILED(hr)) {
      // TODO: Print `HRESULT`.
      throw std::runtime_error(
          std::format("Cannot connect Modus events. Error 0x{:x}", hr));
    }
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
    if (sde_form_) {
      event_sink_->DispEventUnadvise(sde_form_.Get());
    }

    event_sink_->DetachDocument();
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

void ModusDocument::PostInit() {
  EnableTopology(profile_.modus.topology);

  connections_.emplace_back(profile_.AddChangeObserver(
      [this] { EnableTopology(profile_.modus.topology); }));
}

void ModusDocument::EnableTopology(bool enable) {
  {
    Microsoft::WRL::ComPtr<SDECore::ISDEPages> pages;
    sde_document_->get_Pages(pages.ReleaseAndGetAddressOf());

    if (pages) {
      long page_count = 0;
      pages->get_Count(&page_count);

      for (long i = 0; i < page_count; ++i) {
        Microsoft::WRL::ComPtr<SDECore::ISDEPage50> page;
        pages->get_Item(base::win::ScopedVariant{i},
                        page.ReleaseAndGetAddressOf());

        if (page) {
          page->put_UseTopology(AsVariantBool(enable));
        }
      }
    }
  }

  {
    Microsoft::WRL::ComPtr<IDispatch> electric_model_dispatch;
    sde_form_->get_ElectricModel(
        electric_model_dispatch.ReleaseAndGetAddressOf());

    if (electric_model_dispatch) {
      Microsoft::WRL::ComPtr<sde_electric::IElectricModel2> electric_model;
      electric_model_dispatch->QueryInterface(
          electric_model.ReleaseAndGetAddressOf());

      if (electric_model) {
        electric_model->put_Active(AsVariantBool(enable));
        electric_model->put_Mode(enable ? sde_electric::emOnUpdate
                                        : sde_electric::emNone);
      }
    }
  }
}

bool ModusDocument::ShowContainedItem(const scada::NodeId& node_id) {
  auto* object = FindObject(node_id);
  if (!object)
    return false;

  sde_document_->DocHighLight(&object->sde_object(), RGB(0, 255, 0), false);

  if (!object->elements().empty()) {
    selection_callback_(object->elements()[0]->timed_data());
  }

  return true;
}

modus::ModusObject* ModusDocument::FindObject(const scada::NodeId& node_id) {
  for (auto& object : objects_) {
    for (auto& element : object->elements()) {
      if (element->timed_data().node_id() == node_id)
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
    if (!hyperlink.empty()) {
      navigation_callback_(base::AsString16(hyperlink));
    }
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

void ModusDocument::OnDocNavigate(const QString& file_name,
                                  const QString& page_name,
                                  const QString& view_ident,
                                  bool& perform) {
  file_name;
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
