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
      through(lit<"“">, lit<"”">)> {};
  struct COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};

  struct Clause;
  struct Text : rule<assign<&ast::Text::literal>(ref<TEXT>)> {};
  struct Number : rule<assign<&ast::Number::literal>(ref<NUMBER>)> {};
  struct Identifier : rule<assign<&ast::Identifier::literal>(ref<NAME>)> {};
  static constexpr auto argument = ref<Clause> | ref<Text> |
                                   ref<Number> | ref<Identifier>;
  struct Clause : rule<assign<&ast::Clause::keyword>(ref<NAME>) +
      assign<&ast::Clause::open>(lit<"["> | lit<"(">) +
      option(some(append<&ast::Clause::arguments>(argument), lit<",">) +
             option(flag<&ast::Clause::trailingComma>(lit<",">))) +
      assign<&ast::Clause::close>(lit<"]"> | lit<")">)> {};
  struct Comment : rule<assign<&ast::CommentLine::text>(ref<COMMENT>)> {};
  struct EpsgDef : rule<assign<&ast::EpsgDefLine::code>(ref<CODE>) +
      lit<"="> + assign<&ast::EpsgDefLine::wkt>(ref<Clause>) +
      option(lit<","> + assign<&ast::EpsgDefLine::vertical>(ref<Clause>))> {};
  struct Definition : rule<assign<&ast::Definition::wkt>(ref<Clause>) +
      option(lit<","> + assign<&ast::Definition::vertical>(ref<Clause>))> {};
  struct PropsFile : rule<create<ast::PropsFile> + many(
      append<&ast::PropsFile::rows>(ref<Comment> | ref<EpsgDef> |
                                   ref<Definition>))> {};
  using entry = PropsFile;
  using ignored = WS;
};
}  // namespace wktcrsv1
