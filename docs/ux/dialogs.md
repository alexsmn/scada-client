# SCADA Client — Dialogs

> Status: living design document. Conventions for every modal and modeless
> dialog in the client. Rationale is in [`principles.md`](principles.md) §9
> (be a native desktop application); layout of the main window is in
> [`shell.md`](shell.md).

Dialogs are where non-native design shows first and costs most. They are small,
they appear at a decision point, and the operator's expectations for them are
set entirely by the rest of their OS — button order, keyboard behaviour, where
the title lives. A dialog that gets those wrong is read as "this program is a
port" long before anyone looks at the main window.

They are also the easiest surface to get right, because Qt already knows the
platform conventions. Most of the rules below are "let Qt do it".

## 1. The title bar is the title

**A Qt dialog has a real OS title bar. Do not repeat its title in the content
area, and do not add a brand lockup.**

This is the single most common defect inherited from browser-drawn mockups. A
modal in a browser has no OS chrome, so it *must* draw its own heading, its own
product mark, and often its own close button. A `QDialog` gets all three from
the window manager. Reproducing them costs vertical space, pushes the actual
fields down, and produces the title twice.

- Set a `windowTitle` that names the **task**, not the app and not the widget:
  "Change password", "Time range", "Write value". Never leave Qt Designer's
  default `Dialog`.
- No `TC` mark, no "Telecontrol SCADA" wordmark, no subtitle restating what the
  window already says. The application is identified by the window itself, by
  the menu bar, and by the About dialog.
- A short explanatory line above the fields is fine when it tells the operator
  something the title cannot — *what will happen*, *which server*, *what is
  affected*. That is body text, not a heading.

## 2. Always use `QDialogButtonBox`

**Never lay out OK/Cancel by hand in a `QHBoxLayout`.** Button order is a
platform convention, and it is the opposite on the two platforms we ship:

| Platform | Order | Affirmative |
|---|---|---|
| macOS | `[Cancel] [OK]` | rightmost |
| Windows | `[OK] [Cancel]` | leftmost |
| GNOME / KDE | varies by desktop | follows the platform theme |

A hand-rolled row is frozen in one order and is therefore wrong on the other
platform. `QDialogButtonBox` reorders its children according to the current
style, so one form is correct everywhere.

It also gives us, for free:

- **Translated labels.** Standard buttons come from Qt's own catalogue, so
  `Ok`/`Cancel` render as `ОК`/`Отмена` from `qtbase_ru.qm` without an entry in
  our `.ts` files. Hand-rolled buttons need their own translation and silently
  ship English when it is missing.
- **Correct roles.** `AcceptRole`/`RejectRole`/`DestructiveRole`/`HelpRole`
  place and group buttons the way the platform expects.
- **Escape and Return.** The box wires Escape to the reject button; mark the
  affirmative button as default so Return activates it.

Use the standard buttons when they fit:

```cpp
ui.buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
```

When a verb is clearer than "OK" — and for anything consequential it is —
relabel the accept button rather than abandoning the box:

```cpp
ui.buttonBox->button(QDialogButtonBox::Ok)->setText(Translate("Sign in"));
```

## 3. Name the action, not the assent

For a dialog that *does something*, "OK" makes the operator re-read the title to
work out what they are agreeing to. Label the affirmative button with the verb:
**Sign in**, **Write**, **Apply limits**, **Create**, **Export**. Reserve "OK"
for dialogs that only acknowledge or edit settings in place.

This matters most for control actions (`principles.md` §7): a two-stage confirm
whose final button says "OK" has thrown away the second stage's whole purpose.

## 4. Destructive actions

- Put the destructive button in `QDialogButtonBox::DestructiveRole`, so the
  platform positions and styles it correctly.
- Label it with the destruction: **Delete**, **Disable**, **Discard** — never
  "OK", never "Yes".
- Make **Cancel** the default so Return does not destroy anything.
- State the consequence in body text — what is lost, how much, whether it can
  be undone. "Are you sure?" tells the operator nothing they did not know.
- The `role="danger"` dynamic property is available for tinting
  (`aui/qt/theme_qt.h`); it is the one styling hook the global sheet still
  carries.

## 5. Let the platform draw it

- **Do not theme stock dialogs.** `QMessageBox`, `QFileDialog`, `QColorDialog`,
  `QFontDialog`, `QInputDialog` must render natively — on macOS `QFileDialog`
  becomes a real Finder sheet, which no amount of styling can match. Route
  message boxes through `aui/dialog_service.h` and leave their appearance alone.
- **No `setStyleSheet` for colour or font size** in a dialog. Colour comes from
  `QPalette`, sizes from `QStyle::PixelMetric`/`QFontMetrics`
  ([`design-language.md`](design-language.md)).
