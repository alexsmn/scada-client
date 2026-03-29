#include "base/program_options.h"

#ifdef _WIN32
#include <Windows.h>
#include <shellapi.h>
#endif

#include "base/utf_convert.h"

namespace client {

namespace {
boost::program_options::variables_map& GetVariablesMap() {
  static boost::program_options::variables_map vm;
  return vm;
}
}  // namespace

void InitProgramOptions() {
  namespace po = boost::program_options;

  po::options_description desc;
  desc.add_options()
      ("debug", "Enable debug mode")
      ("excel", "Enable Excel export")
      ("locale", po::value<std::string>(), "Application locale")
      ("verbose-logging", "Enable verbose logging")
      ("log-service-read", "Log service read operations")
      ("log-service-browse", "Log service browse operations")
      ("log-service-history", "Log service history operations")
      ("log-service-event", "Log service event operations")
      ("log-service-model-change-event", "Log model change events")
      ("log-service-node-semantics-change-event", "Log node semantics change events")
      ("log-alias-service", "Log alias service operations")
      ("node-service-v2", "Use v2 node service")
      ("log-severity", po::value<std::string>(), "Log severity level");

#ifdef _WIN32
  int argc = 0;
  LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
  if (!wargv) return;

  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i)
    args.push_back(UtfConvert<char>(wargv[i]));
  LocalFree(wargv);

  auto& vm = GetVariablesMap();
  po::store(po::command_line_parser(args).options(desc).allow_unregistered().run(), vm);
  po::notify(vm);
#endif
}

bool HasOption(std::string_view name) {
  return GetVariablesMap().count(std::string{name}) > 0;
}

std::string GetOptionValue(std::string_view name) {
  auto& vm = GetVariablesMap();
  auto it = vm.find(std::string{name});
  if (it == vm.end())
    return {};
  try {
    return it->second.as<std::string>();
  } catch (...) {
    return {};
  }
}

}  // namespace client
