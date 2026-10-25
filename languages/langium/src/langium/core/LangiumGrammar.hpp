#pragma once

#include <cmath>
#include <cstdlib>
#include <memory_resource>
#include <string>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "langium/core/ast.hpp"

namespace langium {

struct Langium : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \n\r\t\v\f\uFEFF\u2028\u2029)"> | uc::Zs)> {
  };
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  inline static constexpr auto id_value = [](std::string_view text) {
    return text.starts_with('^') ? text.substr(1) : text;
  };
  struct ID
      : terminal<option(lit<"^">) + cr<"_a-zA-Z"> + many(cr<"_a-zA-Z0-9">),
                 text_conv<id_value>> {};

  inline static constexpr auto string_value =
      [](std::string_view text, std::pmr::memory_resource* mem) {
        text = text.substr(1, text.size() - 2);
        if (text.find('\\') == text.npos) return text;
        const auto room = cursiv::text_room(mem, text.size());
        std::size_t size = 0;
        for (std::size_t i = 0; i < text.size(); ++i) {
          char c = text[i];
          if (c == '\\' && i + 1 < text.size()) {
            c = text[++i];
            switch (c) {
              case 'b':
                c = '\b';
                break;
              case 'f':
                c = '\f';
                break;
              case 'n':
                c = '\n';
                break;
              case 'r':
                c = '\r';
                break;
              case 't':
                c = '\t';
                break;
              case 'v':
                c = '\v';
                break;
              case '0':
                c = '\0';
                break;
              default:
                break;
            }
          }
          room[size++] = c;
        }
        return std::string_view(room.data(), size);
      };
  struct STRING
      : terminal<
            (lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\)">) + lit<"\"">) |
                (lit<"'"> + many((lit<"\\"> + any) | cr<R"(^'\\)">) + lit<"'">),
            text_conv<string_value>> {};

  inline static constexpr auto number_value = [](std::string_view t) -> double {
    if (t == "NaN") return std::nan("");
    if (t == "Infinity") return HUGE_VAL;
    if (t == "-Infinity") return -HUGE_VAL;
    return std::strtod(std::string(t).c_str(), nullptr);
  };
  struct NUMBER
      : terminal<lit<"NaN"> |
                     (option(lit<"-">) +
                      ((((many(cr<"0-9">) + lit<"."> + some(cr<"0-9">)) |
                         some(cr<"0-9">)) +
                        option(cr<"Ee"> + option(cr<"+-">) + some(cr<"0-9">))) |
                       lit<"Infinity">)),
                 text_conv<number_value>> {};

  static constexpr auto re_escape = lit<"\\"> + any;
  static constexpr auto re_class =
      lit<"["> + many(re_escape | cr<R"(^\\\r\n])">) + lit<"]">;
  struct RegexLiteral
      : terminal<lit<"/", !cr<"/*+?">> +
                 some(re_escape | re_class | cr<R"(^/[\\\r\n)">) + lit<"/"> +
                 many(cr<"a-z">)> {};

  static constexpr auto PrimitiveType =
      kw<"string"> | kw<"number"> | kw<"boolean"> | kw<"Date"> | kw<"bigint">;
  static constexpr auto FeatureName =
      kw<"infix"> | kw<"on"> | kw<"left"> | kw<"right"> | kw<"assoc"> |
      kw<"current"> | kw<"entry"> | kw<"extends"> | kw<"false"> |
      kw<"fragment"> | kw<"grammar"> | kw<"hidden"> | kw<"import"> |
      kw<"interface"> | kw<"returns"> | kw<"terminal"> | kw<"true"> |
      kw<"type"> | kw<"infer"> | kw<"infers"> | kw<"with"> | PrimitiveType |
      ref<ID>;
  static constexpr auto Predicate = lit<"=>"> | lit<"->">;
  static constexpr auto Cardinality = lit<"?"> | lit<"*"> | lit<"+">;

  struct StringLiteralRule
      : rule<assign<&ast::StringLiteral::value>(ref<STRING>)> {};
  struct NumberLiteralRule
      : rule<assign<&ast::NumberLiteral::value>(ref<NUMBER>)> {};
  struct BooleanLiteralRule
      : rule<create<ast::BooleanLiteral> +
             (flag<&ast::BooleanLiteral::value>(kw<"true">) |
              kw<"false">)> {};
  struct ArrayLiteralRule;
  static constexpr auto ValueLiteral =
      ref<StringLiteralRule> | ref<NumberLiteralRule> |
      ref<BooleanLiteralRule> | ref<ArrayLiteralRule>;
  struct ArrayLiteralRule
      : rule<lit<"["> +
             many(append<&ast::ArrayLiteral::elements>(ValueLiteral),
                  lit<",">) +
             lit<"]">> {};

  struct TypeDefinitionRule;
  struct SimpleTypeRule
      : rule<(lit<"("> + ref<TypeDefinitionRule> + lit<")">) |
             create<ast::SimpleType>(
                 assign<&ast::SimpleType::typeRef>(ref<ID>) |
                 assign<&ast::SimpleType::primitiveType>(PrimitiveType) |
                 assign<&ast::SimpleType::stringType>(ref<STRING>))> {};
  struct ReferenceTypeRule
      : rule<(lit<"@"> +
              create<ast::ReferenceType>(
                  assign<&ast::ReferenceType::referenceType>(
                      ref<SimpleTypeRule>) +
                  option(flag<&ast::ReferenceType::isMulti>(lit<"+">)))) |
             ref<SimpleTypeRule>> {};
  struct ArrayTypeRule
      : rule<create<ast::ArrayType>(
                 assign<&ast::ArrayType::elementType>(ref<ReferenceTypeRule>) +
                 lit<"["> + lit<"]">) |
             ref<ReferenceTypeRule>> {};
  struct TypeDefinitionRule
      : rule<create<ast::UnionType>(
                 append<&ast::UnionType::types>(ref<ArrayTypeRule>) +
                 some(lit<"|"> +
                      append<&ast::UnionType::types>(ref<ArrayTypeRule>))) |
             ref<ArrayTypeRule>> {};
  struct TypeAttributeRule
      : rule<assign<&ast::TypeAttribute::name>(FeatureName) +
             option(flag<&ast::TypeAttribute::isOptional>(lit<"?">)) +
             lit<":"> +
             assign<&ast::TypeAttribute::type>(ref<TypeDefinitionRule>) +
             option(lit<"="> +
                    assign<&ast::TypeAttribute::defaultValue>(ValueLiteral)) +
             option(lit<";">)> {};
  struct InterfaceRule
      : rule<kw<"interface"> + assign<&ast::Interface::name>(ref<ID>) +
             option(
                 kw<"extends"> +
                 some(append<&ast::Interface::superTypes>(ref<ID>), lit<",">)) +
             lit<"{"> +
             many(append<&ast::Interface::attributes>(ref<TypeAttributeRule>)) +
             lit<"}"> + option(lit<";">)> {};
  struct TypeRule
      : rule<kw<"type"> + assign<&ast::Type::name>(ref<ID>) + lit<"="> +
             assign<&ast::Type::type>(ref<TypeDefinitionRule>) +
             option(lit<";">)> {};

  struct DisjunctionRule;
  struct ParameterReferenceRule
      : rule<assign<&ast::ParameterReference::parameter>(ref<ID>)> {};
  struct AtomRule : rule<ref<ParameterReferenceRule> |
                         (lit<"("> + ref<DisjunctionRule> + lit<")">) |
                         ref<BooleanLiteralRule>> {};
  struct NegationRule
      : rule<create<ast::Negation>(
                 lit<"!"> + assign<&ast::Negation::value>(ref<NegationRule>)) |
             ref<AtomRule>> {};
  struct ConjunctionRule
      : rule<ref<NegationRule> +
             many(nest<ast::Conjunction, &ast::Conjunction::left>(
                 lit<"&"> +
                 assign<&ast::Conjunction::right>(ref<NegationRule>)))> {};
  struct DisjunctionRule
      : rule<ref<ConjunctionRule> +
             many(nest<ast::Disjunction, &ast::Disjunction::left>(
                 lit<"|"> +
                 assign<&ast::Disjunction::right>(ref<ConjunctionRule>)))> {};

  struct AlternativesRule;
  struct KeywordRule : rule<assign<&ast::Keyword::value>(ref<STRING>)> {};
  struct PredicatedKeywordRule
      : rule<assign<&ast::AbstractElement::predicate>(Predicate) +
             assign<&ast::Keyword::value>(ref<STRING>)> {};
  struct NamedArgumentRule
      : rule<option(assign<&ast::NamedArgument::parameter>(ref<ID>) +
                    flag<&ast::NamedArgument::calledByName>(lit<"=">)) +
             assign<&ast::NamedArgument::value>(ref<DisjunctionRule>)> {};
  static constexpr auto RuleCallBody =
      assign<&ast::RuleCall::rule>(ref<ID>) +
      option(lit<"<"> +
             some(append<&ast::RuleCall::arguments>(ref<NamedArgumentRule>),
                  lit<",">) +
             lit<">">);
  struct RuleCallRule : rule<RuleCallBody> {};
  struct PredicatedRuleCallRule
      : rule<assign<&ast::AbstractElement::predicate>(Predicate) +
             RuleCallBody> {};
  struct CrossReferenceableTerminalRule
      : rule<ref<KeywordRule> | ref<RuleCallRule>> {};
  struct CrossReferenceRule
      : rule<lit<"["> +
             option(flag<&ast::CrossReference::isMulti>(lit<"+">)) +
             assign<&ast::CrossReference::type>(ref<ID>) +
             option(
                 (flag<&ast::CrossReference::deprecatedSyntax>(lit<"|">) |
                  lit<":">)+assign<&ast::CrossReference::
                                       terminal>(
                     ref<CrossReferenceableTerminalRule>)) +
             lit<"]">> {};
  struct ParenthesizedAssignableElementRule;
  static constexpr auto AssignableTerminal =
      ref<KeywordRule> | ref<RuleCallRule> |
      ref<ParenthesizedAssignableElementRule> | ref<CrossReferenceRule>;
  struct ParenthesizedAssignableElementRule
      : rule<lit<"("> +
             (create<ast::Alternatives>(
                  append<&ast::Alternatives::elements>(AssignableTerminal) +
                  some(lit<"|"> + append<&ast::Alternatives::elements>(
                                      AssignableTerminal))) |
              AssignableTerminal) +
             lit<")">> {};
  struct AssignmentRule
      : rule<option(assign<&ast::AbstractElement::predicate>(Predicate)) +
             assign<&ast::Assignment::feature>(FeatureName) +
             assign<&ast::Assignment::op>(lit<"+="> | lit<"="> | lit<"?=">) +
             assign<&ast::Assignment::terminal>(AssignableTerminal)> {};
  struct PredicatedGroupRule
      : rule<assign<&ast::AbstractElement::predicate>(Predicate) + lit<"("> +
             append<&ast::Group::elements>(ref<AlternativesRule>) + lit<")">> {
  };
  struct ParenthesizedElementRule
      : rule<create<ast::Group>(
            lit<"("> + append<&ast::Group::elements>(ref<AlternativesRule>) +
            lit<")">)> {};
  struct EndOfFileRule : rule<create<ast::EndOfFile> + kw<"EOF">> {};
  static constexpr auto AbstractTerminal =
      ref<KeywordRule> | ref<RuleCallRule> | ref<ParenthesizedElementRule> |
      ref<PredicatedKeywordRule> | ref<PredicatedRuleCallRule> |
      ref<PredicatedGroupRule> | ref<EndOfFileRule>;
  struct InferredTypeRule
      : rule<(kw<"infer"> |
              kw<"infers">)+assign<&ast::InferredType::name>(ref<ID>)> {};
  struct ActionRule
      : rule<lit<"{"> +
             (assign<&ast::Action::type>(ref<ID>) |
              assign<&ast::Action::inferredType>(ref<InferredTypeRule>)) +
             option(lit<"."> + assign<&ast::Action::feature>(FeatureName) +
                    assign<&ast::Action::op>(lit<"="> | lit<"+=">) +
                    kw<"current">) +
             lit<"}">> {};
  struct AbstractTokenWithCardinalityRule
      : rule<(ref<AssignmentRule> | AbstractTerminal) +
             option(assign<&ast::AbstractElement::cardinality>(Cardinality))> {
  };
  static constexpr auto AbstractToken =
      ref<AbstractTokenWithCardinalityRule> | ref<ActionRule>;
  struct GroupRule
      : rule<create<ast::Group>(
                 append<&ast::Group::elements>(AbstractToken) +
                 some(append<&ast::Group::elements>(AbstractToken))) |
             AbstractToken> {};
  struct UnorderedGroupRule
      : rule<create<ast::UnorderedGroup>(
                 append<&ast::UnorderedGroup::elements>(ref<GroupRule>) +
                 some(lit<"&"> +
                      append<&ast::UnorderedGroup::elements>(ref<GroupRule>))) |
             ref<GroupRule>> {};
  struct ConditionalBranchRule
      : rule<create<ast::Group>(
                 lit<"<"> +
                 assign<&ast::Group::guardCondition>(ref<DisjunctionRule>) +
                 lit<">"> +
                 some(append<&ast::Group::elements>(AbstractToken))) |
             ref<UnorderedGroupRule>> {};
  struct AlternativesRule
      : rule<create<ast::Alternatives>(
                 append<&ast::Alternatives::elements>(
                     ref<ConditionalBranchRule>) +
                 some(lit<"|"> + append<&ast::Alternatives::elements>(
                                     ref<ConditionalBranchRule>))) |
             ref<ConditionalBranchRule>> {};

  struct TerminalAlternativesRule;
  struct TerminalGroupRule;
  struct TerminalTokenRule;
  static constexpr auto CharacterRangeBody =
      assign<&ast::CharacterRange::left>(ref<KeywordRule>) +
      option(lit<".."> + assign<&ast::CharacterRange::right>(ref<KeywordRule>));
  struct CharacterRangeRule : rule<CharacterRangeBody> {};
  static constexpr auto TerminalRuleCallBody =
      assign<&ast::TerminalRuleCall::rule>(ref<ID>);
  struct TerminalRuleCallRule : rule<TerminalRuleCallBody> {};
  static constexpr auto WildcardBody = create<ast::Wildcard> + lit<".">;
  struct WildcardRule : rule<WildcardBody> {};
  static constexpr auto RegexTokenBody =
      assign<&ast::RegexToken::regex>(ref<RegexLiteral>);
  struct RegexTokenRule : rule<RegexTokenBody> {};
  struct TerminalTokenElementRule;
  static constexpr auto NegatedTokenBody =
      lit<"!"> +
      assign<&ast::NegatedToken::terminal>(ref<TerminalTokenElementRule>);
  struct NegatedTokenRule : rule<NegatedTokenBody> {};
  static constexpr auto UntilTokenBody =
      lit<"->"> +
      assign<&ast::UntilToken::terminal>(ref<TerminalTokenElementRule>);
  struct UntilTokenRule : rule<UntilTokenBody> {};

  static constexpr auto LookaheadMark =
      lit<"?="> | lit<"?!"> | lit<"?<="> | lit<"?<!">;
  struct ParenthesizedTerminalElementRule;
  static constexpr auto LookaheadElement =
      create<ast::CharacterRange>(CharacterRangeBody) |
      create<ast::TerminalRuleCall>(TerminalRuleCallBody) |
      create<ast::TerminalAlternatives>(
          append<&ast::TerminalAlternatives::elements>(
              ref<ParenthesizedTerminalElementRule>)) |
      create<ast::NegatedToken>(NegatedTokenBody) |
      create<ast::UntilToken>(UntilTokenBody) |
      create<ast::RegexToken>(RegexTokenBody) | WildcardBody;
  static constexpr auto LookaheadToken = LookaheadElement;
  static constexpr auto LookaheadGroup =
      create<ast::TerminalGroup>(
          append<&ast::TerminalGroup::elements>(ref<TerminalTokenRule>) +
          some(append<&ast::TerminalGroup::elements>(ref<TerminalTokenRule>))) |
      LookaheadToken |
      create<ast::TerminalGroup>(
          append<&ast::TerminalGroup::elements>(ref<TerminalTokenRule>));
  static constexpr auto LookaheadAlternatives =
      create<ast::TerminalAlternatives>(
          append<&ast::TerminalAlternatives::elements>(ref<TerminalGroupRule>) +
          some(lit<"|"> + append<&ast::TerminalAlternatives::elements>(
                              ref<TerminalGroupRule>))) |
      LookaheadGroup;
  struct ParenthesizedTerminalElementRule
      : rule<(lit<"("> +
              assign<&ast::TerminalElement::lookahead>(LookaheadMark) +
              LookaheadAlternatives +
              flag<&ast::TerminalElement::parenthesized>(lit<")">)) |
             create<ast::TerminalGroup>(
                 lit<"("> +
                 append<&ast::TerminalGroup::elements>(
                     ref<TerminalAlternativesRule>) +
                 flag<&ast::TerminalElement::parenthesized>(lit<")">))> {};
  struct TerminalTokenElementRule
      : rule<ref<CharacterRangeRule> | ref<TerminalRuleCallRule> |
             ref<ParenthesizedTerminalElementRule> | ref<NegatedTokenRule> |
             ref<UntilTokenRule> | ref<RegexTokenRule> | ref<WildcardRule>> {};
  struct TerminalTokenRule
      : rule<ref<TerminalTokenElementRule> +
             option(assign<&ast::AbstractElement::cardinality>(Cardinality))> {
  };
  struct TerminalGroupRule
      : rule<create<ast::TerminalGroup>(
                 append<&ast::TerminalGroup::elements>(ref<TerminalTokenRule>) +
                 some(append<&ast::TerminalGroup::elements>(
                     ref<TerminalTokenRule>))) |
             ref<TerminalTokenRule>> {};
  struct TerminalAlternativesRule
      : rule<create<ast::TerminalAlternatives>(
                 append<&ast::TerminalAlternatives::elements>(
                     ref<TerminalGroupRule>) +
                 some(lit<"|"> + append<&ast::TerminalAlternatives::elements>(
                                     ref<TerminalGroupRule>))) |
             ref<TerminalGroupRule>> {};

  struct ReturnTypeRule
      : rule<assign<&ast::ReturnType::name>(PrimitiveType | ref<ID>)> {};
  struct TerminalRuleRule
      : rule<option(flag<&ast::TerminalRule::hidden>(kw<"hidden">)) +
             kw<"terminal"> +
             ((flag<&ast::TerminalRule::fragment>(kw<"fragment">) +
               assign<&ast::AbstractRule::name>(ref<ID>)) |
              (assign<&ast::AbstractRule::name>(ref<ID>) +
               option(kw<"returns"> +
                      assign<&ast::TerminalRule::type>(ref<ReturnTypeRule>)))) +
             lit<":"> +
             assign<&ast::TerminalRule::definition>(
                 ref<TerminalAlternativesRule>) +
             lit<";">> {};
  struct ParameterRule : rule<assign<&ast::Parameter::name>(ref<ID>)> {};
  static constexpr auto RuleNameAndParams =
      assign<&ast::AbstractRule::name>(ref<ID>) +
      option(
          lit<"<"> +
          many(append<&ast::AbstractParserRule::parameters>(ref<ParameterRule>),
               lit<",">) +
          lit<">">);
  static constexpr auto ParserReturnType = option(
      kw<"returns"> +
          (assign<&ast::AbstractParserRule::returnType>(ref<ID>) |
           assign<&ast::AbstractParserRule::dataType>(PrimitiveType)) |
      assign<&ast::AbstractParserRule::inferredType>(ref<InferredTypeRule>));
  static constexpr auto HiddenClause =
      option(flag<&ast::ParserRule::definesHiddenTokens>(kw<"hidden">) +
             lit<"("> +
             many(append<&ast::ParserRule::hiddenTokens>(ref<ID>), lit<",">) +
             lit<")">);
  struct ParserRuleRule
      : rule<create<ast::ParserRule>(
            option(flag<&ast::ParserRule::entry>(kw<"entry">) |
                   flag<&ast::ParserRule::fragment>(kw<"fragment">)) +
            RuleNameAndParams +
            option(flag<&ast::ParserRule::wildcard>(lit<"*">)) +
            ParserReturnType + HiddenClause + lit<":"> +
            assign<&ast::ParserRule::definition>(ref<AlternativesRule>) +
            lit<";">)> {};
  struct InfixRuleOperatorListRule
      : rule<option(assign<&ast::InfixRuleOperatorList::associativity>(
                        kw<"left"> | kw<"right">) +
                    kw<"assoc">) +
             some(append<&ast::InfixRuleOperatorList::operators>(
                      ref<KeywordRule>),
                  lit<"|">)> {};
  struct InfixRuleOperatorsRule
      : rule<some(append<&ast::InfixRuleOperators::precedences>(
                      ref<InfixRuleOperatorListRule>),
                  lit<">">)> {};
  struct InfixRuleRule
      : rule<create<ast::InfixRule>(
            kw<"infix"> + RuleNameAndParams + kw<"on"> +
            assign<&ast::InfixRule::call>(ref<RuleCallRule>) +
            ParserReturnType + lit<":"> +
            assign<&ast::InfixRule::operators>(ref<InfixRuleOperatorsRule>) +
            lit<";">)> {};
  struct GrammarImportRule
      : rule<kw<"import"> + assign<&ast::GrammarImport::path>(ref<STRING>) +
             option(lit<";">)> {};
  struct GrammarRule
      : rule<create<ast::Grammar>(
            option(flag<&ast::Grammar::isDeclared>(kw<"grammar">) +
                   assign<&ast::Grammar::name>(ref<ID>) +
                   option(kw<"with"> +
                          some(append<&ast::Grammar::usedGrammars>(ref<ID>),
                               lit<",">)) +
                   option(flag<&ast::Grammar::definesHiddenTokens>(
                              kw<"hidden">) +
                          lit<"("> +
                          many(append<&ast::Grammar::hiddenTokens>(ref<ID>),
                               lit<",">) +
                          lit<")">)) +
            many(append<&ast::Grammar::imports>(ref<GrammarImportRule>)) +
            many(append<&ast::Grammar::rules>(ref<ParserRuleRule> |
                                              ref<TerminalRuleRule> |
                                              ref<InfixRuleRule>) |
                 append<&ast::Grammar::interfaces>(ref<InterfaceRule>) |
                 append<&ast::Grammar::types>(ref<TypeRule>)))> {};

  using entry = GrammarRule;
  using ignore = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace langium
