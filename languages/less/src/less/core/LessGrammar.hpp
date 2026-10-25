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
using Unquoted = cursiv::text_conv<unquote>;

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

  struct NUMBER
      : terminal<option(cr<"+-">) + option(many(d) + lit<".">) + some(d),
                 double> {};

  struct COLOR : terminal<lit<"#"> + some(cr<"0-9a-fA-F">)> {};

  // Quoted values retain Less escapes; the native reader normalizes line ends.
  static constexpr auto StringEscape = lit<"\\"> + any;
  struct STRING
      : terminal<(lit<"\""> + many(StringEscape | cr<R"(^"\\)">) + lit<"\"">) |
                     (lit<"'"> + many(StringEscape | cr<R"(^'\\)">) + lit<"'">),
                 Unquoted> {};

  // Only legal right after `url(`: a state that isn't inside url() never
  // offers it, so it never competes with ID elsewhere.
  struct URL_RAW : terminal<some(cr<R"(^)\r\n;)">)> {};

  // Less accepts arbitrary ASCII unit names, with their original case.
  struct UNIT : terminal<lit<"%"> | some(cr<"A-Za-z_">)> {};

  struct ELLIPSIS : terminal<lit<"...">> {};
  struct IMPORTANT : terminal<lit<"!important">> {};

  struct MUL : terminal<lit<"*">, value<ast::MathOp::times>> {};
  struct MADD : terminal<lit<"+">, value<ast::MathOp::plus>> {};
  struct MDIV : terminal<lit<"/">, value<ast::MathOp::div>> {};
  struct MSUB : terminal<lit<"-">, value<ast::MathOp::minus>> {};
  struct MMOD : terminal<lit<"%">, value<ast::MathOp::mod>> {};
  // `*`/`/`/`%` bind tighter than `+`/`-`.
  static constexpr auto MulOp = ref<MUL> | ref<MDIV> | ref<MMOD>;
  static constexpr auto AddOp = ref<MADD> | ref<MSUB>;

  struct CEQ : terminal<lit<"=">, value<ast::CompareOp::eq>> {};
  struct CGE : terminal<lit<">=">, value<ast::CompareOp::ge>> {};
  struct CLE : terminal<lit<"<=">, value<ast::CompareOp::le>> {};
  struct CLT : terminal<lit<"<">, value<ast::CompareOp::lt>> {};
  struct CGT : terminal<lit<">">, value<ast::CompareOp::gt>> {};
  static constexpr auto CompareOperator =
      ref<CGE> | ref<CLE> | ref<CEQ> | ref<CLT> | ref<CGT>;

  struct ATTR_INCLUDES
      : terminal<lit<"~=">, value<ast::AttribRelate::includes>> {};
  struct ATTR_DASHMATCH
      : terminal<lit<"|=">, value<ast::AttribRelate::dashmatch>> {};
  struct ATTR_EQ : terminal<lit<"=">, value<ast::AttribRelate::equals>> {};
  static constexpr auto AttribRelateOp =
      ref<ATTR_INCLUDES> | ref<ATTR_DASHMATCH> | ref<ATTR_EQ>;

  struct PFX_CHILD : terminal<lit<">">, value<ast::SelectorPrefix::child>> {};
  struct PFX_SIBLING : terminal<lit<"+">, value<ast::SelectorPrefix::sibling>> {
  };
  struct PFX_GENERAL_SIBLING
      : terminal<lit<"~">, value<ast::SelectorPrefix::general_sibling>> {};
  static constexpr auto SelectorPrefixOp =
      ref<PFX_CHILD> | ref<PFX_SIBLING> | ref<PFX_GENERAL_SIBLING>;

  struct OPT_REFERENCE
      : terminal<kw<"reference">, value<ast::ImportOption::reference>> {};
  struct OPT_INLINE
      : terminal<kw<"inline">, value<ast::ImportOption::inline_>> {};
  struct OPT_LESS : terminal<kw<"less">, value<ast::ImportOption::less>> {};
  struct OPT_CSS : terminal<kw<"css">, value<ast::ImportOption::css>> {};
  struct OPT_ONCE : terminal<kw<"once">, value<ast::ImportOption::once>> {};
  struct OPT_MULTIPLE
      : terminal<kw<"multiple">, value<ast::ImportOption::multiple>> {};
  static constexpr auto ImportOptionAlt = ref<OPT_REFERENCE> | ref<OPT_INLINE> |
                                          ref<OPT_LESS> | ref<OPT_CSS> |
                                          ref<OPT_ONCE> | ref<OPT_MULTIPLE>;

  static constexpr auto UrlBody = ref<STRING> | ref<URL_RAW>;

  struct InterpRule : rule<create<ast::Interp> + lit<"@{"> +
                           assign<&ast::Interp::name>(ref<ID>) + lit<"}">> {};

  // A fractional dimension must not compete with a dot-prefixed value name.
  // Selectors retain their more permissive ID spelling (for example .123).
  struct VALUE_ID
      : terminal<!(option(lit<"-">) + lit<"."> + d) + ref<ID>> {};
  template <class Token> struct NameParts
      : rule<create<ast::Name> +
             append<&ast::Name::parts>(ref<Token> | ref<InterpRule>) +
             many(gap::none +
                  append<&ast::Name::parts>(ref<Token> | ref<InterpRule>)) +
             !(gap::none + (ref<Token> | lit<"@{">))> {};
  struct NameRule : NameParts<ID> {};
  struct ValueNameRule : NameParts<VALUE_ID> {};

  struct VariableNameRule
      : rule<lit<"@"> +
             (assign<&ast::VariableName::inner>(ref<VariableNameRule>) |
              assign<&ast::VariableName::name>(ref<ID>))> {};

  struct CommandStatementRule;

  struct MeasurementRule
      : rule<assign<&ast::Measurement::value>(ref<NUMBER>) +
             option(gap::none + assign<&ast::Measurement::unit>(ref<UNIT>))> {};

  struct ColorLitRule : rule<assign<&ast::ColorLit::value>(ref<COLOR>)> {};
  struct StringLitRule
      : rule<option(flag<&ast::StringLit::escaped>(lit<"~">)) +
             assign<&ast::StringLit::value>(ref<STRING>)> {};

  struct UrlLitRule : rule<kw<"url"> + lit<"("> +
                           assign<&ast::UrlLit::value>(UrlBody) + lit<")">> {};

  struct FuncCallRule
      : rule<assign<&ast::FuncCall::name>(ref<ValueNameRule>) + lit<"("> +
             option(
                 some(append<&ast::FuncCall::args>(ref<CommandStatementRule>),
                      lit<",">)) +
             lit<")">> {};

  struct IdentValueRule
      : rule<assign<&ast::IdentValue::name>(ref<ValueNameRule>) +
             option(flag<&ast::IdentValue::important>(ref<IMPORTANT>))> {};

  struct VarValueRule
      : rule<assign<&ast::VarValue::name>(ref<VariableNameRule>) +
             option(flag<&ast::VarValue::important>(ref<IMPORTANT>))> {};

  struct ParenExprRule
      : rule<lit<"("> +
             assign<&ast::ParenExpr::value>(ref<CommandStatementRule>) +
             lit<")">> {};

  static constexpr auto ExpressionAlt =
      ref<MeasurementRule> | ref<FuncCallRule> | ref<IdentValueRule> |
      ref<ColorLitRule> | ref<StringLitRule> | ref<UrlLitRule> |
      ref<VarValueRule> | ref<ParenExprRule>;

  struct MulTermRule : rule<some(append<&ast::MulTerm::items>(ExpressionAlt))> {
  };
  struct MulTailRule : rule<assign<&ast::MulTerm::op>(MulOp) +
                            some(append<&ast::MulTerm::items>(ExpressionAlt))> {
  };

  struct MulExprRule
      : rule<create<ast::MulExpr> +
             append<&ast::MulExpr::terms>(ref<MulTermRule>) +
             many(append<&ast::MulExpr::terms>(ref<MulTailRule>))> {};

  struct AddTermRule : rule<assign<&ast::AddTerm::operand>(ref<MulExprRule>)> {
  };
  struct AddTailRule : rule<assign<&ast::AddTerm::op>(AddOp) +
                            assign<&ast::AddTerm::operand>(ref<MulExprRule>)> {
  };

  struct CommandStatementRule
      : rule<create<ast::CommandStatement> +
             append<&ast::CommandStatement::terms>(ref<AddTermRule>) +
             many(append<&ast::CommandStatement::terms>(ref<AddTailRule>))> {};

  struct ValueListRule
      : rule<create<ast::ValueList> +
             some(append<&ast::ValueList::items>(ref<CommandStatementRule>),
                  lit<",">)> {};

  struct ConditionStatementRule
      : rule<assign<&ast::ConditionStatement::left>(ref<CommandStatementRule>) +
             option(assign<&ast::ConditionStatement::op>(CompareOperator) +
                    assign<&ast::ConditionStatement::right>(
                        ref<CommandStatementRule>))> {};

  struct ConditionRule
      : rule<option(flag<&ast::Condition::negated>(kw<"not">)) + lit<"("> +
             assign<&ast::Condition::test>(ref<ConditionStatementRule>) +
             lit<")">> {};

  struct Selector;

  struct AttribRule
      : rule<lit<"["> + assign<&ast::Attrib::name>(ref<ID>) +
             option(assign<&ast::Attrib::relate>(AttribRelateOp) +
                    assign<&ast::Attrib::value>(ref<STRING> | ref<ID>)) +
             lit<"]">> {};

  struct PseudoRule : rule<(flag<&ast::Pseudo::isClass>(lit<"::">) |
                            lit<":">)+assign<&ast::Pseudo::name>(ref<ID>)> {};

  struct NegationRule
      : rule<create<ast::Negation> + lit<":"> + kw<"not"> + lit<"("> +
             option(lit<"[">) +
             some(append<&ast::Negation::alternatives>(ref<Selector>),
                  lit<",">) +
             option(lit<"]">) + lit<")">> {};

  struct ElementRule
      : rule<(assign<&ast::NamedElement::prefix>(SelectorPrefixOp) +
              assign<&ast::NamedElement::name>(ref<NameRule>)) |
             assign<&ast::NamedElement::name>(ref<NameRule>) |
             (lit<"#"> + assign<&ast::IdElement::name>(ref<NameRule>)) |
             ref<PseudoRule> | ref<NegationRule> |
             (create<ast::ParentRefElement> + lit<"&">) |
             (create<ast::WildcardElement> + lit<"*">)> {};

  struct Selector
      : rule<create<ast::Selector> +
             some(append<&ast::Selector::elements>(ref<ElementRule>)) +
             many(append<&ast::Selector::attribs>(ref<AttribRule>)) +
             option(assign<&ast::Selector::trailingPseudo>(ref<PseudoRule>))> {
  };

  struct SelectorsRule
      : rule<create<ast::Selectors> +
             some(append<&ast::Selectors::items>(ref<Selector>), lit<",">)> {};

  struct Statement;

  struct PropertyRule
      : rule<assign<&ast::Property::name>(ref<NameRule>) + lit<":"> +
             assign<&ast::Property::value>(ref<ValueListRule>)> {};

  struct MixinRefRule
      : rule<assign<&ast::MixinRef::target>(ref<Selector>) + lit<"("> +
             option(
                 some(append<&ast::MixinRef::args>(ref<CommandStatementRule>),
                      lit<",">)) +
             lit<")"> +
             option(flag<&ast::MixinRef::important>(ref<IMPORTANT>)) +
             lit<";">> {};

  struct BlockRule
      : rule<create<ast::Block> + lit<"{"> +
             many(append<&ast::Block::items>((ref<PropertyRule> + lit<";">) |
                                             ref<Statement> |
                                             ref<MixinRefRule>)) +
             option(append<&ast::Block::items>(ref<PropertyRule>)) + lit<"}">> {
  };

  struct VariableDeclRule
      : rule<assign<&ast::VariableDecl::name>(ref<VariableNameRule>) +
             lit<":"> + assign<&ast::VariableDecl::value>(ref<ValueListRule>)> {
  };

  static constexpr auto MixinGuard =
      kw<"when"> + some(append<&ast::MixinDef::guard>(ref<ConditionRule>),
                        kw<"and"> | lit<",">);

  struct RulesetRule
      : rule<assign<&ast::Ruleset::selectors>(ref<SelectorsRule>) +
             assign<&ast::Ruleset::body>(ref<BlockRule>)> {};

  // In comma mode a comma ends the parameter. In semicolon mode it can
  // belong to a default value, so the two default-value rules stay distinct.
  struct MixinCommaDefault
      : rule<create<ast::ValueList> +
             append<&ast::ValueList::items>(ref<CommandStatementRule>)> {};
  struct MixinCommaParam
      : rule<assign<&ast::VariableDecl::name>(ref<VariableNameRule>) +
             lit<":"> +
             assign<&ast::VariableDecl::value>(ref<MixinCommaDefault>)> {};

  struct MixinDefRule
      : rule<assign<&ast::MixinDef::selectors>(ref<SelectorsRule>) + lit<"("> +
             option(some(append<&ast::MixinDef::params>(ref<VariableDeclRule> |
                                                        ref<VariableNameRule>),
                         lit<";">) |
                    some(append<&ast::MixinDef::params>(ref<MixinCommaParam> |
                                                        ref<VariableNameRule>),
                         lit<",">)) +
             option(flag<&ast::MixinDef::variadic>(ref<ELLIPSIS>)) +
             lit<")"> + option(MixinGuard) +
             assign<&ast::MixinDef::body>(ref<BlockRule>)> {};

  static constexpr auto ImportUrl =
      assign<&ast::Import::url>(ref<STRING>) |
      (kw<"url"> + lit<"("> + assign<&ast::Import::url>(UrlBody) + lit<")">);

  struct ImportRule
      : rule<lit<"@import"> +
             option(lit<"("> +
                    some(append<&ast::Import::options>(ImportOptionAlt),
                         lit<",">) +
                    lit<")">) +
             ImportUrl +
             option(some(append<&ast::Import::mediaTypes>(ref<ID>), lit<",">)) +
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
  struct AtRuleRule
      : rule<lit<"@"> + assign<&ast::AtRule::name>(ref<ID>) +
             option(assign<&ast::AtRule::prelude>(ref<AtPrelude>)) +
             (assign<&ast::AtRule::body>(ref<BlockRule>) | lit<";">)> {};

  struct Statement : rule<ref<ImportRule> | ref<RulesetRule> |
                          (ref<VariableDeclRule> + lit<";">) |
                          ref<MixinDefRule> | ref<AtRuleRule>> {};

  struct StylesheetRule
      : rule<create<ast::Stylesheet> +
             many(append<&ast::Stylesheet::statements>(ref<Statement>))> {};

  using entry = StylesheetRule;
  using ignore = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

} // namespace less
