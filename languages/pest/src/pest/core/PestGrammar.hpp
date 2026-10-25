#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "pest/core/ast.hpp"

namespace pest {

struct Pest : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  // Block comments nest.
  struct BLOCK_COMMENT
      : terminal<lit<"/*"> +
                 many(ref<BLOCK_COMMENT> | (!lit<"/*"> + !lit<"*/"> + any)) +
                 lit<"*/">> {};
  struct COMMENT
      : terminal<ref<BLOCK_COMMENT> |
                 (lit<"//"> + many(cr<R"(^\r\n)">))> {};

  struct IDENTIFIER : terminal<cr<"_a-zA-Z"> + many(cr<"_a-zA-Z0-9">)> {};

  static constexpr auto escape = lit<"\\"> + any;
  static constexpr auto string_body =
      lit<"\""> + many(escape | cr<R"(^"\\)">) + lit<"\"">;

  struct STRING : terminal<string_body> {};
  struct CHARACTER
      : terminal<lit<"'"> + many(escape | cr<R"(^'\\)">) + lit<"'">> {};

  struct NUMBER : terminal<some(d)> {};
  struct INTEGER : terminal<option(lit<"-">) + some(d)> {};

  struct POS_PRED_OP
      : terminal<lit<"&">, value<ast::PrefixOp::positive_predicate>> {};
  struct NEG_PRED_OP
      : terminal<lit<"!">, value<ast::PrefixOp::negative_predicate>> {};
  struct SEQ_OP : terminal<lit<"~">> {};
  struct CHOICE_OP : terminal<lit<"|">> {};

  struct SILENT_MOD : terminal<lit<"_">, value<ast::Modifier::silent>> {};
  struct ATOMIC_MOD : terminal<lit<"@">, value<ast::Modifier::atomic>> {};
  struct COMPOUND_ATOMIC_MOD
      : terminal<lit<"$">, value<ast::Modifier::compound_atomic>> {};
  struct NON_ATOMIC_MOD
      : terminal<lit<"!">, value<ast::Modifier::non_atomic>> {};

  struct ExpressionRule;

  struct OptionalOp : rule<create<ast::Optional_> + lit<"?">> {};
  struct ZeroOrMoreOp : rule<create<ast::ZeroOrMore> + lit<"*">> {};
  struct OneOrMoreOp : rule<create<ast::OneOrMore> + lit<"+">> {};
  struct RepeatExactOp
      : rule<create<ast::RepeatExact> + lit<"{"> +
             assign<&ast::RepeatExact::count>(ref<NUMBER>) + lit<"}">> {};
  struct RepeatMinOp
      : rule<create<ast::RepeatMin> + lit<"{"> +
             assign<&ast::RepeatMin::min>(ref<NUMBER>) + lit<","> +
             lit<"}">> {};
  struct RepeatMaxOp
      : rule<create<ast::RepeatMax> + lit<"{"> + lit<","> +
             assign<&ast::RepeatMax::max>(ref<NUMBER>) + lit<"}">> {};
  struct RepeatMinMaxOp
      : rule<create<ast::RepeatMinMax> + lit<"{"> +
             assign<&ast::RepeatMinMax::min>(ref<NUMBER>) + lit<","> +
             assign<&ast::RepeatMinMax::max>(ref<NUMBER>) + lit<"}">> {};

  struct TermRule
      : rule<option(lit<"#"> + assign<&ast::Term::tag>(ref<IDENTIFIER>) + lit<"=">) +
             many(append<&ast::Term::prefixes>(ref<POS_PRED_OP> |
                                                ref<NEG_PRED_OP>)) +
             assign<&ast::Term::item>(
                 (lit<"("> + ref<ExpressionRule> + lit<")">)  //
                 | create<ast::Push>(kw<"PUSH"> + lit<"("> +
                                      assign<&ast::Push::expr>(
                                          ref<ExpressionRule>) +
                                      lit<")">)  //
                 | create<ast::PushLiteral>(
                       kw<"PUSH_LITERAL"> + lit<"("> +
                       assign<&ast::PushLiteral::text>(ref<STRING>) + lit<")">)  //
                 | create<ast::PeekSlice>(
                       kw<"PEEK"> + lit<"["> +
                       option(assign<&ast::PeekSlice::start>(ref<INTEGER>)) +
                       lit<".."> +
                       option(assign<&ast::PeekSlice::end>(ref<INTEGER>)) +
                       lit<"]">)  //
                 | create<ast::RuleRef>(
                       assign<&ast::RuleRef::name>(ref<IDENTIFIER> | kw<"PEEK">))  //
                 | create<ast::StringLit>(
                       assign<&ast::StringLit::text>(ref<STRING>))  //
                 | create<ast::InsensitiveStringLit>(
                       lit<"^"> +
                       assign<&ast::InsensitiveStringLit::text>(
                           ref<STRING>))  //
                 | create<ast::CharRange>(
                       assign<&ast::CharRange::start>(ref<CHARACTER>) + lit<".."> +
                       assign<&ast::CharRange::end>(ref<CHARACTER>))) +
             many(append<&ast::Term::postfixes>(
                 ref<OptionalOp> | ref<ZeroOrMoreOp> | ref<OneOrMoreOp> |
                 ref<RepeatExactOp> | ref<RepeatMinOp> | ref<RepeatMaxOp> |
                 ref<RepeatMinMaxOp>))> {};

  // Sequence binds before choice.
  struct SequenceRule
      : rule<append<&ast::Sequence::terms>(ref<TermRule>) +
             many(ref<SEQ_OP> + append<&ast::Sequence::terms>(ref<TermRule>))> {
  };

  struct ExpressionRule
      : rule<option(ref<CHOICE_OP>) +
             append<&ast::Expression::alternatives>(ref<SequenceRule>) +
             many(ref<CHOICE_OP> +
                  append<&ast::Expression::alternatives>(ref<SequenceRule>))> {
  };

  struct RuleDeclRule
      : rule<assign<&ast::RuleDecl::name>(ref<IDENTIFIER>) + lit<"="> +
             option(assign<&ast::RuleDecl::modifier>(
                 ref<SILENT_MOD> | ref<ATOMIC_MOD> | ref<COMPOUND_ATOMIC_MOD> |
                 ref<NON_ATOMIC_MOD>)) +
             lit<"{"> +
             assign<&ast::RuleDecl::expression>(ref<ExpressionRule>) +
             lit<"}">> {};

  struct DocumentRule
      : rule<many(append<&ast::Document::rules>(ref<RuleDeclRule>))> {};

  using entry = DocumentRule;
  using ignore = WS;
  using hidden = list<COMMENT>;
};

}  // namespace pest
