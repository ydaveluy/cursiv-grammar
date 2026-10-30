// The less language: its grammar. LESS layers variables (`@x`), nesting,
// mixins and interpolation (`@{x}`) over CSS syntax.
#pragma once

#include <memory_resource>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "less/core/ast.hpp"

namespace less {

inline constexpr auto unquote = [](std::string_view t,
                                  std::pmr::memory_resource* memory) {
  t = t.substr(1, t.size() - 2);
  if (t.find('\r') == std::string_view::npos) return t;
  const auto room = cursiv::text_room(memory, t.size());
  std::size_t size = 0;
  for (std::size_t i = 0; i < t.size(); ++i) {
    room[size++] = t[i] == '\r' ? '\n' : t[i];
    if (t[i] == '\r' && i + 1 < t.size() && t[i + 1] == '\n') ++i;
  }
  return std::string_view(room.data(), size);
};

struct Less : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  // '.' is a legal identifier byte: a bare class selector (`.button`) lexes
  // as one Identifier, not a DOT followed by a name.
  static constexpr auto identHead = cr<"_.a-zA-Z"> | cr<"Ā-￾">;
  static constexpr auto identCont = cr<"_.a-zA-Z0-9-"> | cr<"Ā-￾">;
  struct ID : terminal<(identHead + many(identCont)) |
                       (lit<"-"> + identHead + many(identCont))> {};

  struct NUMBER : terminal<as<double>(option(cr<"+-">) +
                                      option(many(d) + lit<".">) + some(d))> {};

  struct COLOR : terminal<lit<"#"> + some(cr<"0-9a-fA-F">)> {};

  // Quoted values retain Less escapes; the native reader normalizes line ends.
  static constexpr auto StringEscape = lit<"\\"> + any;
  struct STRING
      : terminal<as<unquote>(
            (lit<"\""> + many(StringEscape | cr<R"(^"\\)">) + lit<"\"">) |
            (lit<"'"> + many(StringEscape | cr<R"(^'\\)">) + lit<"'">))> {};

  // Only legal right after `url(`: a state that isn't inside url() never
  // offers it, so it never competes with ID elsewhere.
  struct URL_RAW : terminal<some(cr<R"(^)\r\n;)">)> {};

  // Less accepts arbitrary ASCII unit names, with their original case.
  struct UNIT : terminal<lit<"%"> | some(cr<"A-Za-z_">)> {};

  struct ELLIPSIS : terminal<lit<"...">> {};
  struct IMPORTANT : terminal<lit<"!important">> {};

  // `*`/`/`/`%` bind tighter than `+`/`-`.
  static constexpr auto MulOp = as<ast::MathOp::times>(lit<"*">) | as<ast::MathOp::div>(lit<"/">) | as<ast::MathOp::mod>(lit<"%">);
  static constexpr auto AddOp = as<ast::MathOp::plus>(lit<"+">) | as<ast::MathOp::minus>(lit<"-">);

  static constexpr auto CompareOperator =
      as<ast::CompareOp::ge>(lit<">="> | lit<"=>">) |
      as<ast::CompareOp::le>(lit<"<="> | lit<"=<">) |
      as<ast::CompareOp::eq>(lit<"=">) | as<ast::CompareOp::lt>(lit<"<">) | as<ast::CompareOp::gt>(lit<">">);

  static constexpr auto AttribRelateOp =
      as<ast::AttribRelate::includes>(lit<"~=">) | as<ast::AttribRelate::dashmatch>(lit<"|=">) | as<ast::AttribRelate::prefix>(lit<"^=">) | as<ast::AttribRelate::suffix>(lit<"$=">) | as<ast::AttribRelate::substring>(lit<"*=">) | as<ast::AttribRelate::equals>(lit<"=">);

  static constexpr auto SelectorPrefixOp =
      as<ast::SelectorPrefix::child>(lit<">">) | as<ast::SelectorPrefix::sibling>(lit<"+">) | as<ast::SelectorPrefix::general_sibling>(lit<"~">);

  static constexpr auto ImportOptionAlt = as<ast::ImportOption::reference>(kw<"reference">) | as<ast::ImportOption::inline_>(kw<"inline">) |
                                          as<ast::ImportOption::less>(kw<"less">) | as<ast::ImportOption::css>(kw<"css">) |
                                          as<ast::ImportOption::once>(kw<"once">) | as<ast::ImportOption::multiple>(kw<"multiple">) | as<ast::ImportOption::optional_>(kw<"optional">);

  static constexpr auto UrlBody = ref<STRING> | ref<URL_RAW>;

