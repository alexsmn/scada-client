#include "core_module.h"

#include "metrics/trace_sink_impl.h"
#include "metrics/tracing.h"

using namespace std::chrono_literals;

CoreModule::CoreModule(std::shared_ptr<Executor> executor) {
  auto trace_sink = std::make_shared<TraceSinkImpl>(executor, 15s);
  singletons_.emplace(trace_sink);

  root_trace_span_ = std::make_unique<TraceSpan>(*trace_sink);
}

CoreModule::~CoreModule() {}