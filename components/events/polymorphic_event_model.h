#pragma once

#include "scada/event.h"

#include <vector>

class PolymorphicEventModel final {
 public:
  std::vector<scada::Event> GetEvents() const { return concept_->GetEvent(); }

  void Ack(scada::EventId ack_id) { concept_->Ack(ack_id); }

  bool IsWorking() const { return concept_->IsWorking(); }

 private:
  class Concept {
    virtual ~Concept() = default;

    virtual std::vector<scada::Event> GetEvents() const = 0;
    virtual void Ack(scada::EventId ack_id) = 0;
    virtual bool IsWorking() const = 0;
  };

  template <class T>
  class ConceptImpl final : public Concept {
    virtual std::vector<scada::Event> GetEvents() const override {
      return impl_->GetEvents();
    }

    virtual void Ack(scada::EventId ack_id) override {
      return impl_->Ack(ack_id);
    }

    virtual bool IsWorking() const override { return impl_->IsWorking(); }

   private:
    T impl_;
  };

  std::unique_ptr<Concept> concept_;
};