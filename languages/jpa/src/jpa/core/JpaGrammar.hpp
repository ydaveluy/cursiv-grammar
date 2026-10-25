// The jpa language: its grammar (JPQL, the Java Persistence Query
// Language). Statements, one unified expression ladder from arithmetic
// through AND/OR, subqueries.
#pragma once

#include <cstddef>
#include <cstdlib>
#include <memory_resource>
#include <span>
#include <string>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "jpa/core/ast.hpp"

namespace jpa {

template <char Close>
inline constexpr auto undouble_impl = [](std::string_view body,
                                         std::pmr::memory_resource* mem) {
  if (body.find(Close) == std::string_view::npos) return body;
  const std::span<char> room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    room[n++] = body[i];
    if (body[i] == Close) ++i;
  }
  return std::string_view(room.data(), n);
};

inline constexpr auto unquote_string = [](std::string_view t,
                                          std::pmr::memory_resource* mem) {
  return undouble_impl<'\''>(t.substr(1, t.size() - 2), mem);
};
using StringConv = cursiv::text_conv<unquote_string>;

inline constexpr auto unquote_char = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using CharConv = cursiv::text_conv<unquote_char>;

inline constexpr auto parse_numeric = [](std::string_view t) -> double {
  const std::string s(t);
  return std::strtod(s.c_str(), nullptr);
};
using NumConv = cursiv::text_conv<parse_numeric>;

struct Jpa : cursiv::GrammarSpec {
  // -- forward declarations: statements, subqueries and the expression
  // ladder are mutually recursive (an expr can hold a subquery, a subquery
  // holds an expr). ----
  struct PathExprRule;
  struct PathExprMultiRule;
  struct NumberLitRule;
  struct StringLitRule;
  struct BoolLitRule;
  struct NullLitRule;
  struct CharLitRule;
  struct InputParamRule;
  struct LiteralRule;
  struct InItemRule;
  struct PatternValueRule;
  struct AggregateExprRule;
  struct Func1ExprRule;
  struct ModExprRule;
  struct ConcatExprRule;
  struct LocateExprRule;
  struct SubstringExprRule;
  struct TrimExprRule;
  struct CurrentValueExprRule;
  struct ObjectExprRule;
  struct ConstructorExprRule;
  struct SubqueryRule;
  struct SubqueryExprRule;
  struct AllAnyExprRule;
  struct ExistsExprRule;
  struct ExprBase;
  struct ExprUnary;
  struct ExprMul;
  struct ExprAdd;
  struct ExprPredicate;
  struct ExprNot;
  struct Expression;
  struct RangeVarDeclRule;
  struct JoinRule;
  struct IdVarDeclRule;
  struct AliasedPathDeclRule;
  struct CollectionMemberDeclRule;
  struct OrderByItemRule;
  struct SelectExpressionRule;
  struct SimpleSelectExpressionRule;
  struct SelectStatementRule;
  struct UpdateItemRule;
  struct UpdateStatementRule;
  struct DeleteStatementRule;
  struct DocumentRule;

  struct WS : terminal<some(s)> {};

  struct IDENTIFICATION_VARIABLE : terminal<cr<"a-zA-Z_"> + many(w)> {};
  struct INT_NUMERAL : terminal<some(d)> {};

  static constexpr auto Digits = some(d);
  struct NUMERIC_LITERAL
      : terminal<Digits + option(lit<"."> + option(Digits)) +
                     option((lit<"e"> | lit<"E">) + option(cr<"+-">) + Digits),
                 NumConv> {};

  struct STRINGLITERAL
      : terminal<lit<"'"> + many(lit<"''"> | cr<"^'">) + lit<"'">,
                 StringConv> {};
  // Also the character type for ESCAPE and TRIM clauses; both reuse this
  // terminal rather than declaring their own.
  struct CHARACTER_LITERAL
      : terminal<lit<"'"> + cr<R"(^'\\)"> + lit<"'">, CharConv> {};

