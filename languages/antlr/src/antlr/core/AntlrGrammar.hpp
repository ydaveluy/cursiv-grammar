#pragma once

#include <memory_resource>
#include <string_view>

#include "antlr/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace antlr {

inline constexpr auto stripOne = [](std::string_view t,
                                    std::pmr::memory_resource *) {
  return t.substr(1, t.size() - 2);
};
using Unwrapped = cursiv::text_conv<stripOne>;

inline constexpr auto keepText = [](std::string_view t,
                                    std::pmr::memory_resource *) { return t; };
using RawText = cursiv::text_conv<keepText>;

template <auto SuffixM, auto GreedyM, class Question, class Star, class Plus>
constexpr auto ebnfSuffix() {
  return cursiv::option(
      cursiv::assign<SuffixM>(cursiv::ref<Question> | cursiv::ref<Star> |
                              cursiv::ref<Plus>) +
      cursiv::option(cursiv::flag<GreedyM>(cursiv::lit<"?">)));
}

template <auto M, class OptionText> constexpr auto elementOptions() {
  return cursiv::option(
      cursiv::lit<"<"> +
      cursiv::option(cursiv::some(cursiv::append<M>(cursiv::ref<OptionText>),
                                  cursiv::lit<",">)) +
      cursiv::lit<">">);
}

template <auto M, class OptionsKw, class Decl> constexpr auto optionsBlock() {
  return cursiv::ref<OptionsKw> +
         cursiv::many(cursiv::append<M>(cursiv::ref<Decl>) + cursiv::lit<";">) +
         cursiv::lit<"}">;
}

struct Antlr : cursiv::GrammarSpec {
  struct BLOCK_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  struct LINE_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct WS : terminal<some(cr<R"( \t\r\n\f\uFEFF)">)> {};

  static constexpr auto IdStart = cr<R"(A-Za-z\u00C0-\u00D6\u00D8-\u00F6\u00F8-\u02FF\u0370-\u037D\u037F-\u1FFF\u200C-\u200D\u2070-\u218F\u2C00-\u2FEF\u3001-\uD7FF\uF900-\uFDCF\uFDF0-\uFEFE\uFF00-\uFFFD)">;
  static constexpr auto IdContinue =
      IdStart | cr<R"(0-9_\u00B7\u0300-\u036F\u203F-\u2040)">;
  static constexpr auto Uppercase = uc::Lu | cr<R"(\u2160-\u216F)">;
  struct TOKEN_REF : terminal<Uppercase + many(IdContinue)> {};
  struct RULE_REF : terminal<!Uppercase + IdStart + many(IdContinue)> {};
  static constexpr auto AnyId = ref<RULE_REF> | ref<TOKEN_REF>;

  struct INT : terminal<some(d)> {};

  static constexpr auto EscSeq = lit<"\\"> + any;

  struct STRING_LITERAL
      : terminal<lit<"'"> + many(EscSeq | cr<R"(^'\r\n\\)">) + lit<"'">> {};

  struct OPTIONS_KW
      : terminal<kw<"options"> + many(cr<R"( \t\r\n\f)">) + lit<"{">> {};
  struct TOKENS_KW
      : terminal<kw<"tokens"> + many(cr<R"( \t\r\n\f)">) + lit<"{">> {};
  struct CHANNELS_KW
      : terminal<kw<"channels"> + many(cr<R"( \t\r\n\f)">) + lit<"{">> {};

