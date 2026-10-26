#include "aui/test/app_environment.h"
#include "base/client_paths.h"
#include "base/path_service.h"
#include "common/test/scoped_temp_dir.h"
#include "controller/test/controller_environment.h"
#include "filesystem/file_cache.h"
#include "modus/qt/modus_controller.h"
#include "profile/window_definition.h"
#include "resources/common_resources.h"
#include "scada/attribute_service.h"
#include "timed_data/timed_data_mock.h"
#include "timed_data/timed_data_spec.h"

#include <QColor>
#include <QCoreApplication>
#include <QImage>
#include <QMouseEvent>
#include <QScrollArea>
#include <QWidget>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdlib>
#include <fstream>
#include <memory>
#include <string>

#ifdef _WIN32
#include <stdlib.h>
#endif

namespace {

using testing::_;
using testing::Eq;
using testing::NotNull;
using testing::Return;

// Points the display loader at the runtime the configure named, before
// anything calls `DisplayRuntime::Get()` -- which loads once per process --
// exactly as `display_view/display_runtime_unittest.cpp` does for its binary.
class RuntimeUnderTest : public testing::Environment {
 public:
  void SetUp() override {
#ifdef SCADA_DISPLAY_RUNTIME_UNDER_TEST
    const char* library = SCADA_DISPLAY_RUNTIME_UNDER_TEST;
    if (!library || !*library)
      return;
#ifdef _WIN32
    _putenv_s("SCADA_DISPLAY_RUNTIME", library);
#else
    ::setenv("SCADA_DISPLAY_RUNTIME", library, /*overwrite=*/1);
#endif
#endif
  }
};

const testing::Environment* kRuntimeUnderTest =
    testing::AddGlobalTestEnvironment(new RuntimeUnderTest);

// Whether this build names a runtime to drive. A client configured without one
// is supported, so the live-value cases skip rather than fail.
bool HaveRuntime() {
#ifdef SCADA_DISPLAY_RUNTIME_UNDER_TEST
  return *SCADA_DISPLAY_RUNTIME_UNDER_TEST != '\0';
#else
  return false;
#endif
}

// Drives the *production* display view — `ModusController` with no injected
// factory, so a real `ModusDisplayView` over a real `DisplayWidget` over the
// display runtime the configure named (ADR 0013), loaded through its C ABI.
// Its sibling `modus_controller_unittest.cpp` substitutes a fake wrapper and
// tests the controller's own logic; this file is here for the part that a fake
// cannot show, which is what the operator ends up looking at.
//
// Note what is deliberately NOT tested here: whether the renderer draws a
// document correctly. That belongs to the `display` product, which owns the
// parsers and the renderer backends and tests them against its own goldens.
// `client/` is a separate product (ADR 0011); its share of the contract is the
// wiring and the failure surface.
//
// Two halves. The failure surface: a window with no document, or one whose
// document will not parse, must say so rather than paint blank (and with no
// runtime configured, "no display runtime" is the same red message). And
// since backlog 915 the live surface: a bound element's value change reaching
// the drawing, a click selecting the bound object, «Мнемосхема» selecting the
// element, and the schematic's «Заголовок» reaching the file cache the display
// menu reads -- every hop between the timed-data service and the runtime,
// which no single-layer test crosses. Those cases need a runtime and skip
// without one.

// The pen `DisplayWidget::paintEvent` uses for every operator-facing error.
constexpr QColor kErrorTextColor{160, 0, 0};

bool ContainsErrorText(const QImage& image) {
  for (int y = 0; y < image.height(); ++y) {
    for (int x = 0; x < image.width(); ++x) {
      if (image.pixelColor(x, y) == kErrorTextColor)
        return true;
    }
  }
  return false;
}

class ModusDisplayTest : public testing::Test {
 protected:
  void SetUp() override {
    // `GetPublicFilePath` throws without a public directory. The install
    // override outlived the dlopen it was for (ADR 0012 phase 3, and the
    // plugin itself went at phase 4); it is kept only so the fixture resolves
    // nothing from a developer's real install.
    scada::base::PathService::Override(client::DIR_PUBLIC, public_dir_.path());
    scada::base::PathService::Override(client::DIR_INSTALL,
                                       install_dir_.path());
  }

  // The inner display widget: `Init` returns the `QScrollArea` that holds it.
  static QWidget* DisplayWidgetOf(UiView& view) {
    auto* scroll_area = qobject_cast<QScrollArea*>(&view);
    return scroll_area ? scroll_area->widget() : nullptr;
  }

  static QImage Render(QWidget& widget) {
    widget.resize(640, 480);
    return widget.grab().toImage();
  }

  std::filesystem::path WriteDocument(std::string_view name) {
    const std::filesystem::path path = public_dir_.path() / name;
    std::ofstream out{path, std::ios::binary};
    out << "not a real vds document";
    return name;
  }

