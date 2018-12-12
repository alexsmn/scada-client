#pragma once

#include "timed_data/timed_data_spec.h"

#include <schematic/element.h>

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
