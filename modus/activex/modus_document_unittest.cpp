#include "modus/activex/modus_document.h"

#include "base/test/test_executor.h"
#include "common/aliases_mock.h"
#include "filesystem/file_cache.h"
#include "filesystem/file_registry.h"
#include "modus/activex/test/sde_form_stub.h"
#include "profile/profile.h"
#include "services/atl_module.h"
#include "timed_data/timed_data_service_mock.h"

#include <gmock/gmock.h>

DummyAtlModule _Module;

namespace modus {

using namespace testing;

class ModusDocumentTest : public Test {
 public:
  std::shared_ptr<Executor> executor_ = std::make_shared<TestExecutor>();
  StrictMock<MockAliasResolver> alias_resolver_;
  StrictMock<MockTimedDataService> timed_data_service_;
  FileRegistry file_registry_;
  FileCache file_cache_{file_registry_};
  Profile profile_;

  StrictMock<MockFunction<void(const std::u16string& title)>> title_callback_;

  StrictMock<MockFunction<void(std::u16string_view hyperlink)>>
      navigation_callback_;

  StrictMock<MockFunction<void(const TimedDataSpec& selection)>>
      selection_callback_;

  StrictMock<MockFunction<void(const aui::Point& point)>> context_menu_handler_;
  StrictMock<MockFunction<void()>> enable_internal_render_callback_;

  CComPtr<SdeFormStub> sde_form_ = CreateSdeForm();

  ModusDocument modus_document_{
      ModusDocumentContext{
          .executor_ = executor_,
          .alias_resolver_ = alias_resolver_.AsStdFunction(),
          .timed_data_service_ = timed_data_service_,
          .file_cache_ = file_cache_,
          .profile_ = profile_,
          .title_callback_ = title_callback_.AsStdFunction(),
          .navigation_callback_ = navigation_callback_.AsStdFunction(),
          .selection_callback_ = selection_callback_.AsStdFunction(),
          .context_menu_callback_ = context_menu_handler_.AsStdFunction(),
          .enable_internal_render_callback_ =
              enable_internal_render_callback_.AsStdFunction()},
      *sde_form_};

 private:
  CComPtr<SdeFormStub> CreateSdeForm();
};

CComPtr<SdeFormStub> ModusDocumentTest::CreateSdeForm() {
  CComObject<SdeFormStub>* sde_form = nullptr;
  CComObject<SdeFormStub>::CreateInstance(&sde_form);
  return sde_form;
}

TEST_F(ModusDocumentTest, Init) {
  std::filesystem::path path;
  modus_document_.Init(path, /*state=*/std::nullopt);
}

}  // namespace modus
