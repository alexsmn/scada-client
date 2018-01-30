#pragma once

#include <cassert>

#include "core/configuration_types.h"
#include "timed_data/timed_data_delegate.h"
#include "timed_data/timed_data_spec.h"
#include "controls/types.h"
#include "selection_model.h"
#include "common/node_ref.h"
#include "common/node_ref_observer.h"

class NodeService;

class SelectionModel : private rt::TimedDataDelegate,
                       private NodeRefObserver {
 public:
  SelectionModel(NodeService& node_service, TimedDataService& timed_data_service);
  SelectionModel(const SelectionModel&) = delete;
  ~SelectionModel();

  bool empty() const { return type_ == EMPTY; }
  bool multiple() const { return type_ == MULTI; }

  std::function<NodeIdSet()> multiple_handler_;

  void Clear();
  void SelectNode(const NodeRef& node);
  void SelectNodeId(const scada::NodeId& node_id);
  void SelectTimedData(const rt::TimedDataSpec& spec);
  void SelectMultiple();

  base::string16 GetTitle() const;
  NodeIdSet GetMultipleNodeIds() const;
  const NodeRef& node() const { return node_; }
  const rt::TimedDataSpec& GetTimedData() const { return timed_data_; }

  typedef std::function<void()> ChangeHandler;
  void set_change_handler(ChangeHandler handler) { change_handler_ = std::move(handler); }

  SelectionModel& operator=(const SelectionModel& source) = delete;

 protected:
  void Changed();

 private:
  void Reset();

  // rt::TimedDataDelegate
  virtual void OnPropertyChanged(rt::TimedDataSpec& spec,
                                 const rt::PropertySet& properties) override;

  // NodeRefObserver
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override;
  virtual void OnModelChange(const ModelChangeEvent& event) override;

  enum Type { EMPTY, NODE, SPEC, MULTI };
  Type type_ = EMPTY;

  NodeService& node_service_;
  TimedDataService& timed_data_service_;
  rt::TimedDataSpec	timed_data_;
  NodeRef node_;

  ChangeHandler change_handler_;

  std::shared_ptr<bool> pending_request_;
};
