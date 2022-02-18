#include "services/progress_host_impl.h"

namespace {

void Merge(const ProgressStatus& from, ProgressStatus& to) {
  if (!from.active)
    return;

  to.active = true;
  to.range += from.range;
  to.current += from.current;
}

}  // namespace

// ProgressHostImpl::RunningProgressImpl

class ProgressHostImpl::RunningProgressImpl : public RunningProgress {
 public:
  explicit RunningProgressImpl(ProgressHostImpl& progress_host_impl)
      : progress_host_impl_{progress_host_impl} {}

  ~RunningProgressImpl() {
    progress_host_impl_.RemoveRunningProgressImpl(*this);
  }

  const ProgressStatus& progress_status() const { return progress_status_; }

  virtual void SetProgress(int range, int position) override {
    progress_status_ = {true, range, position};

    progress_host_impl_.UpdateProgressStatus();
  }

  virtual void SetStatus(std::u16string_view status) override {}

  virtual bool IsCanceled() override { return false; }

 private:
  ProgressHostImpl& progress_host_impl_;

  ProgressStatus progress_status_;
};

// ProgressHostImpl

std::unique_ptr<RunningProgress> ProgressHostImpl::Start() {
  auto running_progress_impl = std::make_unique<RunningProgressImpl>(*this);
  running_progress_impls_.emplace_back(running_progress_impl.get());
  return running_progress_impl;
}

boost::signals2::scoped_connection ProgressHostImpl::Subscribe(
    const ProgressCallback& callback) {
  return signal_.connect(callback);
}

void ProgressHostImpl::RemoveRunningProgressImpl(
    RunningProgressImpl& running_progress_impl) {
  auto i = std::find(running_progress_impls_.begin(),
                     running_progress_impls_.end(), &running_progress_impl);
  assert(i != running_progress_impls_.end());
  if (i != running_progress_impls_.end())
    running_progress_impls_.erase(i);

  UpdateProgressStatus();
}

void ProgressHostImpl::UpdateProgressStatus() {
  ProgressStatus progress_status;
  for (auto* running_progress_impl : running_progress_impls_)
    Merge(running_progress_impl->progress_status(), progress_status);

  if (progress_status_ == progress_status)
    return;

  progress_status_ = progress_status;
  signal_(progress_status_);
}
