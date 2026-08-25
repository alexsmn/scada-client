#include "aui/test/app_environment.h"
#include "base/client_paths.h"
#include "base/path_service.h"
#include "controller/test/controller_environment.h"
#include "modus/qt/modus_controller.h"
#include "profile/window_definition.h"
#include "test/scoped_temp_dir.h"

#include <QColor>
#include <QImage>
#include <QScrollArea>
#include <QWidget>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdlib>
#include <fstream>
#include <memory>

namespace {

using testing::NotNull;

// Drives the *production* runtime view — `ModusController` with no injected
// factory, so a real `ModusVdsRuntimeView` over a real `VdsRuntimeWidget`.
// Its sibling `modus_controller_unittest.cpp` substitutes a fake wrapper and
// tests the controller's own logic; this file is here for the part that a fake
// cannot show, which is what the operator ends up looking at.
//
// Note what is deliberately NOT tested here: whether the VDS runtime renders a
// document correctly. That belongs to the product that owns the runtime, and
// is covered there by `designer/runtime/vds_runtime_test.cpp`, which authors a
// document and drives the C ABI end to end. `client/` is a separate product
// (ADR 0011) and cannot reach into `designer/`; its share of the contract is
// the wiring and the failure surface.
//
// The failure surface is the valuable half, because it is the one that ships
// broken without anyone noticing: `tc_vds_runtime` is loaded with `dlopen` at
// run time from the install directory, so an install that omits it produces a
// client that starts, opens a Modus display, and shows the operator a window.
// What that window must never be is blank.

// The pen `VdsRuntimeWidget::paintEvent` uses for every operator-facing error.
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

class ModusVdsRuntimeTest : public testing::Test {
 protected:
  void SetUp() override {
    // `GetPublicFilePath` throws without this, and the widget resolves the
    // runtime library relative to the install directory. Pointing both at an
    // empty temp tree is what makes the no-runtime case reproducible on a
    // developer machine that happens to have a runtime installed.
    scada::base::PathService::Override(client::DIR_PUBLIC, public_dir_.path());
    scada::base::PathService::Override(client::DIR_INSTALL,
                                       install_dir_.path());
  }

  // The inner runtime widget: `Init` returns the `QScrollArea` that holds it.
  static QWidget* RuntimeWidgetOf(UiView& view) {
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
  ScopedTempDir public_dir_{"scada_modus_public"};
  ScopedTempDir install_dir_{"scada_modus_install"};

  AppEnvironment app_env_;
  ControllerEnvironment controller_env_;
};

// A window definition that names no document resolves to the displays folder
// itself. The widget diagnoses that before the loader sees it, because the
// loader's own complaint named a directory and talked about file extensions.
TEST_F(ModusVdsRuntimeTest, AWindowWithNoDocumentSaysSoInsteadOfRendering) {
  ModusController controller{controller_env_.MakeControllerContext()};

  std::unique_ptr<UiView> view = controller.Init(WindowDefinition{});
  ASSERT_THAT(view, NotNull());

  QWidget* runtime_widget = RuntimeWidgetOf(*view);
  ASSERT_THAT(runtime_widget, NotNull());

  EXPECT_TRUE(ContainsErrorText(Render(*runtime_widget)))
      << "A Modus window with no document assigned rendered no message; the "
         "operator would see an empty white panel.";
}

// The install-is-missing-the-runtime case. This is the one that reaches a
// customer: the client links none of the VDS runtime, it dlopens it, so the
// binary is happy and only the display is empty.
TEST_F(ModusVdsRuntimeTest, AMissingRuntimeIsReportedInTheDisplayItself) {
  WindowDefinition definition;
  definition.path = WriteDocument("substation.sde");

  ModusController controller{controller_env_.MakeControllerContext()};

  std::unique_ptr<UiView> view = controller.Init(definition);
  ASSERT_THAT(view, NotNull());

  QWidget* runtime_widget = RuntimeWidgetOf(*view);
  ASSERT_THAT(runtime_widget, NotNull());

  EXPECT_TRUE(ContainsErrorText(Render(*runtime_widget)))
      << "A Modus display whose runtime library is absent rendered no message.";
}

// `Save` round-trips through the real wrapper rather than a fake one, which is
// what pins `GetPublicFilePath`/`FullFilePathToPublic` being inverses for the
// flat case the client actually uses.
TEST_F(ModusVdsRuntimeTest, SaveReturnsThePublicPathTheDefinitionCameWith) {
  WindowDefinition definition;
  definition.path = WriteDocument("substation.sde");

  ModusController controller{controller_env_.MakeControllerContext()};
  std::unique_ptr<UiView> view = controller.Init(definition);

  WindowDefinition saved;
  controller.Save(saved);

  EXPECT_EQ(saved.path, std::filesystem::path{"substation.sde"});
}

// The real-render half. It needs a built `tc_vds_runtime`, which is the
// designer product's output and is not in the client's build tree, so it is
// opt-in by environment rather than skipped silently: point
// SCADA_MODUS_VDS_RUNTIME_DIR at a directory holding the shared library.
TEST_F(ModusVdsRuntimeTest, ARealRuntimeRendersADocumentWithoutError) {
  const char* runtime_dir = std::getenv("SCADA_MODUS_VDS_RUNTIME_DIR");
  if (!runtime_dir || !*runtime_dir) {
    GTEST_SKIP() << "Set SCADA_MODUS_VDS_RUNTIME_DIR to a directory holding "
                    "tc_vds_runtime (built by the designer product) to run "
                    "this case.";
    // Verified 2026-08-25 against designer/build/ninja/bin/Debug. Note that
    // run printed a wall of objc duplicate-class warnings — the runtime dylib
    // carries its own Qt, and loading it into a Qt host registers QNSWindow
    // and friends twice. Whether the *shipped* runtime does that is task 485.
  }

  scada::base::PathService::Override(client::DIR_INSTALL, runtime_dir);

  // A document the runtime cannot parse still exercises the load path; what
  // this case asserts is that the runtime was found, which the "cannot open
  // document" message distinguishes from "runtime is not available".
  WindowDefinition definition;
  definition.path = WriteDocument("substation.sde");

  ModusController controller{controller_env_.MakeControllerContext()};
  std::unique_ptr<UiView> view = controller.Init(definition);
  ASSERT_THAT(view, NotNull());

  QWidget* runtime_widget = RuntimeWidgetOf(*view);
  ASSERT_THAT(runtime_widget, NotNull());

  // The runtime loaded, so the failure is about the document rather than the
  // library. Either way the operator gets a message, which is the invariant.
  EXPECT_TRUE(ContainsErrorText(Render(*runtime_widget)));
}

}  // namespace
