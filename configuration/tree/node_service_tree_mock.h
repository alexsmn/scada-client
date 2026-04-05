#pragma once

#include "configuration/tree/node_service_tree.h"

#include <gmock/gmock.h>

class MockNodeServiceTree : public NodeServiceTree {
 public:
  MOCK_METHOD(NodeRef, GetRoot, (), (const));

  MOCK_METHOD(bool, HasChildren, (const NodeRef& node), (const));

  MOCK_METHOD(std::vector<ChildRef>,
              GetChildren,
              (const NodeRef& node),
              (const));

  MOCK_METHOD(void, SetObserver, (Observer * observer));
};
