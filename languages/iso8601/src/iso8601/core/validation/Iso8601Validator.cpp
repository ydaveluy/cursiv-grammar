#include "iso8601/core/validation/Iso8601Validator.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "iso8601/core/CoreServices.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <type_traits>

namespace iso8601 {
namespace {
unsigned whole(std::string_view s) {
  unsigned n = 0;
  for (char c : s) {
    if (c < '0' || c > '9')
      break;
    n = n * 10 + (c - '0');
  }
  return n;
}
bool nonzero(std::string_view s) {
  for (char c : s)
    if (c >= '1' && c <= '9')
      return true;
  return false;
}
bool fractional(std::string_view s) {
  auto at = s.find_first_of(",.");
  return at != s.npos && nonzero(s.substr(at + 1));
}
unsigned yearCycle(std::string_view s) {
  unsigned n = 0;
  for (char c : s)
    if (c >= '0' && c <= '9')
      n = (n * 10 + (c - '0')) % 400;
  return s.starts_with('-') && n ? 400 - n : n;
}
bool leap(std::string_view s) {
  unsigned y = yearCycle(s);
  return y % 4 == 0 && (y % 100 != 0 || y == 0);
}
bool amount(const cursiv::AstNode &n) {
  for (auto *p = n.parent; p; p = p->parent)
    if (cursiv::ast_cast<ast::Interval>(p))
      return true;
  return false;
}
const ast::DateValue *dateOf(const ast::Point *p) {
  if (!p)
    return nullptr;
  if (auto *d = std::get_if<ast::DateValue *>(&p->value))
    return *d;
  if (auto *d = std::get_if<ast::DateTimeValue *>(&p->value); d && *d)
    return (*d)->date;
  return nullptr;
}
const ast::DateValue *inherited(const ast::DateValue &d) {
  for (auto *p = d.parent; p; p = p->parent)
    if (auto *r = cursiv::ast_cast<ast::DurationRange>(p)) {
      if (auto *s = std::get_if<ast::Point *>(&r->start))
        return dateOf(*s);
      break;
    }
  return nullptr;
}
bool exceeds(std::string_view value, unsigned limit) {
  return whole(value) > limit || (whole(value) == limit && fractional(value));
}

// Captures are source views. Adjacency identifies basic/extended syntax
// without reparsing the source or converting numeric payloads.
unsigned styleBetween(std::string_view left, std::string_view right,
                      std::size_t basicGap = 0) {
  const auto end = reinterpret_cast<std::uintptr_t>(left.data()) + left.size();
  return reinterpret_cast<std::uintptr_t>(right.data()) == end + basicGap ? 1u : 2u;
}
} // namespace
void Iso8601Validator::checkDate(const ast::DateValue &d,
                                 cursiv::ValidationAcceptor &a) const {
  if (amount(d)) {
    if (d.week || d.weekDay)
      a.error(d, "Alternative duration notation does not use week dates.")
          .code("iso8601.duration-week-date");
    else if ((d.month && whole(*d.month) > 12) ||
             (d.day && whole(*d.day) > 30))
      a.error(d, "Alternative duration components exceed their carry-over points.")
          .code("iso8601.duration-date");
    return;
  }
  auto year = d.year, month = d.month;
  if (const auto *start = inherited(d)) {
    if (!year)
      year = start->year;
    if (!month)
      month = start->month;
  }
  const auto m = month ? whole(*month) : 0;
  if (d.month && (m < 1 || m > 12))
    a.error(d, "Month must be between 1 and 12.")
        .property<&ast::DateValue::month>()
        .code("iso8601.month");
  if (d.day) {
    unsigned limit = 31;
    if (m >= 1 && m <= 12) {
      constexpr unsigned days[] = {31, 28, 31, 30, 31, 30,
                                   31, 31, 30, 31, 30, 31};
      limit = days[m - 1];
      if (m == 2 && (!year || leap(*year)))
        ++limit;
    }
    if (whole(*d.day) < 1 || whole(*d.day) > limit)
      a.error(d, "Day does not exist in this calendar month.")
          .property<&ast::DateValue::day>()
          .code("iso8601.day");
  }
  if (d.ordinalDay &&
      (whole(*d.ordinalDay) < 1 ||
       whole(*d.ordinalDay) > (year && !leap(*year) ? 365u : 366u)))
    a.error(d, "Ordinal day does not exist in this year.")
        .property<&ast::DateValue::ordinalDay>()
        .code("iso8601.ordinal");
  if (d.week) {
    unsigned limit = 53;
    if (year) {
      unsigned y = yearCycle(*year);
      if (!y)
        y = 400;
      unsigned prior = y - 1;
      unsigned jan1 = (365 * prior + prior / 4 - prior / 100 + prior / 400) % 7;
      if (jan1 != 3 && !(jan1 == 2 && leap(*year)))
        limit = 52;
    }
    if (whole(*d.week) < 1 || whole(*d.week) > limit)
      a.error(d, "Week does not exist in this ISO week year.")
          .property<&ast::DateValue::week>()
          .code("iso8601.week");
  }
  if (d.weekDay && (whole(*d.weekDay) < 1 || whole(*d.weekDay) > 7))
    a.error(d, "Weekday must be between 1 and 7.")
        .property<&ast::DateValue::weekDay>()
        .code("iso8601.weekday");
}
void Iso8601Validator::checkTime(const ast::TimeValue &t,
                                 cursiv::ValidationAcceptor &a) const {
  if (amount(t)) {
    if (exceeds(t.hour, 24) || (t.minute && exceeds(*t.minute, 60)) ||
        (t.second && exceeds(*t.second, 60)))
      a.error(t, "Alternative duration components exceed their carry-over points.")
          .code("iso8601.duration-clock");
    return;
  }
  const auto h = whole(t.hour), m = t.minute ? whole(*t.minute) : 0,
             s = t.second ? whole(*t.second) : 0;
  if (h > 24 ||
      (h == 24 && (fractional(t.hour) || (t.minute && nonzero(*t.minute)) ||
                   (t.second && nonzero(*t.second)))))
    a.error(t, "Hour 24 may represent midnight only.").code("iso8601.hour");
  if (t.minute && m >= 60)
    a.error(t, "Minute must be less than 60.")
        .property<&ast::TimeValue::minute>()
        .code("iso8601.minute");
  if (t.second && s > 60)
    a.error(t, "Second must be less than 61, including a possible leap second.")
        .property<&ast::TimeValue::second>()
        .code("iso8601.second");
}
void Iso8601Validator::checkZone(const ast::TimeZoneValue &z,
                                 cursiv::ValidationAcceptor &a) const {
  if (amount(z))
    a.error(z, "Alternative duration notation uses local time without a zone.")
        .code("iso8601.duration-zone");
  else if ((z.hour && whole(*z.hour) > 23) || (z.minute && whole(*z.minute) > 59))
    a.error(z, "UTC offset components are outside their range.")
        .code("iso8601.zone");
  else if (z.negative && (!z.hour || !nonzero(*z.hour)) &&
           (!z.minute || !nonzero(*z.minute)))
    a.error(z, "ISO 8601 does not use a negative zero UTC offset.")
        .code("iso8601.zone-zero");
}
void Iso8601Validator::checkDuration(const ast::DurationParts &d,
                                     cursiv::ValidationAcceptor &a) const {
  std::array values = {d.years, d.months,  d.weeks,  d.days,
                       d.hours, d.minutes, d.seconds};
  unsigned count = 0;
  bool seenFraction = false, badFraction = false;
  for (const auto &v : values)
    if (v) {
      ++count;
      if (seenFraction)
        badFraction = true;
      if (v->find_first_of(",.") != v->npos)
        seenFraction = true;
    }
  if (!count)
    a.error(d.parent ? *d.parent : static_cast<const cursiv::AstNode &>(d),
            "A duration requires at least one component.")
        .code("iso8601.duration-empty");
  else if (d.hasTime && !d.hours && !d.minutes && !d.seconds)
    a.error(d, "The T designator requires a time component.")
        .code("iso8601.duration-time");
  if (d.weeks && (count > 1 || d.hasTime))
    a.error(d, "Week durations cannot contain other components.")
        .code("iso8601.duration-weeks");
  if (badFraction)
    a.error(d, "Only the last duration component may have a fraction.")
        .code("iso8601.fraction");
}

void Iso8601Validator::checkDateTime(const ast::DateTimeValue &d,
                                    cursiv::ValidationAcceptor &a) const {
  if (d.date && !d.date->day && !d.date->ordinalDay && !d.date->weekDay)
    a.error(d, "A date combined with a time must retain its day component.")
        .code("iso8601.datetime-date");
}

void Iso8601Validator::checkDocument(const ast::Document &d,
                                    cursiv::ValidationAcceptor &a) const {
  auto styles = [](auto &&self, const auto *node) -> unsigned {
    if (!node) return 0;
    using T = std::remove_cv_t<std::remove_pointer_t<decltype(node)>>;
    if constexpr (std::is_same_v<T, ast::DateValue>) {
      if (node->year && node->month) return styleBetween(*node->year, *node->month);
      if (node->year && node->ordinalDay) return styleBetween(*node->year, *node->ordinalDay);
      if (node->year && node->week) return styleBetween(*node->year, *node->week, 1);
      if (node->month && node->day) return styleBetween(*node->month, *node->day);
      if (node->week && node->weekDay) return styleBetween(*node->week, *node->weekDay);
      return 0;
    } else if constexpr (std::is_same_v<T, ast::TimeZoneValue>) {
      return node->hour && node->minute ? styleBetween(*node->hour, *node->minute) : 0;
    } else if constexpr (std::is_same_v<T, ast::TimeValue>) {
      return (node->minute ? styleBetween(node->hour, *node->minute) : 0) | self(self, node->zone);
    } else if constexpr (std::is_same_v<T, ast::DateTimeValue>) {
      return self(self, node->date) | self(self, node->time);
    } else if constexpr (std::is_same_v<T, ast::DurationParts>) {
      return 0; // Designators do not select basic or extended notation.
    } else if constexpr (std::is_same_v<T, ast::DurationRange>) {
      const auto arm = [&](auto *x) { return self(self, x); };
      return std::visit(arm, node->start) | std::visit(arm, node->stop);
    } else if constexpr (std::is_same_v<T, ast::Recurring>) {
      return self(self, node->body);
    } else {
      return std::visit([&](auto *x) { return self(self, x); }, node->value);
    }
  };
  if (styles(styles, &d) == 3)
    a.error(d, "Basic and extended date/time formats must not be mixed.")
        .code("iso8601.format");
}
void register_validation_checks(Iso8601CoreServices &services) {
  services.validator = std::make_unique<const Iso8601Validator>(services);
  const auto &v = *services.validator;
  using V = Iso8601Validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkDate>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkTime>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkZone>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkDuration>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkDateTime>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkDocument>(v)});
}
} // namespace iso8601
