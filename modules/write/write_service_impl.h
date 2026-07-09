#pragma once

#include "base/any_executor.h"

#include "base/awaitable.h"
#include "modules/write/write_dialog.h"
#include "modules/write/write_service.h"

struct WriteServiceImplContext {
  AnyExecutor executor_;
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
    // `::ExecuteWriteDialog` returns a lazy awaitable — spawn it detached so
    // the dialog actually opens.
    CoSpawn(executor_, [this, &dialog_service, node_id, manual]() {
      return ::ExecuteWriteDialog(
          dialog_service,
          WriteContext{.executor_ = executor_,
                       .timed_data_service_ = timed_data_service_,
                       .node_id_ = node_id,
                       .profile_ = profile_,
                       .manual_ = manual});
    });
  }

 private:
};
