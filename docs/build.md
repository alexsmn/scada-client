# Client build

Notes on how the client link graph is organised and where time is spent
when rebuilding `client_qt_unittests`.

## Targets

| Target | Produces | Purpose |
| --- | --- | --- |
| `client_qt_app_shared` | `.lib` | Shared Qt app object code, linked into both the exe and the heavy integration test. |
| `client_qt` | `client.exe` | The desktop client. |
| `client_qt_unittests` | `client_qt_unittests.exe` | Heavy integration UT suite that exercises `ClientApplication` end-to-end. |
| `client_startup_exception_tests` | `client_startup_exception_tests.exe` | Platform-neutral UT for `startup_exception.cpp` — links only `scada_base` + `base_unittest`. |
| `client_<module>_qt_unittests` | `client_<module>_qt_unittests.exe` | Auto-generated per `client_module()` target for module-local unit tests. |

`client_qt_unittests` is intentionally monolithic *only* for
`client_application_unittest.cpp`, which exercises the full
`ClientApplication` bootstrap and has to link the same ~130 libraries
as the desktop client itself. Any test that doesn't actually need the
full app wired up should land in a separate small target (the shape of
`client_startup_exception_tests`) or the module-local
`client_<module>_qt_unittests` — do not bolt new cases onto
`client_qt_unittests` out of convenience.

### Why `startup_exception` is split out

`startup_exception.cpp` only needs `LoginCanceled` (extracted to
`app/login_canceled.h`) and standard library facilities. Keeping those
six test cases inside `client_qt_unittests` paid the full link cost of
the whole client for unrelated assertions. The standalone exe links
`scada_base` + `base_unittest` only, builds in seconds, and is not
subject to the x86 link-time flakes described below.

### Measured edit-compile-test cycle

Observed on the `ninja-dev` / `release-dev` preset (`RelWithDebInfo`,
x86 MSVC 14.5, ccache warm), all other dependencies already built, after
touching the respective unittest source:

| Target | Incremental build | Run | Tests |
| --- | ---: | ---: | ---: |
| `client_startup_exception_tests` | ~3 s | ~0.05 s | 6 |
| `client_qt_unittests` | ~33 s | ~1.5 s | 11 |

Numbers come from editing the test's own source (one `cl.exe`
invocation plus the link of that target only) and running the resulting
binary without filters. Treat them as order-of-magnitude — the actual
cost moves with parallel compile load on this machine and whether the
`client_qt_app_shared` PCH / obj cache is current.

The takeaway is the ratio, not the absolute seconds: a change that
would rebuild `client_startup_exception_tests` turns a ~30 s cycle into
a ~3 s cycle, and the flaky heavy link is off the critical path for
those six tests entirely.

## Why the shared lib exists

`client_application.cpp`, `client_application_modules.cpp`,
`opcua_services_module.cpp`, and `startup_exception.cpp` are used by
**both** `client_qt` and `client_qt_unittests`. Compiling them directly
into each target — the previous arrangement — made them go through
`cl.exe` twice on every edit of their headers. `client_application.cpp`
alone pulls in a large transitive include set (Qt, Boost, address-space
headers), so each compile is several seconds of x86 `cl.exe` memory
pressure.

Extracting them into the `client_qt_app_shared` static library makes that
cost payable once. The Wt side (`client_wt_lib`) already followed this
pattern; this aligns the Qt side with it.

## What to add (and what not to add) to the test target

**Do** keep `client_qt_unittests` focused on tests that actually need the
full app link. Today that's `client_application_unittest.cpp`; the other
residents (`startup_exception_unittest.cpp`) are small and tagged along
because they live in `client/app/`, which does not define its own module
library.

**Don't** do either of these:

- Add a `list(APPEND UNITTESTS <file>_unittest.cpp)` from a module that
  already has a `client_<module>_qt_unittests` target. The per-module
  target will pick up the test automatically; duplicating it in
  `UNITTESTS` forces a second compile inside the `client_qt_unittests`
  link. `file_cache_unittest.cpp` used to do this and was removed.
- Add a `.cpp` that isn't a test to `UNITTESTS`/`UNITTESTS_QT` so that
  `client_qt_unittests` can see the symbol. Put it in
  `client_qt_app_shared` (or the appropriate module library) instead so
  the compilation is shared with `client_qt`.

## Known x86 link-time flakes

The 32-bit `link.exe` runs close to its address-space limit when linking
`client_qt_unittests` (~130 libs). Sporadic `LNK1102: out of memory`
under parallel load is expected; rerunning the build succeeds. The
mitigations already applied in the CMakeLists for the RelWithDebInfo
config are:

```cmake
target_link_options(client_qt_unittests PRIVATE
  "$<$<CONFIG:RelWithDebInfo>:/DEBUG:NONE>"
  "$<$<CONFIG:RelWithDebInfo>:/INCREMENTAL:NO>"
  "$<$<CONFIG:RelWithDebInfo>:/PDB:NONE>"
)
```

Compile-side `C1060` on `client_application_modules.cpp` under heavy
parallel ninja load is the same class of problem — retrying picks up
free heap and the target compiles.

Do **not** split a test out of `client_qt_unittests` purely as a flake
workaround — if the test body genuinely needs the full app link, it
still has to pay the flake tax wherever it lives. The structural fix is
what `client_startup_exception_tests` demonstrates: when a test doesn't
need the app wired up, take it off the heavy link entirely. Tests that
*do* need the full app belong in `client_qt_unittests`; module-scoped
tests belong in their module's `client_<module>_qt_unittests`.