  // -- path expressions: state field, single/collection-valued association
  // and join paths all reduce to one dotted identifier chain, PathExpr.
  // PathExprRule also accepts a bare single segment; PathExprMultiRule
  // requires the leading dot a multi-segment path always has, so a
  // FROM-clause item can never read as both a path and a bare range
  // variable declaration.
  struct PathExprRule
      : rule<append<&ast::PathExpr::segments>(ref<IDENTIFICATION_VARIABLE>) +
             many(lit<"."> +
                  append<&ast::PathExpr::segments>(ref<IDENTIFICATION_VARIABLE>))> {
  };
  struct PathExprMultiRule
      : rule<append<&ast::PathExpr::segments>(ref<IDENTIFICATION_VARIABLE>) +
             some(lit<"."> +
                  append<&ast::PathExpr::segments>(ref<IDENTIFICATION_VARIABLE>))> {
  };

  struct NumberLitRule
      : rule<assign<&ast::NumberLit::value>(ref<NUMERIC_LITERAL>)> {};
  struct StringLitRule
      : rule<assign<&ast::StringLit::value>(ref<STRINGLITERAL>)> {};
  struct BoolLitRule
      : rule<create<ast::BoolLit> +
             (flag<&ast::BoolLit::value>(ikw<"true">) | ikw<"false">)> {};
  struct NullLitRule : rule<create<ast::NullLit> + ikw<"NULL">> {};
  struct CharLitRule
      : rule<assign<&ast::CharLit::value>(ref<CHARACTER_LITERAL>)> {};
  struct InputParamRule
      : rule<(flag<&ast::InputParam::positional>(lit<"?">) +
              assign<&ast::InputParam::text>(ref<INT_NUMERAL>)) |
             (lit<":"> + assign<&ast::InputParam::text>(ref<IDENTIFICATION_VARIABLE>))> {
  };

  // An enum literal is just an unqualified name, already a one-segment
  // PathExpr -- no separate case needed here.
  struct LiteralRule
      : rule<ref<StringLitRule> | ref<NumberLitRule> | ref<BoolLitRule> |
             ref<PathExprRule>> {};
  struct InItemRule : rule<ref<LiteralRule> | ref<InputParamRule>> {};
  struct PatternValueRule
      : rule<ref<StringLitRule> | ref<InputParamRule>> {};

  struct AggAvgTag : rule<ikw<"AVG">, value<ast::AggFunc::avg>> {};
  struct AggMaxTag : rule<ikw<"MAX">, value<ast::AggFunc::max>> {};
  struct AggMinTag : rule<ikw<"MIN">, value<ast::AggFunc::min>> {};
  struct AggSumTag : rule<ikw<"SUM">, value<ast::AggFunc::sum>> {};
  struct AggCountTag : rule<ikw<"COUNT">, value<ast::AggFunc::count>> {};
  struct AggregateExprRule
      : rule<assign<&ast::AggregateExpr::func>(
                 ref<AggAvgTag> | ref<AggMaxTag> | ref<AggMinTag> |
                 ref<AggSumTag> | ref<AggCountTag>) +
             lit<"("> +
             option(flag<&ast::AggregateExpr::distinct>(ikw<"DISTINCT">)) +
             assign<&ast::AggregateExpr::arg>(ref<PathExprRule>) + lit<")">> {
  };

