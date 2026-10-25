// The graphql language: its grammar. Covers both the executable query
// language and the type-system definition language in one file.
#pragma once

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <limits>
#include <memory_resource>
#include <span>
#include <vector>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "graphql/core/ast.hpp"

namespace graphql {

inline constexpr auto unquote1 =
    [](std::string_view t, std::pmr::memory_resource* mem)
        -> cursiv::ConversionResult<std::string_view> {
  using Result = cursiv::ConversionResult<std::string_view>;
  t = t.substr(1, t.size() - 2);
  if (t.find('\\') == std::string_view::npos) return Result::ok(t);
  const auto room = cursiv::text_room(mem, t.size());
  std::size_t n = 0;
  const auto hex = [](std::string_view digits) {
    std::uint32_t cp = 0;
    std::from_chars(digits.data(), digits.data() + digits.size(), cp, 16);
    return cp;
  };
  for (std::size_t i = 0; i < t.size(); ++i) {
    if (t[i] != '\\' || i + 1 == t.size()) {
      room[n++] = t[i];
      continue;
    }
    const char escaped = t[++i];
    if (escaped == 'u') {
      std::uint32_t cp = 0;
      if (i + 1 < t.size() && t[i + 1] == '{') {
        const auto end = t.find('}', i + 2);
        if (end == std::string_view::npos || end - i - 2 > 8)
          return Result::err("Invalid GraphQL Unicode escape");
        cp = hex(t.substr(i + 2, end - i - 2));
        i = end;
      } else {
        cp = hex(t.substr(i + 1, 4));
        i += 4;
        if (cp >= 0xd800 && cp <= 0xdbff && t.substr(i + 1, 2) == "\\u" &&
            i + 7 <= t.size()) {
          const auto low = hex(t.substr(i + 3, 4));
          if (low >= 0xdc00 && low <= 0xdfff) {
            cp = 0x10000 + ((cp - 0xd800) << 10) + low - 0xdc00;
            i += 6;
          }
        }
      }
      if (cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff))
        return Result::err("A GraphQL Unicode escape must encode a scalar value");
      if (cp < 0x80) room[n++] = static_cast<char>(cp);
      else {
        if (cp >= 0x10000) room[n++] = static_cast<char>(0xf0 | (cp >> 18));
        else if (cp >= 0x800) room[n++] = static_cast<char>(0xe0 | (cp >> 12));
        else room[n++] = static_cast<char>(0xc0 | (cp >> 6));
        if (cp >= 0x10000) room[n++] = static_cast<char>(0x80 | ((cp >> 12) & 63));
        if (cp >= 0x800) room[n++] = static_cast<char>(0x80 | ((cp >> 6) & 63));
        room[n++] = static_cast<char>(0x80 | (cp & 63));
      }
    } else {
      switch (escaped) {
        case 'b': room[n++] = '\b'; break;
        case 'f': room[n++] = '\f'; break;
        case 'n': room[n++] = '\n'; break;
        case 'r': room[n++] = '\r'; break;
        case 't': room[n++] = '\t'; break;
        default: room[n++] = escaped; break;
      }
    }
  }
  return Result::ok(std::string_view(room.data(), n));
};
using Unquoted = cursiv::text_conv<unquote1>;

inline constexpr auto unquote3 =
    [](std::string_view t, std::pmr::memory_resource* mem) -> std::string_view {
  t = t.substr(3, t.size() - 6);
  const auto room = cursiv::text_room(mem, t.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < t.size(); ++i) {
    if (t.substr(i, 4) == R"(\""")") {
      room[n++] = '"'; room[n++] = '"'; room[n++] = '"';
      i += 3;
    } else if (t[i] == '\r') {
      room[n++] = '\n';
      if (i + 1 < t.size() && t[i + 1] == '\n') ++i;
    } else room[n++] = t[i];
  }
  const std::string_view normalized(room.data(), n);
  std::pmr::vector<std::string_view> lines(mem);
  std::size_t indent = std::numeric_limits<std::size_t>::max();
  for (std::size_t start = 0; start <= normalized.size();) {
    auto end = normalized.find('\n', start);
    if (end == std::string_view::npos) end = normalized.size();
    const auto line = normalized.substr(start, end - start);
    const auto leading = line.find_first_not_of(" \t");
    if (!lines.empty() && leading != std::string_view::npos)
      indent = std::min(indent, leading);
    lines.push_back(line);
    if (end == normalized.size()) break;
    start = end + 1;
  }
  for (std::size_t i = 1; i < lines.size(); ++i)
    lines[i].remove_prefix(std::min(indent, lines[i].size()));
  std::size_t first = 0, last = lines.size();
  while (first < last && lines[first].find_first_not_of(" \t") == std::string_view::npos)
    ++first;
  while (last > first && lines[last - 1].find_first_not_of(" \t") == std::string_view::npos)
    --last;
  std::size_t out = 0;
  for (std::size_t i = first; i < last; ++i) {
    if (i > first) room[out++] = '\n';
    std::copy(lines[i].begin(), lines[i].end(), room.begin() + out);
    out += lines[i].size();
  }
  return std::string_view(room.data(), out);
};
using BlockUnquoted = cursiv::text_conv<unquote3>;

