#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "miniscript/core/ast.hpp"

namespace miniscript {

inline constexpr auto strip_colon = [](std::string_view t) {
  return t.substr(0, t.size() - 1);
};

struct Miniscript : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};

  struct INTEGER : terminal<some(d)> {};
  static constexpr auto hex = cr<"0-9a-fA-F">;
  struct HASH32 : terminal<repeat<64>(hex)> {};
  struct HASH20 : terminal<repeat<40>(hex)> {};

  // Keys remain opaque, including descriptor origins and derivation paths.
  // Keywords are contextual, so pk(pk) needs no duplicate keyword grammar.
  struct KEY : terminal<some(cr<"A-Za-z0-9_./*'[]<>;-">)> {};
  struct WRAPPER_SEQUENCE
      : terminal<as<strip_colon>(some(cr<"acdtvjnlsu">) + lit<":">)> {};

  struct Expression;
  struct BaseExpression;

  struct Constant
      : rule<create<ast::Constant>(
                 assign<&ast::Constant::value>(lit<"0"> | lit<"1">))> {};

  struct KeyExpression
      : rule<create<ast::KeyExpression>(
                 assign<&ast::KeyExpression::op>(as<ast::KeyOp::pk>(kw<"pk">) | as<ast::KeyOp::pkh>(kw<"pkh">) |
                                                  as<ast::KeyOp::pk_k>(kw<"pk_k">) | as<ast::KeyOp::pk_h>(kw<"pk_h">)) +
                 lit<"("> +
                 assign<&ast::KeyExpression::key>(ref<KEY>) +
                 lit<")">)> {};

  struct TimeExpression
      : rule<create<ast::TimeExpression>(
                 assign<&ast::TimeExpression::op>(as<ast::TimeOp::older>(kw<"older">) |
                                                   as<ast::TimeOp::after>(kw<"after">)) +
                 lit<"("> +
                 assign<&ast::TimeExpression::value>(ref<INTEGER>) +
                 lit<")">)> {};

  struct HashExpression
      : rule<create<ast::HashExpression>(
                 (assign<&ast::HashExpression::op>(as<ast::HashOp::sha256>(kw<"sha256">)) +
                  lit<"("> +
                  assign<&ast::HashExpression::hash>(ref<HASH32>) +
                  lit<")">) |
                 (assign<&ast::HashExpression::op>(as<ast::HashOp::hash256>(kw<"hash256">)) +
                  lit<"("> +
                  assign<&ast::HashExpression::hash>(ref<HASH32>) +
                  lit<")">) |
                 (assign<&ast::HashExpression::op>(as<ast::HashOp::hash160>(kw<"hash160">)) +
                  lit<"("> +
                  assign<&ast::HashExpression::hash>(ref<HASH20>) +
                  lit<")">) |
                 (assign<&ast::HashExpression::op>(as<ast::HashOp::ripemd160>(kw<"ripemd160">)) +
                  lit<"("> +
                  assign<&ast::HashExpression::hash>(ref<HASH20>) +
                  lit<")">))> {};

  // Key cardinality is checked after parsing; both forms preserve every key.
  struct MultisigExpression
      : rule<create<ast::MultisigExpression>(
            (kw<"multi"> | flag<&ast::MultisigExpression::isMultiA>(kw<"multi_a">)) +
             lit<"("> +
             assign<&ast::MultisigExpression::threshold>(
                 ref<INTEGER>) +
             lit<","> +
             some(append<&ast::MultisigExpression::keys>(ref<KEY>),
                  lit<",">) +
             lit<")">)> {};

  struct BinaryExpression
      : rule<create<ast::BinaryExpression>(
                 assign<&ast::BinaryExpression::op>(
                     as<ast::BinOp::and_v>(kw<"and_v">) | as<ast::BinOp::and_b>(kw<"and_b">) | as<ast::BinOp::and_n>(kw<"and_n">) |
                     as<ast::BinOp::or_b>(kw<"or_b">) | as<ast::BinOp::or_c>(kw<"or_c">) | as<ast::BinOp::or_d>(kw<"or_d">) | as<ast::BinOp::or_i>(kw<"or_i">)) +
                 lit<"("> +
                 assign<&ast::BinaryExpression::left>(ref<Expression>) +
                 lit<","> +
                 assign<&ast::BinaryExpression::right>(ref<Expression>) +
                 lit<")">)> {};

  struct TernaryExpression
      : rule<create<ast::TernaryExpression>(
                 kw<"andor"> + lit<"("> +
                 assign<&ast::TernaryExpression::a>(ref<Expression>) +
                 lit<","> +
                 assign<&ast::TernaryExpression::b>(ref<Expression>) +
                 lit<","> +
                 assign<&ast::TernaryExpression::c>(ref<Expression>) +
                 lit<")">)> {};

  struct ThresholdExpression
      : rule<create<ast::ThresholdExpression>(
                 kw<"thresh"> + lit<"("> +
                 assign<&ast::ThresholdExpression::threshold>(
                     ref<INTEGER>) +
                 lit<","> +
                 some(append<&ast::ThresholdExpression::expressions>(
                          ref<Expression>),
                      lit<",">) +
                 lit<")">)> {};

  struct BaseExpression
      : rule<ref<Constant> | ref<KeyExpression> |
             ref<TimeExpression> | ref<HashExpression> |
             ref<MultisigExpression> | ref<BinaryExpression> |
             ref<TernaryExpression> | ref<ThresholdExpression>> {};

  struct WrappedExpression
      : rule<create<ast::WrappedExpression>(
                 assign<&ast::WrappedExpression::wrappers>(
                     ref<WRAPPER_SEQUENCE>) +
                 assign<&ast::WrappedExpression::inner>(
                     ref<BaseExpression>))> {};

  struct Expression
      : rule<ref<WrappedExpression> | ref<BaseExpression>> {};

  struct Document
      : rule<assign<&ast::Document::expression>(ref<Expression>)> {};

  using entry = Document;
  using ignored = WS;
};

}  // namespace miniscript
