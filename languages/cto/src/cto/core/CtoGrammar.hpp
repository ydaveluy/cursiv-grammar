// The cto language: its grammar.
#pragma once

#include <charconv>
#include <cstdint>
#include <string>
#include <string_view>

#include "cto/core/ast.hpp"
#include "cto/core/TextValue.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace cto {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};

// Strip separators and the optional type suffix, preserving every digit.
inline std::string numeric_text(std::string_view t, std::string_view suffixes) {
  if (!t.empty() && t.front() == '+') t.remove_prefix(1);
  if (!t.empty() && suffixes.find(t.back()) != std::string_view::npos)
    t.remove_suffix(1);
  std::string text;
  text.reserve(t.size());
  for (const char c : t)
    if (c != '_') text.push_back(c);
  return text;
}

template <int Base>
inline constexpr auto to_integer = [](std::string_view t) {
  const std::string text = numeric_text(t, "lL");
  std::int64_t value = 0;
  const auto [end, error] =
      std::from_chars(text.data(), text.data() + text.size(), value, Base);
  using Result = cursiv::ConversionResult<std::int64_t>;
  if (error != std::errc{} || end != text.data() + text.size())
    return Result::err("Integer literal is outside the signed 64-bit range.");
  return Result::ok(value);
};

inline constexpr auto to_double = [](std::string_view t) {
  const std::string text = numeric_text(t, "fFdD");
  double value = 0.0;
  const auto [end, error] =
      std::from_chars(text.data(), text.data() + text.size(), value);
  using Result = cursiv::ConversionResult<double>;
  if (error != std::errc{} || end != text.data() + text.size())
    return Result::err("Floating-point literal is outside the double range.");
  return Result::ok(value);
};

