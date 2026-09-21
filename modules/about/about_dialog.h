#pragma once

struct AboutInfo;

class DialogService;
class NodeService;
template <class T>
class BasicCommandRegistry;
class UiCommandRegistry;
struct GlobalCommandContext;

namespace scada {
class SessionService;
}

// Reads everything the About dialog states out of the running client. Qt-side,
// because most of it is Qt's to answer: the application's display name, the Qt
// version actually loaded, the OS and the architecture.
//
// The session half comes from `session_service` and is allowed to be absent —
// the Help menu is reachable from the login window, where there is no session
// at all, and `AboutInfo::has_session` is how the dialog knows to draw no
// session rows rather than empty ones.
AboutInfo CollectAboutInfo(scada::SessionService& session_service,
                           NodeService& node_service);

// Shows the dialog against an explicit `info`. This overload is the seam the
// screenshot generator uses: a capture has to be byte-identical between runs,
// and a dialog that read the live build stamp and the host's OS version would
// change on every machine and every commit.
void ShowAboutDialog(DialogService& dialog_service, const AboutInfo& info);

void RegisterAboutCommands(
    BasicCommandRegistry<GlobalCommandContext>& global_commands,
    UiCommandRegistry& ui_command_registry,
    scada::SessionService& session_service,
    NodeService& node_service);
