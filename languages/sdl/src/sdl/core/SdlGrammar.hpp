// The sdl language: its grammar.
#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <system_error>
#include <charconv>
#include <memory_resource>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "sdl/core/ast.hpp"

namespace sdl {

inline constexpr auto unescape = [](std::string_view t,
                                    std::pmr::memory_resource* mem) {
  const std::string_view body = t.substr(1, t.size() - 2);
  if (body.find('\\') == std::string_view::npos) return body;
  const std::span<char> room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    if (body[i] == '\\' && i + 1 < body.size() && body[i + 1] == '"') {
      room[n++] = '"';
      ++i;
    } else {
      room[n++] = body[i];
    }
  }
  return std::string_view(room.data(), n);
};
using Unescaped = cursiv::text_conv<unescape>;

inline constexpr auto hex_value = [](std::string_view t) {
  std::int64_t v = 0;
  const auto [p, ec] = std::from_chars(t.data() + 2, t.data() + t.size(), v, 16);
  if (ec != std::errc{} || p != t.data() + t.size())
    return cursiv::ConversionResult<std::int64_t>::err("hex literal out of range");
  return cursiv::ConversionResult<std::int64_t>::ok(v);
};
using HexValue = cursiv::text_conv<hex_value>;

inline constexpr auto octal_value = [](std::string_view t) {
  std::int64_t v = 0;
  const auto [p, ec] = std::from_chars(t.data() + 2, t.data() + t.size(), v, 8);
  if (ec != std::errc{} || p != t.data() + t.size())
    return cursiv::ConversionResult<std::int64_t>::err("octal literal out of range");
  return cursiv::ConversionResult<std::int64_t>::ok(v);
};
using OctalValue = cursiv::text_conv<octal_value>;

inline constexpr auto binary_value = [](std::string_view t) {
  std::int64_t v = 0;
  const auto [p, ec] = std::from_chars(t.data() + 2, t.data() + t.size(), v, 2);
  if (ec != std::errc{} || p != t.data() + t.size())
    return cursiv::ConversionResult<std::int64_t>::err("binary literal out of range");
  return cursiv::ConversionResult<std::int64_t>::ok(v);
};
using BinaryValue = cursiv::text_conv<binary_value>;

// '%A' followed by one letter: the ASCII code of that letter, upper case.
inline constexpr auto ascii_value = [](std::string_view t) {
  char c = t[2];
  if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
  return cursiv::ConversionResult<std::int64_t>::ok(
      static_cast<std::int64_t>(static_cast<unsigned char>(c)));
};
using AsciiValue = cursiv::text_conv<ascii_value>;

struct Sdl : cursiv::GrammarSpec {
  static constexpr auto word_boundary = cr<"a-zA-Z0-9$_">;

  struct WS : terminal<some(cr<R"( \t\r\n\f)">)> {};
  // A local comment, unconditionally dropped: `{ ... \n`.
  struct LOCAL_COMMENT : terminal<lit<"{"> + many(cr<R"(^\r\n)">)> {};
  // Kept as hidden trivia, so a comment is admitted wherever trivia is
  // read, not only at the specific positions a statement list would name.
  struct COMMENT : terminal<lit<"/*"> + many(cr<R"(^\r\n)">)> {};

  // At least one non-digit somewhere disjoins NAME from INTEGER_NUMBER: a
  // pure digit run must always read as the number, since the two
  // terminals would otherwise tie at equal length.
  struct NAME : terminal<many(cr<"0-9">) + cr<"a-zA-Z$_"> + many(cr<"a-zA-Z0-9$_">)> {};
  struct LOCAL_NAME : terminal<lit<"#"> + some(cr<"a-zA-Z0-9$_">)> {};
  struct STRING_LITERAL
      : terminal<lit<"\""> + many(lit<"\\\""> | cr<R"(^")">) + lit<"\"">, Unescaped> {};

  struct INTEGER_NUMBER : terminal<some(cr<"0-9">), std::int64_t> {};
  struct HEX_NUMBER : terminal<lit<"%"> + cr<"xX"> + some(cr<"0-9a-fA-F">), HexValue> {};
  struct OCTAL_NUMBER : terminal<lit<"%"> + cr<"oO"> + some(cr<"0-7">), OctalValue> {};
  struct BINARY_NUMBER : terminal<lit<"%"> + cr<"bB"> + some(cr<"01">), BinaryValue> {};
  struct ASCII_VALUE : terminal<lit<"%"> + cr<"aA"> + cr<"a-zA-Z">, AsciiValue> {};

