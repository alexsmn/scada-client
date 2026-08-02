#pragma once

#include "base/check.h"
#include "base/ostream_formatter.h"
#include "scada/date_time.h"
#include "scada/date_time_range.h"

#include <ostream>
#include <string>
#include <string_view>

namespace scada {

// A relative / named time range (Day, Week, Month, a fixed Interval, or an
// explicit Custom [start, end]). It resolves against a reference "now" into a
// concrete TimeRange (from/to) via ToTimeRange().
struct RelativeTimeRange {
  // `Day` might be merged into `Interval`, but it's kept for backward
  // compatibility.
  enum class Type { Custom, Interval, Day, Week, Month, Count };

  RelativeTimeRange() {}

  RelativeTimeRange(Type type) : type{type} {
    base::Check(type != Type::Custom && type != Type::Interval &&
                type != Type::Count);
  }

  RelativeTimeRange(Duration interval)
      : type{Type::Interval}, interval{interval} {
    base::Check(interval != Duration::zero());
  }

  RelativeTimeRange(Time start, Time end, bool dates = false)
      : type{Type::Custom}, start{start}, end{end}, dates{dates} {}

  bool is_interval() const { return type == Type::Interval; }
  bool is_custom() const { return type == Type::Custom; }

  bool operator==(const RelativeTimeRange& other) const = default;

  Type type = Type::Day;

  // Only when `type == Type::Custom`.
  Time start = kNullTime;
  Time end = kNullTime;
  bool dates = false;

  // Only when `type == Type::Interval`.
  Duration interval = Duration::zero();
};

// Resolves the relative range against `now` into a concrete [from, to] interval.
TimeRange ToTimeRange(const RelativeTimeRange& range, Time now);
// As ToTimeRange, but leaves the end open (kMaxTime) for an unbounded range.
TimeRange ToTimeRangeWithOpenRange(const RelativeTimeRange& range, Time now);

RelativeTimeRange::Type ParseTimeRangeType(std::string_view str);

// In scada:: so ADL finds these for `stream << range` (e.g. from StructWriter).
std::ostream& operator<<(std::ostream& stream, RelativeTimeRange::Type type);
std::ostream& operator<<(std::ostream& stream, const RelativeTimeRange& range);

}  // namespace scada

// Declared at global scope, like ToString(AttributeId) / ToString(Status) etc.:
// putting these in namespace scada would hide the other global ToString
// overloads from unqualified calls made inside namespace scada.
std::string ToString(scada::RelativeTimeRange::Type type);
std::string ToString(const scada::RelativeTimeRange& range);

// std::format support (used by base::AsOpt / AsList element rendering),
// delegating to the operator<< overloads above.
template <>
struct std::formatter<scada::RelativeTimeRange::Type> : OStreamFormatter {};

template <>
struct std::formatter<scada::RelativeTimeRange> : OStreamFormatter {};
