#pragma once

#include <QKeySequence>
#include <QLineEdit>

#include <functional>

class QAction;
class QEvent;
class QFont;
class QIcon;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;

// The top context bar's command/search field — the entry point to the command
// palette (docs/client/ux/shell.md §2.2).
//
// It looks like a search box but owns no text: clicking it, or typing a
// printable character into it, opens the palette, which is where the typing
// actually happens. The field's whole job is to say that the palette exists and
// how to reach it, which is why it carries two marks the plain QLineEdit it
// replaced did not:
//
//  - a leading magnifier, tinted from QPalette::PlaceholderText so it reads at
//    the same weight as the prompt and follows a live theme switch;
//  - a trailing shortcut hint, drawn in the platform's own key notation
//    (`⌘K` on macOS, `Ctrl+K` on Windows) rather than one spelling for both.
//
// Everything is measured in font metrics and QStyle pixel metrics — the fixed
// 360 px width this replaced ignored the OS text-size setting (shell.md §2.2,
// native rework) — and drawn from the palette, so the field is native-styled on
// every host and needs no stylesheet.
class CommandField : public QLineEdit {
  Q_OBJECT

 public:
  // Invoked when the operator activates the field. The argument is the
  // character that triggered it, so the palette can open already seeded, and is
  // empty for a click.
  using ActivateCallback = std::function<void(const QString& initial_text)>;

  // `prompt` is the placeholder — the field sizes itself to show all of it, so
  // it is a constructor argument rather than something set afterwards.
  CommandField(QWidget* parent,
               QString prompt,
               QKeySequence shortcut,
               ActivateCallback on_activate);

  // QWidget — wide enough for the whole prompt plus the glyph and the hint, so
  // the prompt never elides, and derived from the current font so the field
  // tracks the OS text-size setting.
  QSize sizeHint() const override;

  // The shortcut hint as drawn, in the platform's key notation. Empty when the
  // field was given no shortcut.
  QString HintText() const;

 protected:
  // QWidget — draws the shortcut hint over the field the native style has
  // already painted.
  void paintEvent(QPaintEvent* event) override;

  // QWidget — the field is an affordance, not an editor: a click opens the
  // palette instead of placing a cursor.
  void mouseReleaseEvent(QMouseEvent* event) override;

  // QWidget — a printable character opens the palette seeded with it, so the
  // field behaves like the search box it resembles even though the palette owns
  // the input.
  void keyPressEvent(QKeyEvent* event) override;

  // QWidget — re-tints the glyph and re-measures the hint after a palette,
  // font or style change, so a live theme switch does not leave either drawn
  // for the previous one.
  void changeEvent(QEvent* event) override;

 private:
  // Runs the activation callback, if one was given.
  void Activate(const QString& initial_text);

  // The magnifier at the field's current icon size and placeholder colour.
  QIcon GlyphIcon() const;
  // Re-reads the glyph from the live palette.
  void ApplyGlyph();

  // The hint's font: the field's own, a step down, so it reads as an annotation
  // rather than as content.
  QFont HintFont() const;
  // The hint chip's size, padding included.
  QSize HintSize() const;
  // Where the chip is drawn: right-aligned inside the style's own line-edit
  // contents rect, vertically centred.
  QRect HintRect() const;
  // The breathing space between the chip, the text and the frame.
  int Gap() const;
  // Reserves the chip's width in the text margins so the prompt cannot run
  // underneath it.
  void UpdateHintMargin();

  QKeySequence shortcut_;
  ActivateCallback on_activate_;
  QAction* search_action_ = nullptr;
};
