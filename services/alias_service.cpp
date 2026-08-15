#include "alias_service.h"

#include "base/boost_log.h"
#include "base/check.h"
#include "model/data_items_node_ids.h"
#include "model/scada_node_ids.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"

AliasService::AliasService(AliasServiceContext&& context)
    : AliasServiceContext{std::move(context)} {
  aliases_ = node_service_.GetNode(scada::data_items::id::Aliases);

  if (logger_)
    LOG_INFO(*logger_) << "Fetching";

  node_fetched_connection_ = node_service_.SubscribeNodeFetched(
      [this](const NodeFetchedEvent& event) { OnNodeFetched(event); });
  aliases_.StartFetch(NodeFetchStatus::NodeAndChildren);
  if (aliases_.children_fetched())
    OnFetchCompleted();
}

AliasService::~AliasService() = default;

void AliasService::Resolve(std::string_view alias,
                           const AliasResolveCallback& callback) {
  auto alias_string = std::string{alias};

  if (!fetched_) {
    if (logger_)
      LOG_INFO(*logger_) << std::format("Pending resolution: {}", alias_string);
    pending_aliases_[std::move(alias_string)].emplace_back(callback);
    return;
  }

  auto node_id = ResolveNow(alias_string);
  auto status_code =
      node_id.is_null() ? scada::StatusCode::Bad : scada::StatusCode::Good;
  // Passed by reference, not moved: AliasResolveCallback's second parameter is
  // `const scada::NodeId&`. A `std::move` here would bind the xvalue to that
  // const reference and move nothing, so it only misstates what happens -- and
  // in OnFetchCompleted below, where one resolution feeds several callbacks, it
  // would become a real defect the day the signature took the id by value
  // (NodeId's move constructor nulls its source; see core/scada/node_id.h).
  callback(status_code, node_id);
}

void AliasService::OnFetchCompleted() {
  scada::base::Check(!fetched_);

  if (logger_)
    LOG_INFO(*logger_) << std::format("Fetch completed. {} aliases fetched",
                                      aliases_.targets(scada::id::Organizes).size());
  fetched_ = true;

  for (const auto& [alias, callbacks] : pending_aliases_) {
    auto node_id = ResolveNow(alias);
    auto status_code =
        node_id.is_null() ? scada::StatusCode::Bad : scada::StatusCode::Good;
    // One resolved id delivered to every callback waiting on this alias; see
    // the note in Resolve above for why it is not moved.
    for (const auto& callback : callbacks)
      callback(status_code, node_id);
  }

  pending_aliases_.clear();
}

void AliasService::OnNodeFetched(const NodeFetchedEvent& event) {
  if (!fetched_ && event.node_id == aliases_.node_id() &&
      aliases_.children_fetched()) {
    OnFetchCompleted();
  }
}

scada::NodeId AliasService::ResolveNow(const std::string& alias) const {
  auto alias_node = aliases_[alias];
  auto aliased_node = alias_node.target(scada::data_items::id::AliasOf);
  auto aliased_node_id = aliased_node.node_id();

  if (logger_)
    LOG_INFO(*logger_) << std::format("{} = {}", alias,
                                      aliased_node_id.ToString());

  return aliased_node_id;
}
