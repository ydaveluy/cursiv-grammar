#pragma once

#include <cstddef>
#include <memory_resource>
#include <string_view>
#include "cursiv/core/grammar/Conversion.hpp"

namespace javascript {

// Drops the delimiters a template-text terminal reads along with its text --
// "`" or "}" before it, "`" or "${" after it -- so the text alone reaches the
// AST.
template <std::size_t Tail>
inline constexpr auto drop_ends =
    [](std::string_view t, std::pmr::memory_resource* memory) -> cursiv::ConversionResult<std::string_view> {
      using Result = cursiv::ConversionResult<std::string_view>;
      static_assert(Tail == 1 || Tail == 2);
      if (t.size() < 1 + Tail || (t.front() != '`' && t.front() != '}') ||
          (Tail == 1 ? t.back() != '`' : !t.ends_with("${")))
        return Result::err("A template fragment needs its opening and ending delimiters.");
      const auto body = t.substr(1, t.size() - 1 - Tail);
      if (body.find('\r') == std::string_view::npos) return Result::ok(body);
      const auto room = cursiv::text_room(memory, body.size());
      std::size_t size = 0;
      for (std::size_t i = 0; i < body.size(); ++i) {
        if (body[i] == '\r') {
          room[size++] = '\n';
          if (i + 1 < body.size() && body[i + 1] == '\n') ++i;
        } else room[size++] = body[i];
      }
      return Result::ok(std::string_view(room.data(), size));
    };

}  // namespace javascript
