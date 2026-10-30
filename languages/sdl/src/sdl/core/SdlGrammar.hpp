// The sdl language: its grammar.
#pragma once

#include <cstdint>
#include <bit>
#include <span>
#include <string_view>
#include <system_error>
#include <charconv>
#include <memory_resource>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "sdl/core/ast.hpp"

namespace sdl {

// Native SDL doubles an embedded quote. Backslashes remain ordinary text.
inline constexpr auto unescape = [](std::string_view t,
                                    std::pmr::memory_resource* mem) {
  const auto body = t.substr(1, t.size() - 2);
  if (body.find_first_of("\"\r\n") == body.npos) return body;
  const auto room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    if (body[i] == '\r' || body[i] == '\n') continue;
    room[n++] = body[i];
    if (body[i] == '"' && i + 1 < body.size() && body[i + 1] == '"') ++i;
  }
  return std::string_view(room.data(), n);
};

inline constexpr auto decimal_value = [](std::string_view text) {
  std::int32_t value = 0;
  const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
  if (error != std::errc{} || end != text.data() + text.size())
    return cursiv::ConversionResult<std::int64_t>::err("decimal literal out of longword range");
  return cursiv::ConversionResult<std::int64_t>::ok(value);
};

inline constexpr auto hex_value = [](std::string_view t) {
  std::uint32_t v = 0;
  const auto [p, ec] = std::from_chars(t.data() + 2, t.data() + t.size(), v, 16);
  if (ec != std::errc{} || p != t.data() + t.size())
    return cursiv::ConversionResult<std::int64_t>::err("hex literal out of range");
  return cursiv::ConversionResult<std::int64_t>::ok(std::bit_cast<std::int32_t>(v));
};

inline constexpr auto octal_value = [](std::string_view t) {
  std::uint32_t v = 0;
  const auto [p, ec] = std::from_chars(t.data() + 2, t.data() + t.size(), v, 8);
  if (ec != std::errc{} || p != t.data() + t.size())
    return cursiv::ConversionResult<std::int64_t>::err("octal literal out of range");
  return cursiv::ConversionResult<std::int64_t>::ok(std::bit_cast<std::int32_t>(v));
};

inline constexpr auto binary_value = [](std::string_view t) {
  std::uint32_t v = 0;
  const auto [p, ec] = std::from_chars(t.data() + 2, t.data() + t.size(), v, 2);
  if (ec != std::errc{} || p != t.data() + t.size())
    return cursiv::ConversionResult<std::int64_t>::err("binary literal out of range");
  return cursiv::ConversionResult<std::int64_t>::ok(std::bit_cast<std::int32_t>(v));
};

// The lexer takes the following character verbatim; PUSHTERM uses rank().
inline constexpr auto ascii_value = [](std::string_view t) {
  return cursiv::ConversionResult<std::int64_t>::ok(
      static_cast<std::int64_t>(static_cast<unsigned char>(t[2])));
};

struct Sdl : cursiv::GrammarSpec {
  static constexpr auto word_boundary = cr<"a-zA-Z0-9$_">;

  struct WS : terminal<some(cr<R"( \t\r\n\f)">)> {};
  // A local comment, unconditionally dropped: `{ ... \n`.
  struct LOCAL_COMMENT : terminal<lit<"{"> + many(cr<R"(^\r\n)">)> {};
  // Kept as hidden trivia, so a comment is admitted wherever trivia is
  // read, not only at the specific positions a statement list would name.
  struct COMMENT : terminal<lit<"/*"> + many(cr<R"(^\r\n)">)> {};