  struct CodeString
      : terminal<lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\)">) +
                 lit<"\"">> {};
  struct CodeChar
      : terminal<lit<"'"> + many((lit<"\\"> + any) | cr<R"(^'\\)">) +
                 lit<"'">> {};
  struct ActionText : terminal<some(cr<R"(^{}\\"'/)">)> {};
  struct ArgumentText : terminal<some(cr<R"(^[]"')">)> {};
  struct RawArgAction;
  struct RawAction;
  struct RawArgAction : terminal<lit<"["> +
                                 many(ref<CodeString> | ref<CodeChar> |
                                      ref<RawArgAction> | ref<ArgumentText>) +
                                 lit<"]">> {};
  struct RawAction
      : terminal<lit<"{"> +
                 many(ref<CodeString> | ref<CodeChar> | ref<BLOCK_COMMENT> |
                      ref<LINE_COMMENT> | (lit<"\\"> + any) | ref<RawAction> |
                      ref<ActionText> | lit<"/">) +
                 lit<"}">> {};
  struct ARG_ACTION : terminal<ref<RawArgAction>, Unwrapped> {};
  struct ACTION : terminal<ref<RawAction>, Unwrapped> {};

  struct LEXER_CHAR_SET
      : terminal<lit<"["> + many((lit<"\\"> + any) | cr<R"(^\\])">) + lit<"]">,
                 Unwrapped> {};

  struct EbnfQuestion : rule<lit<"?">, value<ast::EbnfKind::question>> {};
  struct EbnfStar : rule<lit<"*">, value<ast::EbnfKind::star>> {};
  struct EbnfPlus : rule<lit<"+">, value<ast::EbnfKind::plus>> {};

  struct QualifiedIdText : rule<AnyId + many(lit<"."> + AnyId), RawText> {};

  struct OptionValueText
      : rule<(AnyId + many(lit<"."> + AnyId)) | ref<STRING_LITERAL> |
                 ref<RawAction> | ref<INT>,
             RawText> {};

  struct ElementOptionText : rule<AnyId + many(lit<"."> + AnyId) +
                                      option(lit<"="> + ref<OptionValueText>),
                                  RawText> {};

  struct OptionDecl : rule<assign<&ast::Option::name>(AnyId) + lit<"="> +
                           assign<&ast::Option::value>(ref<OptionValueText>)> {
  };

  struct RuleRef;
  struct TokenRef;
  struct StringLit;
  struct CharRange;
  struct Wildcard;
  struct CharSetElem;
  struct NotSet;
  struct Block;
  struct Labeled;
  struct Action;
  struct Element;
  struct Alt;
  struct RuleActionDecl;
  struct ParserRule;
  struct LexerRule;

  static constexpr auto Suffix =
      ebnfSuffix<&ast::Element::suffix, &ast::Element::nonGreedy, EbnfQuestion,
                 EbnfStar, EbnfPlus>();
  static constexpr auto RuleRefBody =
      assign<&ast::RuleRef::name>(ref<RULE_REF>) +
      option(assign<&ast::RuleRef::args>(ref<ARG_ACTION>)) +
      elementOptions<&ast::RuleRef::options, ElementOptionText>();
  static constexpr auto TokenRefBody =
      assign<&ast::TokenRef::name>(ref<TOKEN_REF>) +
      elementOptions<&ast::TokenRef::options, ElementOptionText>();
  static constexpr auto StringLitBody =
      assign<&ast::StringLit::text>(ref<STRING_LITERAL>) +
      elementOptions<&ast::StringLit::options, ElementOptionText>();
  static constexpr auto CharRangeBody =
      assign<&ast::CharRange::from>(ref<STRING_LITERAL>) + lit<".."> +
      assign<&ast::CharRange::to>(ref<STRING_LITERAL>);
  static constexpr auto WildcardBody =
      lit<"."> + elementOptions<&ast::Wildcard::options, ElementOptionText>();
  static constexpr auto CharSetBody =
      assign<&ast::CharSet::body>(ref<LEXER_CHAR_SET>);
  static constexpr auto SetItem = create<ast::CharRange>(CharRangeBody) |
                                  create<ast::TokenRef>(TokenRefBody) |
                                  create<ast::StringLit>(StringLitBody) |
                                  create<ast::CharSet>(CharSetBody);
  static constexpr auto NotSetBody =
      lit<"~"> +
      (append<&ast::NotSet::items>(SetItem) |
       (lit<"("> + some(append<&ast::NotSet::items>(SetItem), lit<"|">) +
        lit<")">));
  static constexpr auto BareAtom = create<ast::RuleRef>(RuleRefBody) | SetItem |
                                   create<ast::NotSet>(NotSetBody) |
                                   create<ast::Wildcard>(WildcardBody);

  struct RuleRef : rule<create<ast::RuleRef>(RuleRefBody + Suffix)> {};
  struct TokenRef : rule<create<ast::TokenRef>(TokenRefBody + Suffix)> {};
  struct StringLit : rule<create<ast::StringLit>(StringLitBody + Suffix)> {};
  struct CharRange : rule<create<ast::CharRange>(CharRangeBody + Suffix)> {};
  struct Wildcard : rule<create<ast::Wildcard>(WildcardBody + Suffix)> {};
  struct CharSetElem : rule<create<ast::CharSet>(CharSetBody + Suffix)> {};
  struct NotSet : rule<create<ast::NotSet>(NotSetBody + Suffix)> {};

  static constexpr auto Atom = ref<CharRange> | ref<TokenRef> | ref<RuleRef> |
                               ref<StringLit> | ref<NotSet> | ref<Wildcard> |
                               ref<CharSetElem>;
  struct BareBlock;
  struct Labeled
      : rule<create<ast::Labeled>(
            assign<&ast::Labeled::name>(AnyId) +
            (flag<&ast::Labeled::isList>(lit<"+=">) |
             lit<"=">)+assign<&ast::Labeled::value>(BareAtom | ref<BareBlock>) +
            Suffix)> {};

  struct Action
      : rule<create<ast::Action>(
            assign<&ast::Action::text>(ref<ACTION>) +
            option(flag<&ast::Action::isPredicate>(lit<"?">)) +
            elementOptions<&ast::Action::options, ElementOptionText>())> {};

  struct Element : rule<ref<Labeled> | Atom | ref<Block> | ref<Action>> {};

  struct LexerCommandDecl
      : rule<create<ast::LexerCommand>(
            assign<&ast::LexerCommand::name>(AnyId) +
            option(lit<"("> +
                   assign<&ast::LexerCommand::arg>(AnyId | ref<INT>) +
                   lit<")">))> {};

  struct Alt
      : rule<create<ast::Alt>(
            elementOptions<&ast::Alt::options, ElementOptionText>() +
            many(append<&ast::Alt::elements>(ref<Element>)) +
            option(lit<"->"> +
                   some(append<&ast::Alt::commands>(ref<LexerCommandDecl>),
                        lit<",">)) +
            option(lit<"#"> + assign<&ast::Alt::label>(AnyId)))> {};

  struct RuleActionDecl
      : rule<create<ast::NamedAction>(
            lit<"@"> + assign<&ast::NamedAction::name>(AnyId) +
            assign<&ast::NamedAction::text>(ref<ACTION>))> {};

  static constexpr auto BlockBody =
      lit<"("> +
      option(
          option(optionsBlock<&ast::Block::options, OPTIONS_KW, OptionDecl>()) +
          many(append<&ast::Block::actions>(ref<RuleActionDecl>)) + lit<":">) +
      some(append<&ast::Block::alternatives>(ref<Alt>), lit<"|">) + lit<")">;
  struct BareBlock : rule<create<ast::Block>(BlockBody)> {};
  struct Block : rule<create<ast::Block>(BlockBody + Suffix)> {};

  struct NamedActionDecl
      : rule<create<ast::NamedAction>(
            lit<"@"> +
            option(assign<&ast::NamedAction::scope>(AnyId) + lit<"::">) +
            assign<&ast::NamedAction::name>(AnyId) +
            assign<&ast::NamedAction::text>(ref<ACTION>))> {};

  static constexpr auto ThrowsClause =
      kw<"throws"> +
      some(append<&ast::ParserRule::throwsTypes>(ref<QualifiedIdText>),
           lit<",">);

  struct CatchDecl
      : rule<create<ast::CatchClause>(
            kw<"catch"> + assign<&ast::CatchClause::argType>(ref<ARG_ACTION>) +
            assign<&ast::CatchClause::body>(ref<ACTION>))> {};

  struct ParserRule
      : rule<create<ast::ParserRule>(
            assign<&ast::ParserRule::name>(ref<RULE_REF>) +
            option(assign<&ast::ParserRule::args>(ref<ARG_ACTION>)) +
            option(kw<"returns"> +
                   assign<&ast::ParserRule::returnsText>(ref<ARG_ACTION>)) +
            option(ThrowsClause) +
            option(kw<"locals"> +
                   assign<&ast::ParserRule::locals>(ref<ARG_ACTION>)) +
            many(optionsBlock<&ast::ParserRule::ruleOptions, OPTIONS_KW,
                              OptionDecl>() |
                 append<&ast::ParserRule::ruleActions>(ref<RuleActionDecl>)) +
            lit<":"> +
            some(append<&ast::ParserRule::alternatives>(ref<Alt>), lit<"|">) +
            lit<";"> + many(append<&ast::ParserRule::catches>(ref<CatchDecl>)) +
            option(kw<"finally"> +
                   assign<&ast::ParserRule::finallyAction>(ref<ACTION>)))> {};

  struct LexerRule
      : rule<create<ast::LexerRule>(
            option(flag<&ast::LexerRule::fragment>(kw<"fragment">)) +
            assign<&ast::LexerRule::name>(ref<TOKEN_REF>) +
            option(optionsBlock<&ast::LexerRule::ruleOptions, OPTIONS_KW,
                                OptionDecl>()) +
            lit<":"> +
            some(append<&ast::LexerRule::alternatives>(ref<Alt>), lit<"|">) +
            lit<";">)> {};

  struct ModeDecl
      : rule<create<ast::Mode>(
            kw<"mode"> + assign<&ast::Mode::name>(AnyId) + lit<";"> +
            many(append<&ast::Mode::rules>(ref<LexerRule>)))> {};

  struct ImportOne
      : rule<create<ast::Import>(assign<&ast::Import::label>(AnyId) + lit<"="> +
                                 assign<&ast::Import::name>(AnyId)) |
             create<ast::Import>(assign<&ast::Import::name>(AnyId))> {};

  static constexpr auto ImportClause =
      kw<"import"> +
      some(append<&ast::Grammar::imports>(ref<ImportOne>), lit<",">) + lit<";">;

  static constexpr auto TokensClause =
      ref<TOKENS_KW> +
      option(some(append<&ast::Grammar::tokens>(AnyId), lit<",">) +
             option(lit<",">)) +
      lit<"}">;

  static constexpr auto ChannelsClause =
      ref<CHANNELS_KW> +
      option(some(append<&ast::Grammar::channels>(AnyId), lit<",">) +
             option(lit<",">)) +
      lit<"}">;

  struct GrammarDecl
      : rule<create<ast::Grammar>(
            ((flag<&ast::Grammar::lexerOnly>(kw<"lexer">) +
              kw<"grammar">) |
             (flag<&ast::Grammar::parserOnly>(kw<"parser">) +
              kw<"grammar">) |
             kw<"grammar">)+assign<&ast::Grammar::name>(AnyId) +
            lit<";"> +
            many(
                optionsBlock<&ast::Grammar::options, OPTIONS_KW, OptionDecl>() |
                ImportClause | TokensClause | ChannelsClause |
                append<&ast::Grammar::actions>(ref<NamedActionDecl>)) +
            many(append<&ast::Grammar::rules>(ref<LexerRule> |
                                              ref<ParserRule>)) +
            many(append<&ast::Grammar::modes>(ref<ModeDecl>)))> {};

  using entry = GrammarDecl;
  using ignore = WS;
  using hidden = list<LINE_COMMENT, BLOCK_COMMENT>;
};

} // namespace antlr
