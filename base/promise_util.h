#pragma once

#include "base/promise.h"
#include "base/range_util.h"

#include <boost/range/adaptor/indexed.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <unordered_map>
#include <vector>

template <class Inputs, class Mapper, class Reducer>
inline auto MakeGrouppedPromise(const Inputs& inputs,
                                const Mapper& mapper,
                                const Reducer& reducer) {
  using Input = std::remove_const_t<element_type_t<Inputs>>;
  using Key = std::invoke_result_t<Mapper, Input>;
  using ResultsPromise = std::invoke_result_t<Reducer, Key, std::vector<Input>>;
  using Results = typename ResultsPromise::value_type;
  using Result = std::remove_const_t<element_type_t<Results>>;

  struct Group {
    std::vector<size_t> indexes;
    std::vector<Input> inputs;
  };

  std::vector<Group> groups;
  std::unordered_map<Key, size_t> group_mapping;

  for (auto&& indexed_input : inputs | boost::adaptors::indexed{}) {
    auto index = indexed_input.index();
    auto&& input = std::move(indexed_input.value());
    const auto& key = mapper(input);
    if (auto i = group_mapping.find(key); i != group_mapping.end()) {
      auto group_index = i->second;
      auto& group = groups[group_index];
      group.indexes.emplace_back(index);
      group.inputs.emplace_back(std::move(input));
    }
  }

  auto promises = group_mapping | boost::adaptors::transformed([&](auto& p) {
                    const auto& group_key = p.first;
                    Group& group = groups[p.second];
                    return reducer(group_key, std::move(group.inputs));
                  });

  return make_all_promise(promises).then(
      [result_size = inputs.size(), groups = std::move(groups)](
          const std::vector<Results>& promise_results) {
        assert(groups.size() == promise_results.size());
        std::vector<Result> results(result_size);
        for (size_t i = 0; i < groups.size(); ++i) {
          const Group& group = groups[i];
          const std::vector<size_t>& group_indexes = group.indexes;
          const auto& group_results = promise_results[i];
          assert(group_indexes.size() == group_results.size());
          for (size_t j = 0; j < group_results.size(); ++j)
            results[group_indexes[j]] = group_results[j];
        }
        return results;
      });
}
