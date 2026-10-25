#pragma once
#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace iso8601::ast {
struct DateValue : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> century, year, month, day, ordinalDay, week, weekDay;
};
struct TimeZoneValue : cursiv::Extends<cursiv::AstNode> {
  bool utc = false;
  bool negative = false;
  optional<string_view> hour, minute;
};
struct TimeValue : cursiv::Extends<cursiv::AstNode> {
  string_view hour;
  optional<string_view> minute, second;
  TimeZoneValue *zone = nullptr;
};
struct DateTimeValue : cursiv::Extends<cursiv::AstNode> {
  DateValue *date = nullptr;
  TimeValue *time = nullptr;
};
struct Point : cursiv::Extends<cursiv::AstNode> {
  variant<DateTimeValue *, DateValue *, TimeValue *> value;
};
struct DurationParts : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> years, months, weeks, days, hours, minutes, seconds;
  bool hasTime = false;
};
struct Interval : cursiv::Extends<cursiv::AstNode> {
  variant<DurationParts *, Point *> value;
};
struct DurationRange : cursiv::Extends<cursiv::AstNode> {
  variant<Point *, Interval *> start, stop;
};
struct Duration : cursiv::Extends<cursiv::AstNode> {
  variant<Interval *, DurationRange *> value;
};
struct Recurring : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> count;
  Duration *body = nullptr;
};
struct Document : cursiv::Extends<cursiv::AstNode> {
  variant<TimeValue *, DateValue *, DateTimeValue *, Duration *, Recurring *>
      value;
};
} // namespace iso8601::ast
