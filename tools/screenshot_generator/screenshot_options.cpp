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

void InitScreenshotOptions() {
  if (g_options_initialized)
    return;

  namespace po = boost::program_options;

  std::string output_dir;
  std::string image_manifest;
  std::string only;

  po::options_description desc{"Screenshot generator options"};
  desc.add_options()
      ("out", po::value(&output_dir)->required(),
       "Output directory for screenshots")
      ("image-manifest", po::value(&image_manifest),
       "Path to screenshot image manifest")
      ("only", po::value(&only),
       "Comma/semicolon/newline-separated filenames to capture");

  po::variables_map vm;
  auto args = GetProcessArgs();
  po::store(po::command_line_parser(args).options(desc).allow_unregistered().run(),
            vm);
  po::notify(vm);

  g_options.output_dir = std::move(output_dir);

  if (vm.count("image-manifest")) {
    g_options.image_manifest = std::move(image_manifest);
  }

  if (vm.count("only")) {
    g_options.only_filenames = ParseOnlyList(only);
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
