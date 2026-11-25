#pragma once

namespace scada::aui {

// What a cell's colouring *means*, as opposed to which RGB value to paint.
//
// A model lives in toolkit-free code and cannot reach a `QPalette`, so naming
// a literal colour was the only way it could grey a cell -- and a fixed
// mid-grey does not follow the OS theme and reads wrong on a dark palette
// (docs/client/ux/README.md). A role lets the model say *disabled* and leaves
// the resolution to the toolkit adapter, which does have the palette. Tree,
// table and grid models all answer with it (docs/client/ux/design-language.md,
// "Models cannot reach the palette").
//
// This is deliberately not a way to express process semantics. Alarm state and
// data quality are ISA-101/ISA-18.2 signals with fixed values that must not
// follow the platform theme; those stay `Color`, supplied from the profile.
// Nor is it an operator's own cell fill, which is a literal by definition.
enum class ColorRole {
  // The view's own colours. The model is not asking for anything.
  Default,
  // A cell whose value this row will not accept an edit for, or whose value
  // is not there yet (fetching, not ready).
  Disabled,
  // A heading: a row that groups the rows beneath it, or a label cell inside
  // a grid body.
  Header,
  // Prompt text in an otherwise empty cell ("Enter expression"). Recolours
  // the text only.
  Placeholder,
  // A configuration fault the engineer has to act on — two retransmission
  // rules sharing a destination address (backlog 1128). The text takes the
  // design system's warning severity colour and the cell carries the
  // platform's warning icon: colour is never the only signal
  // (docs/client/ux/principles.md §5), so the adapter draws the shape with
  // the role rather than trusting every model to remember it. The grid and
  // table adapters draw the icon; a tree answers a column-0 icon of its own,
  // so a tree model must not ask for this role until its adapter does too.
  Warning,
};

}  // namespace scada::aui
