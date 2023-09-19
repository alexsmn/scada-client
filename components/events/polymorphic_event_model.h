#pragma once

#include "scada/event.h"

#include <vector>

class PolymorphicEventModel {
 public:
 private:
  class Concept {
    virtual ~Concept() = default;

    virtual std::vector<scada::Event> GetEvents() const = 0;
    virtual void Ack(scada::EventAcknowledgeId ack_id) = 0;
    virtual bool IsWorking() const = 0;
  };
};