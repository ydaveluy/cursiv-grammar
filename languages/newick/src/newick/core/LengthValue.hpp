#pragma once

#include <charconv>
#include <limits>
#include <locale>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

namespace newick {
inline std::optional<double> length_value(std::string_view text) {
  constexpr std::string_view whitespace = " \t\r\n\f\v";
  const auto begin = text.find_first_not_of(whitespace);
  if (begin == std::string_view::npos) return std::nullopt;
  text = text.substr(begin, text.find_last_not_of(whitespace) - begin + 1);
  std::string normalized;
  if (text.find('_') != std::string_view::npos) {
    for (std::size_t i = 0; i < text.size(); ++i) {
      if (text[i] != '_')
        normalized += text[i];
      else if (i == 0 || i + 1 == text.size() || text[i - 1] < '0' ||
               text[i - 1] > '9' || text[i + 1] < '0' || text[i + 1] > '9')
        return std::nullopt;
    }
    text = normalized;
  }
  if (text.starts_with('+')) text.remove_prefix(1);
  if (text.empty()) return std::nullopt;
  auto magnitude = text;
  if (magnitude.starts_with('-')) magnitude.remove_prefix(1);
  if (!magnitude.empty() &&
      ((magnitude.front() >= 'a' && magnitude.front() <= 'z') ||
       (magnitude.front() >= 'A' && magnitude.front() <= 'Z'))) {
    std::string word(magnitude);
    for (char& c : word)
      if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
    if (word != "inf" && word != "infinity" && word != "nan")
      return std::nullopt;
  }
  double value = 0;
  const auto [end, ec] =
      std::from_chars(text.data(), text.data() + text.size(), value);
  if (end != text.data() + text.size()) return std::nullopt;
  if (ec == std::errc::result_out_of_range) {
    std::istringstream input{std::string(text)};
    input.imbue(std::locale::classic());
    input >> value;
    if (value == std::numeric_limits<double>::max())
      value = std::numeric_limits<double>::infinity();
    else if (value == -std::numeric_limits<double>::max())
      value = -std::numeric_limits<double>::infinity();
  } else if (ec != std::errc{})
    return std::nullopt;
  return value;
}
}  // namespace newick
