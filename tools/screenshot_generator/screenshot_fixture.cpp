#include "screenshot_fixture.h"

#include "aui/qt/theme_qt.h"
#include "aui/translation.h"
#include "base/ui_text.h"
#include "base/utf_convert.h"
#include "favorites/favourites.h"
#include "main_window/main_window.h"
#include "main_window/main_window_manager.h"
#include "model/node_id_util.h"
#include "model/security_node_ids.h"
#include "modules/events/local_events.h"
#include "profile/page.h"
#include "profile/window_definition.h"
#include "scada/event.h"
#include "scada/node_id.h"
#include "screenshot_wait.h"

#include <QApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QMainWindow>
#include <QMenuBar>
#include <QStyleFactory>

#include <boost/json.hpp>

#include <chrono>

namespace scada::screenshot_generator {

ScreenshotConfig& FixtureConfig() {
  // The fixture outlives every test and is deliberately never destroyed: it is
  // loaded once in SetUpTestSuite and read from static-storage teardown paths.
  static scada::base::NoDestructor<ScreenshotConfig> config;
  return *config;
}

void SeedDeviceLog(const boost::json::value& root,
                   scada::LocalMonitoredItemService& monitored_item_service) {
  const auto* block = root.as_object().if_contains("device_log");
  if (!block)
    return;

  const scada::NodeId device_id =
      NodeIdFromScadaString(std::string_view(block->at("device").as_string()));
  const scada::Time now = scada::Now();

  for (const auto& jl : block->at("lines").as_array()) {
    scada::Event event;
    event.event_id = 1;
    const double seconds_ago = jl.at("seconds_ago").to_number<double>();
    event.time = now - std::chrono::round<std::chrono::microseconds>(
                           std::chrono::duration<double>{seconds_ago});
    event.receive_time = event.time;
    event.source_node_id = device_id;
    event.event_type_id = scada::devices::id::DeviceWatchEventType;
    event.message = scada::LocalizedText{
        UtfConvert<char16_t>(std::string(jl.at("message").as_string()))};
    event.severity = scada::kSeverityMin;
    if (const auto* js = jl.as_object().if_contains("severity")) {
      const std::string_view severity = js->as_string();
      if (severity == "warning")
        event.severity = scada::kSeverityWarning;
      else if (severity == "error")
        event.severity = scada::kSeverityCritical;
    }

    const auto* jf = jl.as_object().if_contains("frame");
    if (!jf) {
      monitored_item_service.AddEvent(device_id, std::any{std::move(event)});
      continue;
    }

    event.event_type_id = scada::devices::id::DeviceFrameEventType;

    scada::DeviceFrame frame;
    const std::string_view direction = jf->at("direction").as_string();
    frame.direction = direction == "tx" ? scada::DeviceFrame::kOutbound
                                        : scada::DeviceFrame::kInbound;
    const auto read = [jf](std::string_view name) -> scada::Int32 {
      const auto* value = jf->as_object().if_contains(name);
      return value ? static_cast<scada::Int32>(value->to_number<std::int64_t>())
                   : 0;
    };
    if (const auto* jfmt = jf->as_object().if_contains("format"))
      frame.format = std::string(jfmt->as_string());
    frame.type_id = read("type_id");
    frame.cause = read("cause");
    frame.object_address = read("object_address");
    frame.send_sequence = read("send_sequence");
    frame.receive_sequence = read("receive_sequence");

    monitored_item_service.AddEvent(
        device_id, std::any{scada::DeviceFrameEvent{
                       .base = std::move(event), .frame = std::move(frame)}});
  }
}

// Seeds the Favorites pane from the fixture's `favourites` block. The pane
// renders whatever the profile holds, and the generator starts from an empty
// one — so without this the capture is a blank panel, which is how it shipped
// for months.
void SeedFavourites(const boost::json::value& root, Favourites& favourites) {
  const auto* block = root.as_object().if_contains("favourites");
  if (!block)
    return;

  for (const auto& jf : block->at("folders").as_array()) {
    const std::u16string name =
        UtfConvert<char16_t>(std::string(jf.at("name").as_string()));
    const Page& folder = favourites.GetOrAddFolder(name);

    for (const auto& jw : jf.at("windows").as_array()) {
      WindowDefinition window{std::string(jw.at("type").as_string())};
      window.title =
          UtfConvert<char16_t>(std::string(jw.at("title").as_string()));
      favourites.Add(window, folder);
    }
  }
}

// Reports the fixture's client-side events, which is the only way a
// `LOCAL_EVENT` row reaches the journal: nothing in an offline fixture loses a
// server connection, so the «Local Event» object the manual's event-journal
// pages show never appeared in a generated capture. Absent key: nothing
// reported, same as before.
void SeedLocalEvents(const boost::json::value& root,
                     LocalEvents& local_events) {
  const auto* block = root.as_object().if_contains("local_events");
  if (!block)
    return;

  for (const auto& je : block->as_array()) {
    const std::string_view severity{je.at("severity").as_string()};
    const LocalEvents::Severity level =
        severity == "error"     ? LocalEvents::SEV_ERROR
        : severity == "warning" ? LocalEvents::SEV_WARNING
                                : LocalEvents::SEV_INFO;
    local_events.ReportEvent(level,
                             scada::LocalizedText{UtfConvert<char16_t>(
                                 std::string(je.at("message").as_string()))});
  }
}

QMainWindow* ShowMainWindowForMenuCapture(ClientApplication& app) {
  const auto& main_windows = app.main_window_manager().main_windows();
  if (main_windows.size() != 1u) {
    ADD_FAILURE() << "expected exactly one main window, got "
                  << main_windows.size();
    return nullptr;
  }
  auto* qmain = dynamic_cast<QMainWindow*>(&main_windows.front());
  if (!qmain) {
    ADD_FAILURE() << "the main window is not a QMainWindow";
    return nullptr;
  }
  qmain->resize(1920, 1080);
  qmain->show();
  for (int i = 0; i < 20; ++i)
    QApplication::processEvents();
  return qmain;
}

std::vector<std::u16string> FixtureAccountsWith(scada::AccessRight right) {
  namespace sec = scada::security::id;
  const std::string rights_key =
      NodeIdToScadaString(sec::UserType_AccessRights);

  std::vector<std::u16string> names;
  for (const auto& node : FixtureConfig().json.at("nodes").as_array()) {
    const auto& object = node.as_object();
    const auto* type = object.if_contains("type_definition");
    if (!type || !type->is_string() ||
        NodeIdFromScadaString(std::string_view{type->as_string()}) !=
            sec::UserType) {
      continue;
    }
    const auto* display = object.if_contains("display_name");
    if (!display || !display->is_string())
      continue;

    std::int64_t access_rights = 0;
    if (const auto* properties = object.if_contains("properties");
        properties && properties->is_object()) {
      if (const auto* value = properties->as_object().if_contains(rights_key);
          value && value->is_number()) {
        access_rights = value->to_number<std::int64_t>();
      }
    }
    if (access_rights & scada::AccessRightBit(right))
      names.push_back(UtfConvert<char16_t>(std::string{display->as_string()}));
  }
  return names;
}

ScreenshotGenerator::ScreenshotGenerator() {
  // Install the Russian translator directly. We intentionally bypass
  // InstalledTranslation here: it reads the locale from a QSettings
  // instance that lacks an organization/app name (AppEnvironment builds
  // a bare QApplication), so the value round-trips through an
  // unreliable backing store and the Russian .qm ends up not loading
  // at all.
  //
  // The install must happen per-fixture: each TEST_F spins up a fresh
  // QApplication via AppEnvironment, and QTranslator registrations
  // don't survive across QApplication instances.
  QLocale::setDefault(QLocale{QLocale::Russian, QLocale::Russia});

  // Record that choice where the client reads it back. `GetSelectedLocaleName`
  // (main_window/main_window_module.cpp) falls back to `QLocale::system()` when
  // the setting is absent, and the hermetic settings tree above is empty on
  // every run — so the Language row rendered whatever locale the CAPTURING
  // MACHINE happened to use while the labels beside it were always Russian.
  // That is how settings-dialog.png shipped reading "Язык: English". Pinning it
  // makes the rendered locale a property of the fixture rather than of the
  // host, which is the same reason the style below is pinned to Fusion.
  QSettings{}.setValue("LocaleName", "ru_RU");

  const auto translation_dir =
      QApplication::applicationDirPath() + "/translations";
  // Qt's own catalog first (standard Yes/No/Cancel buttons, spin/date
  // chrome), mirroring InstalledTranslation: prefer Qt's installed
  // translations dir, fall back to the staged app dir. Installed before the
  // client catalog so the client strings win on conflicts.
  // A missing Qt base catalog is a hard failure for the same reason a missing
  // client catalog is: it silently corrupts every published image. This used
  // to be a bare `if (load)`, which is how `message-box.png` could render
  // «Применить изменения?» over buttons reading Yes/No and still pass both
  // screenshot checks. The cause was that `qttranslations` appeared in no
  // product manifest at all, so the port was never installed on any platform
  // and `app/qt/CMakeLists.txt`'s staging lookup missed — unlike the
  // `client_ru` outage below, which was Windows-gated, this one shipped mixed
  // -language chrome through the Windows publish channel too.
  if (qtbase_translator_.load(
          "qtbase_ru", QLibraryInfo::path(QLibraryInfo::TranslationsPath)) ||
      qtbase_translator_.load("qtbase_ru", translation_dir)) {
    QApplication::installTranslator(&qtbase_translator_);
  } else {
    ADD_FAILURE()
        << "qtbase_ru.qm not found in "
        << QLibraryInfo::path(QLibraryInfo::TranslationsPath).toStdString()
        << " or " << translation_dir.toStdString()
        << "; standard Qt chrome (QMessageBox buttons, spin/date "
           "widgets) would render English inside otherwise-Russian "
           "captures. The vcpkg `qttranslations` port is most likely "
           "missing from this build — see "
           "docs/ops/client-screenshots.md.";
  }
  // A missing client catalog is a hard failure, not a silent fall-through to
  // English. This used to be a bare `if (load)`, which is how a macOS build
  // could render every capture in English chrome and still pass
  // `client_screenshot_check` — it asserts existence and dimensions, never
  // text, so nothing in the suite could see it. The cause was a `"platform":
  // "windows"` gate on `qttools` in `client/vcpkg.json`, so no non-Windows
  // build had `lrelease` to produce `client_ru.qm` at all;
  // `client_qt_copy_translations` degraded to an echo. The generator depends on
  // that target, so by the time this runs the catalog must be staged — if it is
  // not, the toolchain is broken in a way that silently corrupts every
  // published image.
  if (translator_.load("client_ru", translation_dir)) {
    QApplication::installTranslator(&translator_);
  } else {
    ADD_FAILURE() << "client_ru.qm not found in "
                  << translation_dir.toStdString()
                  << "; captures would render English chrome. The Qt "
                     "LinguistTools (vcpkg `qttools`) are most likely missing "
                     "from this build — see docs/ops/client-screenshots.md.";
  }

  // Route shared code's operator-facing text through those catalogs. Status
  // descriptions, data-quality flags and boolean value labels are produced
  // below `common/` (core/scada/{status,qualifier,variant}.cpp) and reach the
  // UI through `scada::TranslateUiText`, which returns its English argument
  // verbatim until a translator is installed. The client installs one in
  // `AppInit` — but the generator is a gtest binary and has no `main()` of its
  // own, so `AppInit` never runs here and every such string rendered English
  // no matter what the catalog said. Third instance of this defect shape in
  // this constructor, and the worst-behaved: the two above at least fail
  // loudly now, while this one had no signal at all until
  // `TranslatedUiTextResolvesToRussian` (task 376).
  scada::SetUiTextTranslator(&Translate);

  // Pin Fusion for captures. Unlike the client — which runs the platform style
  // so it looks native (docs/client/ux/principles.md §9) — published
  // screenshots must be byte-comparable across machines, so they deliberately
  // do NOT follow the host style. Set directly rather than through
  // InstalledStyle for the QSettings reason: that also writes back the live
  // style's objectName on destruction, which would let the second TEST_F pick
  // up whatever QStyleFactory returned instead of "Fusion".
  //
  // Consequence to keep in mind while the native migration runs
  // (docs/client/ux/backlog.md P6): these captures show the Fusion rendering,
  // not what an operator sees. Validate native-look changes by running the
  // real client, not only by diffing generated PNGs.
  QApplication::setStyle("Fusion");

  // Render under the design-token appearance `--theme` names (dark by
  // default), applied over the Fusion base exactly as the client applies it
  // over the platform style (see app/qt/installed_appearance.h). ApplyTheme
  // settles the severity/quality ramp to match; this used to repeat that
  // mapping by hand.
  //
  // Resolve `system` here: a capture must pin one concrete appearance, never
  // follow the machine that happens to render it. Resolving also stops
  // ApplyTheme installing the system-following watcher, which would let the
  // host desktop change a capture mid-run.
  scada::aui::ApplyTheme(scada::aui::ResolveTheme(scada::aui::ThemeFromString(
      QString::fromStdString(GetScreenshotOptions().theme),
      scada::aui::Theme::kDark)));

  // Render offscreen. `widget->grab()` renders the Qt widget tree to a
  // QPixmap without needing the window to be on-screen — so this
  // suite works headless (incl. CI without a desktop session).
  // Trade-off: we don't get the native OS title bar / frame; we tried
  // PrintWindow(PW_RENDERFULLCONTENT) for that and it didn't reliably
  // capture child-widget content (see commit history).
  MainWindow::SetHideForTesting();

  // Populate the address space with ns=1 instance nodes; everything
  // else (standard OPC UA / SCADA tree) was already built in code by
  // ScadaTestAddressSpace. The real `v1::NodeServiceImpl`
  // inside ClientApplication browses and reads them through
  // ViewServiceImpl + AttributeServiceImpl on demand.
  PopulateFixtureNodes(address_space_, FixtureConfig().json);
  // The Users grid reads the standard model; project the fixture's accounts
  // onto it so the capture is not an empty grid.
  ProjectFixtureUsersOntoStandardModel(address_space_, FixtureConfig().json);

  // Seed history only for nodes that made it into the address space. The
  // fixture's `nodes` array can declare a node its `tree` never parents, and
  // PopulateFixtureNodes cannot create such an orphan — without this gate the
  // history service would still synthesize a series for it, and a table row
  // bound to that node would paint a convincing sparkline next to a "no data"
  // quality mark.
  history_service_.LoadFromJson(
      FixtureConfig().json, [this](const scada::NodeId& node_id) {
        return address_space_.GetNode(node_id) != nullptr;
      });

  SeedDeviceLog(FixtureConfig().json, monitored_item_service_);

  // Sign the session in as a fixture account. Must follow PopulateFixtureNodes:
  // the status strip resolves this id against the address space to read a
  // display name, and an id naming a node that does not exist yet renders the
  // same empty cell as the null id it replaces.
  session_service_.SetUserId(FixtureConfig().session_user_node_id);
}

ScreenshotGenerator::~ScreenshotGenerator() {
  WaitForAwaitable(executor_, app_.Quit());
}

}  // namespace scada::screenshot_generator
