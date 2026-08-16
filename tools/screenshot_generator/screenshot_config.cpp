#include "screenshot_config.h"

#include "base/boost_json_file.h"
#include "model/node_id_util.h"
#include "screenshot_options.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

std::filesystem::path GetImageManifestPath() {
  const auto& options = GetScreenshotOptions();
  if (!options.image_manifest.empty()) {
    return options.image_manifest.lexically_normal();
  }

  for (const auto& candidate : {
           std::filesystem::path{__FILE__}.parent_path() /
               "../../screenshots/image_manifest.json",
           std::filesystem::current_path() /
               "client/screenshots/image_manifest.json",
           std::filesystem::current_path() / "image_manifest.json",
       }) {
    if (std::filesystem::exists(candidate))
      return candidate.lexically_normal();
  }
  return {};
}

std::unordered_set<std::string> GetManagedImageFilenames() {
  std::unordered_set<std::string> filenames;

  const auto manifest_path = GetImageManifestPath();
  if (manifest_path.empty())
    return filenames;

  auto manifest = ReadBoostJsonFromFile(manifest_path);
  if (!manifest)
    return filenames;

  if (const auto* images = manifest->as_object().if_contains("images")) {
    for (const auto& image : images->as_array()) {
      const auto& object = image.as_object();
      const auto* file = object.if_contains("file");
      const auto* tag = object.if_contains("tag");
      if (!file || !tag)
        continue;

      const std::string_view tag_value = tag->as_string().c_str();
      if (!tag_value.starts_with("auto-"))
        continue;

      filenames.emplace(std::string(file->as_string()));
    }
  }

  return filenames;
}

template <class Spec>
bool IsManagedImage(const std::unordered_set<std::string>& managed_images,
                    const Spec& spec) {
  // An explicit --only list overrides the managed-image gate: the caller named
  // exactly what to capture, which is how reshell / not-yet-published surfaces
  // (absent from the published manifest) are validated headless, e.g.
  //   client_screenshot_generator --theme=dark \
  //       --only hardware-tree.png,config-parameters.png --out <dir>
  // Without this, an unmanaged spec is dropped even when named on --only, so it
  // silently never renders.
  if (!GetScreenshotOptions().only_filenames.empty())
    return ShouldCaptureScreenshot(spec.filename);
  if (!managed_images.empty() && !managed_images.contains(spec.filename))
    return false;
  return ShouldCaptureScreenshot(spec.filename);
}

}  // namespace

bool IsAutoManagedImageFilename(std::string_view filename) {
  const auto managed_images = GetManagedImageFilenames();
  return managed_images.contains(std::string(filename));
}

