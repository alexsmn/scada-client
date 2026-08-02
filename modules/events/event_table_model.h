#pragma once

#include "base/any_executor.h"
#include "base/lifetime.h"

#include "aui/models/table_model.h"
#include "base/any_executor_timer.h"
#include "node_service/node_ref.h"

#include <boost/signals2/connection.hpp>
#include <set>
#include <span>
#include <vector>

class CurrentEventModel;
class HistoricalEventModel;
class LocalEventModel;
class NodeService;
namespace scada { struct RelativeTimeRange; }

enum EventColumnId {
  EventColumnSeverity,
  EventColumnTime,
  EventColumnItem,
  EventColumnValue,
  EventColumnMessage,
  EventColumnUser,
  EventColumnAckUser,
  EventColumnAckTime,
  // Reshell-only leading unacknowledged marker: a severity-coloured dot on
  // every row whose alarm is still pending, so the actionable rows read at a
  // glance (colour + shape, in addition to the "— pending —" cell). Appended
  // so existing column ids in saved state stay stable; shown first in the
  // view only under the opt-in token theme.
  EventColumnUnacked,
  EventColumnCount
};

struct EventTableModelContext {
  // The executor is used for delayed update timer.
  const AnyExecutor executor_;
  NodeService& node_service_;
  CurrentEventModel& current_event_model_;
  HistoricalEventModel& historical_event_model_;
  LocalEventModel& local_event_model_;
  // If true, then show only current events. No historical events are shown.
  const bool current_events_ = true;
};

