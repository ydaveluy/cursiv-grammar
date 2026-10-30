#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "lark/core/ast.hpp"

namespace lark {

struct Lark : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t)">)> {};
  static constexpr auto blank = s | uc::Z | cr<R"(\u001c-\u001f\u0085)">;
  static constexpr auto newline =
      some(option(lit<"\r">) + lit<"\n">) + many(blank);
  struct NL : terminal<newline> {};
  struct OR : terminal<lit<"|"> | (newline + lit<"|">)> {};
  struct COMMENT : terminal<many(blank) + (lit<"//"> | lit<"#">) +
                            many(cr<R"(^\n)">)> {};
  struct CONTINUATION : terminal<lit<"\\"> + many(lit<" ">) + lit<"\n">> {};
  struct RULE : terminal<option(lit<"_">) + cr<"a-z"> + many(cr<"_a-z0-9">)> {};
  struct TOKEN : terminal<option(lit<"_">) + cr<"A-Z"> + many(cr<"_A-Z0-9">)> {};
  struct MODIFIERS : terminal<some(cr<"!?">)> {};
  struct OP : terminal<cr<"+*"> | lit<"?", !cr<"a-z_">>> {};
  struct NUMBER : terminal<option(cr<"+-">) + some(uc::Nd)> {};
  struct STRING
      : terminal<lit<"\""> + many((lit<"\\"> + cr<R"(^\n)">) |
                                  cr<R"(^"\\\n)">) +
                 lit<"\""> + option(lit<"i">)> {};
  struct REGEXP
      : terminal<lit<"/"> + some((lit<"\\"> + any) | cr<R"(^/\\)">) +
                 lit<"/"> + many(cr<"imslux">)> {};

  static constexpr auto name = ref<RULE> | ref<TOKEN>;
  struct Expansions;
  struct Value
      : rule<(assign<&ast::LiteralRange::lo>(ref<STRING>) + lit<".."> +
              assign<&ast::LiteralRange::hi>(ref<STRING>)) |
             (assign<&ast::TemplateUsage::name>(name) + lit<"{"> +
              some(append<&ast::TemplateUsage::args>(ref<Value>), lit<",">) +
              lit<"}">) |
             assign<&ast::NameRef::name>(name) |
             assign<&ast::Literal::text>(ref<REGEXP> | ref<STRING>)> {};
  struct Atom
      : rule<(lit<"("> + assign<&ast::Atom::content>(ref<Expansions>) +
              lit<")">) |
             (flag<&ast::Atom::maybe>(lit<"[">) +
              assign<&ast::Atom::content>(ref<Expansions>) + lit<"]">) |
             assign<&ast::Atom::content>(ref<Value>)> {};
  struct Expr
      : rule<assign<&ast::Expr::atom>(ref<Atom>) +
             option(assign<&ast::Expr::op>(ref<OP>) |
                    (lit<"~"> + assign<&ast::Expr::repeatMin>(ref<NUMBER>) +
                     option(lit<".."> + assign<&ast::Expr::repeatMax>(ref<NUMBER>))))> {};
  struct Expansion : rule<many(append<&ast::Expansion::items>(ref<Expr>))> {};
  struct Alias
      : rule<assign<&ast::Alias::expansion>(ref<Expansion>) +
             option(lit<"->"> + assign<&ast::Alias::label>(name))> {};
  struct Expansions
      : rule<append<&ast::Expansions::alternatives>(ref<Alias>) +
             many(ref<OR> + append<&ast::Expansions::alternatives>(ref<Alias>))> {};
  struct Declaration
      : rule<option(assign<&ast::Declaration::modifiers>(ref<MODIFIERS>)) +
             (flag<&ast::Declaration::terminal>(
                  assign<&ast::Declaration::name>(ref<TOKEN>)) |
              assign<&ast::Declaration::name>(ref<RULE>)) +
             option(lit<"{"> + append<&ast::Declaration::params>(name) +
                    many(lit<","> + append<&ast::Declaration::params>(name)) + lit<"}">) +
             option(lit<"."> + assign<&ast::Declaration::priority>(ref<NUMBER>)) +
             lit<":"> + assign<&ast::Declaration::body>(ref<Expansions>)> {};
  struct Ignore : rule<lit<"%ignore"> + assign<&ast::IgnoreStmt::body>(ref<Expansions>)> {};
  struct Import
      : rule<lit<"%import"> + option(flag<&ast::ImportStmt::relative>(lit<".">)) +
             append<&ast::ImportStmt::path>(name) +
             many(lit<"."> + append<&ast::ImportStmt::path>(name)) +
             option((lit<"->"> + assign<&ast::ImportStmt::alias>(name)) |
                    (lit<"("> + append<&ast::ImportStmt::names>(name) +
                     many(lit<","> + append<&ast::ImportStmt::names>(name)) + lit<")">))> {};
  struct Override
      : rule<(lit<"%override"> | flag<&ast::OverrideStmt::extend>(lit<"%extend">)) +
             assign<&ast::OverrideStmt::declaration>(ref<Declaration>)> {};
  struct Declare : rule<lit<"%declare"> + some(append<&ast::DeclareStmt::names>(name))> {};
  struct Start
      : rule<create<ast::Start> +
             many(ref<NL> |
                  (append<&ast::Start::items>(ref<Declaration> | ref<Ignore> |
                                             ref<Import> | ref<Override> | ref<Declare>) +
                   (ref<NL> | &eof)))> {};
  using entry = Start;
  using ignored = list<WS, CONTINUATION>;
  using hidden = COMMENT;
};

}  // namespace lark
