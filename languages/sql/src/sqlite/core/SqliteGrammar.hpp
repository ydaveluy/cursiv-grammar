// The sqlite language: its grammar.
#pragma once

#include <cstddef>
#include <cstdlib>
#include <memory_resource>
#include <span>
#include <string>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "sqlite/core/ast.hpp"

namespace sqlite {

// Quote-doubling text, shared by STRING_LITERAL ('') and the `"`/`` ` ``
// forms of IDENTIFIER; `[...]` never doubles.
template <char Close>
inline constexpr auto undouble_impl =
    [](std::string_view body, std::pmr::memory_resource* mem) {
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

// One terminal, four shapes: `"..."`, `` `...` ``, `[...]`, or a bare word.
// The value strips the delimiter; `[...]` and the bare word have nothing to
// undouble.
inline constexpr auto unquote_ident = [](std::string_view t,
                                         std::pmr::memory_resource* mem) {
  const char q = t.front();
  if (q == '"') return undouble_impl<'"'>(t.substr(1, t.size() - 2), mem);
  if (q == '`') return undouble_impl<'`'>(t.substr(1, t.size() - 2), mem);
  if (q == '[') return t.substr(1, t.size() - 2);
  return t;
};

inline constexpr auto unquote_blob = [](std::string_view t) {
  return t.substr(2, t.size() - 3);
};

struct Sqlite : cursiv::GrammarSpec {
  // -- forward declarations: the expression/select/DDL grammar is one large
  // mutually-recursive tangle (an expr can hold a subquery, a select can hold
  // a join, a join holds a table item that can hold a select). ----
  struct IDENTIFIER;
  struct NUMERIC_LITERAL;
  struct STRING_LITERAL;
  struct BLOB_LITERAL;
  struct BIND_PARAMETER;
  struct Name;
  struct WindowName;
  struct ColumnName;
  struct NumberLit;
  struct StringLit;
  struct BlobLit;
  struct NullLit;
  struct BoolLit;
  struct CurrentTimeLit;
  struct BindParam;
  struct LiteralValue;
  struct ColumnRef;
  struct QualifiedRef;
  struct TypeNameWord;
  struct TypeNameWords;
  struct SignedNumber;
  struct SignedLiteral;
  struct TypeName;
  struct NameLit;
  struct RaiseFunction;
  struct WhenClause;
  struct CaseExpr;
  struct CastExpr;
  struct RowExpr;
  struct ValueRow;
  struct FrameBound;
  struct FrameSpec;
  struct OrderTerm;
  struct WindowDefn;
  struct FuncCall;
  struct SubqueryExpr;
  struct ExprBase;
  struct ExprUnary;
  struct ExprCollate;
  struct ExprString;
  struct ExprMultiplication;
  struct ExprAddition;
  struct ExprBitwise;
  struct ExprComparison;
  struct ExprBinarySuffix;
  struct ExprNot;
  struct Expression;

  struct IndexedColumn;
  struct ForeignKeyAction;
  struct ForeignKeyClause;
  struct ColumnConstraint;
  struct ColumnDef;
  struct TableConstraint;

  struct CommonTableExpr;
  struct ResultColumn;
  struct TableItem;
  struct JoinStep;
  struct JoinClause;
  struct NamedWindow;
  struct SelectCore;
  struct FirstCompoundPart;
  struct CompoundPart;
  struct Select;

  struct QualifiedTableName;
  struct UpsertAssignment;
  struct UpsertClause;
  struct InsertStmt;
  struct UpdateStmt;
  struct DeleteStmt;
  struct PragmaValue;
  struct PragmaStmt;
  struct VacuumStmt;
  struct AttachStmt;
  struct DetachStmt;
  struct BeginStmt;
  struct CommitStmt;
  struct RollbackStmt;
  struct SavepointStmt;
  struct ReleaseStmt;
  struct AnalyzeStmt;
  struct ReindexStmt;
  struct DropStmt;
  struct CreateIndexStmt;
  struct CreateTableStmt;
  struct CreateTriggerStmt;
  struct CreateViewStmt;
  struct CreateVirtualTableStmt;
  struct AlterTableStmt;
  struct SqlStmt;
  struct Script;

  static constexpr auto IdStart = cr<R"(A-Za-z_\u{80}-\u{10FFFF})">;
  static constexpr auto IdContinue = cr<R"(A-Za-z_0-9$\u{80}-\u{10FFFF})">;
  static constexpr auto word_boundary = IdContinue;

  struct WS : terminal<some(s | lit<"\xef\xbb\xbf">)> {};
  struct LINE_COMMENT : terminal<lit<"--"> + many(cr<R"(^\n)">)> {};
  struct BLOCK_COMMENT : terminal<through(lit<"/*">, lit<"*/"> | eof)> {};

  // parse.y: keywords without ID fallback; INDEXED/JOIN_KW and the three
  // contextual window words remain valid bare names.
  static constexpr auto ReservedName =
      ikw<"ADD"> | ikw<"ALL"> | ikw<"ALTER"> | ikw<"AND"> |
      ikw<"AS"> | ikw<"AUTOINCREMENT"> | ikw<"BETWEEN"> | ikw<"CASE"> |
      ikw<"CHECK"> | ikw<"COLLATE"> | ikw<"COMMIT"> | ikw<"CONSTRAINT"> |
      ikw<"CREATE"> | ikw<"DEFAULT"> | ikw<"DEFERRABLE"> | ikw<"DELETE"> |
      ikw<"DISTINCT"> | ikw<"DROP"> | ikw<"ELSE"> | ikw<"ESCAPE"> |
      ikw<"EXCEPT"> | ikw<"EXISTS"> | ikw<"FOREIGN"> | ikw<"FROM"> |
      ikw<"GROUP"> | ikw<"HAVING"> | ikw<"IN"> | ikw<"INDEX"> |
      ikw<"INSERT"> | ikw<"INTERSECT"> | ikw<"INTO"> | ikw<"IS"> |
      ikw<"ISNULL"> | ikw<"JOIN"> | ikw<"LIMIT"> | ikw<"NOT"> |
      ikw<"NOTHING"> | ikw<"NOTNULL"> | ikw<"NULL"> | ikw<"ON"> |
      ikw<"OR"> | ikw<"ORDER"> | ikw<"PRIMARY"> | ikw<"REFERENCES"> |
      ikw<"RETURNING"> | ikw<"SELECT"> | ikw<"SET"> | ikw<"TABLE"> |
      ikw<"THEN"> | ikw<"TO"> | ikw<"TRANSACTION"> | ikw<"UNION"> |
      ikw<"UNIQUE"> | ikw<"UPDATE"> | ikw<"USING"> | ikw<"VALUES"> |
      ikw<"WHEN"> | ikw<"WHERE">;

  static constexpr auto IdentBody =
      (lit<"\""> + many(lit<"\"\""> | cr<R"(^")">) + lit<"\"">) |
      (lit<"`"> + many(lit<"``"> | cr<"^`">) + lit<"`">) |
      (lit<"["> + many(cr<"^]">) + lit<"]">) | (IdStart + many(IdContinue));
  struct IDENTIFIER : terminal<as<unquote_ident>(!ReservedName + IdentBody)> {};

  static constexpr auto Digits = some(d) + many(lit<"_"> + some(d));
  static constexpr auto HexDigits =
      some(cr<"0-9A-Fa-f">) + many(lit<"_"> + some(cr<"0-9A-Fa-f">));
  struct NUMERIC_LITERAL
      : terminal<as<std::string_view>(
            ((((Digits + option(lit<"."> + option(Digits))) |
               (lit<"."> + Digits)) +
              option((lit<"e"> | lit<"E">) + option(cr<"+-">) + Digits)) |
             ((lit<"0x"> | lit<"0X">) + HexDigits)) + !IdContinue)> {};

  static constexpr auto StringBody =
      lit<"'"> + many(lit<"''"> | cr<"^'">) + lit<"'">;
  struct STRING_LITERAL : terminal<as<unquote_string>(StringBody)> {};
  struct BLOB_LITERAL
      : terminal<as<unquote_blob>((lit<"x"> | lit<"X">)+lit<"'"> +
                                  many(lit<"''"> | cr<"^'">) + lit<"'">)> {};
  struct BIND_PARAMETER
      : terminal<(lit<"?"> + many(d)) |
                 (cr<":@$#"> + many(IdContinue | lit<"::">) +
                  option(lit<"("> + many(cr<R"(^) \t\r\n\f)">) + lit<")">))> {};

  struct Name : rule<ref<IDENTIFIER> | ref<STRING_LITERAL>> {};
  struct WindowName : terminal<!ReservedName + (IdentBody | StringBody)> {};
  struct ColumnName : rule<ref<Name>> {};
  // -- operator ladder (infix, tightest to loosest) ----

  static constexpr auto OpString = as<ast::Operator::concat>(lit<"||">) | as<ast::Operator::jptr2>(lit<"->>">) | as<ast::Operator::jptr>(lit<"->">);
  static constexpr auto OpMulLvl = as<ast::Operator::multiply>(lit<"*">) | as<ast::Operator::divide>(lit<"/">) | as<ast::Operator::modulo>(lit<"%">);
  static constexpr auto OpAddLvl = as<ast::Operator::add>(lit<"+">) | as<ast::Operator::subtract>(lit<"-">);
  static constexpr auto OpBitwise =
      as<ast::Operator::shl>(lit<"<<">) | as<ast::Operator::shr>(lit<">>">) | as<ast::Operator::bitand_>(lit<"&">) | as<ast::Operator::bitor_>(lit<"|">);
  static constexpr auto OpCompare =
      as<ast::Operator::le>(lit<"<=">) | as<ast::Operator::ge>(lit<">=">) | as<ast::Operator::lt>(lit<"<">) | as<ast::Operator::gt>(lit<">">);
  static constexpr auto OpEquality =
      as<ast::Operator::eq>(lit<"==">) | as<ast::Operator::eq>(lit<"=">) | as<ast::Operator::ne>(lit<"!=">) | as<ast::Operator::ne>(lit<"<>">);

  struct NumberLit
      : rule<assign<&ast::NumberLit::value>(ref<NUMERIC_LITERAL>)> {};
  struct StringLit : rule<assign<&ast::StringLit::value>(ref<STRING_LITERAL>)> {
  };
  struct BlobLit : rule<assign<&ast::BlobLit::value>(ref<BLOB_LITERAL>)> {};
  struct NullLit : rule<create<ast::NullLit> + ikw<"NULL">> {};
  struct BoolLit
      : rule<create<ast::BoolLit> +
             (flag<&ast::BoolLit::value>(ikw<"TRUE">) | ikw<"FALSE">)> {};
  struct CurrentTimeLit : rule<assign<&ast::CurrentValueLit::kind>(
                              as<ast::CurrentValueKind::time>(ikw<"CURRENT_TIME">) | as<ast::CurrentValueKind::date>(ikw<"CURRENT_DATE">) |
                              as<ast::CurrentValueKind::timestamp>(ikw<"CURRENT_TIMESTAMP">))> {};
  struct BindParam
      : rule<assign<&ast::BindParam::text>(ref<BIND_PARAMETER>)> {};

  struct LiteralValue
      : rule<ref<NumberLit> | ref<StringLit> | ref<BlobLit> | ref<NullLit> |
             ref<BoolLit> | ref<CurrentTimeLit>> {};

  struct ColumnRef : rule<assign<&ast::ColumnRef::name>(ref<IDENTIFIER>)> {};
  struct QualifiedRef
      : rule<option(assign<&ast::QualifiedRef::schema>(ref<Name>) + lit<".">) +
             assign<&ast::QualifiedRef::table>(ref<Name>) + lit<"."> +
             assign<&ast::QualifiedRef::column>(ref<Name>)> {};

  struct TypeNameWord : rule<ref<ColumnName>> {};
  struct TypeNameWords : rule<some(ref<TypeNameWord>)> {};
  struct SignedNumber
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-">) +
                 assign<&ast::UnaryExpr::operand>(ref<NumberLit>)) |
             ref<NumberLit>> {};
  struct SignedLiteral
      : rule<create<ast::UnaryExpr>(
            assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-">) +
            assign<&ast::UnaryExpr::operand>(ref<LiteralValue>))> {};
  struct TypeName
      : rule<assign<&ast::TypeName::name>(ref<TypeNameWords>) +
             option(lit<"("> +
                    assign<&ast::TypeName::param1>(ref<SignedNumber>) +
                    option(lit<","> +
                           assign<&ast::TypeName::param2>(ref<SignedNumber>)) +
                    lit<")">)> {};

  struct NameLit : rule<assign<&ast::NameLit::value>(ref<ColumnName>)> {};

  struct RaiseFunction
      : rule<ikw<"RAISE"> + lit<"("> +
             (assign<&ast::RaiseExpr::kind>(as<ast::RaiseKind::ignore>(ikw<"IGNORE">)) |
              (assign<&ast::RaiseExpr::kind>(as<ast::RaiseKind::rollback>(ikw<"ROLLBACK">) |
                                             as<ast::RaiseKind::abort>(ikw<"ABORT">) | as<ast::RaiseKind::fail>(ikw<"FAIL">)) +
               lit<","> + assign<&ast::RaiseExpr::message>(ref<Expression>))) +
             lit<")">> {};

  struct WhenClause
      : rule<ikw<"WHEN"> + assign<&ast::WhenClause::when>(ref<Expression>) +
             ikw<"THEN"> + assign<&ast::WhenClause::then>(ref<Expression>)> {};
  struct CaseExpr
      : rule<ikw<"CASE"> +
             option(assign<&ast::CaseExpr::operand>(ref<Expression>)) +
             some(append<&ast::CaseExpr::whens>(ref<WhenClause>)) +
             option(ikw<"ELSE"> +
                    assign<&ast::CaseExpr::elseValue>(ref<Expression>)) +
             ikw<"END">> {};

  struct CastExpr
      : rule<ikw<"CAST"> + lit<"("> +
             assign<&ast::CastExpr::operand>(ref<Expression>) + ikw<"AS"> +
             option(assign<&ast::CastExpr::type>(ref<TypeName>)) +
             lit<")">> {};

  // Two or more items: a single-item parenthesized expression is plain
  // grouping (ExprBase), not a row value.
  struct RowExpr
      : rule<lit<"("> + append<&ast::RowExpr::items>(ref<Expression>) +
             some(lit<","> + append<&ast::RowExpr::items>(ref<Expression>)) +
             lit<")">> {};
  // One or more items: `VALUES (1)` is a single-column row.
  struct ValueRow
      : rule<lit<"("> + some(append<&ast::RowExpr::items>(ref<Expression>), lit<",">) +
             lit<")">> {};


  struct FrameBound
      : rule<assign<&ast::FrameBound::kind>(as<ast::FrameBoundKind::unbounded_preceding>(ikw<"UNBOUNDED"> + ikw<"PRECEDING">) |
                                            as<ast::FrameBoundKind::unbounded_following>(ikw<"UNBOUNDED"> + ikw<"FOLLOWING">) |
                                            as<ast::FrameBoundKind::current_row>(ikw<"CURRENT"> + ikw<"ROW">)) |
             (assign<&ast::FrameBound::offset>(ref<Expression>) +
              assign<&ast::FrameBound::kind>(as<ast::FrameBoundKind::preceding>(ikw<"PRECEDING">) |
                                             as<ast::FrameBoundKind::following>(ikw<"FOLLOWING">)))> {};


  struct FrameSpec
      : rule<assign<&ast::FrameSpec::type>(as<ast::FrameType::range>(ikw<"RANGE">) | as<ast::FrameType::rows>(ikw<"ROWS">) |
                                           as<ast::FrameType::groups>(ikw<"GROUPS">)) +
             (assign<&ast::FrameSpec::start>(ref<FrameBound>) |
              (ikw<"BETWEEN"> +
               assign<&ast::FrameSpec::start>(ref<FrameBound>) +
               ikw<"AND"> +
               assign<&ast::FrameSpec::end>(ref<FrameBound>))) +
             option(ikw<"EXCLUDE"> +
                    assign<&ast::FrameSpec::exclude>(
                        as<ast::FrameExclude::no_others>(ikw<"NO"> + ikw<"OTHERS">) | as<ast::FrameExclude::current_row>(ikw<"CURRENT"> + ikw<"ROW">) |
                        as<ast::FrameExclude::group>(ikw<"GROUP">) | as<ast::FrameExclude::ties>(ikw<"TIES">)))> {};

  struct OrderTerm
      : rule<assign<&ast::OrderTerm::key>(ref<Expression>) +
             option(flag<&ast::OrderTerm::descending>(ikw<"DESC">) |
                    ikw<"ASC">) +
             option(ikw<"NULLS"> +
                    assign<&ast::OrderTerm::nullsLast>(as<true>(ikw<"LAST">) |
                                                       as<false>(ikw<"FIRST">)))> {};

  struct WindowDefn
      : rule<lit<"("> +
             option(assign<&ast::WindowDefn::baseWindow>(ref<WindowName>)) +
             option(ikw<"PARTITION"> + ikw<"BY"> +
                    some(append<&ast::WindowDefn::partitionBy>(ref<Expression>),
                         lit<",">)) +
             option(ikw<"ORDER"> + ikw<"BY"> +
                    some(append<&ast::WindowDefn::orderBy>(ref<OrderTerm>),
                         lit<",">)) +
             option(assign<&ast::WindowDefn::frame>(ref<FrameSpec>)) +
             lit<")">> {};

  struct FuncCall
      : rule<
            assign<&ast::FuncCall::name>(ref<Name>) + lit<"("> +
            option(
                flag<&ast::FuncCall::star>(lit<"*">) |
                (option(flag<&ast::FuncCall::distinct>(ikw<"DISTINCT">) |
                        ikw<"ALL">) +
                 some(append<&ast::FuncCall::args>(ref<Expression>), lit<",">) +
                 option(
                     ikw<"ORDER"> + ikw<"BY"> +
                     some(append<&ast::FuncCall::orderBy>(ref<OrderTerm>),
                          lit<",">)))) +
            lit<")"> +
            option(ikw<"WITHIN"> + ikw<"GROUP"> + lit<"("> + ikw<"ORDER"> +
                   ikw<"BY"> +
                   assign<&ast::FuncCall::percentileOrderBy>(ref<Expression>) +
                   lit<")">) +
            option(ikw<"FILTER"> + lit<"("> + ikw<"WHERE"> +
                   assign<&ast::FuncCall::filter>(ref<Expression>) + lit<")">) +
            option(ikw<"OVER"> +
                   (assign<&ast::FuncCall::overName>(ref<WindowName>) |
                    assign<&ast::FuncCall::overDefn>(ref<WindowDefn>)))> {};

  struct SubqueryExpr
      : rule<option(flag<&ast::SubqueryExpr::isExists>(
                 option(flag<&ast::SubqueryExpr::negated>(ikw<"NOT">)) +
                 ikw<"EXISTS">)) +
             lit<"("> + assign<&ast::SubqueryExpr::query>(ref<Select>) +
             lit<")">> {};

  struct ExprBase : rule<ref<LiteralValue>         //
                         | ref<BindParam>      //
                         | ref<QualifiedRef>       //
                         | ref<ColumnRef>          //
                         | ref<SubqueryExpr>   //
                         | ref<RaiseFunction>  //
                         | ref<FuncCall>       //
                         | ref<RowExpr>        //
                         | ref<CastExpr>       //
                         | ref<CaseExpr>       //
                         | (lit<"("> + ref<Expression> + lit<")">)> {};

  struct ExprUnary
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ikw<"NOT">) +
                 assign<&ast::UnaryExpr::operand>(ref<ExprBinarySuffix>)) |
             create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"-"> | lit<"+"> | lit<"~">) +
                 assign<&ast::UnaryExpr::operand>(ref<ExprUnary>)) |
             ref<ExprBase>> {};

  struct ExprCollate
      : rule<ref<ExprUnary> +
             many(nest<ast::CollateExpr, &ast::CollateExpr::operand>(
                 ikw<"COLLATE"> +
                 assign<&ast::CollateExpr::collation>(ref<Name>)))> {};

  struct ExprString
      : rule<ref<ExprCollate> +
             many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 assign<&ast::BinaryExpr::op>(OpString) +
                 assign<&ast::BinaryExpr::right>(ref<ExprCollate>)))> {};

  struct ExprMultiplication
      : rule<ref<ExprString> +
             many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 assign<&ast::BinaryExpr::op>(OpMulLvl) +
                 assign<&ast::BinaryExpr::right>(ref<ExprString>)))> {};

  struct ExprAddition
      : rule<ref<ExprMultiplication> +
             many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 assign<&ast::BinaryExpr::op>(OpAddLvl) +
                 assign<&ast::BinaryExpr::right>(ref<ExprMultiplication>)))> {};

  struct ExprBitwise
      : rule<ref<ExprAddition> +
             many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 assign<&ast::BinaryExpr::op>(OpBitwise) +
                 assign<&ast::BinaryExpr::right>(ref<ExprAddition>)))> {};

  struct ExprComparison
      : rule<ref<ExprBitwise> +
             many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 assign<&ast::BinaryExpr::op>(OpCompare) +
                 assign<&ast::BinaryExpr::right>(ref<ExprBitwise>)))> {};

  static constexpr auto InTargetSubquery =
      lit<"("> + assign<&ast::InExpr::subquery>(ref<Select>) + lit<")">;
  static constexpr auto InTargetList =
      lit<"("> +
      option(some(append<&ast::InExpr::list>(ref<Expression>), lit<",">)) +
      lit<")">;
  static constexpr auto InTargetTableFunction =
      option(assign<&ast::InExpr::schema>(ref<Name>) + lit<".">) +
      assign<&ast::InExpr::table>(ref<Name>) +
      flag<&ast::InExpr::isTableFunction>(lit<"(">) +
      option(
          some(append<&ast::InExpr::tableArgs>(ref<Expression>), lit<",">)) +
      lit<")">;
  static constexpr auto InTargetTable =
      option(assign<&ast::InExpr::schema>(ref<Name>) + lit<".">) +
      assign<&ast::InExpr::table>(ref<Name>);


  struct ExprBinarySuffix
      : rule<ref<ExprComparison> +
             many(
                 nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                     assign<&ast::BinaryExpr::op>(OpEquality) +
                     assign<&ast::BinaryExpr::right>(ref<ExprComparison>)) |
                 nest<ast::IsExpr, &ast::IsExpr::left>(
                     ikw<"IS"> +
                     option(flag<&ast::IsExpr::negated>(ikw<"NOT">)) +
                     option(flag<&ast::IsExpr::distinct>(ikw<"DISTINCT"> +
                                                              ikw<"FROM">)) +
                     assign<&ast::IsExpr::right>(ref<ExprComparison>)) |
                 nest<ast::BetweenExpr, &ast::BetweenExpr::operand>(
                     option(flag<&ast::BetweenExpr::negated>(ikw<"NOT">)) +
                     ikw<"BETWEEN"> +
                     assign<&ast::BetweenExpr::low>(ref<ExprBinarySuffix>) +
                     ikw<"AND"> +
                     assign<&ast::BetweenExpr::high>(ref<ExprComparison>)) |
                 nest<ast::InExpr, &ast::InExpr::operand>(
                     option(flag<&ast::InExpr::negated>(ikw<"NOT">)) +
                     ikw<"IN"> +
                     (InTargetSubquery | InTargetList | InTargetTableFunction |
                      InTargetTable)) |
                 nest<ast::LikeExpr, &ast::LikeExpr::operand>(
                     option(flag<&ast::LikeExpr::negated>(ikw<"NOT">)) +
                     assign<&ast::LikeExpr::kind>(as<ast::LikeKind::like>(ikw<"LIKE">) | as<ast::LikeKind::glob>(ikw<"GLOB">) |
                                                  as<ast::LikeKind::regexp>(ikw<"REGEXP">) |
                                                  as<ast::LikeKind::match>(ikw<"MATCH">)) +
                     assign<&ast::LikeExpr::pattern>(ref<ExprComparison>) +
                     option(ikw<"ESCAPE"> + assign<&ast::LikeExpr::escape>(
                                                ref<ExprComparison>))) |
                 nest<ast::NullTest, &ast::NullTest::operand>(
                     ikw<"ISNULL"> |
                     flag<&ast::NullTest::negated>(
                         ikw<"NOTNULL"> | (ikw<"NOT"> + ikw<"NULL">))))> {};

  struct ExprNot : rule<ref<ExprBinarySuffix>> {};


  struct Expression : infix<ExprNot, ast::BinaryExpr,
                            levels(left(as<ast::Operator::logical_and>(ikw<"AND">)), left(as<ast::Operator::logical_or>(ikw<"OR">)))> {};

  // -- shared DDL fragments ----
  static constexpr auto ConflictActionAlt =
      as<ast::ConflictAction::rollback>(ikw<"ROLLBACK">) | as<ast::ConflictAction::abort>(ikw<"ABORT">) | as<ast::ConflictAction::fail>(ikw<"FAIL">) |
      as<ast::ConflictAction::ignore>(ikw<"IGNORE">) | as<ast::ConflictAction::replace>(ikw<"REPLACE">);

  struct IndexedColumn
      : rule<assign<&ast::IndexedColumn::expr>(ref<Expression>) +
             option(flag<&ast::IndexedColumn::descending>(ikw<"DESC">) |
                    ikw<"ASC">)> {};

  struct ForeignKeyAction
      : rule<(assign<&ast::ForeignKeyAction::kind>(as<ast::FkActionKind::on_delete>(ikw<"ON"> + ikw<"DELETE">) |
                                                   as<ast::FkActionKind::on_update>(ikw<"ON"> + ikw<"UPDATE">) |
                                                   as<ast::FkActionKind::on_insert>(ikw<"ON"> + ikw<"INSERT">)) +
              assign<&ast::ForeignKeyAction::refAction>(
                  as<ast::FkRefAction::set_null>(ikw<"SET"> + ikw<"NULL">) | as<ast::FkRefAction::set_default>(ikw<"SET"> + ikw<"DEFAULT">) | as<ast::FkRefAction::cascade>(ikw<"CASCADE">) |
                  as<ast::FkRefAction::restrict>(ikw<"RESTRICT">) | as<ast::FkRefAction::no_action>(ikw<"NO"> + ikw<"ACTION">))) |
             (assign<&ast::ForeignKeyAction::kind>(as<ast::FkActionKind::match>(ikw<"MATCH">)) +
              assign<&ast::ForeignKeyAction::matchName>(ref<Name>))> {};

  struct ForeignKeyClause
      : rule<ikw<"REFERENCES"> +
             assign<&ast::ForeignKeyClause::table>(ref<Name>) +
             option(lit<"("> +
                    append<&ast::ForeignKeyClause::columns>(ref<ColumnName>) +
                    many(lit<","> + append<&ast::ForeignKeyClause::columns>(
                                        ref<ColumnName>)) +
                    lit<")">) +
             many(append<&ast::ForeignKeyClause::actions>(
                 ref<ForeignKeyAction>)) +
             option(assign<&ast::ForeignKeyClause::deferrable>(
                        as<ast::Deferrable::deferrable>(ikw<"DEFERRABLE">) | as<ast::Deferrable::not_deferrable>(ikw<"NOT"> + ikw<"DEFERRABLE">)) +
                    option(assign<&ast::ForeignKeyClause::initially>(
                        as<ast::InitiallyMode::deferred>(ikw<"INITIALLY"> + ikw<"DEFERRED">) |
                        as<ast::InitiallyMode::immediate>(ikw<"INITIALLY"> + ikw<"IMMEDIATE">))))> {};

  // `foreign_key` is ColumnConstraintKind's default (index 0): the branch
  // below has no leading keyword of its own to tag with `as<>`.

  struct ColumnConstraint
      : rule<(option(ikw<"CONSTRAINT"> +
                     assign<&ast::ColumnConstraint::name>(ref<Name>)) +
              ((assign<&ast::ColumnConstraint::kind>(as<ast::ColumnConstraintKind::primary_key>(ikw<"PRIMARY"> + ikw<"KEY">)) +
                option(
                    flag<&ast::ColumnConstraint::descending>(ikw<"DESC">) |
                    ikw<"ASC">) +
                option(ikw<"ON"> + ikw<"CONFLICT"> +
                       assign<&ast::ColumnConstraint::conflict>(
                           ConflictActionAlt)) +
                option(flag<&ast::ColumnConstraint::autoincrement>(
                    ikw<"AUTOINCREMENT">)))  //
               |
               (option(flag<&ast::ColumnConstraint::notNull>(ikw<"NOT">)) +
                assign<&ast::ColumnConstraint::kind>(as<ast::ColumnConstraintKind::nullability>(ikw<"NULL">)) +
                option(ikw<"ON"> + ikw<"CONFLICT"> +
                       assign<&ast::ColumnConstraint::conflict>(
                           ConflictActionAlt)))  //
               | (assign<&ast::ColumnConstraint::kind>(as<ast::ColumnConstraintKind::unique>(ikw<"UNIQUE">)) +
                  option(ikw<"ON"> + ikw<"CONFLICT"> +
                         assign<&ast::ColumnConstraint::conflict>(
                             ConflictActionAlt)))  //
               | (assign<&ast::ColumnConstraint::kind>(as<ast::ColumnConstraintKind::check>(ikw<"CHECK">)) +
                  lit<"("> +
                  assign<&ast::ColumnConstraint::expr>(ref<Expression>) +
                  lit<")">)  //
               | (assign<&ast::ColumnConstraint::kind>(as<ast::ColumnConstraintKind::default_>(ikw<"DEFAULT">)) +
                  (assign<&ast::ColumnConstraint::expr>(ref<SignedNumber>) |
                   assign<&ast::ColumnConstraint::expr>(ref<LiteralValue>) |
                   assign<&ast::ColumnConstraint::expr>(ref<NameLit>) |
                   assign<&ast::ColumnConstraint::expr>(ref<SignedLiteral>) |
                   (lit<"("> +
                    assign<&ast::ColumnConstraint::expr>(ref<Expression>) +
                    lit<")">)))  //
               | (assign<&ast::ColumnConstraint::kind>(as<ast::ColumnConstraintKind::collate>(ikw<"COLLATE">)) +
                  assign<&ast::ColumnConstraint::collation>(ref<Name>))  //
               | (assign<&ast::ColumnConstraint::foreignKey>(
                     ref<ForeignKeyClause>))  //
               | (option(flag<&ast::ColumnConstraint::generatedAlways>(
                      ikw<"GENERATED"> + ikw<"ALWAYS">)) +
                  assign<&ast::ColumnConstraint::kind>(as<ast::ColumnConstraintKind::generated>(ikw<"AS">)) +
                  lit<"("> +
                  assign<&ast::ColumnConstraint::expr>(ref<Expression>) +
                  lit<")"> +
                  option(assign<&ast::ColumnConstraint::stored>(
                      as<true>(ikw<"STORED">) | as<false>(ikw<"VIRTUAL">)))))) |
             (option(flag<&ast::ColumnConstraint::notDeferrable>(
                  ikw<"NOT">)) +
              assign<&ast::ColumnConstraint::kind>(as<ast::ColumnConstraintKind::deferral>(ikw<"DEFERRABLE">)) +
              option(assign<&ast::ColumnConstraint::initially>(
                  as<ast::InitiallyMode::deferred>(ikw<"INITIALLY"> + ikw<"DEFERRED">) | as<ast::InitiallyMode::immediate>(ikw<"INITIALLY"> + ikw<"IMMEDIATE">)))) |
             (assign<&ast::ColumnConstraint::kind>(as<ast::ColumnConstraintKind::name_only>(ikw<"CONSTRAINT">)) +
              assign<&ast::ColumnConstraint::name>(ref<Name>))> {};

  struct ColumnDef
      : rule<assign<&ast::ColumnDef::name>(ref<ColumnName>) +
             option(assign<&ast::ColumnDef::type>(ref<TypeName>)) +
             many(append<&ast::ColumnDef::constraints>(
                 ref<ColumnConstraint>))> {};

  // `foreign_key` is TableConstraintKind's default (index 0): same reason.
  struct TableConstraint
      : rule<(option(ikw<"CONSTRAINT"> +
                     assign<&ast::TableConstraint::name>(ref<Name>)) +
              ((assign<&ast::TableConstraint::kind>(as<ast::TableConstraintKind::primary_key>(ikw<"PRIMARY"> + ikw<"KEY">) |
                                                    as<ast::TableConstraintKind::unique>(ikw<"UNIQUE">)) +
                lit<"("> +
                append<&ast::TableConstraint::columns>(ref<IndexedColumn>) +
                many(lit<","> + append<&ast::TableConstraint::columns>(
                                    ref<IndexedColumn>)) +
                option(flag<&ast::TableConstraint::autoincrement>(
                    ikw<"AUTOINCREMENT">)) +
                lit<")"> +
                option(ikw<"ON"> + ikw<"CONFLICT"> +
                       assign<&ast::TableConstraint::conflict>(
                           ConflictActionAlt)))  //
               | (assign<&ast::TableConstraint::kind>(as<ast::TableConstraintKind::check>(ikw<"CHECK">)) +
                  lit<"("> +
                  assign<&ast::TableConstraint::checkExpr>(ref<Expression>) +
                  lit<")"> +
                  option(ikw<"ON"> + ikw<"CONFLICT"> +
                         assign<&ast::TableConstraint::conflict>(
                             ConflictActionAlt)))  //
               | (ikw<"FOREIGN"> + ikw<"KEY"> + lit<"("> +
                  append<&ast::TableConstraint::fkColumns>(ref<ColumnName>) +
                  many(lit<","> + append<&ast::TableConstraint::fkColumns>(
                                      ref<ColumnName>)) +
                  lit<")"> +
                  assign<&ast::TableConstraint::foreignKey>(
                      ref<ForeignKeyClause>)))) |
             (assign<&ast::TableConstraint::kind>(as<ast::TableConstraintKind::name_only>(ikw<"CONSTRAINT">)) +
              assign<&ast::TableConstraint::name>(ref<Name>))> {};

  // -- SELECT machinery ----
  struct CommonTableExpr
      : rule<assign<&ast::CommonTableExpr::name>(ref<Name>) +
             option(lit<"("> +
                    append<&ast::CommonTableExpr::columns>(ref<ColumnName>) +
                    many(lit<","> + append<&ast::CommonTableExpr::columns>(
                                        ref<ColumnName>)) +
                    lit<")">) +
             ikw<"AS"> +
             option(assign<&ast::CommonTableExpr::materialized>(
                 as<true>(ikw<"MATERIALIZED">) | as<false>(ikw<"NOT"> + ikw<"MATERIALIZED">))) +
             lit<"("> + assign<&ast::CommonTableExpr::query>(ref<Select>) +
             lit<")">> {};

  struct ResultColumn
      : rule<flag<&ast::ResultColumn::star>(lit<"*">)  //
             | (assign<&ast::ResultColumn::table>(ref<Name>) + lit<"."> +
                flag<&ast::ResultColumn::star>(lit<"*">))  //
             | (assign<&ast::ResultColumn::value>(ref<Expression>) +
                option(option(ikw<"AS">) +
                       assign<&ast::ResultColumn::alias>(ref<ColumnName>)))> {};

  struct TableItem
      : rule<(lit<"("> + assign<&ast::TableItem::join>(ref<JoinClause>) +
              lit<")"> + option(option(ikw<"AS">) +
                                assign<&ast::TableItem::alias>(ref<Name>)))  //
             | (lit<"("> + assign<&ast::TableItem::subquery>(ref<Select>) +
                lit<")"> +
                option(option(ikw<"AS">) +
                       assign<&ast::TableItem::alias>(ref<Name>)))  //
             | (option(assign<&ast::TableItem::schema>(ref<Name>) + lit<".">) +
                assign<&ast::TableItem::name>(ref<Name>) +
                ((flag<&ast::TableItem::isTableFunction>(lit<"(">) +
                  option(some(append<&ast::TableItem::args>(ref<Expression>), lit<",">)) +
                  lit<")"> +
                  option(option(ikw<"AS">) +
                         assign<&ast::TableItem::alias>(ref<Name>)))  //
                 | (option((ikw<"AS"> +
                            assign<&ast::TableItem::alias>(ref<Name>)) |
                           assign<&ast::TableItem::alias>(ref<Name>)) +
                    option((ikw<"INDEXED"> + ikw<"BY"> +
                            assign<&ast::TableItem::indexedBy>(ref<Name>)) |
                           flag<&ast::TableItem::notIndexed>(
                               ikw<"NOT"> + ikw<"INDEXED">)))))> {};

  struct JoinStep
      : rule<(flag<&ast::JoinStep::comma>(lit<",">) |
              (many(append<&ast::JoinStep::modifiers>(
                   ikw<"NATURAL"> | ikw<"LEFT"> | ikw<"RIGHT"> | ikw<"FULL"> |
                   ikw<"OUTER"> | ikw<"INNER"> | ikw<"CROSS">)) +
               ikw<"JOIN">)) +
             assign<&ast::JoinStep::right>(ref<TableItem>) +
             option((ikw<"ON"> +
                     assign<&ast::JoinStep::onExpr>(ref<Expression>))  //
                    | (ikw<"USING"> + lit<"("> +
                       append<&ast::JoinStep::usingColumns>(ref<ColumnName>) +
                       many(lit<","> + append<&ast::JoinStep::usingColumns>(
                                           ref<ColumnName>)) +
                       lit<")">))> {};

  struct JoinClause
      : rule<assign<&ast::JoinClause::first>(ref<TableItem>) +
             many(append<&ast::JoinClause::steps>(ref<JoinStep>))> {};

  struct NamedWindow
      : rule<assign<&ast::NamedWindow::name>(ref<WindowName>) + ikw<"AS"> +
             assign<&ast::NamedWindow::defn>(ref<WindowDefn>)> {};

  struct SelectCore
      : rule<(ikw<"SELECT"> +
              option(flag<&ast::SelectCore::distinct>(ikw<"DISTINCT">) |
                     ikw<"ALL">) +
              some(append<&ast::SelectCore::columns>(ref<ResultColumn>), lit<",">) +
              option(ikw<"FROM"> +
                     assign<&ast::SelectCore::from>(ref<JoinClause>)) +
              option(ikw<"WHERE"> +
                     assign<&ast::SelectCore::where>(ref<Expression>)) +
              option(ikw<"GROUP"> + ikw<"BY"> +
                     some(append<&ast::SelectCore::groupBy>(ref<Expression>), lit<",">)) +
              option(ikw<"HAVING"> +
                     assign<&ast::SelectCore::having>(ref<Expression>)) +
              option(ikw<"WINDOW"> +
                     append<&ast::SelectCore::windows>(ref<NamedWindow>) +
                     many(lit<","> + append<&ast::SelectCore::windows>(
                                         ref<NamedWindow>))))  //
             | (ikw<"VALUES"> +
                some(append<&ast::SelectCore::values>(ref<ValueRow>), lit<",">))> {};


  struct FirstCompoundPart
      : rule<assign<&ast::CompoundPart::core>(ref<SelectCore>)> {};
  struct CompoundPart
      : rule<assign<&ast::CompoundPart::op>(
                 as<ast::CompoundOp::union_all>(ikw<"UNION"> + ikw<"ALL">) | as<ast::CompoundOp::union_>(ikw<"UNION">) |
                 as<ast::CompoundOp::intersect>(ikw<"INTERSECT">) | as<ast::CompoundOp::except>(ikw<"EXCEPT">)) +
             assign<&ast::CompoundPart::core>(ref<SelectCore>)> {};

  struct Select
      : rule<option(
                 ikw<"WITH"> +
                 option(flag<&ast::Select::recursive>(ikw<"RECURSIVE">)) +
                 some(append<&ast::Select::ctes>(ref<CommonTableExpr>), lit<",">)) +
             append<&ast::Select::parts>(ref<FirstCompoundPart>) +
             many(append<&ast::Select::parts>(ref<CompoundPart>)) +
             option(ikw<"ORDER"> + ikw<"BY"> +
                    some(append<&ast::Select::orderBy>(ref<OrderTerm>), lit<",">)) +
             option(ikw<"LIMIT"> +
                    ((assign<&ast::Select::offset>(ref<Expression>) + lit<","> +
                      assign<&ast::Select::limit>(ref<Expression>)) |
                     (assign<&ast::Select::limit>(ref<Expression>) +
                      option(ikw<"OFFSET"> + assign<&ast::Select::offset>(
                                                 ref<Expression>)))))> {};

  // -- DML ----
  struct QualifiedTableName
      : rule<option(assign<&ast::QualifiedTableName::schema>(ref<Name>) +
                    lit<".">) +
             assign<&ast::QualifiedTableName::name>(ref<Name>) +
             option(ikw<"AS"> +
                    assign<&ast::QualifiedTableName::alias>(ref<ColumnName>)) +
             option((ikw<"INDEXED"> + ikw<"BY"> +
                     assign<&ast::QualifiedTableName::indexedBy>(ref<Name>)) |
                    flag<&ast::QualifiedTableName::notIndexed>(
                        ikw<"NOT"> + ikw<"INDEXED">))> {};

  struct UpsertAssignment
      : rule<(append<&ast::UpsertAssignment::columns>(ref<ColumnName>)  //
              | (lit<"("> +
                 append<&ast::UpsertAssignment::columns>(ref<ColumnName>) +
                 many(lit<","> + append<&ast::UpsertAssignment::columns>(
                                     ref<ColumnName>)) +
                 lit<")">)) +
             lit<"="> +
             assign<&ast::UpsertAssignment::value>(ref<Expression>)> {};

  struct UpsertClause
      : rule<ikw<"ON"> + ikw<"CONFLICT"> +
             option(
                 lit<"("> +
                 append<&ast::UpsertClause::target>(ref<IndexedColumn>) +
                 many(lit<","> + append<&ast::UpsertClause::target>(
                                     ref<IndexedColumn>)) +
                 lit<")"> +
                 option(ikw<"WHERE"> + assign<&ast::UpsertClause::targetWhere>(
                                           ref<Expression>))) +
             ikw<"DO"> +
             (flag<&ast::UpsertClause::doNothing>(ikw<"NOTHING">)  //
              | (ikw<"UPDATE"> + ikw<"SET"> +
                 append<&ast::UpsertClause::assignments>(
                     ref<UpsertAssignment>) +
                 many(lit<","> + append<&ast::UpsertClause::assignments>(
                                     ref<UpsertAssignment>)) +
                 option(ikw<"WHERE"> + assign<&ast::UpsertClause::updateWhere>(
                                           ref<Expression>))))> {};


  struct InsertStmt
      : rule<option(ikw<"WITH"> +
                    option(flag<&ast::InsertStmt::recursive>(
                        ikw<"RECURSIVE">)) +
                    append<&ast::InsertStmt::ctes>(ref<CommonTableExpr>) +
                    many(lit<","> + append<&ast::InsertStmt::ctes>(
                                        ref<CommonTableExpr>))) +
             assign<&ast::InsertStmt::mode>(
                 as<ast::InsertMode::insert_or_replace>(ikw<"INSERT"> + ikw<"OR"> + ikw<"REPLACE">) | as<ast::InsertMode::insert_or_rollback>(ikw<"INSERT"> + ikw<"OR"> + ikw<"ROLLBACK">) |
                 as<ast::InsertMode::insert_or_abort>(ikw<"INSERT"> + ikw<"OR"> + ikw<"ABORT">) | as<ast::InsertMode::insert_or_fail>(ikw<"INSERT"> + ikw<"OR"> + ikw<"FAIL">) |
                 as<ast::InsertMode::insert_or_ignore>(ikw<"INSERT"> + ikw<"OR"> + ikw<"IGNORE">) | as<ast::InsertMode::insert>(ikw<"INSERT">) |
                 as<ast::InsertMode::replace>(ikw<"REPLACE">)) +
             ikw<"INTO"> +
             option(assign<&ast::InsertStmt::schema>(ref<Name>) + lit<".">) +
             assign<&ast::InsertStmt::table>(ref<Name>) +
             option(ikw<"AS"> +
                    assign<&ast::InsertStmt::alias>(ref<ColumnName>)) +
             option(lit<"("> +
                    some(append<&ast::InsertStmt::columns>(ref<ColumnName>), lit<",">) +
                    lit<")">) +
             ((assign<&ast::InsertStmt::query>(ref<Select>) +
               many(append<&ast::InsertStmt::upserts>(
                   ref<UpsertClause>)))  //
              | flag<&ast::InsertStmt::useDefaultValues>(ikw<"DEFAULT"> +
                                                              ikw<"VALUES">)) +
             option(ikw<"RETURNING"> +
                    append<&ast::InsertStmt::returning>(ref<ResultColumn>) +
                    many(lit<","> + append<&ast::InsertStmt::returning>(
                                        ref<ResultColumn>)))> {};

  struct UpdateStmt
      : rule<option(ikw<"WITH"> +
                    option(flag<&ast::UpdateStmt::recursive>(
                        ikw<"RECURSIVE">)) +
                    append<&ast::UpdateStmt::ctes>(ref<CommonTableExpr>) +
                    many(lit<","> + append<&ast::UpdateStmt::ctes>(
                                        ref<CommonTableExpr>))) +
             ikw<"UPDATE"> +
             option(ikw<"OR"> +
                    assign<&ast::UpdateStmt::conflict>(ConflictActionAlt)) +
             assign<&ast::UpdateStmt::target>(ref<QualifiedTableName>) +
             ikw<"SET"> +
             append<&ast::UpdateStmt::assignments>(ref<UpsertAssignment>) +
             many(lit<","> + append<&ast::UpdateStmt::assignments>(
                                 ref<UpsertAssignment>)) +
             option(ikw<"FROM"> +
                    assign<&ast::UpdateStmt::from>(ref<JoinClause>)) +
             option(ikw<"WHERE"> +
                    assign<&ast::UpdateStmt::where>(ref<Expression>)) +
             option(ikw<"RETURNING"> +
                    append<&ast::UpdateStmt::returning>(ref<ResultColumn>) +
                    many(lit<","> + append<&ast::UpdateStmt::returning>(
                                        ref<ResultColumn>))) +
             option(ikw<"ORDER"> + ikw<"BY"> +
                    append<&ast::UpdateStmt::orderBy>(ref<OrderTerm>) +
                    many(lit<","> + append<&ast::UpdateStmt::orderBy>(
                                        ref<OrderTerm>))) +
             option(ikw<"LIMIT"> +
                    ((assign<&ast::UpdateStmt::offset>(ref<Expression>) +
                      lit<","> +
                      assign<&ast::UpdateStmt::limit>(ref<Expression>)) |
                     (assign<&ast::UpdateStmt::limit>(ref<Expression>) +
                      option(ikw<"OFFSET"> + assign<&ast::UpdateStmt::offset>(
                                                 ref<Expression>)))))> {};

  struct DeleteStmt
      : rule<option(ikw<"WITH"> +
                    option(flag<&ast::DeleteStmt::recursive>(
                        ikw<"RECURSIVE">)) +
                    append<&ast::DeleteStmt::ctes>(ref<CommonTableExpr>) +
                    many(lit<","> + append<&ast::DeleteStmt::ctes>(
                                        ref<CommonTableExpr>))) +
             ikw<"DELETE"> + ikw<"FROM"> +
             assign<&ast::DeleteStmt::target>(ref<QualifiedTableName>) +
             option(ikw<"WHERE"> +
                    assign<&ast::DeleteStmt::where>(ref<Expression>)) +
             option(ikw<"RETURNING"> +
                    append<&ast::DeleteStmt::returning>(ref<ResultColumn>) +
                    many(lit<","> + append<&ast::DeleteStmt::returning>(
                                        ref<ResultColumn>))) +
             option(ikw<"ORDER"> + ikw<"BY"> +
                    append<&ast::DeleteStmt::orderBy>(ref<OrderTerm>) +
                    many(lit<","> + append<&ast::DeleteStmt::orderBy>(
                                        ref<OrderTerm>))) +
             option(ikw<"LIMIT"> +
                    ((assign<&ast::DeleteStmt::offset>(ref<Expression>) +
                      lit<","> +
                      assign<&ast::DeleteStmt::limit>(ref<Expression>)) |
                     (assign<&ast::DeleteStmt::limit>(ref<Expression>) +
                      option(ikw<"OFFSET"> + assign<&ast::DeleteStmt::offset>(
                                                 ref<Expression>)))))> {};

  struct PragmaValue
      : rule<ref<SignedNumber> | ref<StringLit> | ref<NameLit> |
             create<ast::NameLit>(assign<&ast::NameLit::value>(
                 ikw<"ON"> | ikw<"DELETE"> | ikw<"DEFAULT">))> {};
  struct PragmaStmt
      : rule<ikw<"PRAGMA"> +
             option(assign<&ast::PragmaStmt::schema>(ref<Name>) + lit<".">) +
             assign<&ast::PragmaStmt::name>(ref<Name>) +
             option((lit<"="> +
                     assign<&ast::PragmaStmt::value>(ref<PragmaValue>)) |
                    (lit<"("> +
                     assign<&ast::PragmaStmt::value>(ref<PragmaValue>) +
                     lit<")">))> {};

  struct VacuumStmt
      : rule<ikw<"VACUUM"> +
             option(assign<&ast::VacuumStmt::schema>(ref<Name>)) +
             option(ikw<"INTO"> +
                    assign<&ast::VacuumStmt::intoFile>(ref<Expression>))> {};

  struct AttachStmt
      : rule<ikw<"ATTACH"> + option(ikw<"DATABASE">) +
             assign<&ast::AttachStmt::file>(ref<Expression>) + ikw<"AS"> +
             assign<&ast::AttachStmt::schemaExpr>(ref<Expression>) +
             option(ikw<"KEY"> + assign<&ast::AttachStmt::key>(ref<Expression>))> {};

  struct DetachStmt : rule<ikw<"DETACH"> + option(ikw<"DATABASE">) +
                               assign<&ast::DetachStmt::schemaExpr>(ref<Expression>)> {};

  struct BeginStmt
      : rule<ikw<"BEGIN"> +
             option(assign<&ast::BeginStmt::kind>(
                 as<ast::TxKind::deferred>(ikw<"DEFERRED">) | as<ast::TxKind::immediate>(ikw<"IMMEDIATE">) | as<ast::TxKind::exclusive>(ikw<"EXCLUSIVE">))) +
             option(ikw<"TRANSACTION"> +
                    option(assign<&ast::BeginStmt::name>(ref<Name>)))> {};

  struct CommitStmt
      : rule<(ikw<"COMMIT"> |
              flag<&ast::CommitStmt::usingEnd>(ikw<"END">)) +
             option(ikw<"TRANSACTION"> +
                    option(assign<&ast::CommitStmt::name>(ref<Name>)))> {};

  struct RollbackStmt
      : rule<ikw<"ROLLBACK"> +
             option(ikw<"TRANSACTION"> +
                    option(assign<&ast::RollbackStmt::name>(ref<Name>))) +
             option(ikw<"TO"> + option(ikw<"SAVEPOINT">) +
                    assign<&ast::RollbackStmt::savepoint>(ref<Name>))> {};

  struct SavepointStmt
      : rule<ikw<"SAVEPOINT"> + assign<&ast::SavepointStmt::name>(ref<Name>)> {
  };

  struct ReleaseStmt : rule<ikw<"RELEASE"> + option(ikw<"SAVEPOINT">) +
                                assign<&ast::ReleaseStmt::name>(ref<Name>)> {};

  struct AnalyzeStmt
      : rule<ikw<"ANALYZE"> +
             option(option(assign<&ast::AnalyzeStmt::schema>(ref<Name>) +
                           lit<".">) +
                    assign<&ast::AnalyzeStmt::name>(ref<Name>))> {};

  // A bare name is ambiguous between a collation and a table/index; both
  // fold into schema/name here.
  struct ReindexStmt
      : rule<ikw<"REINDEX"> +
             option(option(assign<&ast::ReindexStmt::schema>(ref<Name>) +
                           lit<".">) +
                    assign<&ast::ReindexStmt::name>(ref<Name>))> {};

  struct DropStmt
      : rule<ikw<"DROP"> +
             assign<&ast::DropStmt::object>(
                 as<ast::DropObjectKind::index>(ikw<"INDEX">) | as<ast::DropObjectKind::table>(ikw<"TABLE">) | as<ast::DropObjectKind::trigger>(ikw<"TRIGGER">) |
                 as<ast::DropObjectKind::view>(ikw<"VIEW">)) +
             option(flag<&ast::DropStmt::ifExists>(ikw<"IF"> +
                                                        ikw<"EXISTS">)) +
             option(assign<&ast::DropStmt::schema>(ref<Name>) + lit<".">) +
             assign<&ast::DropStmt::name>(ref<Name>)> {};

  struct CreateIndexStmt
      : rule<ikw<"CREATE"> +
             option(flag<&ast::CreateIndexStmt::unique>(ikw<"UNIQUE">)) +
             ikw<"INDEX"> +
             option(flag<&ast::CreateIndexStmt::ifNotExists>(
                 ikw<"IF"> + ikw<"NOT"> + ikw<"EXISTS">)) +
             option(assign<&ast::CreateIndexStmt::schema>(ref<Name>) +
                    lit<".">) +
             assign<&ast::CreateIndexStmt::name>(ref<Name>) + ikw<"ON"> +
             assign<&ast::CreateIndexStmt::table>(ref<Name>) + lit<"("> +
             append<&ast::CreateIndexStmt::columns>(ref<IndexedColumn>) +
             many(lit<","> + append<&ast::CreateIndexStmt::columns>(
                                 ref<IndexedColumn>)) +
             lit<")"> +
             option(ikw<"WHERE"> +
                    assign<&ast::CreateIndexStmt::where>(ref<Expression>))> {};

  struct CreateTableStmt
      : rule<
            ikw<"CREATE"> +
            option(flag<&ast::CreateTableStmt::temp>(ikw<"TEMP"> |
                                                          ikw<"TEMPORARY">)) +
            ikw<"TABLE"> +
            option(flag<&ast::CreateTableStmt::ifNotExists>(
                ikw<"IF"> + ikw<"NOT"> + ikw<"EXISTS">)) +
            option(assign<&ast::CreateTableStmt::schema>(ref<Name>) +
                   lit<".">) +
            assign<&ast::CreateTableStmt::name>(ref<Name>) +
            ((lit<"("> +
              some(append<&ast::CreateTableStmt::columns>(ref<ColumnDef>), lit<",">) +
              option(lit<","> +
                     append<&ast::CreateTableStmt::constraints>(
                         ref<TableConstraint>) +
                     many(option(lit<",">) +
                          append<&ast::CreateTableStmt::constraints>(
                              ref<TableConstraint>))) +
              lit<")"> +
              option((flag<&ast::CreateTableStmt::withoutRowid>(
                          ikw<"WITHOUT"> + ikw<"ROWID">) |
                      flag<&ast::CreateTableStmt::strict>(ikw<"STRICT">)) +
                     many(lit<","> +
                          (flag<&ast::CreateTableStmt::withoutRowid>(
                               ikw<"WITHOUT"> + ikw<"ROWID">) |
                           flag<&ast::CreateTableStmt::strict>(
                               ikw<"STRICT">)))))  //
             | (ikw<"AS"> +
                assign<&ast::CreateTableStmt::asSelect>(ref<Select>)))> {};


  struct CreateTriggerStmt
      : rule<ikw<"CREATE"> +
             option(flag<&ast::CreateTriggerStmt::temp>(
                 ikw<"TEMP"> | ikw<"TEMPORARY">)) +
             ikw<"TRIGGER"> +
             option(flag<&ast::CreateTriggerStmt::ifNotExists>(
                 ikw<"IF"> + ikw<"NOT"> + ikw<"EXISTS">)) +
             option(assign<&ast::CreateTriggerStmt::schema>(ref<Name>) +
                    lit<".">) +
             assign<&ast::CreateTriggerStmt::name>(ref<Name>) +
             option(assign<&ast::CreateTriggerStmt::when>(
                 as<ast::TriggerWhen::before>(ikw<"BEFORE">) | as<ast::TriggerWhen::after>(ikw<"AFTER">) | as<ast::TriggerWhen::instead_of>(ikw<"INSTEAD"> + ikw<"OF">))) +
             ((assign<&ast::CreateTriggerStmt::event>(as<ast::TriggerEvent::delete_>(ikw<"DELETE">)))    //
              | (assign<&ast::CreateTriggerStmt::event>(as<ast::TriggerEvent::insert>(ikw<"INSERT">)))  //
              | (assign<&ast::CreateTriggerStmt::event>(as<ast::TriggerEvent::update>(ikw<"UPDATE">)) +
                 option(ikw<"OF"> +
                        append<&ast::CreateTriggerStmt::updateColumns>(
                            ref<ColumnName>) +
                        many(lit<","> +
                             append<&ast::CreateTriggerStmt::updateColumns>(
                                 ref<ColumnName>))))) +
             ikw<"ON"> + assign<&ast::CreateTriggerStmt::table>(ref<Name>) +
             option(
                 ikw<"FOR"> + ikw<"EACH"> +
                 flag<&ast::CreateTriggerStmt::forEachRow>(ikw<"ROW">)) +
             option(ikw<"WHEN"> + assign<&ast::CreateTriggerStmt::whenExpr>(
                                      ref<Expression>)) +
             ikw<"BEGIN"> +
             some(append<&ast::CreateTriggerStmt::body>(
                      ref<UpdateStmt> | ref<InsertStmt> |
                      ref<DeleteStmt> | ref<Select>) +
                  lit<";">) +
             ikw<"END">> {};

  struct CreateViewStmt
      : rule<ikw<"CREATE"> +
             option(flag<&ast::CreateViewStmt::temp>(ikw<"TEMP"> |
                                                          ikw<"TEMPORARY">)) +
             ikw<"VIEW"> +
             option(flag<&ast::CreateViewStmt::ifNotExists>(
                 ikw<"IF"> + ikw<"NOT"> + ikw<"EXISTS">)) +
             option(assign<&ast::CreateViewStmt::schema>(ref<Name>) +
                    lit<".">) +
             assign<&ast::CreateViewStmt::name>(ref<Name>) +
             option(lit<"("> +
                    append<&ast::CreateViewStmt::columns>(ref<ColumnName>) +
                    many(lit<","> + append<&ast::CreateViewStmt::columns>(
                                        ref<ColumnName>)) +
                    lit<")">) +
             ikw<"AS"> + assign<&ast::CreateViewStmt::query>(ref<Select>)> {
  };

  static constexpr auto ModuleQuoted =
      (lit<"'"> + many(lit<"''"> | cr<"^'">) + lit<"'">) |
      (lit<"\""> + many(lit<"\"\""> | cr<R"(^")">) + lit<"\"">) |
      (lit<"`"> + many(lit<"``"> | cr<"^`">) + lit<"`">) |
      (lit<"["> + many(cr<"^]">) + lit<"]">);
  static constexpr auto ModuleCommentStart = lit<"/*"> | lit<"--">;
  static constexpr auto ModuleComment =
      through(lit<"/*">, lit<"*/"> | eof) | (lit<"--"> + many(cr<R"(^\n)">));
  struct MODULE_GROUP
      : terminal<lit<"("> +
                 many(ModuleQuoted | ModuleComment | ref<MODULE_GROUP> |
                      (!ModuleCommentStart + cr<R"(^()'"`[)">)) +
                 lit<")">> {};
  struct MODULE_ARG
      : terminal<some(ModuleQuoted | ModuleComment | ref<MODULE_GROUP> |
                      (!ModuleCommentStart + cr<R"(^(),'"`[)">))> {};

  struct ModuleArgument : rule<option(ref<MODULE_ARG>)> {};

  struct CreateVirtualTableStmt
      : rule<ikw<"CREATE"> + ikw<"VIRTUAL"> + ikw<"TABLE"> +
             option(flag<&ast::CreateVirtualTableStmt::ifNotExists>(
                 ikw<"IF"> + ikw<"NOT"> + ikw<"EXISTS">)) +
             option(assign<&ast::CreateVirtualTableStmt::schema>(ref<Name>) +
                    lit<".">) +
             assign<&ast::CreateVirtualTableStmt::name>(ref<Name>) +
             ikw<"USING"> +
             assign<&ast::CreateVirtualTableStmt::module>(ref<Name>) +
             option(lit<"("> +
                    append<&ast::CreateVirtualTableStmt::moduleArgs>(
                        ref<ModuleArgument>) +
                    many(lit<","> +
                         append<&ast::CreateVirtualTableStmt::moduleArgs>(
                             ref<ModuleArgument>)) +
                    lit<")">)> {};


  struct AlterTableStmt
      : rule<
            ikw<"ALTER"> + ikw<"TABLE"> +
            option(assign<&ast::AlterTableStmt::schema>(ref<Name>) + lit<".">) +
            assign<&ast::AlterTableStmt::table>(ref<Name>) +
            ((assign<&ast::AlterTableStmt::action>(as<ast::AlterTableStmt::Action::rename_table>(ikw<"RENAME"> + ikw<"TO">)) +
              assign<&ast::AlterTableStmt::newName>(ref<Name>))  //
             |
             (assign<&ast::AlterTableStmt::action>(as<ast::AlterTableStmt::Action::rename_column>(ikw<"RENAME"> + option(ikw<"COLUMN">))) +
              assign<&ast::AlterTableStmt::targetName>(ref<Name>) + ikw<"TO"> +
              assign<&ast::AlterTableStmt::newName>(ref<Name>))  //
             |
             (assign<&ast::AlterTableStmt::action>(as<ast::AlterTableStmt::Action::add_column>(ikw<"ADD"> + option(ikw<"COLUMN">))) +
              assign<&ast::AlterTableStmt::addedColumn>(ref<ColumnDef>))  //
             |
             (assign<&ast::AlterTableStmt::action>(as<ast::AlterTableStmt::Action::add_check>(ikw<"ADD">)) +
              option(ikw<"CONSTRAINT"> +
                     assign<&ast::AlterTableStmt::constraintName>(ref<Name>)) +
              ikw<"CHECK"> + lit<"("> +
              assign<&ast::AlterTableStmt::addedCheck>(ref<Expression>) +
              lit<")"> +
              option(ikw<"ON"> + ikw<"CONFLICT"> +
                     assign<&ast::AlterTableStmt::conflict>(
                         ConflictActionAlt)))  //
             | (assign<&ast::AlterTableStmt::action>(as<ast::AlterTableStmt::Action::drop_column>(ikw<"DROP"> + option(ikw<"COLUMN">))) +
                assign<&ast::AlterTableStmt::targetName>(ref<ColumnName>))  //
             |
             (assign<&ast::AlterTableStmt::action>(as<ast::AlterTableStmt::Action::drop_constraint>(ikw<"DROP"> + ikw<"CONSTRAINT">)) +
              assign<&ast::AlterTableStmt::constraintName>(ref<Name>))  //
             | (ikw<"ALTER"> + option(ikw<"COLUMN">) +
                assign<&ast::AlterTableStmt::targetName>(ref<ColumnName>) +
                ((assign<&ast::AlterTableStmt::action>(as<ast::AlterTableStmt::Action::alter_set_not_null>(ikw<"SET"> + ikw<"NOT"> + ikw<"NULL">)) +
                  option(ikw<"ON"> + ikw<"CONFLICT"> +
                         assign<&ast::AlterTableStmt::conflict>(
                             ConflictActionAlt)))  //
                 | assign<&ast::AlterTableStmt::action>(
                       as<ast::AlterTableStmt::Action::alter_drop_not_null>(ikw<"DROP"> + ikw<"NOT"> + ikw<"NULL">)))))> {};

  struct SqlStmt
      : rule<option(flag<&ast::SqlStmt::explain>(ikw<"EXPLAIN">) +
                    option(flag<&ast::SqlStmt::queryPlan>(ikw<"QUERY"> +
                                                               ikw<"PLAN">))) +
             assign<&ast::SqlStmt::statement>(
                 ref<AlterTableStmt> | ref<AnalyzeStmt> |
                 ref<AttachStmt> | ref<BeginStmt> |
                 ref<CommitStmt> | ref<CreateIndexStmt> |
                 ref<CreateTableStmt> | ref<CreateTriggerStmt> |
                 ref<CreateViewStmt> | ref<CreateVirtualTableStmt> |
                 ref<DeleteStmt> | ref<DetachStmt> | ref<DropStmt> |
                 ref<InsertStmt> | ref<PragmaStmt> |
                 ref<ReindexStmt> | ref<ReleaseStmt> |
                 ref<RollbackStmt> | ref<SavepointStmt> |
                 ref<Select> | ref<UpdateStmt> | ref<VacuumStmt>)> {
  };

  struct Script
      : rule<many(lit<";">) +
             option(append<&ast::Script::statements>(ref<SqlStmt>) +
                    many(some(lit<";">) +
                         append<&ast::Script::statements>(ref<SqlStmt>)) +
                    many(lit<";">))> {};

  using entry = Script;
  using ignored = WS;
  using hidden = list<LINE_COMMENT, BLOCK_COMMENT>;
};

}  // namespace sqlite
