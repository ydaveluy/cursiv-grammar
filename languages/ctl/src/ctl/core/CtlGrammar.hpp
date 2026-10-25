#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "ctl/core/ast.hpp"

namespace ctl {

inline constexpr auto quoted_name = [](std::string_view text) {
  return text.size() >= 2 ? text.substr(1, text.size() - 2) : std::string_view{};
};

struct Ctl : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \r\n\t\f)">)> {};
  static constexpr auto word_boundary = cr<"a-zA-Z0-9_">;
  static constexpr auto quantifier = lit<"A"> | lit<"E">;
  static constexpr auto modality = lit<"X"> | lit<"F"> | lit<"G">;

  // Compact AXp and EFp retain their temporal meaning.
  struct ATOMIC : terminal<!((quantifier + modality) | modality) + cr<"a-zA-Z_"> + many(word_boundary)> {};
  struct QUOTED : terminal<lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\\r\n)">) +
                           lit<"\"">, text_conv<quoted_name>> {};

  struct UNTIL : terminal<lit<"U">, value<ast::TemporalRelation::until>> {};
  struct RELEASE : terminal<lit<"R">, value<ast::TemporalRelation::release>> {};
  struct GLOBALLY : terminal<lit<"G">, value<ast::TemporalModality::globally>> {};
  struct FINALLY : terminal<lit<"F">, value<ast::TemporalModality::finally>> {};
  struct NEXT : terminal<lit<"X">, value<ast::TemporalModality::next>> {};
  struct ALL : terminal<lit<"A">, value<ast::PathQuantifier::inevitable>> {};
  struct EXISTS : terminal<lit<"E">, value<ast::PathQuantifier::exists>> {};
  struct IFF : terminal<lit<"⇔">, value<ast::BinaryOp::biconditional>> {};
  struct IMPLIES : terminal<lit<"⇒"> | lit<"-->">, value<ast::BinaryOp::implication>> {};
  struct AND : terminal<lit<"∧"> | lit<"&"> | kw<"and">, value<ast::BinaryOp::conjunction>> {};
  struct OR : terminal<lit<"∨"> | lit<"|"> | kw<"or">, value<ast::BinaryOp::disjunction>> {};

  struct Primary;
  struct Boolean : infix<Primary, &ast::BinaryProp::left, &ast::BinaryProp::op,
      &ast::BinaryProp::right, (left(ref<AND>) >> left(ref<OR>) >>
      left(ref<IMPLIES>) >> left(ref<IFF>))> {};
  struct Proposition : infix<Boolean, &ast::BinaryTemporal::left,
      &ast::BinaryTemporal::relation, &ast::BinaryTemporal::right,
      left(ref<UNTIL> | ref<RELEASE>)> {};

  struct Primary : rule<
      create<ast::Top>(lit<"⊤"> | kw<"true">) |
      create<ast::Bottom>(lit<"⊥"> | kw<"false">) |
      create<ast::AtomicProp>(assign<&ast::AtomicProp::name>(ref<ATOMIC>) |
          flag<&ast::AtomicProp::quoted>(assign<&ast::AtomicProp::name>(ref<QUOTED>))) |
      create<ast::Quantified>(
          assign<&ast::Quantified::quantifier>(ref<ALL> | ref<EXISTS>) +
          assign<&ast::Quantified::path>(ref<Primary>)) |
      create<ast::UnaryTemporal>(
          assign<&ast::UnaryTemporal::modality>(ref<NEXT> | ref<FINALLY> | ref<GLOBALLY>) +
          assign<&ast::UnaryTemporal::operand>(ref<Primary>)) |
      create<ast::Grouped>(
          (lit<"("> + assign<&ast::Grouped::inner>(ref<Proposition>) + lit<")">) |
          (lit<"["> + assign<&ast::Grouped::inner>(ref<Proposition>) + lit<"]">)) |
      create<ast::Not>((lit<"⌐"> | lit<"~"> | kw<"not">) +
                       assign<&ast::Not::operand>(ref<Primary>)) |
      create<ast::Missing>> {};

  using entry = Proposition;
  using ignore = WS;
};

}  // namespace ctl
