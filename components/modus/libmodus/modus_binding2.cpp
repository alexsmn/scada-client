#include "modus/libmodus/modus_binding2.h"

#include "base/utf_convert.h"
#include "common/node_state.h"
#include "libmodus/render/shape.h"
#include "libmodus/scheme/element.h"
#include "libmodus/scheme/value.h"
#include "modus/libmodus/modus_module2.h"
#include "modus/libmodus/modus_style_library2.h"

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
      return UtfConvert<wchar_t>(v.as_string());
    case scada::Variant::NODE_ID:
      return {};
    default:
      assert(false);
      return {};
  }
}

}  // namespace modus

template <typename T>
inline bool UpdateValue(T& value, const T& new_value) {
  if (value == new_value)
    return false;
  value = new_value;
  return true;
}

ModusBinding2::ModusBinding2(Delegate& delegate,
                             modus::Shape& shape,
                             const std::wstring& binding,
                             TimedDataService& timed_data_service)
    : delegate_(delegate), shape_(shape), styles_(0) {
  data_point_.property_change_handler = [this](const PropertySet& properties) {
    if (Update())
      delegate_.SchedulePaintShape(shape_);
  };

  data_point_.event_change_handler = [this] {
    if (Update())
      delegate_.SchedulePaintShape(shape_);
  };

  std::wstring formula = binding;
  auto p = formula.find(L'=');
  if (p != std::wstring::npos) {
    property_name_ = UtfConvert<char>(formula.substr(0, p));
    formula = formula.substr(p + 1);
  }

  data_point_.Connect(timed_data_service, UtfConvert<char>(formula));

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
      auto value =
          data_value.type() == scada::Variant::DOUBLE
              ? modus::Value(static_cast<float>(data_value.as_double()))
              : modus::Value();
      shape_.element().SetValue("Tech.value", value);
    } else {
      bool closed = data_point_.current().value.get_or(false);
      shape_.element().SetValue("Tech.closed", closed);
    }

  } else {
    shape_.element().SetValue(property_name_,
                              modus::ToValue(data_point_.current().value));
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

void ModusBinding2::OnAnimationStep() {
  delegate_.SchedulePaintShape(shape_);
}
