#pragma once

#include "scada/attribute_service.h"

#include <boost/signals2/signal.hpp>
#include <unordered_map>

class VariableStorage {
 public:
  void AddVariable(const std::string& name) {
    variable_values.try_emplace(name);
  }

  void SetVariableValue(const std::string& name, scada::Variant value) {
    auto data_value = scada::MakeReadResult(std::move(value));
    variable_values.insert_or_assign(name, data_value);
    variable_value_changed_(name, data_value);
  }

  template <class U>
  boost::signals2::scoped_connection SubscribeValueChanged(U&& callback) {
    return variable_value_changed_.connect(std::forward<U>(callback));
  }

  std::unordered_map<std::string, scada::DataValue> variable_values;

  boost::signals2::signal<void(const std::string& name,
                               const scada::DataValue& new_data_value)>
      variable_value_changed_;
};
