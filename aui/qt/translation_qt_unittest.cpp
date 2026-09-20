#include "aui/translation.h"

#include "aui/test/app_environment.h"

#include <QCoreApplication>
#include <QLocale>
#include <QSettings>
#include <QTemporaryDir>
#include <gtest/gtest.h>

#include <string>

namespace {

// `UiLocaleName()` reads QSettings, so the fixture redirects QSettings at a
// temporary directory for the duration of the test rather than reading — or
// writing — the developer's real preferences. The previous format, scope path
// and identity are restored afterwards so the rest of the binary is unaffected.
class UiLocaleNameTest : public testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(settings_dir_.isValid());
    previous_organization_ = QCoreApplication::organizationName();
    previous_application_ = QCoreApplication::applicationName();
    QCoreApplication::setOrganizationName("ScadaClientTest");
    QCoreApplication::setApplicationName("UiLocaleNameTest");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       settings_dir_.path());
    QSettings{}.clear();
  }

  void TearDown() override {
    QSettings{}.clear();
    QCoreApplication::setOrganizationName(previous_organization_);
    QCoreApplication::setApplicationName(previous_application_);
    QSettings::setDefaultFormat(QSettings::NativeFormat);
  }

  AppEnvironment app_env_;
  QTemporaryDir settings_dir_;
  QString previous_organization_;
  QString previous_application_;
};

TEST_F(UiLocaleNameTest, FallsBackToTheSystemLanguage) {
  // No stored choice: the client displays the OS language, so that is the
  // language it must ask the server for.
  EXPECT_EQ(QLocale::system().bcp47Name().toStdString(), UiLocaleName());
}

TEST_F(UiLocaleNameTest, PrefersTheStoredChoice) {
  // The operator picked a language in Settings; that choice governs both what
  // the client renders and what it asks the server for.
  QSettings{}.setValue("LocaleName", "en");
  EXPECT_EQ(std::string{"en"}, UiLocaleName());
}

TEST_F(UiLocaleNameTest, ReadsTheSettingUsedByTheTranslatorInstaller) {
  // The key is shared with InstalledTranslation, which decides which .qm
  // files are installed. Reading a different key would let the window's
  // language and the server's answer disagree, which is the whole defect this
  // exists to prevent.
  QSettings{}.setValue("LocaleName", "ru");
  EXPECT_EQ(std::string{"ru"}, UiLocaleName());
}

TEST_F(UiLocaleNameTest, KeepsARegionalTagIntact) {
  // A region-qualified tag is meaningful to locale negotiation: Part 4 §5.4
  // lets the server answer "en-GB" with an "en" translation, but only the
  // client may say which it prefers.
  QSettings{}.setValue("LocaleName", "en-GB");
  EXPECT_EQ(std::string{"en-GB"}, UiLocaleName());
}

TEST_F(UiLocaleNameTest, AnEmptyStoredChoiceIsNotAChoice) {
  // A cleared setting must fall through to the system language rather than
  // becoming an empty locale id.
  QSettings{}.setValue("LocaleName", "");
  EXPECT_EQ(QLocale::system().bcp47Name().toStdString(), UiLocaleName());
}

}  // namespace
