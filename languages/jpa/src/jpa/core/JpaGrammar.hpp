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

inline constexpr auto unquote_char = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};

inline constexpr auto parse_numeric = [](std::string_view t) -> double {
  const std::string s(t);
  return std::strtod(s.c_str(), nullptr);
};

struct Jpa : cursiv::GrammarSpec {
  // -- forward declarations: statements, subqueries and the expression
  // ladder are mutually recursive (an expr can hold a subquery, a subquery
  // holds an expr). ----
  struct PathExprRule;
  struct PathExprMulti;
  struct NumberLit;
  struct StringLit;
  struct BoolLit;
  struct NullLit;
  struct CharLit;
  struct InputParam;
  struct Literal;
  struct InItem;
  struct PatternValue;
  struct AggregateExpr;
  struct Func1Expr;
  struct ModExpr;
  struct ConcatExpr;
  struct LocateExpr;
  struct SubstringExpr;
  struct TrimExpr;
  struct CurrentValueExpr;
  struct ObjectExpr;
  struct ConstructorExpr;
  struct Subquery;
  struct SubqueryExpr;
  struct AllAnyExpr;
  struct ExistsExpr;
  struct ExprBase;
  struct ExprUnary;
  struct ExprMul;
  struct ExprAdd;
  struct ExprPredicate;
  struct ExprNot;
  struct Expression;
  struct RangeVarDecl;
  struct Join;
  struct IdVarDecl;
  struct AliasedPathDecl;
  struct CollectionMemberDecl;
  struct OrderByItem;
  struct SelectExpression;
  struct SimpleSelectExpression;
  struct SelectStatement;
  struct UpdateItem;
  struct UpdateStatement;
  struct DeleteStatement;
  struct Document;

  struct WS : terminal<some(s)> {};

  // JPQL 1.0, 4.4.1: Character.isJavaIdentifierStart/Part.
  static constexpr auto IdentifierStart = uc::L | uc::Nl | uc::Sc | uc::Pc;
  static constexpr auto IdentifierPart =
      IdentifierStart | uc::Mn | uc::Mc | uc::Nd | uc::Cf |
      cr<R"(\x00-\x08\x0e-\x1b\x7f-\x9f)">;
  static constexpr auto word_boundary = IdentifierPart;
  struct IDENTIFICATION_VARIABLE
      : terminal<IdentifierStart + many(IdentifierPart)> {};
  struct INT_NUMERAL : terminal<some(d)> {};

  static constexpr auto Digits = some(d);
  struct NUMERIC_LITERAL
      : terminal<as<parse_numeric>(
            (Digits + cr<"lL">) |
            (((Digits + option(lit<"."> + option(Digits))) | (lit<"."> + Digits)) +
             option(cr<"eE"> + option(cr<"+-">) + Digits) + option(cr<"fFdD">)))> {};

  struct STRINGLITERAL
      : terminal<as<unquote_string>(lit<"'"> + many(lit<"''"> | cr<"^'">) +
                                    lit<"'">)> {};
  // Also the character type for ESCAPE and TRIM clauses; both reuse this
  // terminal rather than declaring their own.
  struct CHARACTER_LITERAL
      : terminal<as<unquote_string>(lit<"'"> + (lit<"''"> | cr<"^'">) + lit<"'">)> {};

  // -- path expressions: state field, single/collection-valued association
  // and join paths all reduce to one dotted identifier chain, PathExpr.
  // PathExprRule also accepts a bare single segment; PathExprMulti
  // requires the leading dot a multi-segment path always has, so a
  // FROM-clause item can never read as both a path and a bare range
  // variable declaration.
  struct PathExprRule
      : rule<some(append<&ast::PathExpr::segments>(ref<IDENTIFICATION_VARIABLE>), lit<".">)> {
  };
  struct PathExprMulti
      : rule<append<&ast::PathExpr::segments>(ref<IDENTIFICATION_VARIABLE>) +
             some(lit<"."> +
                  append<&ast::PathExpr::segments>(ref<IDENTIFICATION_VARIABLE>))> {
  };

