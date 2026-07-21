#pragma once

struct ScreenshotSpec;
class NodeService;

// Renders the reshelled transmission-rule inspector — the right region of
// transmission-rules.html — from a fixture transmission item, and saves it
// under `GetOutputDir() / spec.filename`.
//
// Standalone like SaveUserAccessScreenshot: it makes the fixture rule (its
// source, type chain, address value and parent endpoint) resident, builds a
// fresh TransmissionRuleInspector, drives it via ShowRule with the real node
// service, then grabs the widget — exercising the real source/endpoint/protocol
// resolution and the Address read.
void SaveTransmissionRuleScreenshot(const ScreenshotSpec& spec,
                                    NodeService& node_service);
