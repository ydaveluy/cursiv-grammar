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
  struct QUOTED : terminal<as<quoted_name>(
                      lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\\r\n)">) +
                      lit<"\"">)> {};

  struct EXISTS : terminal<as<ast::PathQuantifier::exists>(lit<"E">)> {};
  struct IFF : terminal<as<ast::BinaryOp::biconditional>(lit<"⇔">)> {};
  struct IMPLIES
      : terminal<as<ast::BinaryOp::implication>(lit<"⇒"> | lit<"-->">)> {};
  struct AND : terminal<as<ast::BinaryOp::conjunction>(lit<"∧"> | lit<"&"> |
                                                       kw<"and">)> {};
  struct OR : terminal<as<ast::BinaryOp::disjunction>(lit<"∨"> | lit<"|"> |
                                                      kw<"or">)> {};

  struct Primary;
  struct Boolean : infix<Primary, ast::BinaryProp,
                         levels(left(ref<AND>), left(ref<OR>),
                                left(ref<IMPLIES>), left(ref<IFF>))> {};
  struct Proposition
      : infix<Boolean, ast::BinaryTemporal,
              left(as<ast::TemporalRelation::until>(lit<"U">) |
                   as<ast::TemporalRelation::release>(lit<"R">)),
              &ast::BinaryTemporal::left, &ast::BinaryTemporal::relation,
              &ast::BinaryTemporal::right> {};

  struct Primary : rule<
      create<ast::Top>(lit<"⊤"> | kw<"true">) |
      create<ast::Bottom>(lit<"⊥"> | kw<"false">) |
      create<ast::AtomicProp>(assign<&ast::AtomicProp::name>(ref<ATOMIC>) |
          flag<&ast::AtomicProp::quoted>(assign<&ast::AtomicProp::name>(ref<QUOTED>))) |
      create<ast::Quantified>(
          assign<&ast::Quantified::quantifier>(as<ast::PathQuantifier::inevitable>(lit<"A">) | ref<EXISTS>) +
          assign<&ast::Quantified::path>(ref<Primary>)) |
      create<ast::UnaryTemporal>(
          assign<&ast::UnaryTemporal::modality>(as<ast::TemporalModality::next>(lit<"X">) | as<ast::TemporalModality::finally>(lit<"F">) | as<ast::TemporalModality::globally>(lit<"G">)) +
          assign<&ast::UnaryTemporal::operand>(ref<Primary>)) |
      create<ast::Grouped>(
          (lit<"("> + assign<&ast::Grouped::inner>(ref<Proposition>) + lit<")">) |
          (lit<"["> + assign<&ast::Grouped::inner>(ref<Proposition>) + lit<"]">)) |
      create<ast::Not>((lit<"⌐"> | lit<"~"> | kw<"not">) +
                       assign<&ast::Not::operand>(ref<Primary>)) |
      create<ast::Missing>> {};

  using entry = Proposition;
  using ignored = WS;
};

}  // namespace ctl
