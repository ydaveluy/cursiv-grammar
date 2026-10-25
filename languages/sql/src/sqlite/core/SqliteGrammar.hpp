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
using StringConv = cursiv::text_conv<unquote_string>;

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
using IdentConv = cursiv::text_conv<unquote_ident>;

inline constexpr auto unquote_blob = [](std::string_view t) {
  return t.substr(2, t.size() - 3);
};
using BlobConv = cursiv::text_conv<unquote_blob>;

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
  struct CurrentTimeTag;
  struct CurrentDateTag;
  struct CurrentTimestampTag;
  struct CurrentTimeLit;
  struct BindParamRule;
  struct LiteralValue;
  struct ColumnRef;
  struct QualifiedRef;
  struct TypeNameWord;
  struct TypeNameWords;
  struct SignedNumber;
  struct SignedLiteral;
  struct TypeNameRule;
  struct NameLitRule;
  struct RaiseIgnore;
  struct RaiseRollback;
  struct RaiseAbort;
  struct RaiseFail;
  struct RaiseFunctionRule;
  struct WhenClauseRule;
  struct CaseExprRule;
  struct CastExprRule;
  struct RowExprRule;
  struct ValueRowRule;
  struct KwRange;
  struct KwRows;
  struct KwGroups;
  struct BoundUnboundedPreceding;
  struct BoundUnboundedFollowing;
  struct BoundCurrentRow;
  struct KwPreceding;
  struct KwFollowing;
  struct FrameBoundRule;
  struct ExcludeNoOthers;
  struct ExcludeCurrentRow;
  struct ExcludeGroup;
  struct ExcludeTies;
  struct FrameSpecRule;
  struct NullsLastTag;
  struct NullsFirstTag;
  struct OrderTermRule;
  struct WindowDefnRule;
  struct FuncCallRule;
  struct SubqueryExprRule;
  struct ExprBase;
  struct ExprUnary;
  struct ExprCollate;
  struct ExprString;
  struct ExprMultiplication;
  struct ExprAddition;
  struct ExprBitwise;
  struct ExprComparison;
  struct LikeTag;
  struct GlobTag;
  struct RegexpTag;
  struct MatchTag;
  struct ExprBinarySuffix;
  struct ExprNot;
  struct AndOp;
  struct OrOp;
  struct Expression;

  struct ConflictRollback;
  struct ConflictAbort;
  struct ConflictFail;
  struct ConflictIgnore;
  struct ConflictReplace;
  struct IndexedColumnRule;
  struct FkRefSetNull;
  struct FkRefSetDefault;
  struct FkRefCascade;
  struct FkRefRestrict;
  struct FkRefNoAction;
  struct FkKindOnDelete;
  struct FkKindOnUpdate;
  struct FkKindOnInsert;
  struct FkKindMatch;
  struct ForeignKeyActionRule;
  struct DeferrableYes;
  struct DeferrableNo;
  struct InitiallyDeferredTag;
  struct InitiallyImmediateTag;
  struct ForeignKeyClauseRule;
  struct CCPrimaryKeyTag;
  struct CCUniqueTag;
  struct CCCheckTag;
  struct CCDefaultTag;
  struct CCCollateTag;
  struct CCNullTag;
  struct CCGeneratedTag;
  struct StoredTag;
  struct VirtualTag;
  struct ColumnConstraintRule;
  struct ColumnDefRule;
  struct TCPrimaryKeyTag;
  struct TCUniqueTag;
  struct TCCheckTag;
  struct TableConstraintRule;

  struct MaterializedTag;
  struct NotMaterializedTag;
  struct CommonTableExprRule;
  struct ResultColumnRule;
  struct TableItemRule;
  struct JoinStepRule;
  struct JoinClauseRule;
  struct NamedWindowRule;
  struct SelectCoreRule;
  struct CompoundUnionAll;
  struct CompoundUnion;
  struct CompoundIntersect;
  struct CompoundExcept;
  struct FirstCompoundPartRule;
  struct CompoundPartRule;
  struct SelectRule;

  struct QualifiedTableNameRule;
  struct UpsertAssignmentRule;
  struct UpsertClauseRule;
  struct InsModeInsert;
  struct InsModeReplace;
  struct InsModeOrReplace;
  struct InsModeOrRollback;
  struct InsModeOrAbort;
  struct InsModeOrFail;
  struct InsModeOrIgnore;
  struct InsertStmtRule;
  struct UpdateStmtRule;
  struct DeleteStmtRule;
  struct PragmaValueRule;
  struct PragmaStmtRule;
  struct VacuumStmtRule;
  struct AttachStmtRule;
  struct DetachStmtRule;
  struct TxDeferred;
  struct TxImmediate;
  struct TxExclusive;
  struct BeginStmtRule;
  struct CommitStmtRule;
  struct RollbackStmtRule;
  struct SavepointStmtRule;
  struct ReleaseStmtRule;
  struct AnalyzeStmtRule;
  struct ReindexStmtRule;
  struct DropIndexTag;
  struct DropTableTag;
  struct DropTriggerTag;
  struct DropViewTag;
  struct DropStmtRule;
  struct CreateIndexStmtRule;
  struct CreateTableStmtRule;
  struct TrigBefore;
  struct TrigAfter;
  struct TrigInsteadOf;
  struct TrigDelete;
  struct TrigInsert;
  struct TrigUpdate;
  struct CreateTriggerStmtRule;
  struct CreateViewStmtRule;
  struct CreateVirtualTableStmtRule;
  struct AAKindRenameTable;
  struct AAKindRenameColumn;
  struct AAKindAddColumn;
  struct AAKindAddCheck;
  struct AAKindDropColumn;
  struct AAKindDropConstraint;
  struct AAKindSetNotNull;
  struct AAKindDropNotNull;
  struct AlterTableStmtRule;
  struct SqlStmtRule;
  struct ScriptRule;

  static constexpr auto IdStart = cr<R"(A-Za-z_\u{80}-\u{10FFFF})">;
  static constexpr auto IdContinue = cr<R"(A-Za-z_0-9$\u{80}-\u{10FFFF})">;
  static constexpr auto word_boundary = IdContinue;

  struct WS : terminal<some(s | lit<"\xef\xbb\xbf">)> {};
  struct LINE_COMMENT : terminal<lit<"--"> + many(cr<R"(^\n)">)> {};
  struct BLOCK_COMMENT : terminal<through(lit<"/*">, lit<"*/"> | eof)> {};

  static constexpr auto IdentBody =
      (lit<"\""> + many(lit<"\"\""> | cr<R"(^")">) + lit<"\"">) |
      (lit<"`"> + many(lit<"``"> | cr<"^`">) + lit<"`">) |
      (lit<"["> + many(cr<"^]">) + lit<"]">) | (IdStart + many(IdContinue));
  struct IDENTIFIER : terminal<IdentBody, IdentConv> {};

  static constexpr auto Digits = some(d) + many(lit<"_"> + some(d));
  static constexpr auto HexDigits =
      some(cr<"0-9A-Fa-f">) + many(lit<"_"> + some(cr<"0-9A-Fa-f">));
  struct NUMERIC_LITERAL
      : terminal<(((Digits + option(lit<"."> + option(Digits))) |
                  (lit<"."> + Digits)) +
                         option((lit<"e"> | lit<"E">)+option(cr<"+-">) +
                                Digits)) |
                     ((lit<"0x"> | lit<"0X">)+HexDigits),
                 std::string_view> {};

  static constexpr auto StringBody =
      lit<"'"> + many(lit<"''"> | cr<"^'">) + lit<"'">;
  struct STRING_LITERAL : terminal<StringBody, StringConv> {};
  struct BLOB_LITERAL : terminal<(lit<"x"> | lit<"X">)+lit<"'"> +
                                     many(lit<"''"> | cr<"^'">) + lit<"'">,
                                 BlobConv> {};
  struct BIND_PARAMETER
      : terminal<(lit<"?"> + many(d)) |
                 (cr<":@$#"> + many(IdContinue | lit<"::">) +
                  option(lit<"("> + many(cr<R"(^) \t\r\n\f)">) + lit<")">))> {};

  struct Name : rule<ref<IDENTIFIER> | ref<STRING_LITERAL>> {};
  struct WindowName : terminal<IdentBody | StringBody> {};
  struct ColumnName : rule<ref<Name>> {};
  // -- operator ladder (infix, tightest to loosest) ----
  struct OpConcat : terminal<lit<"||">, value<ast::Operator::concat>> {};
  struct OpJptr2 : terminal<lit<"->>">, value<ast::Operator::jptr2>> {};
  struct OpJptr : terminal<lit<"->">, value<ast::Operator::jptr>> {};
  struct OpMul : terminal<lit<"*">, value<ast::Operator::multiply>> {};
  struct OpDiv : terminal<lit<"/">, value<ast::Operator::divide>> {};
  struct OpMod : terminal<lit<"%">, value<ast::Operator::modulo>> {};
  struct OpAdd : terminal<lit<"+">, value<ast::Operator::add>> {};
  struct OpSub : terminal<lit<"-">, value<ast::Operator::subtract>> {};
  struct OpShl : terminal<lit<"<<">, value<ast::Operator::shl>> {};
  struct OpShr : terminal<lit<">>">, value<ast::Operator::shr>> {};
  struct OpBitAnd : terminal<lit<"&">, value<ast::Operator::bitand_>> {};
  struct OpBitOr : terminal<lit<"|">, value<ast::Operator::bitor_>> {};
  struct OpLe : terminal<lit<"<=">, value<ast::Operator::le>> {};
  struct OpGe : terminal<lit<">=">, value<ast::Operator::ge>> {};
  struct OpLt : terminal<lit<"<">, value<ast::Operator::lt>> {};
  struct OpGt : terminal<lit<">">, value<ast::Operator::gt>> {};
  struct OpEq2 : terminal<lit<"==">, value<ast::Operator::eq>> {};
  struct OpEq : terminal<lit<"=">, value<ast::Operator::eq>> {};
  struct OpNe1 : terminal<lit<"!=">, value<ast::Operator::ne>> {};
  struct OpNe2 : terminal<lit<"<>">, value<ast::Operator::ne>> {};

  static constexpr auto OpString = ref<OpConcat> | ref<OpJptr2> | ref<OpJptr>;
  static constexpr auto OpMulLvl = ref<OpMul> | ref<OpDiv> | ref<OpMod>;
  static constexpr auto OpAddLvl = ref<OpAdd> | ref<OpSub>;
  static constexpr auto OpBitwise =
      ref<OpShl> | ref<OpShr> | ref<OpBitAnd> | ref<OpBitOr>;
  static constexpr auto OpCompare =
      ref<OpLe> | ref<OpGe> | ref<OpLt> | ref<OpGt>;
  static constexpr auto OpEquality =
      ref<OpEq2> | ref<OpEq> | ref<OpNe1> | ref<OpNe2>;

  struct NumberLit
      : rule<assign<&ast::NumberLit::value>(ref<NUMERIC_LITERAL>)> {};
  struct StringLit : rule<assign<&ast::StringLit::value>(ref<STRING_LITERAL>)> {
  };
  struct BlobLit : rule<assign<&ast::BlobLit::value>(ref<BLOB_LITERAL>)> {};
  struct NullLit : rule<create<ast::NullLit> + ikw<"NULL">> {};
  struct BoolLit
      : rule<create<ast::BoolLit> +
             (flag<&ast::BoolLit::value>(ikw<"TRUE">) | ikw<"FALSE">)> {};
  struct CurrentTimeTag
      : rule<ikw<"CURRENT_TIME">, value<ast::CurrentValueKind::time>> {};
  struct CurrentDateTag
      : rule<ikw<"CURRENT_DATE">, value<ast::CurrentValueKind::date>> {};
  struct CurrentTimestampTag : rule<ikw<"CURRENT_TIMESTAMP">,
                                    value<ast::CurrentValueKind::timestamp>> {};
  struct CurrentTimeLit : rule<assign<&ast::CurrentValueLit::kind>(
                              ref<CurrentTimeTag> | ref<CurrentDateTag> |
                              ref<CurrentTimestampTag>)> {};
  struct BindParamRule
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
  struct TypeNameRule
      : rule<assign<&ast::TypeName::name>(ref<TypeNameWords>) +
             option(lit<"("> +
                    assign<&ast::TypeName::param1>(ref<SignedNumber>) +
                    option(lit<","> +
                           assign<&ast::TypeName::param2>(ref<SignedNumber>)) +
                    lit<")">)> {};

  struct NameLitRule : rule<assign<&ast::NameLit::value>(ref<ColumnName>)> {};

  struct RaiseIgnore : rule<ikw<"IGNORE">, value<ast::RaiseKind::ignore>> {};
  struct RaiseRollback
      : rule<ikw<"ROLLBACK">, value<ast::RaiseKind::rollback>> {};
  struct RaiseAbort : rule<ikw<"ABORT">, value<ast::RaiseKind::abort>> {};
  struct RaiseFail : rule<ikw<"FAIL">, value<ast::RaiseKind::fail>> {};
  struct RaiseFunctionRule
      : rule<ikw<"RAISE"> + lit<"("> +
             (assign<&ast::RaiseExpr::kind>(ref<RaiseIgnore>) |
              (assign<&ast::RaiseExpr::kind>(ref<RaiseRollback> |
                                             ref<RaiseAbort> | ref<RaiseFail>) +
               lit<","> + assign<&ast::RaiseExpr::message>(ref<Expression>))) +
             lit<")">> {};

  struct WhenClauseRule
      : rule<ikw<"WHEN"> + assign<&ast::WhenClause::when>(ref<Expression>) +
             ikw<"THEN"> + assign<&ast::WhenClause::then>(ref<Expression>)> {};
  struct CaseExprRule
      : rule<ikw<"CASE"> +
             option(assign<&ast::CaseExpr::operand>(ref<Expression>)) +
             some(append<&ast::CaseExpr::whens>(ref<WhenClauseRule>)) +
             option(ikw<"ELSE"> +
                    assign<&ast::CaseExpr::elseValue>(ref<Expression>)) +
             ikw<"END">> {};

  struct CastExprRule
      : rule<ikw<"CAST"> + lit<"("> +
             assign<&ast::CastExpr::operand>(ref<Expression>) + ikw<"AS"> +
             option(assign<&ast::CastExpr::type>(ref<TypeNameRule>)) +
             lit<")">> {};

  // Two or more items: a single-item parenthesized expression is plain
  // grouping (ExprBase), not a row value.
  struct RowExprRule
      : rule<lit<"("> + append<&ast::RowExpr::items>(ref<Expression>) +
             some(lit<","> + append<&ast::RowExpr::items>(ref<Expression>)) +
             lit<")">> {};
  // One or more items: `VALUES (1)` is a single-column row.
  struct ValueRowRule
      : rule<lit<"("> + append<&ast::RowExpr::items>(ref<Expression>) +
             many(lit<","> + append<&ast::RowExpr::items>(ref<Expression>)) +
             lit<")">> {};

  struct KwRange : rule<ikw<"RANGE">, value<ast::FrameType::range>> {};
  struct KwRows : rule<ikw<"ROWS">, value<ast::FrameType::rows>> {};
  struct KwGroups : rule<ikw<"GROUPS">, value<ast::FrameType::groups>> {};

  struct BoundUnboundedPreceding
      : rule<ikw<"UNBOUNDED"> + ikw<"PRECEDING">,
             value<ast::FrameBoundKind::unbounded_preceding>> {};
  struct BoundUnboundedFollowing
      : rule<ikw<"UNBOUNDED"> + ikw<"FOLLOWING">,
             value<ast::FrameBoundKind::unbounded_following>> {};
  struct BoundCurrentRow : rule<ikw<"CURRENT"> + ikw<"ROW">,
                                value<ast::FrameBoundKind::current_row>> {};
  struct KwPreceding
      : rule<ikw<"PRECEDING">, value<ast::FrameBoundKind::preceding>> {};
  struct KwFollowing
      : rule<ikw<"FOLLOWING">, value<ast::FrameBoundKind::following>> {};
  struct FrameBoundRule
      : rule<assign<&ast::FrameBound::kind>(ref<BoundUnboundedPreceding> |
                                            ref<BoundUnboundedFollowing> |
                                            ref<BoundCurrentRow>) |
             (assign<&ast::FrameBound::offset>(ref<Expression>) +
              assign<&ast::FrameBound::kind>(ref<KwPreceding> |
                                             ref<KwFollowing>))> {};

  struct ExcludeNoOthers
      : rule<ikw<"NO"> + ikw<"OTHERS">, value<ast::FrameExclude::no_others>> {};
  struct ExcludeCurrentRow : rule<ikw<"CURRENT"> + ikw<"ROW">,
                                  value<ast::FrameExclude::current_row>> {};
  struct ExcludeGroup : rule<ikw<"GROUP">, value<ast::FrameExclude::group>> {};
  struct ExcludeTies : rule<ikw<"TIES">, value<ast::FrameExclude::ties>> {};

  struct FrameSpecRule
      : rule<assign<&ast::FrameSpec::type>(ref<KwRange> | ref<KwRows> |
                                           ref<KwGroups>) +
             (assign<&ast::FrameSpec::start>(ref<FrameBoundRule>) |
              (ikw<"BETWEEN"> +
               assign<&ast::FrameSpec::start>(ref<FrameBoundRule>) +
               ikw<"AND"> +
               assign<&ast::FrameSpec::end>(ref<FrameBoundRule>))) +
             option(ikw<"EXCLUDE"> +
                    assign<&ast::FrameSpec::exclude>(
                        ref<ExcludeNoOthers> | ref<ExcludeCurrentRow> |
                        ref<ExcludeGroup> | ref<ExcludeTies>))> {};

  struct NullsLastTag : rule<ikw<"LAST">, value<true>> {};
  struct NullsFirstTag : rule<ikw<"FIRST">, value<false>> {};
  struct OrderTermRule
      : rule<assign<&ast::OrderTerm::key>(ref<Expression>) +
             option(flag<&ast::OrderTerm::descending>(ikw<"DESC">) |
                    ikw<"ASC">) +
             option(ikw<"NULLS"> +
                    assign<&ast::OrderTerm::nullsLast>(ref<NullsLastTag> |
                                                       ref<NullsFirstTag>))> {};

  struct WindowDefnRule
      : rule<lit<"("> +
             option(assign<&ast::WindowDefn::baseWindow>(ref<WindowName>)) +
             option(ikw<"PARTITION"> + ikw<"BY"> +
                    some(append<&ast::WindowDefn::partitionBy>(ref<Expression>),
                         lit<",">)) +
             option(ikw<"ORDER"> + ikw<"BY"> +
                    some(append<&ast::WindowDefn::orderBy>(ref<OrderTermRule>),
                         lit<",">)) +
             option(assign<&ast::WindowDefn::frame>(ref<FrameSpecRule>)) +
             lit<")">> {};

  struct FuncCallRule
      : rule<
            assign<&ast::FuncCall::name>(ref<Name>) + lit<"("> +
            option(
                flag<&ast::FuncCall::star>(lit<"*">) |
                (option(flag<&ast::FuncCall::distinct>(ikw<"DISTINCT">) |
                        ikw<"ALL">) +
                 some(append<&ast::FuncCall::args>(ref<Expression>), lit<",">) +
                 option(
                     ikw<"ORDER"> + ikw<"BY"> +
                     some(append<&ast::FuncCall::orderBy>(ref<OrderTermRule>),
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
                    assign<&ast::FuncCall::overDefn>(ref<WindowDefnRule>)))> {};

  struct SubqueryExprRule
      : rule<option(flag<&ast::SubqueryExpr::isExists>(
                 option(flag<&ast::SubqueryExpr::negated>(ikw<"NOT">)) +
                 ikw<"EXISTS">)) +
             lit<"("> + assign<&ast::SubqueryExpr::query>(ref<SelectRule>) +
             lit<")">> {};

  struct ExprBase : rule<ref<LiteralValue>         //
                         | ref<BindParamRule>      //
                         | ref<QualifiedRef>       //
                         | ref<ColumnRef>          //
                         | ref<SubqueryExprRule>   //
                         | ref<RaiseFunctionRule>  //
                         | ref<FuncCallRule>       //
                         | ref<RowExprRule>        //
                         | ref<CastExprRule>       //
                         | ref<CaseExprRule>       //
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
      lit<"("> + assign<&ast::InExpr::subquery>(ref<SelectRule>) + lit<")">;
  static constexpr auto InTargetList =
      lit<"("> +
      option(append<&ast::InExpr::list>(ref<Expression>) +
             many(lit<","> + append<&ast::InExpr::list>(ref<Expression>))) +
      lit<")">;
  static constexpr auto InTargetTableFunction =
      option(assign<&ast::InExpr::schema>(ref<Name>) + lit<".">) +
      assign<&ast::InExpr::table>(ref<Name>) +
      flag<&ast::InExpr::isTableFunction>(lit<"(">) +
      option(
          append<&ast::InExpr::tableArgs>(ref<Expression>) +
          many(lit<","> + append<&ast::InExpr::tableArgs>(ref<Expression>))) +
      lit<")">;
  static constexpr auto InTargetTable =
      option(assign<&ast::InExpr::schema>(ref<Name>) + lit<".">) +
      assign<&ast::InExpr::table>(ref<Name>);

  struct LikeTag : rule<ikw<"LIKE">, value<ast::LikeKind::like>> {};
  struct GlobTag : rule<ikw<"GLOB">, value<ast::LikeKind::glob>> {};
  struct RegexpTag : rule<ikw<"REGEXP">, value<ast::LikeKind::regexp>> {};
  struct MatchTag : rule<ikw<"MATCH">, value<ast::LikeKind::match>> {};

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
                     assign<&ast::LikeExpr::kind>(ref<LikeTag> | ref<GlobTag> |
                                                  ref<RegexpTag> |
                                                  ref<MatchTag>) +
                     assign<&ast::LikeExpr::pattern>(ref<ExprComparison>) +
                     option(ikw<"ESCAPE"> + assign<&ast::LikeExpr::escape>(
                                                ref<ExprComparison>))) |
                 nest<ast::NullTest, &ast::NullTest::operand>(
                     ikw<"ISNULL"> |
                     flag<&ast::NullTest::negated>(
                         ikw<"NOTNULL"> | (ikw<"NOT"> + ikw<"NULL">))))> {};

  struct ExprNot : rule<ref<ExprBinarySuffix>> {};

  struct AndOp : rule<ikw<"AND">, value<ast::Operator::logical_and>> {};
  struct OrOp : rule<ikw<"OR">, value<ast::Operator::logical_or>> {};

  struct Expression
      : infix<ExprNot, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, (left(ref<AndOp>) >> left(ref<OrOp>))> {
  };

  // -- shared DDL fragments ----
  struct ConflictRollback
      : rule<ikw<"ROLLBACK">, value<ast::ConflictAction::rollback>> {};
  struct ConflictAbort : rule<ikw<"ABORT">, value<ast::ConflictAction::abort>> {
  };
  struct ConflictFail : rule<ikw<"FAIL">, value<ast::ConflictAction::fail>> {};
  struct ConflictIgnore
      : rule<ikw<"IGNORE">, value<ast::ConflictAction::ignore>> {};
  struct ConflictReplace
      : rule<ikw<"REPLACE">, value<ast::ConflictAction::replace>> {};
  static constexpr auto ConflictActionAlt =
      ref<ConflictRollback> | ref<ConflictAbort> | ref<ConflictFail> |
      ref<ConflictIgnore> | ref<ConflictReplace>;

  struct IndexedColumnRule
      : rule<assign<&ast::IndexedColumn::expr>(ref<Expression>) +
             option(flag<&ast::IndexedColumn::descending>(ikw<"DESC">) |
                    ikw<"ASC">)> {};

  struct FkRefSetNull
      : rule<ikw<"SET"> + ikw<"NULL">, value<ast::FkRefAction::set_null>> {};
  struct FkRefSetDefault : rule<ikw<"SET"> + ikw<"DEFAULT">,
                                value<ast::FkRefAction::set_default>> {};
  struct FkRefCascade : rule<ikw<"CASCADE">, value<ast::FkRefAction::cascade>> {
  };
  struct FkRefRestrict
      : rule<ikw<"RESTRICT">, value<ast::FkRefAction::restrict>> {};
  struct FkRefNoAction
      : rule<ikw<"NO"> + ikw<"ACTION">, value<ast::FkRefAction::no_action>> {};
  struct FkKindOnDelete
      : rule<ikw<"ON"> + ikw<"DELETE">, value<ast::FkActionKind::on_delete>> {};
  struct FkKindOnUpdate
      : rule<ikw<"ON"> + ikw<"UPDATE">, value<ast::FkActionKind::on_update>> {};
  struct FkKindOnInsert
      : rule<ikw<"ON"> + ikw<"INSERT">, value<ast::FkActionKind::on_insert>> {};
  struct FkKindMatch : rule<ikw<"MATCH">, value<ast::FkActionKind::match>> {};
  struct ForeignKeyActionRule
      : rule<(assign<&ast::ForeignKeyAction::kind>(ref<FkKindOnDelete> |
                                                   ref<FkKindOnUpdate> |
                                                   ref<FkKindOnInsert>) +
              assign<&ast::ForeignKeyAction::refAction>(
                  ref<FkRefSetNull> | ref<FkRefSetDefault> | ref<FkRefCascade> |
                  ref<FkRefRestrict> | ref<FkRefNoAction>)) |
             (assign<&ast::ForeignKeyAction::kind>(ref<FkKindMatch>) +
              assign<&ast::ForeignKeyAction::matchName>(ref<Name>))> {};

  struct DeferrableYes
      : rule<ikw<"DEFERRABLE">, value<ast::Deferrable::deferrable>> {};
  struct DeferrableNo : rule<ikw<"NOT"> + ikw<"DEFERRABLE">,
                             value<ast::Deferrable::not_deferrable>> {};
  struct InitiallyDeferredTag : rule<ikw<"INITIALLY"> + ikw<"DEFERRED">,
                                     value<ast::InitiallyMode::deferred>> {};
  struct InitiallyImmediateTag : rule<ikw<"INITIALLY"> + ikw<"IMMEDIATE">,
                                      value<ast::InitiallyMode::immediate>> {};
  struct ForeignKeyClauseRule
      : rule<ikw<"REFERENCES"> +
             assign<&ast::ForeignKeyClause::table>(ref<Name>) +
             option(lit<"("> +
                    append<&ast::ForeignKeyClause::columns>(ref<ColumnName>) +
                    many(lit<","> + append<&ast::ForeignKeyClause::columns>(
                                        ref<ColumnName>)) +
                    lit<")">) +
             many(append<&ast::ForeignKeyClause::actions>(
                 ref<ForeignKeyActionRule>)) +
             option(assign<&ast::ForeignKeyClause::deferrable>(
                        ref<DeferrableYes> | ref<DeferrableNo>) +
                    option(assign<&ast::ForeignKeyClause::initially>(
                        ref<InitiallyDeferredTag> |
                        ref<InitiallyImmediateTag>)))> {};

  // `foreign_key` is ColumnConstraintKind's default (index 0): the branch
  // below has no leading keyword of its own to tag with `value<>`.
  struct CCPrimaryKeyTag : rule<ikw<"PRIMARY"> + ikw<"KEY">,
                                value<ast::ColumnConstraintKind::primary_key>> {
  };
  struct CCUniqueTag
      : rule<ikw<"UNIQUE">, value<ast::ColumnConstraintKind::unique>> {};
  struct CCCheckTag
      : rule<ikw<"CHECK">, value<ast::ColumnConstraintKind::check>> {};
  struct CCDefaultTag
      : rule<ikw<"DEFAULT">, value<ast::ColumnConstraintKind::default_>> {};
  struct CCCollateTag
      : rule<ikw<"COLLATE">, value<ast::ColumnConstraintKind::collate>> {};
  struct CCNullTag
      : rule<ikw<"NULL">, value<ast::ColumnConstraintKind::nullability>> {};
  struct CCGeneratedTag
      : rule<ikw<"AS">, value<ast::ColumnConstraintKind::generated>> {};
  struct StoredTag : rule<ikw<"STORED">, value<true>> {};
  struct VirtualTag : rule<ikw<"VIRTUAL">, value<false>> {};

  struct CCDeferrableTag
      : rule<ikw<"DEFERRABLE">, value<ast::ColumnConstraintKind::deferral>> {};
  struct CCNameOnlyTag
      : rule<ikw<"CONSTRAINT">, value<ast::ColumnConstraintKind::name_only>> {};
  struct ColumnConstraintRule
      : rule<(option(ikw<"CONSTRAINT"> +
                     assign<&ast::ColumnConstraint::name>(ref<Name>)) +
              ((assign<&ast::ColumnConstraint::kind>(ref<CCPrimaryKeyTag>) +
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
                assign<&ast::ColumnConstraint::kind>(ref<CCNullTag>) +
                option(ikw<"ON"> + ikw<"CONFLICT"> +
                       assign<&ast::ColumnConstraint::conflict>(
                           ConflictActionAlt)))  //
               | (assign<&ast::ColumnConstraint::kind>(ref<CCUniqueTag>) +
                  option(ikw<"ON"> + ikw<"CONFLICT"> +
                         assign<&ast::ColumnConstraint::conflict>(
                             ConflictActionAlt)))  //
               | (assign<&ast::ColumnConstraint::kind>(ref<CCCheckTag>) +
                  lit<"("> +
                  assign<&ast::ColumnConstraint::expr>(ref<Expression>) +
                  lit<")">)  //
               | (assign<&ast::ColumnConstraint::kind>(ref<CCDefaultTag>) +
                  (assign<&ast::ColumnConstraint::expr>(ref<SignedNumber>) |
                   assign<&ast::ColumnConstraint::expr>(ref<LiteralValue>) |
                   assign<&ast::ColumnConstraint::expr>(ref<NameLitRule>) |
                   assign<&ast::ColumnConstraint::expr>(ref<SignedLiteral>) |
                   (lit<"("> +
                    assign<&ast::ColumnConstraint::expr>(ref<Expression>) +
                    lit<")">)))  //
               | (assign<&ast::ColumnConstraint::kind>(ref<CCCollateTag>) +
                  assign<&ast::ColumnConstraint::collation>(ref<Name>))  //
               | (assign<&ast::ColumnConstraint::foreignKey>(
                     ref<ForeignKeyClauseRule>))  //
               | (option(flag<&ast::ColumnConstraint::generatedAlways>(
                      ikw<"GENERATED"> + ikw<"ALWAYS">)) +
                  assign<&ast::ColumnConstraint::kind>(ref<CCGeneratedTag>) +
                  lit<"("> +
                  assign<&ast::ColumnConstraint::expr>(ref<Expression>) +
                  lit<")"> +
                  option(assign<&ast::ColumnConstraint::stored>(
                      ref<StoredTag> | ref<VirtualTag>))))) |
             (option(flag<&ast::ColumnConstraint::notDeferrable>(
                  ikw<"NOT">)) +
              assign<&ast::ColumnConstraint::kind>(ref<CCDeferrableTag>) +
              option(assign<&ast::ColumnConstraint::initially>(
                  ref<InitiallyDeferredTag> | ref<InitiallyImmediateTag>))) |
             (assign<&ast::ColumnConstraint::kind>(ref<CCNameOnlyTag>) +
              assign<&ast::ColumnConstraint::name>(ref<Name>))> {};

  struct ColumnDefRule
      : rule<assign<&ast::ColumnDef::name>(ref<ColumnName>) +
             option(assign<&ast::ColumnDef::type>(ref<TypeNameRule>)) +
             many(append<&ast::ColumnDef::constraints>(
                 ref<ColumnConstraintRule>))> {};

  // `foreign_key` is TableConstraintKind's default (index 0): same reason.
  struct TCPrimaryKeyTag : rule<ikw<"PRIMARY"> + ikw<"KEY">,
                                value<ast::TableConstraintKind::primary_key>> {
  };
  struct TCUniqueTag
      : rule<ikw<"UNIQUE">, value<ast::TableConstraintKind::unique>> {};
  struct TCCheckTag
      : rule<ikw<"CHECK">, value<ast::TableConstraintKind::check>> {};
  struct TCNameOnlyTag
      : rule<ikw<"CONSTRAINT">, value<ast::TableConstraintKind::name_only>> {};
  struct TableConstraintRule
      : rule<(option(ikw<"CONSTRAINT"> +
                     assign<&ast::TableConstraint::name>(ref<Name>)) +
              ((assign<&ast::TableConstraint::kind>(ref<TCPrimaryKeyTag> |
                                                    ref<TCUniqueTag>) +
                lit<"("> +
                append<&ast::TableConstraint::columns>(ref<IndexedColumnRule>) +
                many(lit<","> + append<&ast::TableConstraint::columns>(
                                    ref<IndexedColumnRule>)) +
                option(flag<&ast::TableConstraint::autoincrement>(
                    ikw<"AUTOINCREMENT">)) +
                lit<")"> +
                option(ikw<"ON"> + ikw<"CONFLICT"> +
                       assign<&ast::TableConstraint::conflict>(
                           ConflictActionAlt)))  //
               | (assign<&ast::TableConstraint::kind>(ref<TCCheckTag>) +
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
                      ref<ForeignKeyClauseRule>)))) |
             (assign<&ast::TableConstraint::kind>(ref<TCNameOnlyTag>) +
              assign<&ast::TableConstraint::name>(ref<Name>))> {};

  // -- SELECT machinery ----
  struct MaterializedTag : rule<ikw<"MATERIALIZED">, value<true>> {};
  struct NotMaterializedTag
      : rule<ikw<"NOT"> + ikw<"MATERIALIZED">, value<false>> {};
  struct CommonTableExprRule
      : rule<assign<&ast::CommonTableExpr::name>(ref<Name>) +
             option(lit<"("> +
                    append<&ast::CommonTableExpr::columns>(ref<ColumnName>) +
                    many(lit<","> + append<&ast::CommonTableExpr::columns>(
                                        ref<ColumnName>)) +
                    lit<")">) +
             ikw<"AS"> +
             option(assign<&ast::CommonTableExpr::materialized>(
                 ref<MaterializedTag> | ref<NotMaterializedTag>)) +
             lit<"("> + assign<&ast::CommonTableExpr::query>(ref<SelectRule>) +
             lit<")">> {};

  struct ResultColumnRule
      : rule<flag<&ast::ResultColumn::star>(lit<"*">)  //
             | (assign<&ast::ResultColumn::table>(ref<Name>) + lit<"."> +
                flag<&ast::ResultColumn::star>(lit<"*">))  //
             | (assign<&ast::ResultColumn::value>(ref<Expression>) +
                option(option(ikw<"AS">) +
                       assign<&ast::ResultColumn::alias>(ref<ColumnName>)))> {};

  struct TableItemRule
      : rule<(lit<"("> + assign<&ast::TableItem::join>(ref<JoinClauseRule>) +
              lit<")">)  //
             | (lit<"("> + assign<&ast::TableItem::subquery>(ref<SelectRule>) +
                lit<")"> +
                option(option(ikw<"AS">) +
                       assign<&ast::TableItem::alias>(ref<Name>)))  //
             | (option(assign<&ast::TableItem::schema>(ref<Name>) + lit<".">) +
                assign<&ast::TableItem::name>(ref<Name>) +
                ((flag<&ast::TableItem::isTableFunction>(lit<"(">) +
                  option(append<&ast::TableItem::args>(ref<Expression>) +
                         many(lit<","> +
                              append<&ast::TableItem::args>(ref<Expression>))) +
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

  struct JoinStepRule
      : rule<(flag<&ast::JoinStep::comma>(lit<",">) |
              (many(append<&ast::JoinStep::modifiers>(
                   ikw<"NATURAL"> | ikw<"LEFT"> | ikw<"RIGHT"> | ikw<"FULL"> |
                   ikw<"OUTER"> | ikw<"INNER"> | ikw<"CROSS">)) +
               ikw<"JOIN">)) +
             assign<&ast::JoinStep::right>(ref<TableItemRule>) +
             option((ikw<"ON"> +
                     assign<&ast::JoinStep::onExpr>(ref<Expression>))  //
                    | (ikw<"USING"> + lit<"("> +
                       append<&ast::JoinStep::usingColumns>(ref<ColumnName>) +
                       many(lit<","> + append<&ast::JoinStep::usingColumns>(
                                           ref<ColumnName>)) +
                       lit<")">))> {};

  struct JoinClauseRule
      : rule<assign<&ast::JoinClause::first>(ref<TableItemRule>) +
             many(append<&ast::JoinClause::steps>(ref<JoinStepRule>))> {};

  struct NamedWindowRule
      : rule<assign<&ast::NamedWindow::name>(ref<WindowName>) + ikw<"AS"> +
             assign<&ast::NamedWindow::defn>(ref<WindowDefnRule>)> {};

  struct SelectCoreRule
      : rule<(ikw<"SELECT"> +
              option(flag<&ast::SelectCore::distinct>(ikw<"DISTINCT">) |
                     ikw<"ALL">) +
              append<&ast::SelectCore::columns>(ref<ResultColumnRule>) +
              many(lit<","> +
                   append<&ast::SelectCore::columns>(ref<ResultColumnRule>)) +
              option(ikw<"FROM"> +
                     assign<&ast::SelectCore::from>(ref<JoinClauseRule>)) +
              option(ikw<"WHERE"> +
                     assign<&ast::SelectCore::where>(ref<Expression>)) +
              option(ikw<"GROUP"> + ikw<"BY"> +
                     append<&ast::SelectCore::groupBy>(ref<Expression>) +
                     many(lit<","> +
                          append<&ast::SelectCore::groupBy>(ref<Expression>))) +
              option(ikw<"HAVING"> +
                     assign<&ast::SelectCore::having>(ref<Expression>)) +
              option(ikw<"WINDOW"> +
                     append<&ast::SelectCore::windows>(ref<NamedWindowRule>) +
                     many(lit<","> + append<&ast::SelectCore::windows>(
                                         ref<NamedWindowRule>))))  //
             | (ikw<"VALUES"> +
                append<&ast::SelectCore::values>(ref<ValueRowRule>) +
                many(lit<","> +
                     append<&ast::SelectCore::values>(ref<ValueRowRule>)))> {};

  struct CompoundUnionAll
      : rule<ikw<"UNION"> + ikw<"ALL">, value<ast::CompoundOp::union_all>> {};
  struct CompoundUnion : rule<ikw<"UNION">, value<ast::CompoundOp::union_>> {};
  struct CompoundIntersect
      : rule<ikw<"INTERSECT">, value<ast::CompoundOp::intersect>> {};
  struct CompoundExcept : rule<ikw<"EXCEPT">, value<ast::CompoundOp::except>> {
  };

  struct FirstCompoundPartRule
      : rule<assign<&ast::CompoundPart::core>(ref<SelectCoreRule>)> {};
  struct CompoundPartRule
      : rule<assign<&ast::CompoundPart::op>(
                 ref<CompoundUnionAll> | ref<CompoundUnion> |
                 ref<CompoundIntersect> | ref<CompoundExcept>) +
             assign<&ast::CompoundPart::core>(ref<SelectCoreRule>)> {};

  struct SelectRule
      : rule<option(
                 ikw<"WITH"> +
                 option(flag<&ast::Select::recursive>(ikw<"RECURSIVE">)) +
                 append<&ast::Select::ctes>(ref<CommonTableExprRule>) +
                 many(lit<","> +
                      append<&ast::Select::ctes>(ref<CommonTableExprRule>))) +
             append<&ast::Select::parts>(ref<FirstCompoundPartRule>) +
             many(append<&ast::Select::parts>(ref<CompoundPartRule>)) +
             option(ikw<"ORDER"> + ikw<"BY"> +
                    append<&ast::Select::orderBy>(ref<OrderTermRule>) +
                    many(lit<","> +
                         append<&ast::Select::orderBy>(ref<OrderTermRule>))) +
             option(ikw<"LIMIT"> +
                    ((assign<&ast::Select::offset>(ref<Expression>) + lit<","> +
                      assign<&ast::Select::limit>(ref<Expression>)) |
                     (assign<&ast::Select::limit>(ref<Expression>) +
                      option(ikw<"OFFSET"> + assign<&ast::Select::offset>(
                                                 ref<Expression>)))))> {};

  // -- DML ----
  struct QualifiedTableNameRule
      : rule<option(assign<&ast::QualifiedTableName::schema>(ref<Name>) +
                    lit<".">) +
             assign<&ast::QualifiedTableName::name>(ref<Name>) +
             option(ikw<"AS"> +
                    assign<&ast::QualifiedTableName::alias>(ref<ColumnName>)) +
             option((ikw<"INDEXED"> + ikw<"BY"> +
                     assign<&ast::QualifiedTableName::indexedBy>(ref<Name>)) |
                    flag<&ast::QualifiedTableName::notIndexed>(
                        ikw<"NOT"> + ikw<"INDEXED">))> {};

  struct UpsertAssignmentRule
      : rule<(append<&ast::UpsertAssignment::columns>(ref<ColumnName>)  //
              | (lit<"("> +
                 append<&ast::UpsertAssignment::columns>(ref<ColumnName>) +
                 many(lit<","> + append<&ast::UpsertAssignment::columns>(
                                     ref<ColumnName>)) +
                 lit<")">)) +
             lit<"="> +
             assign<&ast::UpsertAssignment::value>(ref<Expression>)> {};

  struct UpsertClauseRule
      : rule<ikw<"ON"> + ikw<"CONFLICT"> +
             option(
                 lit<"("> +
                 append<&ast::UpsertClause::target>(ref<IndexedColumnRule>) +
                 many(lit<","> + append<&ast::UpsertClause::target>(
                                     ref<IndexedColumnRule>)) +
                 lit<")"> +
                 option(ikw<"WHERE"> + assign<&ast::UpsertClause::targetWhere>(
                                           ref<Expression>))) +
             ikw<"DO"> +
             (flag<&ast::UpsertClause::doNothing>(ikw<"NOTHING">)  //
              | (ikw<"UPDATE"> + ikw<"SET"> +
                 append<&ast::UpsertClause::assignments>(
                     ref<UpsertAssignmentRule>) +
                 many(lit<","> + append<&ast::UpsertClause::assignments>(
                                     ref<UpsertAssignmentRule>)) +
                 option(ikw<"WHERE"> + assign<&ast::UpsertClause::updateWhere>(
                                           ref<Expression>))))> {};

  struct InsModeOrReplace : rule<ikw<"INSERT"> + ikw<"OR"> + ikw<"REPLACE">,
                                 value<ast::InsertMode::insert_or_replace>> {};
  struct InsModeOrRollback : rule<ikw<"INSERT"> + ikw<"OR"> + ikw<"ROLLBACK">,
                                  value<ast::InsertMode::insert_or_rollback>> {
  };
  struct InsModeOrAbort : rule<ikw<"INSERT"> + ikw<"OR"> + ikw<"ABORT">,
                               value<ast::InsertMode::insert_or_abort>> {};
  struct InsModeOrFail : rule<ikw<"INSERT"> + ikw<"OR"> + ikw<"FAIL">,
                              value<ast::InsertMode::insert_or_fail>> {};
  struct InsModeOrIgnore : rule<ikw<"INSERT"> + ikw<"OR"> + ikw<"IGNORE">,
                                value<ast::InsertMode::insert_or_ignore>> {};
  struct InsModeInsert : rule<ikw<"INSERT">, value<ast::InsertMode::insert>> {};
  struct InsModeReplace
      : rule<ikw<"REPLACE">, value<ast::InsertMode::replace>> {};

  struct InsertStmtRule
      : rule<option(ikw<"WITH"> +
                    option(flag<&ast::InsertStmt::recursive>(
                        ikw<"RECURSIVE">)) +
                    append<&ast::InsertStmt::ctes>(ref<CommonTableExprRule>) +
                    many(lit<","> + append<&ast::InsertStmt::ctes>(
                                        ref<CommonTableExprRule>))) +
             assign<&ast::InsertStmt::mode>(
                 ref<InsModeOrReplace> | ref<InsModeOrRollback> |
                 ref<InsModeOrAbort> | ref<InsModeOrFail> |
                 ref<InsModeOrIgnore> | ref<InsModeInsert> |
                 ref<InsModeReplace>) +
             ikw<"INTO"> +
             option(assign<&ast::InsertStmt::schema>(ref<Name>) + lit<".">) +
             assign<&ast::InsertStmt::table>(ref<Name>) +
             option(ikw<"AS"> +
                    assign<&ast::InsertStmt::alias>(ref<ColumnName>)) +
             option(lit<"("> +
                    append<&ast::InsertStmt::columns>(ref<ColumnName>) +
                    many(lit<","> +
                         append<&ast::InsertStmt::columns>(ref<ColumnName>)) +
                    lit<")">) +
             ((assign<&ast::InsertStmt::query>(ref<SelectRule>) +
               many(append<&ast::InsertStmt::upserts>(
                   ref<UpsertClauseRule>)))  //
              | flag<&ast::InsertStmt::useDefaultValues>(ikw<"DEFAULT"> +
                                                              ikw<"VALUES">)) +
             option(ikw<"RETURNING"> +
                    append<&ast::InsertStmt::returning>(ref<ResultColumnRule>) +
                    many(lit<","> + append<&ast::InsertStmt::returning>(
                                        ref<ResultColumnRule>)))> {};

  struct UpdateStmtRule
      : rule<option(ikw<"WITH"> +
                    option(flag<&ast::UpdateStmt::recursive>(
                        ikw<"RECURSIVE">)) +
                    append<&ast::UpdateStmt::ctes>(ref<CommonTableExprRule>) +
                    many(lit<","> + append<&ast::UpdateStmt::ctes>(
                                        ref<CommonTableExprRule>))) +
             ikw<"UPDATE"> +
             option(ikw<"OR"> +
                    assign<&ast::UpdateStmt::conflict>(ConflictActionAlt)) +
             assign<&ast::UpdateStmt::target>(ref<QualifiedTableNameRule>) +
             ikw<"SET"> +
             append<&ast::UpdateStmt::assignments>(ref<UpsertAssignmentRule>) +
             many(lit<","> + append<&ast::UpdateStmt::assignments>(
                                 ref<UpsertAssignmentRule>)) +
             option(ikw<"FROM"> +
                    assign<&ast::UpdateStmt::from>(ref<JoinClauseRule>)) +
             option(ikw<"WHERE"> +
                    assign<&ast::UpdateStmt::where>(ref<Expression>)) +
             option(ikw<"RETURNING"> +
                    append<&ast::UpdateStmt::returning>(ref<ResultColumnRule>) +
                    many(lit<","> + append<&ast::UpdateStmt::returning>(
                                        ref<ResultColumnRule>))) +
             option(ikw<"ORDER"> + ikw<"BY"> +
                    append<&ast::UpdateStmt::orderBy>(ref<OrderTermRule>) +
                    many(lit<","> + append<&ast::UpdateStmt::orderBy>(
                                        ref<OrderTermRule>))) +
             option(ikw<"LIMIT"> +
                    ((assign<&ast::UpdateStmt::offset>(ref<Expression>) +
                      lit<","> +
                      assign<&ast::UpdateStmt::limit>(ref<Expression>)) |
                     (assign<&ast::UpdateStmt::limit>(ref<Expression>) +
                      option(ikw<"OFFSET"> + assign<&ast::UpdateStmt::offset>(
                                                 ref<Expression>)))))> {};

  struct DeleteStmtRule
      : rule<option(ikw<"WITH"> +
                    option(flag<&ast::DeleteStmt::recursive>(
                        ikw<"RECURSIVE">)) +
                    append<&ast::DeleteStmt::ctes>(ref<CommonTableExprRule>) +
                    many(lit<","> + append<&ast::DeleteStmt::ctes>(
                                        ref<CommonTableExprRule>))) +
             ikw<"DELETE"> + ikw<"FROM"> +
             assign<&ast::DeleteStmt::target>(ref<QualifiedTableNameRule>) +
             option(ikw<"WHERE"> +
                    assign<&ast::DeleteStmt::where>(ref<Expression>)) +
             option(ikw<"RETURNING"> +
                    append<&ast::DeleteStmt::returning>(ref<ResultColumnRule>) +
                    many(lit<","> + append<&ast::DeleteStmt::returning>(
                                        ref<ResultColumnRule>))) +
             option(ikw<"ORDER"> + ikw<"BY"> +
                    append<&ast::DeleteStmt::orderBy>(ref<OrderTermRule>) +
                    many(lit<","> + append<&ast::DeleteStmt::orderBy>(
                                        ref<OrderTermRule>))) +
             option(ikw<"LIMIT"> +
                    ((assign<&ast::DeleteStmt::offset>(ref<Expression>) +
                      lit<","> +
                      assign<&ast::DeleteStmt::limit>(ref<Expression>)) |
                     (assign<&ast::DeleteStmt::limit>(ref<Expression>) +
                      option(ikw<"OFFSET"> + assign<&ast::DeleteStmt::offset>(
                                                 ref<Expression>)))))> {};

  struct PragmaValueRule
      : rule<ref<SignedNumber> | ref<StringLit> | ref<NameLitRule>> {};
  struct PragmaStmtRule
      : rule<ikw<"PRAGMA"> +
             option(assign<&ast::PragmaStmt::schema>(ref<Name>) + lit<".">) +
             assign<&ast::PragmaStmt::name>(ref<Name>) +
             option((lit<"="> +
                     assign<&ast::PragmaStmt::value>(ref<PragmaValueRule>)) |
                    (lit<"("> +
                     assign<&ast::PragmaStmt::value>(ref<PragmaValueRule>) +
                     lit<")">))> {};

  struct VacuumStmtRule
      : rule<ikw<"VACUUM"> +
             option(assign<&ast::VacuumStmt::schema>(ref<Name>)) +
             option(ikw<"INTO"> +
                    assign<&ast::VacuumStmt::intoFile>(ref<Expression>))> {};

  struct AttachStmtRule
      : rule<ikw<"ATTACH"> + option(ikw<"DATABASE">) +
             assign<&ast::AttachStmt::file>(ref<Expression>) + ikw<"AS"> +
             assign<&ast::AttachStmt::schemaName>(ref<Name>)> {};

  struct DetachStmtRule : rule<ikw<"DETACH"> + option(ikw<"DATABASE">) +
                               assign<&ast::DetachStmt::schema>(ref<Name>)> {};

  struct TxDeferred : rule<ikw<"DEFERRED">, value<ast::TxKind::deferred>> {};
  struct TxImmediate : rule<ikw<"IMMEDIATE">, value<ast::TxKind::immediate>> {};
  struct TxExclusive : rule<ikw<"EXCLUSIVE">, value<ast::TxKind::exclusive>> {};
  struct BeginStmtRule
      : rule<ikw<"BEGIN"> +
             option(assign<&ast::BeginStmt::kind>(
                 ref<TxDeferred> | ref<TxImmediate> | ref<TxExclusive>)) +
             option(ikw<"TRANSACTION"> +
                    option(assign<&ast::BeginStmt::name>(ref<Name>)))> {};

  struct CommitStmtRule
      : rule<(ikw<"COMMIT"> |
              flag<&ast::CommitStmt::usingEnd>(ikw<"END">)) +
             option(ikw<"TRANSACTION"> +
                    option(assign<&ast::CommitStmt::name>(ref<Name>)))> {};

  struct RollbackStmtRule
      : rule<ikw<"ROLLBACK"> +
             option(ikw<"TRANSACTION"> +
                    option(assign<&ast::RollbackStmt::name>(ref<Name>))) +
             option(ikw<"TO"> + option(ikw<"SAVEPOINT">) +
                    assign<&ast::RollbackStmt::savepoint>(ref<Name>))> {};

  struct SavepointStmtRule
      : rule<ikw<"SAVEPOINT"> + assign<&ast::SavepointStmt::name>(ref<Name>)> {
  };

  struct ReleaseStmtRule : rule<ikw<"RELEASE"> + option(ikw<"SAVEPOINT">) +
                                assign<&ast::ReleaseStmt::name>(ref<Name>)> {};

  struct AnalyzeStmtRule
      : rule<ikw<"ANALYZE"> +
             option(option(assign<&ast::AnalyzeStmt::schema>(ref<Name>) +
                           lit<".">) +
                    assign<&ast::AnalyzeStmt::name>(ref<Name>))> {};

  // A bare name is ambiguous between a collation and a table/index; both
  // fold into schema/name here.
  struct ReindexStmtRule
      : rule<ikw<"REINDEX"> +
             option(option(assign<&ast::ReindexStmt::schema>(ref<Name>) +
                           lit<".">) +
                    assign<&ast::ReindexStmt::name>(ref<Name>))> {};

  struct DropIndexTag : rule<ikw<"INDEX">, value<ast::DropObjectKind::index>> {
  };
  struct DropTableTag : rule<ikw<"TABLE">, value<ast::DropObjectKind::table>> {
  };
  struct DropTriggerTag
      : rule<ikw<"TRIGGER">, value<ast::DropObjectKind::trigger>> {};
  struct DropViewTag : rule<ikw<"VIEW">, value<ast::DropObjectKind::view>> {};
  struct DropStmtRule
      : rule<ikw<"DROP"> +
             assign<&ast::DropStmt::object>(
                 ref<DropIndexTag> | ref<DropTableTag> | ref<DropTriggerTag> |
                 ref<DropViewTag>) +
             option(flag<&ast::DropStmt::ifExists>(ikw<"IF"> +
                                                        ikw<"EXISTS">)) +
             option(assign<&ast::DropStmt::schema>(ref<Name>) + lit<".">) +
             assign<&ast::DropStmt::name>(ref<Name>)> {};

  struct CreateIndexStmtRule
      : rule<ikw<"CREATE"> +
             option(flag<&ast::CreateIndexStmt::unique>(ikw<"UNIQUE">)) +
             ikw<"INDEX"> +
             option(flag<&ast::CreateIndexStmt::ifNotExists>(
                 ikw<"IF"> + ikw<"NOT"> + ikw<"EXISTS">)) +
             option(assign<&ast::CreateIndexStmt::schema>(ref<Name>) +
                    lit<".">) +
             assign<&ast::CreateIndexStmt::name>(ref<Name>) + ikw<"ON"> +
             assign<&ast::CreateIndexStmt::table>(ref<Name>) + lit<"("> +
             append<&ast::CreateIndexStmt::columns>(ref<IndexedColumnRule>) +
             many(lit<","> + append<&ast::CreateIndexStmt::columns>(
                                 ref<IndexedColumnRule>)) +
             lit<")"> +
             option(ikw<"WHERE"> +
                    assign<&ast::CreateIndexStmt::where>(ref<Expression>))> {};

  struct CreateTableStmtRule
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
              append<&ast::CreateTableStmt::columns>(ref<ColumnDefRule>) +
              many(lit<","> +
                   append<&ast::CreateTableStmt::columns>(ref<ColumnDefRule>)) +
              option(lit<","> +
                     append<&ast::CreateTableStmt::constraints>(
                         ref<TableConstraintRule>) +
                     many(option(lit<",">) +
                          append<&ast::CreateTableStmt::constraints>(
                              ref<TableConstraintRule>))) +
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
                assign<&ast::CreateTableStmt::asSelect>(ref<SelectRule>)))> {};

  struct TrigBefore : rule<ikw<"BEFORE">, value<ast::TriggerWhen::before>> {};
  struct TrigAfter : rule<ikw<"AFTER">, value<ast::TriggerWhen::after>> {};
  struct TrigInsteadOf
      : rule<ikw<"INSTEAD"> + ikw<"OF">, value<ast::TriggerWhen::instead_of>> {
  };
  struct TrigDelete : rule<ikw<"DELETE">, value<ast::TriggerEvent::delete_>> {};
  struct TrigInsert : rule<ikw<"INSERT">, value<ast::TriggerEvent::insert>> {};
  struct TrigUpdate : rule<ikw<"UPDATE">, value<ast::TriggerEvent::update>> {};

  struct CreateTriggerStmtRule
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
                 ref<TrigBefore> | ref<TrigAfter> | ref<TrigInsteadOf>)) +
             ((assign<&ast::CreateTriggerStmt::event>(ref<TrigDelete>))    //
              | (assign<&ast::CreateTriggerStmt::event>(ref<TrigInsert>))  //
              | (assign<&ast::CreateTriggerStmt::event>(ref<TrigUpdate>) +
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
                      ref<UpdateStmtRule> | ref<InsertStmtRule> |
                      ref<DeleteStmtRule> | ref<SelectRule>) +
                  lit<";">) +
             ikw<"END">> {};

  struct CreateViewStmtRule
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
             ikw<"AS"> + assign<&ast::CreateViewStmt::query>(ref<SelectRule>)> {
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

  struct CreateVirtualTableStmtRule
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

  struct AAKindRenameTable
      : rule<ikw<"RENAME"> + ikw<"TO">,
             value<ast::AlterTableStmt::Action::rename_table>> {};
  struct AAKindRenameColumn
      : rule<ikw<"RENAME"> + option(ikw<"COLUMN">),
             value<ast::AlterTableStmt::Action::rename_column>> {};
  struct AAKindAddColumn
      : rule<ikw<"ADD"> + option(ikw<"COLUMN">),
             value<ast::AlterTableStmt::Action::add_column>> {};
  struct AAKindAddCheck
      : rule<ikw<"ADD">, value<ast::AlterTableStmt::Action::add_check>> {};
  struct AAKindDropColumn
      : rule<ikw<"DROP"> + option(ikw<"COLUMN">),
             value<ast::AlterTableStmt::Action::drop_column>> {};
  struct AAKindDropConstraint
      : rule<ikw<"DROP"> + ikw<"CONSTRAINT">,
             value<ast::AlterTableStmt::Action::drop_constraint>> {};
  struct AAKindSetNotNull
      : rule<ikw<"SET"> + ikw<"NOT"> + ikw<"NULL">,
             value<ast::AlterTableStmt::Action::alter_set_not_null>> {};
  struct AAKindDropNotNull
      : rule<ikw<"DROP"> + ikw<"NOT"> + ikw<"NULL">,
             value<ast::AlterTableStmt::Action::alter_drop_not_null>> {};

  struct AlterTableStmtRule
      : rule<
            ikw<"ALTER"> + ikw<"TABLE"> +
            option(assign<&ast::AlterTableStmt::schema>(ref<Name>) + lit<".">) +
            assign<&ast::AlterTableStmt::table>(ref<Name>) +
            ((assign<&ast::AlterTableStmt::action>(ref<AAKindRenameTable>) +
              assign<&ast::AlterTableStmt::newName>(ref<Name>))  //
             |
             (assign<&ast::AlterTableStmt::action>(ref<AAKindRenameColumn>) +
              assign<&ast::AlterTableStmt::targetName>(ref<Name>) + ikw<"TO"> +
              assign<&ast::AlterTableStmt::newName>(ref<Name>))  //
             |
             (assign<&ast::AlterTableStmt::action>(ref<AAKindAddColumn>) +
              assign<&ast::AlterTableStmt::addedColumn>(ref<ColumnDefRule>))  //
             |
             (assign<&ast::AlterTableStmt::action>(ref<AAKindAddCheck>) +
              option(ikw<"CONSTRAINT"> +
                     assign<&ast::AlterTableStmt::constraintName>(ref<Name>)) +
              ikw<"CHECK"> + lit<"("> +
              assign<&ast::AlterTableStmt::addedCheck>(ref<Expression>) +
              lit<")"> +
              option(ikw<"ON"> + ikw<"CONFLICT"> +
                     assign<&ast::AlterTableStmt::conflict>(
                         ConflictActionAlt)))  //
             | (assign<&ast::AlterTableStmt::action>(ref<AAKindDropColumn>) +
                assign<&ast::AlterTableStmt::targetName>(ref<ColumnName>))  //
             |
             (assign<&ast::AlterTableStmt::action>(ref<AAKindDropConstraint>) +
              assign<&ast::AlterTableStmt::constraintName>(ref<Name>))  //
             | (ikw<"ALTER"> + option(ikw<"COLUMN">) +
                assign<&ast::AlterTableStmt::targetName>(ref<ColumnName>) +
                ((assign<&ast::AlterTableStmt::action>(ref<AAKindSetNotNull>) +
                  option(ikw<"ON"> + ikw<"CONFLICT"> +
                         assign<&ast::AlterTableStmt::conflict>(
                             ConflictActionAlt)))  //
                 | assign<&ast::AlterTableStmt::action>(
                       ref<AAKindDropNotNull>))))> {};

  struct SqlStmtRule
      : rule<option(flag<&ast::SqlStmt::explain>(ikw<"EXPLAIN">) +
                    option(flag<&ast::SqlStmt::queryPlan>(ikw<"QUERY"> +
                                                               ikw<"PLAN">))) +
             assign<&ast::SqlStmt::statement>(
                 ref<AlterTableStmtRule> | ref<AnalyzeStmtRule> |
                 ref<AttachStmtRule> | ref<BeginStmtRule> |
                 ref<CommitStmtRule> | ref<CreateIndexStmtRule> |
                 ref<CreateTableStmtRule> | ref<CreateTriggerStmtRule> |
                 ref<CreateViewStmtRule> | ref<CreateVirtualTableStmtRule> |
                 ref<DeleteStmtRule> | ref<DetachStmtRule> | ref<DropStmtRule> |
                 ref<InsertStmtRule> | ref<PragmaStmtRule> |
                 ref<ReindexStmtRule> | ref<ReleaseStmtRule> |
                 ref<RollbackStmtRule> | ref<SavepointStmtRule> |
                 ref<SelectRule> | ref<UpdateStmtRule> | ref<VacuumStmtRule>)> {
  };

  struct ScriptRule
      : rule<many(append<&ast::Script::statements>(ref<SqlStmtRule>) |
                  lit<";">)> {};

  using entry = ScriptRule;
  using ignore = WS;
  using hidden = list<LINE_COMMENT, BLOCK_COMMENT>;
};

}  // namespace sqlite
