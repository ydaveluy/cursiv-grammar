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

inline constexpr auto null_value = [](std::string_view) {
  return std::monostate{};
};

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
      : terminal<as<unquote1>(
            lit<"\""> + many(StringEscape | cr<R"(^"\\\r\n)">) + lit<"\"">)> {};
  static constexpr auto BlockUnit =
      option(lit<"\"\""> | lit<"\"">) + (lit<R"(\""")"> | cr<R"(^")">);
  struct BLOCK_STRING : terminal<as<unquote3>(lit<"\"\"\""> + many(BlockUnit) +
                                              lit<"\"\"\"">)> {};

  static constexpr auto IntBody =
      option(lit<"-">) + (lit<"0"> | (cr<"1-9"> + many(d)));
  static constexpr auto NumberEnd = !cr<".0-9_A-Za-z">;
  struct INT : terminal<IntBody + NumberEnd> {};
  struct FLOAT
      : terminal<IntBody + ((lit<"."> + some(d) +
                             option(cr<"eE"> + option(cr<"+-">) + some(d))) |
                            (cr<"eE"> + option(cr<"+-">) + some(d))) + NumberEnd> {};

  struct BOOL : terminal<as<bool>(kw<"true"> | kw<"false">)> {};
  struct NULLK : terminal<as<null_value>(kw<"null">)> {};

  static constexpr auto OperationTypeExpr =
      as<ast::OperationKind::Query>(kw<"query">) | as<ast::OperationKind::Mutation>(kw<"mutation">) | as<ast::OperationKind::Subscription>(kw<"subscription">);

  static constexpr auto DirectiveLocationExpr =
      as<ast::DirectiveLocationKind::Query>(kw<"QUERY">) | as<ast::DirectiveLocationKind::Mutation>(kw<"MUTATION">) | as<ast::DirectiveLocationKind::Subscription>(kw<"SUBSCRIPTION">) | as<ast::DirectiveLocationKind::Field>(kw<"FIELD">) |
      as<ast::DirectiveLocationKind::FragmentDefinition>(kw<"FRAGMENT_DEFINITION">) | as<ast::DirectiveLocationKind::FragmentSpread>(kw<"FRAGMENT_SPREAD">) |
      as<ast::DirectiveLocationKind::InlineFragment>(kw<"INLINE_FRAGMENT">) | as<ast::DirectiveLocationKind::VariableDefinition>(kw<"VARIABLE_DEFINITION">) | as<ast::DirectiveLocationKind::Schema>(kw<"SCHEMA">) |
      as<ast::DirectiveLocationKind::Scalar>(kw<"SCALAR">) | as<ast::DirectiveLocationKind::Object>(kw<"OBJECT">) | as<ast::DirectiveLocationKind::FieldDefinition>(kw<"FIELD_DEFINITION">) |
      as<ast::DirectiveLocationKind::ArgumentDefinition>(kw<"ARGUMENT_DEFINITION">) | as<ast::DirectiveLocationKind::Interface>(kw<"INTERFACE">) | as<ast::DirectiveLocationKind::Union>(kw<"UNION">) |
      as<ast::DirectiveLocationKind::Enum>(kw<"ENUM">) | as<ast::DirectiveLocationKind::EnumValue>(kw<"ENUM_VALUE">) | as<ast::DirectiveLocationKind::InputObject>(kw<"INPUT_OBJECT">) |
      as<ast::DirectiveLocationKind::InputFieldDefinition>(kw<"INPUT_FIELD_DEFINITION">);

  static constexpr auto DescriptionExpr = ref<STRING> | ref<BLOCK_STRING>;

  struct Variable : rule<create<ast::Variable>(
                            lit<"$"> + assign<&ast::Variable::name>(ref<NAME>))> {};
  struct IntValue
      : rule<assign<&ast::IntLit::value>(ref<INT>)> {};
  struct FloatValue
      : rule<assign<&ast::FloatLit::value>(ref<FLOAT>)> {};
  struct EnumLit
      : rule<create<ast::EnumLit>(assign<&ast::EnumLit::name>(ref<NAME>))> {};

  struct ListValue;
  struct ObjectValue;
  static constexpr auto ValueExpr = ref<Variable> | ref<FloatValue> |
                                    ref<IntValue> |
                                    ref<STRING> | ref<BLOCK_STRING> |
                                    ref<BOOL> | ref<NULLK> | ref<EnumLit> |
                                    ref<ListValue> | ref<ObjectValue>;

  struct ListValue
      : rule<create<ast::ListValue>(
            lit<"["> + many(append<&ast::ListValue::items>(ValueExpr)) +
            lit<"]">)> {};

  struct ObjectField
      : rule<create<ast::ObjectField>(
            assign<&ast::ObjectField::name>(ref<NAME>) + lit<":"> +
            assign<&ast::ObjectField::value>(ValueExpr))> {};
  struct ObjectValue
      : rule<create<ast::ObjectValue>(
            lit<"{"> +
            many(append<&ast::ObjectValue::fields>(ref<ObjectField>)) +
            lit<"}">)> {};

  struct TypeRef;
  struct NamedTypeRef
      : rule<create<ast::NamedTypeRef>(
            assign<&ast::NamedTypeRef::name>(ref<NAME>) +
            option(flag<&ast::NamedTypeRef::nonNull>(lit<"!">)))> {};
  struct ListTypeRef
      : rule<create<ast::ListTypeRef>(
            lit<"["> + assign<&ast::ListTypeRef::itemType>(ref<TypeRef>) +
            lit<"]"> +
            option(flag<&ast::ListTypeRef::nonNull>(lit<"!">)))> {};
  struct TypeRef : rule<ref<NamedTypeRef> | ref<ListTypeRef>> {};

  struct Argument : rule<create<ast::Argument>(
                            assign<&ast::Argument::name>(ref<NAME>) + lit<":"> +
                            assign<&ast::Argument::value>(ValueExpr))> {};

  struct Directive
      : rule<create<ast::Directive>(
            lit<"@"> + assign<&ast::Directive::name>(ref<NAME>) +
            option(lit<"("> +
                   some(append<&ast::Directive::arguments>(ref<Argument>)) +
                   lit<")">))> {};

  struct InputValueDefinition
      : rule<create<ast::InputValueDefinition>(
            option(assign<&ast::InputValueDefinition::description>(
                DescriptionExpr)) +
            assign<&ast::InputValueDefinition::name>(ref<NAME>) + lit<":"> +
            assign<&ast::InputValueDefinition::type>(ref<TypeRef>) +
            option(lit<"="> + assign<&ast::InputValueDefinition::defaultValue>(
                                  ValueExpr)) +
            many(append<&ast::InputValueDefinition::directives>(
                ref<Directive>)))> {};

  struct FieldDefinition
      : rule<create<ast::FieldDefinition>(
            option(
                assign<&ast::FieldDefinition::description>(DescriptionExpr)) +
            assign<&ast::FieldDefinition::name>(ref<NAME>) +
            option(lit<"("> +
                   some(append<&ast::FieldDefinition::arguments>(
                       ref<InputValueDefinition>)) +
                   lit<")">) +
            lit<":"> + assign<&ast::FieldDefinition::type>(ref<TypeRef>) +
            many(append<&ast::FieldDefinition::directives>(
                ref<Directive>)))> {};

  struct SelectionSet;
  struct Field
      : rule<create<ast::Field>(
            option(assign<&ast::Field::alias>(ref<NAME>) + lit<":">) +
            assign<&ast::Field::name>(ref<NAME>) +
            option(lit<"("> +
                   some(append<&ast::Field::arguments>(ref<Argument>)) +
                   lit<")">) +
            many(append<&ast::Field::directives>(ref<Directive>)) +
            option(assign<&ast::Field::selectionSet>(ref<SelectionSet>)))> {
  };
  struct FragmentSpread
      : rule<create<ast::FragmentSpread>(
            lit<"..."> + assign<&ast::FragmentSpread::name>(ref<NAME>) +
            many(append<&ast::FragmentSpread::directives>(
                ref<Directive>)))> {};
  struct InlineFragment
      : rule<create<ast::InlineFragment>(
            lit<"..."> +
            option(kw<"on"> +
                   assign<&ast::InlineFragment::typeCondition>(ref<NAME>)) +
            many(append<&ast::InlineFragment::directives>(ref<Directive>)) +
            assign<&ast::InlineFragment::selectionSet>(
                ref<SelectionSet>))> {};
  struct Selection : rule<ref<Field> | ref<FragmentSpread> |
                              ref<InlineFragment>> {};
  struct SelectionSet
      : rule<create<ast::SelectionSet>(
            lit<"{"> +
            some(append<&ast::SelectionSet::selections>(ref<Selection>)) +
            lit<"}">)> {};

  struct VariableDefinition
      : rule<create<ast::VariableDefinition>(
            lit<"$"> + assign<&ast::VariableDefinition::name>(ref<NAME>) +
            lit<":"> +
            assign<&ast::VariableDefinition::type>(ref<TypeRef>) +
            option(lit<"="> +
                   assign<&ast::VariableDefinition::defaultValue>(ValueExpr)) +
            many(append<&ast::VariableDefinition::directives>(
                ref<Directive>)))> {};

  struct OperationDefinition
      : rule<create<ast::OperationDefinition>(
            (assign<&ast::OperationDefinition::operation>(OperationTypeExpr) +
             option(assign<&ast::OperationDefinition::name>(ref<NAME>)) +
             option(lit<"("> +
                    some(append<&ast::OperationDefinition::variableDefinitions>(
                        ref<VariableDefinition>)) +
                    lit<")">) +
             many(append<&ast::OperationDefinition::directives>(
                 ref<Directive>)) +
             assign<&ast::OperationDefinition::selectionSet>(
                 ref<SelectionSet>)) |
            assign<&ast::OperationDefinition::selectionSet>(
                ref<SelectionSet>))> {};

  struct FragmentDefinition
      : rule<create<ast::FragmentDefinition>(
            kw<"fragment"> + assign<&ast::FragmentDefinition::name>(ref<NAME>) +
            kw<"on"> +
            assign<&ast::FragmentDefinition::typeCondition>(ref<NAME>) +
            many(append<&ast::FragmentDefinition::directives>(
                ref<Directive>)) +
            assign<&ast::FragmentDefinition::selectionSet>(
                ref<SelectionSet>))> {};

  struct RootOperationTypeDefinition
      : rule<create<ast::RootOperationTypeDefinition>(
            assign<&ast::RootOperationTypeDefinition::operation>(
                OperationTypeExpr) +
            lit<":"> +
            assign<&ast::RootOperationTypeDefinition::type>(ref<NAME>))> {};

  struct SchemaDefinition
      : rule<create<ast::SchemaDefinition>(
            option(assign<&ast::SchemaDefinition::description>(DescriptionExpr)) +
            kw<"schema"> +
            many(append<&ast::SchemaDefinition::directives>(
                ref<Directive>)) +
            lit<"{"> +
            some(append<&ast::SchemaDefinition::operationTypes>(
                ref<RootOperationTypeDefinition>)) +
            lit<"}">)> {};

  // Incomplete extensions are retained for the empty-extension validator.
  struct SchemaExtension
      : rule<create<ast::SchemaExtension>(
            kw<"extend"> + kw<"schema"> +
            many(
                append<&ast::SchemaExtension::directives>(ref<Directive>)) +
            option(lit<"{"> +
                   some(append<&ast::SchemaExtension::operationTypes>(
                       ref<RootOperationTypeDefinition>)) +
                   lit<"}">))> {};

  struct ScalarTypeDefinition
      : rule<create<ast::ScalarTypeDefinition>(
            option(assign<&ast::ScalarTypeDefinition::description>(
                DescriptionExpr)) +
            kw<"scalar"> + assign<&ast::ScalarTypeDefinition::name>(ref<NAME>) +
            many(append<&ast::ScalarTypeDefinition::directives>(
                ref<Directive>)))> {};

  struct ScalarTypeExtension
      : rule<create<ast::ScalarTypeExtension>(
            kw<"extend"> + kw<"scalar"> +
            assign<&ast::ScalarTypeExtension::name>(ref<NAME>) +
            many(append<&ast::ScalarTypeExtension::directives>(
                ref<Directive>)))> {};

  struct ObjectTypeDefinition
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
                ref<Directive>)) +
            option(lit<"{"> +
                   some(append<&ast::ObjectTypeDefinition::fields>(
                       ref<FieldDefinition>)) +
                   lit<"}">))> {};

  struct ObjectTypeExtension
      : rule<create<ast::ObjectTypeExtension>(
            kw<"extend"> + kw<"type"> +
            assign<&ast::ObjectTypeExtension::name>(ref<NAME>) +
            option(
                kw<"implements"> + option(lit<"&">) +
                append<&ast::ObjectTypeExtension::interfaces>(ref<NAME>) +
                many(lit<"&"> + append<&ast::ObjectTypeExtension::interfaces>(
                                    ref<NAME>))) +
            many(append<&ast::ObjectTypeExtension::directives>(
                ref<Directive>)) +
            option(lit<"{"> +
                   some(append<&ast::ObjectTypeExtension::fields>(
                       ref<FieldDefinition>)) +
                   lit<"}">))> {};

  struct InterfaceTypeDefinition
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
                ref<Directive>)) +
            option(lit<"{"> +
                   some(append<&ast::InterfaceTypeDefinition::fields>(
                       ref<FieldDefinition>)) +
                   lit<"}">))> {};

  struct InterfaceTypeExtension
      : rule<create<ast::InterfaceTypeExtension>(
            kw<"extend"> + kw<"interface"> +
            assign<&ast::InterfaceTypeExtension::name>(ref<NAME>) +
            option(kw<"implements"> + option(lit<"&">) +
                   append<&ast::InterfaceTypeExtension::interfaces>(ref<NAME>) +
                   many(lit<"&"> +
                        append<&ast::InterfaceTypeExtension::interfaces>(
                            ref<NAME>))) +
            many(append<&ast::InterfaceTypeExtension::directives>(
                ref<Directive>)) +
            option(lit<"{"> +
                   some(append<&ast::InterfaceTypeExtension::fields>(
                       ref<FieldDefinition>)) +
                   lit<"}">))> {};

  struct UnionTypeDefinition
      : rule<create<ast::UnionTypeDefinition>(
            option(assign<&ast::UnionTypeDefinition::description>(
                DescriptionExpr)) +
            kw<"union"> + assign<&ast::UnionTypeDefinition::name>(ref<NAME>) +
            many(append<&ast::UnionTypeDefinition::directives>(
                ref<Directive>)) +
            option(lit<"="> + option(lit<"|">) +
                   append<&ast::UnionTypeDefinition::members>(ref<NAME>) +
                   many(lit<"|"> + append<&ast::UnionTypeDefinition::members>(
                                       ref<NAME>))))> {};

  struct UnionTypeExtension
      : rule<create<ast::UnionTypeExtension>(
            kw<"extend"> + kw<"union"> +
            assign<&ast::UnionTypeExtension::name>(ref<NAME>) +
            many(append<&ast::UnionTypeExtension::directives>(
                ref<Directive>)) +
            option(lit<"="> + option(lit<"|">) +
                   append<&ast::UnionTypeExtension::members>(ref<NAME>) +
                   many(lit<"|"> + append<&ast::UnionTypeExtension::members>(
                                       ref<NAME>))))> {};

  struct EnumValueDefinition
      : rule<create<ast::EnumValueDefinition>(
            option(assign<&ast::EnumValueDefinition::description>(
                DescriptionExpr)) +
            assign<&ast::EnumValueDefinition::name>(ref<NAME>) +
            many(append<&ast::EnumValueDefinition::directives>(
                ref<Directive>)))> {};

  struct EnumTypeDefinition
      : rule<create<ast::EnumTypeDefinition>(
            option(assign<&ast::EnumTypeDefinition::description>(
                DescriptionExpr)) +
            kw<"enum"> + assign<&ast::EnumTypeDefinition::name>(ref<NAME>) +
            many(append<&ast::EnumTypeDefinition::directives>(
                ref<Directive>)) +
            option(lit<"{"> +
                   some(append<&ast::EnumTypeDefinition::values>(
                       ref<EnumValueDefinition>)) +
                   lit<"}">))> {};

  struct EnumTypeExtension
      : rule<create<ast::EnumTypeExtension>(
            kw<"extend"> + kw<"enum"> +
            assign<&ast::EnumTypeExtension::name>(ref<NAME>) +
            many(append<&ast::EnumTypeExtension::directives>(
                ref<Directive>)) +
            option(lit<"{"> +
                   some(append<&ast::EnumTypeExtension::values>(
                       ref<EnumValueDefinition>)) +
                   lit<"}">))> {};

  struct InputObjectTypeDefinition
      : rule<create<ast::InputObjectTypeDefinition>(
            option(assign<&ast::InputObjectTypeDefinition::description>(
                DescriptionExpr)) +
            kw<"input"> +
            assign<&ast::InputObjectTypeDefinition::name>(ref<NAME>) +
            many(append<&ast::InputObjectTypeDefinition::directives>(
                ref<Directive>)) +
            option(lit<"{"> +
                   some(append<&ast::InputObjectTypeDefinition::fields>(
                       ref<InputValueDefinition>)) +
                   lit<"}">))> {};

  struct InputObjectTypeExtension
      : rule<create<ast::InputObjectTypeExtension>(
            kw<"extend"> + kw<"input"> +
            assign<&ast::InputObjectTypeExtension::name>(ref<NAME>) +
            many(append<&ast::InputObjectTypeExtension::directives>(
                ref<Directive>)) +
            option(lit<"{"> +
                   some(append<&ast::InputObjectTypeExtension::fields>(
                       ref<InputValueDefinition>)) +
                   lit<"}">))> {};

  struct DirectiveDefinition
      : rule<create<ast::DirectiveDefinition>(
            option(assign<&ast::DirectiveDefinition::description>(
                DescriptionExpr)) +
            kw<"directive"> + lit<"@"> +
            assign<&ast::DirectiveDefinition::name>(ref<NAME>) +
            option(lit<"("> +
                   some(append<&ast::DirectiveDefinition::arguments>(
                       ref<InputValueDefinition>)) +
                   lit<")">) +
            option(flag<&ast::DirectiveDefinition::repeatable>(
                kw<"repeatable">)) +
            kw<"on"> + option(lit<"|">) +
            some(append<&ast::DirectiveDefinition::locations>(
                     DirectiveLocationExpr), lit<"|">))> {};

  // A document is a nonempty sequence of definitions, grouped or
  // interleaved, flattened to one `definition+`.
  struct Definition
      : rule<ref<OperationDefinition> | ref<FragmentDefinition> |
             ref<SchemaDefinition> | ref<ScalarTypeDefinition> |
             ref<ObjectTypeDefinition> | ref<InterfaceTypeDefinition> |
             ref<UnionTypeDefinition> | ref<EnumTypeDefinition> |
             ref<InputObjectTypeDefinition> | ref<DirectiveDefinition> |
             ref<SchemaExtension> | ref<ScalarTypeExtension> |
             ref<ObjectTypeExtension> | ref<InterfaceTypeExtension> |
             ref<UnionTypeExtension> | ref<EnumTypeExtension> |
             ref<InputObjectTypeExtension>> {};

  struct Document
      : rule<create<ast::Document>(
            some(append<&ast::Document::definitions>(ref<Definition>)))> {};

  using entry = Document;
  using ignored = list<WS, COMMA, LineComment, Bom>;
};

} // namespace graphql
