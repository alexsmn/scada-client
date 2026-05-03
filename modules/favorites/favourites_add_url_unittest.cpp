#include "favorites/favourites_add_url.h"

#include "aui/dialog_service_mock.h"
#include "base/any_executor.h"
#include "base/callback_awaitable.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "favorites/favourites.h"

#include <gmock/gmock.h>

using namespace testing;
namespace {

class DeferredString {
 public:
  Awaitable<std::u16string> Wait(AnyExecutor executor) {
    auto [value] =
        co_await CallbackToAwaitable<std::u16string>(
            std::move(executor),
            [this](auto callback) { callback_ = std::move(callback); });
    co_return std::move(value);
  }

  void Resolve(std::u16string value) { callback_(std::move(value)); }

 private:
  std::function<void(std::u16string)> callback_;
};

}  // namespace

class FavouritesAddUrlTest : public Test {
 protected:
  TestExecutor executor_;
  std::shared_ptr<void> lifetime_token_ = std::make_shared<int>(0);
  NiceMock<MockDialogService> dialog_service_;
  Favourites favourites_;
};

TEST_F(FavouritesAddUrlTest, AddsPromptedUrlWhenStillAlive) {
  EXPECT_CALL(dialog_service_, RunMessageBox(_, _, MessageBoxMode::Error))
      .Times(0);
  WaitAwaitable(
      executor_,
      AddUrlToFavouritesWithPromptAsync(
          executor_, lifetime_token_, dialog_service_, favourites_,
          []() -> Awaitable<std::u16string> {
            co_return u"https://example.com";
          },
          [] { return static_cast<const FavouritesNode*>(nullptr); }));

  ASSERT_EQ(favourites_.folders().size(), 1);
  EXPECT_EQ(favourites_.folders().front().GetWindowCount(), 1);
}

TEST_F(FavouritesAddUrlTest, StopsAfterPromptWhenLifetimeExpires) {
  EXPECT_CALL(dialog_service_, RunMessageBox(_, _, MessageBoxMode::Error))
      .Times(0);

  DeferredString prompt;
  auto add = AddUrlToFavouritesWithPromptAsync(
          executor_, lifetime_token_, dialog_service_, favourites_,
          [executor = executor_, &prompt]() mutable -> Awaitable<std::u16string> {
            co_return co_await prompt.Wait(executor);
          },
          [] {
            ADD_FAILURE() << "selection should not be read after destruction";
            return static_cast<const FavouritesNode*>(nullptr);
          });
  bool completed = false;
  CoSpawn(executor_, [&add, &completed]() mutable -> Awaitable<void> {
    co_await std::move(add);
    completed = true;
  });
  Drain(executor_);

  lifetime_token_.reset();
  prompt.Resolve(u"https://example.com");
  Drain(executor_);

  EXPECT_TRUE(completed);
  EXPECT_TRUE(favourites_.folders().empty());
}
