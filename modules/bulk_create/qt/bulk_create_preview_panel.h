#pragma once

#include "base/lifetime.h"
#include "bulk_create/bulk_create_pattern.h"

#include <QWidget>

#include <set>
#include <string>

class QLabel;
class QLineEdit;
class QSpinBox;
class QTableWidget;

// The reshell bulk-create wizard's Naming/Addressing step — the center of
// docs/product/ui-mockups/screens/bulk-create.html. It edits a `{n}`-token
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

  // Which of the two things this run creates. One panel serves both because
  // the naming half is identical; the subject decides only whether the rows
  // are addressed on a link, and so whether the IOA controls and the IOA
  // column are shown at all. Defaults to kDataItem, matching
  // `BulkCreateParams`, so a caller that says nothing gets the branch with no
  // address rather than one silently addressing from stale spin boxes.
  void SetSubject(BulkCreateSubject subject);
  BulkCreateSubject subject() const { return subject_; }

  // The set of NodeIds already present in the target, used to flag conflicts.
  void SetExistingNodeIds(std::set<std::u16string> existing);

  // Seeds the pattern fields and refreshes the preview.
  void SetParams(const BulkCreateParams& params);

  // Re-expands the current pattern into the preview grid + summary.
  void Refresh();

  // Test/inspection accessors.
  const std::vector<BulkCreatePreviewRow>& rows() const SCADA_LIFETIME_BOUND {
    return rows_;
  }

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
  // The form rows the IOA spin boxes sit on, kept so the subject can hide the
  // label with the field: hiding a QFormLayout field alone leaves its label
  // behind, captioning nothing.
  QWidget* ioa_start_label_ = nullptr;
  QWidget* ioa_step_label_ = nullptr;

  QTableWidget* preview_ = nullptr;
  QLabel* summary_ = nullptr;

  BulkCreateSubject subject_ = BulkCreateSubject::kDataItem;
  std::set<std::u16string> existing_node_ids_;
  std::vector<BulkCreatePreviewRow> rows_;
};

// Builds a BulkCreatePreviewPanel. Ownership transfers to the caller.
BulkCreatePreviewPanel* MakeBulkCreatePreviewPanel();
