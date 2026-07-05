#pragma once

#include "aui/models/table_model.h"

#include <boost/signals2/connection.hpp>
#include <utility>
#include <vector>

namespace aui {

// Fake TableModel subscriber that records every received notification, so
// tests can assert on observable event sequences instead of mock call-shape
// expectations.
class RecordingTableModelObserver {
 public:
  using ItemRange = std::pair<int /*first*/, int /*count*/>;

  explicit RecordingTableModelObserver(TableModel& model) {
    connections_.push_back(
        model.SubscribeModelChanged([this] { ++model_changed_count; }));
    connections_.push_back(
        model.SubscribeItemsChanged([this](int first, int count) {
          items_changed.emplace_back(first, count);
        }));
    connections_.push_back(
        model.SubscribeItemsAdding([this](int first, int count) {
          items_adding.emplace_back(first, count);
        }));
    connections_.push_back(
        model.SubscribeItemsAdded([this](int first, int count) {
          items_added.emplace_back(first, count);
        }));
    connections_.push_back(
        model.SubscribeItemsRemoving([this](int first, int count) {
          items_removing.emplace_back(first, count);
        }));
    connections_.push_back(
        model.SubscribeItemsRemoved([this](int first, int count) {
          items_removed.emplace_back(first, count);
        }));
  }

  void ClearEvents() {
    model_changed_count = 0;
    items_changed.clear();
    items_adding.clear();
    items_added.clear();
    items_removing.clear();
    items_removed.clear();
  }

  int model_changed_count = 0;
  std::vector<ItemRange> items_changed;
  std::vector<ItemRange> items_adding;
  std::vector<ItemRange> items_added;
  std::vector<ItemRange> items_removing;
  std::vector<ItemRange> items_removed;

 private:
  std::vector<boost::signals2::scoped_connection> connections_;
};

}  // namespace aui
