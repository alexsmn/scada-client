#include "graph/graph_view.h"

#include "aui/test/app_environment.h"
#include "common_resources.h"
#include "controller/test/controller_environment.h"

#include <boost/asio/io_context.hpp>

#include "base/debug_util-inl.h"

using namespace testing;

class GraphViewTest : public Test {
 public:
  virtual void SetUp() override;

 protected:
  AppEnvironment app_env_;
  ControllerEnvironment env_;

  GraphView graph_view_{env_.MakeControllerContext()};

  std::shared_ptr<UiView> ui_view_;
};

void GraphViewTest::SetUp() {
  WindowDefinition def{GetWindowInfo(ID_GRAPH_VIEW)};
  ui_view_.reset(graph_view_.Init(def));
}

TEST_F(GraphViewTest, Test) {
  auto* time_model = graph_view_.GetTimeModel();
  ASSERT_THAT(time_model, NotNull());

  time_model->SetTimeRange(TimeRange::Type::Day);
  // EXPECT_THAT(time_model->GetTimeRange(), Eq(TimeRange::Type::Day));
}