  static constexpr auto Reserved =
      ikw<"AGGREGATE"> | ikw<"CONSTANT"> | ikw<"END"> | ikw<"END_MODULE"> | ikw<"ENTRY"> |
      ikw<"ITEM"> | ikw<"MODULE"> | ikw<"EQUALS"> | ikw<"STRING"> | ikw<"COMMON"> |
      ikw<"DIMENSION"> | ikw<"PREFIX"> | ikw<"TAG"> | ikw<"INCREMENT"> | ikw<"PARAMETER"> |
      ikw<"RETURNS"> | ikw<"VALUE"> | ikw<"GLOBAL"> | ikw<"VARIABLE"> | ikw<"ADDRESS"> |
      ikw<"BOOLEAN"> | ikw<"BYTE"> | ikw<"CHARACTER"> | ikw<"DECIMAL"> | ikw<"D_FLOATING"> |
      ikw<"F_FLOATING"> | ikw<"G_FLOATING"> | ikw<"H_FLOATING"> | ikw<"LONGWORD"> | ikw<"OCTAWORD"> |
      ikw<"QUADWORD"> | ikw<"BITFIELD"> | ikw<"WORD"> | ikw<"STRUCTURE"> | ikw<"UNION"> |
      ikw<"ANY"> | ikw<"LENGTH"> | ikw<"MASK"> | ikw<"PRECISION"> | ikw<"UNSIGNED"> |
      ikw<"VARYING"> | ikw<"ORIGIN"> | ikw<"DESCRIPTOR"> | ikw<"RTL_STR_DESC"> | ikw<"COMPLEX"> |
      ikw<"COUNTER"> | ikw<"IN"> | ikw<"OUT"> | ikw<"LIST"> | ikw<"NAMED"> |
      ikw<"IDENT"> | ikw<"BASED"> | ikw<"FILL"> | ikw<"ALIAS"> | ikw<"DEFAULT"> |
      ikw<"REFERENCE"> | ikw<"LINKAGE"> | ikw<"INCLUDE"> | ikw<"OPTIONAL"> | ikw<"SIGNED"> |
      ikw<"MARKER"> | ikw<"TYPENAME"> | ikw<"TYPEDEF"> | ikw<"VOID"> | ikw<"SIZEOF"> |
      ikw<"DECLARE"> | ikw<"IFLANGUAGE"> | ikw<"END_IFLANGUAGE"> | ikw<"LITERAL"> | ikw<"END_LITERAL"> |
      ikw<"READ"> | ikw<"INTEGER"> | ikw<"IFSYMBOL"> | ikw<"END_IFSYMBOL"> | ikw<"HARDWARE_ADDRESS"> |
      ikw<"HARDWARE_INTEGER"> | ikw<"POINTER_HW"> | ikw<"POINTER_LONG"> | ikw<"POINTER"> | ikw<"POINTER_QUAD"> |
      ikw<"INTEGER_BYTE"> | ikw<"INTEGER_WORD"> | ikw<"INTEGER_LONG"> | ikw<"INTEGER_QUAD"> | ikw<"INTEGER_HW"> |
      ikw<"ALIGN"> | ikw<"NOALIGN"> | ikw<"BASEALIGN"> | ikw<"ELSE"> | ikw<"ELSE_IFSYMBOL"> |
      ikw<"ELSE_IFLANGUAGE">;
  struct NAME : terminal<!Reserved + cr<"a-zA-Z$_"> + many(cr<"a-zA-Z0-9$_">)> {};
  struct LOCAL_NAME : terminal<lit<"#"> + some(cr<"a-zA-Z0-9$_">)> {};
  struct STRING_LITERAL
      : terminal<as<unescape>(lit<"\""> + many(lit<"\"\""> | cr<R"(^")">) + lit<"\"">)> {};

  struct INTEGER_NUMBER : terminal<as<decimal_value>(some(cr<"0-9">))> {};
  struct HEX_NUMBER
      : terminal<as<hex_value>(lit<"%"> + cr<"xX"> + some(cr<"0-9a-fA-F">))> {};
  struct OCTAL_NUMBER
      : terminal<as<octal_value>(lit<"%"> + cr<"oO"> + some(cr<"0-7">))> {};
  struct BINARY_NUMBER
      : terminal<as<binary_value>(lit<"%"> + cr<"bB"> + some(cr<"01">))> {};
  struct ASCII_VALUE
      : terminal<as<ascii_value>(lit<"%"> + cr<"aA"> + cr<R"(\x00-\x7f)">)> {};

