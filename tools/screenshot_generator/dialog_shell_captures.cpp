// Dialog captures that need no address-space node: the login window, the
// command palette, and the small shared modals (about, period picker, message
// box, CSV options, add-to-favourites, transport settings).
//
// Note which factories are coroutines. `ShowAboutDialog` returns void and shows
// eagerly, so GrabShownDialog suffices; everything returning `Awaitable<T>` has
// to go through StartAwaitable, because a coroutine nobody awaits never runs
// its body and the dialog is simply never shown — a miss that surfaces only as
// "No visible dialog for kind: …".

#include "dialog_kinds.h"

#include "null_task_manager.h"
#include "screenshot_config.h"

#include "aui/qt/dialog_service_impl_qt.h"
#include "aui/translation.h"
#include "base/memory_settings_store.h"
#include "base/relative_time_range.h"
#include "base/utf_convert.h"
#include "controller/command_manager.h"
#include "main_window/command_palette_qt.h"
#include "modules/about/about_dialog.h"
#include "modules/export/csv/csv_export.h"
#include "modules/favorites/add_favourites_dialog.h"
#include "modules/login/login_dialog.h"
#include "modules/time_range/time_range_dialog.h"
#include "profile/profile.h"
#include "profile/window_definition.h"
#include "properties/transport/transport_dialog.h"
#include "scada/data_services_factory.h"

#include <QApplication>
#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <utility>