  // A schematic with a bound breaker, a bound text field and a title, written
  // into the public directory as a real document.
  std::filesystem::path WriteBoundDocument() {
    const std::filesystem::path name = "bound.xsde";
    std::ofstream out{public_dir_.path() / name, std::ios::binary};
    out << R"(<?xml version="1.0" encoding="UTF-8"?>
<SDEDocument Version="520">
  <metadata xmlns:dc="http://purl.org/dc/elements/1.1/">
    <dc:title>Substation 1</dc:title>
  </metadata>
  <Pages>
    <Page title="Main">
      <SDEObjects>
        <Type typ="43" ObjectType="Breaker">
          <SDE tag="1" sTag="11" origin="4 4">
            <Tech className="Breaker" closed="1" ключ_привязки="TS_Q1"/>
          </SDE>
        </Type>
        <Type typ="5" ObjectType="Text">
          <SDE tag="1" sTag="12" origin="12 4" rectWidth="8" rectHeight="2"
               inText="0000" ключ_привязки="текст=TIT_U"/>
        </Type>
      </SDEObjects>
    </Page>
  </Pages>
</SDEDocument>)";
    return name;
  }

  // Gives `formula` its own live value, as the formula service would.
  std::shared_ptr<MockTimedData> BindFormula(const std::string& formula) {
    auto data = std::make_shared<testing::NiceMock<MockTimedData>>();
    ON_CALL(controller_env_.timed_data_service_,
            GetFormulaTimedData(Eq(formula), _))
        .WillByDefault(Return(data));
    return data;
  }

