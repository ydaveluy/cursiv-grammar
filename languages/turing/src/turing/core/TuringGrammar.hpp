// The turing language: its grammar.
#pragma once

#include <cstdint>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "turing/core/ast.hpp"

namespace turing {

struct Turing : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct SL_COMMENT : terminal<lit<"%"> + many(cr<R"(^\r\n)">)> {};

  struct ID : terminal<cr<"a-zA-Z"> + many(w)> {};

  struct INT : terminal<option(cr<"+-">) + some(d), std::int64_t> {};
  // The point (present here) is what tells a real from an int; the exponent
  // is optional, matching ordinary numeric-literal use. Digits are required
  // on both sides of the point: a bare trailing `.` would swallow one dot of
  // a subrange's `..`.
  struct REAL
      : terminal<option(cr<"+-">) +
                     ((some(d) + lit<"."> + some(d)) | (lit<"."> + some(d))) +
                     option(lit<"e"> + option(cr<"+-">) + some(d)),
                 double> {};

  inline static constexpr auto unquote = [](std::string_view t) {
    return t.substr(1, t.size() - 2);
  };
  using Unquoted = text_conv<unquote>;
  struct STRING
      : terminal<lit<"\""> + many(cr<R"(^")">) + lit<"\"">, Unquoted> {};

  struct KW_INT : terminal<kw<"int">, value<ast::StandardType::Kind::int_>> {
  };
  struct KW_REAL
      : terminal<kw<"real">, value<ast::StandardType::Kind::real>> {};
  struct KW_BOOLEAN
      : terminal<kw<"boolean">, value<ast::StandardType::Kind::boolean>> {};
  struct KW_STRING
      : terminal<kw<"string">, value<ast::StandardType::Kind::string>> {};

  struct CAP_GET : terminal<kw<"get">, value<ast::Capability::get_>> {};
  struct CAP_PUT : terminal<kw<"put">, value<ast::Capability::put_>> {};

  // Each operator token denotes its own constant, so `op` holds meaning
  // rather than spelling. Subtraction and unary minus use ASCII `-`,
  // matching the numeric-literal tokens below.
  struct OP_POW : terminal<lit<"**">, value<ast::BinOp::exponent>> {};
  struct OP_MUL : terminal<lit<"*">, value<ast::BinOp::multiply>> {};
  struct OP_DIVR : terminal<lit<"/">, value<ast::BinOp::divide>> {};
  struct OP_DIVI : terminal<kw<"div">, value<ast::BinOp::intDivide>> {};
  struct OP_MOD : terminal<kw<"mod">, value<ast::BinOp::modulo>> {};
  struct OP_ADD : terminal<lit<"+">, value<ast::BinOp::add>> {};
  struct OP_SUB : terminal<lit<"-">, value<ast::BinOp::subtract>> {};
  struct OP_LE : terminal<lit<"<=">, value<ast::BinOp::lessEqual>> {};
  struct OP_GE : terminal<lit<">=">, value<ast::BinOp::greaterEqual>> {};
  struct OP_NE : terminal<lit<"not=">, value<ast::BinOp::notEqual>> {};
  struct OP_LT : terminal<lit<"<">, value<ast::BinOp::less>> {};
  struct OP_GT : terminal<lit<">">, value<ast::BinOp::greater>> {};
  struct OP_EQ : terminal<lit<"=">, value<ast::BinOp::equal>> {};
  struct OP_AND : terminal<kw<"and">, value<ast::BinOp::logicalAnd>> {};
  struct OP_OR : terminal<kw<"or">, value<ast::BinOp::logicalOr>> {};

  struct UNARY_PLUS : terminal<lit<"+">, value<ast::UnOp::plus>> {};
  struct UNARY_MINUS : terminal<lit<"-">, value<ast::UnOp::minus>> {};
  struct UNARY_NOT : terminal<kw<"not">, value<ast::UnOp::logicalNot>> {};

  struct Expression;
  struct TypeSpecRule;
  struct ItemRule;

  // One repeated append covers a field access, an index/call, or a
  // substring range.
  struct FieldSelectorRule
      : rule<create<ast::FieldSelector>(
            lit<"."> + assign<&ast::FieldSelector::field>(ref<ID>))> {};

