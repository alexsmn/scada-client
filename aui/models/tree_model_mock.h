#pragma once

#include "aui/models/tree_model.h"

#include <gmock/gmock.h>

namespace aui {

class MockTreeModel : public TreeModel {
 public:
  MOCK_METHOD(void*, GetRoot, ());
  MOCK_METHOD(void*, GetParent, (void* node));
  MOCK_METHOD(int, GetChildCount, (void* parent));
  MOCK_METHOD(void*, GetChild, (void* parent, int index));
  MOCK_METHOD(base::string16, GetText, (void* node, int column_id));
};

}  // namespace aui