  struct Func1LengthTag : rule<ikw<"LENGTH">, value<ast::Func1::length>> {};
  struct Func1AbsTag : rule<ikw<"ABS">, value<ast::Func1::abs>> {};
  struct Func1SqrtTag : rule<ikw<"SQRT">, value<ast::Func1::sqrt>> {};
  struct Func1SizeTag : rule<ikw<"SIZE">, value<ast::Func1::size>> {};
  struct Func1LowerTag : rule<ikw<"LOWER">, value<ast::Func1::lower>> {};
  struct Func1UpperTag : rule<ikw<"UPPER">, value<ast::Func1::upper>> {};
  struct Func1ExprRule
      : rule<assign<&ast::Func1Expr::func>(
                 ref<Func1LengthTag> | ref<Func1AbsTag> | ref<Func1SqrtTag> |
                 ref<Func1SizeTag> | ref<Func1LowerTag> | ref<Func1UpperTag>) +
             lit<"("> + assign<&ast::Func1Expr::arg>(ref<Expression>) +
             lit<")">> {};
  struct ModExprRule
      : rule<ikw<"MOD"> + lit<"("> +
             assign<&ast::ModExpr::left>(ref<Expression>) + lit<","> +
             assign<&ast::ModExpr::right>(ref<Expression>) + lit<")">> {};
  struct ConcatExprRule
      : rule<ikw<"CONCAT"> + lit<"("> +
             assign<&ast::ConcatExpr::left>(ref<Expression>) + lit<","> +
             assign<&ast::ConcatExpr::right>(ref<Expression>) + lit<")">> {};
  struct LocateExprRule
      : rule<ikw<"LOCATE"> + lit<"("> +
             assign<&ast::LocateExpr::pattern>(ref<Expression>) + lit<","> +
             assign<&ast::LocateExpr::text>(ref<Expression>) +
             option(lit<","> +
                    assign<&ast::LocateExpr::start>(ref<Expression>)) +
             lit<")">> {};
  struct SubstringExprRule
      : rule<ikw<"SUBSTRING"> + lit<"("> +
             assign<&ast::SubstringExpr::source>(ref<Expression>) + lit<","> +
             assign<&ast::SubstringExpr::start>(ref<Expression>) + lit<","> +
             assign<&ast::SubstringExpr::length>(ref<Expression>) +
             lit<")">> {};
  struct TrimLeadingTag : rule<ikw<"LEADING">, value<ast::TrimSpec::leading>> {};
  struct TrimTrailingTag
      : rule<ikw<"TRAILING">, value<ast::TrimSpec::trailing>> {};
  struct TrimBothTag : rule<ikw<"BOTH">, value<ast::TrimSpec::both>> {};
  struct TrimExprRule
      : rule<ikw<"TRIM"> + lit<"("> +
             option(option(assign<&ast::TrimExpr::spec>(
                        ref<TrimLeadingTag> | ref<TrimTrailingTag> |
                        ref<TrimBothTag>)) +
                    option(assign<&ast::TrimExpr::trimChar>(ref<CharLitRule>)) +
                    ikw<"FROM">) +
             assign<&ast::TrimExpr::source>(ref<Expression>) + lit<")">> {};
  struct CurrentDateTag
      : rule<ikw<"CURRENT_DATE">, value<ast::CurrentKind::date>> {};
  struct CurrentTimeTag
      : rule<ikw<"CURRENT_TIME">, value<ast::CurrentKind::time>> {};
  struct CurrentTimestampTag
      : rule<ikw<"CURRENT_TIMESTAMP">, value<ast::CurrentKind::timestamp>> {};
  struct CurrentValueExprRule
      : rule<assign<&ast::CurrentValueExpr::kind>(
            ref<CurrentDateTag> | ref<CurrentTimeTag> |
            ref<CurrentTimestampTag>)> {};

  struct ObjectExprRule
      : rule<ikw<"OBJECT"> + lit<"("> +
             assign<&ast::ObjectExpr::alias>(ref<IDENTIFICATION_VARIABLE>) +
             lit<")">> {};

  // -- from-clause items: no expression dependency, safe to define before
  // the expression ladder that needs SubqueryRule. ----
  struct RangeVarDeclRule
      : rule<assign<&ast::RangeVarDecl::schemaName>(
                 ref<IDENTIFICATION_VARIABLE>) +
             option(ikw<"AS">) +
             assign<&ast::RangeVarDecl::alias>(ref<IDENTIFICATION_VARIABLE>)> {
  };
  struct JoinSpecLeftOuterTag
      : rule<ikw<"LEFT"> + ikw<"OUTER"> + ikw<"JOIN">,
             value<ast::JoinKind::left_outer>> {};
  struct JoinSpecLeftTag
      : rule<ikw<"LEFT"> + ikw<"JOIN">, value<ast::JoinKind::left>> {};
  struct JoinSpecInnerTag
      : rule<ikw<"INNER"> + ikw<"JOIN">, value<ast::JoinKind::inner>> {};
  struct JoinSpecPlainTag : rule<ikw<"JOIN">, value<ast::JoinKind::plain>> {};
  struct JoinRule
      : rule<assign<&ast::JoinStep::kind>(
                 ref<JoinSpecLeftOuterTag> | ref<JoinSpecLeftTag> |
                 ref<JoinSpecInnerTag> | ref<JoinSpecPlainTag>) +
             ((flag<&ast::JoinStep::fetch>(ikw<"FETCH">) +
               assign<&ast::JoinStep::path>(ref<PathExprMultiRule>)) |
              (assign<&ast::JoinStep::path>(ref<PathExprMultiRule>) +
               option(ikw<"AS">) +
               assign<&ast::JoinStep::alias>(ref<IDENTIFICATION_VARIABLE>)))> {
  };
  struct IdVarDeclRule
      : rule<assign<&ast::IdVarDecl::range>(ref<RangeVarDeclRule>) +
             many(append<&ast::IdVarDecl::joins>(ref<JoinRule>))> {};
  struct AliasedPathDeclRule
      : rule<assign<&ast::AliasedPathDecl::path>(ref<PathExprMultiRule>) +
             option(ikw<"AS">) +
             assign<&ast::AliasedPathDecl::alias>(ref<IDENTIFICATION_VARIABLE>)> {
  };
  struct CollectionMemberDeclRule
      : rule<ikw<"IN"> + lit<"("> +
             assign<&ast::CollectionMemberDecl::path>(ref<PathExprMultiRule>) +
             lit<")"> + option(ikw<"AS">) +
             assign<&ast::CollectionMemberDecl::alias>(
                 ref<IDENTIFICATION_VARIABLE>)> {};

