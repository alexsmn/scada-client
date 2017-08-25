#pragma once

#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "client/time_range.h"
#include "client/services/local_events.h"
#include "core/node_observer.h"
#include "core/event.h"
#include "common/event_observer.h"
#include "core/history_service.h"
#include "core/configuration_types.h"
#include "common/node_ref.h"
#include "common/node_ref_observer.h"
#include "ui/base/models/table_model.h"

#include <list>
#include <set>

namespace events {
class EventManager;
}

class NodeRefService;

enum EventColumnId {
  EventColumnSeverity,
  EventColumnTime,
  EventColumnItem,
  EventColumnValue,
  EventColumnMessage,
  EventColumnSource,
  EventColumnAckUser,
  EventColumnAckTime,
  EventColumnCount
};

struct EventTableModelContext {
  NodeRefService& node_service_;
  events::EventManager& event_manager_;
  LocalEvents& local_events_;
  scada::HistoryService& history_service_;
};

class EventTableModel : public ui::TableModel,
                        private NodeRefObserver,
                        private events::EventObserver,
                        private LocalEvents::Observer,
                        private EventTableModelContext {
 public:
  enum EventType { CURRENT_EVENT, HISTORICAL_EVENT, LOCAL_EVENT };

  typedef std::set<scada::NodeId> ItemIds;

  explicit EventTableModel(EventTableModelContext&& context);
  virtual ~EventTableModel();

  using Mode = unsigned;

  Mode mode() const { return mode_; }
  const ItemIds& filter_items() const { return filter_node_ids_; }
  const TimeRange& time_range() const { return time_range_; }
  unsigned severity_min() const { return severity_min_; }

  EventType event_type_at(int row) const { return rows_[row].type; }
  const scada::Event& event_at(int row) const { return *rows_[row].event; }
  bool IsWorking() const;

  void Init(Mode mode, const TimeRange& range, ItemIds filter_items);
  void SetMode(Mode mode);
  void set_time_range(const TimeRange& range) { time_range_ = range; }
  void SetSeverityMin(unsigned severity);

  bool AddFilteredItem(const scada::NodeId& item);
  bool RemoveFilteredItem(const scada::NodeId& item);

  void Update();
  void LockUpdate();
  void UnlockUpdate();

  void AcknowledgeRow(int row);

  void CancelRequest();

  base::string16 MakeTitle() const;

  // ui::GridModel
  virtual int GetRowCount() override;
  virtual void GetCell(ui::TableCell& cell) override;

 private:
  void AddRow(EventType type, const scada::Event& event);
  void RemoveRows(int first, int count);
  int FindRow(const scada::Event& event) const;

  void AckRows(int first, int count);

  int GetInsertIndex(EventType type, const scada::Event& event);

  bool IsEventShown(const scada::Event& event) const;

  void RefilterNow();
  void Refilter();

  void OnQueryEventsCompleted(scada::Status status, scada::QueryEventsResults events);

  // NodeRefObserver
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override;

  // events::EventObserver
  virtual void OnEventReported(const scada::Event& event) override;
  virtual void OnEventAcknowledged(const scada::Event& event) override;
  virtual void OnAllEventsAcknowledged() override;

  // LocalEvents::Observer
  virtual void OnLocalEvent(const scada::Event& event) override;

  // NodeFetcherObserver

  Mode mode_;

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
        : type(type),
          event(&event) {}

    EventType type;
    const scada::Event* event;
    NodeRef node;
    NodeRef user;
    NodeRef acknowledged_user;
  };

  typedef std::vector<Row> Rows;
  Rows rows_;

  struct RowsComparer;

  bool request_running_ = false;

  bool lock_update_ = false;
  bool pending_update_ = false;

  base::OneShotTimer refilter_delay_timer_;

  base::WeakPtrFactory<EventTableModel> weak_factory_{this};
};
