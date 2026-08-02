#pragma once

#include "aui/key_codes.h"
#include "base/awaitable.h"
#include "controller/command_registry.h"
#include "controller/contents_model.h"
#include "controller/controller.h"
#include "controller/controller_context.h"
#include "controller/selection_model.h"
#include "controller/time_model.h"
#include "events/event_menu_model.h"
#include "events/expanded_event_model.h"
#include "export/export_model.h"

#include <memory>

namespace scada::aui {
class Table;
}

class EventTableModel;
class LocalEvents;

class EventView : protected ControllerContext,
                  public Controller,
                  public ContentsModel,
                  public TimeModel,
                  public ExportModel {
 public:
  // `audit_only` scopes the journal to the AuditEventType subtree — the Audit
  // log view. It is a constructor flag rather than a window "mode" because the
  // audit log IS a distinct view: its own WindowInfo, its own command, and
  // admin-gated, unlike the journal's Current/historical modes.
  EventView(const ControllerContext& context,
            LocalEvents& local_events,
            bool is_panel,
            bool audit_only = false);
  virtual ~EventView();

  bool CanAcknowledgeSelection() const;
  void AcknowledgeSelection();

  // Controller
  virtual bool IsWorking() const override;
  virtual std::unique_ptr<UiView> Init(
      const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual SelectionModel* GetSelectionModel() override { return &selection_; }
  virtual ContentsModel* GetContentsModel() override { return this; }
  virtual TimeModel* GetTimeModel() override;
  virtual ExportModel* GetExportModel() override { return this; }

  // ContentsModel
  virtual void AddContainedItem(const scada::NodeId& node_id,
                                unsigned flags) override;
  virtual void RemoveContainedItem(const scada::NodeId& node_id) override;
  virtual NodeIdSet GetContainedItems() const override;

  // TimeModel
  virtual scada::RelativeTimeRange GetTimeRange() const override;
  virtual void SetTimeRange(const scada::RelativeTimeRange& time_range) override;

  // ExportModel
  virtual ExportData GetExportData() override;

 private:
  std::u16string MakeTitle() const;

  void ExportToExcel();

  void SelectSeverity();
  Awaitable<void> SelectSeverityAsync();
  void SetSeverityMin(scada::EventSeverity severity);

  NodeIdSet GetSelectedNodeIds() const;

  void OnSelectionChanged();

  bool OnKeyPressed(scada::aui::KeyCode key_code);

  const bool is_panel_;
  const bool audit_only_ = false;

  LocalEvents& local_events_;

  const std::shared_ptr<EventTableModel> model_;

  // The same journal with its flood groups expanded, handed to exports and
  // printouts so a record contains every occurrence rather than a count.
  ExpandedEventModel expanded_model_{*model_};

  SelectionModel selection_{{timed_data_service_}};

  // Owned by the parent widget.
  scada::aui::Table* table_ = nullptr;

  // The journal's record columns for exports/printouts — the view columns
  // minus the display-only pending-dot marker (EventColumnUnacked).
  std::vector<scada::aui::TableColumn> export_columns_;

  CommandRegistry command_registry_;

  // Cross-platform context menu, backed by `command_registry_`. Declared after
  // it so the registry outlives the menu's delegate.
  EventMenuModel event_menu_model_{command_registry_};
};
