// The refal language: its grammar.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "refal/core/ast.hpp"

namespace refal {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unquoted = cursiv::text_conv<unquote>;

struct Refal : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \r\n\t)">)> {};
  // A line starting with '*' runs to end of line; no other construct opens
  // on a bare '*'.
  struct LINE_COMMENT : terminal<lit<"*"> + many(cr<R"(^\r\n)">)> {};
  struct BLOCK_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  struct IDENT : terminal<cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9_-">)> {};
  struct DIGITS : terminal<some(d)> {};

  struct STRING
      : terminal<lit<"\""> + many(cr<R"(^")">) + lit<"\"">, Unquoted> {};
  struct STRING2 : terminal<lit<"'"> + many(cr<"^'">) + lit<"'">, Unquoted> {};

  struct CHAR : terminal<lit<"\\'"> | lit<"\\\""> | lit<"\\\\"> |
                         lit<"\\n"> | lit<"\\r"> | lit<"\\t"> |
                         (lit<"\\x"> + repeat<2>(d))> {};

  struct Identifier : rule<ref<IDENT> | ref<STRING>> {};
  struct Index : rule<ref<Identifier> | ref<DIGITS>> {};

  // The letter and the dot are one token: `s`/`t`/`e` alone, or as the first
  // letters of a longer word, still lex as IDENT (longest match), so a
  // symbol named `s` needs no guard of its own.
  struct SVAR : terminal<lit<"s"> + lit<".">, value<ast::VarKind::s>> {};
  struct TVAR : terminal<lit<"t"> + lit<".">, value<ast::VarKind::t>> {};
  struct EVAR : terminal<lit<"e"> + lit<".">, value<ast::VarKind::e>> {};

  struct EXTERNAL_KW
      : terminal<lit<"$EXTERNAL"> | lit<"$EXTERN"> | lit<"$EXTRN">> {};

  struct SymbolRule;
  struct VariableRule;
  struct CallRule;
  static constexpr auto term = ref<SymbolRule> | ref<VariableRule> | ref<CallRule>;

  // A symbol derives STRING twice, directly and through Identifier:
  // one choice, same accepted texts.
  struct SymbolRule
      : rule<assign<&ast::Symbol::text>(ref<Identifier> | ref<DIGITS> |
                                        ref<STRING2> | ref<CHAR>)> {};

  struct VariableRule
      : rule<assign<&ast::Variable::kind>(ref<SVAR> | ref<TVAR> | ref<EVAR>) +
             assign<&ast::Variable::index>(ref<Index>)> {};

  struct CallRule
      : rule<lit<"<"> + many(append<&ast::Call::body>(term)) + lit<">">> {};

  struct ConditionRule
      : rule<lit<","> + many(append<&ast::Condition::arg>(term)) + lit<":"> +
             many(append<&ast::Condition::pattern>(term))> {};

  struct SentenceRule;
  struct BlockEndingRule
      : rule<many(append<&ast::BlockEnding::arg>(term)) + lit<":"> +
             lit<"{"> +
             some(append<&ast::BlockEnding::sentences>(ref<SentenceRule>),
                  lit<";">) +
             option(lit<";">) + lit<"}">> {};

  struct SentenceRule
      : rule<many(append<&ast::Sentence::pattern>(term)) +
             many(append<&ast::Sentence::conditions>(ref<ConditionRule>)) +
             ((lit<"="> + many(append<&ast::Sentence::result>(term))) |
              (lit<","> +
               assign<&ast::Sentence::ending>(ref<BlockEndingRule>)))> {};

  struct FunctionDefRule
      : rule<option(flag<&ast::FunctionDef::entry>(lit<"$ENTRY">)) +
             assign<&ast::FunctionDef::name>(ref<Identifier>) + lit<"{"> +
             some(append<&ast::FunctionDef::sentences>(ref<SentenceRule>),
                  lit<";">) +
             option(lit<";">) + lit<"}">> {};

  struct ExternalDeclRule
      : rule<assign<&ast::ExternalDecl::keyword>(ref<EXTERNAL_KW>) +
             some(append<&ast::ExternalDecl::names>(ref<Identifier>),
                  lit<",">)> {};

  static constexpr auto item = (ref<FunctionDefRule> + option(lit<";">)) |
                               (ref<ExternalDeclRule> + lit<";">);

  struct ProgramRule : rule<some(append<&ast::Program::items>(item))> {};

  using entry = ProgramRule;
  using ignore = list<WS, LINE_COMMENT, BLOCK_COMMENT>;
};

}  // namespace refal
