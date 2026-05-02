#include "modus/activex/modus_document.h"

#include "base/test/test_executor.h"
#include "common/aliases_mock.h"
#include "filesystem/file_cache.h"
#include "filesystem/file_registry.h"
#include "modus/activex/test/named_pb_stub.h"
#include "modus/activex/test/param_stub.h"
#include "modus/activex/test/sde_document_stub.h"
#include "modus/activex/test/sde_form_stub.h"
#include "modus/activex/test/sde_object_stub.h"
#include "modus/activex/test/sde_page_stub.h"
#include "profile/profile.h"
#include "scada/attribute_service.h"
#include "services/atl_module.h"
#include "timed_data/timed_data_service_mock.h"

#include "base/utf_convert.h"
#include <gmock/gmock.h>

DummyAtlModule _Module;

namespace modus {

using namespace testing;

class ModusDocumentTest : public Test {
 public:
  CComPtr<SdeFormStub> CreateSdeFormOrDie();

  void InitModusDocument();

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

  std::shared_ptr<MockTimedData> timed_data_ =
      std::make_shared<testing::NiceMock<MockTimedData>>();

  CComPtr<ParamStub> state_param_ = ParamStub::CreateOrDie(L"положение", L"0");
  CComPtr<SdeFormStub> sde_form_ = CreateSdeFormOrDie();

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

  constexpr static char kFormula[] = "tag";
};

CComPtr<SdeFormStub> ModusDocumentTest::CreateSdeFormOrDie() {
  CComObject<SdeFormStub>* sde_form = nullptr;
  if (FAILED(CComObject<SdeFormStub>::CreateInstance(&sde_form))) {
    throw std::runtime_error{"Cannot create a form"};
  }

  CComObject<SdeDocumentStub>* sde_document = nullptr;
  if (FAILED(CComObject<SdeDocumentStub>::CreateInstance(&sde_document))) {
    throw std::runtime_error{"Cannot create a form"};
  }

  sde_form->sde_document_ = sde_document;

  CComObject<SdePageStub>* sde_page = nullptr;
  if (FAILED(CComObject<SdePageStub>::CreateInstance(&sde_page))) {
    throw std::runtime_error{"Cannot create a form"};
  }

  sde_document->sde_page_ = sde_page;

  CComObject<SdeObjectStub>* sde_object = nullptr;
  if (FAILED(CComObject<SdeObjectStub>::CreateInstance(&sde_object))) {
    throw std::runtime_error{"Cannot create a form"};
  }

  sde_page->objects_.emplace_back(sde_object);

  CComObject<NamedPbStub>* tech = nullptr;
  if (FAILED(CComObject<NamedPbStub>::CreateInstance(&tech))) {
    throw std::runtime_error{"Cannot create a form"};
  }

  sde_object->techs_.emplace_back(tech);

  tech->params_.emplace_back(ParamStub::CreateOrDie(
      L"ключ_привязки", UtfConvert<wchar_t>(kFormula)));

  tech->params_.emplace_back(state_param_);

  return sde_form;
}

void ModusDocumentTest::InitModusDocument() {
  EXPECT_CALL(timed_data_service_,
              GetFormulaTimedData(/*formula=*/Eq(kFormula), /*aggregation=*/_))
      .WillOnce(Return(timed_data_));

  std::filesystem::path path;
  modus_document_.Init(path, /*state=*/std::nullopt);
}

TEST_F(ModusDocumentTest, SetsInitialDataOnLoad) {
  timed_data_->data_value_ = scada::MakeReadResult(/*value=*/true);

  InitModusDocument();

  EXPECT_EQ(state_param_->value_, L"1");
}

TEST_F(ModusDocumentTest, UpdatesData) {
  InitModusDocument();

  timed_data_->UpdateData(scada::MakeReadResult(/*value=*/true));

  EXPECT_EQ(state_param_->value_, L"1");

  timed_data_->UpdateData(scada::MakeReadResult(/*value=*/false));

  EXPECT_EQ(state_param_->value_, L"0");
}

}  // namespace modus
