#pragma once

#include "aui/models/table_model.h"
#include "base/executor_timer.h"
#include "base/memory/weak_ptr.h"
#include "node_service/node_observer.h"
#include "node_service/node_ref.h"
#include "scada/event.h"
#include "scada/history_service.h"
#include "services/local_events.h"
#include "time_range.h"

#include <list>
#include <set>

class CurrentEventModel;
class Executor;
class NodeService;

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
  // The executor is the current executor used to sync `history_service_`
  // responses.
  const std::shared_ptr<Executor> executor_;
  NodeService& node_service_;
  CurrentEventModel& current_event_model_;
  LocalEvents& local_events_;
  scada::HistoryService& history_service_;
  const bool current_events_ = true;
};

class EventTableModel : public aui::TableModel,
                        private NodeRefObserver,
                        private LocalEvents::Observer,
                        private EventTableModelContext {
 public:
  enum EventType { CURRENT_EVENT, HISTORICAL_EVENT, LOCAL_EVENT };

  using ItemIds = std::set<scada::NodeId>;

  explicit EventTableModel(EventTableModelContext&& context);
  virtual ~EventTableModel();

  void Init(const TimeRange& range, ItemIds filter_items);

  bool current_events() const { return current_events_; }
  const ItemIds& filter_items() const { return filter_node_ids_; }
  const TimeRange& time_range() const { return time_range_; }
  unsigned severity_min() const { return severity_min_; }

  EventType event_type_at(int row) const { return rows_[row].type; }
  const scada::Event& event_at(int row) const { return *rows_[row].event; }
  bool IsWorking() const;

  void SetTimeRange(const TimeRange& range);
  void SetSeverityMin(unsigned severity);

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
  void AddRows(EventType type, base::span<const scada::Event* const> events);
  void RemoveRows(int first, int count);
  int FindRow(const scada::Event& event) const;

  void AckRows(int first, int count);

  bool IsEventShown(const scada::Event& event) const;

  void RefilterNow();
  void Refilter();

  void UpdateAffectedRows(const scada::NodeId& node_id);

  void OnHistoryReadEventsCompleted(scada::Status&& status,
                                    std::vector<scada::Event>&& events);

  void OnCurrentEvents(base::span<const scada::Event* const> events);

  // NodeRefObserver
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override;
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;

  // LocalEvents::Observer
  virtual void OnLocalEvent(const scada::Event& event) override;

  // Filter.
  TimeRange time_range_;
  unsigned severity_min_ = 0;
  ItemIds filter_node_ids_;

  // Contains only historical events. |rows_| holds pointers on items from
  // it, so it shall not be vector.
  using EventContainer = std::list<scada::Event>;
  EventContainer historical_events_;

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

  bool request_running_ = false;

  bool lock_update_ = false;
  bool pending_update_ = false;

  ExecutorTimer refilter_delay_timer_{executor_};

  boost::signals2::scoped_connection on_events_connection_;
  boost::signals2::scoped_connection all_acked_connection_;

  base::WeakPtrFactory<EventTableModel> weak_factory_{this};
};