  struct IndexSelectorRule
      : rule<create<ast::IndexSelector>(
            lit<"("> +
            some(append<&ast::IndexSelector::args>(ref<Expression>), lit<",">) +
            lit<")">)> {};

  struct StarPositionRule
      : rule<create<ast::Position>(
            flag<&ast::Position::fromEnd>(lit<"*">) +
            option(lit<"-"> +
                   assign<&ast::Position::offset>(ref<Expression>)))> {};
  struct PlainPositionRule
      : rule<create<ast::Position>(
            assign<&ast::Position::value>(ref<Expression>))> {};
  static constexpr auto Position =
      ref<StarPositionRule> | ref<PlainPositionRule>;

  // A range needs `*` or an explicit `..` to be told apart from a plain
  // one-argument index/call -- a lone value is genuinely ambiguous between
  // the two, and this keeps that case an index.
  struct RangeSelectorRule
      : rule<lit<"("> +
            create<ast::RangeSelector>(
                (assign<&ast::RangeSelector::from>(ref<StarPositionRule>) +
                 option(lit<".."> +
                        assign<&ast::RangeSelector::to>(Position))) |
                (assign<&ast::RangeSelector::from>(ref<PlainPositionRule>) +
                 lit<".."> + assign<&ast::RangeSelector::to>(Position))) +
            lit<")">> {};

  struct SelectorRule
      : rule<ref<FieldSelectorRule> | ref<IndexSelectorRule> |
            ref<RangeSelectorRule>> {};

  struct ReferenceRule
      : rule<create<ast::Reference>(
            assign<&ast::Reference::name>(ref<ID>) +
            many(append<&ast::Reference::selectors>(ref<SelectorRule>)))> {};

  struct IntLitRule
      : rule<create<ast::IntLit>(assign<&ast::IntLit::value>(ref<INT>))> {};
  struct RealLitRule
      : rule<create<ast::RealLit>(assign<&ast::RealLit::value>(ref<REAL>))> {
  };
  struct StringLitRule
      : rule<create<ast::StringLit>(
            assign<&ast::StringLit::value>(ref<STRING>))> {};
  struct BoolLitRule
      : rule<create<ast::BoolLit>(
            flag<&ast::BoolLit::value>(kw<"true">) | kw<"false">)> {};

  struct Primary
      : rule<create<ast::ParenExpr>(
                 lit<"("> + assign<&ast::ParenExpr::inner>(ref<Expression>) +
                 lit<")">) |
            ref<RealLitRule> | ref<IntLitRule> | ref<StringLitRule> |
            ref<BoolLitRule> |
            ref<ReferenceRule>> {};