  static constexpr auto NameStr = ref<NAME> | ref<STRING_LITERAL>;

  // Forward declarations: the grammar below is mutually recursive
  // (expressions inside dimensions, aggregates nesting aggregates, symbol
  // conditionals nesting statements).
  struct PrimitiveType;
  struct NamedType;
  struct Expression;
  struct DimExpr;
  struct BaseAlign;
  struct Param;
  struct ModuleStmt;
  struct AggrBodyItem;

  static constexpr auto DataOrUserType = ref<PrimitiveType> | ref<NamedType>;

  static constexpr auto SignOption =
      option(flag<&ast::PrimitiveType::signed_>(ikw<"SIGNED">) |
             flag<&ast::PrimitiveType::unsigned_>(ikw<"UNSIGNED">));
  static constexpr auto ComplexOption =
      option(flag<&ast::PrimitiveType::complex_>(ikw<"COMPLEX">));
  static constexpr auto VieldFlags =
      flag<&ast::PrimitiveType::mask_>(ikw<"MASK">) |
      flag<&ast::PrimitiveType::signed_>(ikw<"SIGNED">) |
      flag<&ast::PrimitiveType::unsigned_>(ikw<"UNSIGNED">);
  static constexpr auto VieldOptions =
      many(VieldFlags) +
      option(ikw<"LENGTH"> +
             assign<&ast::PrimitiveType::length>(ref<Expression>) +
             many(VieldFlags));
  static constexpr auto CharVarying =
      flag<&ast::PrimitiveType::varying_>(ikw<"VARYING">);
  static constexpr auto CharOptions =
      many(CharVarying) +
      option(ikw<"LENGTH"> +
             (flag<&ast::PrimitiveType::lengthStar>(lit<"*">) |
              assign<&ast::PrimitiveType::length>(ref<Expression>)) +
             many(CharVarying));

  static constexpr auto EntryOptionsBody =
      many((ikw<"PARAMETER"> + lit<"("> +
            some(append<&ast::EntryOpts::params>(ref<Param>), lit<",">) +
            lit<")">) |
           (ikw<"RETURNS"> +
            (flag<&ast::EntryOpts::returnVoid>(ikw<"VOID">) |
             append<&ast::EntryOpts::returnTypes>(DataOrUserType)) +
            option(ikw<"NAMED"> +
                   append<&ast::EntryOpts::returnNames>(NameStr))) |
           flag<&ast::EntryOpts::variable_>(ikw<"VARIABLE">) |
           (ikw<"TYPENAME"> + append<&ast::EntryOpts::typeNames>(NameStr)) |
           (ikw<"ALIAS"> + append<&ast::EntryOpts::aliases>(NameStr)) |
           (ikw<"LINKAGE"> + append<&ast::EntryOpts::linkages>(NameStr)));

  static constexpr auto ObjectOption = option(
      lit<"("> +
      assign<&ast::PrimitiveType::object>(create<ast::ObjectSpec>(
          ((assign<&ast::ObjectSpec::type>(DataOrUserType) +
            option(ikw<"DIMENSION"> +
                   assign<&ast::ObjectSpec::dimension>(ref<DimExpr>))) |
           (ikw<"ENTRY"> + assign<&ast::ObjectSpec::entry>(
                               create<ast::EntryOpts>(EntryOptionsBody)))) +
          option(assign<&ast::ObjectSpec::basealign>(ref<BaseAlign>)))) +
      lit<")">);

  // --- Expressions ---


