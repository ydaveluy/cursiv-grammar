#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "ttm/core/ast.hpp"

namespace ttm {
struct Ttm : cursiv::GrammarSpec {
  static constexpr auto scalar = cr<R"(\x01-\u{10ffff})">;
  static constexpr auto escaped = lit<"@"> + scalar;
  static constexpr auto sharp = lit<"#"> + !(lit<"<"> | lit<"#<">);
  static constexpr auto text_char = ~(cr<"#<>;@"> | ~scalar);
  static constexpr auto plain_char = ~(cr<"#<@"> | ~scalar);
  struct TEXT : terminal<some(escaped | sharp | text_char)> {};
  struct PLAIN : terminal<some(escaped | sharp | plain_char)> {};
  struct QUOTED
      : terminal<lit<"<"> +
                 many(escaped | ref<QUOTED> | ~(cr<"<>@"> | ~scalar)) +
                 lit<">">> {};

  struct Active;
  struct Neutral;
  static constexpr auto function = ref<Active> | ref<Neutral>;
  static constexpr auto part = function | ref<QUOTED> | ref<TEXT>;
  struct Name : rule<create<ast::Argument> +
                     some(append<&ast::Argument::parts>(part))> {};
  struct Argument : rule<create<ast::Argument> +
                         many(append<&ast::Argument::parts>(part))> {};
  static constexpr auto call_body =
      assign<&ast::Call::name>(ref<Name>) +
      many(lit<";"> + append<&ast::Call::args>(ref<Argument>)) + lit<">">;
  struct Active : rule<create<ast::Active> + lit<"#<"> + call_body> {};
  struct Neutral : rule<create<ast::Neutral> + lit<"##<"> + call_body> {};
  struct Program
      : rule<create<ast::Program> +
             many(append<&ast::Program::parts>(function | ref<QUOTED> | ref<PLAIN>))> {};
  using entry = Program;
};
} // namespace ttm
