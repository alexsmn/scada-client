#include "aui/aui_ns_compat.h"

#include "aui/qt/theme_qt.h"

#include <QApplication>
#include <QString>

namespace scada::aui {

namespace {

// Builds the three token tables from the exact values in
// client/docs/ux/design-language.md. Semi-transparent tokens (borders, soft
// tints) carry an alpha derived from the documented opacity percentage.
ThemeTokens MakeDarkTokens() {
  ThemeTokens t;
  // Surfaces.
  t.bg = QColor(0x07, 0x11, 0x1b);
  t.bg_elevated = QColor(0x0b, 0x16, 0x23);
  t.surface = QColor(0x0f, 0x19, 0x25);
  t.surface_muted = QColor(0x15, 0x26, 0x35);
  t.rail_bg = QColor(0x06, 0x11, 0x1b);
  t.topbar_bg = QColor(0x0b, 0x16, 0x23);
  // Text.
  t.fg = QColor(0xee, 0xf5, 0xfb);
  t.fg_muted = QColor(0xc3, 0xd0, 0xdb);
  t.fg_subtle = QColor(0x8f, 0xa3, 0xb4);
  t.fg_on_dark = QColor(0xee, 0xf5, 0xfb);
  // Lines: rgba(255,255,255,.12) / .24.
  t.border = QColor(255, 255, 255, 31);
  t.border_strong = QColor(255, 255, 255, 61);
  // Accent and quality.
  t.accent = QColor(0x77, 0xb4, 0xf3);
  t.accent_fg = QColor(0x0b, 0x16, 0x23);
  t.accent_soft = QColor(119, 180, 243, 38);  // rgba(...,.15)
  t.good = QColor(0x44, 0xc0, 0x91);
  t.uncertain = QColor(0xe6, 0xb2, 0x4b);
  t.bad = QColor(0xf0, 0x71, 0x68);
  // Severity ramp.
  t.severity_critical = QColor(0xe8, 0x5a, 0x52);
  t.severity_high = QColor(0xf0, 0x71, 0x68);
  t.severity_medium = QColor(0xe6, 0xb2, 0x4b);
  t.severity_low = QColor(0x77, 0xb4, 0xf3);
  // Single-line diagram.
  t.sl_live = QColor(0xe6, 0xb2, 0x4b);
  t.sl_energized = QColor(0x8f, 0xa3, 0xb4);
  t.sl_closed = QColor(0x44, 0xc0, 0x91);
  t.sl_open = QColor(0x8f, 0xa3, 0xb4);
  return t;
}

ThemeTokens MakeLightTokens() {
  ThemeTokens t;
  t.bg = QColor(0xf5, 0xf8, 0xfb);
  t.bg_elevated = QColor(0xf8, 0xfb, 0xfe);
  t.surface = QColor(0xff, 0xff, 0xff);
  t.surface_muted = QColor(0xf1, 0xf5, 0xf9);
  t.rail_bg = QColor(0x0d, 0x1a, 0x27);
  t.topbar_bg = QColor(0xff, 0xff, 0xff);
  t.fg = QColor(0x11, 0x18, 0x27);
  t.fg_muted = QColor(0x4b, 0x5b, 0x6c);
  t.fg_subtle = QColor(0x6b, 0x7b, 0x8d);
  t.fg_on_dark = QColor(0xee, 0xf5, 0xfb);
  // Lines: rgba(15,23,42,.12) / .22.
  t.border = QColor(15, 23, 42, 31);
  t.border_strong = QColor(15, 23, 42, 56);
  t.accent = QColor(0x0f, 0x6b, 0xff);
  t.accent_fg = QColor(0xff, 0xff, 0xff);
  t.accent_soft = QColor(15, 107, 255, 26);  // rgba(...,.1)
  t.good = QColor(0x14, 0x82, 0x5f);
  t.uncertain = QColor(0xb6, 0x7a, 0x17);
  t.bad = QColor(0xc5, 0x3d, 0x35);
  t.severity_critical = QColor(0x8f, 0x24, 0x1f);
  t.severity_high = QColor(0xb7, 0x31, 0x2b);
  t.severity_medium = QColor(0xc1, 0x8a, 0x24);
  t.severity_low = QColor(0x23, 0x5f, 0x98);
  t.sl_live = QColor(0xb6, 0x7a, 0x17);
  t.sl_energized = QColor(0x6b, 0x7b, 0x8d);
  t.sl_closed = QColor(0x14, 0x82, 0x5f);
  t.sl_open = QColor(0x8a, 0x99, 0xa8);
  return t;
}

ThemeTokens MakeHighContrastTokens() {
  ThemeTokens t;
  t.bg = QColor(0x00, 0x00, 0x00);
  t.bg_elevated = QColor(0x00, 0x00, 0x00);
  t.surface = QColor(0x00, 0x00, 0x00);
  t.surface_muted = QColor(0x11, 0x11, 0x11);
  t.rail_bg = QColor(0x00, 0x00, 0x00);
  t.topbar_bg = QColor(0x00, 0x00, 0x00);
  t.fg = QColor(0xff, 0xff, 0xff);
  t.fg_muted = QColor(0xff, 0xff, 0xff);
  t.fg_subtle = QColor(0xff, 0xff, 0xff);
  t.fg_on_dark = QColor(0xff, 0xff, 0xff);
  t.border = QColor(0xff, 0xff, 0xff);
  t.border_strong = QColor(0xff, 0xff, 0xff);
  t.accent = QColor(0xff, 0xff, 0x00);
  t.accent_fg = QColor(0x00, 0x00, 0x00);
  t.accent_soft = QColor(0x11, 0x11, 0x11);
  t.good = QColor(0x00, 0xff, 0x7a);
  t.uncertain = QColor(0xff, 0xff, 0x00);
  t.bad = QColor(0xff, 0x6b, 0x6b);
  t.severity_critical = QColor(0xff, 0x6b, 0x6b);
  t.severity_high = QColor(0xff, 0x9f, 0x43);
  t.severity_medium = QColor(0xff, 0xff, 0x00);
  t.severity_low = QColor(0x00, 0xd4, 0xff);
  t.sl_live = QColor(0xff, 0xff, 0x00);
  t.sl_energized = QColor(0xff, 0xff, 0xff);
  t.sl_closed = QColor(0x00, 0xff, 0x7a);
  t.sl_open = QColor(0xff, 0xff, 0xff);
  return t;
}

// Formats a colour for a Qt style sheet. Opaque colours use `#RRGGBB`;
// translucent colours use Qt's `#AARRGGBB` form, which QSS parses natively (its
// rgba() function's alpha handling is version-dependent, so we avoid it).
QString Css(const QColor& c) {
  if (c.alpha() == 255) {
    return QString::asprintf("#%02x%02x%02x", c.red(), c.green(), c.blue());
  }
  return QString::asprintf("#%02x%02x%02x%02x", c.alpha(), c.red(), c.green(),
                           c.blue());
}

}  // namespace

const ThemeTokens& GetThemeTokens(Theme theme) {
  static const ThemeTokens kDark = MakeDarkTokens();
  static const ThemeTokens kLight = MakeLightTokens();
  static const ThemeTokens kHighContrast = MakeHighContrastTokens();
  switch (theme) {
    case Theme::kLight:
      return kLight;
    case Theme::kHighContrast:
      return kHighContrast;
    case Theme::kDark:
      break;
  }
  return kDark;
}

Theme ThemeFromString(const QString& name, Theme fallback) {
  const QString key = name.trimmed().toLower();
  if (key == QStringLiteral("dark")) {
    return Theme::kDark;
  }
  if (key == QStringLiteral("light")) {
    return Theme::kLight;
  }
  if (key == QStringLiteral("hc") || key == QStringLiteral("high-contrast") ||
      key == QStringLiteral("highcontrast")) {
    return Theme::kHighContrast;
  }
  return fallback;
}

QString ThemeToString(Theme theme) {
  switch (theme) {
    case Theme::kLight:
      return QStringLiteral("light");
    case Theme::kHighContrast:
      return QStringLiteral("hc");
    case Theme::kDark:
      break;
  }
  return QStringLiteral("dark");
}

QPalette BuildThemePalette(const ThemeTokens& t) {
  QPalette p;

  // Base surfaces and text.
  p.setColor(QPalette::Window, t.bg);
  p.setColor(QPalette::WindowText, t.fg);
  p.setColor(QPalette::Base, t.surface);
  p.setColor(QPalette::AlternateBase, t.surface_muted);
  p.setColor(QPalette::Text, t.fg);
  p.setColor(QPalette::PlaceholderText, t.fg_subtle);
  p.setColor(QPalette::ToolTipBase, t.surface);
  p.setColor(QPalette::ToolTipText, t.fg);

  // Buttons.
  p.setColor(QPalette::Button, t.surface_muted);
  p.setColor(QPalette::ButtonText, t.fg);
  p.setColor(QPalette::BrightText, t.bad);

  // Selection / links.
  p.setColor(QPalette::Highlight, t.accent);
  p.setColor(QPalette::HighlightedText, t.accent_fg);
  p.setColor(QPalette::Link, t.accent);
  p.setColor(QPalette::LinkVisited, t.accent);

  // Frame shading, derived from the token surfaces so Fusion's bevels read as
  // flat hairlines rather than default grey.
  p.setColor(QPalette::Light, t.surface_muted);
  p.setColor(QPalette::Midlight, t.surface_muted);
  p.setColor(QPalette::Mid, t.border_strong);
  p.setColor(QPalette::Dark, t.bg_elevated);
  p.setColor(QPalette::Shadow, t.rail_bg);

  // Disabled group: dim the text/foreground roles.
  p.setColor(QPalette::Disabled, QPalette::WindowText, t.fg_subtle);
  p.setColor(QPalette::Disabled, QPalette::Text, t.fg_subtle);
  p.setColor(QPalette::Disabled, QPalette::ButtonText, t.fg_subtle);
  p.setColor(QPalette::Disabled, QPalette::HighlightedText, t.fg_subtle);
  p.setColor(QPalette::Disabled, QPalette::Highlight, t.surface_muted);

  return p;
}

QString BuildThemeStyleSheet(const ThemeTokens& t) {
  // A single generated sheet styling the shared chrome vocabulary. Kept flat
  // (thin hairlines, small radii, no bevels) to match the workbench mockups.
  QString qss;
  qss +=
      QStringLiteral(
          // Menu bar / menus.
          "QMenuBar{background:%1;color:%2;border-bottom:1px solid %3;}"
          "QMenuBar::item{background:transparent;padding:4px 9px;}"
          "QMenuBar::item:selected{background:%4;color:%5;border-radius:4px;}"
          "QMenu{background:%6;color:%2;border:1px solid %7;padding:4px;}"
          "QMenu::item{padding:5px 22px;border-radius:4px;}"
          "QMenu::item:selected{background:%4;color:%5;}"
          "QMenu::separator{height:1px;background:%3;margin:4px 8px;}")
          .arg(Css(t.topbar_bg), Css(t.fg), Css(t.border), Css(t.accent),
               Css(t.accent_fg), Css(t.surface), Css(t.border_strong));

  qss += QStringLiteral(
             // Tool bars.
             "QToolBar{background:%1;border-bottom:1px solid %2;spacing:2px;"
             "padding:2px 6px;}"
             "QToolButton{color:%3;padding:4px 8px;border-radius:4px;}"
             "QToolButton:hover{background:%4;}"
             "QToolButton:pressed,QToolButton:checked{background:%5;color:%6;}"
             "QToolBar::separator{width:1px;background:%2;margin:0 4px;}")
             .arg(Css(t.topbar_bg), Css(t.border), Css(t.fg_muted),
                  Css(t.surface_muted), Css(t.accent_soft), Css(t.fg));

  qss += QStringLiteral(
             // Dock widgets.
             "QDockWidget{color:%1;titlebar-close-icon:none;}"
             "QDockWidget::title{background:%2;padding:4px 8px;"
             "border-bottom:1px solid %3;}"
             "QMainWindow::separator{background:%4;width:1px;height:1px;}")
             .arg(Css(t.fg), Css(t.bg_elevated), Css(t.border), Css(t.border));

  qss +=
      QStringLiteral(
          // Item views (trees, tables, lists) and headers.
          "QTreeView,QTableView,QListView{background:%1;alternate-background-"
          "color:%2;color:%3;border:1px solid %4;gridline-color:%4;}"
          "QTreeView::item,QListView::item{padding:2px 4px;}"
          "QTreeView::item:selected,QTableView::item:selected,"
          "QListView::item:selected{background:%5;color:%3;}"
          "QHeaderView::section{background:%2;color:%6;padding:4px 8px;"
          "border:0;border-right:1px solid %4;border-bottom:1px solid %4;}")
          .arg(Css(t.surface), Css(t.surface_muted), Css(t.fg), Css(t.border),
               Css(t.accent_soft), Css(t.fg_subtle));

  qss +=
      QStringLiteral(
          // Editor-style workspace tabs: flat and document-mode; the active
          // tab drops its separators and blends into the content pane with an
          // accent top marker, inactive tabs sit on the elevated bar.
          "QTabWidget::pane{border:1px solid %1;background:%2;}"
          "QTabBar{background:%3;}"
          "QTabBar::tab{background:%3;color:%4;padding:6px 16px;"
          "border:0;border-right:1px solid %1;}"
          "QTabBar::tab:hover{background:%7;color:%5;}"
          "QTabBar::tab:selected{background:%2;color:%5;"
          "border-top:2px solid %6;}"
          "QTabBar::close-button:hover{background:%7;border-radius:3px;}")
          .arg(Css(t.border), Css(t.bg), Css(t.bg_elevated), Css(t.fg_subtle),
               Css(t.fg), Css(t.accent), Css(t.surface_muted));

  qss += QStringLiteral(
             // Dock panels: a slim themed title strip in place of the native OS
             // title chrome, so panels read as workbench regions, not windows.
             "QDockWidget{color:%1;}"
             "QDockWidget::title{background:%2;color:%1;padding:5px 8px;"
             "border-bottom:1px solid %3;}")
             .arg(Css(t.fg_subtle), Css(t.surface_muted), Css(t.border));

  qss += QStringLiteral(
             // Status bar.
             "QStatusBar{background:%1;color:%2;border-top:1px solid %3;}"
             "QStatusBar::item{border:0;}")
             .arg(Css(t.rail_bg), Css(t.fg_subtle), Css(t.border));

  qss +=
      QStringLiteral(
          // Push buttons: default (accent), normal, and role=danger.
          "QPushButton{background:%1;color:%2;border:1px solid %3;"
          "border-radius:6px;padding:5px 14px;}"
          "QPushButton:hover{border-color:%4;}"
          "QPushButton:default{background:%4;color:%5;border-color:%4;}"
          "QPushButton:disabled{color:%6;}"
          "QPushButton[role=\"danger\"]{background:%7;color:%5;"
          "border-color:%7;}")
          .arg(Css(t.surface_muted), Css(t.fg), Css(t.border_strong),
               Css(t.accent), Css(t.accent_fg), Css(t.fg_subtle), Css(t.bad));

  qss += QStringLiteral(
             // Text inputs and combos.
             "QLineEdit,QPlainTextEdit,QTextEdit,QSpinBox,QDoubleSpinBox,"
             "QComboBox{background:%1;color:%2;border:1px solid %3;"
             "border-radius:6px;padding:4px 8px;selection-background-color:%4;}"
             "QLineEdit:focus,QSpinBox:focus,QDoubleSpinBox:focus,"
             "QComboBox:focus{border-color:%4;}"
             "QComboBox QAbstractItemView{background:%5;color:%2;"
             "border:1px solid %3;selection-background-color:%6;}")
             .arg(Css(t.bg_elevated), Css(t.fg), Css(t.border_strong),
                  Css(t.accent), Css(t.surface), Css(t.accent_soft));

  qss +=
      QStringLiteral(
          // Thin scrollbars.
          "QScrollBar:vertical{background:%1;width:10px;margin:0;}"
          "QScrollBar:horizontal{background:%1;height:10px;margin:0;}"
          "QScrollBar::handle{background:%2;border-radius:4px;min-height:24px;"
          "min-width:24px;}"
          "QScrollBar::handle:hover{background:%3;}"
          "QScrollBar::add-line,QScrollBar::sub-line{width:0;height:0;}"
          "QScrollBar::add-page,QScrollBar::sub-page{background:transparent;}")
          .arg(Css(t.bg_elevated), Css(t.border_strong), Css(t.fg_subtle));

  return qss;
}

void ApplyTheme(Theme theme, ThemeScope scope) {
  const ThemeTokens& tokens = GetThemeTokens(theme);
  // Order matters: setStyle() resets the application palette to the style's
  // standard palette, so the palette and stylesheet must be installed after.
  QApplication::setStyle(QStringLiteral("Fusion"));
  QApplication::setPalette(BuildThemePalette(tokens));
  if (auto* app = qApp) {
    // Palette-first: install the global stylesheet only for kFull. Clearing it
    // for kPaletteOnly keeps a live switch from leaving a stale sheet behind.
    app->setStyleSheet(scope == ThemeScope::kFull ? BuildThemeStyleSheet(tokens)
                                                  : QString());
  }
}

}  // namespace scada::aui
