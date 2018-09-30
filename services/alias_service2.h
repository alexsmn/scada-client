#pragma once

#include "common/aliases.h"
#include "common/node_observer.h"
#include "common/node_ref.h"

#include <map>
#include <unordered_map>

class Logger;
class NodeRef;
class NodeService;

struct AliasService2Context {
  const std::shared_ptr<Logger> logger_;
  NodeService& node_service_;
};

class AliasService2 final : private AliasService2Context,
                            private NodeRefObserver {
 public:
  explicit AliasService2(AliasService2Context&& context);
  ~AliasService2();

  void Resolve(base::StringPiece alias, const AliasResolveCallback& callback);

 private:
  void OnFetchCompleted();

  scada::NodeId ResolveNow(const std::string& alias) const;

  NodeRef aliases_;

  bool fetched_ = false;
  std::unordered_map<std::string, std::vector<AliasResolveCallback>>
      pending_aliases_;
};