struct Cto : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n\f)">)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  // `$` and non-ASCII characters are word characters too, so a keyword does
  // not read as ended before one.
  static constexpr auto word_boundary = cr<R"(a-zA-Z0-9_$\u{80}-\u{10FFFF})">;

  // A leading digit would tie a bare number with an identifier of the same
  // spelling at the same length, which the lexer cannot order. Restricted to
  // a non-digit lead, disjoint from the numeric literals below.
  struct IDENTIFIER
      : terminal<(uc::XID_Start | cr<"_$">) + many(uc::XID_Continue | cr<"_$">)> {
  };

  // Dual-use: a keyword that opens an asset/participant declaration, and
  // (via the field name below) a name a field may take.
  struct ASSET : terminal<kw<"asset">> {};
  struct PARTICIPANT : terminal<kw<"participant">> {};

  static constexpr auto fieldName = ref<IDENTIFIER> | ref<ASSET> | ref<PARTICIPANT>;

  static constexpr auto digits = cr<"0-9"> + option(many(cr<"0-9_">) + cr<"0-9">);

  // Composer's SignedInteger is decimal even with leading zeroes.
  // Separators and L suffixes remain explicit lexical port extensions.
  struct DECIMAL_LITERAL
      : terminal<as<to_integer<10>>(option(cr<"+-">) + digits + option(cr<"lL">))> {};

  static constexpr auto exponent_part = cr<"eE"> + option(cr<"+-">) + digits;
  static constexpr auto float_frac =
      ((digits + lit<"."> + option(digits)) | (lit<"."> + digits)) +
      option(exponent_part);
  struct FLOAT_LITERAL
      : terminal<as<to_double>(
            option(cr<"+-">) + ((float_frac + option(cr<"fFdD">)) |
            (digits + ((exponent_part + option(cr<"fFdD">)) | cr<"fFdD">))))> {};

  inline static constexpr auto to_hex = [](std::string_view text) {
    text.remove_prefix(2);
    return to_integer<16>(text);
  };
  struct HEX_LITERAL : terminal<as<to_hex>(lit<"0"> + cr<"xX"> + some(cr<"0-9a-fA-F">))> {};

  struct BOOL_LITERAL : terminal<as<bool>(kw<"true"> | kw<"false">)> {};

  // `'` is CHAR_LITERAL's own delimiter, excluded from its body so an
  // unescaped one closes it.
  static constexpr auto octal_escape =
      option(option(cr<"0-3">) + cr<"0-7">) + cr<"0-7">;
  static constexpr auto unicode_escape =
      some(lit<"u">) + repeat<4>(cr<"0-9a-fA-F">);
  static constexpr auto escape =
      lit<"\\"> + (unicode_escape | (lit<"x"> + repeat<2>(cr<"0-9a-fA-F">)) |
                     octal_escape | lit<"\r\n"> | cr<"^ux0-9">);
  struct CHAR_LITERAL
      : terminal<as<string_value>(lit<"'"> + many(escape | cr<R"(^'\\\\\r\n\u{2028}\u{2029})">) +
                             lit<"'">)> {};
  struct STRING_LITERAL
      : terminal<as<string_value>(lit<"\""> + many(escape | cr<R"(^"\\\\\r\n\u{2028}\u{2029})">) +
                             lit<"\"">)> {};

  static constexpr auto regexEscape = lit<"\\"> + cr<R"(^\r\n)">;
  static constexpr auto regexClass = lit<"["> +
      many(regexEscape | cr<R"(^]\\\r\n)">) + lit<"]">;
  struct REGEX_EXPR : terminal<as<unquote>(lit<"/"> +
      (regexEscape | regexClass | cr<R"(^*/[\\\r\n)">) +
      many(regexEscape | regexClass | cr<R"(^/[\\\r\n)">) + lit<"/">)> {};
  struct IMPORT_URI : terminal<some(cr<R"(^ \t\r\n\f{})">)> {};

  static constexpr auto literalExpr = ref<BOOL_LITERAL> | ref<DECIMAL_LITERAL> |
                                      ref<FLOAT_LITERAL> | ref<HEX_LITERAL> |
                                      ref<CHAR_LITERAL> | ref<STRING_LITERAL>;
  static constexpr auto numberExpr =
      ref<DECIMAL_LITERAL> | ref<FLOAT_LITERAL>;

  struct QualifiedName : rule<some(ref<IDENTIFIER>, lit<".">)> {};

  struct Decorator
      : rule<lit<"@"> + assign<&ast::Decorator::name>(ref<QualifiedName>) +
             option(lit<"("> +
                    option(some(append<&ast::Decorator::arguments>(
                            create<ast::DecoratorIdentifier>(
                                assign<&ast::DecoratorIdentifier::name>(ref<IDENTIFIER>) +
                                option(flag<&ast::DecoratorIdentifier::isArray>(lit<"["> + lit<"]">))) |
                            literalExpr),
                        lit<",">)) +
                    lit<")">)> {};

  static constexpr auto square = lit<"["> + lit<"]">;
  static constexpr auto arrayFlag =
      option(flag<&ast::Property::isArray>(some(square)));

  struct Property
      : rule<many(append<&ast::Property::decorators>(ref<Decorator>)) +
             ((kw<"o"> + assign<&ast::Property::typeName>(kw<"String">) +
              arrayFlag + assign<&ast::Property::name>(fieldName) +
              option(kw<"default"> + lit<"="> +
                     assign<&ast::Property::defaultValue>(
                         ref<CHAR_LITERAL> | ref<STRING_LITERAL>)) +
              option(kw<"regex"> + lit<"="> +
                     assign<&ast::Property::regex>(ref<REGEX_EXPR>) +
                     option(gap::none + assign<&ast::Property::regexFlags>(ref<IDENTIFIER>))) +
              option(flag<&ast::Property::isOptional>(kw<"optional">))) |
             (kw<"o"> + assign<&ast::Property::typeName>(kw<"Boolean">) +
              arrayFlag + assign<&ast::Property::name>(fieldName) +
              option(kw<"default"> + lit<"="> +
                     assign<&ast::Property::defaultValue>(ref<BOOL_LITERAL>)) +
              option(flag<&ast::Property::isOptional>(kw<"optional">))) |
             (kw<"o"> +
              assign<&ast::Property::typeName>(kw<"Double"> | kw<"Integer"> |
                                               kw<"Long">) +
              arrayFlag + assign<&ast::Property::name>(fieldName) +
              option(kw<"default"> + lit<"="> +
                     assign<&ast::Property::defaultValue>(numberExpr)) +
              option(kw<"range"> + lit<"="> + lit<"["> +
                     option(assign<&ast::Property::rangeMin>(numberExpr)) +
                     lit<","> +
                     option(assign<&ast::Property::rangeMax>(numberExpr)) +
                     lit<"]">) +
              option(flag<&ast::Property::isOptional>(kw<"optional">))) |
             (kw<"o"> + assign<&ast::Property::typeName>(kw<"DateTime">) +
              arrayFlag + assign<&ast::Property::name>(fieldName) +
              option(kw<"default"> + lit<"="> +
                     assign<&ast::Property::defaultValue>(
                         ref<CHAR_LITERAL> | ref<STRING_LITERAL>)) +
              option(flag<&ast::Property::isOptional>(kw<"optional">))) |
             (kw<"o"> + assign<&ast::Property::typeName>(ref<IDENTIFIER>) +
              arrayFlag + assign<&ast::Property::name>(fieldName) +
              option(kw<"default"> + lit<"="> +
                     assign<&ast::Property::defaultValue>(ref<CHAR_LITERAL> | ref<STRING_LITERAL>)) +
              option(flag<&ast::Property::isOptional>(kw<"optional">))) |
             (flag<&ast::Property::isReference>(lit<"-->">) +
              assign<&ast::Property::typeName>(ref<IDENTIFIER>) + arrayFlag +
              assign<&ast::Property::name>(fieldName) +
              option(flag<&ast::Property::isOptional>(kw<"optional">))))> {};

  static constexpr auto classModifiers =
      many(append<&ast::ClassDeclaration::decorators>(ref<Decorator>) |
           flag<&ast::ClassDeclaration::isAbstract>(kw<"abstract">));

  static constexpr auto classHeritage =
      option(kw<"identified"> + kw<"by"> +
             assign<&ast::ClassDeclaration::identifiedBy>(ref<IDENTIFIER>)) +
      option(kw<"extends"> +
             assign<&ast::ClassDeclaration::extends>(ref<IDENTIFIER>));

  static constexpr auto classBody =
      lit<"{"> +
      many(lit<";"> | append<&ast::ClassDeclaration::properties>(ref<Property>)) +
      lit<"}">;

  struct AssetDeclaration
      : rule<create<ast::AssetDeclaration>(
            classModifiers + ref<ASSET> +
            assign<&ast::Declaration::name>(ref<IDENTIFIER>) +
            classHeritage + classBody)> {};

  struct ConceptDeclaration
      : rule<create<ast::ConceptDeclaration>(
            classModifiers + kw<"concept"> +
            assign<&ast::Declaration::name>(ref<IDENTIFIER>) +
            option(kw<"extends"> +
                   assign<&ast::ClassDeclaration::extends>(ref<IDENTIFIER>)) +
            classBody)> {};

  struct ParticipantDeclaration
      : rule<create<ast::ParticipantDeclaration>(
            classModifiers + ref<PARTICIPANT> +
            assign<&ast::Declaration::name>(ref<IDENTIFIER>) +
            classHeritage + classBody)> {};

  struct TransactionDeclaration
      : rule<create<ast::TransactionDeclaration>(
            classModifiers + kw<"transaction"> +
            assign<&ast::Declaration::name>(ref<IDENTIFIER>) + classHeritage + classBody)> {};

  struct EventDeclaration
      : rule<create<ast::EventDeclaration>(
            classModifiers + kw<"event"> + assign<&ast::Declaration::name>(ref<IDENTIFIER>) +
            classHeritage + classBody)> {};

  struct EnumConstant
      : rule<many(append<&ast::EnumConstant::decorators>(ref<Decorator>)) +
             kw<"o"> + assign<&ast::EnumConstant::name>(ref<IDENTIFIER>) +
             option(flag<&ast::EnumConstant::isOptional>(kw<"optional">))> {};

  struct EnumDeclaration
      : rule<create<ast::EnumDeclaration>(
            many(append<&ast::EnumDeclaration::decorators>(ref<Decorator>)) +
            kw<"enum"> + assign<&ast::Declaration::name>(ref<IDENTIFIER>) +
            lit<"{"> +
            many(append<&ast::EnumDeclaration::constants>(ref<EnumConstant>)) +
            lit<"}">)> {};

  struct Import
      : rule<kw<"import"> +
             assign<&ast::Import::path>(ref<QualifiedName>) +
             option(lit<"."> +
                    flag<&ast::Import::wildcard>(lit<"*">)) +
             option(kw<"from"> + assign<&ast::Import::uri>(ref<IMPORT_URI>))> {};

  struct Model
      : rule<create<ast::Model>(
            kw<"namespace"> +
            assign<&ast::Model::namespaceName>(ref<QualifiedName>) +
            many(append<&ast::Model::imports>(ref<Import>)) +
            many(append<&ast::Model::declarations>(
                ref<AssetDeclaration> | ref<ConceptDeclaration> |
                ref<EnumDeclaration> | ref<ParticipantDeclaration> |
                ref<TransactionDeclaration> | ref<EventDeclaration>)))> {
  };

  using entry = Model;
  using ignored = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace cto
