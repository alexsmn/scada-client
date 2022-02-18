#pragma once

#include <memory>
#include <string_view>

#include <boost/signals2/connection.hpp>

class RunningProgress {
 public:
  virtual ~RunningProgress() = default;

  virtual void SetProgress(int range, int position) = 0;

  virtual void SetStatus(std::u16string_view status) = 0;

  virtual bool IsCanceled() = 0;
};

struct ProgressStatus {
  bool active = false;
  int range = 0;
  int current = 0;
};

class ProgressHost {
 public:
  virtual ~ProgressHost() = default;

  virtual std::unique_ptr<RunningProgress> Start() = 0;

  using ProgressCallback = std::function<void(const ProgressStatus& status)>;

  virtual boost::signals2::scoped_connection Subscribe(
      const ProgressCallback& callback) = 0;
};

inline bool operator==(const ProgressStatus& a, const ProgressStatus& b) {
  return std::tie(a.active, a.range, a.current) ==
         std::tie(b.active, b.range, b.current);
}
