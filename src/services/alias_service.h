#pragma once

#include "common/aliases.h"
#include "common/node_observer.h"

#include <map>
#include <unordered_map>

class Logger;
class NodeRef;
class NodeService;

struct AliasServiceContext {
  const std::shared_ptr<Logger> logger_;
  NodeService& node_service_;
};

class AliasService final : private AliasServiceContext,
                           private NodeRefObserver {
 public:
  explicit AliasService(AliasServiceContext&& context);
  ~AliasService();

  void Resolve(base::StringPiece alias, const AliasResolveCallback& callback);

 private:
  void OnRecursiveUpdateCompleted();
  void UpdateRecursive(const NodeRef& node);
  void Update(const NodeRef& node);
  void Set(const scada::NodeId& node_id, std::string alias);

  // NodeRefObserver
  virtual void OnModelChange(const ModelChangeEvent& event) final;
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) final;

  std::unordered_map<std::string, scada::NodeId> resolved_aliases_;
  std::map<scada::NodeId, std::string> resolved_nodes_;

  unsigned fetch_count_ = 0;
  std::unordered_map<std::string, std::vector<AliasResolveCallback>>
      pending_aliases_;
};