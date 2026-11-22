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
#include "screenshot_wait.h"

#include "aui/qt/dialog_service_impl_qt.h"
#include "aui/translation.h"
#include "base/memory_settings_store.h"
#include "base/relative_time_range.h"
#include "base/utf_convert.h"
#include "main_window/command_palette_qt.h"
#include "main_window/context_bar_qt.h"
#include "modules/about/about_dialog.h"
#include "modules/about/about_info.h"
#include "modules/export/csv/csv_export.h"
#include "modules/favorites/add_favourites_dialog.h"
#include "modules/login/login_dialog.h"
#include "modules/time_range/time_range_dialog.h"
#include "profile/profile.h"
#include "profile/window_definition.h"
#include "properties/transport/transport_dialog.h"
#include "scada/data_services_factory.h"

#include <QApplication>
#include <QListWidget>
#include <QShortcut>
#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <utility>

namespace scada::screenshot_generator {

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
  // The Server is a hosted service reached over encrypted OPC UA, and the
  // manual's sign-in steps say so: type «OPC UA», the address Telecontrol
  // gives (shown with the documentation domain), «Подпись и шифрование». A
  // non-zero security mode also opens the «Защита и сертификаты» disclosure.
  settings_store->SetString("ServerType", "OpcUa");
  settings_store->SetString("Host:OpcUa", "scada.example.com");
  settings_store->SetString("SecurityMode", "SignAndEncrypt");

  auto lifetime = StartAwaitable(
      env.executor,
      ExecuteLoginDialog(env.executor, std::move(services_context),
                         std::move(settings_store)));
  QApplication::processEvents();
  return GrabThenAwait(context, lifetime);
}

bool CaptureCommandPaletteDialog(const DialogCaptureContext& context) {
  // Opened the way an operator opens it: the main window's Ctrl-K shortcut,
  // which shows the palette over every command the window has registered plus
  // the address-space tags. Until 2026-11-22 the capture built the palette over
  // ten invented English titles that no catalog translates, so the Russian
  // manual showed «Acknowledge Alarm», «Refresh»… — a list the client never
  // displays. The palette is a self-deleting modal QDialog, so the generic grab
  // picks it up and closes it.
  QWidget* main_window = context.env.main_window;
  if (!main_window) {
    ADD_FAILURE() << "CommandPalette needs the main window";
    return false;
  }
  QShortcut* palette_shortcut = nullptr;
  for (QShortcut* shortcut : main_window->findChildren<QShortcut*>()) {
    if (shortcut->key() == ContextBar::PaletteKey()) {
      palette_shortcut = shortcut;
      break;
    }
  }
  if (!palette_shortcut) {
    ADD_FAILURE() << "the main window has no "
                  << ContextBar::PaletteKey().toString().toStdString()
                  << " shortcut to open the command palette with";
    return false;
  }
  // The unfiltered list opens on whatever sorts first (aggregation intervals,
  // «1-Минутные»…), which says nothing about what the palette is for. A query
  // shows what the manual describes: commands and signals matched as you type.
  //
  // The signals come from an address-space browse the first opening starts,
  // and a palette copies its entries when it is built — so an early palette
  // has no signals. Reopen until the query matches a signal, as an operator
  // pressing Ctrl-K again a moment later would. A signal is told from a
  // command by its entry id, which the palette offsets past every command id
  // (kExtraItemBase in command_palette_qt.cpp).
  constexpr unsigned kFirstNonCommandId = 1u << 30;
  constexpr int kMaxOpenings = 50;
  for (int opening = 0; opening < kMaxOpenings; ++opening) {
    emit palette_shortcut->activated();
    if (context.spec.filter.empty())
      break;
    auto* palette = main_window->findChild<CommandPalette*>();
    if (!palette) {
      ADD_FAILURE() << "the Ctrl-K shortcut opened no command palette";
      return false;
    }
    palette->PresetFilter(QString::fromStdString(context.spec.filter));
    bool matches_a_signal = false;
    if (auto* list = palette->findChild<QListWidget*>()) {
      for (int row = 0; row < list->count(); ++row) {
        if (list->item(row)->data(Qt::UserRole).toUInt() >=
            kFirstNonCommandId) {
          matches_a_signal = true;
        }
      }
    }
    if (matches_a_signal)
      break;
    if (opening + 1 == kMaxOpenings) {
      ADD_FAILURE() << "the command palette matched no signal for «"
                    << context.spec.filter << "»";
      return false;
    }
    // Deleted now rather than deferred, so the next findChild cannot return
    // this one.
    delete palette;
    PumpEventLoopFor(std::chrono::milliseconds{100});
  }
  return GrabShownDialog(context);
}

bool CaptureAboutDialog(const DialogCaptureContext& context) {
  // A FIXED AboutInfo, never `CollectAboutInfo`. Every field the live
  // collector reads varies with the machine or the commit — the build stamp
  // moves on every landing, `QSysInfo::prettyProductName()` differs between
  // the two hosts that may render this gallery, and the session rows depend on
  // a connected server. Reading them here would make this capture differ on
  // every regeneration, which is exactly the signal the tracked gallery
  // exists to carry (client/CLAUDE.md, "Doc screenshots").
  //
  // The values are the ones the shared screen draws for the signed-in state:
  // docs/product/ui-mockups/screens/about.html.
  //
  // Eagerly-shown self-owned modal; the generic grab rejects it and the dialog
  // deleteLater's itself.
  ShowAboutDialog(context.dialog_service,
                  AboutInfo{.product = u"Telecontrol SCADA",
                            .version = u"2.6.0",
                            .build_commit = u"9f3c21a",
                            .build_date = u"2026-09-18T10:00:00+03:00",
                            .runtime = u"Qt 6.8.2 · macOS 15.6 · arm64",
                            .server = u"opc.tcp://sub-north.tc.local:4840",
                            .user = u"operator1",
                            .has_session = true,
                            .connected = true});
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
  auto lifetime =
      StartAwaitable(context.env.executor,
                     context.dialog_service.RunMessageBox(
                         Translate("Apply changes?"), Translate("Import"),
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
      ShowAddFavouritesDialog(context.dialog_service,
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
  const transport::TransportString endpoint{
      "TCP;Active;Host=192.168.1.50;Port=2404"};
  auto lifetime =
      StartAwaitable(context.env.executor,
                     ShowTransportDialog(context.dialog_service, endpoint));
  return GrabThenAwait(context, lifetime);
}

}  // namespace scada::screenshot_generator
