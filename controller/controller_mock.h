#pragma once

#include "controller/controller.h"

#include <gmock/gmock.h>

#if defined(UI_QT)
#include <QWidget>
#elif defined(UI_WT)
#include <Wt/WWidget.h>
#endif

class MockController : public Controller {
 public:
  MOCK_METHOD(SelectionModel*, GetSelectionModel, (), (override));

  MOCK_METHOD(std::unique_ptr<UiView>,
              Init,
              (const WindowDefinition& definition),
              (override));

  MOCK_METHOD(bool, CanClose, (), (const override));
  MOCK_METHOD(bool, IsWorking, (), (const override));

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
