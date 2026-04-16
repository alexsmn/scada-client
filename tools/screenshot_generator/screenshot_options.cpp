#include "screenshot_options.h"

#include "base/utf_convert.h"

#include <boost/program_options.hpp>

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#include <shellapi.h>
#endif

namespace {

ScreenshotOptions g_options;
bool g_options_initialized = false;

std::string TrimAsciiWhitespace(std::string value) {
  auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };
  auto begin = std::ranges::find_if_not(value, is_space);
  auto end = std::ranges::find_if_not(value.rbegin(), value.rend(), is_space)
                 .base();
  if (begin >= end)
    return {};
  return std::string(begin, end);
}

std::vector<std::string> GetProcessArgs() {
  std::vector<std::string> args;

#ifdef _WIN32
  int argc = 0;
  LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
  if (!wargv)
    return args;

  for (int i = 1; i < argc; ++i)
    args.push_back(UtfConvert<char>(wargv[i]));
  LocalFree(wargv);
#endif

  return args;
}

std::unordered_set<std::string> ParseOnlyList(std::string_view raw) {
  std::unordered_set<std::string> filenames;
  std::string token;
  for (char ch : raw) {
    if (ch == ',' || ch == ';' || ch == '\n' || ch == '\r') {
      auto trimmed = TrimAsciiWhitespace(std::move(token));
      if (!trimmed.empty())
        filenames.emplace(std::move(trimmed));
      token.clear();
      continue;
    }
    token.push_back(ch);
  }

  auto trimmed = TrimAsciiWhitespace(std::move(token));
  if (!trimmed.empty())
    filenames.emplace(std::move(trimmed));

  return filenames;
}

}  // namespace

void InitScreenshotOptions() {
  if (g_options_initialized)
    return;

  namespace po = boost::program_options;

  po::options_description desc;
  desc.add_options()
      ("out", po::value<std::string>(), "Output directory for screenshots")
      ("image-manifest", po::value<std::string>(),
       "Path to screenshot image manifest")
      ("only", po::value<std::string>(),
       "Comma/semicolon/newline-separated filenames to capture");

  po::variables_map vm;
  auto args = GetProcessArgs();
  po::store(po::command_line_parser(args).options(desc).allow_unregistered().run(),
            vm);
  po::notify(vm);

  if (!vm.count("out")) {
    throw std::runtime_error{
        "--out must be provided for screenshot_generator"};
  }

  g_options.output_dir = vm["out"].as<std::string>();

  if (vm.count("image-manifest")) {
    g_options.image_manifest = vm["image-manifest"].as<std::string>();
  }

  if (vm.count("only")) {
    g_options.only_filenames = ParseOnlyList(vm["only"].as<std::string>());
  }

  g_options_initialized = true;
}

const ScreenshotOptions& GetScreenshotOptions() {
  if (!g_options_initialized)
    InitScreenshotOptions();
  return g_options;
}

bool ShouldCaptureScreenshot(std::string_view filename) {
  const auto& only = GetScreenshotOptions().only_filenames;
  return only.empty() || only.contains(std::string(filename));
}
