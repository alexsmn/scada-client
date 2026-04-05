#pragma once

#include "base/range_util.h"
#include "test/display_tester/qt/variable_storage.h"
#include "timed_data/base_timed_data.h"
#include "timed_data/timed_data_property.h"
#include "timed_data/timed_data_service.h"

#include "base/utf_convert.h"
#include <boost/range/adaptor/map.hpp>

class VariableTimedData : public BaseTimedData {
 public:
  explicit VariableTimedData(std::string formula)
      : formula_{std::move(formula)} {}

  virtual std::string GetFormula(bool aliases) const override {
    return formula_;
  }

  virtual scada::LocalizedText GetTitle() const {
    return UtfConvert<char16_t>(formula_);
  }

  void SetDataValue(const scada::DataValue& new_data_value) {
    if (UpdateCurrent(new_data_value)) {
      NotifyPropertyChanged(PropertySet(PROPERTY_CURRENT));
    }
  }

 private:
  const std::string formula_;
};

class VariableTimedDataService : public TimedDataService {
 public:
  explicit VariableTimedDataService(VariableStorage& variable_storage)
      : variable_storage_{variable_storage} {}

  virtual std::shared_ptr<TimedData> GetNodeTimedData(
      const scada::NodeId& node_id,
      const scada::AggregateFilter& aggregation) override {
    return nullptr;
  }

  virtual std::shared_ptr<TimedData> GetFormulaTimedData(
      std::string_view formula,
      const scada::AggregateFilter& aggregation) override {
    auto i = timed_data_map_.find(std::string{formula});
    if (i != timed_data_map_.end()) {
      return i->second;
    }

    variable_storage_.AddVariable(std::string{formula});

    auto timed_data = std::make_shared<VariableTimedData>(std::string{formula});
    timed_data_map_.try_emplace(std::string{formula}, timed_data);
    return timed_data;
  }

 private:
  VariableStorage& variable_storage_;

  std::unordered_map<std::string, std::shared_ptr<VariableTimedData>>
      timed_data_map_;

  boost::signals2::scoped_connection variable_subscription_ =
      variable_storage_.SubscribeValueChanged(
          [this](const std::string& name,
                 const scada::DataValue& new_data_value) {
            auto i = timed_data_map_.find(name);
            if (i != timed_data_map_.end()) {
              i->second->SetDataValue(new_data_value);
            }
          });
};
