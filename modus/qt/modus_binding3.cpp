#include "modus_binding3.h"

#include "base/string_util.h"

namespace {

enum class ModusStyle { Disconnected, BadQuality, Alerting };

class ModusStyles {
 public:
  ModusStyles() {}
  ModusStyles(ModusStyle value) : value_{1u << static_cast<unsigned>(value)} {}

  explicit operator bool() const { return value_ != 0; }
  bool operator!() const { return value_ == 0; }

  ModusStyles operator&(ModusStyles other) const {
    return ModusStyles{value_ & other.value_};
  }
  ModusStyles operator|(ModusStyles other) const {
    return ModusStyles{value_ | other.value_};
  }

  ModusStyles& operator&=(ModusStyles other) {
    value_ &= other.value_;
    return *this;
  }
  ModusStyles& operator|=(ModusStyles other) {
    value_ |= other.value_;
    return *this;
  }

 private:
  explicit ModusStyles(unsigned value) : value_{value} {}

  unsigned value_ = 0;
};

void AppendStyleString(std::wstring& str, const std::wstring_view style) {
  if (!str.empty())
    str += L',';
  str.append(style);
}

}  // namespace

ModusBinding3::ModusBinding3(Schematic::Element& element) : element_{element} {}

void ModusBinding3::Bind(const QString& property_name,
                         TimedDataService& timed_data_service,
                         const QString& formula) {
  auto& spec = bindings_[property_name];
  spec.property_change_handler = [this](const PropertySet& properties) {
    Update();
  };
  auto formula_string = formula.toStdString();
  if (IsStringASCII(formula_string))
    spec.Connect(timed_data_service, formula_string);
}

void ModusBinding3::Update() {
  ModusStyles styles;

  for (auto& [property_name, spec] : bindings_) {
    if (!spec.connected()) {
      styles |= ModusStyle::Disconnected;
      continue;
    }

    const auto& current = spec.current();

    if (spec.alerting())
      styles |= ModusStyle::Alerting;

    if (current.is_null() || current.qualifier.general_bad())
      styles |= ModusStyle::BadQuality;

    element_.Set(property_name,
                 QString::fromStdString(ToString(current.value)));
  }

  std::wstring str;
  if (styles & ModusStyle::Disconnected)
    AppendStyleString(str, L"НЕСуществующийТС");
  if (styles & ModusStyle::Alerting)
    AppendStyleString(str, L"НеКвитировано");
  if (styles & ModusStyle::BadQuality)
    AppendStyleString(str, L"BadQuality");
  str.insert(0, 1, L'[');
  str.append(1, L']');

  element_.Set("Styles", QString::fromStdWString(str));
}
