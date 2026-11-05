#pragma once

#include <locale.h>

#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>

namespace wkt {
inline std::optional<double> ordinate_value(std::string_view raw) {
  if (raw.empty()) return {};
  const std::string text(raw);
  char* end = nullptr;
#if defined(_WIN32)
  static const auto locale = _create_locale(LC_NUMERIC, "C");
  const double value = _strtod_l(text.c_str(), &end, locale);
#elif defined(__unix__) || defined(__APPLE__)
  static const auto locale = newlocale(LC_NUMERIC_MASK, "C", nullptr);
  const double value = strtod_l(text.c_str(), &end, locale);
#else
  const double value = std::strtod(text.c_str(), &end);
#endif
  if (end != text.c_str() + text.size()) return {};
  return value;
}
}  // namespace wkt
