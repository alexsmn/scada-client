#pragma once

#include "aui/models/table_model.h"
#include "base/executor_timer.h"
#include "node_service/node_observer.h"
#include "node_service/node_ref.h"

#include <boost/signals2/connection.hpp>
#include <set>
#include <span>

class CurrentEventModel;
class Executor;
class HistoricalEventModel;
class LocalEventModel;
class NodeService;
struct TimeRange;

enum EventColumnId {
  EventColumnSeverity,
  EventColumnTime,
  EventColumnItem,
  EventColumnValue,
  EventColumnMessage,
  EventColumnUser,
  EventColumnAckUser,
  EventColumnAckTime,
  EventColumnCount
};

struct EventTableModelContext {
  // The executor is used for delayed update timer.
  const std::shared_ptr<Executor> executor_;
  NodeService& node_service_;
  CurrentEventModel& current_event_model_;
  HistoricalEventModel& historical_event_model_;
  LocalEventModel& local_event_model_;
  // If true, then show only current events. No historical events are shown.
  const bool current_events_ = true;
};

class EventTableModel : public aui::TableModel,
                        private NodeRefObserver,
                        private EventTableModelContext {
 public:
  enum EventType { CURRENT_EVENT, HISTORICAL_EVENT, LOCAL_EVENT };

  using ItemIds = std::set<scada::NodeId>;

  explicit EventTableModel(EventTableModelContext&& context);
  virtual ~EventTableModel();

  void Init(const TimeRange& range, ItemIds filter_items);

  bool current_events() const { return current_events_; }

  EventType event_type_at(int row) const { return rows_[row].type; }
  const scada::Event& event_at(int row) const { return *rows_[row].event; }
  bool IsWorking() const;

  const TimeRange& time_range() const;
  void SetTimeRange(const TimeRange& range);

  unsigned severity_min() const { return severity_min_; }
  void SetSeverityMin(unsigned severity);

  const ItemIds& filter_items() const { return filter_node_ids_; }
  bool AddFilteredItem(const scada::NodeId& item);
  bool RemoveFilteredItem(const scada::NodeId& item);

  void Update();
  void LockUpdate();
  void UnlockUpdate();
  bool IsUpdateLocked() const { return lock_update_; }

  void AcknowledgeRow(int row);

  void CancelRequest();

  std::u16string MakeTitle() const;

  // aui::TableModel
  virtual int GetRowCount() override;
  virtual void GetCell(aui::TableCell& cell) override;
  virtual int CompareCells(int row1, int row2, int column_id) override;

 private:
  void AddRows(EventType type, std::span<const scada::Event* const> events);
  void RemoveRows(int first, int count);
  int FindRow(const scada::Event& event) const;

  // TODO: Remove this method. Keep only `OnCurrentEvents()`.
  void AckRows(int first, int count);

  bool IsEventShown(const scada::Event& event) const;

  void RefilterNow();
  void Refilter();

  void UpdateAffectedRows(const scada::NodeId& node_id);

  void OnCurrentEvents(std::span<const scada::Event* const> events);
  void OnLocalEvent(const scada::Event& event);

  // NodeRefObserver
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override;
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;

  // Filter.
  unsigned severity_min_ = 0;
  ItemIds filter_node_ids_;

  // Rows displayed in grid.
  struct Row {
    Row(EventType type, const scada::Event& event)
        : type(type), event(&event) {}

    void Update(NodeService& node_service);
    bool IsAffected(const scada::NodeId& node_id) const;

    EventType type;
    const scada::Event* event;
    NodeRef node;
    NodeRef user;
    NodeRef acknowledged_user;
  };

  using Rows = std::vector<Row>;
  Rows rows_;

  bool lock_update_ = false;
  bool pending_update_ = false;

  ExecutorTimer refilter_delay_timer_{executor_};

  std::vector<boost::signals2::scoped_connection> connections_;
};