  struct VARIABLE_ID : terminal<some(cr<"a-zA-Z0-9_-">)> {};
  struct Interp : rule<create<ast::Interp> + lit<"@{"> + gap::none +
                           assign<&ast::Interp::name>(ref<VARIABLE_ID>) + gap::none + lit<"}">> {};

  // A fractional dimension must not compete with a dot-prefixed value name.
  // Selectors retain their more permissive ID spelling (for example .123).
  struct VALUE_ID
      : terminal<!(option(lit<"-">) + lit<"."> + d) + ref<ID>> {};
  template <class Token> struct NameParts
      : rule<create<ast::Name> +
             append<&ast::Name::parts>(ref<Token> | ref<Interp>) +
             many(gap::none +
                  append<&ast::Name::parts>(ref<Token> | ref<Interp>)) +
             !(gap::none + (ref<Token> | lit<"@{">))> {};
  struct Name : NameParts<ID> {};
  struct ValueName : NameParts<VALUE_ID> {};

  struct DirectVariableName : rule<lit<"@"> + gap::none +
      assign<&ast::VariableName::name>(ref<VARIABLE_ID>)> {};
  struct VariableName : rule<lit<"@"> + gap::none +
      (assign<&ast::VariableName::inner>(ref<DirectVariableName>) |
       assign<&ast::VariableName::name>(ref<VARIABLE_ID>))> {};

  struct CommandStatement;
  struct Block;
  struct ArgumentValue;
  struct ArgumentValueList;

  struct Measurement
      : rule<assign<&ast::Measurement::value>(ref<NUMBER>) +
             option(gap::none + assign<&ast::Measurement::unit>(ref<UNIT>))> {};

  struct ColorLit : rule<assign<&ast::ColorLit::value>(ref<COLOR>)> {};
  struct StringLit
      : rule<option(flag<&ast::StringLit::escaped>(lit<"~">)) +
             assign<&ast::StringLit::value>(ref<STRING>)> {};

  struct UrlLit : rule<kw<"url"> + lit<"("> +
                           assign<&ast::UrlLit::value>(UrlBody) + lit<")">> {};

  struct FuncCall
      : rule<assign<&ast::FuncCall::name>(ref<ValueName>) + lit<"("> +
             option(
                 (append<&ast::FuncCall::argumentGroups>(ref<ArgumentValueList>) +
                  some(lit<";"> + option(append<&ast::FuncCall::argumentGroups>(
                      ref<ArgumentValueList>)))) |
                 some(append<&ast::FuncCall::args>(ref<ArgumentValue>), lit<",">)) +
             lit<")">> {};

  struct IdentValue
      : rule<assign<&ast::IdentValue::name>(ref<ValueName>) +
             option(flag<&ast::IdentValue::important>(ref<IMPORTANT>))> {};

  struct VarValue
      : rule<assign<&ast::VarValue::name>(ref<VariableName>) +
             option(flag<&ast::VarValue::important>(ref<IMPORTANT>))> {};

  struct ParenExpr
      : rule<lit<"("> +
             assign<&ast::ParenExpr::value>(ref<CommandStatement>) +
             lit<")">> {};

  static constexpr auto ExpressionAlt =
      ref<Measurement> | ref<FuncCall> | ref<IdentValue> |
      ref<ColorLit> | ref<StringLit> | ref<UrlLit> |
      ref<VarValue> | ref<ParenExpr>;

  struct MulTerm : rule<some(append<&ast::MulTerm::items>(ExpressionAlt))> {
  };
  struct MulTail : rule<assign<&ast::MulTerm::op>(MulOp) +
                            some(append<&ast::MulTerm::items>(ExpressionAlt))> {
  };

  struct MulExpr
      : rule<create<ast::MulExpr> +
             append<&ast::MulExpr::terms>(ref<MulTerm>) +
             many(append<&ast::MulExpr::terms>(ref<MulTail>))> {};

  struct AddTerm : rule<assign<&ast::AddTerm::operand>(ref<MulExpr>)> {
  };
  struct AddTail : rule<assign<&ast::AddTerm::op>(AddOp) +
                            assign<&ast::AddTerm::operand>(ref<MulExpr>)> {
  };

  struct CommandStatement
      : rule<create<ast::CommandStatement> +
             append<&ast::CommandStatement::terms>(ref<AddTerm>) +
             many(append<&ast::CommandStatement::terms>(ref<AddTail>))> {};

  struct ValueList
      : rule<create<ast::ValueList> +
             some(append<&ast::ValueList::items>(ref<CommandStatement>),
                  lit<",">)> {};

