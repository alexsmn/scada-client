#pragma once

#include "components/write/write_dialog.h"
#include "components/write/write_service.h"

struct WriteServiceImplContext {
  std::shared_ptr<Executor> executor_;
  TimedDataService& timed_data_service_;
  Profile& profile_;
};

class WriteServiceImpl final : private WriteServiceImplContext,
                               public WriteService {
 public:
  explicit WriteServiceImpl(WriteServiceImplContext&& context)
      : WriteServiceImplContext{std::move(context)} {}

  virtual void ExecuteWriteDialog(DialogService& dialog_service,
                                  const scada::NodeId& node_id,
                                  bool manual) override {
    ::ExecuteWriteDialog(
        dialog_service, WriteContext{.executor_ = executor_,
                                     .timed_data_service_ = timed_data_service_,
                                     .node_id_ = node_id,
                                     .profile_ = profile_,
                                     .manual_ = manual});
  }

 private:
};