  static constexpr auto NameStr = ref<NAME> | ref<STRING_LITERAL>;

  // Forward declarations: the grammar below is mutually recursive
  // (expressions inside dimensions, aggregates nesting aggregates, symbol
  // conditionals nesting statements).
  struct PrimitiveTypeRule;
  struct NamedTypeRule;
  struct ExpressionRule;
  struct DimExprRule;
  struct BaseAlignRule;
  struct ParamRule;
  struct ModuleStmtRule;
  struct AggrBodyItemRule;

  static constexpr auto DataOrUserType = ref<PrimitiveTypeRule> | ref<NamedTypeRule>;

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
             assign<&ast::PrimitiveType::length>(ref<ExpressionRule>) +
             many(VieldFlags));
  static constexpr auto CharVarying =
      flag<&ast::PrimitiveType::varying_>(ikw<"VARYING">);
  static constexpr auto CharOptions =
      many(CharVarying) +
      option(ikw<"LENGTH"> +
             (flag<&ast::PrimitiveType::lengthStar>(lit<"*">) |
              assign<&ast::PrimitiveType::length>(ref<ExpressionRule>)) +
             many(CharVarying));

  static constexpr auto EntryOptionsBody =
      many((ikw<"PARAMETER"> + lit<"("> +
            some(append<&ast::EntryOpts::params>(ref<ParamRule>), lit<",">) +
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
                   assign<&ast::ObjectSpec::dimension>(ref<DimExprRule>))) |
           (ikw<"ENTRY"> + assign<&ast::ObjectSpec::entry>(
                               create<ast::EntryOpts>(EntryOptionsBody)))) +
          option(assign<&ast::ObjectSpec::basealign>(ref<BaseAlignRule>)))) +
      lit<")">);

  // --- Expressions ---

  struct OpMul : terminal<lit<"*">, value<ast::BinOp::multiply>> {};
  struct OpDiv : terminal<lit<"/">, value<ast::BinOp::divide>> {};
  struct OpAdd : terminal<lit<"+">, value<ast::BinOp::add>> {};
  struct OpSub : terminal<lit<"-">, value<ast::BinOp::subtract>> {};
  struct OpAt : terminal<lit<"@">, value<ast::BinOp::at>> {};
  struct OpBang : terminal<lit<"!">, value<ast::BinOp::bang>> {};
  struct OpAmp : terminal<lit<"&">, value<ast::BinOp::amp>> {};

  static constexpr auto MulLvl = ref<OpMul> | ref<OpDiv>;
  static constexpr auto AddLvl = ref<OpAdd> | ref<OpSub>;
  static constexpr auto AtLvl = ref<OpAt>;
  static constexpr auto BangLvl = ref<OpBang>;
  static constexpr auto AmpLvl = ref<OpAmp>;

  struct NumberLitRule
      : rule<assign<&ast::NumberLit::value>(
            ref<INTEGER_NUMBER> | ref<HEX_NUMBER> | ref<OCTAL_NUMBER> |
            ref<BINARY_NUMBER> | ref<ASCII_VALUE>)> {};
  struct LocalRefRule : rule<assign<&ast::LocalRef::name>(ref<LOCAL_NAME>)> {};
  struct NameRefRule : rule<assign<&ast::NameRef::name>(ref<NAME>)> {};
  struct StringRefRule : rule<assign<&ast::StringRef::text>(ref<STRING_LITERAL>)> {};
  struct LocationRefRule
      : rule<assign<&ast::LocationRef::symbol>(lit<"."> | lit<"^"> | lit<":">)> {};

  struct PrimaryRule
      : rule<ref<NumberLitRule> | ref<LocalRefRule> | ref<NameRefRule> |
             ref<StringRefRule> | ref<LocationRefRule> |
             (lit<"("> + ref<ExpressionRule> + lit<")">)> {};

  struct UnaryRule
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-">) +
                 assign<&ast::UnaryExpr::operand>(ref<PrimaryRule>)) |
             ref<PrimaryRule>> {};

  struct ExpressionRule
      : infix<UnaryRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(MulLvl) >> left(AddLvl) >> left(AtLvl) >> left(BangLvl) >>
               left(AmpLvl))> {};

  struct DimExprRule
      : rule<create<ast::DimSpec>(
            flag<&ast::DimSpec::star>(lit<"*">) |
            (option(assign<&ast::DimSpec::lo>(ref<ExpressionRule>) + lit<":">) +
             assign<&ast::DimSpec::hi>(ref<ExpressionRule>)))> {};

  struct BaseAlignRule
      : rule<create<ast::BaseAlign>(
            ikw<"BASEALIGN"> +
            (assign<&ast::BaseAlign::type>(DataOrUserType) |
             (lit<"("> + assign<&ast::BaseAlign::size>(ref<ExpressionRule>) +
              lit<")">)))> {};

  struct ParamRule
      : rule<create<ast::Param>(
            (flag<&ast::Param::any_>(ikw<"ANY">) |
             assign<&ast::Param::type>(DataOrUserType)) +
            many(flag<&ast::Param::value_>(ikw<"VALUE">) |
                 flag<&ast::Param::descriptor_>(ikw<"DESCRIPTOR">) |
                 flag<&ast::Param::rtlStrDesc_>(ikw<"RTL_STR_DESC">) |
                 (ikw<"DIMENSION"> +
                  append<&ast::Param::dimensions>(ref<DimExprRule>)) |
                 flag<&ast::Param::in_>(ikw<"IN">) |
                 flag<&ast::Param::out_>(ikw<"OUT">) |
                 flag<&ast::Param::list_>(ikw<"LIST">) |
                 (ikw<"NAMED"> + append<&ast::Param::named>(NameStr)) |
                 (ikw<"DEFAULT"> +
                  append<&ast::Param::defaults>(ref<ExpressionRule>)) |
                 flag<&ast::Param::optional_>(ikw<"OPTIONAL">) |
                 flag<&ast::Param::reference_>(ikw<"REFERENCE">) |
                 (ikw<"TYPENAME"> +
                  append<&ast::Param::typeNames>(NameStr))))> {};

  // --- Types ---

  struct PrimitiveTypeRule
      : rule<create<ast::PrimitiveType>(
            (assign<&ast::PrimitiveType::keyword>(ikw<"BYTE">) + SignOption) |
            (assign<&ast::PrimitiveType::keyword>(ikw<"WORD">) + SignOption) |
            (assign<&ast::PrimitiveType::keyword>(ikw<"LONGWORD">) + SignOption) |
            (assign<&ast::PrimitiveType::keyword>(ikw<"QUADWORD">) + SignOption) |
            (assign<&ast::PrimitiveType::keyword>(ikw<"OCTAWORD">) + SignOption) |
            (assign<&ast::PrimitiveType::keyword>(ikw<"F_FLOATING">) +
             ComplexOption) |
            (assign<&ast::PrimitiveType::keyword>(ikw<"D_FLOATING">) +
             ComplexOption) |
            (assign<&ast::PrimitiveType::keyword>(ikw<"G_FLOATING">) +
             ComplexOption) |
            (assign<&ast::PrimitiveType::keyword>(ikw<"H_FLOATING">) +
             ComplexOption) |
            (assign<&ast::PrimitiveType::keyword>(ikw<"DECIMAL">) +
             ikw<"PRECISION"> + lit<"("> +
             assign<&ast::PrimitiveType::precision>(ref<ExpressionRule>) +
             lit<","> +
             assign<&ast::PrimitiveType::scale>(ref<ExpressionRule>) +
             lit<")">) |
            (assign<&ast::PrimitiveType::keyword>(ikw<"BITFIELD">) +
             VieldOptions) |
            (assign<&ast::PrimitiveType::keyword>(ikw<"CHARACTER">) +
             CharOptions) |
            assign<&ast::PrimitiveType::keyword>(ikw<"BOOLEAN">) |
            (assign<&ast::PrimitiveType::keyword>(ikw<"ADDRESS">) + ObjectOption) |
            (assign<&ast::PrimitiveType::keyword>(ikw<"POINTER">) + ObjectOption) |
            (assign<&ast::PrimitiveType::keyword>(ikw<"POINTER_LONG">) +
             ObjectOption) |
            (assign<&ast::PrimitiveType::keyword>(ikw<"POINTER_QUAD">) +
             ObjectOption) |
            (assign<&ast::PrimitiveType::keyword>(ikw<"POINTER_HW">) +
             ObjectOption) |
            (assign<&ast::PrimitiveType::keyword>(ikw<"INTEGER_BYTE">) +
             SignOption) |
            (assign<&ast::PrimitiveType::keyword>(ikw<"INTEGER_WORD">) +
             SignOption) |
            (assign<&ast::PrimitiveType::keyword>(ikw<"INTEGER_LONG">) +
             SignOption) |
            (assign<&ast::PrimitiveType::keyword>(ikw<"INTEGER_QUAD">) +
             SignOption) |
            (assign<&ast::PrimitiveType::keyword>(ikw<"INTEGER_HW">) +
             SignOption) |
            (assign<&ast::PrimitiveType::keyword>(ikw<"INTEGER">) + SignOption) |
            (assign<&ast::PrimitiveType::keyword>(ikw<"HARDWARE_ADDRESS">) +
             ObjectOption) |
            (assign<&ast::PrimitiveType::keyword>(ikw<"HARDWARE_INTEGER">) +
             SignOption))> {};

  struct NamedTypeRule
      : rule<create<ast::NamedType>(
            assign<&ast::NamedType::name>(NameStr) +
            option(ikw<"SIZEOF"> +
                   (assign<&ast::NamedType::sizeofType>(DataOrUserType) |
                    (lit<"("> +
                     assign<&ast::NamedType::sizeofExpr>(ref<ExpressionRule>) +
                     lit<")">))))> {};

  // --- Statements ---

  struct LocalAssignmentRule
      : rule<create<ast::LocalAssignment>(
            assign<&ast::LocalAssignment::name>(ref<LOCAL_NAME>) + lit<"="> +
            assign<&ast::LocalAssignment::value>(ref<ExpressionRule>) +
            lit<";">)> {};

  struct ConstantPhraseRule
      : rule<create<ast::ConstantPhrase>(
            (append<&ast::ConstantPhrase::names>(NameStr) |
             (lit<"("> + many(lit<",">) +
              some(append<&ast::ConstantPhrase::names>(NameStr),
                   some(lit<",">)) +
              many(lit<",">) + lit<")">)) +
            ikw<"EQUALS"> +
            ((assign<&ast::ConstantPhrase::value>(ref<ExpressionRule>) +
              many((ikw<"INCREMENT"> +
                    append<&ast::ConstantPhrase::increments>(
                        ref<ExpressionRule>)) |
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

  struct ConstantStmtRule
      : rule<create<ast::ConstantStmt>(
            ikw<"CONSTANT"> +
            some(append<&ast::ConstantStmt::phrases>(ref<ConstantPhraseRule>),
                 lit<",">) +
            lit<";">)> {};

  struct EntryDeclRule
      : rule<create<ast::EntryDecl>(
            ikw<"ENTRY"> + assign<&ast::EntryDecl::name>(NameStr) +
            assign<&ast::EntryDecl::opts>(create<ast::EntryOpts>(EntryOptionsBody)) +
            lit<";">)> {};

  struct ItemDeclRule
      : rule<create<ast::ItemDecl>(
            ikw<"ITEM"> + assign<&ast::ItemDecl::name>(NameStr) +
            assign<&ast::ItemDecl::type>(DataOrUserType) +
            many(flag<&ast::ItemDecl::common_>(ikw<"COMMON">) |
                 flag<&ast::ItemDecl::global_>(ikw<"GLOBAL">) |
                 flag<&ast::ItemDecl::typedef_>(ikw<"TYPEDEF">) |
                 (ikw<"PREFIX"> + append<&ast::ItemDecl::prefixes>(NameStr)) |
                 (ikw<"TAG"> + append<&ast::ItemDecl::tags>(NameStr)) |
                 (ikw<"DIMENSION"> +
                  append<&ast::ItemDecl::dimensions>(ref<DimExprRule>)) |
                 append<&ast::ItemDecl::basealigns>(ref<BaseAlignRule>)) +
            lit<";">)> {};

  static constexpr auto DeclarePrefix =
      ikw<"PREFIX"> + assign<&ast::DeclareStmt::prefix>(NameStr);
  static constexpr auto DeclareTag =
      ikw<"TAG"> + assign<&ast::DeclareStmt::tag>(NameStr);

  struct DeclareStmtRule
      : rule<create<ast::DeclareStmt>(
            ikw<"DECLARE"> + assign<&ast::DeclareStmt::name>(NameStr) +
            ikw<"SIZEOF"> +
            (assign<&ast::DeclareStmt::sizeofType>(DataOrUserType) |
             (lit<"("> +
              assign<&ast::DeclareStmt::sizeofExpr>(ref<ExpressionRule>) +
              lit<")">)) +
            option((DeclarePrefix + option(DeclareTag)) |
                   (DeclareTag + option(DeclarePrefix))))> {};

  struct AggregateMemberRule
      : rule<(assign<&ast::Member::name>(NameStr) +
              assign<&ast::Member::type>(DataOrUserType) +
              many((ikw<"PREFIX"> + append<&ast::Member::prefixes>(NameStr)) |
                   (ikw<"TAG"> + append<&ast::Member::tags>(NameStr)) |
                   flag<&ast::Member::fill_>(ikw<"FILL">) |
                   (ikw<"DIMENSION"> +
                    append<&ast::Member::dimensions>(ref<DimExprRule>)) |
                   flag<&ast::Member::align_>(ikw<"ALIGN">) |
                   flag<&ast::Member::noalign_>(ikw<"NOALIGN">) |
                   append<&ast::Member::basealigns>(ref<BaseAlignRule>)) +
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
                        ref<DimExprRule>)) |
                   flag<&ast::NestedAggregate::align_>(ikw<"ALIGN">) |
                   flag<&ast::NestedAggregate::noalign_>(ikw<"NOALIGN">) |
                   append<&ast::NestedAggregate::basealigns>(
                       ref<BaseAlignRule>)) +
              lit<";"> +
              many(append<&ast::NestedAggregate::body>(ref<AggrBodyItemRule>) |
                   lit<";">) +
              ikw<"END"> +
              option(assign<&ast::NestedAggregate::endName>(NameStr)) +
              lit<";">)> {};

  struct IfSymbolClauseRule
      : rule<create<ast::IfSymbolStmt>(
            ikw<"IFSYMBOL"> + assign<&ast::IfSymbolStmt::symbol>(NameStr) +
            lit<";"> +
            many(append<&ast::IfSymbolStmt::thenBody>(ref<AggrBodyItemRule>) |
                 lit<";">) +
            many(append<&ast::IfSymbolStmt::elseIfs>(create<ast::ElseIfBranch>(
                ikw<"ELSE_IFSYMBOL"> +
                assign<&ast::ElseIfBranch::symbol>(NameStr) + lit<";"> +
                many(append<&ast::ElseIfBranch::body>(ref<AggrBodyItemRule>) |
                     lit<";">)))) +
            option(flag<&ast::IfSymbolStmt::hasElse>(ikw<"ELSE">) +
                   lit<";"> +
                   many(append<&ast::IfSymbolStmt::elseBody>(
                            ref<AggrBodyItemRule>) |
                        lit<";">)) +
            ikw<"END_IFSYMBOL"> + lit<";">)> {};

  struct AggrBodyItemRule
      : rule<ref<AggregateMemberRule> | ref<LocalAssignmentRule> |
             ref<ConstantStmtRule> | ref<IfSymbolClauseRule>> {};

  struct AggregateStmtRule
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
                 append<&ast::AggregateStmt::basealigns>(ref<BaseAlignRule>) |
                 (ikw<"PREFIX"> +
                  append<&ast::AggregateStmt::prefixes>(NameStr)) |
                 (ikw<"TAG"> + append<&ast::AggregateStmt::tags>(NameStr)) |
                 (ikw<"DIMENSION"> +
                  append<&ast::AggregateStmt::dimensions>(ref<DimExprRule>)) |
                 (ikw<"ORIGIN"> +
                  append<&ast::AggregateStmt::origins>(NameStr)) |
                 (flag<&ast::AggregateStmt::based_>(ikw<"BASED">) +
                  option(append<&ast::AggregateStmt::basedNames>(NameStr))) |
                 (ikw<"MARKER"> +
                  append<&ast::AggregateStmt::markers>(NameStr)) |
                 flag<&ast::AggregateStmt::fill_>(ikw<"FILL">)) +
            lit<";"> +
            many(append<&ast::AggregateStmt::body>(ref<AggrBodyItemRule>) |
                 lit<";">) +
            ikw<"END"> + option(assign<&ast::AggregateStmt::endName>(NameStr)) +
            lit<";">)> {};

  struct ConditionalStmtRule
      : rule<create<ast::ConditionalStmt>(
            ikw<"IFLANGUAGE"> +
            some(append<&ast::ConditionalStmt::languages>(NameStr)) +
            many(append<&ast::ConditionalStmt::body>(ref<ModuleStmtRule>) |
                 lit<";">) +
            ikw<"END_IFLANGUAGE"> +
            many(append<&ast::ConditionalStmt::endLanguages>(NameStr)))> {};

  struct IfSymbolStmtRule
      : rule<create<ast::IfSymbolStmt>(
            ikw<"IFSYMBOL"> + assign<&ast::IfSymbolStmt::symbol>(NameStr) +
            lit<";"> +
            many(append<&ast::IfSymbolStmt::thenBody>(ref<ModuleStmtRule>) |
                 lit<";">) +
            many(append<&ast::IfSymbolStmt::elseIfs>(create<ast::ElseIfBranch>(
                ikw<"ELSE_IFSYMBOL"> +
                assign<&ast::ElseIfBranch::symbol>(NameStr) + lit<";"> +
                many(append<&ast::ElseIfBranch::body>(ref<ModuleStmtRule>) |
                     lit<";">)))) +
            option(flag<&ast::IfSymbolStmt::hasElse>(ikw<"ELSE">) +
                   lit<";"> +
                   many(append<&ast::IfSymbolStmt::elseBody>(
                            ref<ModuleStmtRule>) |
                        lit<";">)) +
            ikw<"END_IFSYMBOL"> + lit<";">)> {};

  struct LiteralStmtRule
      : rule<create<ast::LiteralStmt>(
            ikw<"LITERAL"> + lit<";"> +
            many(append<&ast::LiteralStmt::strings>(ref<STRING_LITERAL>)) +
            ikw<"END_LITERAL">)> {};

  struct ReadStmtRule
      : rule<create<ast::ReadStmt>(
            ikw<"READ"> + assign<&ast::ReadStmt::path>(ref<STRING_LITERAL>) +
            lit<";">)> {};

  struct IncludeStmtRule
      : rule<create<ast::IncludeStmt>(
            ikw<"INCLUDE"> +
            assign<&ast::IncludeStmt::path>(ref<STRING_LITERAL>) + lit<";">)> {
  };

  struct ModuleStmtRule
      : rule<ref<LocalAssignmentRule> | ref<ConstantStmtRule> |
             ref<EntryDeclRule> | ref<AggregateStmtRule> | ref<ItemDeclRule> |
             ref<DeclareStmtRule> | ref<IfSymbolStmtRule> |
             ref<ConditionalStmtRule> | ref<LiteralStmtRule> |
             ref<ReadStmtRule> | ref<IncludeStmtRule>> {};

  struct ModuleRule
      : rule<create<ast::Module>(
            ikw<"MODULE"> + assign<&ast::Module::name>(NameStr) +
            option(ikw<"IDENT"> + assign<&ast::Module::ident>(NameStr)) +
            lit<";"> +
            many(append<&ast::Module::body>(ref<ModuleStmtRule>) | lit<";">) +
            ikw<"END_MODULE"> + option(assign<&ast::Module::endName>(NameStr)) +
            lit<";">)> {};

  struct ProgramRule
      : rule<create<ast::Program>(
            some(append<&ast::Program::items>(ref<ModuleRule>) |
                 append<&ast::Program::items>(ref<LocalAssignmentRule>)))> {};

  using entry = ProgramRule;
  using ignore = list<WS, LOCAL_COMMENT>;
  using hidden = list<COMMENT>;
};

}  // namespace sdl
