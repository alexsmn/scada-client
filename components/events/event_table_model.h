#pragma once

#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "common/event_observer.h"
#include "node_service/node_observer.h"
#include "node_service/node_ref.h"
#include "core/configuration_types.h"
#include "core/event.h"
#include "core/history_service.h"
#include "services/local_events.h"
#include "time_range.h"
#include "ui/base/models/table_model.h"

#include <list>
#include <set>

class EventManager;
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
  NodeService& node_service_;
  EventManager& event_manager_;
  LocalEvents& local_events_;
  scada::HistoryService& history_service_;
  const bool current_events_;
};

class EventTableModel : public ui::TableModel,
                        private NodeRefObserver,
                        private EventObserver,
                        private LocalEvents::Observer,
                        private EventTableModelContext {
 public:
  enum EventType { CURRENT_EVENT, HISTORICAL_EVENT, LOCAL_EVENT };

  typedef std::set<scada::NodeId> ItemIds;

  explicit EventTableModel(EventTableModelContext&& context);
  virtual ~EventTableModel();

  bool current_events() const { return current_events_; }
  const ItemIds& filter_items() const { return filter_node_ids_; }
  const TimeRange& time_range() const { return time_range_; }
  unsigned severity_min() const { return severity_min_; }

  EventType event_type_at(int row) const { return rows_[row].type; }
  const scada::Event& event_at(int row) const { return *rows_[row].event; }
  bool IsWorking() const;

  void Init(const TimeRange& range, ItemIds filter_items);
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

  base::string16 MakeTitle() const;

  // ui::TableModel
  virtual int GetRowCount() override;
  virtual void GetCell(ui::TableCell& cell) override;
  virtual int CompareCells(int row1, int row2, int column_id) override;

 private:
  void AddRow(EventType type, const scada::Event& event);
  void RemoveRows(int first, int count);
  int FindRow(const scada::Event& event) const;

  void AckRows(int first, int count);

  bool IsEventShown(const scada::Event& event) const;

  void RefilterNow();
  void Refilter();

  void UpdateAffectedRows(const scada::NodeId& node_id);

  void OnHistoryReadEventsCompleted(scada::Status&& status,
                                    std::vector<scada::Event>&& events);

  // NodeRefObserver
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override;
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;

  // EventObserver
  virtual void OnEventReported(const scada::Event& event) override;
  virtual void OnEventAcknowledged(const scada::Event& event) override;
  virtual void OnAllEventsAcknowledged() override;

  // LocalEvents::Observer
  virtual void OnLocalEvent(const scada::Event& event) override;

  // Filter.
  TimeRange time_range_;
  unsigned severity_min_ = 0;
  ItemIds filter_node_ids_;

  // Contains only historical events. |rows_| holds pointers on items from
  // it, so it shall not be vector.
  typedef std::list<scada::Event> EventContainer;
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

  typedef std::vector<Row> Rows;
  Rows rows_;

  bool request_running_ = false;

  bool lock_update_ = false;
  bool pending_update_ = false;

  base::OneShotTimer refilter_delay_timer_;

  base::WeakPtrFactory<EventTableModel> weak_factory_{this};
};
