// The fol language: its grammar -- first-order logic formulas, one per
// non-blank line.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "fol/core/ast.hpp"

namespace fol {

inline constexpr auto strip_sigil = [](std::string_view text,
                                     std::pmr::memory_resource* memory) {
  text.remove_prefix(1);
  if (text.find_first_of(" \t") == text.npos) return text;
  const auto room = cursiv::text_room(memory, text.size());
  std::size_t size = 0;
  for (const char c : text)
    if (c != ' ' && c != '\t') room[size++] = c;
  return std::string_view(room.data(), size);
};
using Name = cursiv::text_conv<strip_sigil>;

struct Fol : cursiv::GrammarSpec {
  struct WHITESPACE : terminal<some(cr<R"( \t)">)> {};
  struct ENDLINE : terminal<some(cr<R"(\r\n)">)> {};

  static constexpr auto nameChars = many(cr<"0-9a-zA-Z \t">);
  struct VARIABLE : terminal<lit<"?"> + nameChars, Name> {};
  struct PRED_CONSTANT : terminal<lit<"_"> + nameChars, Name> {};
  struct IND_CONSTANT : terminal<lit<"#"> + nameChars, Name> {};
  struct FUNC_CONSTANT : terminal<lit<"."> + nameChars, Name> {};

  // Spellings as the grammar defines them, not standard logic notation:
  // CONJ lexes '\/', DISJ lexes '^'.
  struct CONJ : terminal<lit<"\\/">, value<ast::BinOp::conj>> {};
  struct DISJ : terminal<lit<"^">, value<ast::BinOp::disj>> {};
  struct IMPL : terminal<lit<"->">, value<ast::BinOp::impl>> {};
  struct BICOND : terminal<lit<"<->">, value<ast::BinOp::bicond>> {};

  struct VariableRule : rule<assign<&ast::Variable::name>(ref<VARIABLE>)> {};

  struct FuncCallRule;

  struct TermRule
      : rule<create<ast::IndConstant>(
                 assign<&ast::IndConstant::name>(ref<IND_CONSTANT>))  //
             | ref<VariableRule>                                     //
             | ref<FuncCallRule>> {};

  struct FuncCallRule
      : rule<assign<&ast::FuncCall::func>(ref<FUNC_CONSTANT>) + lit<"("> +
             some(append<&ast::FuncCall::args>(ref<TermRule>), lit<",">) +
             lit<")">> {};

  struct PrimaryFormula;

  struct Formula
      : infix<PrimaryFormula, &ast::BinaryFormula::left,
              &ast::BinaryFormula::op, &ast::BinaryFormula::right,
              left(ref<CONJ> | ref<DISJ> | ref<IMPL> | ref<BICOND>)> {};

  struct PrimaryFormula
      : rule<create<ast::BinaryFormula>(
                 assign<&ast::BinaryFormula::left>(create<ast::Negation>(
                     lit<"!"> + assign<&ast::Negation::formula>(ref<Formula>))) +
                 assign<&ast::BinaryFormula::op>(ref<CONJ> | ref<DISJ> |
                                                ref<IMPL> | ref<BICOND>) +
                 assign<&ast::BinaryFormula::right>(ref<Formula>)) |
             create<ast::Negation>(
                 lit<"!"> + assign<&ast::Negation::formula>(ref<Formula>))  //
             | create<ast::ForallFormula>(
                   kw<"Forall"> + lit<"("> +
                   assign<&ast::ForallFormula::variable>(ref<VariableRule>) +
                   lit<")"> +
                   assign<&ast::ForallFormula::formula>(ref<Formula>))  //
             | create<ast::ExistsFormula>(
                   kw<"Exists"> + lit<"("> +
                   assign<&ast::ExistsFormula::variable>(ref<VariableRule>) +
                   lit<")"> +
                   assign<&ast::ExistsFormula::formula>(ref<Formula>))  //
             | create<ast::PredicateAtom>(
                   assign<&ast::PredicateAtom::pred>(ref<PRED_CONSTANT>) +
                   lit<"("> +
                   some(append<&ast::PredicateAtom::args>(ref<TermRule>),
                        lit<",">) +
                   lit<")">)  //
             | create<ast::EqualityAtom>(
                   assign<&ast::EqualityAtom::left>(ref<TermRule>) + lit<"="> +
                   assign<&ast::EqualityAtom::right>(ref<TermRule>))> {};

  struct ConditionRule
      : rule<some(append<&ast::Condition::formulas>(ref<Formula>),
                  ref<ENDLINE>) +
             many(ref<ENDLINE>)> {};

  using entry = ConditionRule;
  using ignore = WHITESPACE;
};

}  // namespace fol