class EventTableModel : public scada::aui::TableModel,
                        private EventTableModelContext {
 public:
  enum EventType { CURRENT_EVENT, HISTORICAL_EVENT, LOCAL_EVENT };

  using ItemIds = std::set<scada::NodeId>;

  explicit EventTableModel(EventTableModelContext&& context);
  virtual ~EventTableModel();

  void Init(const scada::RelativeTimeRange& range, ItemIds filter_items);

  bool current_events() const { return current_events_; }

  EventType event_type_at(int row) const { return rows_[row].type; }
  const scada::Event& event_at(int row) const SCADA_LIFETIME_BOUND {
    return *rows_[row].event;
  }
  bool IsWorking() const;

  const scada::RelativeTimeRange& time_range() const;
  void SetTimeRange(const scada::RelativeTimeRange& range);

  unsigned severity_min() const { return severity_min_; }
  void SetSeverityMin(unsigned severity);

  // When set, the journal hides already-acknowledged events, leaving only the
  // actionable (unacknowledged) ones — the active-alarm surface. No effect on
  // the current-events surface, which is unacknowledged by construction.
  bool unacknowledged_only() const { return unacknowledged_only_; }
  void SetUnacknowledgedOnly(bool value);

  // When set, the journal shows ONLY the audit trail — the AuditEventType
  // subtree (see events/audit_events.h). This is what makes an Audit log view
  // possible: the same journal, scoped to the events that record who did what.
  //
  // It is a filter over what was fetched, not a narrowing of the fetch, so it
  // cannot hide an event the journal would otherwise have shown.
  bool audit_only() const { return audit_only_; }
  void SetAuditOnly(bool value);

  const ItemIds& filter_items() const SCADA_LIFETIME_BOUND {
    return filter_node_ids_;
  }
  bool AddFilteredItem(const scada::NodeId& item);
  bool RemoveFilteredItem(const scada::NodeId& item);

  void Update();
  void LockUpdate();
  void UnlockUpdate();
  bool IsUpdateLocked() const { return lock_update_; }

  // Acknowledges the alarms at `rows` — every occurrence collapsed into them
  // when the rows are flood groups, so acknowledging a collapsed row never
  // leaves hidden unacknowledged alarms behind.
  //
  // Takes the whole selection at once because acknowledging notifies, and a
  // notification can remove rows or regroup them wholesale: indices resolved
  // before the first acknowledgement are stale by the second, which would
  // acknowledge alarms the operator never selected.
  void AcknowledgeRows(std::span<const int> rows);
  void AcknowledgeRow(int row) { AcknowledgeRows({&row, 1}); }

  // Whether the journal's historical rows are currently collapsed into flood
  // groups. Decided by the model itself on each rebuild (see
  // kAlarmFloodThreshold), not set by the caller: the journal groups exactly
  // while the operator is buried. The live (current/local) rows never group —
  // see the note in RefilterNow().
  bool grouped() const { return grouped_; }

  // The displayed alarm backlog, for the journal's footer summary: how many
  // unacknowledged occurrences are on display (counting every member of a
  // collapsed row) and the highest raw severity among them (0 when none).
  struct AlarmSummary {
    int unacknowledged = 0;
    unsigned max_severity = 0;

    bool operator==(const AlarmSummary&) const = default;
  };
  AlarmSummary GetAlarmSummary() const;

  // Unacknowledged counts for the journal's Areas sidebar: the overall count
  // plus one count per entry of `areas` (an event belongs to an area when its
  // source or any containing node is that area). Ignores the active area
  // filter — the sidebar's counts stay meaningful for every area while one
  // of them is filtering the rows — but respects the severity and
  // unacknowledged-only filters, so each count matches what selecting that
  // area would show.
  struct AreaCounts {
    int total = 0;
    std::vector<int> per_area;

    bool operator==(const AreaCounts&) const = default;
  };
  AreaCounts CountUnacknowledgedByArea(
      std::span<const scada::NodeId> areas) const;

  // Occurrences collapsed into `row`, 1 when it stands for a single event.
  int group_count_at(int row) const;

  // Every occurrence the journal is holding, counting each member of a
  // collapsed row separately — what the rows would number if nothing were
  // grouped.
  int GetOccurrenceCount() const;

  // Renders `cell` for the `cell.row`-th *occurrence* in row order, expanding
  // collapsed rows back into one entry each. This is what a record of the
  // journal should contain — an export or a printout is evidence of what
  // happened, not of how the display chose to fold it — so those paths read
  // here rather than through GetCell().
  void GetOccurrenceCell(scada::aui::TableCell& cell);

  void CancelRequest();

  std::u16string MakeTitle() const;

  // aui::TableModel
  virtual int GetRowCount() override;
  virtual void GetCell(scada::aui::TableCell& cell) override;
  virtual int CompareCells(int row1, int row2, int column_id) override;

 private:
  void AddRows(EventType type, std::span<const scada::Event* const> events);
  void RemoveRows(int first, int count);
  int FindRow(const scada::Event& event) const;

  // Row holding `event` — as its representative or as one of its collapsed
  // repeats — or -1 when no row holds it.
  int FindOccurrenceRow(const scada::Event& event) const;

  // Row of `type` whose alarm `event` is another occurrence of, or -1. Only
  // consulted while grouped.
  int FindAlarmGroupRow(EventType type, const scada::Event& event) const;

  // Drops one occurrence from a collapsed row, promoting the newest remaining
  // occurrence when the dropped one was on display. Returns false when `event`
  // was the row's only occurrence, leaving the row untouched for the caller to
  // remove or convert.
  bool RemoveOccurrence(int index, const scada::Event& event);

  // Moves `event` into the journal's history and folds it into the historical
  // rows. Used when a live group survives losing one occurrence to
  // acknowledgement: the storage pointer dies with the notification, so the
  // occurrence must be copied to keep it in the journal.
  void MoveOccurrenceToHistory(const scada::Event& event);

  // Unacknowledged occurrences currently held, counting every member of a
  // collapsed row — the signal the flood threshold is judged on.
  int CountUnacknowledged() const;

  // Rebuilds when the backlog has crossed the flood threshold in either
  // direction, so rows collapse as a flood starts and expand once it is worked
  // off.
  void RegroupIfFloodChanged();

  // TODO: Remove this method. Keep only `OnCurrentEvents()`.
  void AckRows(int first, int count);

  bool IsEventShown(const scada::Event& event) const;

  // IsEventShown split so the sidebar counting can apply every filter except
  // the area one.
  bool PassesFilters(const scada::Event& event, bool include_area_filter) const;
  // The event's source, or any node containing it, is one of `areas`.
  bool IsUnderAnyOf(const scada::Event& event, const ItemIds& areas) const;

  void RefilterNow();
  void Refilter();

  void UpdateAffectedRows(const scada::NodeId& node_id);

  void OnCurrentEvents(std::span<const scada::Event* const> events);
  void OnLocalEvent(const scada::Event& event);

  void OnNodeSemanticChanged(const scada::NodeId& node_id);
  void OnModelChanged(const scada::ModelChangeEvent& event);

  // Filter.
  unsigned severity_min_ = 0;
  bool unacknowledged_only_ = false;
  bool audit_only_ = false;
  ItemIds filter_node_ids_;

  // Rows displayed in grid.
  struct Row {
    Row(EventType type, const scada::Event& event)
        : type(type), event(&event) {}

    void Update(NodeService& node_service);
    bool IsAffected(const scada::NodeId& node_id) const;

    EventType type;
    const scada::Event* event;
    // Further occurrences of the same alarm collapsed into this row (flood
    // grouping); empty when the row stands for a single event.
    std::vector<const scada::Event*> repeats;
    NodeRef node;
    NodeRef user;
    NodeRef acknowledged_user;
  };

  using Rows = std::vector<Row>;
  Rows rows_;

  // Appends `events` to `rows`: one row each, or — when `grouped` — one row per
  // collapsed alarm group.
  // Renders `cell` from `event` as it appears in `row`. `group_count` is the
  // occurrence count to show (1 when the event is being rendered on its own).
  void GetEventCell(const Row& row,
                    const scada::Event& event,
                    int group_count,
                    scada::aui::TableCell& cell) const;

  // The row and the event behind the `index`-th occurrence, in row order.
  std::pair<const Row*, const scada::Event*> OccurrenceAt(int index) const;

  void AppendRows(Rows& rows,
                  EventType type,
                  std::span<const scada::Event* const> events,
                  bool grouped) const;

  // Set on rebuild when the unacknowledged backlog constitutes a flood; applies
  // to the historical rows only.
  bool grouped_ = false;

  bool lock_update_ = false;
  bool pending_update_ = false;

  AnyExecutorTimer refilter_delay_timer_{executor_};

  std::vector<boost::signals2::scoped_connection> connections_;
};