  struct NumberLit
      : rule<assign<&ast::NumberLit::value>(ref<NUMERIC_LITERAL>)> {};
  struct StringLit
      : rule<assign<&ast::StringLit::value>(ref<STRINGLITERAL>)> {};
  struct BoolLit
      : rule<create<ast::BoolLit> +
             (flag<&ast::BoolLit::value>(ikw<"true">) | ikw<"false">)> {};
  struct NullLit : rule<create<ast::NullLit> + ikw<"NULL">> {};
  struct CharLit
      : rule<assign<&ast::CharLit::value>(ref<CHARACTER_LITERAL>)> {};
  struct InputParam
      : rule<(flag<&ast::InputParam::positional>(lit<"?">) +
              assign<&ast::InputParam::text>(ref<INT_NUMERAL>)) |
             (lit<":"> + assign<&ast::InputParam::text>(ref<IDENTIFICATION_VARIABLE>))> {
  };

  // An enum literal is just an unqualified name, already a one-segment
  // PathExpr -- no separate case needed here.
  struct Literal
      : rule<ref<StringLit> | ref<NumberLit> | ref<BoolLit> |
             ref<PathExprRule>> {};
  struct InItem : rule<ref<Literal> | ref<InputParam>> {};
  struct PatternValue
      : rule<ref<StringLit> | ref<InputParam>> {};

  struct AggregateExpr
      : rule<assign<&ast::AggregateExpr::func>(
                 as<ast::AggFunc::avg>(ikw<"AVG">) | as<ast::AggFunc::max>(ikw<"MAX">) | as<ast::AggFunc::min>(ikw<"MIN">) |
                 as<ast::AggFunc::sum>(ikw<"SUM">) | as<ast::AggFunc::count>(ikw<"COUNT">)) +
             lit<"("> +
             option(flag<&ast::AggregateExpr::distinct>(ikw<"DISTINCT">)) +
             assign<&ast::AggregateExpr::arg>(ref<PathExprRule>) + lit<")">> {
  };

  struct Func1Expr
      : rule<assign<&ast::Func1Expr::func>(
                 as<ast::Func1::length>(ikw<"LENGTH">) | as<ast::Func1::abs>(ikw<"ABS">) | as<ast::Func1::sqrt>(ikw<"SQRT">) |
                 as<ast::Func1::size>(ikw<"SIZE">) | as<ast::Func1::lower>(ikw<"LOWER">) | as<ast::Func1::upper>(ikw<"UPPER">)) +
             lit<"("> + assign<&ast::Func1Expr::arg>(ref<Expression>) +
             lit<")">> {};
  struct ModExpr
      : rule<ikw<"MOD"> + lit<"("> +
             assign<&ast::ModExpr::left>(ref<Expression>) + lit<","> +
             assign<&ast::ModExpr::right>(ref<Expression>) + lit<")">> {};
  struct ConcatExpr
      : rule<ikw<"CONCAT"> + lit<"("> +
             assign<&ast::ConcatExpr::left>(ref<Expression>) + lit<","> +
             assign<&ast::ConcatExpr::right>(ref<Expression>) + lit<")">> {};
  struct LocateExpr
      : rule<ikw<"LOCATE"> + lit<"("> +
             assign<&ast::LocateExpr::pattern>(ref<Expression>) + lit<","> +
             assign<&ast::LocateExpr::text>(ref<Expression>) +
             option(lit<","> +
                    assign<&ast::LocateExpr::start>(ref<Expression>)) +
             lit<")">> {};
  struct SubstringExpr
      : rule<ikw<"SUBSTRING"> + lit<"("> +
             assign<&ast::SubstringExpr::source>(ref<Expression>) + lit<","> +
             assign<&ast::SubstringExpr::start>(ref<Expression>) + lit<","> +
             assign<&ast::SubstringExpr::length>(ref<Expression>) +
             lit<")">> {};
  struct TrimExpr
      : rule<ikw<"TRIM"> + lit<"("> +
             option(option(assign<&ast::TrimExpr::spec>(
                        as<ast::TrimSpec::leading>(ikw<"LEADING">) | as<ast::TrimSpec::trailing>(ikw<"TRAILING">) |
                        as<ast::TrimSpec::both>(ikw<"BOTH">))) +
                    option(assign<&ast::TrimExpr::trimChar>(ref<CharLit> | ref<InputParam>)) +
                    ikw<"FROM">) +
             assign<&ast::TrimExpr::source>(ref<Expression>) + lit<")">> {};
  struct CurrentValueExpr
      : rule<assign<&ast::CurrentValueExpr::kind>(
            as<ast::CurrentKind::date>(ikw<"CURRENT_DATE">) | as<ast::CurrentKind::time>(ikw<"CURRENT_TIME">) |
            as<ast::CurrentKind::timestamp>(ikw<"CURRENT_TIMESTAMP">))> {};

