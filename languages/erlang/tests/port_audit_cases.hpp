#pragma once
#include <cstdio>
#include <string>
#include <variant>
#include "cursiv/core/parser/Parser.hpp"
#include "erlang/core/ast.hpp"
namespace port_audit_cases {
inline bool run(const cursiv::Parser& parser) {
  int failures = 0;
  auto require = [&](bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "erlang port audit: %s\n", message); ++failures; }
  };
  for (const std::string& source : {
    std::string("-module(audit_andalso_assoc).\nf(A,B,C)->A andalso B andalso C.\n"),
    std::string("-module(audit_attribute_prefix).\n-special(foo).\n-callback_extra(bar).\nf()->ok.\n"),
    std::string("-module(audit_char_newline).\nf()->$\n.\n"),
    std::string("-module(audit_chars).\nf()->[$😄,$\\x{1f600},$\\x41,$\\101,$\\1,$\\^A].\n"),
    std::string("-module(audit_comment_cr).\nf()->ok. % comment\rf()->not_a_function.\n"),
    std::string("\n"),
    std::string("-module(audit_macros).\n-define(DOUBLE(X),(X+X)).\nf()->{?MODULE,?DOUBLE(2)}.\n"),
    std::string("-module(audit_map_comprehension).\nf(M)->#{K=>V+1 || K:=V <- M}.\n"),
    std::string("-module(audit_maybe).\nf(F)->maybe {ok,X} ?= F(), X+1 else error -> 0; _ -> -1 end.\n"),
    std::string("-module(audit_numbers).\nf()->{1_000,16#ff_ff,1_000.0_1e+1_2,16#ff.ff#e+2}.\n"),
    std::string("-module(audit_orelse_assoc).\nf(A,B,C)->A orelse B orelse C.\n"),
    std::string("-module(audit_pp_empty).\n-define(FLAG,true).\n-ifdef(FLAG).\nf()->true.\n-else.\nf()->false.\n-endif.\n"),
    std::string("-module(audit_sigil_delims).\nf()->{~s(abc),~s[abc],~s<abc>,~s|abc|,~s#abc#,~s`abc`,~s'abc'}.\n"),
    std::string("-module(audit_sigil_triple).\nf()->~s\"\"\"\n  hello\\n\n  \"\"\".\n"),
    std::string("-module(audit_sigil_verbatim).\nf()->{~S\"ends\\\",~B/ends\\/,~s\"escaped\\\"quote\"}.\n"),
    std::string("-module(audit_sigils).\nf()->{~s\"hello\\n\",~S/hello\\n/,~b{hello},~\"hello\"}.\n"),
    std::string("-module(audit_spec_spacing).\n- spec f() -> integer().\nf()->1.\n"),
    std::string("-module(audit_strict_binary).\nf(B)->[X || <<X>> <:= B].\n"),
    std::string("-module(audit_strict_list).\nf(L)->[X || X <:- L].\n"),
    std::string("-module(audit_strict_map).\nf(M)->[K || K:=V <:- M].\n"),
    std::string("-module(audit_string_four_quotes).\nf()->\"\"\"\"\n  hello \"\"\" world\n  \"\"\"\".\n"),
    std::string("-module(audit_string_triple).\nf()->\"\"\"\n  hello\n  \"\"\".\n"),
    std::string("-module(audit_stringify).\n-define(TEXT(X),??X).\nf()->?TEXT(a+b).\n"),
    std::string("-module(audit_subtract).\nf()->1-2, 1.5-2.0, -3.\n"),
    std::string("-module(audit_triple_mid_quotes).\nf()->\"\"\"\n  mid \"\"\" quotes\n  \"\"\".\n"),
    std::string("-module(audit_triple_raw).\nf()->\"\"\"\n  raw\\n\n  \"\"\".\n"),
    std::string("-module(audit_whitespace).\nf() ->ok.\n"),
    std::string("-module(audit_zip).\nf(L,R)->[{X,Y} || X <- L && Y <- R].\n"),
  }) {
    const auto result = parser.parse(source);
    const bool clean = result.accepted && !result.cst().recovered() && result.diagnostics.empty() && result.value;
    if (!clean) std::fprintf(stderr, "erlang port audit input: %s\n", source.c_str());
    require(clean, "native-valid source must parse cleanly");
  }
  auto body = [&](const std::string& source, auto check) {
    const auto result = parser.parse(source);
    const auto* forms = result.template value_as<erlang::ast::Forms>();
    if (!forms || result.cst().recovered() || !result.diagnostics.empty()) return;
    for (const auto& form : forms->forms)
      if (const auto* fn = std::get_if<erlang::ast::FunctionDecl*>(&form)) {
        check((*fn)->clauses[0]->body); return;
      }
    require(false, "function AST expected");
  };
  using namespace erlang::ast;
  body("f(A,B,C)->1-2,1.5-2.0,-3,A andalso B andalso C,A orelse B orelse C.", [&](const auto& b) {
    require(b.size() == 5, "subtraction does not become signed literal tokens");
    if (b.size() != 5) return;
    const auto* first = cursiv::ast_cast<BinaryExpr>(b[0]);
    const auto* second = cursiv::ast_cast<BinaryExpr>(b[1]);
    require(first && second && first->op == Operator::sub && second->op == Operator::sub, "integer and float subtraction");
    require(cursiv::ast_cast<UnaryExpr>(b[2]), "negative integer retains unary syntax");
    for (int i : {3, 4}) {
      const auto* top = cursiv::ast_cast<BinaryExpr>(b[i]);
      require(top && cursiv::ast_cast<BinaryExpr>(top->right) && !cursiv::ast_cast<BinaryExpr>(top->left), "short circuit operators associate right");
    }
  });
  body("f(M,L,R,B)->#{K=>V || K:=V <- M},[X || X <:- L],[X || <<X>> <:= B],[{X,Y} || X<-L && Y<-R].", [&](const auto& b) {
    require(b.size() == 4, "all comprehensions retained");
    if (b.size() != 4) return;
    const auto* map = cursiv::ast_cast<MapCompr>(b[0]);
    const auto* list = cursiv::ast_cast<ListCompr>(b[1]);
    const auto* bin = cursiv::ast_cast<ListCompr>(b[2]);
    const auto* zip = cursiv::ast_cast<ListCompr>(b[3]);
    const auto* mq = map && map->quals.size() == 1 ? cursiv::ast_cast<LcClause>(map->quals[0]) : nullptr;
    require(map && map->head && mq && mq->mapValue && mq->op == "<-", "map head and map generator bindings");
    const auto* lq = list && list->quals.size() == 1 ? cursiv::ast_cast<LcClause>(list->quals[0]) : nullptr;
    const auto* bq = bin && bin->quals.size() == 1 ? cursiv::ast_cast<LcClause>(bin->quals[0]) : nullptr;
    require(lq && lq->op == "<:-" && bq && bq->op == "<:=", "strict generator spelling retained");
    const auto* zq = zip && zip->quals.size() == 1 ? cursiv::ast_cast<ZipQualifier>(zip->quals[0]) : nullptr;
    require(zq && zq->generators.size() == 2 && zq->generators[0]->op == "<-", "zip keeps generators in a single group");
  });
  body("f(F)->maybe {ok,X}?=F(),X else error->0; _ -> -1 end.", [&](const auto& b) {
    const auto* m = b.empty() ? nullptr : cursiv::ast_cast<MaybeExpr>(b[0]);
    require(m && m->body.size() == 2 && cursiv::ast_cast<MaybeMatch>(m->body[0]) && m->hasElse && m->elseClauses.size() == 2, "maybe body, match and else clauses retained");
  });
  body("f()->?MODULE,?DOUBLE(2),??Arg,~S|raw\\n|,\"a\" \"b\".", [&](const auto& b) {
    require(b.size() == 5, "macro, sigil and string AST");
    if (b.size() != 5) return;
    const auto* macro = cursiv::ast_cast<MacroExpr>(b[0]);
    const auto* call = cursiv::ast_cast<Call>(b[1]);
    const auto* stringify = cursiv::ast_cast<MacroExpr>(b[2]);
    const auto* sigil = cursiv::ast_cast<SigilLit>(b[3]);
    const auto* strings = cursiv::ast_cast<StringLit>(b[4]);
    require(macro && macro->name == "MODULE" && call && call->args.size() == 1 && cursiv::ast_cast<MacroExpr>(call->target), "macros preserve names and arguments");
    require(stringify && stringify->stringify && stringify->name == "Arg", "stringified macro argument");
    require(sigil && sigil->spelling == "~S|raw\\n|", "sigil raw syntax retained");
    require(strings && strings->parts.size() == 2 && strings->parts[0] == "\"a\"" && strings->parts[1] == "\"b\"", "adjacent strings retain spelling");
  });
  body("f()->\"\"\"\n  mid \"\"\" quotes\n  \"\"\".", [&](const auto& b) {
    const auto* str = b.size() == 1 ? cursiv::ast_cast<StringLit>(b[0]) : nullptr;
    require(str && str->parts.size() == 1 && str->parts[0].find("mid \"\"\" quotes") != std::string_view::npos, "triple quote delimiter closes only after line indentation");
  });
  {
    const std::string source = "f()->ok. % comment\rf()->not_a_function.\n";
    const auto result = parser.parse(source);
    const auto* forms = result.template value_as<Forms>();
    require(forms && forms->forms.size() == 1, "carriage return does not end an Erlang comment");
  }
  return failures == 0;
}
}  // namespace port_audit_cases
