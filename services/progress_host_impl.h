#pragma once

#include "services/progress_host.h"

#include <boost/signals2/signal.hpp>

class ProgressHostImpl : public ProgressHost {
 public:
  // ProgressHost
  virtual std::unique_ptr<RunningProgress> Start() override;
  virtual boost::signals2::scoped_connection Subscribe(
      const ProgressCallback& callback) override;

 private:
  class RunningProgressImpl;

  void RemoveRunningProgressImpl(RunningProgressImpl& running_progress_impl);

  void UpdateProgressStatus();

  ProgressStatus progress_status_;

  std::vector<RunningProgressImpl*> running_progress_impls_;

  boost::signals2::signal<void(const ProgressStatus& status)> signal_;
};