  struct ObjectExpr
      : rule<ikw<"OBJECT"> + lit<"("> +
             assign<&ast::ObjectExpr::alias>(ref<IDENTIFICATION_VARIABLE>) +
             lit<")">> {};

  // -- from-clause items: no expression dependency, safe to define before
  // the expression ladder that needs Subquery. ----
  struct RangeVarDecl
      : rule<assign<&ast::RangeVarDecl::schemaName>(
                 ref<IDENTIFICATION_VARIABLE>) +
             option(ikw<"AS">) +
             assign<&ast::RangeVarDecl::alias>(ref<IDENTIFICATION_VARIABLE>)> {
  };
  struct Join
      : rule<assign<&ast::JoinStep::kind>(
                 as<ast::JoinKind::left_outer>(ikw<"LEFT"> + ikw<"OUTER"> + ikw<"JOIN">) | as<ast::JoinKind::left>(ikw<"LEFT"> + ikw<"JOIN">) |
                 as<ast::JoinKind::inner>(ikw<"INNER"> + ikw<"JOIN">) | as<ast::JoinKind::plain>(ikw<"JOIN">)) +
             ((flag<&ast::JoinStep::fetch>(ikw<"FETCH">) +
               assign<&ast::JoinStep::path>(ref<PathExprMulti>)) |
              (assign<&ast::JoinStep::path>(ref<PathExprMulti>) +
               option(ikw<"AS">) +
               assign<&ast::JoinStep::alias>(ref<IDENTIFICATION_VARIABLE>)))> {
  };
  struct IdVarDecl
      : rule<assign<&ast::IdVarDecl::range>(ref<RangeVarDecl>) +
             many(append<&ast::IdVarDecl::joins>(ref<Join>))> {};
  struct AliasedPathDecl
      : rule<assign<&ast::AliasedPathDecl::path>(ref<PathExprMulti>) +
             option(ikw<"AS">) +
             assign<&ast::AliasedPathDecl::alias>(ref<IDENTIFICATION_VARIABLE>)> {
  };
  struct CollectionMemberDecl
      : rule<ikw<"IN"> + lit<"("> +
             assign<&ast::CollectionMemberDecl::path>(ref<PathExprMulti>) +
             lit<")"> + option(ikw<"AS">) +
             assign<&ast::CollectionMemberDecl::alias>(
                 ref<IDENTIFICATION_VARIABLE>)> {};

  struct SimpleSelectExpression
      : rule<ref<PathExprRule> | ref<AggregateExpr>> {};
  struct ConstructorExpr
      : rule<ikw<"NEW"> +
             assign<&ast::ConstructorExpr::className>(ref<PathExprRule>) +
             lit<"("> +
             append<&ast::ConstructorExpr::args>(
                 ref<SimpleSelectExpression>) +
             many(lit<","> +
                  append<&ast::ConstructorExpr::args>(
                      ref<SimpleSelectExpression>)) +
             lit<")">> {};
  struct SelectExpression
      : rule<ref<PathExprRule> | ref<AggregateExpr> | ref<ObjectExpr> |
             ref<ConstructorExpr>> {};

