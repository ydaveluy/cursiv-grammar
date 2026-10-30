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
  static constexpr auto word_boundary = cr<"_a-zA-Z0-9.-">;

  static constexpr auto id_part = cr<"_a-zA-Z0-9.-">;
  static constexpr auto width_word = lit<"w"> + some(d) + !id_part;
  static constexpr auto reserved_word =
      kw<"array"> | kw<"query"> | kw<"symbolic"> | kw<"true"> | kw<"false"> |
      kw<"def"> | kw<"var"> | kw<"define"> | kw<"declare"> | width_word |
      (lit<"i"> + some(d) + !id_part) |
      (lit<"fp"> + d + ((some(d) + option(lit<"."> + many(id_part))) |
                         (lit<"."> + many(id_part))) + !id_part);
  struct ID : terminal<!reserved_word + cr<"_a-zA-Z"> + many(id_part)> {};
  struct WidthType : terminal<width_word> {};

  static constexpr auto ConstantBody = (d + many(cr<"0-9_">)) |
                                       (lit<"0b"> + some(cr<"01_">)) |
                                       (lit<"0o"> + some(cr<"0-7_">)) |
                                       (lit<"0x"> + some(cr<"0-9a-fA-F_">));
  struct Constant : terminal<ConstantBody + !cr<"A-Za-z0-9_">> {};
  struct SignedConstant : terminal<cr<"+-"> + ConstantBody + !cr<"A-Za-z0-9_">> {};
  struct Boolean : terminal<kw<"true"> | kw<"false">> {};

  struct Expression;
  struct ArrayVersion;

  struct Number
      : rule<assign<&ast::NumberLit::text>(ref<Boolean> | ref<SignedConstant> |
                                           ref<Constant>)> {};

  struct VariableName : rule<assign<&ast::VarRef::name>(ref<ID>)> {};

  struct NamedAbbrev
      : rule<assign<&ast::NamedExpr::name>(ref<ID>) + lit<":"> +
             assign<&ast::NamedExpr::value>(ref<Expression>)> {};

  struct SizeQueryExpr
      : rule<lit<"("> + assign<&ast::SizeExpr::width>(ref<WidthType>) +
             assign<&ast::SizeExpr::value>(ref<Number>) + lit<")">> {};

  static constexpr auto ArithOp = kw<"Add"> | kw<"Sub"> | kw<"Mul"> |
                                  kw<"UDiv"> | kw<"URem"> | kw<"SDiv"> |
                                  kw<"SRem">;
  struct ArithExpr
      : rule<lit<"("> + assign<&ast::BinaryExpr::op>(ArithOp) +
             option(assign<&ast::BinaryExpr::width>(ref<WidthType>)) +
             assign<&ast::BinaryExpr::left>(ref<Expression>) +
             assign<&ast::BinaryExpr::right>(ref<Expression>) + lit<")">> {};

  struct NotExpr
      : rule<lit<"("> + assign<&ast::UnaryExpr::op>(kw<"Not">) +
             option(assign<&ast::UnaryExpr::width>(ref<WidthType>)) +
             assign<&ast::UnaryExpr::operand>(ref<Expression>) + lit<")">> {};

  static constexpr auto BitwOp = kw<"And"> | kw<"Or"> | kw<"Xor"> | kw<"Shl"> |
                                 kw<"LShr"> | kw<"AShr">;
  struct BitwExpr
      : rule<lit<"("> + assign<&ast::BinaryExpr::op>(BitwOp) +
             option(assign<&ast::BinaryExpr::width>(ref<WidthType>)) +
             assign<&ast::BinaryExpr::left>(ref<Expression>) +
             assign<&ast::BinaryExpr::right>(ref<Expression>) + lit<")">> {};

  static constexpr auto CmpOp = kw<"Eq"> | kw<"Ne"> | kw<"Ult"> | kw<"Ule"> |
                                kw<"Ugt"> | kw<"Uge"> | kw<"Slt"> | kw<"Sle"> |
                                kw<"Sgt"> | kw<"Sge">;
  struct CompExpr
      : rule<lit<"("> + assign<&ast::BinaryExpr::op>(CmpOp) +
             option(assign<&ast::BinaryExpr::width>(ref<WidthType>)) +
             assign<&ast::BinaryExpr::left>(ref<Expression>) +
             assign<&ast::BinaryExpr::right>(ref<Expression>) + lit<")">> {};

  struct ConcatExpr
      : rule<create<ast::BinaryExpr>(
            lit<"("> + assign<&ast::BinaryExpr::op>(kw<"Concat">) +
            option(assign<&ast::BinaryExpr::width>(ref<WidthType>)) +
            assign<&ast::BinaryExpr::left>(ref<Expression>) +
            assign<&ast::BinaryExpr::right>(ref<Expression>) + lit<")">) |
             create<ast::ConcatExpr>(
            lit<"("> + kw<"Concat"> +
            option(assign<&ast::ConcatExpr::width>(ref<WidthType>)) +
            some(append<&ast::ConcatExpr::operands>(ref<Expression>)) + lit<")">)> {};

  struct ExtractExpr
      : rule<lit<"("> + kw<"Extract"> +
             option(assign<&ast::ExtractExpr::width>(ref<WidthType>)) +
             assign<&ast::ExtractExpr::offset>(ref<Number>) +
             assign<&ast::ExtractExpr::value>(ref<Expression>) + lit<")">> {};

  struct BitExtractExpr
      : rule<lit<"("> +
             assign<&ast::UnaryExpr::op>(kw<"ZExt"> | kw<"SExt">) +
             option(assign<&ast::UnaryExpr::width>(ref<WidthType>)) +
             assign<&ast::UnaryExpr::operand>(ref<Expression>) + lit<")">> {};

  struct ReadExpr
      : rule<lit<"("> +
             assign<&ast::ReadExpr::op>(kw<"Read"> | kw<"ReadLSB"> |
                                        kw<"ReadMSB">) +
             assign<&ast::ReadExpr::width>(ref<WidthType>) +
             assign<&ast::ReadExpr::index>(ref<Expression>) +
             assign<&ast::ReadExpr::array>(ref<ArrayVersion>) +
             lit<")">> {};

  struct SelectExpr
      : rule<lit<"("> + kw<"Select"> +
             option(assign<&ast::SelectExpr::width>(ref<WidthType>)) +
             assign<&ast::SelectExpr::cond>(ref<Expression>) +
             assign<&ast::SelectExpr::whenTrue>(ref<Expression>) +
             assign<&ast::SelectExpr::whenFalse>(ref<Expression>) +
             lit<")">> {};

  struct NegExpr
      : rule<lit<"("> + assign<&ast::UnaryExpr::op>(kw<"Neg">) +
             option(assign<&ast::UnaryExpr::width>(ref<WidthType>)) +
             assign<&ast::UnaryExpr::operand>(ref<Expression>) + lit<")">> {};

  struct UpdatePair
      : rule<assign<&ast::UpdatePair::index>(ref<Expression>) + lit<"="> +
             assign<&ast::UpdatePair::value>(ref<Expression>)> {};

  // `version`'s update-list form, merged straight into `expression`: its
  // other form (`varName (':' expression)?`) is the same text `varName` and
  // `NamedAbbrev`/`VariableName` already read, and keeping it as its own
  // rule forked every array-version reference into two equal derivations --
  // pathological on the chained updates `U0:[...] @ U1:[...] @ ...` this
  // corpus writes.
  struct UpdateExpr
      : rule<lit<"["> +
             option(some(append<&ast::UpdateExpr::updates>(ref<UpdatePair>),
                        lit<",">)) +
             lit<"]"> + lit<"@"> +
             assign<&ast::UpdateExpr::base>(ref<ArrayVersion>)> {};

  struct NamedVersion
      : rule<assign<&ast::NamedExpr::name>(ref<ID>) + lit<":"> +
             assign<&ast::NamedExpr::value>(ref<UpdateExpr>)> {};
  struct ArrayVersion : rule<ref<NamedVersion> | ref<VariableName> | ref<UpdateExpr>> {};

  struct Expression
      : rule<ref<NamedAbbrev> | ref<VariableName> | ref<SizeQueryExpr> |
             ref<ArithExpr> | ref<NotExpr> | ref<BitwExpr> |
             ref<CompExpr> | ref<ConcatExpr> | ref<ExtractExpr> |
             ref<BitExtractExpr> | ref<ReadExpr> |
             ref<SelectExpr> | ref<NegExpr> | ref<UpdateExpr> |
             ref<Number>> {};

  struct ArrayDecl
      : rule<kw<"array"> + assign<&ast::ArrayDecl::name>(ref<ID>) + lit<"["> +
             option(assign<&ast::ArrayDecl::size>(ref<Constant>)) + lit<"]"> +
             lit<":"> + assign<&ast::ArrayDecl::domain>(ref<WidthType>) +
             lit<"->"> + assign<&ast::ArrayDecl::range>(ref<WidthType>) +
             lit<"="> +
             (flag<&ast::ArrayDecl::symbolic>(kw<"symbolic">) |
              (lit<"["> +
               many(append<&ast::ArrayDecl::values>(ref<Number>) + option(lit<",">)) +
               lit<"]">))> {};

  struct QueryCommand
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

  struct Document
      : rule<create<ast::Document>(
            many(append<&ast::Document::units>(ref<ArrayDecl>) |
                 append<&ast::Document::units>(ref<QueryCommand>)))> {};

  using entry = Document;
  using ignored = WS;
  using hidden = list<LineComment>;
};

}  // namespace kquery
