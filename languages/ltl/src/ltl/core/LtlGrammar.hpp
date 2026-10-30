#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "ltl/core/ast.hpp"

namespace ltl {

struct Ltl : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \r\n\t)">)> {};

  struct ATOMIC : terminal<cr<"a-z"> + many(cr<"a-zA-Z0-9_">)> {};

  struct NOT
      : terminal<as<ast::UnaryOp::negate>(lit<"⌐"> | lit<"!"> | lit<"¬">)> {};
  struct GLOBALLY : terminal<as<ast::UnaryOp::globally>(lit<"G"> | lit<"[]">)> {
  };
  struct FINALLY : terminal<as<ast::UnaryOp::finally>(lit<"F"> | lit<"<>">)> {};

  struct AND
      : terminal<as<ast::BinaryOp::land>(lit<"∧"> | lit<"&&"> | lit<"/\\">)> {};
  struct OR
      : terminal<as<ast::BinaryOp::lor>(lit<"∨"> | lit<"||"> | lit<"\\/">)> {};
  struct ARROW : terminal<as<ast::BinaryOp::implies>(lit<"→"> | lit<"->">)> {};
  struct UNTIL : terminal<as<ast::BinaryOp::until>(lit<"U">)> {};
  struct WEAK : terminal<as<ast::BinaryOp::weak>(lit<"W">)> {};
  struct RELEASE : terminal<as<ast::BinaryOp::release>(lit<"R"> | lit<"V">)> {};

  struct STRONG_RELEASE : terminal<as<ast::BinaryOp::strong_release>(lit<"M">)> {};

  struct EQUIV
      : terminal<as<ast::BinaryOp::equivalent>(lit<"↔"> | lit<"<->">)> {};

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
                                             ref<FINALLY> | as<ast::UnaryOp::next>(lit<"X">)) +
                 assign<&ast::UnaryExpr::operand>(ref<Unary>)) |
             ref<Atom>> {};

  struct Proposition
      : infix<Unary, ast::BinaryExpr,
              levels(left(ref<UNTIL> | ref<WEAK> | ref<RELEASE> | ref<STRONG_RELEASE>),
                     left(ref<AND>), left(ref<OR>), right(ref<ARROW>), left(ref<EQUIV>))> {};

  using entry = Proposition;
  using ignored = WS;
};

} // namespace ltl
