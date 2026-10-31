#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "sexpression/core/ast.hpp"

namespace sexpression {
struct Sexpression : cursiv::GrammarSpec {
  static constexpr auto whitespace = uc::Z | cr<R"( \t\r\n\f\v﻿)">;
  static constexpr auto delimiter = whitespace | cr<R"(()\[\]{}"'`,;)">;
  static constexpr auto plain = ~(delimiter | cr<R"(|\\)">);
  static constexpr auto symbol_piece =
      plain | (lit<"\\"> + any) | (lit<"|"> + many(cr<R"(^|)">) + lit<"|">);
  struct WS : terminal<some(whitespace)> {};
  struct COMMENT
      : terminal<lit<";"> + many(cr<R"(^\n)">)> {};
  struct BLOCK_COMMENT
      : terminal<lit<"#|"> +
                 many(ref<BLOCK_COMMENT> | (!lit<"#|"> + !lit<"|#"> + any)) +
                 lit<"|#">> {};

  struct ATOM
      : terminal<(!lit<"#"> + !(lit<"."> + !plain) + some(symbol_piece)) |
                 (lit<"#"> + cr<"eEiIbBoOdDxX%"> + many(symbol_piece))> {};
  struct KEYWORD : terminal<lit<"#:"> + many(symbol_piece)> {};
  struct BOOLEAN : terminal<(lit<"#true"> | lit<"#false"> | lit<"#t"> |
                             lit<"#f"> | lit<"#T"> | lit<"#F">)+!(~delimiter)> {};
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
                               (!cr<"uU"> + uc::Alphabetic + many(uc::Alphabetic)) | any)> {};
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
  struct DIRECTIVE : terminal<(lit<"#lang "> | lit<"#reader "> |
                               lit<"#!">)+many(cr<R"(^\r\n)">)> {};

  struct Atom : rule<assign<&ast::Atom::text>(ref<ATOM>)> {};
  struct String
      : rule<assign<&ast::String::text>(ref<STRING> | ref<HERE_STRING>)> {};
  struct Character : rule<assign<&ast::Character::text>(ref<CHARACTER>)> {};
  struct Boolean : rule<assign<&ast::Boolean::text>(ref<BOOLEAN>)> {};
  struct Keyword : rule<assign<&ast::Keyword::text>(ref<KEYWORD>)> {};
  struct Bytes : rule<assign<&ast::Bytes::text>(ref<BYTES>)> {};
  struct Regex : rule<assign<&ast::Regex::text>(ref<REGEX>)> {};
  struct GraphRef : rule<assign<&ast::GraphRef::text>(ref<GRAPH_REF>)> {};
  struct Directive : rule<assign<&ast::Directive::text>(ref<DIRECTIVE>)> {};
  struct List;
  struct Prefix;
  struct Discard;
  static constexpr auto value =
      ref<Atom> | ref<Boolean> | ref<String> | ref<Character> |
      ref<Keyword> | ref<Bytes> | ref<Regex> | ref<GraphRef> |
      ref<List> | ref<Prefix>;
  static constexpr auto datum = value | ref<Discard> | ref<Directive>;
  template <auto Member>
  static constexpr auto OperandInto =
      many(append<&ast::Item::discards>(ref<Discard>)) +
      assign<Member>(value);
  struct List
      : rule<assign<&ast::List::opener>(ref<OPEN>) +
             many(append<&ast::List::items>(datum)) +
             option(lit<"."> + OperandInto<&ast::List::tail> +
                    option(flag<&ast::List::infix>(lit<".">) +
                           many(append<&ast::List::suffix>(datum)))) +
             assign<&ast::List::closer>(ref<CLOSE>)> {};
  struct Prefix : rule<assign<&ast::Prefix::marker>(ref<PREFIX>) +
                           OperandInto<&ast::Prefix::value>> {};
  struct Discard : rule<lit<"#;"> + OperandInto<&ast::Discard::value>> {};
  struct Sexpr
      : rule<create<ast::Sexpr> + many(append<&ast::Sexpr::items>(datum))> {};
  using entry = Sexpr;
  using ignored = WS;
  using hidden = list<COMMENT, BLOCK_COMMENT>;
};
} // namespace sexpression
