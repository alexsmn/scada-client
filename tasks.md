# Client tasks

Work that lives **only** in `scada-client` (UI, components, profile,
modus, vidicon). Cross-cutting items that span the client and server (or
that touch the wire protocol) belong in [`tasks.md`](../tasks.md) at the
repo root.

Tasks are grouped by area. New entries go under the matching cluster or,
if none fits, into **Misc** — never as a flat top-level entry.

## Object, hardware & file trees

### `ObjectView`: Don't show "has children" for TS and TIT

### `ConfigurationNodeView`: Show "loading" when node itself is not loaded

### `HardwareTree`: Metrics from root

### Save expanded tree nodes

### Locate current object in the `ObjectTree`

### `ObjectTree`: Search / filter

### Progress bar for object pulls for `WindowDefinition` building

## Property editor, limits & dialogs

### `PropertyView`: Expand on the first open. Especially for just created nodes

### Don't open duplicate node-properties window

### `LimitDialog`: Show error when set limits fails and don't close the dialog

### Property editor: Non-modifiable attributes must be grey

## Command framework

### Split `SelectionCommands` into multiple files or per components

### Split `OpenedViewCommands` into multiple files or per components

### `MainCommands`, `OpenedViewCommands` and `SelectionCommands` should be singletons

Then they can be constructed via `ComponentApi`.

### Apply dependency-injection framework

### Isolate components and make them responsible for wiring

## Login & data services

### Login: Make server list depend on type

Needed for OPC UA connection.

### `DeviceCommands`: Use aliases where possible

Debatable.

### Don't create `MonitoredItem` on selection

E.g. for files selected in File View.

## Events, graphs & display

### Show local error events in event view in red color

### Graph: A line mark for historical vs real-time points

### Icons or hints for value indicators like `[C]`

## Modus & data-item editor

### Modus Qt: Implement Modus commands

### `DataItem`: Address builder

## Misc

### Add more shutdown logs

### Custom table: Open new table for multiple selection

### Get rid of `IsWorking`
