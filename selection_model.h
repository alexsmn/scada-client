#pragma once

#include "core/configuration_types.h"
#include "node_id_set.h"
#include "node_service/node_observer.h"
#include "node_service/node_ref.h"
#include "timed_data/timed_data_spec.h"

class TimedDataService;

struct SelectionModelContext {
  TimedDataService& timed_data_service_;
};

class SelectionModel final : private SelectionModelContext,
                             private NodeRefObserver {
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

  std::wstring GetTitle() const;
  NodeIdSet GetMultipleNodeIds() const;
  const NodeRef& node() const { return node_; }
  const TimedDataSpec& timed_data() const { return timed_data_; }

  using ChangeHandler = std::function<void()>;
  ChangeHandler change_handler;

  std::function<NodeIdSet()> multiple_handler;

  SelectionModel& operator=(const SelectionModel& source) = delete;

 protected:
  void Changed();

 private:
  void Reset();

  // NodeRefObserver
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override;
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;

  enum Type { EMPTY, NODE, SPEC, MULTI };
  Type type_ = EMPTY;

  TimedDataSpec timed_data_;
  NodeRef node_;
};