namespace scada::screenshot_generator {
namespace {

// Registers a representative spread of operator/engineering commands so the
// palette capture shows a realistic list. Titles go through Translate() (no
// Cyrillic literals in source); the generator loads no .ts, so they render in
// English.
void RegisterSampleCommands(CommandManager& manager) {
  const char* const titles[] = {
      "Acknowledge Alarm", "Acknowledge All Alarms",
      "Open Display",      "Export to CSV",
      "Write Value",       "Show Event Journal",
      "Add to Favorites",  "Print Preview",
      "Device Metrics",    "Refresh",
  };
  unsigned id = 1;
  for (const char* title : titles) {
    manager.RegisterCommand(
        CommandDescriptor{.command_id = id++, .title = Translate(title)});
  }
}

}  // namespace

bool CaptureLoginDialog(const DialogCaptureContext& context) {
  DialogEnvironment& env = context.env;
  DataServicesContext services_context{context.logger, env.executor,
                                       context.transport_factory,
                                       scada::ServiceLogParams{}};
  // Hermetic settings: the production dialog reads the saved user list and
  // server addresses from the registry / per-user settings file, so on a used
  // dev box the capture would leak the real server address. Seed an in-memory
  // store with the state the docs image shows instead.
  //
  // The accounts come from the fixture's `login_user_list` and are joined the
  // way LoginController stores them — comma-separated, since it reads the
  // setting back through its own ParseListString.
  auto settings_store = std::make_shared<MemorySettingsStore>();
  std::u16string user_list;
  for (const auto& user : env.login_user_list) {
    if (!user_list.empty())
      user_list += u',';
    user_list += UtfConvert<char16_t>(user);
  }
  settings_store->SetString16(
      "User", env.login_user_list.empty()
                  ? std::u16string{}
                  : UtfConvert<char16_t>(env.login_user_list.front()));
  settings_store->SetString16("UserList", user_list);
  settings_store->SetString("Host", "127.0.0.1");

  auto lifetime = StartAwaitable(
      env.executor,
      ExecuteLoginDialog(env.executor, std::move(services_context),
                         std::move(settings_store)));
  QApplication::processEvents();
  return GrabThenAwait(context, lifetime);
}

bool CaptureCommandPaletteDialog(const DialogCaptureContext& context) {
  // The palette is a plain modal QDialog (not a CoSpawn'd coroutine like the
  // others): build it over a fixture command list and let the generic grab
  // pick it up. The manager can die once the ctor has copied its entries.
  CommandManager command_manager;
  RegisterSampleCommands(command_manager);
  auto* palette =
      new CommandPalette(nullptr, command_manager,
                         [](unsigned) -> CommandHandler* { return nullptr; });
  palette->setAttribute(Qt::WA_DeleteOnClose);
  palette->show();
  return GrabShownDialog(context);
}

bool CaptureAboutDialog(const DialogCaptureContext& context) {
  // Eagerly-shown self-owned modal; the generic grab rejects it and the dialog
  // deleteLater's itself.
  ShowAboutDialog(context.dialog_service);
  return GrabShownDialog(context);
}

bool CaptureTimeRangeDialog(const DialogCaptureContext& context) {
  // The journal/graph period picker over its default (interval) range; the
  // date edits render the frozen fixture clock, so output is deterministic.
  if (!context.env.profile) {
    ADD_FAILURE() << "TimeRangeDialog needs a profile";
    return false;
  }
  auto lifetime = StartAwaitable(
      context.env.executor,
      ShowTimeRangeDialog(
          context.dialog_service,
          TimeRangeContext{.profile_ = *context.env.profile,
                           .time_range_ = scada::RelativeTimeRange{}}));
  return GrabThenAwait(context, lifetime);
}

bool CaptureMessageBoxDialog(const DialogCaptureContext& context) {
  // The shared confirmation surface (DialogService::RunMessageBox), in its
  // question variant — the shape the two-stage confirms and apply/discard
  // prompts use. Mirrors the Excel-import "Apply changes?" call.
  auto lifetime = StartAwaitable(
      context.env.executor,
      context.dialog_service.RunMessageBox(Translate("Apply changes?"),
                                           Translate("Import"),
                                           MessageBoxMode::QuestionYesNo));
  return GrabThenAwait(context, lifetime);
}

bool CaptureCsvExportDialog(const DialogCaptureContext& context) {
  // The CSV options the export command asks for before writing the file:
  // separator, encoding, and whether to expand grouped rows. `can_expand` is
  // true so the expand option is IN the picture — the dialog hides it when the
  // data has no groups rather than offering it with no effect, and a capture of
  // the hidden state would document the smaller dialog.
  //
  // The web side of this row deliberately has no counterpart: it exports
  // straight from each view through the browser's own save picker, which is
  // recorded as `web.no_surface` in the parity matrix.
  if (!context.env.profile) {
    ADD_FAILURE() << "CsvExportDialog needs a profile";
    return false;
  }
  auto lifetime = StartAwaitable(
      context.env.executor,
      ShowCsvExportDialog(context.dialog_service, *context.env.profile,
                          /*can_expand=*/true));
  return GrabThenAwait(context, lifetime);
}

bool CaptureAddFavouritesDialog(const DialogCaptureContext& context) {
  // «Add to favourites»: names the entry and picks the folder it lands in. The
  // folder combo fills from the seeded store, which is why this takes the app's
  // Favourites rather than a fresh one — the fixture's two folders are what
  // make the combo worth photographing.
  if (!context.env.favourites) {
    ADD_FAILURE() << "AddFavouritesDialog needs a favourites store";
    return false;
  }
  WindowDefinition window_def{"Graph"};
  window_def.title = Translate("Voltages");
  auto lifetime = StartAwaitable(
      context.env.executor,
      ShowAddFavouritesDialog(
          context.dialog_service,
          AddFavouritesContext{*context.env.favourites,
                               std::move(window_def)}));
  return GrabThenAwait(context, lifetime);
}

bool CaptureTransportDialog(const DialogCaptureContext& context) {
  // The transport-settings dialog, which edits the connection string a device
  // or link uses. Seeded with a TCP endpoint rather than an empty string so the
  // host and port fields are populated and the protocol combo shows a resolved
  // selection.
  // Named rather than a temporary: ShowTransportDialog takes its
  // TransportString by const reference AND is a coroutine, and a coroutine does
  // not copy reference parameters into its frame — so a temporary is destroyed
  // before the lazy body ever reads it. GrabThenAwait waits for the coroutine
  // to unwind, so this local outlives it. Same hazard ShowAddFavouritesDialog
  // documents on its own by-value context parameter.
  const transport::TransportString endpoint{"tcp://192.168.1.50:2404"};
  auto lifetime = StartAwaitable(
      context.env.executor,
      ShowTransportDialog(context.dialog_service, endpoint));
  return GrabThenAwait(context, lifetime);
}

}  // namespace scada::screenshot_generator
