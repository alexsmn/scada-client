#include "modus/activex/modus_object.h"

#include "modus/activex/modus_element.h"

#include <string_view>

namespace modus {

ModusObject::ModusObject(SDECore::ISDEObject50& sde_object)
    : sde_object_(&sde_object), current_states_(0) {}

ModusObject::~ModusObject() {}

void ModusObject::Init() {
  for (const auto& element : elements_)
    element->Init();

  UpdateStyle(true);
}

void ModusObject::UpdateStyle(bool init) {
  unsigned states = 0;
  for (const auto& element : elements_)
    states |= element->style();

  if (init || current_states_ != states) {
    current_states_ = states;

    std::wstring style;
    if (current_states_ & ModusElement::MODUS_INVAL) {
      if (!style.empty())
        style += L',';
      style += L"НЕСуществующийТС";
    }
    if (current_states_ & ModusElement::MODUS_INACT) {
      if (!style.empty())
        style += L',';
      style += L"НЕАктивныйТС";
    }
    if (current_states_ & ModusElement::MODUS_BADQ) {
      if (!style.empty())
        style += L',';
      style += L"BadQuality";
    }
    if (current_states_ & ModusElement::MODUS_ALERT) {
      if (!style.empty())
        style += L',';
      style += L"НеКвитировано";
    }

    style.insert(style.begin(), L'[');
    style += L']';

    SDEParams params;
    sde_object().get_Params(params.GetAddressOf());
    if (params) {
      SetParamValue(*params.Get(), kParameterStyle,
                    base::win::ScopedBstr(style));
    }
  }
}

}  // namespace modus
