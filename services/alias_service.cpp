#include "alias_service.h"

#include "base/logger.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "model/data_items_node_ids.h"
#include "model/scada_node_ids.h"

AliasService::AliasService(AliasServiceContext&& context)
    : AliasServiceContext{std::move(context)} {
  node_service_.Subscribe(*this);

  UpdateRecursive(node_service_.GetNode(data_items::id::DataItems));
}

AliasService::~AliasService() {
  node_service_.Unsubscribe(*this);
}

void AliasService::Resolve(base::StringPiece alias,
                           const AliasResolveCallback& callback) {
  auto alias_string = alias.as_string();

  logger_->WriteF(LogSeverity::Normal, "Resolve: %s", alias_string.c_str());

  auto i = resolved_aliases_.find(alias_string);
  if (i != resolved_aliases_.end()) {
    auto& node_id = i->second;
    logger_->WriteF(LogSeverity::Normal, "Resolved immediately: %s = %s",
                    alias_string.c_str(), node_id.ToString().c_str());
    return callback(scada::StatusCode::Good, node_id);
  }

  if (fetch_count_ == 0) {
    logger_->WriteF(LogSeverity::Normal, "Wrong immediate alias: %s",
                    alias_string.c_str());
    return callback(scada::StatusCode::Bad, {});
  }

  logger_->WriteF(LogSeverity::Normal, "Pending resolution: %s",
                  alias_string.c_str());
  pending_aliases_[alias_string].emplace_back(callback);
}

void AliasService::OnModelChanged(const scada::ModelChangeEvent& event) {
  if (event.verb & scada::ModelChangeEvent::NodeDeleted) {
    Set(event.node_id, {});

  } else if (event.verb & scada::ModelChangeEvent::NodeAdded) {
    assert(!event.type_definition_id.is_null());
    auto type_definition = node_service_.GetNode(event.type_definition_id);
    if (IsSubtypeOf(type_definition, data_items::id::DataItemType))
      Update(node_service_.GetNode(event.node_id));
  }
}

void AliasService::OnNodeSemanticChanged(const scada::NodeId& node_id) {
  Update(node_service_.GetNode(node_id));
}

void AliasService::UpdateRecursive(const NodeRef& node) {
  ++fetch_count_;
  node.Fetch(NodeFetchStatus::NodeAndChildren(), [this](const NodeRef& node) {
    assert(node.children_fetched());

    for (const auto& child : node.targets(scada::id::Organizes)) {
      Update(child);
      UpdateRecursive(child);
    }

    --fetch_count_;
    if (fetch_count_ == 0)
      OnRecursiveUpdateCompleted();
  });
}

void AliasService::Update(const NodeRef& node) {
  node.Fetch(NodeFetchStatus::NodeOnly(), [this](const NodeRef& node) {
    assert(node.fetched());

    if (!IsInstanceOf(node, data_items::id::DataItemType))
      return;

    auto alias =
        node[data_items::id::DataItemType_Alias].value().get_or(std::string{});
    Set(node.node_id(), std::move(alias));
  });
}

void AliasService::Set(const scada::NodeId& node_id, std::string alias) {
  auto i = resolved_nodes_.find(node_id);

  if (alias.empty()) {
    if (i == resolved_nodes_.end())
      return;

    logger_->WriteF(LogSeverity::Normal, "Removed: %s", alias.c_str());

    resolved_aliases_.erase(i->second);
    resolved_nodes_.erase(i);

  } else if (i == resolved_nodes_.end()) {
    logger_->WriteF(LogSeverity::Normal, "Added: %s=%s", alias.c_str(),
                    node_id.ToString().c_str());

    resolved_aliases_.emplace(alias, node_id);
    resolved_nodes_.emplace(node_id, alias);

  } else if (i->second != alias) {
    logger_->WriteF(LogSeverity::Normal, "Updated: %s=%s", alias.c_str(),
                    node_id.ToString().c_str());

    resolved_aliases_.erase(i->second);
    i->second = alias;
    resolved_aliases_.emplace(alias, node_id);

  } else {
    return;
  }

  auto j = pending_aliases_.find(alias);
  if (j != pending_aliases_.end()) {
    auto callbacks = std::move(j->second);
    pending_aliases_.erase(j);

    if (!callbacks.empty()) {
      logger_->WriteF(LogSeverity::Normal, "Resolved pending: %s=%s",
                      alias.c_str(), node_id.ToString().c_str());

      auto copied_node_id = node_id;
      for (auto& c : callbacks)
        c(scada::StatusCode::Good, copied_node_id);
    }
  }
}

void AliasService::OnRecursiveUpdateCompleted() {
  logger_->Write(LogSeverity::Normal, "Recursive update completed");

  auto pending_aliases = std::move(pending_aliases_);
  for (auto& [alias, callbacks] : pending_aliases) {
    logger_->WriteF(LogSeverity::Normal, "Wrong pending alias: %s",
                    alias.c_str());

    for (auto& c : callbacks)
      c(scada::StatusCode::Bad, {});
  }
}
