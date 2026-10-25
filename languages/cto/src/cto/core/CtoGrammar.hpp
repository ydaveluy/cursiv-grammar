// The cto language: its grammar.
#pragma once

#include <charconv>
#include <cstdint>
#include <string_view>

#include "cto/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace cto {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unquoted = cursiv::text_conv<unquote>;

// A digit run may carry `_` separators and a trailing `l`/`L`/`f`/`F`/`d`/`D`
// unit suffix; none of that reaches `std::from_chars`, so both are scrubbed
// before conversion.
inline constexpr auto to_decimal = [](std::string_view t) {
  std::int64_t v = 0;
  for (const char c : t) {
    if (c == '_') continue;
    if (c == 'l' || c == 'L') break;
    v = v * 10 + (c - '0');
  }
  return v;
};
using DecimalValue = cursiv::text_conv<to_decimal>;

inline constexpr auto to_octal = [](std::string_view t) {
  std::int64_t v = 0;
  for (const char c : t) {
    if (c == '_') continue;
    if (c == 'l' || c == 'L') break;
    v = v * 8 + (c - '0');
  }
  return v;
};
using OctalValue = cursiv::text_conv<to_octal>;

inline constexpr auto to_double = [](std::string_view t) {
  char buf[64];
  std::size_t n = 0;
  for (const char c : t) {
    if (c == '_') continue;
    if (c == 'f' || c == 'F' || c == 'd' || c == 'D') break;
    if (n < sizeof(buf)) buf[n++] = c;
  }
  double v = 0.0;
  std::from_chars(buf, buf + n, v);
  return v;
};
using FloatValue = cursiv::text_conv<to_double>;

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

  struct DECIMAL_LITERAL
      : terminal<(lit<"0"> |
                  (cr<"1-9"> + (option(digits) | (some(lit<"_">) + digits)))) +
                     option(cr<"lL">),
                 DecimalValue> {};

  struct OCT_LITERAL
      : terminal<lit<"0"> + many(lit<"_">) + cr<"0-7"> +
                     option(many(cr<"0-7_">) + cr<"0-7">) + option(cr<"lL">),
                 OctalValue> {};

  static constexpr auto exponent_part = cr<"eE"> + option(cr<"+-">) + digits;
  static constexpr auto float_frac =
      ((digits + lit<"."> + option(digits)) | (lit<"."> + digits)) +
      option(exponent_part);
  struct FLOAT_LITERAL
      : terminal<(float_frac + option(cr<"fFdD">)) |
                     (digits +
                      ((exponent_part + option(cr<"fFdD">)) | cr<"fFdD">)),
                 FloatValue> {};

  struct BOOL_LITERAL : terminal<kw<"true"> | kw<"false">, bool> {};

  // The opening and closing bound are read independently, with no
  // requirement that they match -- not `mark`/`same`, so `"...'` is
  // accepted too.
  static constexpr auto digit = cr<"0-9">;
  static constexpr auto year = repeat<4>(digit);
  static constexpr auto month = (lit<"0"> + digit) | (lit<"1"> + cr<"0-2">);
  static constexpr auto day = (cr<"0-2"> + digit) | (lit<"3"> + cr<"01">);
  static constexpr auto full_date = year + lit<"-"> + month + lit<"-"> + day;
  static constexpr auto sixty = cr<"0-5"> + digit;
  static constexpr auto half_hour = lit<"00"> | lit<"30">;
  static constexpr auto partial_time = repeat<2>(digit) + lit<":"> +
                                       sixty + lit<":"> + sixty +
                                       option(lit<"."> + many(digit));
  static constexpr auto time_num_offset =
      (lit<"-"> + cr<"01"> + cr<"0-2"> + option(lit<":"> + half_hour)) |
      (lit<"+"> + cr<"01"> + cr<"0-5"> +
       option(lit<":"> + (half_hour | lit<"45">)));
  static constexpr auto time_offset = lit<"Z"> | time_num_offset;
  static constexpr auto full_time = partial_time + time_offset;
  static constexpr auto bound = lit<"\""> | lit<"'">;
  struct DATE_TIME_LITERAL
      : terminal<bound + full_date + lit<"T"> + full_time + bound, Unquoted> {
  };

  // `'` is CHAR_LITERAL's own delimiter, excluded from its body so an
  // unescaped one closes it.
  static constexpr auto octal_escape =
      option(option(cr<"0-3">) + cr<"0-7">) + cr<"0-7">;
  static constexpr auto unicode_escape =
      some(lit<"u">) + repeat<4>(cr<"0-9a-fA-F">);
  static constexpr auto escape =
      lit<"\\"> + (cr<R"(btnfr"'\\\\)"> | unicode_escape | octal_escape);
  struct CHAR_LITERAL
      : terminal<lit<"'"> + many(escape | cr<R"(^'\\\\\r\n)">) + lit<"'">,
                 Unquoted> {};
  struct STRING_LITERAL
      : terminal<lit<"\""> + many(escape | cr<R"(^"\\\\\r\n)">) + lit<"\"">,
                 Unquoted> {};

  struct REGEX_EXPR : terminal<through(lit<"/">, lit<"/">), Unquoted> {};

  static constexpr auto literalExpr = ref<BOOL_LITERAL> | ref<DECIMAL_LITERAL> |
                                      ref<OCT_LITERAL> | ref<FLOAT_LITERAL> |
                                      ref<CHAR_LITERAL> | ref<STRING_LITERAL>;
  static constexpr auto numberExpr =
      ref<DECIMAL_LITERAL> | ref<OCT_LITERAL> | ref<FLOAT_LITERAL>;

  struct QualifiedNameRule : rule<some(ref<IDENTIFIER>, lit<".">)> {};

  struct DecoratorRule
      : rule<lit<"@"> + assign<&ast::Decorator::name>(ref<QualifiedNameRule>) +
             option(lit<"("> +
                    some(append<&ast::Decorator::arguments>(literalExpr),
                        lit<",">) +
                    lit<")">)> {};

  static constexpr auto square = lit<"["> + lit<"]">;
  static constexpr auto arrayFlag =
      option(flag<&ast::Property::isArray>(some(square)));

  struct PropertyRule
      : rule<(kw<"o"> + assign<&ast::Property::typeName>(kw<"String">) +
              arrayFlag + assign<&ast::Property::name>(fieldName) +
              option(kw<"default"> + lit<"="> +
                     assign<&ast::Property::defaultValue>(
                         ref<CHAR_LITERAL> | ref<STRING_LITERAL>)) +
              option(kw<"regex"> + lit<"="> +
                     assign<&ast::Property::regex>(ref<REGEX_EXPR>)) +
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
                     ((assign<&ast::Property::rangeMin>(numberExpr) +
                       lit<","> +
                       option(assign<&ast::Property::rangeMax>(numberExpr))) |
                      (lit<","> +
                       assign<&ast::Property::rangeMax>(numberExpr))) +
                     lit<"]">) +
              option(flag<&ast::Property::isOptional>(kw<"optional">))) |
             (kw<"o"> + assign<&ast::Property::typeName>(kw<"DateTime">) +
              arrayFlag + assign<&ast::Property::name>(fieldName) +
              option(kw<"default"> + lit<"="> +
                     assign<&ast::Property::defaultValue>(
                         ref<DATE_TIME_LITERAL>)) +
              option(flag<&ast::Property::isOptional>(kw<"optional">))) |
             (kw<"o"> + assign<&ast::Property::typeName>(ref<IDENTIFIER>) +
              arrayFlag + assign<&ast::Property::name>(fieldName)) |
             (flag<&ast::Property::isReference>(lit<"-->">) +
              assign<&ast::Property::typeName>(ref<IDENTIFIER>) + arrayFlag +
              assign<&ast::Property::name>(fieldName))> {};

  static constexpr auto classModifiers =
      many(append<&ast::ClassDeclaration::decorators>(ref<DecoratorRule>) |
           flag<&ast::ClassDeclaration::isAbstract>(kw<"abstract">));

  static constexpr auto extendsOrIdentified =
      (kw<"extends"> + assign<&ast::ClassDeclaration::extends>(ref<IDENTIFIER>)) |
      ((kw<"identified"> + kw<"by">)+
       assign<&ast::ClassDeclaration::identifiedBy>(ref<IDENTIFIER>));

  static constexpr auto classBody =
      lit<"{"> +
      many(lit<";"> | append<&ast::ClassDeclaration::properties>(ref<PropertyRule>)) +
      lit<"}">;

  struct AssetDeclarationRule
      : rule<create<ast::AssetDeclaration>(
            classModifiers + ref<ASSET> +
            assign<&ast::Declaration::name>(ref<IDENTIFIER>) +
            extendsOrIdentified + classBody)> {};

  struct ConceptDeclarationRule
      : rule<create<ast::ConceptDeclaration>(
            classModifiers + kw<"concept"> +
            assign<&ast::Declaration::name>(ref<IDENTIFIER>) +
            option(kw<"extends"> +
                   assign<&ast::ClassDeclaration::extends>(ref<IDENTIFIER>)) +
            classBody)> {};

  struct ParticipantDeclarationRule
      : rule<create<ast::ParticipantDeclaration>(
            classModifiers + ref<PARTICIPANT> +
            assign<&ast::Declaration::name>(ref<IDENTIFIER>) +
            extendsOrIdentified + classBody)> {};

  struct TransactionDeclarationRule
      : rule<create<ast::TransactionDeclaration>(
            classModifiers + kw<"transaction"> +
            assign<&ast::Declaration::name>(ref<IDENTIFIER>) + classBody)> {};

  struct EventDeclarationRule
      : rule<create<ast::EventDeclaration>(
            kw<"event"> + assign<&ast::Declaration::name>(ref<IDENTIFIER>) +
            classBody)> {};

  struct EnumConstantRule
      : rule<kw<"o"> + assign<&ast::EnumConstant::name>(ref<IDENTIFIER>)> {};

  struct EnumDeclarationRule
      : rule<create<ast::EnumDeclaration>(
            kw<"enum"> + assign<&ast::Declaration::name>(ref<IDENTIFIER>) +
            lit<"{"> +
            many(append<&ast::EnumDeclaration::constants>(ref<EnumConstantRule>)) +
            lit<"}">)> {};

  struct ImportRule
      : rule<kw<"import"> +
             assign<&ast::Import::path>(ref<QualifiedNameRule>) +
             option(lit<"."> +
                    flag<&ast::Import::wildcard>(lit<"*">))> {};

  struct ModelRule
      : rule<create<ast::Model>(
            kw<"namespace"> +
            assign<&ast::Model::namespaceName>(ref<QualifiedNameRule>) +
            many(append<&ast::Model::imports>(ref<ImportRule>)) +
            many(append<&ast::Model::declarations>(
                ref<AssetDeclarationRule> | ref<ConceptDeclarationRule> |
                ref<EnumDeclarationRule> | ref<ParticipantDeclarationRule> |
                ref<TransactionDeclarationRule> | ref<EventDeclarationRule>)))> {
  };

  using entry = ModelRule;
  using ignore = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace cto
