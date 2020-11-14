#pragma once

#include "common/aliases.h"
#include "node_service/node_observer.h"
#include "node_service/node_ref.h"

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

  void Resolve(std::string_view alias, const AliasResolveCallback& callback);

 private:
  void OnFetchCompleted();

  scada::NodeId ResolveNow(const std::string& alias) const;

  NodeRef aliases_;

  bool fetched_ = false;
  std::unordered_map<std::string, std::vector<AliasResolveCallback>>
      pending_aliases_;
};
