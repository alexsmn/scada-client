# Client Telemetry Gaps (traces and logs)

- **Status:** Plan
- **Owns:** the client half of the OpenTelemetry pipeline (`app/client_application.cpp`, `core/core_module.cpp`)
- **Last verified against code:** 2026-07-24

The server exports all three OpenTelemetry signals; the Qt client exports one.
This document scopes closing that gap. It exists because the E2E suite can now
be pointed at a telemetry viewer
([`e2e-client-server.md`](e2e-client-server.md), "Viewing a run's telemetry")
and the client is the conspicuous blank in what that viewer shows.

## Current state

| Signal | Server tiers | Qt client |
|---|---|---|
| Metrics | `MetricModule` (`metrics` config block) | ✅ `OpenTelemetryMetrics`, `--otlp-endpoint` |
| Traces | `CompositeTraceSink` = watchdog + `OtelTraceSink` | ❌ watchdog sink only |
| Logs | `ScopedOtelLogSink` on the Boost.Log core (`log.otlp`) | ❌ file/console sinks only |

The client is not missing the *abstractions* — it already builds a `Tracer`.
[`core/core_module.cpp`](../core/core_module.cpp) constructs a `TraceSinkImpl`
(the hung-span watchdog, 15 s) and wraps it in a `Tracer`, which is exactly the
shape the server had before OTel export was added. Everything the client spans
today is logged locally by the watchdog and then dropped.

## Consequences

1. **Every waterfall starts at the server.** The client's session and view
   proxies do set `Request.trace_id` from the `ServiceContext`
   ([`core/remote/session_proxy.cpp:628`](../../core/remote/session_proxy.cpp:628),
   [`view_service_proxy.cpp:26`](../../core/remote/view_service_proxy.cpp:26)),
   but with no exporting sink there is no client span to parent to, so the value
   is not a W3C traceparent and the server's `scada.grpc/*` span starts a fresh
   root. A trace therefore answers "what did the server do" but never "how long
   did the user wait", which is the question a desktop client's telemetry is
   worth collecting for.
2. **Client log records never reach the viewer.** They stay in the per-run
   `ClientLogs/` directory the E2E assertions grep. Correlating a client-side
   error with the server spans it triggered is a manual, timestamp-eyeballing
   exercise.
3. **A whole tier of latency is invisible** — UI work, the node-service tree
   build, profile load — none of it is on the timeline next to the server calls
   it interleaves with.

## Work items

### 3a. Client trace export (the substantial one)

Mirror the framework's composition in the client's `CoreModule`. The server
does exactly this in
[`scada-server-framework/modules/core/core_module.cpp:63`](../../scada-server-framework/modules/core/core_module.cpp:63):
collect sinks into a `std::vector<TraceSink*>`, add an `OtelTraceSink` over an
`OpenTelemetryTraces` runtime when export is configured, and hand a
`CompositeTraceSink` to the `Tracer`. The client's `CoreModule` takes only an
`AnyExecutor` today, so it needs the endpoint/sampling parameters threaded in
from `ClientApplication`.

The real work is not the wiring but **choosing the spans**. The server's seams
are natural choke points (service stubs, adapters); the client's are not
obvious, and instrumenting the wrong thing produces either noise or a flat,
useless trace. Candidates, roughly in value order:

- outbound service calls at the proxy boundary (mirrors the server's CLIENT
  spans, and gives the traceparent that fixes consequence 1);
- login/bootstrap phases (`ClientApplication::PostLogin()`), which is where a
  slow start actually hurts;
- node-service tree fetches and monitored-item subscribe batches.

UI-thread work is deliberately excluded from this list: spanning Qt event
handling invites per-repaint spans that drown the trace.

Also required for consequence 1 to actually resolve: `ServiceContext` must
carry the started span's `traceparent()` before the proxies read `trace_id()`,
following the three-step convention in
[`tracing.md`](../../scada-server-framework/docs/tracing.md), "Context
propagation".

**Risk to keep in view:** the client is a long-running interactive process, not
a request-scoped server. Span lifetime is tied to UI and coroutine flows that
can be abandoned (a cancelled dialog, a closed view), so a naive RAII span can
outlive its logical operation and trip the 15 s hung-span watchdog. Expect to
spend real effort on span *ending*, not span starting.

### 3b. Client OTLP log export (small)

`ScopedOtelLogSink` is already a standalone, Boost.Log-level component and the
client already uses Boost.Log. Adapting
[`ServerOtelLogExport`](../../scada-server-framework/base/server.cpp:300) —
an `OpenTelemetryLogs` runtime plus the scoped sink, in that declaration order
so the sink detaches before the provider shuts down — is close to a copy. The
open questions are policy, not plumbing: which severity floor, and whether the
client should reuse `--otlp-endpoint` or take its own switch.

Worth doing **before** 3a: it is far cheaper, it is what makes a failing E2E
run readable in one place, and `OtelLogSinkBackend` already stamps
`trace_id`/`span_id` from the `TraceParent` log attribute, so the correlation
plumbing lights up on its own once 3a lands.

### 3c. Configuration shape (small, do with 3a)

`--otlp-endpoint` and `--otlp-export-interval-ms` were added as command-line
switches because that is what the E2E harness can drive. If traces and logs
each grow their own knobs (sampling ratio, severity floor), the switch list
stops scaling and the client should read a `metrics`/`log.otlp` block from its
settings instead — matching the server's JSON shape rather than inventing a
second vocabulary for the same pipeline.

## Suggested order

**3b → 3c → 3a.** Logs first for immediate payoff at near-zero risk, then the
config shape while there are still few knobs to migrate, then traces once there
is somewhere coherent to configure them and a decision on which spans are worth
having.
