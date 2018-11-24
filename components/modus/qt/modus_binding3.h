#pragma once

#include "timed_data/timed_data_spec.h"

#include <schematic/element.h>

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

class ModusBinding3 {
 public:
  explicit ModusBinding3(Schematic::Element& element);

  void Bind(const QString& property_name,
            TimedDataService& timed_data_service,
            const QString& formula);

  void Update();

 private:
  Schematic::Element& element_;

  std::map<QString, rt::TimedDataSpec> bindings_;
};

inline ModusBinding3::ModusBinding3(Schematic::Element& element)
    : element_{element} {}

inline void ModusBinding3::Bind(const QString& property_name,
                                TimedDataService& timed_data_service,
                                const QString& formula) {
  auto& spec = bindings_[property_name];
  spec.property_change_handler = [this](const rt::PropertySet& properties) {
    Update();
  };
  spec.Connect(timed_data_service, formula.toStdString());
}

inline void ModusBinding3::Update() {
  ModusStyles styles;

  for (auto& [property_name, spec] : bindings_) {
    if (!spec.connected()) {
      styles |= ModusStyle::Disconnected;
      continue;
    }

    const auto& current = spec.current();

    if (spec.alerting())
      styles |= ModusStyle::Alerting;

    if (current.qualifier.general_bad())
      styles |= ModusStyle::BadQuality;

    element_.Set(property_name,
                 QString::fromStdString(ToString(current.value)));
  }

  // if (styles & ModusStyle::Alert)
}
