#pragma once

#include "bulk_create/bulk_create_pattern.h"
#include "scada/node_id.h"

#include <vector>

class DialogService;
class QWizard;
class NodeService;
class TaskManager;

// What a bulk-create run needs from its host.
struct BulkCreateWizardContext {
  NodeService& node_service_;
  TaskManager& task_manager_;
  // Where the created nodes go.
  scada::NodeId parent_id_;

  // The nodes created rules forward, for the transmission subject. Gathered by
  // the caller from the operator's selection, because a rule forwards an
  // *existing* node rather than a name the wizard can invent — which is why
  // this cannot be a template like everything else here.
  //
  // Empty means this invocation cannot create rules, and the wizard then
  // offers the data-item subject alone rather than a subject that would
  // produce nothing. One wizard, two subjects, and the caller decides which
  // are available by what it hands over.
  std::vector<scada::NodeId> source_node_ids_;
};

// Opens the bulk-create wizard, modally, and creates on Finish.
//
// The steps are `docs/product/ui-mockups/screens/bulk-create.html`'s, less the
// Defaults step (units, scale, limits), which is not built — see the backlog.
// Everything it creates goes through `PlanBulkCreate`, so the preview grid and
// the nodes that appear are expanded from one pattern by one function.
void ShowBulkCreateWizard(DialogService& dialog_service,
                          BulkCreateWizardContext&& context);

// Builds the wizard without showing it. Ownership transfers to the caller.
//
// This is what makes the wizard testable at all: `ShowBulkCreateWizard` opens
// a self-owned modal, which a test cannot drive. Every control carries an
// object name (`bulkCreateSubject`, `bulkCreateItemType`, `bulkCreateDevice`,
// `bulkCreateSourcePath`, `bulkCreateReview`), so a test reaches them by
// `findChild` rather than through a widened class declaration.
QWizard* MakeBulkCreateWizard(BulkCreateWizardContext&& context);
