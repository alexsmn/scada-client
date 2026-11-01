#include "bulk_create_capture.h"

#include "screenshot_config.h"
#include "widget_capture.h"

#include "bulk_create/bulk_create_pattern.h"
#include "bulk_create/qt/bulk_create_preview_panel.h"

void SaveBulkCreateScreenshot(const ScreenshotSpec& spec) {
  BulkCreatePreviewPanel panel;

  // One address a rule on the destination already carries, so row 3 shows
  // the duplicate-address flag a rule landing on a taken IOA gets.
  panel.SetExistingAddresses({4003});

  BulkCreateParams params;
  params.name_template = u"TS{n} current";
  // Empty, as the wizard now proposes and the manual recommends: the Server
  // assigns the ids. The former string template is one the Server refuses
  // (backlog 960), so the manual must not show it filled in.
  params.node_id_template = {};
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
