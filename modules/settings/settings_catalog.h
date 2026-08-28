#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace scada::aui {
class MenuModel;
}

// The catalogue behind the Settings surface: one entry per preference this
// client publishes, carrying the four things
// `docs/product/ui-mockups/screens/settings.html` draws for every row -- which
// category it belongs to, what it is called, what it does, and where its value
// is stored.
//
// The screen is the north star for both realms (the full inventory is in
// `settings-rows.html`; the web half of it is
// `web/apps/app/src/features/settings/settings-catalog.ts`), and it is a list
// rather than a form for a reason: the table of contents, the search field and
// the scope tabs are all views onto the same catalogue, and none of them can
// exist while the rows are hand-built widgets. Everything here is data so those
// three can be derived.
//
// **The menu model stops being the whole specification, and does not stop being
// the source of truth.** `SettingsDialog` walked the `MainMenuId::Settings`
// model and drew whatever it found, which is why a plain command in it had
// nowhere to go and why no row could carry a description or a store. The
// catalogue supplies what the model cannot -- category, storage scope, the
// sentence under the title, and the fact that a row is an action rather than a
// preference -- and takes everything else from the model: a row exists only
// when the shell actually publishes its command, and its **title is the
// command's own, verbatim**. So no setting can be invented here, and one the
// shell drops loses its row rather than becoming a control over nothing.

// Which heading a setting sits under. The order is the screen's, which is also
// the table of contents' order and the order rows are laid out in, so it is the
// enumerator order too.
//
// `Profile` and `Administration` are the two categories on the shared screen
// that this realm has no row for: the plant name and the configuration snapshot
// are web rows, and Qt reaches the snapshot from its configuration page
// instead. They are absent rather than drawn empty -- a realm that does not
// implement a setting shows no door to it.
enum class SettingCategory {
  kAppearance,
  kEventsAlarms,
  kControl,
  kWorkspace,
  kDisplays,
};

// Where a value lives, which is the question a two-client workbench has to
// answer: will this follow me to the other client, or to another workstation?
//
// The three stores are real and differ. `kClient` is machine-local (`QSettings`
// here, `localStorage` in the web client); `kProfile` is the profile envelope
// on the server, which both clients read; `kWindow` is `MainWindowDef` inside
// that envelope, keyed by main-window id, which is why Toolbar and Status Bar
// are per window rather than per account. `kAction` is not a store -- it marks
// a row that runs something rather than holding a value, and is counted
// separately for that reason.
enum class SettingScope {
  kClient,
  kProfile,
  kWindow,
  kAction,
};

// The shape of the row's control. `kChoice` is a menu submenu of mutually
// exclusive items -- a combo box on the surface, the same translation
// `SettingsDialog` made; `kToggle` is a checkable item; `kAction` is a plain
// command, which the dialog had to skip because a command drawn as a checkbox
// misreports it.
enum class SettingControl {
  kChoice,
  kToggle,
  kAction,
};

// One row of the surface: what the catalogue says about it, joined to the live
// menu item that actually carries its state.
struct SettingRow {
  // Stable handle -- the row widget's `objectName` and the test's handle.
  // Never shown to the operator, so it is not translated and does not move when
  // a title is renamed.
  std::string_view id;
  // The command behind the row, and the catalogue's join key.
  unsigned command_id = 0;
  SettingCategory category = SettingCategory::kAppearance;
  SettingScope scope = SettingScope::kProfile;
  SettingControl control = SettingControl::kToggle;

  // The command's own title, read from the model rather than restated here, so
  // the surface and the menu can only ever agree.
  std::u16string title;
  // The sentence under the title. Every row carries one -- a preference whose
  // effect cannot be stated in a sentence is one the operator cannot make a
  // decision about.
  std::u16string description;

  // The model and index the row's control drives. For `kChoice` this is the
  // submenu *item*; the options are `model->GetSubmenuModelAt(index)`.
  scada::aui::MenuModel* model = nullptr;
  int index = 0;
};

// Categories in screen order.
std::span<const SettingCategory> SettingCategoryOrder();

// The scopes a scope tab can filter by. `kAction` is never one: it is not a
// store, and a tab holding only the action rows would claim it was.
std::span<const SettingScope> SettingStorageScopes();

// The heading for a category, used by the heading and its table-of-contents
// entry alike.
std::u16string SettingCategoryLabel(SettingCategory category);

// The label for a scope, used by the row's chip and by its scope tab.
std::u16string SettingScopeLabel(SettingScope scope);

// Every command id the catalogue describes, in screen order.
//
// This is the surface's contract with the shell, and it is checkable in both
// directions: a command listed here that the Settings menu stops publishing
// loses its row, and one the menu gains without an entry here never gets one --
// see `UndescribedSettingsCommands`.
std::span<const unsigned> SettingsCatalogCommandIds();

// Builds the catalogue from `settings_menu` -- the `MainMenuId::Settings`
// model, which the caller has already had `MenuWillShow` called on.
//
// Screen order, not menu order: the catalogue is what says where a row belongs,
// and the two differ (the menu lists Style before Colour scheme; the screen
// lists Colour scheme before Widget style). A described command the model does
// not publish, or publishes invisibly, contributes no row -- which is how the
// Modus rows disappear from a build without the Modus module and how an
// admin-gated contribution stays hidden.
std::vector<SettingRow> BuildSettingsCatalog(
    scada::aui::MenuModel& settings_menu);

// The command ids `settings_menu` publishes that the catalogue has no row for.
//
// Empty in a shipping shell. A preference a module adds to
// `MainMenuId::Settings` reaches the operator only once it is described here,
// so without this the surface would silently swallow it -- the one regression a
// model-walking dialog could not have.
std::vector<unsigned> UndescribedSettingsCommands(
    scada::aui::MenuModel& settings_menu);

// Does a row match the search box's contents?
//
// Every whitespace-separated term must appear somewhere in the row's title,
// description or category name -- so "modus renderer" and "renderer modus" both
// find the same row, and a term that matches nothing rules it out. An empty
// query matches everything. Matching is case-insensitive over what the operator
// can actually read, so it runs on the translated strings.
bool SettingRowMatchesQuery(const SettingRow& row, std::u16string_view query);

// Rows surviving the search box and the selected scope tab, in catalogue order.
// An unset `scope` is the "All" tab.
std::vector<SettingRow> FilterSettingRows(std::span<const SettingRow> rows,
                                          std::u16string_view query,
                                          std::optional<SettingScope> scope);

struct SettingCategoryGroup {
  SettingCategory category = SettingCategory::kAppearance;
  std::u16string label;
  std::vector<SettingRow> rows;
};

// Groups rows under their category headings, in screen order.
//
// A category with no surviving row is dropped rather than drawn empty: the
// table of contents is built from this, and an entry with a count of zero is a
// destination that scrolls nowhere.
std::vector<SettingCategoryGroup> GroupSettingRows(
    std::span<const SettingRow> rows);

// The scope tabs to draw, given the rows this session can see. Only scopes some
// visible row actually uses -- a tab that can only ever come back empty is
// worse than no tab.
std::vector<SettingScope> VisibleSettingScopes(
    std::span<const SettingRow> rows);

struct SettingRowCounts {
  std::size_t settings = 0;
  std::size_t actions = 0;
};

// How many of these rows hold a value, and how many just run something.
SettingRowCounts CountSettingRows(std::span<const SettingRow> rows);
