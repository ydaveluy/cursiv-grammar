// The kquery language: its grammar. KLEE's KQuery is one S-expression
// language: array declarations, then query commands whose every operator
// form is `( keyword width? operand... )`, dispatched on the keyword alone.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "kquery/core/ast.hpp"

namespace kquery {

struct Kquery : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct LineComment : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};

  // Identifiers may carry dots (`sample1_pse`, `main.users`-style names),
  // so the boundary a keyword may not run into has to include one too.
  static constexpr auto word_boundary = cr<"_a-zA-Z0-9.">;

  struct ID : terminal<cr<"_a-zA-Z"> + many(cr<"_a-zA-Z0-9.">)> {};

  struct WidthType : terminal<lit<"w"> + some(d)> {};

  static constexpr auto ConstantBody = some(d) |
                                       (lit<"0b"> + some(cr<"01_">)) |
                                       (lit<"0o"> + some(cr<"0-7_">)) |
                                       (lit<"0x"> + some(cr<"0-9a-fA-F_">));
  struct Constant : terminal<ConstantBody> {};
  struct SignedConstant : terminal<cr<"+-"> + ConstantBody> {};
  struct Boolean : terminal<kw<"true"> | kw<"false">> {};

  struct Expression;

  struct NumberRule
      : rule<assign<&ast::NumberLit::text>(ref<Boolean> | ref<SignedConstant> |
                                           ref<Constant>)> {};

  struct VariableName : rule<assign<&ast::VarRef::name>(ref<ID>)> {};

  struct NamedAbbrev
      : rule<assign<&ast::NamedExpr::name>(ref<ID>) + lit<":"> +
             assign<&ast::NamedExpr::value>(ref<Expression>)> {};

  struct SizeQueryExpr
      : rule<lit<"("> + assign<&ast::SizeExpr::width>(ref<WidthType>) +
             assign<&ast::SizeExpr::value>(ref<NumberRule>) + lit<")">> {};

  static constexpr auto ArithOp = kw<"Add"> | kw<"Sub"> | kw<"Mul"> |
                                  kw<"UDiv"> | kw<"URem"> | kw<"SDiv"> |
                                  kw<"SRem">;
  struct ArithExprRule
      : rule<lit<"("> + assign<&ast::BinaryExpr::op>(ArithOp) +
             assign<&ast::BinaryExpr::width>(ref<WidthType>) +
             assign<&ast::BinaryExpr::left>(ref<Expression>) +
             assign<&ast::BinaryExpr::right>(ref<Expression>) + lit<")">> {};

  struct NotExprRule
      : rule<lit<"("> + assign<&ast::UnaryExpr::op>(kw<"Not">) + lit<"["> +
             assign<&ast::UnaryExpr::width>(ref<WidthType>) + lit<"]"> +
             assign<&ast::UnaryExpr::operand>(ref<Expression>) + lit<")">> {};

  static constexpr auto BitwOp = kw<"And"> | kw<"Or"> | kw<"Xor"> | kw<"Shl"> |
                                 kw<"LShr"> | kw<"AShr">;
  struct BitwExprRule
      : rule<lit<"("> + assign<&ast::BinaryExpr::op>(BitwOp) +
             assign<&ast::BinaryExpr::width>(ref<WidthType>) +
             assign<&ast::BinaryExpr::left>(ref<Expression>) +
             assign<&ast::BinaryExpr::right>(ref<Expression>) + lit<")">> {};

  static constexpr auto CmpOp = kw<"Eq"> | kw<"Ne"> | kw<"Ult"> | kw<"Ule"> |
                                kw<"Ugt"> | kw<"Uge"> | kw<"Slt"> | kw<"Sle"> |
                                kw<"Sgt"> | kw<"Sge">;
  struct CompExprRule
      : rule<lit<"("> + assign<&ast::BinaryExpr::op>(CmpOp) +
             option(assign<&ast::BinaryExpr::width>(ref<WidthType>)) +
             assign<&ast::BinaryExpr::left>(ref<Expression>) +
             assign<&ast::BinaryExpr::right>(ref<Expression>) + lit<")">> {};

  struct ConcatExprRule
      : rule<lit<"("> + assign<&ast::BinaryExpr::op>(kw<"Concat">) +
             option(assign<&ast::BinaryExpr::width>(ref<WidthType>)) +
             assign<&ast::BinaryExpr::left>(ref<Expression>) +
             assign<&ast::BinaryExpr::right>(ref<Expression>) + lit<")">> {};

  struct ExtractExprRule
      : rule<lit<"("> + kw<"Extract"> +
             assign<&ast::ExtractExpr::width>(ref<WidthType>) +
             assign<&ast::ExtractExpr::offset>(ref<NumberRule>) +
             assign<&ast::ExtractExpr::value>(ref<Expression>) + lit<")">> {};

  struct BitExtractExprRule
      : rule<lit<"("> +
             assign<&ast::UnaryExpr::op>(kw<"ZExt"> | kw<"SExt">) +
             assign<&ast::UnaryExpr::width>(ref<WidthType>) +
             assign<&ast::UnaryExpr::operand>(ref<Expression>) + lit<")">> {};

  struct ReadExprRule
      : rule<lit<"("> +
             assign<&ast::ReadExpr::op>(kw<"Read"> | kw<"ReadLSB"> |
                                        kw<"ReadMSB">) +
             assign<&ast::ReadExpr::width>(ref<WidthType>) +
             assign<&ast::ReadExpr::index>(ref<Expression>) +
             option(assign<&ast::ReadExpr::array>(ref<Expression>)) +
             lit<")">> {};

  struct SelectExprRule
      : rule<lit<"("> + kw<"Select"> +
             assign<&ast::SelectExpr::width>(ref<WidthType>) +
             assign<&ast::SelectExpr::cond>(ref<Expression>) +
             assign<&ast::SelectExpr::whenTrue>(ref<Expression>) +
             assign<&ast::SelectExpr::whenFalse>(ref<Expression>) +
             lit<")">> {};

  struct NegExprRule
      : rule<lit<"("> + assign<&ast::UnaryExpr::op>(kw<"Neg">) +
             option(assign<&ast::UnaryExpr::width>(ref<WidthType>)) +
             assign<&ast::UnaryExpr::operand>(ref<Expression>) + lit<")">> {};

  struct UpdatePairRule
      : rule<assign<&ast::UpdatePair::index>(ref<Expression>) + lit<"="> +
             assign<&ast::UpdatePair::value>(ref<Expression>)> {};

  // `version`'s update-list form, merged straight into `expression`: its
  // other form (`varName (':' expression)?`) is the same text `varName` and
  // `NamedAbbrev`/`VariableName` already read, and keeping it as its own
  // rule forked every array-version reference into two equal derivations --
  // pathological on the chained updates `U0:[...] @ U1:[...] @ ...` this
  // corpus writes.
  struct UpdateExprRule
      : rule<lit<"["> +
             option(some(append<&ast::UpdateExpr::updates>(ref<UpdatePairRule>),
                        lit<",">)) +
             lit<"]"> + lit<"@"> +
             assign<&ast::UpdateExpr::base>(ref<Expression>)> {};

  struct Expression
      : rule<ref<NamedAbbrev> | ref<VariableName> | ref<SizeQueryExpr> |
             ref<ArithExprRule> | ref<NotExprRule> | ref<BitwExprRule> |
             ref<CompExprRule> | ref<ConcatExprRule> | ref<ExtractExprRule> |
             ref<BitExtractExprRule> | ref<ReadExprRule> |
             ref<SelectExprRule> | ref<NegExprRule> | ref<UpdateExprRule> |
             ref<NumberRule>> {};

  struct ArrayDeclRule
      : rule<kw<"array"> + assign<&ast::ArrayDecl::name>(ref<ID>) + lit<"["> +
             assign<&ast::ArrayDecl::size>(ref<Constant>) + lit<"]"> +
             lit<":"> + assign<&ast::ArrayDecl::domain>(ref<WidthType>) +
             lit<"->"> + assign<&ast::ArrayDecl::range>(ref<WidthType>) +
             lit<"="> +
             (flag<&ast::ArrayDecl::symbolic>(kw<"symbolic">) |
              (lit<"["> +
               many(append<&ast::ArrayDecl::values>(ref<NumberRule>) + option(lit<",">)) +
               lit<"]">))> {};

  struct QueryCommandRule
      : rule<lit<"("> + kw<"query"> + lit<"["> +
             many(append<&ast::QueryCommand::constraints>(ref<Expression>)) +
             lit<"]"> + assign<&ast::QueryCommand::query>(ref<Expression>) +
             option(lit<"["> +
                    many(append<&ast::QueryCommand::evalExprs>(ref<Expression>)) +
                    lit<"]"> +
                    option(lit<"["> +
                           many(append<&ast::QueryCommand::evalArrayNames>(
                               ref<ID>)) +
                           lit<"]">)) +
             lit<")">> {};

  struct DocumentRule
      : rule<create<ast::Document>(
            many(append<&ast::Document::units>(ref<ArrayDeclRule>) |
                 append<&ast::Document::units>(ref<QueryCommandRule>)))> {};

  using entry = DocumentRule;
  using ignore = WS;
  using hidden = list<LineComment>;
};

}  // namespace kquery
