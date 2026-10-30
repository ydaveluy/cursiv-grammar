#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "tl/core/ast.hpp"

namespace tl {

struct Tl : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \r\n\t)">)> {};
  struct ATOMIC : terminal<some(cr<"a-z">)> {};
  struct OR : terminal<lit<"∨">> {};

  struct Operand;
  struct Proposition : infix<Operand, ast::Or, left(ref<OR>)> {};

  struct Operand : rule<
      create<ast::Bottom>(lit<"⊥">) |
      create<ast::AtomicProp>(assign<&ast::AtomicProp::name>(ref<ATOMIC>)) |
      create<ast::Group>(lit<"("> + assign<&ast::Group::inner>(ref<Proposition>) + lit<")">) |
      create<ast::Not>(lit<"⌐"> + assign<&ast::Not::operand>(ref<Operand>)) |
      create<ast::TenseOp>(assign<&ast::TenseOp::tense>(as<ast::Tense::always>(lit<"G">) | as<ast::Tense::was>(lit<"H">)) +
                           assign<&ast::TenseOp::operand>(ref<Proposition>)) |
      create<ast::Missing>> {};

  struct File : rule<assign<&ast::Formula::value>(ref<Proposition>)> {};

  using entry = File;
  using ignored = WS;
};

}  // namespace tl