  struct Subquery
      : rule<ikw<"SELECT"> +
             option(flag<&ast::Subquery::distinct>(ikw<"DISTINCT">)) +
             assign<&ast::Subquery::column>(ref<SimpleSelectExpression>) +
             ikw<"FROM"> +
             append<&ast::Subquery::from>(
                 ref<IdVarDecl> | ref<AliasedPathDecl> |
                 ref<CollectionMemberDecl>) +
             many(lit<","> +
                  append<&ast::Subquery::from>(
                      ref<IdVarDecl> | ref<AliasedPathDecl> |
                      ref<CollectionMemberDecl>)) +
             option(ikw<"WHERE"> + assign<&ast::Subquery::where>(ref<Expression>)) +
             option(ikw<"GROUP"> + ikw<"BY"> +
                    some(append<&ast::Subquery::groupBy>(ref<PathExprRule>), lit<",">)) +
             option(ikw<"HAVING"> +
                    assign<&ast::Subquery::having>(ref<Expression>))> {};

  struct SubqueryExpr
      : rule<lit<"("> + assign<&ast::SubqueryExpr::query>(ref<Subquery>) +
             lit<")">> {};
  struct AllAnyExpr
      : rule<assign<&ast::AllAnyExpr::kind>(
                 as<ast::AllAnyKind::all>(ikw<"ALL">) | as<ast::AllAnyKind::any>(ikw<"ANY">) | as<ast::AllAnyKind::some>(ikw<"SOME">)) +
             lit<"("> + assign<&ast::AllAnyExpr::query>(ref<Subquery>) +
             lit<")">> {};
  struct ExistsExpr
      : rule<option(flag<&ast::ExistsExpr::negated>(ikw<"NOT">)) +
             ikw<"EXISTS"> + lit<"("> +
             assign<&ast::ExistsExpr::query>(ref<Subquery>) + lit<")">> {};

  // -- expression ladder, tightest to loosest ----
  struct ExprBase
      : rule<ref<NumberLit> | ref<StringLit> | ref<BoolLit> |
             ref<NullLit> | ref<InputParam> | ref<ExistsExpr> |
             ref<AggregateExpr> | ref<Func1Expr> | ref<ModExpr> |
             ref<ConcatExpr> | ref<LocateExpr> |
             ref<SubstringExpr> | ref<TrimExpr> |
             ref<CurrentValueExpr> | ref<SubqueryExpr> |
             ref<PathExprRule> | (lit<"("> + ref<Expression> + lit<")">)> {};

