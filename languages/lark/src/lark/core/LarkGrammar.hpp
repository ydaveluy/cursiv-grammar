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
  struct OP : terminal<cr<"+*?">> {};
  struct NUMBER : terminal<option(cr<"+-">) + some(uc::Nd)> {};
  struct STRING
      : terminal<lit<"\""> + many((lit<"\\"> + cr<R"(^\n)">) |
                                  cr<R"(^"\\\n)">) +
                 lit<"\""> + option(lit<"i">)> {};
  struct REGEXP
      : terminal<lit<"/"> + some((lit<"\\"> + any) | cr<R"(^/\\)">) +
                 lit<"/"> + many(cr<"imslux">)> {};

  static constexpr auto name = ref<RULE> | ref<TOKEN>;
  struct ExpansionsRule;
  struct ValueRule
      : rule<(assign<&ast::LiteralRange::lo>(ref<STRING>) + lit<".."> +
              assign<&ast::LiteralRange::hi>(ref<STRING>)) |
             (assign<&ast::TemplateUsage::name>(name) + lit<"{"> +
              append<&ast::TemplateUsage::args>(ref<ValueRule>) +
              many(lit<","> + append<&ast::TemplateUsage::args>(ref<ValueRule>)) +
              lit<"}">) |
             assign<&ast::NameRef::name>(name) |
             assign<&ast::Literal::text>(ref<REGEXP> | ref<STRING>)> {};
  struct AtomRule
      : rule<(lit<"("> + assign<&ast::Atom::content>(ref<ExpansionsRule>) +
              lit<")">) |
             (flag<&ast::Atom::maybe>(lit<"[">) +
              assign<&ast::Atom::content>(ref<ExpansionsRule>) + lit<"]">) |
             assign<&ast::Atom::content>(ref<ValueRule>)> {};
  struct ExprRule
      : rule<assign<&ast::Expr::atom>(ref<AtomRule>) +
             option(assign<&ast::Expr::op>(ref<OP>) |
                    (lit<"~"> + assign<&ast::Expr::repeatMin>(ref<NUMBER>) +
                     option(lit<".."> + assign<&ast::Expr::repeatMax>(ref<NUMBER>))))> {};
  struct ExpansionRule : rule<many(append<&ast::Expansion::items>(ref<ExprRule>))> {};
  struct AliasRule
      : rule<assign<&ast::Alias::expansion>(ref<ExpansionRule>) +
             option(lit<"->"> + assign<&ast::Alias::label>(name))> {};
  struct ExpansionsRule
      : rule<append<&ast::Expansions::alternatives>(ref<AliasRule>) +
             many(ref<OR> + append<&ast::Expansions::alternatives>(ref<AliasRule>))> {};
  struct DeclarationRule
      : rule<option(assign<&ast::Declaration::modifiers>(ref<MODIFIERS>)) +
             (flag<&ast::Declaration::terminal>(
                  assign<&ast::Declaration::name>(ref<TOKEN>)) |
              assign<&ast::Declaration::name>(ref<RULE>)) +
             option(lit<"{"> + append<&ast::Declaration::params>(name) +
                    many(lit<","> + append<&ast::Declaration::params>(name)) + lit<"}">) +
             option(lit<"."> + assign<&ast::Declaration::priority>(ref<NUMBER>)) +
             lit<":"> + assign<&ast::Declaration::body>(ref<ExpansionsRule>)> {};
  struct IgnoreRule : rule<lit<"%ignore"> + assign<&ast::IgnoreStmt::body>(ref<ExpansionsRule>)> {};
  struct ImportRule
      : rule<lit<"%import"> + option(flag<&ast::ImportStmt::relative>(lit<".">)) +
             append<&ast::ImportStmt::path>(name) +
             many(lit<"."> + append<&ast::ImportStmt::path>(name)) +
             option((lit<"->"> + assign<&ast::ImportStmt::alias>(name)) |
                    (lit<"("> + append<&ast::ImportStmt::names>(name) +
                     many(lit<","> + append<&ast::ImportStmt::names>(name)) + lit<")">))> {};
  struct OverrideRule
      : rule<(lit<"%override"> | flag<&ast::OverrideStmt::extend>(lit<"%extend">)) +
             assign<&ast::OverrideStmt::declaration>(ref<DeclarationRule>)> {};
  struct DeclareRule : rule<lit<"%declare"> + some(append<&ast::DeclareStmt::names>(name))> {};
  struct StartRule
      : rule<create<ast::Start> +
             many(ref<NL> |
                  (append<&ast::Start::items>(ref<DeclarationRule> | ref<IgnoreRule> |
                                             ref<ImportRule> | ref<OverrideRule> | ref<DeclareRule>) +
                   (ref<NL> | &eof)))> {};
  using entry = StartRule;
  using ignore = list<WS, CONTINUATION>;
  using hidden = COMMENT;
};

}  // namespace lark
