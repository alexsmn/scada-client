#pragma once

#include "vidicon/teleclient/data_point_manager.h"

#include <memory>

class Executor;
class TimedDataService;

namespace vidicon {

// Must be constructed and destructed from the main thread.
class DataPointManagerImpl : public DataPointManager {
 public:
  DataPointManagerImpl(std::shared_ptr<Executor> executor,
                       TimedDataService& timed_data_service);
  ~DataPointManagerImpl();

  virtual void Subscribe(const DataPointAddress& address,
                         std::stop_token cancelation,
                         const DataChangeHandler& handler) override;

 private:
  // Constructs and runs under `executor_`.
  struct Backend;

  const std::shared_ptr<Executor> executor_;
  std::unique_ptr<Backend> backend_;
};

}  // namespace vidicon
