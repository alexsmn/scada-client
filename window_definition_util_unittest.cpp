#include "window_definition_util.h"

#include <gmock/gmock.h>

#include "base/debug_util-inl.h"

using namespace testing;

namespace {

template <class T>
base::Value MakeListValue(std::initializer_list<T> values) {
  base::Value::ListStorage list;
  list.reserve(values.size());
  for (auto&& value : values)
    list.emplace_back(std::move(value));
  return base::Value{std::move(list)};
}

template <class T>
base::Value MakeDictValue(
    std::initializer_list<std::pair<std::string, T>> key_values) {
  base::Value::DictStorage dict;
  dict.reserve(key_values.size());
  for (auto&& [key, value] : key_values) {
    dict.try_emplace(std::move(key),
                     std::make_unique<base::Value>(std::move(value)));
  }
  return base::Value{std::move(dict)};
}

static const WindowItems kTestWindowItems = {
    {"empty", base::Value{}},
    {"int", base::Value{123}},
    {"int", base::Value{321}},
    {"string", base::Value{"text"}},
    {"string", base::Value{"more text"}},
    {"list", MakeListValue({1, 2, 3})},
    {"dict", MakeDictValue<int>({{"a", 1}, {"b", 2}, {"c", 3}})},
};

static const std::string_view kTestWindowItemsJson = R"(
    [
        { "name": "empty",  "@value": null },
        { "name": "int",    "@value": 123 },
        { "name": "int",    "@value": 321 },
        { "name": "string", "@value": "text" },
        { "name": "string", "@value": "more text" },
        { "name": "list",   "@value": [1, 2, 3] },
        { "name": "dict", "a": 1, "b": 2, "c": 3 }
    ])";

}  // namespace

TEST(FromJson, NodeId) {
  EXPECT_FALSE(FromJson<scada::NodeId>(base::Value()));
  EXPECT_FALSE(FromJson<scada::NodeId>(base::Value(123)));
  EXPECT_FALSE(FromJson<scada::NodeId>(base::Value("abcdef")));
  EXPECT_EQ(scada::NodeId(123, 1),
            FromJson<scada::NodeId>(base::Value("TS.123")));
}

TEST(ToJson, NodeId) {
  EXPECT_EQ(base::Value(), ToJson(scada::NodeId()));
  EXPECT_EQ(base::Value("TS.123"), ToJson(scada::NodeId(123, 1)));
}

TEST(FromJson, WindowItems) {
  std::unique_ptr<base::Value> json = LoadJsonFromString(kTestWindowItemsJson);
  ASSERT_TRUE(json);

  auto window_items = FromJson<WindowItems>(*json);

  EXPECT_EQ(kTestWindowItems, window_items);
}

TEST(ToJson, WindowItems) {
  auto json = ToJson(kTestWindowItems);

  std::unique_ptr<base::Value> expected_json =
      LoadJsonFromString(kTestWindowItemsJson);
  ASSERT_TRUE(expected_json);

  EXPECT_EQ(*expected_json, json);
}
