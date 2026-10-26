#pragma once

#include "base/any_executor.h"

#include "base/awaitable.h"
#include "scada/data_services.h"

#include <memory>
#include <optional>

struct DataServices;
struct DataServicesContext;
class SettingsStore;

// Coroutine: `services_context` is taken by value (CP.53) — the dialog is
// created on first resume, which happens after the caller's frame may have
// died, so a reference parameter would dangle.
//
// `settings_store` overrides where the dialog reads/writes the saved user
// list and server addresses. Null means the production default (registry /
// per-user settings file, or the E2E `--test-settings-file` override).
// Hermetic harnesses — the screenshot generator — pass an in-memory store so
// captures never leak machine state.
//
// `allow_auto_login` is true only for the sign-in at application start: that
// is the one the stored "Sign in automatically" setting submits by itself. A
// Connect after a Disconnect passes false, so it never signs the previous
// operator back in (backlog 934).
Awaitable<std::optional<DataServices>> ExecuteLoginDialog(
    AnyExecutor executor,
    DataServicesContext services_context,
    std::shared_ptr<SettingsStore> settings_store = {},
    bool allow_auto_login = false);
