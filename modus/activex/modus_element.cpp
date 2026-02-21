#include "modus/activex/modus_element.h"

#include "base/format.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util_win.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/scoped_bstr.h"
#include "events/event_set.h"
#include "model/data_items_node_ids.h"
#include "model/scada_node_ids.h"
#include "modus/activex/modus_object.h"

namespace modus {

const base::win::ScopedVariant kParameterBinding(OLESTR("ключ_привязки"));
const base::win::ScopedVariant kParameterText(OLESTR("текст"));
const base::win::ScopedVariant kParameterValue(OLESTR("значение_базовое"));
const base::win::ScopedVariant kParameterState(OLESTR("положение"));
const base::win::ScopedVariant kParameterStyle(OLESTR("композитный_стиль"));
const base::win::ScopedVariant kParameterHyperlink(OLESTR("гиперссылка"));
const base::win::ScopedVariant kParameterLimits(OLESTR("уставки"));

const wchar_t kStateClose[] = L"включен";
const wchar_t kStateOpen[] = L"отключен";

const double kNoLimit = std::numeric_limits<double>::max();

SDEParam GetParam(ISDEParams& params, const VARIANT& index) {
  SDEParam param;
  params.get_Item(index, param.GetAddressOf());
  return param;
}

bool HasParam(ISDEParams& params, const VARIANT& index) {
  SDEParam param = GetParam(params, index);
  if (!param)
    return false;

  base::win::ScopedBstr name;
  param->get_Name(name.Receive());
  return name != nullptr;
}

std::wstring GetParamValue(ISDEParams& params, const VARIANT& index) {
  SDEParam param = GetParam(params, index);
  if (!param)
    return std::wstring();

  base::win::ScopedBstr val;
  if (FAILED(param->get_Value(val.Receive())) || !val)
    return std::wstring();

  return static_cast<const wchar_t*>(val);
}

bool SetParamValue(ISDEParams& params, const VARIANT& index, BSTR value) {
  SDEParam param = GetParam(params, index);
  return param ? SUCCEEDED(param->put_Value(value)) : false;
}

bool SetParamValue(ISDEParams& params, std::wstring_view index, BSTR value) {
  base::win::ScopedVariant index_variant{index.data(),
                                         static_cast<UINT>(index.size())};
  return SetParamValue(params, index_variant, value);
}

std::wstring GetHyperlink(ISDEObject& object) {
  SDEParams params;
  object.get_Params(params.GetAddressOf());
  if (!params)
    return std::wstring();

  SDEParam param = GetParam(*params.Get(), kParameterHyperlink);
  if (!param)
    return std::wstring();

  long size;
  if (FAILED(param->get_Dim(&size)) || size == 0)
    return std::wstring();

  base::win::ScopedBstr value;
  if (FAILED(param->get_IndexedValue(1, value.Receive())))
    return std::wstring();

  return static_cast<const wchar_t*>(value);
}

inline std::string StrTok(std::string& str, const char* delimiters) {
  std::string::size_type p = str.find_first_of(delimiters);
  std::string res = str.substr(0, p);
  p = str.find_first_not_of(delimiters, p + 1);
  str.erase(0, p);
  return res;
}

Limits GetLimits(const NodeRef& node) {
  return {
      node[data_items::id::AnalogItemType_LimitLoLo].value().get_or(kNoLimit),
      node[data_items::id::AnalogItemType_LimitLo].value().get_or(kNoLimit),
      node[data_items::id::AnalogItemType_LimitHi].value().get_or(kNoLimit),
      node[data_items::id::AnalogItemType_LimitHiHi].value().get_or(kNoLimit),
  };
}

std::wstring_view ToString(Limit limit) {
  static const std::wstring_view kStrings[] = {L"мин_аларм", L"мин_уставка",
                                               L"макс_уставка", L"макс_аларм"};
  static_assert(std::size(kStrings) == static_cast<int>(Limit::Count),
                "Wrong limits");
  auto index = static_cast<size_t>(limit);
  assert(index < std::size(kStrings));
  return kStrings[index];
}

std::wstring GetLimitSetString(const Limits& limits) {
  std::wstring result;
  for (int i = 0; i < static_cast<int>(Limit::Count); ++i) {
    if (limits.limits[i] != kNoLimit) {
      if (!result.empty())
        result += u',';
      auto limit_string = ToString(static_cast<Limit>(i));
      result.append(limit_string.data(), limit_string.size());
    }
  }
  return base::StrCat({L"[", result, L"]"});
}

inline bool operator==(const Limits& left, const Limits& right) {
  return std::equal(std::begin(left.limits), std::end(left.limits),
                    std::begin(right.limits));
}

inline bool operator!=(const Limits& left, const Limits& right) {
  return !(left == right);
}

// ModusElement

ModusElement::ModusElement(ModusElementContext&& context)
    : ModusElementContext{std::move(context)} {
  data_spec_.property_change_handler = [this](const PropertySet& properties) {
    UpdateData(false);
  };
  data_spec_.node_modified_handler = [this] { UpdateData(false); };
  data_spec_.deletion_handler = [this] { UpdateData(false); };
  data_spec_.event_change_handler = [this] { UpdateData(false); };
}

void ModusElement::Init() {
  UpdateData(true);
}

void ModusElement::UpdateData(bool init) {
  style_ &= ~MODUS_BADQ;
  style_ |= MODUS_INVAL;

  if (data_spec_.connected()) {
    style_ &= ~MODUS_INVAL;

    const auto& current = data_spec_.current();

    double value = current.value.get_or(0.0);
    if (init || value != value_) {
      value_ = value;

      std::wstring text;
      if (!state_strings_.empty()) {
        bool state = std::abs(value_) >= std::numeric_limits<double>::epsilon();
        size_t state_no = state ? 1 : 0;
        text = (state_no < state_strings_.size())
                   ? state_strings_[state_no ? 1 : 0]
                   : std::wstring{};
        if (text.empty())
          text = state ? kStateClose : kStateOpen;

      } else {
        text = base::AsWString(WideFormat(value_));
      }

      SetParamValue(*sde_params_.Get(), prop_name_,
                    base::win::ScopedBstr(text));
    }

    // bad quality
    if (current.qualifier.general_bad())
      style_ |= MODUS_BADQ;

    // inactive
    /*if (entry.rec->subs == NULL_UINT8 || !entry.rec->subs)
      entry.style |= MODUS_INACT;*/

    // limits
    if (has_limits_) {
      auto node = data_spec_.node();
      auto limits = node ? GetLimits(node) : Limits{};
      if (init || limits_ != limits) {
        limits_ = limits;
        auto limit_set_string = GetLimitSetString(limits_);
        SetParamValue(*sde_params_.Get(), kParameterLimits,
                      base::win::ScopedBstr(limit_set_string));
        for (size_t i = 0; i < static_cast<size_t>(Limit::Count); ++i) {
          if (limits.limits[i] != kNoLimit) {
            SetParamValue(*sde_params_.Get(), ToString(static_cast<Limit>(i)),
                          base::win::ScopedBstr(
                              base::UTF16ToWide(WideFormat(limits.limits[i]))));
          }
        }
      }
    }
  }

  const EventSet* events = data_spec_.GetEvents();
  if (events && !events->empty())
    style_ |= MODUS_ALERT;
  else
    style_ &= ~MODUS_ALERT;

  if (!init)
    object_.UpdateStyle(false);
}

}  // namespace modus
