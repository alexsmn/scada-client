# Client Coroutine Migration Plan

## Summary

The client currently uses `promise<T>`-based async control flow across startup,
service calls, file I/O, and UI-triggered background work. The migration should
move client internals to coroutine-first implementations while preserving the
existing UI-facing threading guarantees and avoiding a repo-wide API break in
the first phase.

The recommended approach is:

1. Migrate client implementations and workflows to coroutine-native internals,
   and switch client async code to the coroutine versions of the core SCADA
   services.
2. Migrate internal client workflows with the highest callback/promise nesting.
3. Keep public service contracts and module wiring stable with temporary
   adapters at legacy boundaries until the migrated paths are proven in tests.

## Current Constraints

- UI work must stay affinity-safe for both Qt and Wt frontends.
- Most client async flows still assume `promise<T>` and
  `BindPromiseExecutor`-style continuation pinning.
- Client tests and screenshot tooling rely on deterministic local services and
  must keep working without a real server.
- Some client test targets are already memory-heavy to compile, so migration
  should avoid template-heavy abstractions unless they remove equivalent
  promise/callback boilerplate.

## Goals

- Reduce nested `.then()` chains and callback wrapping in client workflows.
- Standardize coroutine usage at client/core boundaries without turning
  per-class adapters into a permanent architecture.
- Preserve executor/thread affinity for UI-facing continuations.
- Keep startup, shutdown, cancellation, and task-progress behavior unchanged
  from the user's perspective.
- Keep the first migration phase compatible with existing tests and local mock
  services.

## Non-Goals

- A full repo-wide replacement of `promise<T>` in one pass.
- Rewriting all services or all UI modules at once.
- Changing the frontend split between Qt and Wt.
- Changing wire protocols, authentication flow, or local-service semantics.

## Phase 1: Shared Adapter Layer

Add coroutine support without breaking current public client interfaces.

### Work

- Reuse the shared adapter utilities introduced in `core/base` and
  `core/scada`:
  - promise-to-awaitable bridging
  - callback-to-awaitable bridging
  - named coroutine service adapters in
    `core/scada/coroutine_services.h`
- Define the default client-side rule for core SCADA service calls:
  - coroutine code should depend on `Coroutine*Service` interfaces or their
    adapters
  - do not add new client coroutine code that calls callback-style
    `AttributeService`, `MethodService`, `HistoryService`, `ViewService`, or
    `NodeManagementService` directly
  - do not add new client coroutine code that awaits raw `SessionService`
    promises directly when `PromiseToCoroutineSessionServiceAdapter` can be
    used instead
- Prefer these boundary conversions:
  - callback core service -> `CallbackToCoroutine*ServiceAdapter`
  - promise core service -> `PromiseToCoroutineSessionServiceAdapter`
  - coroutine implementation exposed back to legacy code ->
    `CoroutineToCallback*ServiceAdapter` or
    `CoroutineToPromiseSessionServiceAdapter`
- Prefer migration over adapter proliferation:
  - when touching a client class for coroutine work, first prefer converting
    that class's internal control flow to coroutines
  - add adapters only at legacy boundaries that cannot be migrated in the same
    slice
  - treat those adapters as temporary compatibility shims, not new long-term
    layering for every class
- Add client-focused guidance for executor usage:
  - UI-bound coroutine continuations must resume on the original executor
  - background work may hop to worker executors only through explicit adapters
- Standardize one cancellation rule:
  - module/application lifetime owns coroutine lifetime
  - closing the app, page, or session cancels outstanding work before teardown

### Acceptance Criteria

- New coroutine code uses the coroutine versions of core SCADA services rather
  than inline callback wrapping or ad hoc promise bridging.
- Client modules can await core SCADA service calls through
  `Coroutine*Service` interfaces or the named adapters from
  `core/scada/coroutine_services.h`.
- Existing `promise<T>`-returning client entry points can be implemented via
  coroutine internals.
- No observable UI thread-affinity regressions.

## Phase 2: Startup And Session Flow

Migrate the startup path first because it is central, heavily async, and easy
to regression-test.

### Target Areas

- `client/application` startup sequence
- login/session establishment
- initial model population after login
- shutdown and canceled-login flows

### Work

- Rewrite internal `Start()` orchestration as coroutine steps:
  - load profile
  - show/login resolve
  - create session through the coroutine session service adapter
  - initialize modules/models
  - surface startup errors centrally
- Keep outward `promise<void>` or equivalent entry points unchanged in phase 2.
- Replace deeply chained `.then()` trees with `co_await` sequencing.
- When bridging promise-based session lifecycle APIs into coroutine internals,
  ensure reconnect/shutdown flows wait on each completion boundary exactly
  once.
  In particular, do not combine a disconnect promise that already waits for a
  connect-loop shutdown signal with a second explicit wait on that same signal.

### Risks

