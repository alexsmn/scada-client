#include "core/core_module.h"

#include "core/progress_host_impl.h"
#include "metrics/trace_sink_impl.h"
#include "metrics/tracer.h"

using namespace std::chrono_literals;

CoreModule::CoreModule(std::shared_ptr<Executor> executor) {
  auto trace_sink = std::make_shared<TraceSinkImpl>(executor, 15s);
  singletons_.emplace(trace_sink);

  tracer_ = std::make_unique<Tracer>(*trace_sink);

  progress_host_ = std::make_unique<ProgressHostImpl>();
}

CoreModule::~CoreModule() {}