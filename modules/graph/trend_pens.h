#pragma once

#include <QColor>

#include <span>

// The default colours a trend gives the series nobody has picked a colour for.
//
// These are identity colours, not process semantics: they tell one series from
// another and mean nothing else. So the sequence leaves out every hue the
// client reserves for a process signal — the severity ramp's red, orange,
// amber and yellow, its low-severity blue, and the quality green — because a
// pen in an alarm hue reads as an alarm (docs/client/ux/principles.md §1). It
// replaced picking the first readable entry of the colour-name palette
// (`aui/color.cpp`), which is Black, White, Red, … and so gave the first
// series of every dark-theme trend an alarm red (backlog 1124).
//
// A pen is persisted with the window as a literal colour, so a series keeps the
// pen it was given when the appearance changes afterwards. Every entry is
// therefore pitched to stay readable on BOTH canvases rather than per theme:
// each reaches at least 3:1 (WCAG 1.4.11's non-text minimum) against the dark
// canvas (#171717), the light one (#ffffff) and high contrast (#000000).
std::span<const QColor> TrendPenSequence();

// True when `color` is distinguishable as a line on `background`.
bool IsReadablePenOnBackground(const QColor& color, const QColor& background);

// Picks the colour for a series being added to a trend whose canvas is
// `background` and whose existing series are drawn in `used`.
//
// `preferred` is the operator's own default pen (Profile's
// `graph_view.default_color`, set from the line setup dialog); an invalid
// QColor means they never chose one. An explicit choice is honoured whatever
// its hue, while it is readable and not already taken. Otherwise the first
// unused pen of TrendPenSequence() readable on the canvas wins, and once every
// pen is taken the sequence repeats in order.
QColor ChooseNewPenColor(const QColor& preferred,
                         const QColor& background,
                         std::span<const QColor> used);
