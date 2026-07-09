#pragma once

#include "base/lifetime.h"
#include "common/node_state.h"
#include "controller/node_id_set.h"
#include "node_service/node_ref.h"
#include "timed_data/timed_data_spec.h"

#include <boost/signals2/connection.hpp>

class TimedDataService;

struct SelectionModelContext {
  TimedDataService& timed_data_service_;
};

class SelectionModel final : private SelectionModelContext {
 public:
  explicit SelectionModel(SelectionModelContext&& context);
  SelectionModel(const SelectionModel&) = delete;
  ~SelectionModel();

  bool empty() const { return type_ == EMPTY; }
  bool multiple() const { return type_ == MULTI; }

  void Clear();
  void SelectNode(const NodeRef& node);
  void SelectTimedData(const TimedDataSpec& spec);
  void SelectMultiple();

  std::u16string GetTitle() const;
  NodeIdSet GetMultipleNodeIds() const;
  const NodeRef& node() const SCADA_LIFETIME_BOUND { return node_; }
  const TimedDataSpec& timed_data() const SCADA_LIFETIME_BOUND {
    return timed_data_;
  }

  using ChangeHandler = std::function<void()>;
  ChangeHandler change_handler;

  std::function<NodeIdSet()> multiple_handler;

  SelectionModel& operator=(const SelectionModel& source) = delete;

 protected:
  void Changed();

 private:
  void Reset();

  void SubscribeNode();

  void OnNodeSemanticChanged(const scada::NodeId& node_id);
  void OnModelChanged(const scada::ModelChangeEvent& event);

  enum Type { EMPTY, NODE, SPEC, MULTI };
  Type type_ = EMPTY;

  TimedDataSpec timed_data_;
  NodeRef node_;

  boost::signals2::scoped_connection node_semantic_changed_connection_;
  boost::signals2::scoped_connection model_changed_connection_;
};
