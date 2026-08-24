#pragma once

#include "base/lifetime.h"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

// A temp directory owned by one test fixture, removed when the fixture is.
//
// The client's twin of `scada-server-framework/test/scoped_temp_dir.h`. It is
// duplicated rather than shared because `client/` is a separate product that
// must stay standalone (ADR 0011) and may not reach into the framework's tree;
// keep the two in step.
//
// Prefer this to naming a directory by hand. The pattern it replaces —
//
//     temp_dir_ = std::filesystem::temp_directory_path() /
//                 ("scada_test_" + std::to_string(
//                      std::chrono::steady_clock::now()
//                          .time_since_epoch().count()));
//     std::filesystem::create_directories(temp_dir_);
//
// is wrong in ways that compound. The salt is a clock reading, so uniqueness is
// a probability rather than a fact. `create_directories` succeeds just as
// happily on a directory that already exists, so a collision is silent — two
// fixtures share one tree and the failure surfaces as an unexplained assertion
// somewhere else. And fixtures reusing the same prefix had only that clock
// reading keeping them apart from each other rather than merely from
// themselves. A fixed path is the same bug with the probability set to one.
//
// Here the PID separates concurrent processes — which is what `ctest -j` and
// two checkouts testing at once produce — and `create_directory`, which reports
// whether it created the directory or merely found it, separates fixtures
// within one process by walking the suffix until it wins.
//
// Declare it BEFORE any member that opens a file inside it. Members are
// destroyed in reverse declaration order, so a ScopedTempDir declared last is
// destroyed first, and removing the tree out from under an open handle fails on
// Windows and silently leaves it behind.
class ScopedTempDir {
 public:
  // `prefix` names the fixture in the directory name, for the rare case where
  // one survives a crash and someone has to work out where it came from.
  explicit ScopedTempDir(std::string_view prefix = "scada_client_test") {
    const std::filesystem::path base = std::filesystem::temp_directory_path();
    const std::string stem =
        std::string{prefix} + "_" + std::to_string(CurrentPid()) + "_";
    for (int attempt = 0; attempt < 1000; ++attempt) {
      std::filesystem::path candidate = base / (stem + std::to_string(attempt));
      std::error_code ec;
      if (std::filesystem::create_directory(candidate, ec)) {
        path_ = std::move(candidate);
        return;
      }
    }
    throw std::runtime_error{"Could not create a unique temp directory under " +
                             base.string()};
  }

  ScopedTempDir(const ScopedTempDir&) = delete;
  ScopedTempDir& operator=(const ScopedTempDir&) = delete;

  ~ScopedTempDir() {
    std::error_code ec;
    std::filesystem::remove_all(path_, ec);
  }

  const std::filesystem::path& path() const SCADA_LIFETIME_BOUND {
    return path_;
  }

 private:
  static int CurrentPid() {
#ifdef _WIN32
    return ::_getpid();
#else
    return static_cast<int>(::getpid());
#endif
  }

  std::filesystem::path path_;
};
