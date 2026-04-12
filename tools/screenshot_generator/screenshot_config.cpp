#include "screenshot_config.h"

#include "base/boost_json_file.h"

#include <gtest/gtest.h>

void ScreenshotConfig::Load(const std::filesystem::path& path) {
  auto opt = ReadBoostJsonFromFile(path);
  ASSERT_TRUE(opt.has_value()) << "Failed to read " << path.string();
  json = std::move(*opt);

  for (const auto& js : json.at("screenshots").as_array()) {
    ScreenshotSpec spec;
    spec.window_type = std::string(js.at("type").as_string());
    spec.filename = std::string(js.at("filename").as_string());
    spec.width = static_cast<int>(js.at("width").as_int64());
    spec.height = static_cast<int>(js.at("height").as_int64());
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