  struct SimpleSelectExpressionRule
      : rule<ref<PathExprRule> | ref<AggregateExprRule>> {};
  struct ConstructorExprRule
      : rule<ikw<"NEW"> +
             assign<&ast::ConstructorExpr::className>(ref<PathExprRule>) +
             lit<"("> +
             append<&ast::ConstructorExpr::args>(
                 ref<SimpleSelectExpressionRule>) +
             many(lit<","> +
                  append<&ast::ConstructorExpr::args>(
                      ref<SimpleSelectExpressionRule>)) +
             lit<")">> {};
  struct SelectExpressionRule
      : rule<ref<PathExprRule> | ref<AggregateExprRule> | ref<ObjectExprRule> |
             ref<ConstructorExprRule>> {};

  struct SubqueryRule
      : rule<ikw<"SELECT"> +
             option(flag<&ast::Subquery::distinct>(ikw<"DISTINCT">)) +
             assign<&ast::Subquery::column>(ref<SimpleSelectExpressionRule>) +
             ikw<"FROM"> +
             append<&ast::Subquery::from>(
                 ref<IdVarDeclRule> | ref<AliasedPathDeclRule> |
                 ref<CollectionMemberDeclRule>) +
             many(lit<","> +
                  append<&ast::Subquery::from>(
                      ref<IdVarDeclRule> | ref<AliasedPathDeclRule> |
                      ref<CollectionMemberDeclRule>)) +
             option(ikw<"WHERE"> + assign<&ast::Subquery::where>(ref<Expression>)) +
             option(ikw<"GROUP"> + ikw<"BY"> +
                    append<&ast::Subquery::groupBy>(ref<PathExprRule>) +
                    many(lit<","> +
                         append<&ast::Subquery::groupBy>(ref<PathExprRule>))) +
             option(ikw<"HAVING"> +
                    assign<&ast::Subquery::having>(ref<Expression>))> {};

  struct SubqueryExprRule
      : rule<lit<"("> + assign<&ast::SubqueryExpr::query>(ref<SubqueryRule>) +
             lit<")">> {};
  struct AllAnyAllTag : rule<ikw<"ALL">, value<ast::AllAnyKind::all>> {};
  struct AllAnyAnyTag : rule<ikw<"ANY">, value<ast::AllAnyKind::any>> {};
  struct AllAnySomeTag : rule<ikw<"SOME">, value<ast::AllAnyKind::some>> {};
  struct AllAnyExprRule
      : rule<assign<&ast::AllAnyExpr::kind>(
                 ref<AllAnyAllTag> | ref<AllAnyAnyTag> | ref<AllAnySomeTag>) +
             lit<"("> + assign<&ast::AllAnyExpr::query>(ref<SubqueryRule>) +
             lit<")">> {};
  struct ExistsExprRule
      : rule<option(flag<&ast::ExistsExpr::negated>(ikw<"NOT">)) +
             ikw<"EXISTS"> + lit<"("> +
             assign<&ast::ExistsExpr::query>(ref<SubqueryRule>) + lit<")">> {};

  // -- expression ladder, tightest to loosest ----
  struct ExprBase
      : rule<ref<NumberLitRule> | ref<StringLitRule> | ref<BoolLitRule> |
             ref<NullLitRule> | ref<InputParamRule> | ref<ExistsExprRule> |
             ref<AggregateExprRule> | ref<Func1ExprRule> | ref<ModExprRule> |
             ref<ConcatExprRule> | ref<LocateExprRule> |
             ref<SubstringExprRule> | ref<TrimExprRule> |
             ref<CurrentValueExprRule> | ref<SubqueryExprRule> |
             ref<PathExprRule> | (lit<"("> + ref<Expression> + lit<")">)> {};

