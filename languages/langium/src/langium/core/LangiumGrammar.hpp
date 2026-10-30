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

  static constexpr auto PrimitiveType =
      kw<"string"> | kw<"number"> | kw<"boolean"> | kw<"Date"> | kw<"bigint">;
  static constexpr auto FeatureWord =
      kw<"infix"> | kw<"on"> | kw<"left"> | kw<"right"> | kw<"assoc"> |
      kw<"current"> | kw<"entry"> | kw<"extends"> | kw<"false"> |
      kw<"fragment"> | kw<"grammar"> | kw<"hidden"> | kw<"import"> |
      kw<"interface"> | kw<"returns"> | kw<"terminal"> | kw<"true"> |
      kw<"type"> | kw<"infer"> | kw<"infers"> | kw<"with"> | PrimitiveType;
  static constexpr auto ReservedWord = FeatureWord | kw<"EOF">;

  inline static constexpr auto id_value = [](std::string_view text) {
    return text.starts_with('^') ? text.substr(1) : text;
  };
  struct ID : terminal<as<id_value>(!ReservedWord + option(lit<"^">) + cr<"_a-zA-Z"> +
                                    many(cr<"_a-zA-Z0-9">))> {};

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
  static constexpr auto string_escape = lit<"\\"> + cr<R"(^\r\n\u2028\u2029)">;
  struct STRING
      : terminal<as<string_value>(
            (lit<"\""> + many(string_escape | cr<R"(^"\\)">) + lit<"\"">) |
            (lit<"'"> + many(string_escape | cr<R"(^'\\)">) + lit<"'">))> {
  };

  inline static constexpr auto number_value = [](std::string_view t) -> double {
    if (t == "NaN") return std::nan("");
    if (t == "Infinity") return HUGE_VAL;
    if (t == "-Infinity") return -HUGE_VAL;
    return std::strtod(std::string(t).c_str(), nullptr);
  };
  struct NUMBER
      : terminal<as<number_value>(
            lit<"NaN"> |
            (option(lit<"-">) +
             ((((many(cr<"0-9">) + lit<"."> + some(cr<"0-9">)) |
                some(cr<"0-9">)) +
               option(cr<"Ee"> + option(cr<"+-">) + some(cr<"0-9">))) |
              lit<"Infinity">)))> {};

  static constexpr auto re_escape = string_escape;
  static constexpr auto re_class =
      lit<"["> + many(re_escape | cr<R"(^\\\r\n])">) + lit<"]">;
  struct RegexLiteral
      : terminal<lit<"/", !cr<"/*+?">> +
                 some(re_escape | re_class | cr<R"(^/[\\\r\n)">) + lit<"/"> +
                 many(cr<"a-z">)> {};

  static constexpr auto FeatureName = FeatureWord | ref<ID>;
  static constexpr auto Predicate = lit<"=>"> | lit<"->">;
  static constexpr auto Cardinality = lit<"?"> | lit<"*"> | lit<"+">;

  struct StringLiteral
      : rule<assign<&ast::StringLiteral::value>(ref<STRING>)> {};
  struct NumberLiteral
      : rule<assign<&ast::NumberLiteral::value>(ref<NUMBER>)> {};
  struct BooleanLiteral
      : rule<create<ast::BooleanLiteral> +
             (flag<&ast::BooleanLiteral::value>(kw<"true">) |
              kw<"false">)> {};
  struct ArrayLiteral;
  static constexpr auto ValueLiteral =
      ref<StringLiteral> | ref<NumberLiteral> |
      ref<BooleanLiteral> | ref<ArrayLiteral>;
  struct ArrayLiteral
      : rule<lit<"["> +
             many(append<&ast::ArrayLiteral::elements>(ValueLiteral),
                  lit<",">) +
             lit<"]">> {};

  struct TypeDefinition;
  struct SimpleType
      : rule<(lit<"("> + ref<TypeDefinition> + lit<")">) |
             create<ast::SimpleType>(
                 assign<&ast::SimpleType::typeRef>(ref<ID>) |
                 assign<&ast::SimpleType::primitiveType>(PrimitiveType) |
                 assign<&ast::SimpleType::stringType>(ref<STRING>))> {};
  struct ReferenceType
      : rule<(lit<"@"> +
              create<ast::ReferenceType>(
                  assign<&ast::ReferenceType::referenceType>(
                      ref<SimpleType>) +
                  option(flag<&ast::ReferenceType::isMulti>(lit<"+">)))) |
             ref<SimpleType>> {};
  struct ArrayType
      : rule<create<ast::ArrayType>(
                 assign<&ast::ArrayType::elementType>(ref<ReferenceType>) +
                 lit<"["> + lit<"]">) |
             ref<ReferenceType>> {};
  struct TypeDefinition
      : rule<create<ast::UnionType>(
                 append<&ast::UnionType::types>(ref<ArrayType>) +
                 some(lit<"|"> +
                      append<&ast::UnionType::types>(ref<ArrayType>))) |
             ref<ArrayType>> {};
  struct TypeAttribute
      : rule<assign<&ast::TypeAttribute::name>(FeatureName) +
             option(flag<&ast::TypeAttribute::isOptional>(lit<"?">)) +
             lit<":"> +
             assign<&ast::TypeAttribute::type>(ref<TypeDefinition>) +
             option(lit<"="> +
                    assign<&ast::TypeAttribute::defaultValue>(ValueLiteral)) +
             option(lit<";">)> {};
  struct Interface
      : rule<kw<"interface"> + assign<&ast::Interface::name>(ref<ID>) +
             option(
                 kw<"extends"> +
                 some(append<&ast::Interface::superTypes>(ref<ID>), lit<",">)) +
             lit<"{"> +
             many(append<&ast::Interface::attributes>(ref<TypeAttribute>)) +
             lit<"}"> + option(lit<";">)> {};
  struct Type
      : rule<kw<"type"> + assign<&ast::Type::name>(ref<ID>) + lit<"="> +
             assign<&ast::Type::type>(ref<TypeDefinition>) +
             option(lit<";">)> {};

  struct Disjunction;
  struct ParameterReference
      : rule<assign<&ast::ParameterReference::parameter>(ref<ID>)> {};
  struct Atom : rule<ref<ParameterReference> |
                         (lit<"("> + ref<Disjunction> + lit<")">) |
                         ref<BooleanLiteral>> {};
  struct Negation
      : rule<create<ast::Negation>(
                 lit<"!"> + assign<&ast::Negation::value>(ref<Negation>)) |
             ref<Atom>> {};
  struct Conjunction
      : rule<ref<Negation> +
             many(nest<ast::Conjunction, &ast::Conjunction::left>(
                 lit<"&"> +
                 assign<&ast::Conjunction::right>(ref<Negation>)))> {};
  struct Disjunction
      : rule<ref<Conjunction> +
             many(nest<ast::Disjunction, &ast::Disjunction::left>(
                 lit<"|"> +
                 assign<&ast::Disjunction::right>(ref<Conjunction>)))> {};

  struct Alternatives;
  struct Keyword : rule<assign<&ast::Keyword::value>(ref<STRING>)> {};
  struct PredicatedKeyword
      : rule<assign<&ast::AbstractElement::predicate>(Predicate) +
             assign<&ast::Keyword::value>(ref<STRING>)> {};
  struct NamedArgument
      : rule<option(assign<&ast::NamedArgument::parameter>(ref<ID>) +
                    flag<&ast::NamedArgument::calledByName>(lit<"=">)) +
             assign<&ast::NamedArgument::value>(ref<Disjunction>)> {};
  static constexpr auto RuleCallBody =
      assign<&ast::RuleCall::rule>(ref<ID>) +
      option(lit<"<"> +
             some(append<&ast::RuleCall::arguments>(ref<NamedArgument>),
                  lit<",">) +
             lit<">">);
  struct RuleCall : rule<RuleCallBody> {};
  struct PredicatedRuleCall
      : rule<assign<&ast::AbstractElement::predicate>(Predicate) +
             RuleCallBody> {};
  struct CrossReferenceableTerminal
      : rule<ref<Keyword> | ref<RuleCall>> {};
  struct CrossReference
      : rule<lit<"["> +
             option(flag<&ast::CrossReference::isMulti>(lit<"+">)) +
             assign<&ast::CrossReference::type>(ref<ID>) +
             option(
                 (flag<&ast::CrossReference::deprecatedSyntax>(lit<"|">) |
                  lit<":">)+assign<&ast::CrossReference::
                                       terminal>(
                     ref<CrossReferenceableTerminal>)) +
             lit<"]">> {};
  struct ParenthesizedAssignableElement;
  static constexpr auto AssignableTerminal =
      ref<Keyword> | ref<RuleCall> |
      ref<ParenthesizedAssignableElement> | ref<CrossReference>;
  struct ParenthesizedAssignableElement
      : rule<lit<"("> +
             (create<ast::Alternatives>(
                  append<&ast::Alternatives::elements>(AssignableTerminal) +
                  some(lit<"|"> + append<&ast::Alternatives::elements>(
                                      AssignableTerminal))) |
              AssignableTerminal) +
             lit<")">> {};
  struct Assignment
      : rule<option(assign<&ast::AbstractElement::predicate>(Predicate)) +
             assign<&ast::Assignment::feature>(FeatureName) +
             assign<&ast::Assignment::op>(lit<"+="> | lit<"="> | lit<"?=">) +
             assign<&ast::Assignment::terminal>(AssignableTerminal)> {};
  struct PredicatedGroup
      : rule<assign<&ast::AbstractElement::predicate>(Predicate) + lit<"("> +
             append<&ast::Group::elements>(ref<Alternatives>) + lit<")">> {
  };
  struct ParenthesizedElement
      : rule<create<ast::Group>(
            lit<"("> + append<&ast::Group::elements>(ref<Alternatives>) +
            lit<")">)> {};
  struct EndOfFile : rule<create<ast::EndOfFile> + kw<"EOF">> {};
  static constexpr auto AbstractTerminal =
      ref<Keyword> | ref<RuleCall> | ref<ParenthesizedElement> |
      ref<PredicatedKeyword> | ref<PredicatedRuleCall> |
      ref<PredicatedGroup> | ref<EndOfFile>;
  struct InferredType
      : rule<(kw<"infer"> |
              kw<"infers">)+assign<&ast::InferredType::name>(ref<ID>)> {};
  struct Action
      : rule<lit<"{"> +
             (assign<&ast::Action::type>(ref<ID>) |
              assign<&ast::Action::inferredType>(ref<InferredType>)) +
             option(lit<"."> + assign<&ast::Action::feature>(FeatureName) +
                    assign<&ast::Action::op>(lit<"="> | lit<"+=">) +
                    kw<"current">) +
             lit<"}">> {};
  struct AbstractTokenWithCardinality
      : rule<(ref<Assignment> | AbstractTerminal) +
             option(assign<&ast::AbstractElement::cardinality>(Cardinality))> {
  };
  static constexpr auto AbstractToken =
      ref<AbstractTokenWithCardinality> | ref<Action>;
  struct Group
      : rule<create<ast::Group>(
                 append<&ast::Group::elements>(AbstractToken) +
                 some(append<&ast::Group::elements>(AbstractToken))) |
             AbstractToken> {};
  struct UnorderedGroup
      : rule<create<ast::UnorderedGroup>(
                 append<&ast::UnorderedGroup::elements>(ref<Group>) +
                 some(lit<"&"> +
                      append<&ast::UnorderedGroup::elements>(ref<Group>))) |
             ref<Group>> {};
  struct ConditionalBranch
      : rule<create<ast::Group>(
                 lit<"<"> +
                 assign<&ast::Group::guardCondition>(ref<Disjunction>) +
                 lit<">"> +
                 some(append<&ast::Group::elements>(AbstractToken))) |
             ref<UnorderedGroup>> {};
  struct Alternatives
      : rule<create<ast::Alternatives>(
                 append<&ast::Alternatives::elements>(
                     ref<ConditionalBranch>) +
                 some(lit<"|"> + append<&ast::Alternatives::elements>(
                                     ref<ConditionalBranch>))) |
             ref<ConditionalBranch>> {};

  struct TerminalAlternatives;
  struct TerminalGroup;
  struct TerminalToken;
  static constexpr auto CharacterRangeBody =
      assign<&ast::CharacterRange::left>(ref<Keyword>) +
      option(lit<".."> + assign<&ast::CharacterRange::right>(ref<Keyword>));
  struct CharacterRange : rule<CharacterRangeBody> {};
  static constexpr auto TerminalRuleCallBody =
      assign<&ast::TerminalRuleCall::rule>(ref<ID>);
  struct TerminalRuleCall : rule<TerminalRuleCallBody> {};
  static constexpr auto WildcardBody = create<ast::Wildcard> + lit<".">;
  struct Wildcard : rule<WildcardBody> {};
  static constexpr auto RegexTokenBody =
      assign<&ast::RegexToken::regex>(ref<RegexLiteral>);
  struct RegexToken : rule<RegexTokenBody> {};
  struct TerminalTokenElement;
  static constexpr auto NegatedTokenBody =
      lit<"!"> +
      assign<&ast::NegatedToken::terminal>(ref<TerminalTokenElement>);
  struct NegatedToken : rule<NegatedTokenBody> {};
  static constexpr auto UntilTokenBody =
      lit<"->"> +
      assign<&ast::UntilToken::terminal>(ref<TerminalTokenElement>);
  struct UntilToken : rule<UntilTokenBody> {};

  static constexpr auto LookaheadMark =
      lit<"?="> | lit<"?!"> | lit<"?<="> | lit<"?<!">;
  struct ParenthesizedTerminalElement;
  static constexpr auto LookaheadElement =
      create<ast::CharacterRange>(CharacterRangeBody) |
      create<ast::TerminalRuleCall>(TerminalRuleCallBody) |
      create<ast::TerminalAlternatives>(
          append<&ast::TerminalAlternatives::elements>(
              ref<ParenthesizedTerminalElement>)) |
      create<ast::NegatedToken>(NegatedTokenBody) |
      create<ast::UntilToken>(UntilTokenBody) |
      create<ast::RegexToken>(RegexTokenBody) | WildcardBody;
  static constexpr auto LookaheadToken = LookaheadElement;
  static constexpr auto LookaheadGroup =
      create<ast::TerminalGroup>(
          append<&ast::TerminalGroup::elements>(ref<TerminalToken>) +
          some(append<&ast::TerminalGroup::elements>(ref<TerminalToken>))) |
      LookaheadToken |
      create<ast::TerminalGroup>(
          append<&ast::TerminalGroup::elements>(ref<TerminalToken>));
  static constexpr auto LookaheadAlternatives =
      create<ast::TerminalAlternatives>(
          append<&ast::TerminalAlternatives::elements>(ref<TerminalGroup>) +
          some(lit<"|"> + append<&ast::TerminalAlternatives::elements>(
                              ref<TerminalGroup>))) |
      LookaheadGroup;
  struct ParenthesizedTerminalElement
      : rule<(lit<"("> +
              assign<&ast::TerminalElement::lookahead>(LookaheadMark) +
              LookaheadAlternatives +
              flag<&ast::TerminalElement::parenthesized>(lit<")">)) |
             create<ast::TerminalGroup>(
                 lit<"("> +
                 append<&ast::TerminalGroup::elements>(
                     ref<TerminalAlternatives>) +
                 flag<&ast::TerminalElement::parenthesized>(lit<")">))> {};
  struct TerminalTokenElement
      : rule<ref<CharacterRange> | ref<TerminalRuleCall> |
             ref<ParenthesizedTerminalElement> | ref<NegatedToken> |
             ref<UntilToken> | ref<RegexToken> | ref<Wildcard>> {};
  struct TerminalToken
      : rule<ref<TerminalTokenElement> +
             option(assign<&ast::AbstractElement::cardinality>(Cardinality))> {
  };
  struct TerminalGroup
      : rule<create<ast::TerminalGroup>(
                 append<&ast::TerminalGroup::elements>(ref<TerminalToken>) +
                 some(append<&ast::TerminalGroup::elements>(
                     ref<TerminalToken>))) |
             ref<TerminalToken>> {};
  struct TerminalAlternatives
      : rule<create<ast::TerminalAlternatives>(
                 append<&ast::TerminalAlternatives::elements>(
                     ref<TerminalGroup>) +
                 some(lit<"|"> + append<&ast::TerminalAlternatives::elements>(
                                     ref<TerminalGroup>))) |
             ref<TerminalGroup>> {};

  struct ReturnType
      : rule<assign<&ast::ReturnType::name>(PrimitiveType | ref<ID>)> {};
  struct TerminalRule
      : rule<option(flag<&ast::TerminalRule::hidden>(kw<"hidden">)) +
             kw<"terminal"> +
             ((flag<&ast::TerminalRule::fragment>(kw<"fragment">) +
               assign<&ast::AbstractRule::name>(ref<ID>)) |
              (assign<&ast::AbstractRule::name>(ref<ID>) +
               option(kw<"returns"> +
                      assign<&ast::TerminalRule::type>(ref<ReturnType>)))) +
             lit<":"> +
             assign<&ast::TerminalRule::definition>(
                 ref<TerminalAlternatives>) +
             lit<";">> {};
  struct Parameter : rule<assign<&ast::Parameter::name>(ref<ID>)> {};
  static constexpr auto RuleNameAndParams =
      assign<&ast::AbstractRule::name>(ref<ID>) +
      option(
          lit<"<"> +
          many(append<&ast::AbstractParserRule::parameters>(ref<Parameter>),
               lit<",">) +
          lit<">">);
  static constexpr auto ParserReturnType = option(
      (kw<"returns"> +
          (assign<&ast::AbstractParserRule::returnType>(ref<ID>) |
           assign<&ast::AbstractParserRule::dataType>(PrimitiveType))) |
      assign<&ast::AbstractParserRule::inferredType>(ref<InferredType>));
  static constexpr auto HiddenClause =
      option(flag<&ast::ParserRule::definesHiddenTokens>(kw<"hidden">) +
             lit<"("> +
             many(append<&ast::ParserRule::hiddenTokens>(ref<ID>), lit<",">) +
             lit<")">);
  struct ParserRule
      : rule<create<ast::ParserRule>(
            option(flag<&ast::ParserRule::entry>(kw<"entry">) |
                   flag<&ast::ParserRule::fragment>(kw<"fragment">)) +
            RuleNameAndParams +
            option(flag<&ast::ParserRule::wildcard>(lit<"*">)) +
            ParserReturnType + HiddenClause + lit<":"> +
            assign<&ast::ParserRule::definition>(ref<Alternatives>) +
            lit<";">)> {};
  struct InfixRuleOperatorList
      : rule<option(assign<&ast::InfixRuleOperatorList::associativity>(
                        kw<"left"> | kw<"right">) +
                    kw<"assoc">) +
             some(append<&ast::InfixRuleOperatorList::operators>(
                      ref<Keyword>),
                  lit<"|">)> {};
  struct InfixRuleOperators
      : rule<some(append<&ast::InfixRuleOperators::precedences>(
                      ref<InfixRuleOperatorList>),
                  lit<">">)> {};
  struct InfixRule
      : rule<create<ast::InfixRule>(
            kw<"infix"> + RuleNameAndParams + kw<"on"> +
            assign<&ast::InfixRule::call>(ref<RuleCall>) +
            ParserReturnType + lit<":"> +
            assign<&ast::InfixRule::operators>(ref<InfixRuleOperators>) +
            lit<";">)> {};
  struct GrammarImport
      : rule<kw<"import"> + assign<&ast::GrammarImport::path>(ref<STRING>) +
             option(lit<";">)> {};
  struct Grammar
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
            many(append<&ast::Grammar::imports>(ref<GrammarImport>)) +
            many(append<&ast::Grammar::rules>(ref<ParserRule> |
                                              ref<TerminalRule> |
                                              ref<InfixRule>) |
                 append<&ast::Grammar::interfaces>(ref<Interface>) |
                 append<&ast::Grammar::types>(ref<Type>)))> {};

  using entry = Grammar;
  using ignored = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace langium
