#include "main_window/pages/initial_page.h"

#include "main_window/overview_page.h"

Page CreateInitialPage() {
  // A fresh profile lands on the operator Overview cockpit (trend + active
  // alarms). The Explorer, journal and the other surfaces are workbench chrome
  // reached from the activity rail, so they are not laid out as page windows.
  return MakeOverviewPage();
}
