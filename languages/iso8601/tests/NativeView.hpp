#pragma once
#include "cursiv/core/services/JsonValue.hpp"
#include "iso8601/core/ast.hpp"
using J = cursiv::JsonValue;
namespace ia = iso8601::ast;
template <class T> J opt(const std::optional<T> &x) {
  return x ? J(*x) : J(nullptr);
}
inline J date_view(const ia::DateValue &d) {
  return J::Object{{"kind", "date"},
                   {"century", opt(d.century)},
                   {"year", opt(d.year)},
                   {"month", opt(d.month)},
                   {"day", opt(d.day)},
                   {"week", opt(d.week)},
                   {"weekDay", opt(d.weekDay)},
                   {"ordinalDay", opt(d.ordinalDay)}};
}
inline J time_view(const ia::TimeValue &t) {
  J z = nullptr;
  if (t.zone)
    z = J::Object{{"utc", t.zone->utc},
                  {"negative", t.zone->negative},
                  {"hour", opt(t.zone->hour)},
                  {"minute", opt(t.zone->minute)}};
  return J::Object{{"kind", "time"},
                   {"hour", t.hour},
                   {"minute", opt(t.minute)},
                   {"second", opt(t.second)},
                   {"zone", z}};
}
inline J dt_view(const ia::DateTimeValue &d) {
  return J::Object{{"kind", "datetime"},
                   {"date", d.date ? date_view(*d.date) : J(nullptr)},
                   {"time", d.time ? time_view(*d.time) : J(nullptr)}};
}
inline J point_view(const ia::Point &p) {
  return std::visit(
      [](auto *x) -> J {
        using T = std::decay_t<decltype(x)>;
        if (!x)
          return nullptr;
        if constexpr (std::is_same_v<T, ia::DateValue *>)
          return date_view(*x);
        else if constexpr (std::is_same_v<T, ia::TimeValue *>)
          return time_view(*x);
        else
          return dt_view(*x);
      },
      p.value);
}
inline J amount_view(const ia::Interval &i) {
  return std::visit(
      [](auto *p) -> J {
        if (!p)
          return nullptr;
        using T = std::decay_t<decltype(p)>;
        if constexpr (std::is_same_v<T, ia::DurationParts *>)
          return J::Object{
              {"kind", "amount"},           {"years", opt(p->years)},
              {"months", opt(p->months)},   {"weeks", opt(p->weeks)},
              {"days", opt(p->days)},       {"hours", opt(p->hours)},
              {"minutes", opt(p->minutes)}, {"seconds", opt(p->seconds)}};
        else {
          J years = nullptr, months = nullptr, days = nullptr, hours = nullptr,
            minutes = nullptr, seconds = nullptr;
          std::visit(
              [&](auto q) {
                using Q = std::decay_t<decltype(q)>;
                if constexpr (std::is_same_v<Q, ia::DateTimeValue *>) {
                  if (q && q->date) {
                    years = opt(q->date->year);
                    months = opt(q->date->month);
                    days = opt(q->date->day);
                  }
                  if (q && q->time) {
                    hours = q->time->hour;
                    minutes = opt(q->time->minute);
                    seconds = opt(q->time->second);
                  }
                }
              },
              p->value);
          return J::Object{{"kind", "amount"},   {"years", years},
                           {"months", months},   {"weeks", nullptr},
                           {"days", days},       {"hours", hours},
                           {"minutes", minutes}, {"seconds", seconds}};
        }
      },
      i.value);
}
inline J duration_view(const ia::Duration &d) {
  return std::visit(
      [](auto *p) -> J {
        if (!p)
          return nullptr;
        using T = std::decay_t<decltype(p)>;
        if constexpr (std::is_same_v<T, ia::Interval *>)
          return amount_view(*p);
        else {
          J start = nullptr, end = nullptr, amount = nullptr;
          std::visit(
              [&](auto q) {
                using Q = std::decay_t<decltype(q)>;
                if constexpr (std::is_same_v<Q, ia::Interval *>) {
                  if (q)
                    amount = amount_view(*q);
                } else if constexpr (std::is_same_v<Q, ia::Point *>) {
                  if (q)
                    start = point_view(*q);
                } else if (q)
                  start = dt_view(*q);
              },
              p->start);
          std::visit(
              [&](auto q) {
                using Q = std::decay_t<decltype(q)>;
                if constexpr (std::is_same_v<Q, ia::Interval *>) {
                  if (q)
                    amount = amount_view(*q);
                } else if constexpr (std::is_same_v<Q, ia::Point *>) {
                  if (q)
                    end = point_view(*q);
                } else if (q)
                  end = dt_view(*q);
              },
              p->stop);
          return J::Object{{"kind", "range"},
                           {"start", start},
                           {"end", end},
                           {"duration", amount}};
        }
      },
      d.value);
}
inline J iso_view(const ia::Document &d) {
  return std::visit(
      [](auto *p) -> J {
        if (!p)
          return nullptr;
        using T = std::decay_t<decltype(p)>;
        if constexpr (std::is_same_v<T, ia::DateValue *>)
          return date_view(*p);
        else if constexpr (std::is_same_v<T, ia::TimeValue *>)
          return time_view(*p);
        else if constexpr (std::is_same_v<T, ia::DateTimeValue *>)
          return dt_view(*p);
        else if constexpr (std::is_same_v<T, ia::Duration *>)
          return duration_view(*p);
        else
          return J::Object{
              {"kind", "repeat"},
              {"count", opt(p->count)},
              {"body", p->body ? duration_view(*p->body) : J(nullptr)}};
      },
      d.value);
}

inline J normalize_separator(const J &v) {
  if (v.isString()) {
    auto s = v.string();
    for (auto &c : s)
      if (c == ',')
        c = '.';
    return s;
  }
  if (v.isArray()) {
    J::Array a;
    for (const auto &x : v.array())
      a.push_back(normalize_separator(x));
    return a;
  }
  if (v.isObject()) {
    J::Object o;
    for (const auto &[k, x] : v.object())
      o[k] = normalize_separator(x);
    return o;
  }
  return v;
}
