// The ttm language: its grammar.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "ttm/core/ast.hpp"

namespace ttm {

inline constexpr auto unwrap = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unwrapped = cursiv::text_conv<unwrap>;

struct Ttm : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  struct STRING : terminal<some(cr<R"(a-zA-Z0-9@!#+\x2d*&$%'?=".|_ )">)> {};

  struct ESCSTRING
      : terminal<lit<"<"> + some(cr<"a-zA-Z">) + lit<">">, Unwrapped> {};

  static constexpr auto text = ref<STRING> | ref<ESCSTRING>;

  struct Active;
  struct Neutral;

  static constexpr auto function_ = ref<Active> | ref<Neutral>;

  // The wrapped call and ESCSTRING both open on '<': disjoint since a call
  // reads on as '#', ESCSTRING as a letter.
  static constexpr auto arg =
      function_ | (lit<"<"> + function_ + lit<">">) | text;

  static constexpr auto args =
      option(lit<";"> + some(append<&ast::Call::args>(arg), lit<";">));

  struct Active : rule<create<ast::Active> + lit<"#<"> +
                       assign<&ast::Call::name>(text) + args + lit<">">> {};

  struct Neutral : rule<create<ast::Neutral> + lit<"##<"> +
                        assign<&ast::Call::name>(text) + args + lit<">">> {};

  struct Program : rule<create<ast::Program> +
                        many(append<&ast::Program::calls>(function_))> {};

  using entry = Program;
  using ignore = WS;
};

}  // namespace ttm
