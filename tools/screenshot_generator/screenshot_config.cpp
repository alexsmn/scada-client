#include "screenshot_config.h"

#include "screenshot_options.h"
#include "base/boost_json_file.h"
#include "model/node_id_util.h"

#include <gtest/gtest.h>

#include <unordered_set>

namespace {

std::filesystem::path GetImageManifestPath() {
  const auto& options = GetScreenshotOptions();
  if (!options.image_manifest.empty()) {
    return options.image_manifest.lexically_normal();
  }

  for (const auto& candidate : {
           std::filesystem::path{__FILE__}.parent_path() /
               "../../docs/screenshots/image_manifest.json",
           std::filesystem::current_path() /
               "client/docs/screenshots/image_manifest.json",
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

  if (const auto* node_id = json.as_object().if_contains("dialog_analog_node_id")) {
    dialog_analog_node_id =
        NodeIdFromScadaString(std::string_view(node_id->as_string()));
  }
  ASSERT_FALSE(dialog_analog_node_id.is_null())
      << "Missing or invalid dialog_analog_node_id in " << path.string();

  for (const auto& js : json.at("screenshots").as_array()) {
    ScreenshotSpec spec;
    spec.window_type = std::string(js.at("type").as_string());
    spec.filename = std::string(js.at("filename").as_string());
    if (const auto* item_path = js.as_object().if_contains("path"))
      spec.path = std::string(item_path->as_string());
    spec.width = static_cast<int>(js.at("width").as_int64());
    spec.height = static_cast<int>(js.at("height").as_int64());
    if (IsManagedImage(managed_images, spec))
      screenshots.push_back(std::move(spec));
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
