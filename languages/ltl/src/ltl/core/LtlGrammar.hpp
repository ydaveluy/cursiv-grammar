#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "ltl/core/ast.hpp"

namespace ltl {

struct Ltl : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \r\n\t)">)> {};

  struct ATOMIC : terminal<cr<"a-z"> + many(cr<"a-zA-Z0-9_">)> {};

  struct NOT
      : terminal<lit<"⌐"> | lit<"!"> | lit<"¬">, value<ast::UnaryOp::negate>> {
  };
  struct GLOBALLY
      : terminal<lit<"G"> | lit<"[]">, value<ast::UnaryOp::globally>> {};
  struct FINALLY
      : terminal<lit<"F"> | lit<"<>">, value<ast::UnaryOp::finally>> {};
  struct NEXT : terminal<lit<"X">, value<ast::UnaryOp::next>> {};

  struct AND : terminal<lit<"∧"> | lit<"&&"> | lit<"/\\">,
                        value<ast::BinaryOp::land>> {};
  struct OR
      : terminal<lit<"∨"> | lit<"||"> | lit<"\\/">, value<ast::BinaryOp::lor>> {
  };
  struct ARROW : terminal<lit<"→"> | lit<"->">, value<ast::BinaryOp::implies>> {
  };
  struct UNTIL : terminal<lit<"U">, value<ast::BinaryOp::until>> {};
  struct WEAK : terminal<lit<"W">, value<ast::BinaryOp::weak>> {};
  struct RELEASE
      : terminal<lit<"R"> | lit<"V">, value<ast::BinaryOp::release>> {};

  struct EQUIV
      : terminal<lit<"↔"> | lit<"<->">, value<ast::BinaryOp::equivalent>> {};

  struct Proposition;

  struct BoolLit
      : rule<create<ast::BoolLit> +
             (flag<&ast::BoolLit::value>(kw<"true">) | kw<"false">)> {};

  struct AtomicProp : rule<assign<&ast::AtomicProp::name>(ref<ATOMIC>)> {};

  struct Atom : rule<(lit<"("> + ref<Proposition> + lit<")">) | //
                     ref<BoolLit> |                             //
                     ref<AtomicProp>> {};

  struct Unary
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ref<NOT> | ref<GLOBALLY> |
                                             ref<FINALLY> | ref<NEXT>) +
                 assign<&ast::UnaryExpr::operand>(ref<Unary>)) |
             ref<Atom>> {};

  struct Proposition
      : infix<Unary, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(ref<UNTIL> | ref<WEAK> | ref<RELEASE>) >>
               left(ref<AND> | ref<OR> | ref<ARROW> | ref<EQUIV>))> {};

  using entry = Proposition;
  using ignore = WS;
};

} // namespace ltl