inline constexpr auto null_value = [](std::string_view) {
  return std::monostate{};
};
using NullValue = cursiv::text_conv<null_value>;

struct Graphql : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\n\r)">)> {};
  struct COMMA : terminal<lit<",">> {};
  struct LineComment : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};
  struct Bom : terminal<lit<"\xEF\xBB\xBF">> {};

  static constexpr auto NameBody = cr<"_A-Za-z"> + many(cr<"_0-9A-Za-z">);
  struct NAME : terminal<NameBody> {};

  static constexpr auto StringEscape =
      lit<"\\"> +
      (cr<R"("\\/bfnrt)"> | (lit<"u"> + (repeat<4>(cr<"0-9a-fA-F">) |
                      (lit<"{"> + repeat<1, 8>(cr<"0-9a-fA-F">) + lit<"}">))));
  struct STRING
      : terminal<lit<"\""> + many(StringEscape | cr<R"(^"\\\r\n)">) + lit<"\"">,
                 Unquoted> {};
  static constexpr auto BlockUnit =
      option(lit<"\"\""> | lit<"\"">) + (lit<R"(\""")"> | cr<R"(^")">);
  struct BLOCK_STRING
      : terminal<lit<"\"\"\""> + many(BlockUnit) + lit<"\"\"\"">,
                 BlockUnquoted> {};

  static constexpr auto IntBody =
      option(lit<"-">) + (lit<"0"> | (cr<"1-9"> + many(d)));
  static constexpr auto NumberEnd = !cr<".0-9_A-Za-z">;
  struct INT : terminal<IntBody + NumberEnd> {};
  struct FLOAT
      : terminal<IntBody + ((lit<"."> + some(d) +
                             option(cr<"eE"> + option(cr<"+-">) + some(d))) |
                            (cr<"eE"> + option(cr<"+-">) + some(d))) + NumberEnd> {};

  struct BOOL : terminal<kw<"true"> | kw<"false">, bool> {};
  struct NULLK : terminal<kw<"null">, NullValue> {};

  struct OP_QUERY : terminal<kw<"query">, value<ast::OperationKind::Query>> {};
  struct OP_MUTATION
      : terminal<kw<"mutation">, value<ast::OperationKind::Mutation>> {};
  struct OP_SUBSCRIPTION
      : terminal<kw<"subscription">, value<ast::OperationKind::Subscription>> {
  };
  static constexpr auto OperationTypeExpr =
      ref<OP_QUERY> | ref<OP_MUTATION> | ref<OP_SUBSCRIPTION>;

  struct DL_QUERY
      : terminal<kw<"QUERY">, value<ast::DirectiveLocationKind::Query>> {};
  struct DL_MUTATION
      : terminal<kw<"MUTATION">, value<ast::DirectiveLocationKind::Mutation>> {
  };
  struct DL_SUBSCRIPTION
      : terminal<kw<"SUBSCRIPTION">,
                 value<ast::DirectiveLocationKind::Subscription>> {};
  struct DL_FIELD
      : terminal<kw<"FIELD">, value<ast::DirectiveLocationKind::Field>> {};
  struct DL_FRAGMENT_DEFINITION
      : terminal<kw<"FRAGMENT_DEFINITION">,
                 value<ast::DirectiveLocationKind::FragmentDefinition>> {};
  struct DL_FRAGMENT_SPREAD
      : terminal<kw<"FRAGMENT_SPREAD">,
                 value<ast::DirectiveLocationKind::FragmentSpread>> {};
  struct DL_INLINE_FRAGMENT
      : terminal<kw<"INLINE_FRAGMENT">,
                 value<ast::DirectiveLocationKind::InlineFragment>> {};
  struct DL_VARIABLE_DEFINITION
      : terminal<kw<"VARIABLE_DEFINITION">,
                 value<ast::DirectiveLocationKind::VariableDefinition>> {};
  struct DL_SCHEMA
      : terminal<kw<"SCHEMA">, value<ast::DirectiveLocationKind::Schema>> {};
  struct DL_SCALAR
      : terminal<kw<"SCALAR">, value<ast::DirectiveLocationKind::Scalar>> {};
  struct DL_OBJECT
      : terminal<kw<"OBJECT">, value<ast::DirectiveLocationKind::Object>> {};
  struct DL_FIELD_DEFINITION
      : terminal<kw<"FIELD_DEFINITION">,
                 value<ast::DirectiveLocationKind::FieldDefinition>> {};
  struct DL_ARGUMENT_DEFINITION
      : terminal<kw<"ARGUMENT_DEFINITION">,
                 value<ast::DirectiveLocationKind::ArgumentDefinition>> {};
  struct DL_INTERFACE : terminal<kw<"INTERFACE">,
                                 value<ast::DirectiveLocationKind::Interface>> {
  };
  struct DL_UNION
      : terminal<kw<"UNION">, value<ast::DirectiveLocationKind::Union>> {};
  struct DL_ENUM
      : terminal<kw<"ENUM">, value<ast::DirectiveLocationKind::Enum>> {};
  struct DL_ENUM_VALUE
      : terminal<kw<"ENUM_VALUE">,
                 value<ast::DirectiveLocationKind::EnumValue>> {};
  struct DL_INPUT_OBJECT
      : terminal<kw<"INPUT_OBJECT">,
                 value<ast::DirectiveLocationKind::InputObject>> {};
  struct DL_INPUT_FIELD_DEFINITION
      : terminal<kw<"INPUT_FIELD_DEFINITION">,
                 value<ast::DirectiveLocationKind::InputFieldDefinition>> {};
  static constexpr auto DirectiveLocationExpr =
      ref<DL_QUERY> | ref<DL_MUTATION> | ref<DL_SUBSCRIPTION> | ref<DL_FIELD> |
      ref<DL_FRAGMENT_DEFINITION> | ref<DL_FRAGMENT_SPREAD> |
      ref<DL_INLINE_FRAGMENT> | ref<DL_VARIABLE_DEFINITION> | ref<DL_SCHEMA> |
      ref<DL_SCALAR> | ref<DL_OBJECT> | ref<DL_FIELD_DEFINITION> |
      ref<DL_ARGUMENT_DEFINITION> | ref<DL_INTERFACE> | ref<DL_UNION> |
      ref<DL_ENUM> | ref<DL_ENUM_VALUE> | ref<DL_INPUT_OBJECT> |
      ref<DL_INPUT_FIELD_DEFINITION>;

  static constexpr auto DescriptionExpr = ref<STRING> | ref<BLOCK_STRING>;

  struct VariableRule : rule<create<ast::Variable>(
                            lit<"$"> + assign<&ast::Variable::name>(ref<NAME>))> {};
  struct IntValueRule
      : rule<assign<&ast::IntLit::value>(ref<INT>)> {};
  struct FloatValueRule
      : rule<assign<&ast::FloatLit::value>(ref<FLOAT>)> {};
  struct EnumLitRule
      : rule<create<ast::EnumLit>(assign<&ast::EnumLit::name>(ref<NAME>))> {};

  struct ListValueRule;
  struct ObjectValueRule;
  static constexpr auto ValueExpr = ref<VariableRule> | ref<FloatValueRule> |
                                    ref<IntValueRule> |
                                    ref<STRING> | ref<BLOCK_STRING> |
                                    ref<BOOL> | ref<NULLK> | ref<EnumLitRule> |
                                    ref<ListValueRule> | ref<ObjectValueRule>;

  struct ListValueRule
      : rule<create<ast::ListValue>(
            lit<"["> + many(append<&ast::ListValue::items>(ValueExpr)) +
            lit<"]">)> {};

  struct ObjectFieldRule
      : rule<create<ast::ObjectField>(
            assign<&ast::ObjectField::name>(ref<NAME>) + lit<":"> +
            assign<&ast::ObjectField::value>(ValueExpr))> {};
  struct ObjectValueRule
      : rule<create<ast::ObjectValue>(
            lit<"{"> +
            many(append<&ast::ObjectValue::fields>(ref<ObjectFieldRule>)) +
            lit<"}">)> {};

  struct TypeRefRule;
  struct NamedTypeRefRule
      : rule<create<ast::NamedTypeRef>(
            assign<&ast::NamedTypeRef::name>(ref<NAME>) +
            option(flag<&ast::NamedTypeRef::nonNull>(lit<"!">)))> {};
  struct ListTypeRefRule
      : rule<create<ast::ListTypeRef>(
            lit<"["> + assign<&ast::ListTypeRef::itemType>(ref<TypeRefRule>) +
            lit<"]"> +
            option(flag<&ast::ListTypeRef::nonNull>(lit<"!">)))> {};
  struct TypeRefRule : rule<ref<NamedTypeRefRule> | ref<ListTypeRefRule>> {};

  struct ArgumentRule : rule<create<ast::Argument>(
                            assign<&ast::Argument::name>(ref<NAME>) + lit<":"> +
                            assign<&ast::Argument::value>(ValueExpr))> {};

  struct DirectiveRule
      : rule<create<ast::Directive>(
            lit<"@"> + assign<&ast::Directive::name>(ref<NAME>) +
            option(lit<"("> +
                   some(append<&ast::Directive::arguments>(ref<ArgumentRule>)) +
                   lit<")">))> {};

  struct InputValueDefinitionRule
      : rule<create<ast::InputValueDefinition>(
            option(assign<&ast::InputValueDefinition::description>(
                DescriptionExpr)) +
            assign<&ast::InputValueDefinition::name>(ref<NAME>) + lit<":"> +
            assign<&ast::InputValueDefinition::type>(ref<TypeRefRule>) +
            option(lit<"="> + assign<&ast::InputValueDefinition::defaultValue>(
                                  ValueExpr)) +
            many(append<&ast::InputValueDefinition::directives>(
                ref<DirectiveRule>)))> {};

  struct FieldDefinitionRule
      : rule<create<ast::FieldDefinition>(
            option(
                assign<&ast::FieldDefinition::description>(DescriptionExpr)) +
            assign<&ast::FieldDefinition::name>(ref<NAME>) +
            option(lit<"("> +
                   some(append<&ast::FieldDefinition::arguments>(
                       ref<InputValueDefinitionRule>)) +
                   lit<")">) +
            lit<":"> + assign<&ast::FieldDefinition::type>(ref<TypeRefRule>) +
            many(append<&ast::FieldDefinition::directives>(
                ref<DirectiveRule>)))> {};

  struct SelectionSetRule;
  struct FieldRule
      : rule<create<ast::Field>(
            option(assign<&ast::Field::alias>(ref<NAME>) + lit<":">) +
            assign<&ast::Field::name>(ref<NAME>) +
            option(lit<"("> +
                   some(append<&ast::Field::arguments>(ref<ArgumentRule>)) +
                   lit<")">) +
            many(append<&ast::Field::directives>(ref<DirectiveRule>)) +
            option(assign<&ast::Field::selectionSet>(ref<SelectionSetRule>)))> {
  };
  struct FragmentSpreadRule
      : rule<create<ast::FragmentSpread>(
            lit<"..."> + assign<&ast::FragmentSpread::name>(ref<NAME>) +
            many(append<&ast::FragmentSpread::directives>(
                ref<DirectiveRule>)))> {};
  struct InlineFragmentRule
      : rule<create<ast::InlineFragment>(
            lit<"..."> +
            option(kw<"on"> +
                   assign<&ast::InlineFragment::typeCondition>(ref<NAME>)) +
            many(append<&ast::InlineFragment::directives>(ref<DirectiveRule>)) +
            assign<&ast::InlineFragment::selectionSet>(
                ref<SelectionSetRule>))> {};
  struct SelectionRule : rule<ref<FieldRule> | ref<FragmentSpreadRule> |
                              ref<InlineFragmentRule>> {};
  struct SelectionSetRule
      : rule<create<ast::SelectionSet>(
            lit<"{"> +
            some(append<&ast::SelectionSet::selections>(ref<SelectionRule>)) +
            lit<"}">)> {};

  struct VariableDefinitionRule
      : rule<create<ast::VariableDefinition>(
            lit<"$"> + assign<&ast::VariableDefinition::name>(ref<NAME>) +
            lit<":"> +
            assign<&ast::VariableDefinition::type>(ref<TypeRefRule>) +
            option(lit<"="> +
                   assign<&ast::VariableDefinition::defaultValue>(ValueExpr)) +
            many(append<&ast::VariableDefinition::directives>(
                ref<DirectiveRule>)))> {};

  struct OperationDefinitionRule
      : rule<create<ast::OperationDefinition>(
            (assign<&ast::OperationDefinition::operation>(OperationTypeExpr) +
             option(assign<&ast::OperationDefinition::name>(ref<NAME>)) +
             option(lit<"("> +
                    some(append<&ast::OperationDefinition::variableDefinitions>(
                        ref<VariableDefinitionRule>)) +
                    lit<")">) +
             many(append<&ast::OperationDefinition::directives>(
                 ref<DirectiveRule>)) +
             assign<&ast::OperationDefinition::selectionSet>(
                 ref<SelectionSetRule>)) |
            assign<&ast::OperationDefinition::selectionSet>(
                ref<SelectionSetRule>))> {};

  struct FragmentDefinitionRule
      : rule<create<ast::FragmentDefinition>(
            kw<"fragment"> + assign<&ast::FragmentDefinition::name>(ref<NAME>) +
            kw<"on"> +
            assign<&ast::FragmentDefinition::typeCondition>(ref<NAME>) +
            many(append<&ast::FragmentDefinition::directives>(
                ref<DirectiveRule>)) +
            assign<&ast::FragmentDefinition::selectionSet>(
                ref<SelectionSetRule>))> {};

  struct RootOperationTypeDefinitionRule
      : rule<create<ast::RootOperationTypeDefinition>(
            assign<&ast::RootOperationTypeDefinition::operation>(
                OperationTypeExpr) +
            lit<":"> +
            assign<&ast::RootOperationTypeDefinition::type>(ref<NAME>))> {};

  struct SchemaDefinitionRule
      : rule<create<ast::SchemaDefinition>(
            option(assign<&ast::SchemaDefinition::description>(DescriptionExpr)) +
            kw<"schema"> +
            many(append<&ast::SchemaDefinition::directives>(
                ref<DirectiveRule>)) +
            lit<"{"> +
            some(append<&ast::SchemaDefinition::operationTypes>(
                ref<RootOperationTypeDefinitionRule>)) +
            lit<"}">)> {};

  // The extension bodies (directives, the braced block) are parsed
  // all-optional: `extend schema` alone over-accepts, harmlessly.
  struct SchemaExtensionRule
      : rule<create<ast::SchemaExtension>(
            kw<"extend"> + kw<"schema"> +
            many(
                append<&ast::SchemaExtension::directives>(ref<DirectiveRule>)) +
            option(lit<"{"> +
                   some(append<&ast::SchemaExtension::operationTypes>(
                       ref<RootOperationTypeDefinitionRule>)) +
                   lit<"}">))> {};

  struct ScalarTypeDefinitionRule
      : rule<create<ast::ScalarTypeDefinition>(
            option(assign<&ast::ScalarTypeDefinition::description>(
                DescriptionExpr)) +
            kw<"scalar"> + assign<&ast::ScalarTypeDefinition::name>(ref<NAME>) +
            many(append<&ast::ScalarTypeDefinition::directives>(
                ref<DirectiveRule>)))> {};

  struct ScalarTypeExtensionRule
      : rule<create<ast::ScalarTypeExtension>(
            kw<"extend"> + kw<"scalar"> +
            assign<&ast::ScalarTypeExtension::name>(ref<NAME>) +
            many(append<&ast::ScalarTypeExtension::directives>(
                ref<DirectiveRule>)))> {};

  struct ObjectTypeDefinitionRule
      : rule<create<ast::ObjectTypeDefinition>(
            option(assign<&ast::ObjectTypeDefinition::description>(
                DescriptionExpr)) +
            kw<"type"> + assign<&ast::ObjectTypeDefinition::name>(ref<NAME>) +
            option(
                kw<"implements"> + option(lit<"&">) +
                append<&ast::ObjectTypeDefinition::interfaces>(ref<NAME>) +
                many(lit<"&"> + append<&ast::ObjectTypeDefinition::interfaces>(
                                    ref<NAME>))) +
            many(append<&ast::ObjectTypeDefinition::directives>(
                ref<DirectiveRule>)) +
            option(lit<"{"> +
                   some(append<&ast::ObjectTypeDefinition::fields>(
                       ref<FieldDefinitionRule>)) +
                   lit<"}">))> {};

  struct ObjectTypeExtensionRule
      : rule<create<ast::ObjectTypeExtension>(
            kw<"extend"> + kw<"type"> +
            assign<&ast::ObjectTypeExtension::name>(ref<NAME>) +
            option(
                kw<"implements"> + option(lit<"&">) +
                append<&ast::ObjectTypeExtension::interfaces>(ref<NAME>) +
                many(lit<"&"> + append<&ast::ObjectTypeExtension::interfaces>(
                                    ref<NAME>))) +
            many(append<&ast::ObjectTypeExtension::directives>(
                ref<DirectiveRule>)) +
            option(lit<"{"> +
                   some(append<&ast::ObjectTypeExtension::fields>(
                       ref<FieldDefinitionRule>)) +
                   lit<"}">))> {};

  struct InterfaceTypeDefinitionRule
      : rule<create<ast::InterfaceTypeDefinition>(
            option(assign<&ast::InterfaceTypeDefinition::description>(
                DescriptionExpr)) +
            kw<"interface"> +
            assign<&ast::InterfaceTypeDefinition::name>(ref<NAME>) +
            option(
                kw<"implements"> + option(lit<"&">) +
                append<&ast::InterfaceTypeDefinition::interfaces>(ref<NAME>) +
                many(lit<"&"> +
                     append<&ast::InterfaceTypeDefinition::interfaces>(
                         ref<NAME>))) +
            many(append<&ast::InterfaceTypeDefinition::directives>(
                ref<DirectiveRule>)) +
            option(lit<"{"> +
                   some(append<&ast::InterfaceTypeDefinition::fields>(
                       ref<FieldDefinitionRule>)) +
                   lit<"}">))> {};

  struct InterfaceTypeExtensionRule
      : rule<create<ast::InterfaceTypeExtension>(
            kw<"extend"> + kw<"interface"> +
            assign<&ast::InterfaceTypeExtension::name>(ref<NAME>) +
            option(kw<"implements"> + option(lit<"&">) +
                   append<&ast::InterfaceTypeExtension::interfaces>(ref<NAME>) +
                   many(lit<"&"> +
                        append<&ast::InterfaceTypeExtension::interfaces>(
                            ref<NAME>))) +
            many(append<&ast::InterfaceTypeExtension::directives>(
                ref<DirectiveRule>)) +
            option(lit<"{"> +
                   some(append<&ast::InterfaceTypeExtension::fields>(
                       ref<FieldDefinitionRule>)) +
                   lit<"}">))> {};

  struct UnionTypeDefinitionRule
      : rule<create<ast::UnionTypeDefinition>(
            option(assign<&ast::UnionTypeDefinition::description>(
                DescriptionExpr)) +
            kw<"union"> + assign<&ast::UnionTypeDefinition::name>(ref<NAME>) +
            many(append<&ast::UnionTypeDefinition::directives>(
                ref<DirectiveRule>)) +
            option(lit<"="> + option(lit<"|">) +
                   append<&ast::UnionTypeDefinition::members>(ref<NAME>) +
                   many(lit<"|"> + append<&ast::UnionTypeDefinition::members>(
                                       ref<NAME>))))> {};

  struct UnionTypeExtensionRule
      : rule<create<ast::UnionTypeExtension>(
            kw<"extend"> + kw<"union"> +
            assign<&ast::UnionTypeExtension::name>(ref<NAME>) +
            many(append<&ast::UnionTypeExtension::directives>(
                ref<DirectiveRule>)) +
            option(lit<"="> + option(lit<"|">) +
                   append<&ast::UnionTypeExtension::members>(ref<NAME>) +
                   many(lit<"|"> + append<&ast::UnionTypeExtension::members>(
                                       ref<NAME>))))> {};

  struct EnumValueDefinitionRule
      : rule<create<ast::EnumValueDefinition>(
            option(assign<&ast::EnumValueDefinition::description>(
                DescriptionExpr)) +
            assign<&ast::EnumValueDefinition::name>(ref<NAME>) +
            many(append<&ast::EnumValueDefinition::directives>(
                ref<DirectiveRule>)))> {};

  struct EnumTypeDefinitionRule
      : rule<create<ast::EnumTypeDefinition>(
            option(assign<&ast::EnumTypeDefinition::description>(
                DescriptionExpr)) +
            kw<"enum"> + assign<&ast::EnumTypeDefinition::name>(ref<NAME>) +
            many(append<&ast::EnumTypeDefinition::directives>(
                ref<DirectiveRule>)) +
            option(lit<"{"> +
                   some(append<&ast::EnumTypeDefinition::values>(
                       ref<EnumValueDefinitionRule>)) +
                   lit<"}">))> {};

  struct EnumTypeExtensionRule
      : rule<create<ast::EnumTypeExtension>(
            kw<"extend"> + kw<"enum"> +
            assign<&ast::EnumTypeExtension::name>(ref<NAME>) +
            many(append<&ast::EnumTypeExtension::directives>(
                ref<DirectiveRule>)) +
            option(lit<"{"> +
                   some(append<&ast::EnumTypeExtension::values>(
                       ref<EnumValueDefinitionRule>)) +
                   lit<"}">))> {};

  struct InputObjectTypeDefinitionRule
      : rule<create<ast::InputObjectTypeDefinition>(
            option(assign<&ast::InputObjectTypeDefinition::description>(
                DescriptionExpr)) +
            kw<"input"> +
            assign<&ast::InputObjectTypeDefinition::name>(ref<NAME>) +
            many(append<&ast::InputObjectTypeDefinition::directives>(
                ref<DirectiveRule>)) +
            option(lit<"{"> +
                   some(append<&ast::InputObjectTypeDefinition::fields>(
                       ref<InputValueDefinitionRule>)) +
                   lit<"}">))> {};

  struct InputObjectTypeExtensionRule
      : rule<create<ast::InputObjectTypeExtension>(
            kw<"extend"> + kw<"input"> +
            assign<&ast::InputObjectTypeExtension::name>(ref<NAME>) +
            many(append<&ast::InputObjectTypeExtension::directives>(
                ref<DirectiveRule>)) +
            option(lit<"{"> +
                   some(append<&ast::InputObjectTypeExtension::fields>(
                       ref<InputValueDefinitionRule>)) +
                   lit<"}">))> {};

  struct DirectiveDefinitionRule
      : rule<create<ast::DirectiveDefinition>(
            option(assign<&ast::DirectiveDefinition::description>(
                DescriptionExpr)) +
            kw<"directive"> + lit<"@"> +
            assign<&ast::DirectiveDefinition::name>(ref<NAME>) +
            option(lit<"("> +
                   some(append<&ast::DirectiveDefinition::arguments>(
                       ref<InputValueDefinitionRule>)) +
                   lit<")">) +
            option(flag<&ast::DirectiveDefinition::repeatable>(
                kw<"repeatable">)) +
            kw<"on"> + option(lit<"|">) +
            some(append<&ast::DirectiveDefinition::locations>(
                     DirectiveLocationExpr), lit<"|">))> {};

  // A document is a nonempty sequence of definitions, grouped or
  // interleaved, flattened to one `definition+`.
  struct DefinitionRule
      : rule<ref<OperationDefinitionRule> | ref<FragmentDefinitionRule> |
             ref<SchemaDefinitionRule> | ref<ScalarTypeDefinitionRule> |
             ref<ObjectTypeDefinitionRule> | ref<InterfaceTypeDefinitionRule> |
             ref<UnionTypeDefinitionRule> | ref<EnumTypeDefinitionRule> |
             ref<InputObjectTypeDefinitionRule> | ref<DirectiveDefinitionRule> |
             ref<SchemaExtensionRule> | ref<ScalarTypeExtensionRule> |
             ref<ObjectTypeExtensionRule> | ref<InterfaceTypeExtensionRule> |
             ref<UnionTypeExtensionRule> | ref<EnumTypeExtensionRule> |
             ref<InputObjectTypeExtensionRule>> {};

  struct DocumentRule
      : rule<create<ast::Document>(
            some(append<&ast::Document::definitions>(ref<DefinitionRule>)))> {};

  using entry = DocumentRule;
  using ignore = list<WS, COMMA, LineComment, Bom>;
};

} // namespace graphql
