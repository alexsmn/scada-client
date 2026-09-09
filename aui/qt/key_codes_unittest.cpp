#include "aui/qt/key_codes.h"

#include <QKeySequence>
#include <gtest/gtest.h>

namespace scada::aui {
namespace {

// The one shared conversion replaces two hand-rolled copies that added the
// key and modifier enum values. Composing them as Qt defines it must give the
// same sequence Qt builds from its own operator.
TEST(KeyCodesQtTest, ToQKeySequenceComposesModifiersAndKey) {
  EXPECT_EQ(ToQKeySequence(ControlModifier, KeyCode::C),
            QKeySequence{Qt::CTRL | Qt::Key_C});
  EXPECT_EQ(ToQKeySequence(KeyModifiers{}, KeyCode::Delete),
            QKeySequence{Qt::Key_Delete});
  EXPECT_EQ(ToQKeySequence(ControlModifier | ShiftModifier, KeyCode::V),
            QKeySequence{Qt::CTRL | Qt::SHIFT | Qt::Key_V});
}

}  // namespace
}  // namespace scada::aui
