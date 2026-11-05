#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include "cursiv/grammar/Conversion.hpp"

namespace javascript {

// Drops the delimiters a template-text terminal reads along with its text --
// "`" or "}" before it, "`" or "${" after it -- so the text alone reaches the
// AST.
template <std::size_t Tail>
inline constexpr auto drop_ends =
    [](std::string_view t) -> cursiv::ConversionResult<std::string> {
      using Result = cursiv::ConversionResult<std::string>;
      static_assert(Tail == 1 || Tail == 2);
      if (t.size() < 1 + Tail || (t.front() != '`' && t.front() != '}') ||
          (Tail == 1 ? t.back() != '`' : !t.ends_with("${")))
        return Result::err("A template fragment needs its opening and ending delimiters.");
      const auto body = t.substr(1, t.size() - 1 - Tail);
      if (body.find('\r') == std::string_view::npos) return Result::ok(std::string(body));
      std::string out;
      out.reserve(body.size());
      for (std::size_t i = 0; i < body.size(); ++i) {
        if (body[i] == '\r') {
          out += '\n';
          if (i + 1 < body.size() && body[i + 1] == '\n') ++i;
        } else out += body[i];
      }
      return Result::ok(std::move(out));
    };

}  // namespace javascript