- Startup currently encodes ordering implicitly in promise chains; coroutine
  rewrites must preserve that order exactly.
- Error propagation must still map to the same user-visible dialogs/messages.
- Session reconnect logic is especially sensitive to duplicate completion
  waits; a coroutine migration can accidentally create self-referential promise
  chains if old loop-completion promises are awaited both indirectly and
  explicitly.

## Phase 3: Task And Progress Infrastructure

The next migration slice should target the places where the client currently
wraps async work to report progress or serialize UI updates.

### Target Areas

- `TaskManager`
- progress hosts / progress dialogs
- command handlers that queue async work
- long-running export/import/file operations

### Work

- Introduce coroutine-friendly task submission helpers that:
  - accept awaitable work
  - report progress consistently
  - preserve current cancellation semantics
- Convert command handlers from promise chains to coroutine bodies.
- Remove repeated callback-to-promise glue from module actions.

### Acceptance Criteria

- Commands triggered from menus, toolbars, and dialogs can be written as
  coroutine functions with a single task-submission path.
- Progress reporting and cancellation remain consistent across Qt and Wt.

## Phase 4: Data/Service Consumers

Once the adapter layer is stable, migrate client modules that frequently call
SCADA services or local service doubles.

### Priority Order

1. `services/`
2. `events/`
3. `filesystem/`
4. `profile/`
5. `main_window/`
6. `configuration/*`
7. `graph/`, `modus/`, and vidicon-specific modules

### Work

- Replace direct callback/promise core service usage with the coroutine
  versions of those services.
- Normalize module wiring so service-heavy client code receives
  `CoroutineAttributeService`, `CoroutineMethodService`,
  `CoroutineHistoryService`, `CoroutineViewService`,
  `CoroutineNodeManagementService`, and coroutine-adapted session access where
  appropriate.
- Keep callback/promise core services only at compatibility boundaries and
  convert them once at module setup time with the shared adapters.
- Convert module-level async helpers to coroutine functions.
- Keep model-update points explicit so UI mutation still happens on the correct
  executor/thread.

## Phase 5: Public API Evaluation

After the internal migration is stable, decide whether the client should keep
`promise<T>` as its public async boundary or expose coroutine-native entry
points.

### Options

- Keep `promise<T>` at public boundaries for compatibility and testing.
- Add dual APIs where internals are coroutine-first but public entry points can
  still return `promise<T>`.
- Convert selected internal-only interfaces fully to awaitables.

### Recommendation

Do not make this decision during the first implementation pass. Treat it as a
follow-up design review after startup/task/service migrations land cleanly.

## Execution Model Rules

- UI mutations must happen on the same executor/thread they happen on today.
- Coroutine suspension points must not hide thread hops.
- Background work should be explicit and rare.
- Each module should own the lifetime of its in-flight coroutine work.
- Destruction paths must cancel work before releasing UI/model state.

## Testing Strategy

### Unit Tests

- Adapter tests:
  - `promise<T>` success/failure propagation
  - callback-to-awaitable success/failure/cancellation
  - executor-affinity preservation
- Startup tests:
  - successful login/start
  - canceled login
  - startup failure propagation
  - clean shutdown during in-flight startup
- Task/progress tests:
  - progress completion
  - cancellation
  - exception propagation

### Integration / UI-Adjacent Tests

- Existing `client_qt_unittests` and `client_wt_unittests`
- module-specific unit tests in `events`, `filesystem`, `profile`,
  `main_window`, `configuration`, and `graph`
- screenshot generator smoke flows that rely on local services and async model
  population

### Regression Focus

- UI thread-affinity bugs
- startup ordering regressions
- missing cancellation on shutdown
- reconnect/session lifecycle deadlocks caused by duplicate waits on the same
  completion signal
- double-completion / dangling-callback behavior during teardown

## Rollout Guidance

- Migrate one vertical slice at a time.
- Avoid mixing service-interface redesign with flow-control migration.
- Prefer migrating classes to coroutine-native internals over introducing new
  permanent adapter classes.
- Use shared adapters at legacy boundaries instead of ad hoc wrapper lambdas.
- Once a client slice moves to coroutines, switch that slice to the coroutine
  versions of core SCADA services instead of continuing to call the legacy
  callback/promise service APIs from coroutine bodies.
- If a legacy public interface must remain in place, keep the adapter thin and
  temporary while the implementation behind it becomes coroutine-first.
- Keep old and new implementations behaviorally identical before broadening
  scope.
- Land tests with each slice before moving to the next module group.

## Recommended First Implementation Slice

1. Shared adapter layer validation in client code.
2. `client_application` startup/login/shutdown coroutine internals.
3. `TaskManager` coroutine submission helpers.
4. One representative service-heavy module such as `events` or `filesystem`.

This sequence gives the best coverage of real client async behavior while
keeping the migration incremental and testable.
