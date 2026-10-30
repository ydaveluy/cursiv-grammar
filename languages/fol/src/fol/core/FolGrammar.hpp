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

struct Fol : cursiv::GrammarSpec {
  struct WHITESPACE : terminal<some(cr<R"( \t)">)> {};
  struct ENDLINE : terminal<some(cr<R"(\r\n)">)> {};

  static constexpr auto nameChars = many(cr<"0-9a-zA-Z \t">);
  struct VARIABLE : terminal<as<strip_sigil>(lit<"?"> + nameChars)> {};
  struct PRED_CONSTANT : terminal<as<strip_sigil>(lit<"_"> + nameChars)> {};
  struct IND_CONSTANT : terminal<as<strip_sigil>(lit<"#"> + nameChars)> {};
  struct FUNC_CONSTANT : terminal<as<strip_sigil>(lit<"."> + nameChars)> {};

  // Spellings as the grammar defines them, not standard logic notation:
  // CONJ lexes '\/', DISJ lexes '^'.
  struct CONJ : terminal<as<ast::BinOp::conj>(lit<"\\/">)> {};
  struct DISJ : terminal<as<ast::BinOp::disj>(lit<"^">)> {};

  struct Variable : rule<assign<&ast::Variable::name>(ref<VARIABLE>)> {};

  struct FuncCall;

  struct Term
      : rule<create<ast::IndConstant>(
                 assign<&ast::IndConstant::name>(ref<IND_CONSTANT>))  //
             | ref<Variable>                                     //
             | ref<FuncCall>> {};

  struct FuncCall
      : rule<assign<&ast::FuncCall::func>(ref<FUNC_CONSTANT>) + lit<"("> +
             some(append<&ast::FuncCall::args>(ref<Term>), lit<",">) +
             lit<")">> {};

  struct PrimaryFormula;

  struct Formula
      : infix<PrimaryFormula, ast::BinaryFormula,
              left(ref<CONJ> | ref<DISJ> | as<ast::BinOp::impl>(lit<"->">) | as<ast::BinOp::bicond>(lit<"<->">))> {};

  struct PrimaryFormula
      : rule<create<ast::BinaryFormula>(
                 assign<&ast::BinaryFormula::left>(create<ast::Negation>(
                     lit<"!"> + assign<&ast::Negation::formula>(ref<Formula>))) +
                 assign<&ast::BinaryFormula::op>(ref<CONJ> | ref<DISJ> |
                                                as<ast::BinOp::impl>(lit<"->">) | as<ast::BinOp::bicond>(lit<"<->">)) +
                 assign<&ast::BinaryFormula::right>(ref<Formula>)) |
             create<ast::Negation>(
                 lit<"!"> + assign<&ast::Negation::formula>(ref<Formula>))  //
             | create<ast::ForallFormula>(
                   kw<"Forall"> + lit<"("> +
                   assign<&ast::ForallFormula::variable>(ref<Variable>) +
                   lit<")"> +
                   assign<&ast::ForallFormula::formula>(ref<Formula>))  //
             | create<ast::ExistsFormula>(
                   kw<"Exists"> + lit<"("> +
                   assign<&ast::ExistsFormula::variable>(ref<Variable>) +
                   lit<")"> +
                   assign<&ast::ExistsFormula::formula>(ref<Formula>))  //
             | create<ast::PredicateAtom>(
                   assign<&ast::PredicateAtom::pred>(ref<PRED_CONSTANT>) +
                   lit<"("> +
                   some(append<&ast::PredicateAtom::args>(ref<Term>),
                        lit<",">) +
                   lit<")">)  //
             | create<ast::EqualityAtom>(
                   assign<&ast::EqualityAtom::left>(ref<Term>) + lit<"="> +
                   assign<&ast::EqualityAtom::right>(ref<Term>))> {};

  struct Condition
      : rule<some(append<&ast::Condition::formulas>(ref<Formula>),
                  ref<ENDLINE>) +
             many(ref<ENDLINE>)> {};

  using entry = Condition;
  using ignored = WHITESPACE;
};

}  // namespace fol
