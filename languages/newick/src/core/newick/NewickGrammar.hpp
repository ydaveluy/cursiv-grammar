#pragma once

#include <string>

#include "cursiv/grammar/GrammarSpec.hpp"
#include "newick/ast.hpp"

namespace newick {
struct Newick : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct COMMENT
      : terminal<lit<"["> + many(ref<COMMENT> | cr<"^[]">) +
                 lit<"]">> {};

  inline static constexpr auto decode_label =
      [](std::string_view text) -> std::string {
        const bool quoted = text.starts_with("'");
        if (quoted) text = text.substr(1, text.size() - 2);
        if (quoted ? text.find("''") == std::string_view::npos
                   : text.find_first_of("_[") == std::string_view::npos)
          return std::string(text);
        std::string out;
        out.reserve(text.size());
        unsigned depth = 0;
        for (std::size_t i = 0; i < text.size(); ++i) {
          const char c = text[i];
          if (!quoted && c == '[') {
            ++depth;
            continue;
          }
          if (!quoted && c == ']' && depth) {
            --depth;
            continue;
          }
          if (depth) continue;
          out += !quoted && c == '_' ? ' ' : c;
          if (quoted && c == '\'' && i + 1 < text.size() && text[i + 1] == '\'')
            ++i;
        }
        return out;
      };
  struct Label : terminal<as<decode_label>(
                     (cr<R"(^()[],:; \t\r\n')"> +
                      many(cr<R"(^()[],:; \t\r\n)"> | ref<COMMENT>)) |
                     (lit<"'"> + many(lit<"''"> | cr<"^'">) + lit<"'">))> {};

  static constexpr auto name =
      flag<&ast::Clade::hasName>(assign<&ast::Clade::name>(ref<Label>));
  static constexpr auto length =
      lit<":"> + assign<&ast::Clade::length>(ref<Label>);
  struct Clade
      : rule<create<ast::Clade> +
             option(
                 lit<"("> +
                 some(append<&ast::Clade::children>(ref<Clade>), lit<",">) +
                 lit<")">) +
             ((option(name) + option(length)) | (length + name))> {};
  struct Tree : rule<ref<Clade> + lit<";">> {};
  using entry = Tree;
  using ignored = WS;
  using hidden = list<COMMENT>;
};
}  // namespace newick
