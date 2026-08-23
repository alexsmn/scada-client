#pragma once

#include "base/any_executor.h"

struct ScreenshotSpec;
class NodeService;

// Renders the reshelled transmission-rule inspector — the right region of
// transmission-rules.html — from a fixture transmission item, and saves it
// under `GetOutputDir() / spec.filename`.
//
// Standalone like SaveUserAccessScreenshot: it builds a fresh
// TransmissionRuleInspector, wires the load handler the shell wires, and drives
// it via ShowRule with the real node service — so the capture exercises the
// panel's own fetch as well as the source/endpoint/protocol resolution and the
// Address read. The executor is what the load runs on.
void SaveTransmissionRuleScreenshot(const ScreenshotSpec& spec,
                                    NodeService& node_service,
                                    AnyExecutor executor);
