#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "wktcrsv1/core/ast.hpp"

namespace wktcrsv1 {
struct WktCrsV1 : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \r\n\t\v\f)">)> {};
  struct NAME : terminal<cr<"A-Za-z_"> + many(cr<"A-Za-z_0-9">)> {};
  struct CODE : terminal<some(cr<"A-Za-z_0-9.:-">)> {};
  struct NUMBER : terminal<option(cr<"+-">) +
      ((some(d) + option(lit<"."> + many(d))) | (lit<"."> + some(d))) +
      option(cr<"eE"> + option(cr<"+-">) + some(d))> {};
  struct TEXT : terminal<
      (lit<"\""> + many(lit<"\"\""> | cr<R"(^")">) + lit<"\"">) |
      (lit<"“"> + many(!lit<"”"> + any) + lit<"”">)> {};
  struct COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};

  struct ClauseRule;
  struct TextRule : rule<assign<&ast::Text::literal>(ref<TEXT>)> {};
  struct NumberRule : rule<assign<&ast::Number::literal>(ref<NUMBER>)> {};
  struct IdentifierRule : rule<assign<&ast::Identifier::literal>(ref<NAME>)> {};
  static constexpr auto argument = ref<ClauseRule> | ref<TextRule> |
                                   ref<NumberRule> | ref<IdentifierRule>;
  struct ClauseRule : rule<assign<&ast::Clause::keyword>(ref<NAME>) +
      assign<&ast::Clause::open>(lit<"["> | lit<"(">) +
      option(append<&ast::Clause::arguments>(argument) +
             many(lit<","> + append<&ast::Clause::arguments>(argument)) +
             option(flag<&ast::Clause::trailingComma>(lit<",">))) +
      assign<&ast::Clause::close>(lit<"]"> | lit<")">)> {};
  struct CommentRule : rule<assign<&ast::CommentLine::text>(ref<COMMENT>)> {};
  struct EpsgDefRule : rule<assign<&ast::EpsgDefLine::code>(ref<CODE>) +
      lit<"="> + assign<&ast::EpsgDefLine::wkt>(ref<ClauseRule>) +
      option(lit<","> + assign<&ast::EpsgDefLine::vertical>(ref<ClauseRule>))> {};
  struct DefinitionRule : rule<assign<&ast::Definition::wkt>(ref<ClauseRule>) +
      option(lit<","> + assign<&ast::Definition::vertical>(ref<ClauseRule>))> {};
  struct PropsFileRule : rule<create<ast::PropsFile> + many(
      append<&ast::PropsFile::rows>(ref<CommentRule> | ref<EpsgDefRule> |
                                   ref<DefinitionRule>))> {};
  using entry = PropsFileRule;
  using ignore = WS;
};
}  // namespace wktcrsv1
