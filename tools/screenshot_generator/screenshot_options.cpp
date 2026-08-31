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
#elif defined(__APPLE__)
#include <crt_externs.h>
#endif

namespace {

ScreenshotOptions g_options;
bool g_options_initialized = false;

std::string TrimAsciiWhitespace(std::string value) {
  auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };
  auto begin = std::ranges::find_if_not(value, is_space);
  auto end =
      std::ranges::find_if_not(value.rbegin(), value.rend(), is_space).base();
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
#elif defined(__APPLE__)
  auto* argc = _NSGetArgc();
  auto*** argv = _NSGetArgv();
  if (!argc || !argv || !*argv)
    return args;

  for (int i = 1; i < *argc; ++i)
    args.emplace_back((*argv)[i]);
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

ScreenshotOptions ParseScreenshotOptions(std::span<const std::string> args) {
  namespace po = boost::program_options;

  std::string output_dir;
  std::string image_manifest;
  std::string data_file;
  std::string only;
  std::string theme;

  po::options_description desc{"Screenshot generator options"};
  desc.add_options()("out", po::value(&output_dir)->required(),
                     "Output directory for screenshots")(
      "image-manifest", po::value(&image_manifest),
      "Path to screenshot image manifest")(
      "data", po::value(&data_file),
      "Path to the screenshot fixture (default: the source tree's "
      "screenshot_data.json)")(
      "only", po::value(&only),
      "Comma/semicolon/newline-separated filenames to capture")(
      "theme", po::value(&theme),
      "Design-token appearance to render under: dark|light|hc (default: dark)");

  // `allow_unregistered` is what lets `--gtest_*` reach gtest untouched. It is
  // also what silently swallowed a misspelt or non-existent flag, so the
  // leftovers are collected and rejected below rather than discarded.
  const std::vector<std::string> arg_copy(args.begin(), args.end());
  auto parsed = po::command_line_parser(arg_copy)
                    .options(desc)
                    .allow_unregistered()
                    .run();

  std::vector<std::string> unknown;
  for (auto& token :
       po::collect_unrecognized(parsed.options, po::include_positional)) {
    if (token.starts_with("--gtest_"))
      continue;
    unknown.push_back(std::move(token));
  }
  if (!unknown.empty()) {
    std::string message = "unknown option(s):";
    for (const auto& token : unknown) {
      message += ' ';
      message += token;
    }
    throw std::runtime_error(message);
  }

  po::variables_map vm;
  po::store(std::move(parsed), vm);
  po::notify(vm);

  ScreenshotOptions options;
  options.output_dir = std::move(output_dir);

  if (vm.count("image-manifest")) {
    options.image_manifest = std::move(image_manifest);
  }

  if (vm.count("data")) {
    options.data_file = std::move(data_file);
  }

  if (vm.count("only")) {
    options.only_filenames = ParseOnlyList(only);
  }

  if (vm.count("theme")) {
    options.theme = std::move(theme);
  }

  return options;
}

void InitScreenshotOptions() {
  if (g_options_initialized)
    return;

  g_options = ParseScreenshotOptions(GetProcessArgs());
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
