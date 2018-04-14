#include "components/modus/views/modus_loader.h"

#include "base/strings/string_split.h"
#include "base/strings/sys_string_conversions.h"
#include "common/node_service.h"
#include "components/modus/modus.h"
#include "components/modus/views/modus_element.h"
#include "components/modus/views/modus_object.h"
#include "components/modus/views/modus_view.h"
#include "address_space/address_space_util.h"
#include "services/file_cache_updater.h"
#include "views/client_utils_views.h"
#include "window_info.h"

namespace modus {

ModusLoader::ModusLoader(ModusLoaderContext&& context)
    : ModusLoaderContext{std::move(context)} {}

void ModusLoader::Load(SDECore::ISDEDocument50& document,
                       const base::FilePath& path,
                       ModusView* view) {
  view_ = view;

  {
    base::win::ScopedBstr bstr;
    document.get_Title(bstr.Receive());
    if (bstr && bstr != L"без имени")
      title_ = bstr;
  }

  if (title_.empty()) {
    base::win::ScopedBstr bstr;
    document.get_Name(bstr.Receive());
    if (bstr)
      title_ = bstr;
  }

  if (title_.empty())
    title_ = path.BaseName().RemoveExtension().value();

  cache_updater_ = FileCacheUpdater::Create(FileCacheUpdaterContext{
      VIEW_TYPE_MODUS,
      FullFilePathToPublic(path),
      title_,
      alias_resolver_,
      file_cache_,
  });

  base::win::ScopedComPtr<SDECore::ISDEPage50> page;
  document.get_CurrentPage(page.Receive());
  if (page) {
    base::win::ScopedComPtr<SDECore::ISDEObjects2> objects;
    page->get_SDEObjects(objects.Receive());
    if (objects)
      LoadObjects(*objects);
  }
}

void ModusLoader::AddElement(ModusObject*& object,
                             SDECore::ISDEObject50& sde_object,
                             SDECore::IParams& params,
                             const base::string16& binding,
                             long object_tag) {
  DCHECK(!binding.empty());

  // determine object type
  base::string16 prop_name;
  std::string formula;

  size_t p = binding.find(L'=');
  if (p == base::string16::npos) {
    if (HasParam(params, kParameterState))
      prop_name = static_cast<const VARIANT&>(kParameterState).bstrVal;
    else if (HasParam(params, kParameterValue))
      prop_name = static_cast<const VARIANT&>(kParameterValue).bstrVal;
    else if (HasParam(params, kParameterText))
      prop_name = static_cast<const VARIANT&>(kParameterText).bstrVal;
    formula = base::SysWideToNativeMB(binding);
  } else {
    prop_name = binding.substr(0, p);
    formula = base::SysWideToNativeMB(binding.substr(p + 1));
  }

  if (view_) {
    if (!object)
      object = new ModusObject(sde_object);

    ModusElement* element = new ModusElement(*object, params, prop_name);
    element->timed_data().Connect(timed_data_service_, formula);
    object->AddElement(*element);
  }

  // update index
  if (object_tag != -1)
    cache_updater_->Add(formula, object_tag);
}

void ModusLoader::AddObject(SDECore::ISDEObject50& sde_object) {
  base::win::ScopedComPtr<SDECore::INamedPBs> techs;
  sde_object.get_Techs(techs.Receive());
  if (!techs)
    return;

  long tag = -1;
  sde_object.get_Tag(&tag);

  long id = -1;
  sde_object.get_RTID(&id);

  long count = 0;
  techs->get_Count(&count);

  ModusObject* object = NULL;

  for (long i = 0; i < count; ++i) {
    base::win::ScopedComPtr<SDECore::INamedPB> named_pb;
    techs->get_Item(base::win::ScopedVariant(i), named_pb.Receive());

    SDEParams params;
    named_pb->get_Params(params.Receive());
    if (!params)
      continue;

    // key
    base::string16 bindings = GetParamValue(*params, kParameterBinding);
    if (bindings.empty())
      continue;

    auto binding_list = base::SplitString(bindings, L";", base::TRIM_WHITESPACE,
                                          base::SPLIT_WANT_NONEMPTY);
    for (auto& binding : binding_list)
      AddElement(object, sde_object, *params, binding, tag);
  }

  if (object)
    object->Init();

  if (view_ && object) {
    view_->objects_.push_back(object);

    // TODO: ModusObject should do this.
    if (id != -1)
      view_->object_map_[id] = object;
  }
}

void ModusLoader::LoadObjects(SDECore::ISDEObjects2& objects) {
  long count = 0;
  objects.get_Count(&count);
  for (long i = 0; i < count; i++) {
    base::win::ScopedComPtr<SDECore::ISDEObject50> object;
    objects.get_Item(base::win::ScopedVariant(i), object.Receive());
    if (!object)
      continue;

    AddObject(*object);

    // Process child objects.
    base::win::ScopedComPtr<SDECore::ISDEObjects2> children;
    object->get_Elements(children.Receive());
    if (children)
      LoadObjects(*children);
  }
}

}  // namespace modus
