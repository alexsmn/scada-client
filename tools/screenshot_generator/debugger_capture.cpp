#include "debugger_capture.h"
#include "publish_guard.h"

#include "screenshot_config.h"
#include "widget_capture.h"

#include "modules/debugger/debugger_context.h"
#include "modules/debugger/qt/debugger_qt.h"
#include "scada/session_debugger.h"
#include "aui/qt/table.h"
#include "scada/session_service_mock.h"

#include <QApplication>
#include <QTabWidget>
#include <QWidget>
#include <boost/signals2/signal.hpp>
#include <gmock/gmock.h>

#include <chrono>
#include <memory>
#include <thread>

namespace {

// The only seam RequestTableModel has: it subscribes through
// SessionService::GetSessionDebugger(). Replaying events through it drives the
// real model rather than a stand-in for it.
class FixtureSessionDebugger : public scada::SessionDebugger {
 public:
  boost::signals2::scoped_connection SubscribeRequestEvents(
      const RequestEventCallback& callback) override {
    return signal_.connect(callback);
  }

  void Emit(const RequestEvent& event) { signal_(event); }

 private:
  boost::signals2::signal<void(const RequestEvent&)> signal_;
};

// A trace with one of each status, so the capture shows the three colourings
// the model applies (succeeded = default, running = uncertain, failed = bad)
// instead of a uniformly green list.
void ReplayFixtureTrace(FixtureSessionDebugger& debugger) {
  using Phase = scada::SessionDebugger::RequestPhase;

  struct Step {
    scada::SessionDebugger::RequestId id;
    Phase phase;
    const char* title;
    const char* body;
    const char* response;
  };

  // Each finished request is replayed twice — Running, then its terminal phase
  // — because that is the sequence a live session produces, and it is what
  // gives the row a start and a finish to measure a duration between.
  static const Step kSteps[] = {
      {1, Phase::Running, "Browse", "NodeId: i=85 (Objects)", ""},
      {1, Phase::Succeeded, "Browse", "NodeId: i=85 (Objects)",
       "References: 14"},
      {2, Phase::Running, "Read", "NodeId: ns=2;s=KPY.TC1.I, Attribute: Value",
       ""},
      {2, Phase::Succeeded, "Read", "NodeId: ns=2;s=KPY.TC1.I, Attribute: Value",
       "Value: 195.7 A, Quality: Good"},
      {3, Phase::Running, "CreateMonitoredItems",
       "SubscriptionId: 4, Items: 12", ""},
      {3, Phase::Succeeded, "CreateMonitoredItems",
       "SubscriptionId: 4, Items: 12", "Created: 12, Revised interval: 1000 ms"},
      {4, Phase::Running, "HistoryRead",
       "NodeId: ns=2;s=ESTRA.T, Range: last 24 h", ""},
      {4, Phase::Failed, "HistoryRead",
       "NodeId: ns=2;s=ESTRA.T, Range: last 24 h",
       "BadNoDataAvailable (0x809B0000)"},
      // Left Running so the trace shows an outstanding request too.
      {5, Phase::Running, "Call",
       "ObjectId: ns=2;s=ESTRA.TU.Q1, Method: Operate", ""},
  };

  for (const Step& step : kSteps) {
    // RequestTableModel stamps start/finish from std::chrono::system_clock,
    // which the generator's frozen scada clock does not cover. Replaying the
    // whole trace in one go would therefore render "0 ms" in every Duration
    // cell. Pausing briefly before each terminal phase lets the model measure
    // a real interval, so the column shows plausible durations rather than
    // fabricated ones.
    if (step.phase != Phase::Running)
      std::this_thread::sleep_for(std::chrono::milliseconds(35));
    debugger.Emit({.request_id = step.id,
                   .phase = step.phase,
                   .title = step.title,
                   .body = step.body,
                   .response_body = step.response});
  }
}

// `Debugger::Open()` creates its window unparented and shows it, so the capture
// has to find it rather than being handed it.
QWidget* FindDebuggerWindow() {
  for (QWidget* widget : QApplication::topLevelWidgets()) {
    if (qobject_cast<QTabWidget*>(widget) &&
        widget->windowTitle() == QStringLiteral("Debugger")) {
      return widget;
    }
  }
  return nullptr;
}

}  // namespace

void SaveDebuggerScreenshot(const ScreenshotSpec& spec) {
  CapturePublishGuard publish_guard{spec.filename};

  FixtureSessionDebugger session_debugger;
  ::testing::NiceMock<scada::MockSessionService> session_service;
  ON_CALL(session_service, GetSessionDebugger())
      .WillByDefault(::testing::Return(&session_debugger));

  // Order matters: the Debugger's constructor builds the RequestTableModel,
  // which is what subscribes. Events replayed before that would go nowhere.
  Debugger debugger{DebuggerContext{.session_service_ = session_service}};
  ReplayFixtureTrace(session_debugger);
  debugger.Open();

  QWidget* window = FindDebuggerWindow();
  EXPECT_NE(window, nullptr) << "Debugger::Open() created no window to capture";

  // Select a row: the detail pane is driven by the table's selection-change
  // handler, so without this half the capture is an empty QTextEdit.
  // aui::Table carries no Q_OBJECT, so it has to be found by dynamic_cast
  // rather than findChild<> (same as FindTreeWidget in main.cpp).
  if (window) {
    scada::aui::Table* table = nullptr;
    for (QWidget* child : window->findChildren<QWidget*>()) {
      if ((table = dynamic_cast<scada::aui::Table*>(child)))
        break;
    }
    if (table)
      table->SelectRow(0);
    else
      ADD_FAILURE() << "no aui::Table in the debugger window to select";
  }

  if (!publish_guard.ShouldPublish())
    return;

  SaveScreenshot(window, spec);

  if (window)
    delete window;
}
