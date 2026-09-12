#include "aui/test/app_environment.h"
#include "base/client_paths.h"
#include "base/path_service.h"
#include "common/test/scoped_temp_dir.h"
#include "controller/test/controller_environment.h"
#include "modus/qt/modus_controller.h"
#include "profile/window_definition.h"

#include <QColor>
#include <QImage>
#include <QScrollArea>
#include <QWidget>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fstream>
#include <memory>

namespace {

using testing::NotNull;

// Drives the *production* display view — `ModusController` with no injected
// factory, so a real `ModusDisplayView` over a real `DisplayWidget` over the
// linked `display` renderer. Its sibling `modus_controller_unittest.cpp`
// substitutes a fake wrapper and tests the controller's own logic; this file is
// here for the part that a fake cannot show, which is what the operator ends up
// looking at.
//
// Note what is deliberately NOT tested here: whether the renderer draws a
// document correctly. That belongs to the `display` product, which owns the
// parsers and the renderer backends and tests them against its own goldens.
// `client/` is a separate product (ADR 0011); its share of the contract is the
// wiring and the failure surface.
//
// The failure surface is the valuable half, because it is the one that ships
// broken without anyone noticing. Until ADR 0012 phase 4 the failure this file
// pinned was a *missing shared library*: the renderer was `dlopen`ed from the
// install directory, so an install that omitted it produced a client that
// started, opened a Modus display and showed the operator a blank window. That
// cannot happen any more — the renderer is linked in, so the only way to get a
// display with nothing in it is a document that will not open. Both remaining
// cases below are that: no document named, and a document that will not parse.
// What the window must never be, in either, is blank.

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

}  // namespace