void ScreenshotConfig::Load(const std::filesystem::path& path) {
  auto opt = ReadBoostJsonFromFile(path);
  ASSERT_TRUE(opt.has_value()) << "Failed to read " << path.string();
  json = std::move(*opt);
  const auto managed_images = GetManagedImageFilenames();

  if (const auto* node_id =
          json.as_object().if_contains("dialog_analog_node_id")) {
    dialog_analog_node_id =
        NodeIdFromScadaString(std::string_view(node_id->as_string()));
  }
  ASSERT_FALSE(dialog_analog_node_id.is_null())
      << "Missing or invalid dialog_analog_node_id in " << path.string();

  if (const auto* users = json.as_object().if_contains("login_user_list")) {
    for (const auto& user : users->as_array())
      login_user_list.emplace_back(user.as_string());
  }
  ASSERT_FALSE(login_user_list.empty())
      << "Missing or empty login_user_list in " << path.string();

  // Under --only the caller drives the selection, so "skips" are just
  // unrequested specs, not managed-gate drops — don't report them.
  const bool only_mode = !GetScreenshotOptions().only_filenames.empty();
  std::vector<std::string> skipped_screenshots;
  for (const auto& js : json.at("screenshots").as_array()) {
    ScreenshotSpec spec;
    if (const auto* window_type = js.as_object().if_contains("type"))
      spec.window_type = std::string(window_type->as_string());
    if (const auto* capture = js.as_object().if_contains("capture"))
      spec.capture = std::string(capture->as_string());
    spec.filename = std::string(js.at("filename").as_string());
    // Exactly one of the two: `type` puts the spec on the profile page,
    // `capture` dispatches a standalone routine. Carrying both would leave a
    // window on the page that no capture ever grabs; carrying neither leaves
    // the spec with nothing to render.
    ASSERT_NE(spec.window_type.empty(), spec.capture.empty())
        << "Screenshot " << spec.filename << " in " << path.string()
        << " needs exactly one of \"type\" and \"capture\"";
    if (const auto* item_path = js.as_object().if_contains("path"))
      spec.path = std::string(item_path->as_string());
    if (const auto* item_paths = js.as_object().if_contains("paths")) {
      for (const auto& p : item_paths->as_array())
        spec.paths.emplace_back(p.as_string());
    }
    if (const auto* column_width = js.as_object().if_contains("column_width"))
      spec.column_width = static_cast<int>(column_width->as_int64());
    if (const auto* widths = js.as_object().if_contains("column_widths")) {
      for (const auto& w : widths->as_array())
        spec.column_widths.push_back(static_cast<int>(w.as_int64()));
    }
    if (const auto* cells = js.as_object().if_contains("cells")) {
      for (const auto& jc : cells->as_array()) {
        const auto& cell = jc.as_object();
        SheetCellSpec sheet_cell;
        sheet_cell.row = static_cast<int>(cell.at("row").as_int64());
        sheet_cell.column = static_cast<int>(cell.at("col").as_int64());
        sheet_cell.text = std::string(cell.at("text").as_string());
        if (const auto* align = cell.if_contains("align"))
          sheet_cell.align = std::string(align->as_string());
        if (const auto* color = cell.if_contains("color"))
          sheet_cell.color = std::string(color->as_string());
        spec.cells.push_back(std::move(sheet_cell));
      }
    }
    spec.width = static_cast<int>(js.at("width").as_int64());
    spec.height = static_cast<int>(js.at("height").as_int64());
    if (const auto* min_rows = js.as_object().if_contains("min_rows"))
      spec.min_rows = static_cast<int>(min_rows->as_int64());
    if (const auto* exact_rows = js.as_object().if_contains("exact_rows"))
      spec.exact_rows = static_cast<int>(exact_rows->as_int64());
    if (const auto* min_columns = js.as_object().if_contains("min_columns"))
      spec.min_columns = static_cast<int>(min_columns->as_int64());
    if (const auto* click_object = js.as_object().if_contains("click_object"))
      spec.click_object = std::string(click_object->as_string());
    if (const auto* expand = js.as_object().if_contains("expand"))
      spec.expand = expand->as_bool();
    if (const auto* graph_config = js.as_object().if_contains("graph"))
      spec.graph_config = std::string(graph_config->as_string());
    if (IsManagedImage(managed_images, spec))
      screenshots.push_back(std::move(spec));
    else if (!only_mode)
      skipped_screenshots.push_back(spec.filename);
  }

  // Surface fixture specs the managed-image gate dropped, so a missing capture
  // is discoverable: re-run with --only <name> (which overrides the gate) to
  // render them anyway. Silent when nothing was skipped (e.g. under --only).
  if (!skipped_screenshots.empty()) {
    std::sort(skipped_screenshots.begin(), skipped_screenshots.end());
    std::string list;
    for (const auto& name : skipped_screenshots) {
      if (!list.empty())
        list += ", ";
      list += name;
    }
    std::cout << "Skipped " << skipped_screenshots.size()
              << " fixture screenshot(s) not in the managed manifest set "
                 "(pass --only <name> to capture): "
              << list << std::endl;
  }

  if (const auto* jd = json.as_object().if_contains("dialogs")) {
    for (const auto& js : jd->as_array()) {
      DialogSpec spec;
      spec.kind = std::string(js.at("kind").as_string());
      spec.filename = std::string(js.at("filename").as_string());
      if (auto* w = js.as_object().if_contains("width"))
        spec.width = static_cast<int>(w->as_int64());
      if (auto* h = js.as_object().if_contains("height"))
        spec.height = static_cast<int>(h->as_int64());
      if (auto* themed = js.as_object().if_contains("themed_only"))
        spec.themed_only = themed->as_bool();
      if (auto* combo = js.as_object().if_contains("expand_combo"))
        spec.expand_combo = std::string(combo->as_string());
      if (auto* node = js.as_object().if_contains("node")) {
        spec.node_id =
            NodeIdFromScadaString(std::string_view(node->as_string()));
        ASSERT_FALSE(spec.node_id.is_null())
            << "Invalid node override on dialog " << spec.filename << " in "
            << path.string();
      }
      if (IsManagedImage(managed_images, spec))
        dialogs.push_back(std::move(spec));
    }
  }
}

std::filesystem::path GetDataFilePath() {
  for (auto candidate : {
           std::filesystem::path{__FILE__}.parent_path() /
               "screenshot_data.json",
           std::filesystem::current_path() / "screenshot_data.json",
       }) {
    if (std::filesystem::exists(candidate))
      return candidate;
  }
  return "screenshot_data.json";
}
