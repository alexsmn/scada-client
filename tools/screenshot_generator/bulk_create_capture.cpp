#include "bulk_create_capture.h"

#include "screenshot_config.h"
#include "widget_capture.h"

#include "bulk_create/bulk_create_pattern.h"
#include "bulk_create/qt/bulk_create_preview_panel.h"

void SaveBulkCreateScreenshot(const ScreenshotSpec& spec) {
  BulkCreatePreviewPanel panel;

  // One existing NodeId so the preview flags a conflict, mirroring the mockup's
  // "1 conflict" (TS8 already exists).
  panel.SetExistingNodeIds({u"ns=2;s=RTU.TS8.I"});

  BulkCreateParams params;
  params.name_template = u"TS{n} current";
  params.node_id_template = u"ns=2;s=RTU.TS{n}.I";
  params.start_index = 1;
  params.count = 12;
  params.index_step = 1;
  // Stated explicitly, and it is the whole subject of this capture: the IOA
  // fields below and the Адрес column only exist for a transmission item. The
  // default is kDataItem, and leaving it there is what made the tracked image
  // render address 0 on every row while still showing the address controls.
  params.subject = BulkCreateSubject::kTransmissionItem;
  params.ioa_start = 4001;
  params.ioa_step = 1;
  panel.SetParams(params);

  SaveScreenshot(&panel, spec);
}
