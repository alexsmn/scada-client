#pragma once

#include "bulk_create/bulk_create_pattern.h"

#include <QWidget>

#include <set>
#include <string>

class QLabel;
class QLineEdit;
class QSpinBox;
class QTableWidget;

// The reshell bulk-create wizard's Naming/Addressing step — the center of
// client/docs/ui-mockups/screens/bulk-create.html. It edits a `{n}`-token
// naming + addressing pattern (name template, NodeId template, start index,
// count, index step, IOA start/step) and shows a **live preview** grid
// (# / Name / NodeId / IOA / Status) that re-expands on every edit, flagging
// rows whose NodeId already exists, plus a "N new / M conflict" summary.
//
// The actual node creation is not done here — it rides the existing
// MultiCreateModel::Run (Session.addNodes via TaskManager); this panel is the
// pattern-and-preview surface.
//
// Opt-in: construct only under the reshell UX theme (see
// MakeBulkCreatePreviewPanel).
class BulkCreatePreviewPanel : public QWidget {
  Q_OBJECT

 public:
  explicit BulkCreatePreviewPanel(QWidget* parent = nullptr);
  ~BulkCreatePreviewPanel() override;

  // The set of NodeIds already present in the target, used to flag conflicts.
  void SetExistingNodeIds(std::set<std::u16string> existing);

  // Seeds the pattern fields and refreshes the preview.
  void SetParams(const BulkCreateParams& params);

  // Re-expands the current pattern into the preview grid + summary.
  void Refresh();

  // Test/inspection accessors.
  const std::vector<BulkCreatePreviewRow>& rows() const { return rows_; }

 private:
  QWidget* BuildForm();
  QWidget* BuildPreview();
  BulkCreateParams CurrentParams() const;

  QLineEdit* name_template_ = nullptr;
  QLineEdit* node_id_template_ = nullptr;
  QSpinBox* start_index_ = nullptr;
  QSpinBox* count_ = nullptr;
  QSpinBox* index_step_ = nullptr;
  QSpinBox* ioa_start_ = nullptr;
  QSpinBox* ioa_step_ = nullptr;

  QTableWidget* preview_ = nullptr;
  QLabel* summary_ = nullptr;

  std::set<std::u16string> existing_node_ids_;
  std::vector<BulkCreatePreviewRow> rows_;
};

// Builds a BulkCreatePreviewPanel under the reshell UX theme
// (scada::aui::GetSeverityTheme() != SeverityTheme::kLegacy); returns nullptr in
// the legacy look. Ownership transfers to the caller.
BulkCreatePreviewPanel* MakeBulkCreatePreviewPanel();
