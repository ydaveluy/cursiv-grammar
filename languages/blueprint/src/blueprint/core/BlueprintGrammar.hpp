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

  struct BOOLEAN : terminal<as<bool>(kw<"true"> | kw<"false">)> {};
  struct INT_TEXT : terminal<d + many(cr<"a-zA-Z0-9_">)> {};
  struct IntegerText : rule<option(lit<"-">) + ref<INT_TEXT>> {};
  struct Integer : rule<assign<&ast::IntegerLit::raw>(ref<IntegerText>)> {};
  struct STRING
      : terminal<(lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\)">) +
                  lit<"\"">) |
                 (lit<"`"> + many(cr<"^`">) + lit<"`">)> {};
  struct String : rule<assign<&ast::StringLit::raw>(ref<STRING>)> {};

  static constexpr auto NameStart = uc::L | cr<"_">;
  static constexpr auto NameContinue = uc::L | uc::Nd | cr<"_-">;
  static constexpr auto word_boundary = NameContinue;
  struct IDENT : terminal<NameStart + many(NameContinue)> {};

  struct Expression;
  struct Select;
  struct ListValue;
  struct MapValue;

  struct Variable : rule<assign<&ast::Variable::name>(ref<IDENT>)> {};

  static constexpr auto value =
      ref<BOOLEAN> | ref<Select> | ref<Variable> | ref<Integer> |
      ref<String> | ref<ListValue> | ref<MapValue>;

  struct Expression
      : rule<append<&ast::Expr::terms>(value) +
             many(lit<"+"> + append<&ast::Expr::terms>(value))> {};

  struct ListValue
      : rule<lit<"["> +
             many(append<&ast::ListValue::items>(ref<Expression>),
                  lit<",">) +
             option(flag<&ast::ListValue::trailingComma>(lit<",">)) +
             lit<"]">> {};

  struct Property
      : rule<assign<&ast::Property::name>(ref<IDENT>) +
             (flag<&ast::Property::equals>(lit<"=">) |
              lit<":">)+assign<&ast::Property::value>(ref<Expression>)> {};

  struct MapValue
      : rule<lit<"{"> +
             many(append<&ast::MapValue::properties>(ref<Property>),
                  lit<",">) +
             option(flag<&ast::MapValue::trailingComma>(lit<",">)) + lit<"}">> {
  };

  struct Condition
      : rule<assign<&ast::Condition::function>(ref<IDENT>) + lit<"("> +
             many(append<&ast::Condition::args>(ref<String>), lit<",">) +
             option(flag<&ast::Condition::trailingComma>(lit<",">)) +
             lit<")">> {};

  struct AnyPattern
      : rule<create<ast::AnyPattern> + kw<"any"> +
             option(lit<"@"> + assign<&ast::AnyPattern::binding>(ref<IDENT>))> {
  };

  struct DefaultPattern
      : rule<create<ast::DefaultPattern> + kw<"default">> {};

  static constexpr auto selectOnePattern =
      ref<AnyPattern> | ref<DefaultPattern> | ref<BOOLEAN> |
      ref<Integer> | ref<String>;

  struct SelectCase
      : rule<(append<&ast::SelectCase::patterns>(selectOnePattern) |
              (flag<&ast::SelectCase::tuple>(lit<"(">) +
               some(append<&ast::SelectCase::patterns>(selectOnePattern),
                    lit<",">) +
               option(lit<",">) + lit<")">)) +
             lit<":"> +
             (kw<"unset"> |
              assign<&ast::SelectCase::value>(ref<Expression>)) +
             option(flag<&ast::SelectCase::comma>(lit<",">))> {};

  struct Select
      : rule<kw<"select"> + lit<"("> +
             (append<&ast::Select::conditions>(ref<Condition>) |
              (flag<&ast::Select::tuple>(lit<"(">) +
               some(append<&ast::Select::conditions>(ref<Condition>),
                    lit<",">) +
               option(lit<",">) + lit<")">)) +
             lit<","> + lit<"{"> +
             many(append<&ast::Select::cases>(ref<SelectCase>)) + lit<"}"> +
             lit<")">> {};

  struct Assignment
      : rule<assign<&ast::Assignment::name>(ref<IDENT>) +
             (flag<&ast::Assignment::append>(lit<"+"> + lit<"=">) |
              lit<"=">)+assign<&ast::Assignment::value>(ref<Expression>)> {
  };

  struct Module
      : rule<assign<&ast::Module::type>(ref<IDENT>) +
             ((lit<"{"> +
               many(append<&ast::Module::properties>(ref<Property>),
                    lit<",">) +
               option(flag<&ast::Module::trailingComma>(lit<",">)) + lit<"}">) |
              (flag<&ast::Module::parenthesized>(lit<"(">) +
               many(append<&ast::Module::properties>(ref<Property>),
                    lit<",">) +
               option(flag<&ast::Module::trailingComma>(lit<",">)) +
               lit<")">))> {};

  struct Document
      : rule<option(flag<&ast::Document::bom>(lit<"\xEF\xBB\xBF">)) +
             many(append<&ast::Document::definitions>(ref<Assignment> |
                                                      ref<Module>))> {};

  using entry = Document;
  using ignored = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

} // namespace blueprint
