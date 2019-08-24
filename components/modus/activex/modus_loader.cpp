#include "components/modus/activex/modus_loader.h"

#include "base/strings/string_split.h"
#include "base/strings/sys_string_conversions.h"
#include "client_utils.h"
#include "common_resources.h"
#include "components/modus/activex/modus.h"
#include "components/modus/activex/modus_document.h"
#include "components/modus/activex/modus_element.h"
#include "components/modus/activex/modus_object.h"
#include "core/debug_util.h"
#include "services/file_cache_updater.h"
#include "window_info.h"

#include "core/debug_util-inl.h"

namespace modus {

namespace {

std::wstring GetShortPath(SDECore::ISDEObject50& sde_object) {
  base::win::ScopedBstr result;
  sde_object.get_ShortPath(result.Receive());
  return static_cast<const wchar_t*>(result);
}

std::vector<base::string16> GetStateStrings(SDECore::IParams& params,
                                            base::StringPiece16 param_name) {
  SDEParam param = GetParam(
      params, base::win::ScopedVariant(param_name.data(), param_name.size()));
  if (!param)
    return {};

  base::win::ScopedBstr values;
  if (FAILED(param->get_Values(values.Receive())))
    return {};
  if (!values)
    return {};

  return base::SplitString(base::string16(values), L";", base::TRIM_WHITESPACE,
                           base::SPLIT_WANT_NONEMPTY);
}

}  // namespace

ModusLoader::ModusLoader(ModusLoaderContext&& context)
    : ModusLoaderContext{std::move(context)} {}

void ModusLoader::Load(SDECore::ISDEDocument50& sde_document,
                       const base::FilePath& path,
                       ModusDocument* document) {
  LOG_INFO(logger_) << "Load" << LOG_TAG("Path", path.value());

  document_ = document;

  {
    base::win::ScopedBstr bstr;
    sde_document.get_Title(bstr.Receive());
    if (bstr && bstr != L"без имени")
      title_ = bstr;
  }

  if (title_.empty()) {
    base::win::ScopedBstr bstr;
#ifdef NDEBUG
    sde_document.get_Name(bstr.Receive());
#endif
    if (bstr)
      title_ = bstr;
  }

  if (title_.empty())
    title_ = path.BaseName().RemoveExtension().value();

  LOG_INFO(logger_) << "Load title" << LOG_TAG("Title", title_);

  cache_updater_ = FileCacheUpdater::Create(FileCacheUpdaterContext{
      ID_MODUS_VIEW,
      FullFilePathToPublic(path),
      title_,
      alias_resolver_,
      file_cache_,
  });

  Microsoft::WRL::ComPtr<SDECore::ISDEPage50> page;
  sde_document.get_CurrentPage(page.GetAddressOf());
  if (page) {
    Microsoft::WRL::ComPtr<SDECore::ISDEObjects2> objects;
    page->get_SDEObjects(objects.GetAddressOf());
    if (objects)
      LoadObjects(*objects.Get());
  }

  LOG_INFO(logger_) << "Load completed";
}

base::string16 GetPropName(SDECore::IParams& params) {
  if (HasParam(params, kParameterState))
    return static_cast<const VARIANT&>(kParameterState).bstrVal;
  else if (HasParam(params, kParameterValue))
    return static_cast<const VARIANT&>(kParameterValue).bstrVal;
  else if (HasParam(params, kParameterText))
    return static_cast<const VARIANT&>(kParameterText).bstrVal;
  else
    return {};
}

void ModusLoader::LoadElement(std::unique_ptr<ModusObject>& object,
                              SDECore::ISDEObject50& sde_object,
                              SDECore::IParams& initial_params,
                              const base::string16& binding,
                              long object_tag,
                              long tech_index) {
  DCHECK(!binding.empty());

  // determine object type
  base::string16 prop_name;
  std::string formula;

  SDEParams params = &initial_params;

  size_t p = binding.find(L'=');
  if (p == base::string16::npos) {
    prop_name = GetPropName(*params.Get());

    // WORKAROUND: Elements like "Tablo" don't provide valid attributes through
    // tech params. Use object params in that case.
    if (prop_name.empty() && tech_index == 0) {
      SDEParams object_params;
      sde_object.get_Params(object_params.ReleaseAndGetAddressOf());
      if (object_params) {
        prop_name = GetPropName(*object_params.Get());
        if (!prop_name.empty()) {
          LOG_WARNING(logger_) << "Substitute tech params by object params"
                               << LOG_TAG("ShortPath", GetShortPath(sde_object))
                               << LOG_TAG("Tag", object_tag);
          params = std::move(object_params);
        }
      }
    }

    if (prop_name.empty()) {
      LOG_WARNING(logger_) << "Can't recognize element type"
                           << LOG_TAG("ShortPath", GetShortPath(sde_object))
                           << LOG_TAG("Tag", object_tag);
      return;
    }

    formula = base::SysWideToNativeMB(binding);

  } else {
    prop_name = binding.substr(0, p);
    formula = base::SysWideToNativeMB(binding.substr(p + 1));

    // WORKAROUND: Use object params to access to enumerable types.
    if (tech_index == 0) {
      SDEParams object_params;
      sde_object.get_Params(object_params.ReleaseAndGetAddressOf());
      if (object_params) {
        LOG_WARNING(logger_) << "Substitute tech params by object params"
                             << LOG_TAG("ShortPath", GetShortPath(sde_object))
                             << LOG_TAG("Tag", object_tag);
        params = std::move(object_params);
      }
    }
  }

  if (document_) {
    if (!object) {
      LOG_INFO(logger_) << "Create object"
                        << LOG_TAG("ShortPath", GetShortPath(sde_object))
                        << LOG_TAG("Tag", object_tag);
      object = std::make_unique<ModusObject>(sde_object);
    }

    auto state_strings = GetStateStrings(*params.Get(), prop_name);
    auto has_limits = HasParam(*params.Get(), kParameterLimits);

    LOG_INFO(logger_) << "Create element" << LOG_TAG("PropName", prop_name)
                      << LOG_TAG("Formula", formula)
                      << LOG_TAG("StateStrings", ToString(state_strings))
                      << LOG_TAG("HasLimits", has_limits);

    ModusElement* element = new ModusElement(ModusElementContext{
        *object, std::move(params), prop_name, state_strings, has_limits});
    element->timed_data().Connect(timed_data_service_, formula);
    object->AddElement(*element);
  }

  // update index
  if (object_tag != -1)
    cache_updater_->Add(formula, object_tag);
}

void ModusLoader::LoadObject(SDECore::ISDEObject50& sde_object) {
  LOG_INFO(logger_) << "Processing object"
                    << LOG_TAG("ShortPath", GetShortPath(sde_object));

  Microsoft::WRL::ComPtr<SDECore::INamedPBs> techs;
  sde_object.get_Techs(techs.GetAddressOf());
  if (!techs) {
    LOG_WARNING(logger_) << "No techs";
    return;
  }

  long tag = -1;
  sde_object.get_Tag(&tag);

  long id = -1;
  sde_object.get_RTID(&id);

  long count = 0;
  techs->get_Count(&count);

  LOG_INFO(logger_) << "Object info" << LOG_TAG("Tag", tag)
                    << LOG_TAG("RTID", id) << LOG_TAG("TechCount", count);

  std::unique_ptr<ModusObject> object;

  for (long i = 0; i < count; ++i) {
    Microsoft::WRL::ComPtr<SDECore::INamedPB> named_pb;
    techs->get_Item(base::win::ScopedVariant(i), named_pb.GetAddressOf());

    SDEParams params;
    named_pb->get_Params(params.GetAddressOf());
    if (!params) {
      LOG_WARNING(logger_) << "Tech has no params" << LOG_TAG("Index", i);
      continue;
    }

    // key
    base::string16 bindings = GetParamValue(*params.Get(), kParameterBinding);
    if (bindings.empty())
      continue;

    LOG_INFO(logger_) << "Processing tech" << LOG_TAG("Index", i)
                      << LOG_TAG("Bindings", bindings);

    auto binding_list = base::SplitString(bindings, L";", base::TRIM_WHITESPACE,
                                          base::SPLIT_WANT_NONEMPTY);
    for (auto& binding : binding_list)
      LoadElement(object, sde_object, *params.Get(), binding, tag, i);

    LOG_INFO(logger_) << "Processing tech completed";
  }

  if (object)
    object->Init();

  if (document_ && object) {
    auto& added_object = document_->objects_.emplace_back(std::move(object));

    if (id != -1)
      document_->object_map_[id] = added_object.get();
  }

  LOG_INFO(logger_) << "Processing object completed";
}

void ModusLoader::LoadObjects(SDECore::ISDEObjects2& objects) {
  long count = 0;
  objects.get_Count(&count);
  for (long i = 0; i < count; i++) {
    Microsoft::WRL::ComPtr<SDECore::ISDEObject50> object;
    objects.get_Item(base::win::ScopedVariant(i), object.GetAddressOf());
    if (!object)
      continue;

    LoadObject(*object.Get());

    // Process child objects.
    Microsoft::WRL::ComPtr<SDECore::ISDEObjects2> children;
    object->get_Elements(children.GetAddressOf());
    if (children)
      LoadObjects(*children.Get());
  }
}

}  // namespace modus