  struct DetachedRuleset
      : rule<assign<&ast::DetachedRuleset::body>(ref<Block>)> {};
  // Preserve the existing CommandStatement argument API, including every
  // ownership edge, while keeping detached values out of ExpressionAlt.
  template <class Atom> struct AtomicArgument
      : rule<create<ast::CommandStatement>(
            append<&ast::CommandStatement::terms>(create<ast::AddTerm>(
                assign<&ast::AddTerm::operand>(create<ast::MulExpr>(
                    append<&ast::MulExpr::terms>(create<ast::MulTerm>(
                        append<&ast::MulTerm::items>(ref<Atom>))))))))> {};
  struct ArgumentValue : rule<ref<AtomicArgument<DetachedRuleset>> |
                              ref<CommandStatement>> {};
  struct ArgumentValueList : rule<create<ast::ValueList> +
      some(append<&ast::ValueList::items>(ref<ArgumentValue>), lit<",">)> {};
  struct SingleArgumentValueList : rule<create<ast::ValueList> +
      append<&ast::ValueList::items>(ref<ArgumentValue>)> {};
  struct NamedArgument : rule<assign<&ast::NamedArgument::name>(ref<VariableName>) +
      lit<":"> + assign<&ast::NamedArgument::value>(ref<SingleArgumentValueList>)> {};
  struct NamedArgumentList : rule<assign<&ast::NamedArgument::name>(ref<VariableName>) +
      lit<":"> + assign<&ast::NamedArgument::value>(ref<ArgumentValueList>)> {};
  struct MixinArgument : rule<ref<AtomicArgument<NamedArgument>> |
                             ref<ArgumentValue>> {};
  struct NamedArgumentGroup : rule<create<ast::ValueList> +
      append<&ast::ValueList::items>(ref<AtomicArgument<NamedArgumentList>>)> {};
  struct MixinArgumentGroup : rule<ref<NamedArgumentGroup> |
                                  ref<ArgumentValueList>> {};

  struct ConditionStatement
      : rule<assign<&ast::ConditionStatement::left>(ref<CommandStatement>) +
             option(assign<&ast::ConditionStatement::op>(CompareOperator) +
                    assign<&ast::ConditionStatement::right>(
                        ref<CommandStatement>))> {};

  struct Condition
      : rule<option(flag<&ast::Condition::negated>(kw<"not">)) + lit<"("> +
             assign<&ast::Condition::test>(ref<ConditionStatement>) +
             lit<")">> {};

  struct Selector;
  struct SelectorsRule;

  struct Attrib
      : rule<lit<"["> + assign<&ast::Attrib::name>(ref<ID>) +
             option(assign<&ast::Attrib::relate>(AttribRelateOp) +
                    assign<&ast::Attrib::value>(ref<STRING> | ref<ID>) +
                    option(assign<&ast::Attrib::modifier>(ikw<"i"> | ikw<"s">))) + lit<"]">> {};

