#include "yara/core/Integer.hpp"
#include <charconv>
namespace yara {
std::optional<std::int64_t> integerValue(std::string_view text) {
  const bool negative = text.starts_with('-');
  if (negative) {
    text.remove_prefix(1);
    for (;;) {
      auto next = text.find_first_not_of(" \t\r\n");
      if (next == text.npos)
        return {};
      text.remove_prefix(next);
      if (text.starts_with("/*")) {
        auto end = text.find("*/", 2);
        if (end == text.npos)
          return {};
        text.remove_prefix(end + 2);
      } else if (text.starts_with("//")) {
        auto end = text.find_first_of("\r\n", 2);
        if (end == text.npos)
          return {};
        text.remove_prefix(end);
      } else
        break;
    }
  }
  std::uint64_t scale = 1;
  if (text.ends_with("KB")) {
    scale = 1024;
    text.remove_suffix(2);
  } else if (text.ends_with("MB")) {
    scale = 1048576;
    text.remove_suffix(2);
  }
  int base = 10;
  if (text.starts_with("0x")) {
    base = 16;
    text.remove_prefix(2);
  } else if (text.starts_with("0o")) {
    base = 8;
    text.remove_prefix(2);
  }
  if (text.empty())
    return {};
  std::uint64_t value = 0;
  auto [end, error] =
      std::from_chars(text.data(), text.data() + text.size(), value, base);
  if (error != std::errc{} || end != text.data() + text.size() ||
      value > std::uint64_t(INT64_MAX) / scale)
    return {};
  auto result = std::int64_t(value * scale);
  return negative ? -result : result;
}
} // namespace yara
