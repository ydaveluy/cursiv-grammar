#pragma once

#include <charconv>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>

#include "lrc/core/ast.hpp"

namespace lrc {

inline constexpr std::string_view whitespace[]{
    "\011",         "\012",         "\013",         "\014",
    "\015",         "\034",         "\035",         "\036",
    "\037",         "\040",         "\302\205",     "\302\240",
    "\341\232\200", "\342\200\200", "\342\200\201", "\342\200\202",
    "\342\200\203", "\342\200\204", "\342\200\205", "\342\200\206",
    "\342\200\207", "\342\200\210", "\342\200\211", "\342\200\212",
    "\342\200\250", "\342\200\251", "\342\200\257", "\342\201\237",
    "\343\200\200"};

inline std::string_view trim_left(std::string_view raw) {
  bool changed = true;
  while (changed) {
    changed = false;
    for (auto space : whitespace)
      if (raw.starts_with(space)) {
        raw.remove_prefix(space.size());
        changed = true;
        break;
      }
  }
  return raw;
}

inline std::string_view trim(std::string_view raw) {
  raw = trim_left(raw);
  bool changed = true;
  while (changed) {
    changed = false;
    for (auto space : whitespace)
      if (raw.ends_with(space)) {
        raw.remove_suffix(space.size());
        changed = true;
        break;
      }
  }
  return raw;
}

inline std::optional<std::int64_t> integer(std::string_view raw) {
  raw = trim(raw);
  if (raw.starts_with('+'))
    raw.remove_prefix(1);
  if (raw.empty())
    return std::nullopt;
  std::int64_t value = 0;
  const auto parsed =
      std::from_chars(raw.data(), raw.data() + raw.size(), value);
  if (parsed.ec != std::errc{} || parsed.ptr != raw.data() + raw.size())
    return std::nullopt;
  return value;
}

inline bool fraction_digits(std::string_view raw) {
  return !raw.empty() && raw.find_first_not_of("0123456789") == raw.npos;
}

inline std::optional<std::int64_t> unsigned_integer(std::string_view raw) {
  return fraction_digits(raw) ? integer(raw) : std::nullopt;
}

inline std::optional<std::int64_t> milliseconds(const ast::Timestamp &stamp) {
  const auto minutes = unsigned_integer(stamp.minutes),
             seconds = unsigned_integer(stamp.seconds);
  if (!minutes || *minutes < 0 || !seconds || *seconds < 0 || *seconds >= 60 ||
      (stamp.fraction && !fraction_digits(*stamp.fraction)))
    return std::nullopt;
  std::int64_t fraction = 0;
  if (stamp.fraction)
    for (std::size_t i = 0; i < 3; ++i)
      fraction = fraction * 10 +
                 (i < stamp.fraction->size() ? (*stamp.fraction)[i] - '0' : 0);
  const auto remainder = *seconds * 1000 + fraction;
  if (*minutes > (std::numeric_limits<std::int64_t>::max() - remainder) / 60000)
    return std::nullopt;
  return *minutes * 60000 + remainder;
}

} // namespace lrc