  static constexpr auto RawQuote =
      (lit<"\""> + many(StringEscape | cr<R"(^"\\)">) + lit<"\"">) |
      (lit<"'"> + many(StringEscape | cr<R"(^'\\)">) + lit<"'">);
  struct MediaGroup : terminal<lit<"("> +
      many(ref<MediaGroup> | RawQuote | StringEscape | cr<R"(^()'"\\)">) + lit<")">> {};
  struct PseudoName : terminal<!(lit<"not"> + many(ref<WS>) + lit<"(">) + ref<ID>> {};
  struct Pseudo : rule<(flag<&ast::Pseudo::isClass>(lit<"::">) | lit<":">) +
      assign<&ast::Pseudo::name>(ref<PseudoName>) +
      option(flag<&ast::Pseudo::hasArguments>(lit<"(">) +
             option(assign<&ast::Pseudo::argumentSelectors>(ref<SelectorsRule>) |
                    assign<&ast::Pseudo::argumentValues>(ref<ValueList>)) + lit<")">)> {};
  struct ExtendElement;

  struct Negation
      : rule<create<ast::Negation> + lit<":"> + kw<"not"> + lit<"("> +
             some(append<&ast::Negation::alternatives>(ref<Selector>),
                  lit<",">) + lit<")">> {};

  struct Element
      : rule<(assign<&ast::NamedElement::prefix>(SelectorPrefixOp) +
              assign<&ast::NamedElement::name>(ref<Name>)) |
             assign<&ast::NamedElement::name>(ref<Name>) |
             (lit<"#"> + assign<&ast::IdElement::name>(ref<Name>)) |
             ref<ExtendElement> | ref<Pseudo> | ref<Negation> |
             (create<ast::ParentRefElement> + lit<"&">) |
             (create<ast::WildcardElement> + lit<"*">)> {};

  static constexpr auto SelectorPart =
      append<&ast::Selector::elements>(ref<Element>) |
      append<&ast::Selector::attribs>(ref<Attrib>);
  static constexpr auto SelectorBody = create<ast::Selector> + some(SelectorPart);
  struct Selector : rule<SelectorBody> {};
  // Native CSS guards belong to a single selector. Keep this separate from
  // the ordinary selector list, so neither a first nor a last guarded item
  // can accidentally admit a comma-separated list.
  struct GuardedSelector : rule<SelectorBody + kw<"when"> +
      assign<&ast::Selector::guard>(ref<Condition>)> {};
  struct GuardedSelectors : rule<create<ast::Selectors> +
      append<&ast::Selectors::items>(ref<GuardedSelector>)> {};
  static constexpr auto ExtendOptionEnd =
      option(lit<"!">) + kw<"all"> + (lit<")"> | lit<",">);
  struct ExtendSelector : rule<create<ast::Selector> +
      some(!ExtendOptionEnd + SelectorPart)> {};
  struct ExtendTarget : rule<assign<&ast::ExtendTarget::selector>(ref<ExtendSelector>) +
      option(assign<&ast::ExtendTarget::option>(kw<"all"> | lit<"!all">))> {};
  struct ExtendElement : rule<lit<":extend("> +
      some(append<&ast::ExtendElement::targets>(ref<ExtendTarget>), lit<",">) + lit<")">> {};

  struct SelectorsRule
      : rule<create<ast::Selectors> +
             some(append<&ast::Selectors::items>(ref<Selector>), lit<",">)> {};

  struct Statement;
  struct BodyStatement;

  struct Property
      : rule<assign<&ast::Property::name>(ref<Name>) + lit<":"> +
             assign<&ast::Property::value>(ref<ValueList>)> {};

  // The general Selector fallback can also read `property:pseudo(...)`.
  // Only a mixin name path followed by '(' admits named args, not a CSS
  // pseudo-selector on a property or class. The existing Selector builds it.
  struct MixinBareHead : terminal<some(cr<"A-Za-z0-9_-">)> {};
  struct MixinClassHead : terminal<lit<"."> + some(cr<"A-Za-z0-9_-">)> {};
  static constexpr auto MixinHeadElement = ref<MixinClassHead> |
      (lit<"#"> + gap::none + ref<MixinBareHead>);
  static constexpr auto MixinCallHead = MixinHeadElement +
      many(option(lit<">">) + MixinHeadElement) + lit<"(">;
  template <class Argument, class Group>
  static constexpr auto MixinCallBody =
      assign<&ast::MixinRef::target>(ref<Selector>) + lit<"("> +
             option(
                 (append<&ast::MixinRef::argumentGroups>(ref<Group>) +
                  some(lit<";"> + option(append<&ast::MixinRef::argumentGroups>(ref<Group>)))) |
                 some(append<&ast::MixinRef::args>(ref<Argument>), lit<",">)) +
             lit<")"> +
             option(flag<&ast::MixinRef::important>(ref<IMPORTANT>)) +
             (lit<";"> | &lit<"}">);
  struct MixinRef
      : rule<(&MixinCallHead + MixinCallBody<MixinArgument, MixinArgumentGroup>) |
             (!MixinCallHead + MixinCallBody<ArgumentValue, ArgumentValueList>)> {};

  struct Block
      : rule<create<ast::Block> + lit<"{"> +
             many(append<&ast::Block::items>((ref<Property> + lit<";">) |
                                             ref<BodyStatement> | ref<MixinRef>)) +
             option(append<&ast::Block::items>(ref<Property>)) + lit<"}">> {
  };

  struct VariableDecl
      : rule<assign<&ast::VariableDecl::name>(ref<DirectVariableName>) +
             lit<":"> + assign<&ast::VariableDecl::value>(ref<ValueList>)> {
  };

  static constexpr auto MixinGuard =
      kw<"when"> + some(append<&ast::MixinDef::guard>(ref<Condition>),
                        kw<"and"> | lit<",">);

  struct Ruleset
      : rule<assign<&ast::Ruleset::selectors>(ref<GuardedSelectors> | ref<SelectorsRule>) +
             assign<&ast::Ruleset::body>(ref<Block>)> {};

  // In comma mode a comma ends the parameter. In semicolon mode it can
  // belong to a default value, so the two default-value rules stay distinct.
  struct MixinCommaDefault
      : rule<create<ast::ValueList> +
             append<&ast::ValueList::items>(ref<ArgumentValue>)> {};
  struct MixinSemiParam : rule<assign<&ast::VariableDecl::name>(ref<VariableName>) +
      lit<":"> + assign<&ast::VariableDecl::value>(ref<ArgumentValueList>)> {};
  struct MixinCommaParam
      : rule<assign<&ast::VariableDecl::name>(ref<VariableName>) +
             lit<":"> +
             assign<&ast::VariableDecl::value>(ref<MixinCommaDefault>)> {};

  struct MixinLiteralParam : rule<!lit<"@"> + ref<CommandStatement>> {};
  struct MixinDef
      : rule<assign<&ast::MixinDef::selectors>(ref<SelectorsRule>) + lit<"("> +
             option(some(append<&ast::MixinDef::params>(ref<MixinSemiParam> |
                                                        ref<VariableName> | ref<MixinLiteralParam>),
                         lit<";">) |
                    some(append<&ast::MixinDef::params>(ref<MixinCommaParam> |
                                                        ref<VariableName> | ref<MixinLiteralParam>),
                         lit<",">)) +
             option(flag<&ast::MixinDef::variadic>(ref<ELLIPSIS>)) +
             lit<")"> + option(MixinGuard) +
             assign<&ast::MixinDef::body>(ref<Block>)> {};

  struct DetachedRulesetDecl : rule<
      assign<&ast::DetachedRulesetDecl::name>(ref<DirectVariableName>) + lit<":"> +
      assign<&ast::DetachedRulesetDecl::body>(ref<Block>) + option(lit<";">)> {};
  struct DetachedCall : rule<assign<&ast::DetachedCall::target>(ref<DirectVariableName>) +
      gap::none + lit<"()"> + (lit<";"> | &lit<"}"> | eof)> {};
  struct MixinCallStatement : rule<assign<&ast::MixinCallStatement::call>(ref<MixinRef>)> {};
  struct ExtendStatement : rule<lit<"&"> +
      assign<&ast::ExtendStatement::extension>(ref<ExtendElement>) + lit<";">> {};
  struct MediaQuery : terminal<!s + some(ref<MediaGroup> | RawQuote | StringEscape |
                                    cr<R"(^;(){}'"\\)">)> {};

  static constexpr auto ImportUrl =
      assign<&ast::Import::url>(ref<STRING>) |
      (kw<"url"> + lit<"("> + assign<&ast::Import::url>(UrlBody) + lit<")">);

  struct Import
      : rule<lit<"@import"> +
             option(lit<"("> +
                    some(append<&ast::Import::options>(ImportOptionAlt),
                         lit<",">) +
                    lit<")">) +
             ImportUrl +
             option(assign<&ast::Import::mediaQuery>(ref<MediaQuery>)) +
             lit<";">> {};

  static constexpr auto PreludeEscape = lit<"\\"> + any;
  static constexpr auto PreludeQuote =
      (lit<"\""> + many(PreludeEscape | cr<R"(^"\\)">) + lit<"\"">) |
      (lit<"'"> + many(PreludeEscape | cr<R"(^'\\)">) + lit<"'">);
  static constexpr auto PreludeUnit = PreludeQuote | PreludeEscape |
                                      through(lit<"/*">, lit<"*/">) |
                                      cr<R"(^{};'"\\)">;
  // A colon immediately after @name starts a variable declaration. Keep it
  // out of the first prelude unit, even when whitespace precedes it.
  struct AtPrelude : terminal<(PreludeQuote | PreludeEscape |
                               cr<R"(^{};:'"\\ \t\r\n)">)+many(PreludeUnit)> {};
  struct AtRule
      : rule<!(lit<"@"> + gap::none + ref<VARIABLE_ID> + gap::none + lit<"()"> + (lit<";"> | &lit<"}"> | eof)) +
             lit<"@"> + gap::none + assign<&ast::AtRule::name>(ref<ID>) +
             option(assign<&ast::AtRule::prelude>(ref<AtPrelude>)) +
             (assign<&ast::AtRule::body>(ref<Block>) | lit<";">)> {};

  static constexpr auto StatementCore =
      ref<DetachedRulesetDecl> | ref<DetachedCall> | ref<ExtendStatement> |
      ref<Import> | ref<Ruleset> | (ref<VariableDecl> + lit<";">) |
      ref<MixinDef> | ref<AtRule>;
  struct BodyStatement : rule<StatementCore> {};
  struct Statement : rule<ref<MixinCallStatement> | StatementCore> {};

  struct Stylesheet
      : rule<create<ast::Stylesheet> +
             many(append<&ast::Stylesheet::statements>(ref<Statement>))> {};

  using entry = Stylesheet;
  using ignored = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

} // namespace less
