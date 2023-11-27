#pragma once

#include "components/watch/watch_combined_event_source.h"
#include "components/watch/watch_current_event_source.h"
#include "components/watch/watch_history_event_source.h"
#include "components/watch/watch_model.h"

namespace {

struct WatchModelHolder {
  explicit WatchModelHolder(std::shared_ptr<Executor> executor,
                            NodeService& node_service)
      : executor_{std::move(executor)}, node_service{node_service} {}

  std::shared_ptr<Executor> executor_;
  NodeService& node_service;

  WatchCombinedEventSource combined_event_source{
      {std::make_shared<WatchCurrentEventSource>(
           WatchCurrentEventSourceContext{node_service}),
       std::make_shared<WatchHistoryEventSource>(
           WatchHistorySourceContext{executor_, node_service})}};

  WatchModel model{WatchModelContext{node_service, combined_event_source}};
};

}  // namespace

struct WatchModelBuilder {
  std::shared_ptr<WatchModel> CreateWatchModel() {
    auto holder = std::make_shared<WatchModelHolder>(executor_, node_service);
    return std::shared_ptr<WatchModel>(holder, &holder->model);
  }

  std::shared_ptr<Executor> executor_;
  NodeService& node_service;
};