  static constexpr auto MulLvl = as<ast::BinOp::multiply>(lit<"*">) | as<ast::BinOp::divide>(lit<"/">);
  static constexpr auto AddLvl = as<ast::BinOp::add>(lit<"+">) | as<ast::BinOp::subtract>(lit<"-">);
  static constexpr auto AtLvl = as<ast::BinOp::at>(lit<"@">);
  static constexpr auto BangLvl = as<ast::BinOp::bang>(lit<"!">);
  static constexpr auto AmpLvl = as<ast::BinOp::amp>(lit<"&">);

  struct NumberLit
      : rule<assign<&ast::NumberLit::value>(
            ref<INTEGER_NUMBER> | ref<HEX_NUMBER> | ref<OCTAL_NUMBER> |
            ref<BINARY_NUMBER> | ref<ASCII_VALUE>)> {};
  struct LocalRef : rule<assign<&ast::LocalRef::name>(ref<LOCAL_NAME>)> {};
  struct NameRef : rule<assign<&ast::NameRef::name>(ref<NAME>)> {};
  struct StringRef : rule<assign<&ast::StringRef::text>(ref<STRING_LITERAL>)> {};
  struct LocationRef
      : rule<assign<&ast::LocationRef::symbol>(lit<"."> | lit<"^"> | lit<":">)> {};

  struct Primary
      : rule<ref<NumberLit> | ref<LocalRef> | ref<NameRef> |
             ref<StringRef> | ref<LocationRef> |
             (lit<"("> + ref<Expression> + lit<")">)> {};

