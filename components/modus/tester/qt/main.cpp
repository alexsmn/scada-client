#include "base/client_paths.h"
#include "base/test/test_executor.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/modus/modus_component.h"
#include "components/modus/qt/modus_view.h"
#include "qt/message_loop_qt.h"
#include "services/atl_module.h"
#include "services/file_cache.h"
#include "services/file_registry.h"
#include "test/display_tester/qt/display_tester_state.h"
#include "test/display_tester/qt/display_tester_window.h"
#include "window_definition.h"

#include <QApplication>
#include <atlbase.h>

struct State : DisplayTesterState {
  std::shared_ptr<Executor> executor = std::make_shared<TestExecutor>();
  AliasResolver alias_resolver = [](std::string_view alias,
                                    const AliasResolveCallback& callback) {};
  FileRegistry file_registry;
  FileCache file_cache{file_registry};
};

QWidget* CreateModusView(State& state, const WindowDefinition& definition) {
  ModusView* modus_view = new ModusView{modus::ModusDocumentContext{
      .alias_resolver_ = state.alias_resolver,
      .timed_data_service_ = state.timed_data_service,
      .file_cache_ = state.file_cache,
      .title_callback_ = [](const std::u16string& title) {},
      .navigation_callback_ = [](std::u16string_view hyperlink) {},
      .selection_callback_ = [](const TimedDataSpec& selection) {},
      .context_menu_callback_ = [](const aui::Point& point) {},
      .enable_internal_render_callback_ = [] {}}};

  modus_view->Open(definition);

  return modus_view;
}

QWidget* CreateModusViewFromPath(State& state,
                                 const std::filesystem::path& path) {
  WindowDefinition definition{kModusWindowInfo};
  definition.path =
      !path.empty()
          ? path
          : std::filesystem::path{
                R"(c:\ProgramData\Telecontrol\SCADA Client\main.sde)"};

  return CreateModusView(state, definition);
}

DummyAtlModule _Module;

int main(int argc, char* argv[]) {
  client::RegisterPathProvider();

  QApplication qapp(argc, argv);
  base::ThreadTaskRunnerHandle message_loop{new MessageLoopQt};
  State state;

  DisplayTesterWindow tester_window{
      state, std::bind_front(&CreateModusViewFromPath, std::ref(state))};

  WindowDefinition definition{kModusWindowInfo};

  tester_window.toolbar->addAction("Setup", [&] {
    if (tester_window.opened_view) {
      static_cast<ModusView*>(tester_window.opened_view)->ShowSetupDialog();
    }
  });

  tester_window.toolbar->addAction("Toolbar", [&] {
    if (tester_window.opened_view) {
      static_cast<ModusView*>(tester_window.opened_view)
          ->SetToolbarVisible(
              !static_cast<ModusView*>(tester_window.opened_view)
                   ->IsToolbarVisible());
    }
  });

  tester_window.toolbar->addAction("Save", [&] {
    if (tester_window.opened_view) {
      definition = WindowDefinition{kModusWindowInfo};
      static_cast<ModusView*>(tester_window.opened_view)->Save(definition);
    }
  });

  tester_window.toolbar->addAction("Load", [&] {
    tester_window.AddView(*CreateModusView(state, definition));
  });

  tester_window.show();

  return QApplication::exec();
}