  struct ExprUnary
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-">) +
                 assign<&ast::UnaryExpr::operand>(ref<ExprUnary>)) |
             ref<ExprBase>> {};

  struct OpMul : terminal<lit<"*">, value<ast::Operator::multiply>> {};
  struct OpDiv : terminal<lit<"/">, value<ast::Operator::divide>> {};
  struct ExprMul
      : rule<ref<ExprUnary> +
             many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 assign<&ast::BinaryExpr::op>(ref<OpMul> | ref<OpDiv>) +
                 assign<&ast::BinaryExpr::right>(ref<ExprUnary>)))> {};

  struct OpAdd : terminal<lit<"+">, value<ast::Operator::add>> {};
  struct OpSub : terminal<lit<"-">, value<ast::Operator::subtract>> {};
  struct ExprAdd
      : rule<ref<ExprMul> +
             many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 assign<&ast::BinaryExpr::op>(ref<OpAdd> | ref<OpSub>) +
                 assign<&ast::BinaryExpr::right>(ref<ExprMul>)))> {};

  struct CmpLe : terminal<lit<"<=">, value<ast::Operator::le>> {};
  struct CmpGe : terminal<lit<">=">, value<ast::Operator::ge>> {};
  struct CmpNe : terminal<lit<"<>">, value<ast::Operator::ne>> {};
  struct CmpLt : terminal<lit<"<">, value<ast::Operator::lt>> {};
  struct CmpGt : terminal<lit<">">, value<ast::Operator::gt>> {};
  struct CmpEq : terminal<lit<"=">, value<ast::Operator::eq>> {};

  // Comparison, BETWEEN, LIKE, IN, IS [NOT] NULL/EMPTY, MEMBER OF: one
  // shape regardless of operand type (string/boolean/enum/datetime/entity/
  // arithmetic) -- the distinction is semantic, not syntactic, the same
  // reasoning that already folds every path-expression flavor into
  // PathExpr.
  struct ExprPredicate
      : rule<ref<ExprAdd> +
             option(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                      assign<&ast::BinaryExpr::op>(
                          ref<CmpLe> | ref<CmpGe> | ref<CmpNe> | ref<CmpLt> |
                          ref<CmpGt> | ref<CmpEq>) +
                      assign<&ast::BinaryExpr::right>(ref<ExprAdd> |
                                                      ref<AllAnyExprRule>)) |
                  nest<ast::BetweenExpr, &ast::BetweenExpr::operand>(
                      option(flag<&ast::BetweenExpr::negated>(
                          ikw<"NOT">)) +
                      ikw<"BETWEEN"> +
                      assign<&ast::BetweenExpr::low>(ref<ExprAdd>) +
                      ikw<"AND"> +
                      assign<&ast::BetweenExpr::high>(ref<ExprAdd>)) |
                  nest<ast::LikeExpr, &ast::LikeExpr::operand>(
                      option(flag<&ast::LikeExpr::negated>(ikw<"NOT">)) +
                      ikw<"LIKE"> +
                      assign<&ast::LikeExpr::pattern>(ref<PatternValueRule>) +
                      option(ikw<"ESCAPE"> +
                             assign<&ast::LikeExpr::escape>(
                                 ref<CharLitRule>))) |
                  nest<ast::InExpr, &ast::InExpr::operand>(
                      option(flag<&ast::InExpr::negated>(ikw<"NOT">)) +
                      ikw<"IN"> + lit<"("> +
                      ((append<&ast::InExpr::items>(ref<InItemRule>) +
                        many(lit<","> +
                             append<&ast::InExpr::items>(ref<InItemRule>))) |
                       assign<&ast::InExpr::subquery>(ref<SubqueryRule>)) +
                      lit<")">) |
                  nest<ast::IsNullExpr, &ast::IsNullExpr::operand>(
                      ikw<"IS"> +
                      option(flag<&ast::IsNullExpr::negated>(
                          ikw<"NOT">)) +
                      ikw<"NULL">) |
                  nest<ast::IsEmptyExpr, &ast::IsEmptyExpr::operand>(
                      ikw<"IS"> +
                      option(flag<&ast::IsEmptyExpr::negated>(
                          ikw<"NOT">)) +
                      ikw<"EMPTY">) |
                  nest<ast::MemberOfExpr, &ast::MemberOfExpr::entity>(
                      option(flag<&ast::MemberOfExpr::negated>(
                          ikw<"NOT">)) +
                      ikw<"MEMBER"> + option(ikw<"OF">) +
                      assign<&ast::MemberOfExpr::collection>(
                          ref<PathExprRule>)))> {};

  struct ExprNot
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ikw<"NOT">) +
                 assign<&ast::UnaryExpr::operand>(ref<ExprNot>)) |
             ref<ExprPredicate>> {};

  struct AndOp : rule<ikw<"AND">, value<ast::Operator::logical_and>> {};
  struct OrOp : rule<ikw<"OR">, value<ast::Operator::logical_or>> {};
  struct Expression
      : infix<ExprNot, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(ref<AndOp>) >> left(ref<OrOp>))> {};

  struct OrderByItemRule
      : rule<assign<&ast::OrderByItem::key>(ref<PathExprRule>) +
             option(flag<&ast::OrderByItem::descending>(ikw<"DESC">) |
                    ikw<"ASC">)> {};

  // -- statements ----
  struct SelectStatementRule
      : rule<ikw<"SELECT"> +
             option(flag<&ast::SelectStatement::distinct>(
                 ikw<"DISTINCT">)) +
             append<&ast::SelectStatement::columns>(ref<SelectExpressionRule>) +
             many(lit<","> +
                  append<&ast::SelectStatement::columns>(
                      ref<SelectExpressionRule>)) +
             ikw<"FROM"> +
             append<&ast::SelectStatement::from>(ref<IdVarDeclRule>) +
             many(lit<","> +
                  (append<&ast::SelectStatement::from>(ref<IdVarDeclRule>) |
                   append<&ast::SelectStatement::from>(
                       ref<CollectionMemberDeclRule>))) +
             option(ikw<"WHERE"> +
                    assign<&ast::SelectStatement::where>(ref<Expression>)) +
             option(ikw<"GROUP"> + ikw<"BY"> +
                    append<&ast::SelectStatement::groupBy>(ref<PathExprRule>) +
                    many(lit<","> +
                         append<&ast::SelectStatement::groupBy>(
                             ref<PathExprRule>))) +
             option(ikw<"HAVING"> +
                    assign<&ast::SelectStatement::having>(ref<Expression>)) +
             option(ikw<"ORDER"> + ikw<"BY"> +
                    append<&ast::SelectStatement::orderBy>(
                        ref<OrderByItemRule>) +
                    many(lit<","> +
                         append<&ast::SelectStatement::orderBy>(
                             ref<OrderByItemRule>)))> {};

  struct UpdateItemRule
      : rule<assign<&ast::UpdateItem::target>(ref<PathExprRule>) + lit<"="> +
             assign<&ast::UpdateItem::value>(ref<Expression>)> {};
  struct UpdateStatementRule
      : rule<ikw<"UPDATE"> +
             assign<&ast::UpdateStatement::schemaName>(
                 ref<IDENTIFICATION_VARIABLE>) +
             option(option(ikw<"AS">) +
                    assign<&ast::UpdateStatement::alias>(
                        ref<IDENTIFICATION_VARIABLE>)) +
             ikw<"SET"> +
             append<&ast::UpdateStatement::items>(ref<UpdateItemRule>) +
             many(lit<","> +
                  append<&ast::UpdateStatement::items>(ref<UpdateItemRule>)) +
             option(ikw<"WHERE"> +
                    assign<&ast::UpdateStatement::where>(ref<Expression>))> {};

  struct DeleteStatementRule
      : rule<ikw<"DELETE"> + ikw<"FROM"> +
             assign<&ast::DeleteStatement::schemaName>(
                 ref<IDENTIFICATION_VARIABLE>) +
             option(option(ikw<"AS">) +
                    assign<&ast::DeleteStatement::alias>(
                        ref<IDENTIFICATION_VARIABLE>)) +
             option(ikw<"WHERE"> +
                    assign<&ast::DeleteStatement::where>(ref<Expression>))> {};

  struct DocumentRule
      : rule<assign<&ast::Document::statement>(
            ref<SelectStatementRule> | ref<UpdateStatementRule> |
            ref<DeleteStatementRule>)> {};

  using entry = DocumentRule;
  using ignore = WS;
};

}  // namespace jpa
