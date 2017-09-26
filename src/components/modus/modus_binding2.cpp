#include "components/modus/modus_binding2.h"

#include "base/strings/sys_string_conversions.h"
#include "libmodus/render/shape.h"
#include "libmodus/scheme/element.h"
#include "libmodus/scheme/value.h"
#include "core/configuration_types.h"
#include "components/modus/modus_module2.h"
#include "components/modus/modus_style_library2.h"
#include "ui/gfx/canvas.h"
#include "translation.h"

namespace modus {

Value ToValue(const scada::Variant& v) {
  switch (v.type()) {
    case scada::Variant::EMPTY:
      return {};
    case scada::Variant::BOOL:
      return v.as_bool();
    case scada::Variant::INT32:
    case scada::Variant::INT64:
    case scada::Variant::DOUBLE: {
      double d;
      if (v.get(d))
        return static_cast<Value::Scalar>(d);
      else
        return {};
    }
    case scada::Variant::STRING:
      return base::SysNativeMBToWide(v.as_string());
    case scada::Variant::LOCALIZED_TEXT:
      return ToString16(v.as_localized_text());
    case scada::Variant::NODE_ID:
      return {};
    default:
      assert(false);
      return {};
  }
}

} // namespace modus

template<typename T>
inline bool UpdateValue(T& value, const T& new_value) {
  if (value == new_value)
    return false;
  value = new_value;
  return true;
}

ModusBinding2::ModusBinding2(Delegate& delegate, TimedDataService& timed_data_service, modus::Shape& shape, const std::wstring& binding)
    : delegate_(delegate),
      shape_(shape),
      styles_(0) {
  data_point_.set_delegate(this);

  std::wstring formula = binding;
  auto p = formula.find(L'=');
  if (p != std::wstring::npos) {
    property_name_ = base::SysWideToNativeMB(formula.substr(0, p));
    formula = formula.substr(p + 1);
  }

  try {
    data_point_.Connect(timed_data_service, base::SysWideToNativeMB(formula));
  } catch (const std::exception& e) {
    LOG(WARNING) << "Binding '" << binding << "' error: " << e.what();
  }

  Update();
}

ModusBinding2::~ModusBinding2() {
  SetStyles(0);
}

void ModusBinding2::Paint(Gdiplus::Graphics& graphics, bool background) {
  if (!styles_)
    return;

  auto bounds = shape_.bounds();
  Gdiplus::RectF rect(bounds.x(), bounds.y(), bounds.width(), bounds.height());
  if (!graphics.IsVisible(rect))
    return;

  auto& library = ModusModule2::GetInstance()->style_library();

  for (size_t i = 0; i < static_cast<size_t>(StyleId::COUNT); ++i) {
    if (styles_ & (1 << i)) {
      auto* style = library.GetStyle(static_cast<StyleId>(i));
      style->Paint(graphics, rect, background);
    }
  }  
}

bool ModusBinding2::Update() {
  if (property_name_.empty()) {
    if (shape_.element().GetValue("typ").as_scalar() == 134) {
      auto data_value = data_point_.current().value;
      auto value = data_value.type() == scada::Variant::DOUBLE ?
          modus::Value(static_cast<float>(data_value.as_double())) :
          modus::Value();
      shape_.element().SetValue("Tech.value", value);
    } else {
      bool closed = data_point_.current().value.get_or(false);
      shape_.element().SetValue("Tech.closed", closed);
    }

  } else {
    shape_.element().SetValue(property_name_, modus::ToValue(data_point_.current().value));
  }

  bool changed = false;

  unsigned styles = 0;
  if (!data_point_.connected())
    styles |= 1 << static_cast<unsigned>(StyleId::INVAL);
  if (data_point_.current().qualifier.general_bad())
    styles |= 1 << static_cast<unsigned>(StyleId::BADQ);
  if (data_point_.alerting())
    styles |= 1 << static_cast<unsigned>(StyleId::ALERT);
  changed |= SetStyles(styles);

  return changed;  
}

bool ModusBinding2::SetStyles(unsigned styles) {
  if (styles_ == styles)
    return false;

  auto& library = ModusModule2::GetInstance()->style_library();

  unsigned added = styles & ~styles_;
  unsigned removed = styles_ & ~styles;

  for (size_t i = 0; i < static_cast<size_t>(StyleId::COUNT); ++i) {
    auto* style = library.GetStyle(static_cast<StyleId>(i));
    auto mask = 1 << i;
    if (added & mask)
      style->AddAnimationObserver(*this);
    else if (removed & mask)
      style->RemoveAnimationObserver(*this);
  }

  styles_ = styles;
  return true;
}

void ModusBinding2::OnPropertyChanged(rt::TimedDataSpec& spec,
                                      const rt::PropertySet& properties) {
  if (Update())
    delegate_.SchedulePaintShape(shape_);
}

void ModusBinding2::OnEventsChanged(rt::TimedDataSpec& spec,
                                    const events::EventSet& events) {
  if (Update())
    delegate_.SchedulePaintShape(shape_);
}

void ModusBinding2::OnAnimationStep() {
  delegate_.SchedulePaintShape(shape_);
}