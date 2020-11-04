#include "alias_service.h"

#include "base/logger.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "model/data_items_node_ids.h"
#include "model/scada_node_ids.h"

AliasService::AliasService(AliasServiceContext&& context)
    : AliasServiceContext{std::move(context)} {
  aliases_ = node_service_.GetNode(data_items::id::Aliases);

  logger_->WriteF(LogSeverity::Normal, "Fetching");

  // TODO: weak ptr.
  aliases_.Fetch(NodeFetchStatus::NodeAndChildren(),
                 [this](const NodeRef& aliases) {
                   if (aliases.children_fetched())
                     OnFetchCompleted();
                 });

  node_service_.Subscribe(*this);
}

AliasService::~AliasService() {
  node_service_.Unsubscribe(*this);
}

void AliasService::Resolve(base::StringPiece alias,
                            const AliasResolveCallback& callback) {
  auto alias_string = alias.as_string();

  if (!fetched_) {
    logger_->WriteF(LogSeverity::Normal, "Pending resolution: %s",
                    alias_string.c_str());
    pending_aliases_[std::move(alias_string)].emplace_back(callback);
    return;
  }

  auto node_id = ResolveNow(alias_string);
  auto status_code =
      node_id.is_null() ? scada::StatusCode::Bad : scada::StatusCode::Good;
  callback(status_code, std::move(node_id));
}

void AliasService::OnFetchCompleted() {
  assert(!fetched_);

  logger_->WriteF(LogSeverity::Normal, "Fetch completed. %Iu aliases fetched",
                  aliases_.targets(scada::id::Organizes).size());
  fetched_ = true;

  for (const auto& [alias, callbacks] : pending_aliases_) {
    auto node_id = ResolveNow(alias);
    auto status_code =
        node_id.is_null() ? scada::StatusCode::Bad : scada::StatusCode::Good;
    for (const auto& callback : callbacks)
      callback(status_code, std::move(node_id));
  }

  pending_aliases_.clear();
}

scada::NodeId AliasService::ResolveNow(const std::string& alias) const {
  auto alias_node = aliases_[alias];
  auto aliased_node = alias_node.target(data_items::id::AliasOf);
  auto aliased_node_id = aliased_node.node_id();

  logger_->WriteF(LogSeverity::Normal, "%s = %s", alias.c_str(),
                  aliased_node_id.ToString().c_str());

  return aliased_node_id;
}
