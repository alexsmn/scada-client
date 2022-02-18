#include "components/events/events_component.h"

#include "components/events/event_view.h"
#include "controller_registry.h"

class EventPanel : public EventView {
 public:
  explicit EventPanel(const ControllerContext& context)
      : EventView(context, true) {}
};

class EventJournal : public EventView {
 public:
  explicit EventJournal(const ControllerContext& context)
      : EventView(context, false) {}
};

const WindowInfo kEventWindowInfo = {
    ID_EVENT_VIEW, "Event", u"События", WIN_SING | WIN_DOCKB | WIN_CAN_PRINT,
    800,           200,     0};

const WindowInfo kEventJournalWindowInfo = {ID_EVENT_JOURNAL_VIEW,
                                            "EventJournal",
                                            u"Журнал событий",
                                            WIN_INS | WIN_CAN_PRINT,
                                            0,
                                            0,
                                            0};

REGISTER_CONTROLLER(EventPanel, kEventWindowInfo);
REGISTER_CONTROLLER(EventJournal, kEventJournalWindowInfo);