  struct ExprUnary
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-">) +
                 assign<&ast::UnaryExpr::operand>(ref<ExprUnary>)) |
             ref<ExprBase>> {};

  struct ExprMul
      : rule<ref<ExprUnary> +
             many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 assign<&ast::BinaryExpr::op>(as<ast::Operator::multiply>(lit<"*">) | as<ast::Operator::divide>(lit<"/">)) +
                 assign<&ast::BinaryExpr::right>(ref<ExprUnary>)))> {};

  struct ExprAdd
      : rule<ref<ExprMul> +
             many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 assign<&ast::BinaryExpr::op>(as<ast::Operator::add>(lit<"+">) | as<ast::Operator::subtract>(lit<"-">)) +
                 assign<&ast::BinaryExpr::right>(ref<ExprMul>)))> {};


  // Comparison, BETWEEN, LIKE, IN, IS [NOT] NULL/EMPTY, MEMBER OF: one
  // shape regardless of operand type (string/boolean/enum/datetime/entity/
  // arithmetic) -- the distinction is semantic, not syntactic, the same
  // reasoning that already folds every path-expression flavor into
  // PathExpr.
  struct ExprPredicate
      : rule<ref<ExprAdd> +
             option(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                      assign<&ast::BinaryExpr::op>(
                          as<ast::Operator::le>(lit<"<=">) | as<ast::Operator::ge>(lit<">=">) | as<ast::Operator::ne>(lit<"<>">) | as<ast::Operator::lt>(lit<"<">) |
                          as<ast::Operator::gt>(lit<">">) | as<ast::Operator::eq>(lit<"=">)) +
                      assign<&ast::BinaryExpr::right>(ref<ExprAdd> |
                                                      ref<AllAnyExpr>)) |
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
                      assign<&ast::LikeExpr::pattern>(ref<PatternValue>) +
                      option(ikw<"ESCAPE"> +
                             assign<&ast::LikeExpr::escape>(
                                 ref<CharLit> | ref<InputParam>))) |
                  nest<ast::InExpr, &ast::InExpr::operand>(
                      option(flag<&ast::InExpr::negated>(ikw<"NOT">)) +
                      ikw<"IN"> + lit<"("> +
                      ((some(append<&ast::InExpr::items>(ref<InItem>), lit<",">)) |
                       assign<&ast::InExpr::subquery>(ref<Subquery>)) +
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

  struct Expression
      : infix<ExprNot, ast::BinaryExpr,
              levels(left(as<ast::Operator::logical_and>(ikw<"AND">)), left(as<ast::Operator::logical_or>(ikw<"OR">)))> {};

  struct OrderByItem
      : rule<assign<&ast::OrderByItem::key>(ref<PathExprRule>) +
             option(flag<&ast::OrderByItem::descending>(ikw<"DESC">) |
                    ikw<"ASC">)> {};

  // -- statements ----
  struct SelectStatement
      : rule<ikw<"SELECT"> +
             option(flag<&ast::SelectStatement::distinct>(
                 ikw<"DISTINCT">)) +
             append<&ast::SelectStatement::columns>(ref<SelectExpression>) +
             many(lit<","> +
                  append<&ast::SelectStatement::columns>(
                      ref<SelectExpression>)) +
             ikw<"FROM"> +
             append<&ast::SelectStatement::from>(ref<IdVarDecl>) +
             many(lit<","> +
                  (append<&ast::SelectStatement::from>(ref<IdVarDecl>) |
                   append<&ast::SelectStatement::from>(
                       ref<CollectionMemberDecl>))) +
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
                        ref<OrderByItem>) +
                    many(lit<","> +
                         append<&ast::SelectStatement::orderBy>(
                             ref<OrderByItem>)))> {};

  struct UpdateItem
      : rule<assign<&ast::UpdateItem::target>(ref<PathExprRule>) + lit<"="> +
             assign<&ast::UpdateItem::value>(ref<Expression>)> {};
  struct UpdateStatement
      : rule<ikw<"UPDATE"> +
             assign<&ast::UpdateStatement::schemaName>(
                 ref<IDENTIFICATION_VARIABLE>) +
             option(option(ikw<"AS">) +
                    assign<&ast::UpdateStatement::alias>(
                        ref<IDENTIFICATION_VARIABLE>)) +
             ikw<"SET"> +
             some(append<&ast::UpdateStatement::items>(ref<UpdateItem>), lit<",">) +
             option(ikw<"WHERE"> +
                    assign<&ast::UpdateStatement::where>(ref<Expression>))> {};

  struct DeleteStatement
      : rule<ikw<"DELETE"> + ikw<"FROM"> +
             assign<&ast::DeleteStatement::schemaName>(
                 ref<IDENTIFICATION_VARIABLE>) +
             option(option(ikw<"AS">) +
                    assign<&ast::DeleteStatement::alias>(
                        ref<IDENTIFICATION_VARIABLE>)) +
             option(ikw<"WHERE"> +
                    assign<&ast::DeleteStatement::where>(ref<Expression>))> {};

  struct Document
      : rule<assign<&ast::Document::statement>(
            ref<SelectStatement> | ref<UpdateStatement> |
            ref<DeleteStatement>)> {};

  using entry = Document;
  using ignored = WS;
};

}  // namespace jpa
