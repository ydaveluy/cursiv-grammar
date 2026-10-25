#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "racketbsl/core/ast.hpp"

namespace racketbsl {
struct RacketBsl : cursiv::GrammarSpec {
  static constexpr auto whitespace = uc::Z | cr<R"( \t\r\n\f\v﻿)">;
  static constexpr auto delimiter = whitespace | cr<R"(()\[\]{}"'`,;)">;
  static constexpr auto plain = ~(delimiter | cr<R"(|\\)">);
  static constexpr auto symbol_piece =
      plain | (lit<"\\"> + any) | (lit<"|"> + many(cr<R"(^|)">) + lit<"|">);
  struct WS : terminal<some(whitespace)> {};
  struct COMMENT : terminal<lit<";"> + many(cr<R"(^\n)">)> {};
  struct BLOCK_COMMENT
      : terminal<lit<"#|"> +
                 many(ref<BLOCK_COMMENT> | (!lit<"#|"> + !lit<"|#"> + any)) +
                 lit<"|#">> {};

  struct ATOM
      : terminal<(!lit<"#"> + !(lit<"."> + !plain) + some(symbol_piece)) |
                 (lit<"#"> + cr<"eEiIbBoOdDxX%"> + many(symbol_piece))> {};
  struct KEYWORD : terminal<lit<"#:"> + many(symbol_piece)> {};
  struct BOOLEAN : terminal<(lit<"#true"> | lit<"#false"> | lit<"#t"> |
                             lit<"#f"> | lit<"#T"> | lit<"#F">)+!plain> {};
  static constexpr auto quoted =
      lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\)">) + lit<"\"">;
  struct HereTag;
  struct HERE_STRING
      : terminal<lit<"#<<"> + mark<HereTag>(many(cr<R"(^\n)">)) + lit<"\n"> +
                 many(!(same<HereTag> + !cr<R"(^\n)">)+many(cr<R"(^\n)">) +
                      lit<"\n">) +
                 same<HereTag> + !cr<R"(^\n)">> {};
  struct STRING : terminal<quoted> {};
  struct BYTES : terminal<lit<"#"> + quoted> {};
  struct REGEX : terminal<(lit<"#rx"> | lit<"#px">)+option(lit<"#">) + quoted> {
  };
  struct CHARACTER
      : terminal<lit<"#\\"> + ((lit<"u"> + repeat<0, 4>(cr<"0-9a-fA-F">)) |
                               (lit<"U"> + repeat<0, 8>(cr<"0-9a-fA-F">)) |
                               (cr<"0-7"> + repeat<0, 2>(cr<"0-7">)) |
                               (!cr<"uU"> + uc::L + many(uc::L)) | any)> {};
  struct GRAPH_REF : terminal<lit<"#"> + some(d) + lit<"#">> {};
  struct PREFIX : terminal<lit<"'"> | lit<"`"> | lit<",@"> | lit<","> |
                           lit<"#'"> | lit<"#`"> | lit<"#,@"> | lit<"#,"> |
                           lit<"#&"> | (lit<"#"> + some(d) + lit<"=">) |
                           (lit<"#"> + cr<"cC"> + cr<"iIsS">)> {};
  struct OPEN : terminal<option(lit<"#"> + (many(d) | lit<"s"> |
                                            (cr<"fF"> + cr<"lx"> + many(d)) |
                                            (cr<"hH"> + cr<"aA"> + cr<"sS"> +
                                             cr<"hH"> + many(cr<"a-zA-Z">)))) +
                         (lit<"("> | lit<"["> | lit<"{">)> {};
  struct CLOSE : terminal<lit<")"> | lit<"]"> | lit<"}">> {};
  struct DIRECTIVE
      : terminal<((lit<"#lang "> | lit<"#reader ">)+many(cr<R"(^\r\n)">)) |
                 (lit<"#!"> +
                  many(lit<"\\\n"> | (!lit<"\\\n"> + cr<R"(^\n)">)))> {};

  struct AtomRule : rule<assign<&ast::Atom::text>(ref<ATOM>)> {};
  struct StringRule
      : rule<assign<&ast::String::text>(ref<STRING> | ref<HERE_STRING>)> {};
  struct CharacterRule : rule<assign<&ast::Character::text>(ref<CHARACTER>)> {};
  struct BooleanRule : rule<assign<&ast::Boolean::text>(ref<BOOLEAN>)> {};
  struct KeywordRule : rule<assign<&ast::Keyword::text>(ref<KEYWORD>)> {};
  struct BytesRule : rule<assign<&ast::Bytes::text>(ref<BYTES>)> {};
  struct RegexRule : rule<assign<&ast::Regex::text>(ref<REGEX>)> {};
  struct GraphRefRule : rule<assign<&ast::GraphRef::text>(ref<GRAPH_REF>)> {};
  struct DirectiveRule : rule<assign<&ast::Directive::text>(ref<DIRECTIVE>)> {};
  struct ListRule;
  struct PrefixRule;
  struct DiscardRule;
  static constexpr auto value =
      ref<AtomRule> | ref<BooleanRule> | ref<StringRule> | ref<CharacterRule> |
      ref<KeywordRule> | ref<BytesRule> | ref<RegexRule> | ref<GraphRefRule> |
      ref<ListRule> | ref<PrefixRule>;
  static constexpr auto datum = value | ref<DiscardRule> | ref<DirectiveRule>;
  template <auto Member>
  static constexpr auto OperandInto =
      many(append<&ast::Datum::discards>(ref<DiscardRule>)) +
      assign<Member>(value);
  struct ListRule : rule<assign<&ast::List::opener>(ref<OPEN>) +
                         many(append<&ast::List::items>(datum)) +
                         option(lit<"."> + OperandInto<&ast::List::tail>) +
                         assign<&ast::List::closer>(ref<CLOSE>)> {};
  struct PrefixRule : rule<assign<&ast::Prefix::marker>(ref<PREFIX>) +
                           OperandInto<&ast::Prefix::value>> {};
  struct DiscardRule : rule<lit<"#;"> + OperandInto<&ast::Discard::value>> {};
  struct ProgramRule
      : rule<create<ast::Program> + many(append<&ast::Program::forms>(datum))> {
  };
  using entry = ProgramRule;
  using ignore = WS;
  using hidden = list<COMMENT, BLOCK_COMMENT>;
};
} // namespace racketbsl
