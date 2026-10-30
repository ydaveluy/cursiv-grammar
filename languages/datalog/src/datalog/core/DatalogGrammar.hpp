// The datalog language: its grammar.
#pragma once

#include <memory_resource>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "datalog/core/ast.hpp"

namespace datalog {

inline constexpr auto unquote = [](std::string_view t,
                                   std::pmr::memory_resource* resource) {
  if (t.size() == 2) return std::string_view{};
  auto* value = static_cast<char*>(resource->allocate(t.size() - 2));
  std::size_t size = 0;
  for (std::size_t i = 1; i + 1 < t.size(); ++i) {
    if (t[i] == '\\' && i + 2 < t.size()) ++i;
    value[size++] = t[i];
  }
  return std::string_view(value, size);
};

struct Datalog : cursiv::GrammarSpec {
  static constexpr auto NameChar = cr<R"(^(,)=:.~?"% \n)">;
  static constexpr auto word_boundary = NameChar;
  static constexpr auto Upper =
      uc::Lu | cr<"\u2160-\u216F\u24B6-\u24CF\U0001F130-\U0001F149"
                  "\U0001F150-\U0001F169\U0001F170-\U0001F189">;
  static constexpr auto Lower =
      uc::Ll | cr<"\u00AA\u00BA\u02B0-\u02B8\u02C0-\u02C1\u02E0-\u02E4"
                  "\u0345\u037A\u10FC\u1D2C-\u1D6A\u1D78\u1D9B-\u1DBF"
                  "\u2071\u207F\u2090-\u209C\u2170-\u217F\u24D0-\u24E9"
                  "\u2C7C-\u2C7D\uA69C-\uA69D\uA770\uA7F2-\uA7F4"
                  "\uA7F8-\uA7F9\uAB5C-\uAB5F\uAB69\U00010780"
                  "\U00010783-\U00010785\U00010787-\U000107B0"
                  "\U000107B2-\U000107BA\U0001E030-\U0001E06D">;
  static constexpr auto VariableBody = Upper + many(cr<"0-9_"> | Upper | Lower);
  static constexpr auto IntegerBody =
      lit<"0"> | (option(lit<"-">) + cr<"1-9"> + many(d));

  struct WS : terminal<some(s | uc::Zs | uc::Zl | uc::Zp)> {};
  struct COMMENT
      : terminal<(lit<"%"> | (lit<"#"> + &(cr<" \t"> | lit<"lang">))) +
                 many(cr<R"(^\n)">)> {};

  struct VARIABLE : terminal<VariableBody + !NameChar> {};
  struct IDENTIFIER : terminal<some(NameChar)> {};
  struct ATOM : terminal<!((IntegerBody | VariableBody | lit<"true"> |
                            lit<"false">)+!NameChar) +
                         some(NameChar)> {};
  struct STRING
      : terminal<as<unquote>(
            lit<"\""> + many(cr<R"(^"\\\n)"> | (lit<"\\"> + cr<R"("\\\n)">)) +
            lit<"\"">)> {};
  struct INTEGER : terminal<IntegerBody + !NameChar> {};
  struct NameValue
      : rule<ref<IDENTIFIER> | ref<INTEGER> | kw<"true"> | kw<"false">> {};

  struct Variable : rule<assign<&ast::Variable::name>(ref<VARIABLE>)> {};

  struct AtomConstant : rule<assign<&ast::AtomConstant::name>(ref<ATOM>)> {
  };

  struct StringConstant
      : rule<assign<&ast::StringConstant::value>(ref<STRING>)> {};

  struct IntConstant
      : rule<assign<&ast::IntConstant::value>(ref<INTEGER>)> {};

  struct BoolConstant
      : rule<create<ast::BoolConstant> +
             (flag<&ast::BoolConstant::value>(kw<"true">) | kw<"false">)> {
  };

  struct Term : rule<ref<Variable> | ref<AtomConstant> |
                         ref<StringConstant> | ref<IntConstant> |
                         ref<BoolConstant>> {};

  struct ExternalLiteral
      : rule<
            create<ast::ExternalLiteral> +
            assign<&ast::ExternalLiteral::variable>(ref<Variable>) +
            lit<":-"> +
            assign<&ast::ExternalLiteral::external>(ref<NameValue>) + lit<"("> +
            some(append<&ast::ExternalLiteral::args>(ref<Term>), lit<",">) +
            lit<")">> {};

  struct EqualityLiteral
      : rule<create<ast::EqualityLiteral> +
             assign<&ast::EqualityLiteral::left>(ref<Term>) +
             (flag<&ast::EqualityLiteral::negated>(lit<"!=">) |
              lit<"=">)+assign<&ast::EqualityLiteral::right>(ref<Term>)> {};

  struct PredicateAtom
      : rule<create<ast::PredicateAtom> +
             (assign<&ast::PredicateAtom::predicate>(ref<NameValue>) |
              flag<&ast::PredicateAtom::quoted>(
                  assign<&ast::PredicateAtom::predicate>(ref<STRING>))) +
             option(
                 lit<"("> +
                 option(some(append<&ast::PredicateAtom::args>(ref<Term>),
                             lit<",">)) +
                 lit<")">)> {};

  struct Literal : rule<ref<ExternalLiteral> |
                            ref<EqualityLiteral> | ref<PredicateAtom>> {
  };

  struct Clause
      : rule<create<ast::Clause> +
             assign<&ast::Clause::head>(ref<Literal>) +
             option(lit<":-"> +
                    some(append<&ast::Clause::body>(ref<Literal>),
                         lit<",">))> {};

  struct Assertion
      : rule<assign<&ast::Assertion::clause>(ref<Clause>) + lit<".">> {};

  struct Retraction
      : rule<assign<&ast::Retraction::clause>(ref<Clause>) + lit<"~">> {};

  struct Query
      : rule<assign<&ast::Query::literal>(ref<Literal>) + lit<"?">> {};

  struct Requirement
      : rule<lit<"("> + assign<&ast::Requirement::name>(ref<NameValue>) +
             lit<")"> + lit<".">> {};

  struct Statement : rule<ref<Requirement> | ref<Assertion> |
                              ref<Retraction> | ref<Query>> {};

  struct Program
      : rule<create<ast::Program> +
             many(append<&ast::Program::statements>(ref<Statement>))> {};

  using entry = Program;
  using ignored = list<WS, COMMENT>;
};

}  // namespace datalog
