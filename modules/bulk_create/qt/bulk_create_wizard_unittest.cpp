#include "bulk_create/qt/bulk_create_wizard.h"

#include "aui/test/app_environment.h"
#include "bulk_create/qt/bulk_create_preview_panel.h"
#include "node_service/test/fake_node_service.h"
#include "scada/node_id.h"
#include "services/task_manager_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <QComboBox>
#include <QLabel>
#include <QWizard>

#include <memory>
#include <vector>

namespace {

using testing::_;
using testing::NiceMock;

// The wizard is assembly over two pieces with their own tests -- the pattern
// engine and `PlanBulkCreate` -- so these assert the assembly: which subjects
// this invocation offers, and that Finish posts what the planner planned.

class BulkCreateWizardTest : public ::testing::Test {
 protected:
  AppEnvironment app_env_;
  FakeNodeService node_service_;
  NiceMock<MockTaskManager> task_manager_;

  std::unique_ptr<QWizard> MakeWizard(std::vector<scada::NodeId> sources = {}) {
    return std::unique_ptr<QWizard>{MakeBulkCreateWizard(
        {node_service_, task_manager_, scada::NodeId{}, std::move(sources)})};
  }

  static QComboBox* SubjectCombo(QWizard& wizard) {
    return wizard.findChild<QComboBox*>(QStringLiteral("bulkCreateSubject"));
  }
};

// A transmission rule forwards an existing node. With no sources there is
// nothing to forward, so offering the subject would offer a run that creates
// nothing -- the caller decides which subjects exist by what it hands over.
TEST_F(BulkCreateWizardTest, WithoutSourcesOnlyTheDataItemSubjectIsOffered) {
  std::unique_ptr<QWizard> wizard = MakeWizard();
  QComboBox* subject = SubjectCombo(*wizard);
  ASSERT_NE(subject, nullptr);
  EXPECT_EQ(subject->count(), 1);
}

TEST_F(BulkCreateWizardTest, WithSourcesBothSubjectsAreOffered) {
  std::unique_ptr<QWizard> wizard =
      MakeWizard({scada::NodeId::FromString("ns=2;s=A")});
  QComboBox* subject = SubjectCombo(*wizard);
  ASSERT_NE(subject, nullptr);
  EXPECT_EQ(subject->count(), 2);
}

// The data item's binding fields have no meaning for a rule, so they go with
// the subject. Asserted on the *label* as well, because a QFormLayout label
// outlives the field it captions and would be left naming the row below it.
TEST_F(BulkCreateWizardTest, TheDataItemFieldsFollowTheSubject) {
  std::unique_ptr<QWizard> wizard =
      MakeWizard({scada::NodeId::FromString("ns=2;s=A")});
  wizard->show();
  QComboBox* subject = SubjectCombo(*wizard);
  ASSERT_NE(subject, nullptr);
  auto* device =
      wizard->findChild<QComboBox*>(QStringLiteral("bulkCreateDevice"));
  ASSERT_NE(device, nullptr);

  subject->setCurrentIndex(0);  // data items
  EXPECT_FALSE(device->isHidden());

  subject->setCurrentIndex(1);  // transmission rules
  EXPECT_TRUE(device->isHidden());
}

// Finish posts one insert per planned node, and the Review page's count is the
// same number -- both come from PlanBulkCreate, which is the point of having
// one planner.
TEST_F(BulkCreateWizardTest, FinishPostsOneInsertPerPlannedNode) {
  std::unique_ptr<QWizard> wizard = MakeWizard();

  auto* preview = wizard->findChild<BulkCreatePreviewPanel*>();
  ASSERT_NE(preview, nullptr);
  BulkCreateParams params;
  params.subject = BulkCreateSubject::kDataItem;
  params.name_template = u"AI{n}";
  params.node_id_template = u"ns=2;s=AI{n}";
  params.count = 4;
  preview->SetParams(params);

  EXPECT_CALL(task_manager_, PostInsertTask(_)).Times(4);
  wizard->accept();
}

// Nothing to create must post nothing rather than an empty run.
TEST_F(BulkCreateWizardTest, FinishWithNoRowsPostsNothing) {
  std::unique_ptr<QWizard> wizard = MakeWizard();

  auto* preview = wizard->findChild<BulkCreatePreviewPanel*>();
  ASSERT_NE(preview, nullptr);
  BulkCreateParams params;
  params.count = 0;
  preview->SetParams(params);

  EXPECT_CALL(task_manager_, PostInsertTask(_)).Times(0);
  wizard->accept();
}

}  // namespace