  struct Unary
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ref<UNARY_PLUS> |
                                              ref<UNARY_MINUS> |
                                              ref<UNARY_NOT>) +
                 assign<&ast::UnaryExpr::operand>(ref<Unary>)) |
            ref<Primary>> {};

  struct Expression
      : infix<Unary, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (right(ref<OP_POW>) >>
               left(ref<OP_MUL> | ref<OP_DIVR> | ref<OP_DIVI> |
                    ref<OP_MOD>) >>
               left(ref<OP_ADD> | ref<OP_SUB>) >>
               left(ref<OP_LE> | ref<OP_GE> | ref<OP_NE> | ref<OP_EQ> |
                    ref<OP_LT> | ref<OP_GT>) >>
               left(ref<OP_AND>) >> left(ref<OP_OR>))> {};

  // An initializer is a plain value, or `init(...)` nesting more of the same
  // for an array or record.
  struct InitListRule
      : rule<create<ast::InitList>(
            kw<"init"> + lit<"("> +
            some(append<&ast::InitList::items>(ref<Expression> |
                                                ref<InitListRule>),
                 lit<",">) +
            lit<")">)> {};
  static constexpr auto InitValue = ref<Expression> | ref<InitListRule>;

  struct StandardTypeRule
      : rule<create<ast::StandardType>(
            assign<&ast::StandardType::kind>(ref<KW_INT> | ref<KW_REAL> |
                                              ref<KW_BOOLEAN>) |
            (assign<&ast::StandardType::kind>(ref<KW_STRING>) +
             option(lit<"("> +
                    assign<&ast::StandardType::length>(ref<Expression>) +
                    lit<")">)))> {};

  struct SubrangeTypeRule
      : rule<create<ast::SubrangeType>(
            assign<&ast::SubrangeType::low>(ref<Expression>) + lit<".."> +
            assign<&ast::SubrangeType::high>(ref<Expression>))> {};

  struct NamedTypeRule
      : rule<create<ast::NamedType>(
            assign<&ast::NamedType::name>(ref<ID>))> {};

  static constexpr auto IndexType = ref<SubrangeTypeRule> | ref<NamedTypeRule>;

  struct ArrayTypeRule
      : rule<create<ast::ArrayType>(
            kw<"array"> +
            some(append<&ast::ArrayType::indices>(IndexType), lit<",">) +
            kw<"of"> +
            assign<&ast::ArrayType::element>(ref<TypeSpecRule>))> {};

  struct RecordFieldRule
      : rule<create<ast::RecordField>(
            some(append<&ast::RecordField::names>(ref<ID>), lit<",">) +
            lit<":"> +
            assign<&ast::RecordField::type>(ref<TypeSpecRule>))> {};

  struct RecordTypeRule
      : rule<create<ast::RecordType>(
            kw<"record"> +
            some(append<&ast::RecordType::fields>(ref<RecordFieldRule>)) +
            kw<"end"> + kw<"record">)> {};

  struct TypeSpecRule
      : rule<ref<StandardTypeRule> | ref<SubrangeTypeRule> |
            ref<ArrayTypeRule> | ref<RecordTypeRule> | ref<NamedTypeRule>> {};

  struct StringStarTypeRule
      : rule<create<ast::StringStarType> + kw<"string"> + lit<"("> +
            lit<"*"> + lit<")">> {};

  struct SimpleParamTypeRule
      : rule<create<ast::SimpleParamType>(
            assign<&ast::SimpleParamType::type>(ref<TypeSpecRule> |
                                                 ref<StringStarTypeRule>))> {
  };

  struct OpenArrayParamTypeRule
      : rule<create<ast::OpenArrayParamType>(
            kw<"array"> +
            some(append<&ast::OpenArrayParamType::bounds>(ref<Expression>) +
                     lit<".."> + lit<"*">,
                 lit<",">) +
            kw<"of"> +
            assign<&ast::OpenArrayParamType::element>(
                ref<TypeSpecRule> | ref<StringStarTypeRule>))> {};

  struct ParamTypeRule
      : rule<ref<SimpleParamTypeRule> | ref<OpenArrayParamTypeRule>> {};

  struct ParamRule
      : rule<create<ast::Param>(
            option(flag<&ast::Param::isVar>(kw<"var">)) +
            some(append<&ast::Param::names>(ref<ID>), lit<",">) + lit<":"> +
            assign<&ast::Param::type>(ref<ParamTypeRule>))> {};

  // The two forms real Turing declares: a procedure has no return type, a
  // function's header ends in `: typeSpec`.
  struct SubprogramDeclRule
      : rule<create<ast::SubprogramDecl>(
                 kw<"procedure"> +
                 assign<&ast::SubprogramDecl::name>(ref<ID>) +
                 option(lit<"("> +
                        some(append<&ast::SubprogramDecl::params>(
                                 ref<ParamRule>),
                             lit<",">) +
                        lit<")">) +
                 many(append<&ast::SubprogramDecl::body>(ref<ItemRule>)) +
                 kw<"end"> +
                 assign<&ast::SubprogramDecl::endName>(ref<ID>)) |
            create<ast::SubprogramDecl>(
                flag<&ast::SubprogramDecl::isFunction>(
                    kw<"function">) +
                assign<&ast::SubprogramDecl::name>(ref<ID>) +
                option(lit<"("> +
                       some(append<&ast::SubprogramDecl::params>(
                                ref<ParamRule>),
                            lit<",">) +
                       lit<")">) +
                lit<":"> +
                assign<&ast::SubprogramDecl::returnType>(ref<TypeSpecRule>) +
                many(append<&ast::SubprogramDecl::body>(ref<ItemRule>)) +
                kw<"end"> +
                assign<&ast::SubprogramDecl::endName>(ref<ID>))> {};

  struct ConstDeclRule
      : rule<create<ast::ConstDecl>(
            kw<"const"> + assign<&ast::ConstDecl::name>(ref<ID>) +
            option(lit<":"> +
                   assign<&ast::ConstDecl::typeSpec>(ref<TypeSpecRule>)) +
            lit<":="> + assign<&ast::ConstDecl::value>(InitValue))> {};

  struct VarDeclRule
      : rule<create<ast::VarDecl>(
            kw<"var"> +
            some(append<&ast::VarDecl::names>(ref<ID>), lit<",">) +
            ((lit<":="> + assign<&ast::VarDecl::value>(InitValue)) |
             (lit<":"> +
              assign<&ast::VarDecl::typeSpec>(ref<TypeSpecRule>) +
              option(lit<":="> +
                     assign<&ast::VarDecl::value>(InitValue)))))> {};

  struct TypeDeclRule
      : rule<create<ast::TypeDecl>(
            kw<"type"> + assign<&ast::TypeDecl::name>(ref<ID>) + lit<":"> +
            assign<&ast::TypeDecl::typeSpec>(ref<TypeSpecRule>))> {};

  struct DeclRule
      : rule<ref<ConstDeclRule> | ref<VarDeclRule> | ref<TypeDeclRule>> {};

  struct AssignStmtRule
      : rule<create<ast::AssignStmt>(
            assign<&ast::AssignStmt::target>(ref<ReferenceRule>) +
            lit<":="> + assign<&ast::AssignStmt::value>(ref<Expression>))> {
  };
  struct CallStmtRule
      : rule<create<ast::CallStmt>(
            assign<&ast::CallStmt::call>(ref<ReferenceRule>))> {};
  struct AssertStmtRule
      : rule<create<ast::AssertStmt>(
            kw<"assert"> + assign<&ast::AssertStmt::cond>(ref<Expression>))> {
  };
  struct ResultStmtRule
      : rule<create<ast::ResultStmt>(
            kw<"result"> +
            assign<&ast::ResultStmt::value>(ref<Expression>))> {};

  struct ElsifClauseRule
      : rule<create<ast::ElsifClause>(
            kw<"elsif"> + assign<&ast::ElsifClause::cond>(ref<Expression>) +
            kw<"then"> +
            many(append<&ast::ElsifClause::body>(ref<ItemRule>)))> {};

  struct IfStmtRule
      : rule<create<ast::IfStmt>(
            kw<"if"> + assign<&ast::IfStmt::cond>(ref<Expression>) +
            kw<"then"> +
            many(append<&ast::IfStmt::thenBody>(ref<ItemRule>)) +
            many(append<&ast::IfStmt::elsifs>(ref<ElsifClauseRule>)) +
            option(kw<"else"> +
                   many(append<&ast::IfStmt::elseBody>(ref<ItemRule>))) +
            kw<"end"> + kw<"if">)> {};

  struct LoopStmtRule
      : rule<create<ast::LoopStmt>(
            kw<"loop"> + many(append<&ast::LoopStmt::body>(ref<ItemRule>)) +
            kw<"end"> + kw<"loop">)> {};

  struct ExitStmtRule
      : rule<create<ast::ExitStmt>(
            kw<"exit"> +
            option(kw<"when"> +
                   assign<&ast::ExitStmt::cond>(ref<Expression>)))> {};

  // Simplified: a bare default `label :` may repeat and appear anywhere,
  // rather than at most once and last.
  struct CaseLabelRule
      : rule<create<ast::CaseLabel>(
            kw<"label"> +
            ((some(append<&ast::CaseLabel::values>(ref<Expression>), lit<",">) +
              lit<":">) |
             lit<":">) +
            many(append<&ast::CaseLabel::body>(ref<ItemRule>)))> {};

  struct CaseStmtRule
      : rule<create<ast::CaseStmt>(
            kw<"case"> + assign<&ast::CaseStmt::value>(ref<Expression>) +
            kw<"of"> +
            some(append<&ast::CaseStmt::labels>(ref<CaseLabelRule>)) +
            kw<"end"> + kw<"case">)> {};

  struct ForStmtRule
      : rule<create<ast::ForStmt>(
            kw<"for"> +
            option(flag<&ast::ForStmt::decreasing>(kw<"decreasing">)) +
            assign<&ast::ForStmt::varName>(ref<ID>) + lit<":"> +
            assign<&ast::ForStmt::from>(ref<Expression>) + lit<".."> +
            assign<&ast::ForStmt::to>(ref<Expression>) +
            option(kw<"by"> +
                   assign<&ast::ForStmt::step>(ref<Expression>)) +
            many(append<&ast::ForStmt::body>(ref<ItemRule>)) + kw<"end"> +
            kw<"for">)> {};

  struct PutItemRule
      : rule<create<ast::PutItem>(kw<"skip">) |
            create<ast::PutItem>(
                assign<&ast::PutItem::value>(ref<Expression>) +
                option(lit<":"> +
                       assign<&ast::PutItem::width>(ref<Expression>) +
                       option(lit<":"> +
                              assign<&ast::PutItem::fraction>(
                                  ref<Expression>) +
                              option(lit<":"> +
                                     assign<&ast::PutItem::exponent>(
                                         ref<Expression>)))))> {};

  struct PutStmtRule
      : rule<create<ast::PutStmt>(
            kw<"put"> +
            option(lit<":"> +
                   assign<&ast::PutStmt::stream>(ref<Expression>) +
                   lit<",">) +
            some(append<&ast::PutStmt::items>(ref<PutItemRule>), lit<",">) +
            option(flag<&ast::PutStmt::noNewline>(lit<"..">)))> {};

  struct GetItemRule
      : rule<create<ast::GetItem>(
                 flag<&ast::GetItem::skipToEnd>(kw<"skip">) +
                 assign<&ast::GetItem::target>(ref<ReferenceRule>) +
                 lit<":"> + lit<"*">) |
            create<ast::GetItem>(
                assign<&ast::GetItem::target>(ref<ReferenceRule>) +
                option(lit<":"> +
                       assign<&ast::GetItem::width>(ref<Expression>)))> {};

  struct GetStmtRule
      : rule<create<ast::GetStmt>(
            kw<"get"> +
            option(lit<":"> +
                   assign<&ast::GetStmt::stream>(ref<Expression>) +
                   lit<",">) +
            some(append<&ast::GetStmt::items>(ref<GetItemRule>), lit<",">))> {
  };

  struct OpenStmtRule
      : rule<create<ast::OpenStmt>(
            kw<"open"> + lit<":"> +
            assign<&ast::OpenStmt::fileNumber>(ref<Expression>) + lit<","> +
            assign<&ast::OpenStmt::path>(ref<STRING>) + lit<","> +
            some(append<&ast::OpenStmt::capabilities>(ref<CAP_GET> |
                                                        ref<CAP_PUT>),
                 lit<",">))> {};

  struct CloseStmtRule
      : rule<create<ast::CloseStmt>(
            kw<"close"> + lit<":"> +
            assign<&ast::CloseStmt::fileNumber>(ref<Expression>))> {};

  struct StmtRule
      : rule<ref<AssignStmtRule> | ref<CallStmtRule> | ref<AssertStmtRule> |
            ref<ResultStmtRule> | ref<IfStmtRule> | ref<LoopStmtRule> |
            ref<ExitStmtRule> | ref<CaseStmtRule> | ref<ForStmtRule> |
            ref<PutStmtRule> | ref<GetStmtRule> | ref<OpenStmtRule> |
            ref<CloseStmtRule>> {};

  struct ItemRule : rule<ref<DeclRule> | ref<StmtRule>> {};

  struct ProgramItemRule
      : rule<ref<DeclRule> | ref<StmtRule> | ref<SubprogramDeclRule>> {};

  struct ProgramRule
      : rule<create<ast::Program>(
            some(append<&ast::Program::items>(ref<ProgramItemRule>)))> {};

  using entry = ProgramRule;
  using ignore = WS;
  using hidden = list<SL_COMMENT>;
};

}  // namespace turing
