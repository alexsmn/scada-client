# Client build

Notes on how the client link graph is organised and where time is spent
when rebuilding `client_qt_unittests`.

## Targets

| Target | Produces | Purpose |
| --- | --- | --- |
| `client_qt_app_shared` | `.lib` | Shared Qt app object code, linked into both the exe and the tests. |
| `client_qt` | `client.exe` | The desktop client. |
| `client_qt_unittests` | `client_qt_unittests.exe` | Integration/UT suite that exercises `ClientApplication` end-to-end. |
| `client_<module>_qt_unittests` | `client_<module>_qt_unittests.exe` | Auto-generated per `client_module()` target for module-local unit tests. |

`client_qt_unittests` is intentionally monolithic because
`client_application_unittest.cpp` exercises the full `ClientApplication`
bootstrap — it has to link the same ~130 libraries as the desktop client
itself. Smaller, module-scoped tests belong in their module's
`*_qt_unittests` target, not here.

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

Do **not** split `client_qt_unittests` across smaller executables as a
flake workaround; the per-test link cost would dominate. The structural
fix is to keep adding new test coverage to the per-module
`client_<module>_qt_unittests` binaries, so that `client_qt_unittests`
stays scoped to the handful of tests that genuinely need the whole
application wired up.
