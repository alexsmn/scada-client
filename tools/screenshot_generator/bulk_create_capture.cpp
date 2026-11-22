#include "bulk_create_capture.h"

#include "screenshot_config.h"
#include "widget_capture.h"

#include "base/utf_convert.h"
#include "bulk_create/bulk_create_pattern.h"
#include "bulk_create/qt/bulk_create_preview_panel.h"

#include <boost/json.hpp>

#include <gtest/gtest.h>

#include <string>

void SaveBulkCreateScreenshot(const ScreenshotSpec& spec,
                              const boost::json::value& json) {
  // The figure follows dev/data-items.md's worked example, which sits right
  // above it: five TI «Ток фидера 1» … «Ток фидера 5». Until 2026-11-22 it
  // showed a run of transmission rules («Правило {n}» with IOA 4001...), which
  // is a different branch of the wizard from the one that page teaches. The
  // template lives in the fixture rather than here because client source may
  // not carry Russian literals.
  const auto* block = json.as_object().if_contains("bulk_create");
  if (!block) {
    ADD_FAILURE() << spec.filename << ": the fixture has no bulk_create block";
    return;
  }
  const boost::json::object& example = block->as_object();

  BulkCreatePreviewPanel panel;

  BulkCreateParams params;
  // Data items: the subject the page describes. Its preview has no address
  // column, which only a transmission rule carries.
  params.subject = BulkCreateSubject::kDataItem;
  params.name_template = UtfConvert<char16_t>(
      std::string{example.at("name_template").as_string()});
  // Empty, as the wizard proposes and the manual recommends: the Server
  // assigns the ids. A string template is one the Server refuses (backlog
  // 960), so the manual must not show one filled in.
  params.node_id_template = {};
  params.start_index = static_cast<int>(example.at("start_index").as_int64());
  params.count = static_cast<int>(example.at("count").as_int64());
  params.index_step = static_cast<int>(example.at("index_step").as_int64());
  panel.SetParams(params);

  SaveScreenshot(&panel, spec);
}