  // Clicks the widget on a grid until the selection holds `spec`, and says
  // whether any click did.
  static bool ClickSelects(QWidget& widget,
                           SelectionModel& selection,
                           const TimedDataSpec& spec) {
    for (int y = 2; y < widget.height(); y += 4) {
      for (int x = 2; x < widget.width(); x += 4) {
        const QPointF point(x, y);
        QMouseEvent press(QEvent::MouseButtonPress, point, point, point,
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(&widget, &press);
        if (!selection.empty() && selection.timed_data() == spec)
          return true;
      }
    }
    return false;
  }

  // Declared before anything that opens a file inside them.
  scada::ScopedTempDir public_dir_{"scada_modus_public"};
  scada::ScopedTempDir install_dir_{"scada_modus_install"};

  AppEnvironment app_env_;
  ControllerEnvironment controller_env_;
};

// A window definition that names no document resolves to the displays folder
// itself. The widget diagnoses that before the loader sees it, because the
// loader's own complaint named a directory and talked about file extensions.
TEST_F(ModusDisplayTest, AWindowWithNoDocumentSaysSoInsteadOfRendering) {
  ModusController controller{controller_env_.MakeControllerContext()};

  std::unique_ptr<UiView> view = controller.Init(WindowDefinition{});
  ASSERT_THAT(view, NotNull());

  QWidget* display_widget = DisplayWidgetOf(*view);
  ASSERT_THAT(display_widget, NotNull());

  EXPECT_TRUE(ContainsErrorText(Render(*display_widget)))
      << "A Modus window with no document assigned rendered no message; the "
         "operator would see an empty white panel.";
}

// The document-will-not-parse case, which is what reaches a customer now that
// the renderer is linked rather than loaded: the binary is happy, the window
// opens, and only the display is empty. The fixture writes bytes that are not a
// schematic, so the parser is what fails.
TEST_F(ModusDisplayTest, AnUnreadableDocumentIsReportedInTheDisplayItself) {
  WindowDefinition definition;
  definition.path = WriteDocument("substation.sde");

  ModusController controller{controller_env_.MakeControllerContext()};

  std::unique_ptr<UiView> view = controller.Init(definition);
  ASSERT_THAT(view, NotNull());

  QWidget* display_widget = DisplayWidgetOf(*view);
  ASSERT_THAT(display_widget, NotNull());

  EXPECT_TRUE(ContainsErrorText(Render(*display_widget)))
      << "A Modus display whose document will not parse rendered no message.";
}

// `Save` round-trips through the real wrapper rather than a fake one, which is
// what pins `GetPublicFilePath`/`FullFilePathToPublic` being inverses for the
// flat case the client actually uses.
TEST_F(ModusDisplayTest, SaveReturnsThePublicPathTheDefinitionCameWith) {
  WindowDefinition definition;
  definition.path = WriteDocument("substation.sde");

  ModusController controller{controller_env_.MakeControllerContext()};
  std::unique_ptr<UiView> view = controller.Init(definition);

  WindowDefinition saved;
  controller.Save(saved);

  EXPECT_EQ(saved.path, std::filesystem::path{"substation.sde"});
}

// ── Live values through the whole chain (backlog 915) ───────────────────────

// A value change on a bound object reaches the drawing: timed-data service ->
// `ModusLiveData` -> `ModusController` -> `DisplayWidget` -> the runtime.
TEST_F(ModusDisplayTest, AValueChangeRepaintsTheBoundElements) {
  if (!HaveRuntime())
    GTEST_SKIP() << "this client was configured without a display runtime";

  std::shared_ptr<MockTimedData> breaker = BindFormula("TS_Q1");
  std::shared_ptr<MockTimedData> voltage = BindFormula("TIT_U");
  breaker->data_value_ = scada::MakeReadResult(true);
  voltage->data_value_ = scada::MakeReadResult(10.5);

  WindowDefinition definition;
  definition.path = WriteBoundDocument();
  ModusController controller{controller_env_.MakeControllerContext()};
  std::unique_ptr<UiView> view = controller.Init(definition);
  QWidget* display_widget = DisplayWidgetOf(*view);
  ASSERT_THAT(display_widget, NotNull());

  const QImage closed = Render(*display_widget);
  ASSERT_FALSE(ContainsErrorText(closed)) << "the bound document did not open";

  breaker->UpdateData(scada::MakeReadResult(false));
  const QImage open = Render(*display_widget);
  EXPECT_NE(open, closed) << "a position change did not reach the drawing";

  breaker->UpdateData(scada::MakeReadResult(true));
  EXPECT_EQ(Render(*display_widget), closed)
      << "returning to the first position did not restore the drawing";

  voltage->UpdateData(scada::MakeReadResult(42.25));
  EXPECT_NE(Render(*display_widget), closed)
      << "a value change did not reach the bound text";
}

// A click on a bound element selects the object its key names, through the
// real hit test and the formula service.
TEST_F(ModusDisplayTest, AClickOnABoundElementSelectsItsObject) {
  if (!HaveRuntime())
    GTEST_SKIP() << "this client was configured without a display runtime";

  std::shared_ptr<MockTimedData> breaker = BindFormula("TS_Q1");
  std::shared_ptr<MockTimedData> voltage = BindFormula("TIT_U");

  WindowDefinition definition;
  definition.path = WriteBoundDocument();
  ModusController controller{controller_env_.MakeControllerContext()};
  std::unique_ptr<UiView> view = controller.Init(definition);
  QWidget* display_widget = DisplayWidgetOf(*view);
  ASSERT_THAT(display_widget, NotNull());
  display_widget->resize(640, 480);

  SelectionModel& selection = *controller.GetSelectionModel();
  EXPECT_TRUE(ClickSelects(*display_widget, selection, TimedDataSpec{breaker}));
  EXPECT_TRUE(ClickSelects(*display_widget, selection, TimedDataSpec{voltage}));
}

// Backlog 910: «Мнемосхема» for an object selects the element bound to it.
TEST_F(ModusDisplayTest, ShowContainedItemSelectsTheBoundElement) {
  if (!HaveRuntime())
    GTEST_SKIP() << "this client was configured without a display runtime";

  const scada::NodeId breaker_node{42, 1};
  std::shared_ptr<MockTimedData> breaker = BindFormula("TS_Q1");
  ON_CALL(*breaker, GetNode())
      .WillByDefault(
          Return(NodeRef{breaker_node, &controller_env_.node_service_}));
  BindFormula("TIT_U");

  WindowDefinition definition;
  definition.path = WriteBoundDocument();
  ModusController controller{controller_env_.MakeControllerContext()};
  std::unique_ptr<UiView> view = controller.Init(definition);

  ASSERT_TRUE(controller.ShowContainedItem(breaker_node));
  SelectionModel& selection = *controller.GetSelectionModel();
  ASSERT_FALSE(selection.empty());
  EXPECT_EQ(selection.timed_data(), TimedDataSpec{breaker});
}

// The schematic's «Заголовок», read by the runtime, reaches the file cache the
// display menu lists from, together with the objects it is bound to.
TEST_F(ModusDisplayTest, OpeningIndexesTheDocumentsTitleAndObjects) {
  if (!HaveRuntime())
    GTEST_SKIP() << "this client was configured without a display runtime";

  const scada::NodeId breaker_node{42, 1};
  std::shared_ptr<MockTimedData> breaker = BindFormula("TS_Q1");
  ON_CALL(*breaker, GetNode())
      .WillByDefault(
          Return(NodeRef{breaker_node, &controller_env_.node_service_}));
  BindFormula("TIT_U");

  WindowDefinition definition;
  definition.path = WriteBoundDocument();
  ModusController controller{controller_env_.MakeControllerContext()};
  std::unique_ptr<UiView> view = controller.Init(definition);

  const FileCache::FileList& list =
      controller_env_.file_cache_.GetList(ID_MODUS_VIEW);
  const int index = list.Find("bound.xsde");
  ASSERT_NE(index, -1);
  EXPECT_EQ(list[index].title, u"Substation 1");
  EXPECT_TRUE(list[index].items.contains(breaker_node));
}

}  // namespace