- **Do not fix pixel sizes.** Let layouts compute the size hint so the dialog
  grows with the OS font-size setting and with translated strings, which are
  routinely longer in Russian than in English.
- **Use `QFormLayout`** for label/field pairs. It applies the platform's label
  alignment and spacing — right-aligned on Windows, left-aligned on macOS —
  which a hand-built grid does not.

## 6. Keyboard and focus

- Set an explicit initial focus widget: the first field the operator will type
  in, not the button box.
- Mark the affirmative button `setDefault(true)`.
- Every field gets a label with a buddy (`QLabel::setBuddy`) so mnemonics and
  screen readers work.
- Escape must always cancel without side effects.

## 7. Parenting and modality

- Always pass a parent so the dialog centres on the right window and inherits
  the task-modal behaviour.
- Prefer `QDialog::open()` (window-modal) over `exec()` (application-modal)
  where the rest of the app should stay live — a control-room operator should
  not be locked out of an alarm view by a settings dialog.

## 8. Text

- **Sentence case** for titles, labels and buttons: "Change password", not
  "Change Password".
- Labels end with a colon; buttons do not.
- English in source, Russian via the `.ts` files — see the localisation notes
  in the client `CLAUDE.md`. Standard `QDialogButtonBox` buttons need no entry.
- **Never hard-code a user-facing string as a `u"..."` literal.** It can then
  never be translated, and nothing notices: `lupdate` does not recognise
  `Translate()`, the `.ui` checker only reads forms, and a lookup that finds
  nothing falls back silently to the English source. This is how the
  select-before-operate confirmation — the most safety-critical text in the
  client — shipped in English. `client_untranslated_string_check` (ctest,
  `tools/check_untranslated_ui_strings.py`) now fails on a literal reaching a
  message box, a `ResourceError`, a file-dialog title or `setWindowTitle`.
- A namespace-scope `const char16_t k…[] = u"…"` **cannot** call `Translate()`
  at all — it reads the installed catalog and so needs a running
  `QApplication`. Make it a function returning `std::u16string`. That constant
  form is where the bug hides, so the check resolves constants too.

## Audit and revision (2026-07-26)

All eleven forms were revised in one pass. What was wrong:

- **Ten of eleven hand-rolled their buttons** in a `QHBoxLayout`, every one of
  them ordered `[OK] [Cancel]` — the Windows convention, and therefore
  backwards on macOS in all ten. Only `login_dialog.ui` used
  `QDialogButtonBox`, and it was correct on both platforms for exactly that
  reason.
- **`login_dialog.cpp` drew its own chrome** over the real title bar: a `TC`
  brand mark, a "Sign in" heading duplicating the window title, and a product
  subtitle — roughly a third of the dialog's height before the first field.
- **`write_dialog.ui` still had Qt Designer's default `windowTitle`**, so the
  control dialog's title bar read `Dialog`.

What changed:

| Dialog | Change |
|---|---|
| `login_dialog` | Brand mark, heading and subtitle removed (§1); accept button relabelled **Sign in** (§3) |
| `write_dialog` | `windowTitle` `Dialog` → **Write value**; `QDialogButtonBox`; the enable/disable of the accept button moved to `button(QDialogButtonBox::Ok)` |
| `about_dialog` | `QDialogButtonBox` (Ok only) |
| `change_password`, `create_service_item`, `csv_export`, `add_favourites`, `limit`, `multi_create`, `time_range`, `transport` | `QDialogButtonBox` (Ok + Cancel) |

The button-order fix is free at runtime: `QDialogButtonBox` reads
`QStyle::SH_DialogButtonLayout`, so the same form renders `[Cancel] [OK]` on
macOS and `[OK] [Cancel]` on Windows. The hand-rolled `OK`/`Cancel` strings
also disappeared from the forms, so those labels now come from Qt's own
`qtbase_ru.qm` rather than needing entries in `client_ru.ts`.

### Still to do

**Naming the action (§3) is applied only to the login dialog.** The remaining
dialogs still accept with `OK`, and the verbs are a terminology decision worth
making deliberately rather than inventing. The ones that most want it:

| Dialog | Suggested accept label |
|---|---|
| `write_dialog` (manual input / control) | *Write* / *Execute* — §7 argues a two-stage confirm ending in "OK" wastes the second stage |
| `multi_create`, `create_service_item` | *Create* |
| `csv_export` | *Export* |
| `add_favourites` | *Add* |
| `limit_dialog` | *Apply* |

`change_password` and `transport` are settings edits, where `OK` is defensible.

## Maintenance

- A new dialog uses `QDialogButtonBox` from the start; a review that sees a
  hand-rolled button row should reject it.
- Validate against **real Qt widgets** via the headless
  `client_screenshot_generator` (`docs/screenshots.md`), in both a light and a
  dark system appearance — not against HTML mockups, whose chrome predates the
  native direction.
