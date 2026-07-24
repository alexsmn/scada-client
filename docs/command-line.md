# Command-Line Switches

## Logging

* verbose-logging

* log-service-read

* log-service-browse

* log-service-history

* log-service-event

* log-service-model-change-event

* log-service-node-semantics-change-event

## Telemetry

* otlp-endpoint=`<host:port>`

  OTLP/gRPC collector the process metrics (`service.name = scada-client`) are
  exported to, e.g. `localhost:4317`. Unset — the default — disables export
  entirely: an empty endpoint makes the OTLP exporter log `empty endpoint` and
  return a null channel, so nothing leaves the process.

  The client exports **metrics only**. It runs no trace sink and no OTLP log
  sink, so its spans and log records stay local; see
  [`e2e-client-server.md`](e2e-client-server.md), "Viewing a run's telemetry".

* otlp-export-interval-ms=`<ms>`

  Metric export period, default `60000`. Short-lived runs need a smaller value
  — a process that exits before the first export reports nothing at all.
  Non-numeric or non-positive values fall back to the default rather than
  failing startup.
