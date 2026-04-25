#include "favorites/favourites_add_url.h"

#include "aui/dialog_service_mock.h"
#include "base/awaitable_promise.h"
#include "base/executor_conversions.h"
#include "base/test/test_executor.h"
#include "favorites/favourites.h"

#include <gmock/gmock.h>

using namespace testing;
using namespace std::chrono_literals;

namespace {

void WaitForPromise(std::shared_ptr<TestExecutor> executor, promise<> promise) {
  while (promise.wait_for(1ms) == promise_wait_status::timeout) {
    executor->Poll();
  }

  promise.get();
}

}  // namespace

class FavouritesAddUrlTest : public Test {
 protected:
  std::shared_ptr<TestExecutor> executor_ = std::make_shared<TestExecutor>();
  std::shared_ptr<void> lifetime_token_ = std::make_shared<int>(0);
  NiceMock<MockDialogService> dialog_service_;
  Favourites favourites_;
};

TEST_F(FavouritesAddUrlTest, AddsPromptedUrlWhenStillAlive) {
  EXPECT_CALL(dialog_service_, RunMessageBox(_, _, MessageBoxMode::Error))
      .Times(0);
  auto add_promise = ToPromise(
      MakeAnyExecutor(executor_),
      AddUrlToFavouritesWithPromptAsync(
          executor_, lifetime_token_, dialog_service_, favourites_,
          [] { return make_resolved_promise<std::u16string>(
                   u"https://example.com"); },
          [] { return static_cast<const FavouritesNode*>(nullptr); }));

  WaitForPromise(executor_, std::move(add_promise));

  ASSERT_EQ(favourites_.folders().size(), 1);
  EXPECT_EQ(favourites_.folders().front().GetWindowCount(), 1);
}

TEST_F(FavouritesAddUrlTest, StopsAfterPromptWhenLifetimeExpires) {
  EXPECT_CALL(dialog_service_, RunMessageBox(_, _, MessageBoxMode::Error))
      .Times(0);

  promise<std::u16string> prompt_promise;
  auto add_promise = ToPromise(
      MakeAnyExecutor(executor_),
      AddUrlToFavouritesWithPromptAsync(
          executor_, lifetime_token_, dialog_service_, favourites_,
          [prompt_promise]() mutable { return std::move(prompt_promise); },
          [] {
            ADD_FAILURE() << "selection should not be read after destruction";
            return static_cast<const FavouritesNode*>(nullptr);
          }));

  lifetime_token_.reset();
  prompt_promise.resolve(u"https://example.com");
  WaitForPromise(executor_, std::move(add_promise));

  EXPECT_TRUE(favourites_.folders().empty());
}
