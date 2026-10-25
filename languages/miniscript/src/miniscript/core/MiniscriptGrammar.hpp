#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "miniscript/core/ast.hpp"

namespace miniscript {

inline constexpr auto strip_colon = [](std::string_view t) {
  return t.substr(0, t.size() - 1);
};
using Wrappers = cursiv::text_conv<strip_colon>;

struct Miniscript : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};

  // Every keyword also names an opaque key (reservedKeyWord): each is a
  // plain text terminal, referenced both as a tag below and directly as a
  // key spelling.
  struct PK : terminal<kw<"pk">> {};
  struct PKH : terminal<kw<"pkh">> {};
  struct PK_K : terminal<kw<"pk_k">> {};
  struct PK_H : terminal<kw<"pk_h">> {};
  struct OLDER : terminal<kw<"older">> {};
  struct AFTER : terminal<kw<"after">> {};
  struct SHA256 : terminal<kw<"sha256">> {};
  struct HASH256 : terminal<kw<"hash256">> {};
  struct HASH160 : terminal<kw<"hash160">> {};
  struct RIPEMD160 : terminal<kw<"ripemd160">> {};
  struct MULTI : terminal<kw<"multi">> {};
  struct MULTI_A : terminal<kw<"multi_a">> {};
  struct AND_V : terminal<kw<"and_v">> {};
  struct AND_B : terminal<kw<"and_b">> {};
  struct AND_N : terminal<kw<"and_n">> {};
  struct OR_B : terminal<kw<"or_b">> {};
  struct OR_C : terminal<kw<"or_c">> {};
  struct OR_D : terminal<kw<"or_d">> {};
  struct OR_I : terminal<kw<"or_i">> {};
  struct ANDOR : terminal<kw<"andor">> {};
  struct THRESH : terminal<kw<"thresh">> {};

  struct PkTag : rule<ref<PK>, value<ast::KeyOp::pk>> {};
  struct PkhTag : rule<ref<PKH>, value<ast::KeyOp::pkh>> {};
  struct PkKTag : rule<ref<PK_K>, value<ast::KeyOp::pk_k>> {};
  struct PkHTag : rule<ref<PK_H>, value<ast::KeyOp::pk_h>> {};

  struct OlderTag : rule<ref<OLDER>, value<ast::TimeOp::older>> {};
  struct AfterTag : rule<ref<AFTER>, value<ast::TimeOp::after>> {};

  struct Sha256Tag : rule<ref<SHA256>, value<ast::HashOp::sha256>> {};
  struct Hash256Tag : rule<ref<HASH256>, value<ast::HashOp::hash256>> {};
  struct Hash160Tag : rule<ref<HASH160>, value<ast::HashOp::hash160>> {};
  struct Ripemd160Tag
      : rule<ref<RIPEMD160>, value<ast::HashOp::ripemd160>> {};

  struct AndVTag : rule<ref<AND_V>, value<ast::BinOp::and_v>> {};
  struct AndBTag : rule<ref<AND_B>, value<ast::BinOp::and_b>> {};
  struct AndNTag : rule<ref<AND_N>, value<ast::BinOp::and_n>> {};
  struct OrBTag : rule<ref<OR_B>, value<ast::BinOp::or_b>> {};
  struct OrCTag : rule<ref<OR_C>, value<ast::BinOp::or_c>> {};
  struct OrDTag : rule<ref<OR_D>, value<ast::BinOp::or_d>> {};
  struct OrITag : rule<ref<OR_I>, value<ast::BinOp::or_i>> {};

  // Fixed-width hex/decimal payloads. HEX32_BYTES/HEX20_BYTES are hex; an
  // argument that happens to be all-digits lexes as DIGITS_64/DIGITS_40
  // instead, so every rule that reads a hash or a key admits both kinds.
  static constexpr auto hex = cr<"0-9a-fA-F">;
  static constexpr auto digit = cr<"0-9">;
  static constexpr auto non_zero_digit = cr<"1-9">;

  struct COMPRESSED_PUBLIC_KEY
      : terminal<(lit<"02"> | lit<"03">) + repeat<64>(hex)> {};
  struct HEX32_BYTES : terminal<repeat<64>(hex)> {};
  struct HEX20_BYTES : terminal<repeat<40>(hex)> {};
  struct DIGITS_64
      : terminal<non_zero_digit + repeat<63>(digit)> {};
  struct DIGITS_40
      : terminal<non_zero_digit + repeat<39>(digit)> {};
  struct ZERO : terminal<lit<"0">> {};
  struct ONE : terminal<lit<"1">> {};
  struct POSITIVE_INTEGER
      : terminal<(cr<"2-9"> + many(digit)) | (lit<"1"> + some(digit))> {};

  // Opaque key text: hex keys, xpubs with origins and derivation paths,
  // symbolic names -- validating a BIP-380 shape is left downstream.
  struct KEY : terminal<some(cr<"A-Za-z0-9_./*'[]<>;-">)> {};

  struct WRAPPER_SEQUENCE
      : terminal<some(cr<"acdtvjnlsu">) + lit<":">, Wrappers> {};

  struct ReservedKeyWordRule
      : rule<ref<PK> | ref<PKH> | ref<PK_K> | ref<PK_H> | ref<OLDER> |
             ref<AFTER> | ref<SHA256> | ref<HASH256> | ref<HASH160> |
             ref<RIPEMD160> | ref<MULTI> | ref<MULTI_A> | ref<AND_V> |
             ref<AND_B> | ref<AND_N> | ref<OR_B> | ref<OR_C> | ref<OR_D> |
             ref<OR_I> | ref<ANDOR> | ref<THRESH>> {};

  struct KeyRule
      : rule<ref<COMPRESSED_PUBLIC_KEY> | ref<HEX32_BYTES> |
             ref<HEX20_BYTES> | ref<DIGITS_64> | ref<DIGITS_40> | ref<KEY> |
             ref<ZERO> | ref<ONE> | ref<POSITIVE_INTEGER> |
             ref<ReservedKeyWordRule>> {};

  // DIGITS_64/DIGITS_40 double as ordinary (very large) decimal integers.
  struct PositiveIntegerRule
      : rule<ref<ONE> | ref<DIGITS_64> | ref<DIGITS_40> |
             ref<POSITIVE_INTEGER>> {};

  struct Hash32Rule : rule<ref<HEX32_BYTES> | ref<DIGITS_64>> {};
  struct Hash20Rule : rule<ref<HEX20_BYTES> | ref<DIGITS_40>> {};

  struct ExpressionRule;
  struct BaseExpressionRule;

  struct ConstantRule
      : rule<create<ast::Constant>(
                 assign<&ast::Constant::value>(ref<ZERO> | ref<ONE>))> {};

  struct KeyExpressionRule
      : rule<create<ast::KeyExpression>(
                 assign<&ast::KeyExpression::op>(ref<PkTag> | ref<PkhTag> |
                                                  ref<PkKTag> | ref<PkHTag>) +
                 lit<"("> +
                 assign<&ast::KeyExpression::key>(ref<KeyRule>) +
                 lit<")">)> {};

  struct TimeExpressionRule
      : rule<create<ast::TimeExpression>(
                 assign<&ast::TimeExpression::op>(ref<OlderTag> |
                                                   ref<AfterTag>) +
                 lit<"("> +
                 assign<&ast::TimeExpression::value>(ref<PositiveIntegerRule>) +
                 lit<")">)> {};

  struct HashExpressionRule
      : rule<create<ast::HashExpression>(
                 (assign<&ast::HashExpression::op>(ref<Sha256Tag>) +
                  lit<"("> +
                  assign<&ast::HashExpression::hash>(ref<Hash32Rule>) +
                  lit<")">) |
                 (assign<&ast::HashExpression::op>(ref<Hash256Tag>) +
                  lit<"("> +
                  assign<&ast::HashExpression::hash>(ref<Hash32Rule>) +
                  lit<")">) |
                 (assign<&ast::HashExpression::op>(ref<Hash160Tag>) +
                  lit<"("> +
                  assign<&ast::HashExpression::hash>(ref<Hash20Rule>) +
                  lit<")">) |
                 (assign<&ast::HashExpression::op>(ref<Ripemd160Tag>) +
                  lit<"("> +
                  assign<&ast::HashExpression::hash>(ref<Hash20Rule>) +
                  lit<")">))> {};

  // Key cardinality is checked after parsing; both forms preserve every key.
  struct MultisigExpressionRule
      : rule<create<ast::MultisigExpression>(
            (ref<MULTI> + lit<"("> +
             assign<&ast::MultisigExpression::threshold>(
                 ref<PositiveIntegerRule>) +
             lit<","> + append<&ast::MultisigExpression::keys>(ref<KeyRule>) +
             many(lit<","> +
                  append<&ast::MultisigExpression::keys>(ref<KeyRule>)) +
             lit<")">) |
            (flag<&ast::MultisigExpression::isMultiA>(ref<MULTI_A>) +
             lit<"("> +
             assign<&ast::MultisigExpression::threshold>(
                 ref<PositiveIntegerRule>) +
             lit<","> +
             some(append<&ast::MultisigExpression::keys>(ref<KeyRule>),
                  lit<",">) +
             lit<")">))> {};

  struct BinaryExpressionRule
      : rule<create<ast::BinaryExpression>(
                 assign<&ast::BinaryExpression::op>(
                     ref<AndVTag> | ref<AndBTag> | ref<AndNTag> |
                     ref<OrBTag> | ref<OrCTag> | ref<OrDTag> | ref<OrITag>) +
                 lit<"("> +
                 assign<&ast::BinaryExpression::left>(ref<ExpressionRule>) +
                 lit<","> +
                 assign<&ast::BinaryExpression::right>(ref<ExpressionRule>) +
                 lit<")">)> {};

  struct TernaryExpressionRule
      : rule<create<ast::TernaryExpression>(
                 ref<ANDOR> + lit<"("> +
                 assign<&ast::TernaryExpression::a>(ref<ExpressionRule>) +
                 lit<","> +
                 assign<&ast::TernaryExpression::b>(ref<ExpressionRule>) +
                 lit<","> +
                 assign<&ast::TernaryExpression::c>(ref<ExpressionRule>) +
                 lit<")">)> {};

  struct ThresholdExpressionRule
      : rule<create<ast::ThresholdExpression>(
                 ref<THRESH> + lit<"("> +
                 assign<&ast::ThresholdExpression::threshold>(
                     ref<PositiveIntegerRule>) +
                 lit<","> +
                 some(append<&ast::ThresholdExpression::expressions>(
                          ref<ExpressionRule>),
                      lit<",">) +
                 lit<")">)> {};

  struct BaseExpressionRule
      : rule<ref<ConstantRule> | ref<KeyExpressionRule> |
             ref<TimeExpressionRule> | ref<HashExpressionRule> |
             ref<MultisigExpressionRule> | ref<BinaryExpressionRule> |
             ref<TernaryExpressionRule> | ref<ThresholdExpressionRule>> {};

  struct WrappedExpressionRule
      : rule<create<ast::WrappedExpression>(
                 assign<&ast::WrappedExpression::wrappers>(
                     ref<WRAPPER_SEQUENCE>) +
                 assign<&ast::WrappedExpression::inner>(
                     ref<BaseExpressionRule>))> {};

  struct ExpressionRule
      : rule<ref<WrappedExpressionRule> | ref<BaseExpressionRule>> {};

  struct DocumentRule
      : rule<assign<&ast::Document::expression>(ref<ExpressionRule>)> {};

  using entry = DocumentRule;
  using ignore = WS;
};

}  // namespace miniscript
