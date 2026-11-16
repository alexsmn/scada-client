#pragma once

#include "controller/controller.h"

#include <gmock/gmock.h>

#if defined(UI_QT)
#include <QWidget>
#endif

class MockController : public Controller {
 public:
  MockController() {
    // An Awaitable-returning mock needs a default action (root CLAUDE.md,
    // "Unit Test Guidance"): unstubbed, nothing is pending, so the view may
    // close.
    ON_CALL(*this, ResolvePendingChanges())
        .WillByDefault([]() -> Awaitable<bool> { co_return true; });
  }

  MOCK_METHOD(SelectionModel*, GetSelectionModel, (), (override));

  MOCK_METHOD(std::unique_ptr<UiView>,
              Init,
              (const WindowDefinition& definition),
              (override));

  MOCK_METHOD(bool, HasPendingChanges, (), (const override));
  MOCK_METHOD(Awaitable<bool>, ResolvePendingChanges, (), (override));

  MOCK_METHOD(void, Save, (WindowDefinition & definition), (override));
  MOCK_METHOD(void, OnViewNodeCreated, (const NodeRef& node), (override));

  MOCK_METHOD(bool,
              ShowContainedItem,
              (const scada::NodeId& item_id),
              (override));

  MOCK_METHOD(CommandHandler*,
              GetCommandHandler,
              (unsigned command_id),
              (override));

  MOCK_METHOD(ContentsModel*, GetContentsModel, (), (override));

  MOCK_METHOD(TimeModel*, GetTimeModel, (), (override));

  MOCK_METHOD(ExportModel*, GetExportModel, (), (override));

  // View root node for creation.
  MOCK_METHOD(NodeRef, GetRootNode, (), (const override));

  MOCK_METHOD(std::optional<OpenContext>, GetOpenContext, (), (const override));
};
