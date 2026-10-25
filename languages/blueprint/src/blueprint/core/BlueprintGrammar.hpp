// The blueprint language: its grammar. Android's Blueprint (Android.bp)
// build file format -- module declarations and top-level variable
// assignments, values including `select()` on product/arch conditions.
#pragma once

#include "blueprint/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace blueprint {

struct Blueprint : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  struct BOOLEAN : terminal<kw<"true"> | kw<"false">, bool> {};
  struct INT_TEXT : terminal<d + many(cr<"a-zA-Z0-9_">)> {};
  struct IntegerText : rule<option(lit<"-">) + ref<INT_TEXT>> {};
  struct IntegerRule : rule<assign<&ast::IntegerLit::raw>(ref<IntegerText>)> {};
  struct STRING
      : terminal<(lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\)">) +
                  lit<"\"">) |
                 (lit<"`"> + many(cr<"^`">) + lit<"`">)> {};
  struct StringRule : rule<assign<&ast::StringLit::raw>(ref<STRING>)> {};

  static constexpr auto NameStart = uc::L | cr<"_">;
  static constexpr auto NameContinue = uc::L | uc::Nd | cr<"_-">;
  static constexpr auto word_boundary = NameContinue;
  struct IDENT : terminal<NameStart + many(NameContinue)> {};

  struct ExpressionRule;
  struct SelectRule;
  struct ListValueRule;
  struct MapValueRule;

  struct VariableRule : rule<assign<&ast::Variable::name>(ref<IDENT>)> {};

  static constexpr auto value =
      ref<BOOLEAN> | ref<SelectRule> | ref<VariableRule> | ref<IntegerRule> |
      ref<StringRule> | ref<ListValueRule> | ref<MapValueRule>;

  struct ExpressionRule
      : rule<append<&ast::Expr::terms>(value) +
             many(lit<"+"> + append<&ast::Expr::terms>(value))> {};

  struct ListValueRule
      : rule<lit<"["> +
             many(append<&ast::ListValue::items>(ref<ExpressionRule>),
                  lit<",">) +
             option(flag<&ast::ListValue::trailingComma>(lit<",">)) +
             lit<"]">> {};

  struct PropertyRule
      : rule<assign<&ast::Property::name>(ref<IDENT>) +
             (flag<&ast::Property::equals>(lit<"=">) |
              lit<":">)+assign<&ast::Property::value>(ref<ExpressionRule>)> {};

  struct MapValueRule
      : rule<lit<"{"> +
             many(append<&ast::MapValue::properties>(ref<PropertyRule>),
                  lit<",">) +
             option(flag<&ast::MapValue::trailingComma>(lit<",">)) + lit<"}">> {
  };

  struct ConditionRule
      : rule<assign<&ast::Condition::function>(ref<IDENT>) + lit<"("> +
             many(append<&ast::Condition::args>(ref<StringRule>), lit<",">) +
             option(flag<&ast::Condition::trailingComma>(lit<",">)) +
             lit<")">> {};

  struct AnyPatternRule
      : rule<create<ast::AnyPattern> + kw<"any"> +
             option(lit<"@"> + assign<&ast::AnyPattern::binding>(ref<IDENT>))> {
  };

  struct DefaultPatternRule
      : rule<create<ast::DefaultPattern> + kw<"default">> {};

  static constexpr auto selectOnePattern =
      ref<AnyPatternRule> | ref<DefaultPatternRule> | ref<BOOLEAN> |
      ref<IntegerRule> | ref<StringRule>;

  struct SelectCaseRule
      : rule<(append<&ast::SelectCase::patterns>(selectOnePattern) |
              (flag<&ast::SelectCase::tuple>(lit<"(">) +
               some(append<&ast::SelectCase::patterns>(selectOnePattern),
                    lit<",">) +
               option(lit<",">) + lit<")">)) +
             lit<":"> +
             (kw<"unset"> |
              assign<&ast::SelectCase::value>(ref<ExpressionRule>)) +
             option(flag<&ast::SelectCase::comma>(lit<",">))> {};

  struct SelectRule
      : rule<kw<"select"> + lit<"("> +
             (append<&ast::Select::conditions>(ref<ConditionRule>) |
              (flag<&ast::Select::tuple>(lit<"(">) +
               some(append<&ast::Select::conditions>(ref<ConditionRule>),
                    lit<",">) +
               option(lit<",">) + lit<")">)) +
             lit<","> + lit<"{"> +
             many(append<&ast::Select::cases>(ref<SelectCaseRule>)) + lit<"}"> +
             lit<")">> {};

  struct AssignmentRule
      : rule<assign<&ast::Assignment::name>(ref<IDENT>) +
             (flag<&ast::Assignment::append>(lit<"+"> + lit<"=">) |
              lit<"=">)+assign<&ast::Assignment::value>(ref<ExpressionRule>)> {
  };

  struct ModuleRule
      : rule<assign<&ast::Module::type>(ref<IDENT>) +
             ((lit<"{"> +
               many(append<&ast::Module::properties>(ref<PropertyRule>),
                    lit<",">) +
               option(flag<&ast::Module::trailingComma>(lit<",">)) + lit<"}">) |
              (flag<&ast::Module::parenthesized>(lit<"(">) +
               many(append<&ast::Module::properties>(ref<PropertyRule>),
                    lit<",">) +
               option(flag<&ast::Module::trailingComma>(lit<",">)) +
               lit<")">))> {};

  struct DocumentRule
      : rule<option(flag<&ast::Document::bom>(lit<"\xEF\xBB\xBF">)) +
             many(append<&ast::Document::definitions>(ref<AssignmentRule> |
                                                      ref<ModuleRule>))> {};

  using entry = DocumentRule;
  using ignore = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

} // namespace blueprint
