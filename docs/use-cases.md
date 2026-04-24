# Client Use Cases

This page is the test-oriented companion to the client use-case diagram.

<p align="center">
  <img src="use-cases.svg" alt="Client use-case diagram" width="820">
</p>

> Source: [`use-cases.mmd`](use-cases.mmd). Regenerate the SVG with
> `mmdc -i use-cases.mmd -o use-cases.svg -b transparent`.

## Test Matrix

The client/server E2E suite provides automated smoke coverage for operator use
cases UC-1 through UC-11 through
`ClientServerE2eTest.OperatorUseCases_OpenRegisteredSurfaces`. Configuration
and administration use cases should be tested with the manual checks below until
dedicated automated coverage exists.

### UC-1 Monitor live values

Test by opening the watch/log view against a running server and confirming live
item values render without startup errors. Automated coverage verifies that the
production Qt client can construct the live-value operator view after login.

### UC-2 Visualise time-series on a graph

Test by opening the graph view, selecting a time-series item, and confirming the
graph control loads the item history or current trend without crashing.
Automated coverage verifies graph-view construction after login.

### UC-3 View tables, summaries and sheets

Test by opening table, summary and custom-table views from the operator profile
and confirming rows or sheet cells are populated from the server session.
Automated coverage opens the registered table, summary and custom-table
surfaces.

### UC-4 Acknowledge events / alarms

Test by opening the event/alarm view, selecting an active event that permits
acknowledgement, issuing the acknowledge action, and confirming the event state
updates. Automated coverage verifies the event surface opens against the live
session.

### UC-5 Browse event journals

Test by opening the event journal, applying a time range, and confirming journal
queries complete with either matching rows or an empty result state. Automated
coverage opens the event-journal surface.

### UC-6 Watch a custom spreadsheet

Test by opening a custom spreadsheet view and confirming calculated or bound
cells render. Automated coverage opens the custom-table surface used by custom
spreadsheets.

### UC-7 Issue control commands

Test by selecting a writable point in a controlled test system, issuing write
and manual-write commands, and confirming the server accepts or rejects the
command with the expected status. Automated coverage verifies the write command
registrations are present in the logged-in client.

### UC-8 Manage favourites and portfolios

Test by opening favourites and portfolio views, adding/removing an item, and
confirming the updated list is persisted or reflected in the UI. Automated
coverage opens both registered surfaces.

### UC-9 Print / export the active view

Test by opening a printable view such as a table or event journal, invoking
print/export, and confirming the preview, printer handoff, or exported file is
created. Automated coverage verifies printable metadata for the table and event
journal views.

### UC-10 Browse server files

Test by opening the server file-system view, expanding a directory, and
confirming file metadata is loaded from the server. Automated coverage opens the
file-system surface.

### UC-11 View Modus / Vidicon schematics

Test on a workstation with the required schematic dependencies installed by
opening a Modus or Vidicon schematic and confirming the diagram renders.
Automated coverage verifies the Modus and Vidicon window registrations without
requiring the external display components.

### UC-12 Edit device parameters, limits and aliases

Test with an engineer account by editing a non-production device parameter,
limit and alias, saving the changes, reopening the view, and confirming the
persisted values match. Also verify validation rejects invalid values.

### UC-13 Bulk-create data items

Test by importing or generating a small batch of data items in a temporary
configuration, confirming all expected items are created, and checking duplicate
or invalid rows produce actionable errors without partial corruption.

### UC-14 Export / import configuration

Test by exporting a known temporary configuration, importing it into a clean
workspace, and comparing the resulting devices, data items, users and rules
against the source configuration.

### UC-15 Inspect protocol traffic

Test by opening the protocol traffic inspection view for an enabled protocol,
generating a known request, and confirming the traffic list records timestamps,
direction and decoded payload details.

### UC-16 Save window layouts and profiles

Test by changing window layout/profile state, closing the client, reopening it,
and confirming the same windows, sizes, positions and active profile are
restored.

### UC-17 Authenticate against a back-end

Test successful and failed login paths for each supported back end. Automated
E2E coverage verifies successful and bad-password login behavior for the SCADA
remote-session and OPC UA back ends.

### UC-18 Manage users / passwords

Test with an admin account by creating a user, changing its password and rights,
logging in as that user, and confirming access is granted or denied according to
the assigned permissions.

### UC-19 Configure transmission rules

Test by creating or editing a transmission rule in a temporary configuration,
triggering matching data or event conditions, and confirming the configured
transmission action runs once with the expected target and payload.
