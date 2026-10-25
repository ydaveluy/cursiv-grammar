#pragma once

#include <cstdio>
#include <initializer_list>
#include <string_view>
#include "cursiv/core/parser/Parser.hpp"
#include "graphemes/core/ast.hpp"
#include "graphemes/generated/Graphemes/Module.hpp"

namespace native_regressions {
inline bool run() {
  using namespace std::literals;
  auto parser = graphemes::generated::Graphemes::create_parser();
  bool ok = true;
  const auto check = [&](std::string_view text, std::initializer_list<std::string_view> expected) {
    const auto parsed = parser->parse(text);
    const auto* document = cursiv::ast_cast<graphemes::ast::Document>(parsed.value);
    bool same = parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty() &&
                document && document->clusters.size() == expected.size();
    if (same) {
      std::size_t index = 0;
      for (const auto cluster : expected) same &= document->clusters[index++]->text == cluster;
    }
    if (!same) { std::fprintf(stderr, "Unicode regression: incorrect grapheme boundaries\n"); ok = false; }
  };
  check("", {});
  check("\r\ń", {"\r\n", "́"});
  check("\0a\ń"sv, {"\0"sv, "a", "\n", "́"});
  check("a‍b", {"a‍", "b"});
  check("©̀‍©", {"©̀‍©"});
  check("❤ः‍❤", {"❤ः‍", "❤"});
  check("क्‍क", {"क्‍क"});
  check("क्a", {"क्", "a"});
  check("؀؀", {"؀؀"});
  check("🇦🇧🇨🇩🇪", {"🇦🇧", "🇨🇩", "🇪"});
  check("ᄀᅠᆨ", {"ᄀᅠᆨ"});
  return ok;
}
}  // namespace native_regressions
