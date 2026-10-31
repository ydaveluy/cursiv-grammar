// The refal language: its grammar.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "refal/core/ast.hpp"
#include "refal/core/StringValue.hpp"

namespace refal {

struct Refal : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \r\n\t)">)> {};
  static constexpr auto comment_line = lit<"*"> + many(cr<R"(^\r\n)">);
  // The newline belongs to the comment token, so '*' is in column one.
  // Blank lines before it may contain horizontal whitespace.
  struct LINE_COMMENT
      : terminal<some(many(cr<" \t">) + eol) + comment_line> {};
  struct INITIAL_COMMENT : terminal<comment_line> {};
  struct BLOCK_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  struct IDENT : terminal<cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9_-">)> {};
  struct DIGITS : terminal<some(d)> {};

  static constexpr auto escape = lit<"\\"> +
      (cr<R"(\\'"nrt()<>)"> | (lit<"x"> + repeat<2>(cr<"0-9a-fA-F">)));
  struct STRING : terminal<as<decode_string>(
      lit<"\""> + many(escape | cr<R"(^"\\\r\n)">) + lit<"\"">)> {};
  struct STRING2 : terminal<as<decode_string>(
      lit<"'"> + many(escape | cr<R"(^'\\\r\n)">) + lit<"'">)> {};

  struct Identifier : rule<ref<IDENT> | ref<STRING>> {};
  struct Index : rule<ref<Identifier> | ref<DIGITS>> {};

  // The letter and the dot are one token: `s`/`t`/`e` alone, or as the first
  // letters of a longer word, still lex as IDENT (longest match), so a
  // symbol named `s` needs no guard of its own.
  struct SVAR : terminal<as<ast::VarKind::s>(lit<"s"> + lit<".">)> {};
  struct TVAR : terminal<as<ast::VarKind::t>(lit<"t"> + lit<".">)> {};
  struct EVAR : terminal<as<ast::VarKind::e>(lit<"e"> + lit<".">)> {};

  struct EXTERNAL_KW
      : terminal<(lit<"$EXTERNAL"> | lit<"$EXTERN"> | lit<"$EXTRN">) + !cr<"a-zA-Z0-9_-">> {};

  struct Symbol;
  struct Variable;
  struct Call;
  struct Group;
  struct PatternGroup;
  static constexpr auto term =
      ref<Symbol> | ref<Variable> | ref<Group> | ref<Call>;
  static constexpr auto patternTerm =
      ref<Symbol> | ref<Variable> | ref<PatternGroup>;

  struct Symbol : rule<
      assign<&ast::Symbol::text>(ref<Identifier>) |
      create<ast::NumberSymbol>(assign<&ast::Symbol::text>(ref<DIGITS>)) |
      create<ast::Characters>(assign<&ast::Symbol::text>(ref<STRING2>))> {};
  struct Group : rule<create<ast::Group>(lit<"("> +
      many(append<&ast::Group::body>(term)) + lit<")">)> {};
  struct PatternGroup : rule<create<ast::Group>(lit<"("> +
      many(append<&ast::Group::body>(patternTerm)) + lit<")">)> {};

  struct Variable
      : rule<assign<&ast::Variable::kind>(ref<SVAR> | ref<TVAR> | ref<EVAR>) +
             gap::none + assign<&ast::Variable::index>(ref<Index>)> {};

  // Absorb an arithmetic activation name with '<', before '*' can open
  // a line comment. body[0] remains the function symbol for existing users.
  struct OPERATOR_CALL : terminal<as<[](std::string_view t) {
      return t.substr(1);
    }>(lit<"<"> + cr<"+*/-">)> {};
  struct OperatorName : rule<assign<&ast::Symbol::text>(ref<OPERATOR_CALL>)> {};
  struct FunctionName : rule<assign<&ast::Symbol::text>(ref<Identifier>)> {};
  struct Call : rule<
      (append<&ast::Call::body>(ref<OperatorName>) |
       (lit<"<"> + gap::none + append<&ast::Call::body>(ref<FunctionName>))) +
      many(append<&ast::Call::body>(term)) + lit<">">> {};

  struct Condition
      : rule<lit<","> + many(append<&ast::Condition::arg>(term)) + lit<":"> +
             many(append<&ast::Condition::pattern>(patternTerm))> {};

  struct Sentence;
  struct BlockEnding
      : rule<many(append<&ast::BlockEnding::arg>(term)) + lit<":"> +
             lit<"{"> +
             some(append<&ast::BlockEnding::sentences>(ref<Sentence>),
                  lit<";">) +
             option(lit<";">) + lit<"}">> {};

  struct Sentence
      : rule<many(append<&ast::Sentence::pattern>(patternTerm)) +
             many(append<&ast::Sentence::conditions>(ref<Condition>)) +
             ((lit<"="> + many(append<&ast::Sentence::result>(term))) |
              (lit<","> +
               assign<&ast::Sentence::ending>(ref<BlockEnding>)))> {};

  struct FunctionDef
      : rule<option(flag<&ast::FunctionDef::entry>(lit<"$ENTRY", !cr<"a-zA-Z0-9_-">>)) +
             assign<&ast::FunctionDef::name>(ref<Identifier>) + lit<"{"> +
             some(append<&ast::FunctionDef::sentences>(ref<Sentence>),
                  lit<";">) +
             option(lit<";">) + lit<"}">> {};

  struct ExternalDecl
      : rule<assign<&ast::ExternalDecl::keyword>(ref<EXTERNAL_KW>) +
             some(append<&ast::ExternalDecl::names>(ref<Identifier>),
                  lit<",">)> {};

  static constexpr auto item = (ref<FunctionDef> + option(lit<";">)) |
                               (ref<ExternalDecl> + lit<";">);

  struct Program : rule<option(gap::none + ref<INITIAL_COMMENT>) +
                        some(append<&ast::Program::items>(item))> {};

  using entry = Program;
  using ignored = WS;
  using hidden = list<LINE_COMMENT, BLOCK_COMMENT>;
};

}  // namespace refal
