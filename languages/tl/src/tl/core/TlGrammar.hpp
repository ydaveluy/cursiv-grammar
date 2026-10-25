#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "tl/core/ast.hpp"

namespace tl {

struct Tl : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \r\n\t)">)> {};
  struct ATOMIC : terminal<some(cr<"a-z">)> {};
  struct ALWAYS : terminal<lit<"G">, value<ast::Tense::always>> {};
  struct WAS : terminal<lit<"H">, value<ast::Tense::was>> {};
  struct OR : terminal<lit<"∨">> {};

  struct Operand;
  struct PropositionRule : infix<Operand, &ast::Or::left, &ast::Or::op,
      &ast::Or::right, left(ref<OR>)> {};

  struct Operand : rule<
      create<ast::Bottom>(lit<"⊥">) |
      create<ast::AtomicProp>(assign<&ast::AtomicProp::name>(ref<ATOMIC>)) |
      create<ast::Group>(lit<"("> + assign<&ast::Group::inner>(ref<PropositionRule>) + lit<")">) |
      create<ast::Not>(lit<"⌐"> + assign<&ast::Not::operand>(ref<Operand>)) |
      create<ast::TenseOp>(assign<&ast::TenseOp::tense>(ref<ALWAYS> | ref<WAS>) +
                           assign<&ast::TenseOp::operand>(ref<PropositionRule>)) |
      create<ast::Missing>> {};

  struct FileRule : rule<assign<&ast::Formula::value>(ref<PropositionRule>)> {};

  using entry = FileRule;
  using ignore = WS;
};

}  // namespace tl