  struct Unary
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-">) +
                 assign<&ast::UnaryExpr::operand>(ref<Primary>)) |
             ref<Primary>> {};

  struct Expression
      : infix<Unary, ast::BinaryExpr,
              levels(left(MulLvl), left(AddLvl), left(AtLvl), left(BangLvl),
                     left(AmpLvl))> {};

  struct DimExpr
      : rule<create<ast::DimSpec>(
            flag<&ast::DimSpec::star>(lit<"*">) |
            (option(assign<&ast::DimSpec::lo>(ref<Expression>) + lit<":">) +
             assign<&ast::DimSpec::hi>(ref<Expression>)))> {};

  struct BaseAlign
      : rule<create<ast::BaseAlign>(
            ikw<"BASEALIGN"> +
            (assign<&ast::BaseAlign::type>(DataOrUserType) |
             (lit<"("> + assign<&ast::BaseAlign::size>(ref<Expression>) +
              lit<")">)))> {};

  struct Param
      : rule<create<ast::Param>(
            (flag<&ast::Param::any_>(ikw<"ANY">) |
             assign<&ast::Param::type>(DataOrUserType)) +
            many(flag<&ast::Param::value_>(ikw<"VALUE">) |
                 flag<&ast::Param::descriptor_>(ikw<"DESCRIPTOR">) |
                 flag<&ast::Param::rtlStrDesc_>(ikw<"RTL_STR_DESC">) |
                 (ikw<"DIMENSION"> +
                  append<&ast::Param::dimensions>(ref<DimExpr>)) |
                 flag<&ast::Param::in_>(ikw<"IN">) |
                 flag<&ast::Param::out_>(ikw<"OUT">) |
                 flag<&ast::Param::list_>(ikw<"LIST">) |
                 (ikw<"NAMED"> + append<&ast::Param::named>(NameStr)) |
                 (ikw<"DEFAULT"> +
                  append<&ast::Param::defaults>(ref<Expression>)) |
                 flag<&ast::Param::optional_>(ikw<"OPTIONAL">) |
                 flag<&ast::Param::reference_>(ikw<"REFERENCE">) |
                 (ikw<"TYPENAME"> +
                  append<&ast::Param::typeNames>(NameStr))))> {};

  // --- Types ---

  static constexpr auto SignedKeyword =
      ikw<"BYTE"> | ikw<"WORD"> | ikw<"LONGWORD"> | ikw<"QUADWORD"> |
      ikw<"OCTAWORD"> | ikw<"INTEGER_BYTE"> | ikw<"INTEGER_WORD"> |
      ikw<"INTEGER_LONG"> | ikw<"INTEGER_QUAD"> | ikw<"INTEGER_HW"> |
      ikw<"INTEGER"> | ikw<"HARDWARE_INTEGER">;
  static constexpr auto FloatingKeyword =
      ikw<"F_FLOATING"> | ikw<"D_FLOATING"> | ikw<"G_FLOATING"> | ikw<"H_FLOATING">;
  static constexpr auto ObjectKeyword =
      ikw<"ADDRESS"> | ikw<"POINTER"> | ikw<"POINTER_LONG"> |
      ikw<"POINTER_QUAD"> | ikw<"POINTER_HW"> | ikw<"HARDWARE_ADDRESS">;
  struct PrimitiveType
      : rule<create<ast::PrimitiveType>(
            (assign<&ast::PrimitiveType::keyword>(SignedKeyword) + SignOption) |
            (assign<&ast::PrimitiveType::keyword>(FloatingKeyword) + ComplexOption) |
            (assign<&ast::PrimitiveType::keyword>(ObjectKeyword) + ObjectOption) |
            (assign<&ast::PrimitiveType::keyword>(ikw<"DECIMAL">) +
             ikw<"PRECISION"> + lit<"("> + assign<&ast::PrimitiveType::precision>(ref<Expression>) +
             lit<","> + assign<&ast::PrimitiveType::scale>(ref<Expression>) + lit<")">) |
            (assign<&ast::PrimitiveType::keyword>(ikw<"BITFIELD">) + VieldOptions) |
            (assign<&ast::PrimitiveType::keyword>(ikw<"CHARACTER">) + CharOptions) |
            assign<&ast::PrimitiveType::keyword>(ikw<"BOOLEAN">))> {};

  struct NamedType
      : rule<create<ast::NamedType>(
            assign<&ast::NamedType::name>(NameStr) +
            option(ikw<"SIZEOF"> +
                   (assign<&ast::NamedType::sizeofType>(DataOrUserType) |
                    (lit<"("> +
                     assign<&ast::NamedType::sizeofExpr>(ref<Expression>) +
                     lit<")">))))> {};

  // --- Statements ---

  struct LocalAssignment
      : rule<create<ast::LocalAssignment>(
            assign<&ast::LocalAssignment::name>(ref<LOCAL_NAME>) + lit<"="> +
            assign<&ast::LocalAssignment::value>(ref<Expression>) +
            lit<";">)> {};

  struct ConstantPhrase
      : rule<create<ast::ConstantPhrase>(
            (append<&ast::ConstantPhrase::names>(NameStr) |
             (lit<"("> + many(lit<",">) +
              some(append<&ast::ConstantPhrase::names>(NameStr),
                   some(lit<",">)) +
              many(lit<",">) + lit<")">)) +
            ikw<"EQUALS"> +
            ((assign<&ast::ConstantPhrase::value>(ref<Expression>) +
              many((ikw<"INCREMENT"> +
                    append<&ast::ConstantPhrase::increments>(
                        ref<Expression>)) |
                   (ikw<"PREFIX"> +
                    append<&ast::ConstantPhrase::prefixes>(NameStr)) |
                   (ikw<"TAG"> + append<&ast::ConstantPhrase::tags>(NameStr)) |
                   (ikw<"COUNTER"> +
                    append<&ast::ConstantPhrase::counters>(ref<LOCAL_NAME>)) |
                   (ikw<"TYPENAME"> +
                    append<&ast::ConstantPhrase::typeNames>(NameStr)))) |
             (flag<&ast::ConstantPhrase::isString>(ikw<"STRING">) +
              assign<&ast::ConstantPhrase::stringValue>(ref<STRING_LITERAL>) +
              many((ikw<"PREFIX"> +
                    append<&ast::ConstantPhrase::prefixes>(NameStr)) |
                   (ikw<"TAG"> +
                    append<&ast::ConstantPhrase::tags>(NameStr))))))> {};

  struct ConstantStmt
      : rule<create<ast::ConstantStmt>(
            ikw<"CONSTANT"> +
            some(append<&ast::ConstantStmt::phrases>(ref<ConstantPhrase>),
                 lit<",">) +
            lit<";">)> {};

  struct EntryDecl
      : rule<create<ast::EntryDecl>(
            ikw<"ENTRY"> + assign<&ast::EntryDecl::name>(NameStr) +
            assign<&ast::EntryDecl::opts>(create<ast::EntryOpts>(EntryOptionsBody)) +
            lit<";">)> {};

  struct ItemDecl
      : rule<create<ast::ItemDecl>(
            ikw<"ITEM"> + assign<&ast::ItemDecl::name>(NameStr) +
            assign<&ast::ItemDecl::type>(DataOrUserType) +
            many(flag<&ast::ItemDecl::common_>(ikw<"COMMON">) |
                 flag<&ast::ItemDecl::global_>(ikw<"GLOBAL">) |
                 flag<&ast::ItemDecl::typedef_>(ikw<"TYPEDEF">) |
                 (ikw<"PREFIX"> + append<&ast::ItemDecl::prefixes>(NameStr)) |
                 (ikw<"TAG"> + append<&ast::ItemDecl::tags>(NameStr)) |
                 (ikw<"DIMENSION"> +
                  append<&ast::ItemDecl::dimensions>(ref<DimExpr>)) |
                 append<&ast::ItemDecl::basealigns>(ref<BaseAlign>)) +
            lit<";">)> {};

  static constexpr auto DeclarePrefix =
      ikw<"PREFIX"> + assign<&ast::DeclareStmt::prefix>(NameStr);
  static constexpr auto DeclareTag =
      ikw<"TAG"> + assign<&ast::DeclareStmt::tag>(NameStr);

  struct DeclareStmt
      : rule<create<ast::DeclareStmt>(
            ikw<"DECLARE"> + assign<&ast::DeclareStmt::name>(NameStr) +
            ikw<"SIZEOF"> +
            (assign<&ast::DeclareStmt::sizeofType>(DataOrUserType) |
             (lit<"("> +
              assign<&ast::DeclareStmt::sizeofExpr>(ref<Expression>) +
              lit<")">)) +
            option((DeclarePrefix + option(DeclareTag)) |
                   (DeclareTag + option(DeclarePrefix))))> {};

  struct AggregateMember
      : rule<(assign<&ast::Member::name>(NameStr) +
              assign<&ast::Member::type>(DataOrUserType) +
              many((ikw<"PREFIX"> + append<&ast::Member::prefixes>(NameStr)) |
                   (ikw<"TAG"> + append<&ast::Member::tags>(NameStr)) |
                   flag<&ast::Member::fill_>(ikw<"FILL">) |
                   (ikw<"DIMENSION"> +
                    append<&ast::Member::dimensions>(ref<DimExpr>)) |
                   flag<&ast::Member::align_>(ikw<"ALIGN">) |
                   flag<&ast::Member::noalign_>(ikw<"NOALIGN">) |
                   append<&ast::Member::basealigns>(ref<BaseAlign>)) +
              lit<";">) |
             (assign<&ast::NestedAggregate::name>(NameStr) +
              assign<&ast::NestedAggregate::kind>(ikw<"STRUCTURE"> |
                                                   ikw<"UNION">) +
              option(assign<&ast::NestedAggregate::implType>(DataOrUserType)) +
              many((ikw<"PREFIX"> +
                    append<&ast::NestedAggregate::prefixes>(NameStr)) |
                   (ikw<"TAG"> + append<&ast::NestedAggregate::tags>(NameStr)) |
                   flag<&ast::NestedAggregate::fill_>(ikw<"FILL">) |
                   (ikw<"DIMENSION"> +
                    append<&ast::NestedAggregate::dimensions>(
                        ref<DimExpr>)) |
                   flag<&ast::NestedAggregate::align_>(ikw<"ALIGN">) |
                   flag<&ast::NestedAggregate::noalign_>(ikw<"NOALIGN">) |
                   append<&ast::NestedAggregate::basealigns>(
                       ref<BaseAlign>)) +
              lit<";"> +
              many(append<&ast::NestedAggregate::body>(ref<AggrBodyItem>) |
                   lit<";">) +
              ikw<"END"> +
              option(assign<&ast::NestedAggregate::endName>(NameStr)) +
              lit<";">)> {};

  struct IfSymbolClause
      : rule<create<ast::IfSymbolStmt>(
            ikw<"IFSYMBOL"> + assign<&ast::IfSymbolStmt::symbol>(NameStr) +
            lit<";"> +
            many(append<&ast::IfSymbolStmt::thenBody>(ref<AggrBodyItem>) |
                 lit<";">) +
            many(append<&ast::IfSymbolStmt::elseIfs>(create<ast::ElseIfBranch>(
                ikw<"ELSE_IFSYMBOL"> +
                assign<&ast::ElseIfBranch::symbol>(NameStr) + lit<";"> +
                many(append<&ast::ElseIfBranch::body>(ref<AggrBodyItem>) |
                     lit<";">)))) +
            option(flag<&ast::IfSymbolStmt::hasElse>(ikw<"ELSE">) +
                   lit<";"> +
                   many(append<&ast::IfSymbolStmt::elseBody>(
                            ref<AggrBodyItem>) |
                        lit<";">)) +
            ikw<"END_IFSYMBOL"> + lit<";">)> {};

  struct AggrBodyItem
      : rule<ref<AggregateMember> | ref<LocalAssignment> |
             ref<ConstantStmt> | ref<IfSymbolClause>> {};

  struct AggregateStmt
      : rule<create<ast::AggregateStmt>(
            ikw<"AGGREGATE"> + assign<&ast::AggregateStmt::name>(NameStr) +
            assign<&ast::AggregateStmt::kind>(ikw<"STRUCTURE"> |
                                               ikw<"UNION">) +
            option(assign<&ast::AggregateStmt::implType>(DataOrUserType)) +
            many(flag<&ast::AggregateStmt::common_>(ikw<"COMMON">) |
                 flag<&ast::AggregateStmt::global_>(ikw<"GLOBAL">) |
                 flag<&ast::AggregateStmt::typedef_>(ikw<"TYPEDEF">) |
                 flag<&ast::AggregateStmt::align_>(ikw<"ALIGN">) |
                 flag<&ast::AggregateStmt::noalign_>(ikw<"NOALIGN">) |
                 append<&ast::AggregateStmt::basealigns>(ref<BaseAlign>) |
                 (ikw<"PREFIX"> +
                  append<&ast::AggregateStmt::prefixes>(NameStr)) |
                 (ikw<"TAG"> + append<&ast::AggregateStmt::tags>(NameStr)) |
                 (ikw<"DIMENSION"> +
                  append<&ast::AggregateStmt::dimensions>(ref<DimExpr>)) |
                 (ikw<"ORIGIN"> +
                  append<&ast::AggregateStmt::origins>(NameStr)) |
                 (flag<&ast::AggregateStmt::based_>(ikw<"BASED">) +
                  option(append<&ast::AggregateStmt::basedNames>(NameStr))) |
                 (ikw<"MARKER"> +
                  append<&ast::AggregateStmt::markers>(NameStr)) |
                 flag<&ast::AggregateStmt::fill_>(ikw<"FILL">)) +
            lit<";"> +
            many(append<&ast::AggregateStmt::body>(ref<AggrBodyItem>) |
                 lit<";">) +
            ikw<"END"> + option(assign<&ast::AggregateStmt::endName>(NameStr)) +
            lit<";">)> {};

  struct ConditionalStmt
      : rule<create<ast::ConditionalStmt>(
            ikw<"IFLANGUAGE"> +
            some(append<&ast::ConditionalStmt::languages>(NameStr)) +
            many(append<&ast::ConditionalStmt::body>(ref<ModuleStmt>) |
                 lit<";">) +
            ikw<"END_IFLANGUAGE"> +
            many(append<&ast::ConditionalStmt::endLanguages>(NameStr)))> {};

  struct IfSymbolStmt
      : rule<create<ast::IfSymbolStmt>(
            ikw<"IFSYMBOL"> + assign<&ast::IfSymbolStmt::symbol>(NameStr) +
            lit<";"> +
            many(append<&ast::IfSymbolStmt::thenBody>(ref<ModuleStmt>) |
                 lit<";">) +
            many(append<&ast::IfSymbolStmt::elseIfs>(create<ast::ElseIfBranch>(
                ikw<"ELSE_IFSYMBOL"> +
                assign<&ast::ElseIfBranch::symbol>(NameStr) + lit<";"> +
                many(append<&ast::ElseIfBranch::body>(ref<ModuleStmt>) |
                     lit<";">)))) +
            option(flag<&ast::IfSymbolStmt::hasElse>(ikw<"ELSE">) +
                   lit<";"> +
                   many(append<&ast::IfSymbolStmt::elseBody>(
                            ref<ModuleStmt>) |
                        lit<";">)) +
            ikw<"END_IFSYMBOL"> + lit<";">)> {};

  // The native input reader switches to raw records in LITERAL mode;
  // apparent quotes, comments and SDL declarations are pass-through text.
  static constexpr auto EndLiteral = cursiv::Lit<"END_LITERAL", true>{};
  struct LITERAL_TEXT : terminal<some(!EndLiteral + any)> {};
  struct LiteralStmt
      : rule<create<ast::LiteralStmt>(
            ikw<"LITERAL"> + tight(lit<";"> +
                 option(assign<&ast::LiteralStmt::text>(ref<LITERAL_TEXT>)) +
                 ikw<"END_LITERAL">))> {};

  struct ReadStmt
      : rule<create<ast::ReadStmt>(
            ikw<"READ"> + assign<&ast::ReadStmt::path>(ref<STRING_LITERAL>) +
            lit<";">)> {};

  struct IncludeStmt
      : rule<create<ast::IncludeStmt>(
            ikw<"INCLUDE"> +
            assign<&ast::IncludeStmt::path>(ref<STRING_LITERAL>) + lit<";">)> {
  };

  struct ModuleStmt
      : rule<ref<LocalAssignment> | ref<ConstantStmt> |
             ref<EntryDecl> | ref<AggregateStmt> | ref<ItemDecl> |
             ref<DeclareStmt> | ref<IfSymbolStmt> |
             ref<ConditionalStmt> | ref<LiteralStmt> |
             ref<ReadStmt> | ref<IncludeStmt>> {};

  struct Module
      : rule<create<ast::Module>(
            ikw<"MODULE"> + assign<&ast::Module::name>(NameStr) +
            option(ikw<"IDENT"> + assign<&ast::Module::ident>(NameStr)) +
            lit<";"> +
            many(append<&ast::Module::body>(ref<ModuleStmt>) | lit<";">) +
            ikw<"END_MODULE"> + option(assign<&ast::Module::endName>(NameStr)) +
            lit<";">)> {};

  struct Program
      : rule<create<ast::Program>(
            some(append<&ast::Program::items>(ref<Module>) |
                 append<&ast::Program::items>(ref<LocalAssignment>)))> {};

  using entry = Program;
  using ignored = list<WS, LOCAL_COMMENT>;
  using hidden = list<COMMENT>;
};

}  // namespace sdl
