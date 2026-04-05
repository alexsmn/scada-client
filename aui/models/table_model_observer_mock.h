#pragma once

#include "aui/models/table_model_observer.h"

#include <gmock/gmock.h>

namespace aui {

class TableModelObserverMock : public TableModelObserver {
 public:
  MOCK_METHOD0(OnModelChanged, void());
  MOCK_METHOD2(OnItemsChanged, void(int first, int count));
  MOCK_METHOD2(OnItemsAdding, void(int first, int count));
  MOCK_METHOD2(OnItemsAdded, void(int first, int count));
  MOCK_METHOD2(OnItemsRemoving, void(int first, int count));
  MOCK_METHOD2(OnItemsRemoved, void(int first, int count));
};

}  // namespace aui