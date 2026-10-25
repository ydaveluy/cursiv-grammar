// The informix language: its grammar. Informix 4GL: a screen-oriented
// procedural language with embedded SQL. Keywords are case-insensitive;
// identifiers are ASCII lowercase, digits and underscore. `;` and EOL are
// always optional trivia, never a mandatory statement separator.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "informix/core/ast.hpp"

namespace informix {

inline constexpr auto unquote =
    [](std::string_view t, std::pmr::memory_resource*) {
      return t.substr(1, t.size() - 2);
    };
using Unquoted = cursiv::text_conv<unquote>;

struct Informix : cursiv::GrammarSpec {
  // ---------------------------------------------------------------- Lexer

  struct WS : terminal<some(cr<R"( \t\r\n;)">)> {};
  struct SL_COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};
  struct SL_COMMENT2 : terminal<lit<"--"> + many(cr<R"(^\r\n)">)> {};

  struct ID : terminal<cr<"A-Za-z_"> + many(cr<"A-Za-z_0-9">)> {};

  struct UINT : terminal<some(d)> {};
  struct NUMBER
      : terminal<((some(d) + option(lit<"."> + many(d))) |
                  (lit<"."> + some(d))) +
                     option(cr<"eE"> + option(cr<"+-">) + some(d)),
                 double> {};

  struct STRING_DQ
      : terminal<lit<"\""> + many(cr<R"(^"\\)">) + lit<"\"">, Unquoted> {};
  struct STRING_SQ
      : terminal<lit<"'"> + many(cr<R"(^'\\)">) + lit<"'">, Unquoted> {};

  // ----------------------------------------------------------- Soft words

  // constantIdentifier's keyword list, a name everywhere but where the
  // state reads it as the word itself. NOT and NULL are excluded: the
  // reference lists them too, but admitting them as plain names collides
  // with their use as operator/literal keywords through most of the
  // grammar for no realistic gain.
  static constexpr auto SoftKeyword =
      ikw<"ACCEPT"> | ikw<"ASCII"> | ikw<"COUNT"> | ikw<"CURRENT"> |
      ikw<"FALSE"> | ikw<"FIRST"> | ikw<"FOUND"> | ikw<"GROUP"> |
      ikw<"HIDE"> | ikw<"INDEX"> | ikw<"INT_FLAG"> | ikw<"INTERRUPT"> |
      ikw<"LAST"> | ikw<"LENGTH"> | ikw<"LINENO"> | ikw<"MDY"> | ikw<"NO"> |
      ikw<"NOTFOUND"> | ikw<"PAGENO"> | ikw<"REAL"> | ikw<"SIZE"> |
      ikw<"SQL"> | ikw<"STATUS"> | ikw<"TEMP"> | ikw<"TIME"> |
      ikw<"TODAY"> | ikw<"TRUE"> | ikw<"USER"> | ikw<"WAIT"> |
      ikw<"WEEKDAY"> | ikw<"WORK">;
  static constexpr auto Name = ref<ID> | SoftKeyword;

  // functionIdentifier's own keyword set, beyond SoftKeyword.
  static constexpr auto FuncKeyword =
      ikw<"DAY"> | ikw<"YEAR"> | ikw<"MONTH"> | ikw<"EXTEND"> |
      ikw<"INFIELD"> | ikw<"PREPARE"> | ikw<"DECODE"> | ikw<"NVL"> |
      ikw<"COLUMN"> | ikw<"SUM"> | ikw<"AVG"> | ikw<"MIN"> | ikw<"MAX"> |
      ikw<"MOD"> | ikw<"DATE">;
  static constexpr auto FuncName = Name | FuncKeyword;

  // -------------------------------------------------------------- Operators

  struct OP_MUL : terminal<lit<"*">, value<ast::Op::multiply>> {};
  struct OP_DIV : terminal<lit<"/">, value<ast::Op::divide>> {};
  struct OP_INTDIV : terminal<ikw<"DIV">, value<ast::Op::int_div>> {};
  struct OP_MOD : terminal<ikw<"MOD">, value<ast::Op::mod>> {};
  struct OP_ADD : terminal<lit<"+">, value<ast::Op::add>> {};
  struct OP_SUB : terminal<lit<"-">, value<ast::Op::subtract>> {};
  struct OP_CONCAT : terminal<lit<"||">, value<ast::Op::concat>> {};
  struct OP_NE
      : terminal<lit<"<>"> | lit<"!="> | lit<"^=">, value<ast::Op::ne>> {};
  struct OP_LE : terminal<lit<"<=">, value<ast::Op::le>> {};
  struct OP_GE : terminal<lit<">=">, value<ast::Op::ge>> {};
  struct OP_EQ : terminal<lit<"=">, value<ast::Op::eq>> {};
  struct OP_LT : terminal<lit<"<">, value<ast::Op::lt>> {};
  struct OP_GT : terminal<lit<">">, value<ast::Op::gt>> {};
  struct OP_AND : terminal<ikw<"AND">, value<ast::Op::logical_and>> {};
  struct OP_OR : terminal<ikw<"OR">, value<ast::Op::logical_or>> {};

  static constexpr auto OpMul =
      ref<OP_MUL> | ref<OP_DIV> | ref<OP_INTDIV> | ref<OP_MOD>;
  static constexpr auto OpAdd = ref<OP_ADD> | ref<OP_SUB>;
  static constexpr auto OpConcat = ref<OP_CONCAT>;
  static constexpr auto OpCompare = ref<OP_NE> | ref<OP_LE> | ref<OP_GE> |
                                    ref<OP_EQ> | ref<OP_LT> | ref<OP_GT>;
  static constexpr auto OpAnd = ref<OP_AND>;
  static constexpr auto OpOr = ref<OP_OR>;

  struct CmpNe : rule<ref<OP_NE>, value<ast::CompareOp::ne>> {};
  struct CmpLe : rule<ref<OP_LE>, value<ast::CompareOp::le>> {};
  struct CmpGe : rule<ref<OP_GE>, value<ast::CompareOp::ge>> {};
  struct CmpEq : rule<ref<OP_EQ>, value<ast::CompareOp::eq>> {};
  struct CmpLt : rule<ref<OP_LT>, value<ast::CompareOp::lt>> {};
  struct CmpGt : rule<ref<OP_GT>, value<ast::CompareOp::gt>> {};

  // ------------------------------------------------------------------ Expr

  struct Expression;
  struct Unary;
  struct ArithConcat;
  struct Postfix;
  struct Atom;
  struct ComponentRef;
  struct Primary;
  struct FuncCall;
  struct SelectStmt;
  struct UnionArm;
  struct FromItem;
  struct Statement;

  struct NullLitR : rule<create<ast::NullLit> + ikw<"NULL">> {};
  struct BoolLitR
      : rule<create<ast::BoolLit> +
             (flag<&ast::BoolLit::value>(ikw<"TRUE">) |
              ikw<"FALSE">)> {};
  struct NumberLitR : rule<assign<&ast::NumberLit::value>(ref<NUMBER>)> {};
  struct StringLitR
      : rule<assign<&ast::StringLit::value>(ref<STRING_DQ> |
                                            ref<STRING_SQ>)> {};
  struct StarExprR : rule<create<ast::StarExpr> + lit<"*">> {};

  // A signed numeric literal: array/type sizes, `HELP`/margin numbers.
  struct SignedNumber
      : rule<option(flag<&ast::NumberLit::negative>(lit<"-">) |
                    lit<"+">) +
             assign<&ast::NumberLit::value>(ref<NUMBER>)> {};
  static constexpr auto SignedNumberText =
      option(lit<"-"> | lit<"+">) + ref<NUMBER>;

  struct Primary : rule<assign<&ast::NameRef::name>(Name)> {};

  struct ComponentRef
      : rule<ref<Primary> +
             many(nest<ast::IndexExpr, &ast::IndexExpr::base>(
                      lit<"["> +
                      some(append<&ast::IndexExpr::indices>(
                               ref<Expression>),
                           lit<",">) +
                      lit<"]">) |
                  nest<ast::AllFieldsRef, &ast::AllFieldsRef::base>(
                      lit<"."> + lit<"*">) |
                  nest<ast::NameRef, &ast::NameRef::base>(
                      lit<"."> + assign<&ast::NameRef::name>(Name))) +
             option(nest<ast::FieldRange, &ast::FieldRange::from>(
                        (ikw<"THROUGH"> | ikw<"THRU">) +
                        option(flag<&ast::FieldRange::same>(
                            ikw<"SAME"> + lit<".">)) +
                        assign<&ast::FieldRange::to>(ref<ComponentRef>)))> {
  };

  static constexpr auto FuncArgs =
      lit<"("> +
      option(flag<&ast::FuncCall::star>(lit<"*">) |
             (option(flag<&ast::FuncCall::distinct>(ikw<"DISTINCT">) |
                     flag<&ast::FuncCall::all>(ikw<"ALL">)) +
              some(append<&ast::FuncCall::args>(ref<Expression>),
                   lit<",">))) +
      lit<")">;

  // FuncKeyword's own keywords never denote a plain name, so their call
  // parens stay optional. The Name-overlapping keywords (COUNT, LENGTH,
  // TIME, TODAY, ...) keep parens mandatory: bare, they already parse as a
  // name, and optional parens there would make every occurrence ambiguous
  // between a call and a name.
  struct FuncCall
      : rule<option(flag<&ast::FuncCall::group>(ikw<"GROUP">)) +
             ((assign<&ast::FuncCall::name>(FuncKeyword) +
               option(FuncArgs)) |
              (assign<&ast::FuncCall::name>(Name) + FuncArgs))> {};

  struct SubqueryExprR
      : rule<lit<"("> +
             assign<&ast::SubqueryExpr::select>(ref<SelectStmt>) +
             lit<")">> {};

  // sqlFactor2's row-value form: `(a, b, ...)`, two or more items -- one
  // item alone is the plain grouping alternative below instead.
  struct TupleExprR
      : rule<create<ast::TupleExpr>(
                 lit<"("> + append<&ast::TupleExpr::items>(ref<Expression>) +
                 some(lit<","> +
                          append<&ast::TupleExpr::items>(ref<Expression>)) +
                 lit<")">)> {};

  struct Atom : rule<(lit<"("> + ref<Expression> + lit<")">)  //
                     | ref<NullLitR>                          //
                     | ref<BoolLitR>                          //
                     | ref<NumberLitR>                        //
                     | ref<StringLitR>                        //
                     | ref<StarExprR>                         //
                     | ref<FuncCall>                          //
                     | ref<SubqueryExprR>                     //
                     | ref<TupleExprR>                        //
                     | ref<ComponentRef>> {};

  struct UnitYear : rule<ikw<"YEAR">, value<ast::TimeUnit::year>> {};
  struct UnitMonth : rule<ikw<"MONTH">, value<ast::TimeUnit::month>> {};
  struct UnitDay : rule<ikw<"DAY">, value<ast::TimeUnit::day>> {};
  struct UnitHour : rule<ikw<"HOUR">, value<ast::TimeUnit::hour>> {};
  struct UnitMinute : rule<ikw<"MINUTE">, value<ast::TimeUnit::minute>> {};
  struct UnitSecond : rule<ikw<"SECOND">, value<ast::TimeUnit::second>> {};
  struct UnitFraction
      : rule<ikw<"FRACTION">, value<ast::TimeUnit::fraction>> {};
  static constexpr auto AnyUnit = ref<UnitYear> | ref<UnitMonth> |
                                  ref<UnitDay> | ref<UnitHour> |
                                  ref<UnitMinute> | ref<UnitSecond> |
                                  ref<UnitFraction>;

  struct UnitsAtom
      : rule<ref<Atom> +
             option(nest<ast::UnitsExpr, &ast::UnitsExpr::operand>(
                 ikw<"UNITS"> + assign<&ast::UnitsExpr::unit>(AnyUnit)))> {
  };

  struct ExistsExprR
      : rule<create<ast::ExistsExpr>(
                 option(flag<&ast::ExistsExpr::negated>(ikw<"NOT">)) +
                 ikw<"EXISTS"> + lit<"("> +
                 assign<&ast::ExistsExpr::subquery>(ref<SelectStmt>) +
                 lit<")">)> {};

  struct QuantifiedExprR
      : rule<create<ast::QuantifiedExpr>(
                 assign<&ast::QuantifiedExpr::left>(ref<ArithConcat>) +
                 assign<&ast::QuantifiedExpr::op>(
                     ref<CmpNe> | ref<CmpLe> | ref<CmpGe> | ref<CmpEq> |
                     ref<CmpLt> | ref<CmpGt>) +
                 option(flag<&ast::QuantifiedExpr::all>(ikw<"ALL">) |
                        flag<&ast::QuantifiedExpr::any>(ikw<"ANY">)) +
                 lit<"("> +
                 assign<&ast::QuantifiedExpr::subquery>(ref<SelectStmt>) +
                 lit<")">)> {};

  struct Unary
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"-"> | lit<"+"> |
                                             ikw<"NOT">) +
                 assign<&ast::UnaryExpr::operand>(ref<Unary>)) |
             ref<UnitsAtom>> {};

  struct ArithConcat
      : infix<Unary, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(OpMul) >>      //
               left(OpAdd) >>      //
               left(OpConcat))> {};

  // Comparisons, LIKE/MATCHES, BETWEEN, IS NULL, EXISTS and the quantified
  // subquery forms all sit at one tier, above the arithmetic ladder: each
  // takes the full arithmetic/concatenation expression as its operand, not
  // a bare atom (`a + b BETWEEN c AND d` is `(a + b) BETWEEN c AND d`).
  struct Postfix
      : rule<create<ast::IsNullExpr>(
                 assign<&ast::IsNullExpr::operand>(ref<ArithConcat>) +
                 ikw<"IS"> +
                 option(flag<&ast::IsNullExpr::negated>(ikw<"NOT">)) +
                 ikw<"NULL">) |
             create<ast::BetweenExpr>(
                 assign<&ast::BetweenExpr::operand>(ref<ArithConcat>) +
                 option(flag<&ast::BetweenExpr::negated>(ikw<"NOT">)) +
                 ikw<"BETWEEN"> +
                 assign<&ast::BetweenExpr::low>(ref<ArithConcat>) +
                 ikw<"AND"> +
                 assign<&ast::BetweenExpr::high>(ref<ArithConcat>)) |
             create<ast::LikeExpr>(
                 assign<&ast::LikeExpr::operand>(ref<ArithConcat>) +
                 option(flag<&ast::LikeExpr::negated>(ikw<"NOT">)) +
                 (ikw<"LIKE"> |
                  flag<&ast::LikeExpr::matches>(ikw<"MATCHES">)) +
                 assign<&ast::LikeExpr::pattern>(ref<ArithConcat>)) |
             create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<ArithConcat>) +
                 assign<&ast::BinaryExpr::op>(OpCompare) +
                 assign<&ast::BinaryExpr::right>(ref<ArithConcat>)) |
             ref<ExistsExprR>       //
             | ref<QuantifiedExprR>  //
             | ref<ArithConcat>> {};

  struct Expression
      : infix<Postfix, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(OpAnd) >> left(OpOr))> {};

  static constexpr auto ActualParam = ref<StarExprR> | ref<Expression>;

  // ------------------------------------------------------------------ Types

  struct TypeSpec;
  struct VarGroup;

  static constexpr auto SizeParen =
      lit<"("> + assign<&ast::ScalarType::size1>(ref<UINT>) +
      option(lit<","> + assign<&ast::ScalarType::size2>(ref<UINT>)) +
      lit<")">;

  struct CharType
      : rule<(assign<&ast::ScalarType::keyword>(ikw<"VARCHAR"> |
                                                ikw<"NVARCHAR">) +
              SizeParen) |
             (assign<&ast::ScalarType::keyword>(
                  ikw<"CHAR"> | ikw<"NCHAR"> | ikw<"CHARACTER">) +
              option(lit<"("> + assign<&ast::ScalarType::size1>(ref<UINT>) +
                     lit<")">))> {};

  struct NumberType
      : rule<assign<&ast::ScalarType::keyword>(
                 ikw<"BIGINT"> | ikw<"INTEGER"> | ikw<"INT"> |
                 ikw<"SMALLINT"> | ikw<"SMALLFLOAT">) |
             (assign<&ast::ScalarType::keyword>(
                  ikw<"DECIMAL"> | ikw<"DEC"> | ikw<"NUMERIC"> |
                  ikw<"MONEY">) +
              option(SizeParen)) |
             (assign<&ast::ScalarType::keyword>(ikw<"FLOAT"> |
                                                ikw<"DOUBLE">) +
              option(lit<"("> +
                     assign<&ast::ScalarType::size1>(ref<UINT>) +
                     lit<")">))> {};

  static constexpr auto FromUnitExpr =
      assign<&ast::DateTimeType::from>(AnyUnit) +
      option(lit<"("> +
             assign<&ast::DateTimeType::fromPrecision>(ref<UINT>) +
             lit<")">);
  static constexpr auto ToUnitExpr =
      assign<&ast::DateTimeType::to>(AnyUnit) +
      option(lit<"("> + assign<&ast::DateTimeType::toPrecision>(ref<UINT>) +
             lit<")">);

  struct TimeType
      : rule<create<ast::DateTimeType>(
                 flag<&ast::DateTimeType::isDate>(ikw<"DATE">)) |
             create<ast::DateTimeType>(ikw<"DATETIME"> + FromUnitExpr +
                                       ikw<"TO"> + ToUnitExpr) |
             create<ast::DateTimeType>(
                 flag<&ast::DateTimeType::isInterval>(
                     ikw<"INTERVAL">) +
                 FromUnitExpr + ikw<"TO"> + ToUnitExpr)> {};

  struct LargeType
      : rule<assign<&ast::ScalarType::keyword>(ikw<"TEXT"> |
                                               ikw<"BYTE">)> {};

  struct IndirectType
      : rule<ikw<"LIKE"> + assign<&ast::LikeType::table>(Name) + lit<"."> +
             assign<&ast::LikeType::column>(Name)> {};

  struct ArrayDimsR
      : rule<create<ast::ArrayDims>(
                 lit<"["> + append<&ast::ArrayDims::sizes>(ref<UINT>) +
                 many(lit<","> + append<&ast::ArrayDims::sizes>(ref<UINT>)) +
                 lit<"]">)> {};

  struct RecordType
      : rule<ikw<"RECORD"> + ikw<"LIKE"> +
             assign<&ast::RecordType::likeTable>(Name) + lit<"."> +
             lit<"*">> {};

  struct RecordTypeBody
      : rule<ikw<"RECORD"> +
             some(append<&ast::RecordType::fields>(ref<VarGroup>),
                  lit<",">) +
             ikw<"END"> + ikw<"RECORD">> {};

  struct ArrayTypeR
      : rule<ikw<"ARRAY"> + assign<&ast::ArrayType::dims>(ref<ArrayDimsR>) +
             ikw<"OF"> + assign<&ast::ArrayType::element>(ref<TypeSpec>)> {};

  struct DynArrayTypeR
      : rule<ikw<"DYNAMIC"> + ikw<"ARRAY"> + ikw<"WITH"> +
             assign<&ast::DynArrayType::dimensions>(ref<UINT>) +
             ikw<"DIMENSIONS"> + ikw<"OF"> +
             assign<&ast::DynArrayType::element>(ref<TypeSpec>)> {};

  struct TypeSpec : rule<ref<CharType>          //
                         | ref<NumberType>       //
                         | ref<TimeType>         //
                         | ref<IndirectType>     //
                         | ref<LargeType>        //
                         | ref<RecordType>       //
                         | ref<RecordTypeBody>   //
                         | ref<ArrayTypeR>       //
                         | ref<DynArrayTypeR>> {};

  struct VarGroup
      : rule<create<ast::VarGroup>(
                 append<&ast::VarGroup::names>(Name) +
                 many(lit<","> + append<&ast::VarGroup::names>(Name)) +
                 assign<&ast::VarGroup::type>(ref<TypeSpec>)) |
             create<ast::VarGroup>(
                 append<&ast::VarGroup::names>(Name) +
                 assign<&ast::VarGroup::type>(ref<TypeSpec>))> {};

  struct TypeDeclR
      : rule<create<ast::TypeDecl>(
                 ikw<"DEFINE"> +
                 append<&ast::TypeDecl::groups>(ref<VarGroup>) +
                 many(lit<","> +
                      append<&ast::TypeDecl::groups>(ref<VarGroup>)))> {};

  // -------------------------------------------------------------- Statement

  struct LetStmtR
      : rule<create<ast::LetStmt>(
                 ikw<"LET"> +
                 assign<&ast::LetStmt::target>(ref<ComponentRef>) +
                 lit<"="> + append<&ast::LetStmt::values>(ref<Expression>) +
                 many(lit<","> +
                      append<&ast::LetStmt::values>(ref<Expression>)))> {};

  struct CallStmtR
      : rule<create<ast::CallStmt>(
                 ikw<"CALL"> + assign<&ast::CallStmt::name>(FuncName) +
                 lit<"("> +
                 option(some(append<&ast::CallStmt::args>(ActualParam),
                             lit<",">)) +
                 lit<")"> +
                 option(ikw<"RETURNING"> +
                        append<&ast::CallStmt::returning>(
                            ref<ComponentRef>) +
                        many(lit<","> + append<&ast::CallStmt::returning>(
                                            ref<ComponentRef>))))> {};

  struct RunStmtR
      : rule<create<ast::RunStmt>(
                 ikw<"RUN"> +
                 assign<&ast::RunStmt::command>(ref<ComponentRef> |
                                                ref<StringLitR>) +
                 option(flag<&ast::RunStmt::formMode>(
                            ikw<"IN"> + ikw<"FORM"> + ikw<"MODE">) |
                        flag<&ast::RunStmt::lineMode>(
                            ikw<"IN"> + ikw<"LINE"> + ikw<"MODE">)) +
                 option(flag<&ast::RunStmt::withoutWaiting>(
                            ikw<"WITHOUT"> + ikw<"WAITING">) |
                        (ikw<"RETURNING"> +
                         assign<&ast::RunStmt::returningVar>(
                             ref<ComponentRef>))))> {};

  struct GotoStmtR
      : rule<create<ast::GotoStmt>(
                 ikw<"GOTO"> + option(lit<":">) +
                 assign<&ast::GotoStmt::label>(Name))> {};

  struct SleepStmtR
      : rule<create<ast::SleepStmt>(
                 ikw<"SLEEP"> +
                 assign<&ast::SleepStmt::seconds>(ref<Expression>))> {};

  struct BkForeach
      : rule<ikw<"FOREACH">, value<ast::BlockKind::foreach_>> {};
  struct BkFor : rule<ikw<"FOR">, value<ast::BlockKind::for_>> {};
  struct BkCase : rule<ikw<"CASE">, value<ast::BlockKind::case_>> {};
  struct BkConstruct
      : rule<ikw<"CONSTRUCT">, value<ast::BlockKind::construct>> {};
  struct BkDisplay : rule<ikw<"DISPLAY">, value<ast::BlockKind::display>> {};
  struct BkInput : rule<ikw<"INPUT">, value<ast::BlockKind::input>> {};
  struct BkMenu : rule<ikw<"MENU">, value<ast::BlockKind::menu>> {};
  struct BkReport : rule<ikw<"REPORT">, value<ast::BlockKind::report>> {};
  struct BkWhile : rule<ikw<"WHILE">, value<ast::BlockKind::while_>> {};
  struct BkProgram : rule<ikw<"PROGRAM">, value<ast::BlockKind::program>> {};
  static constexpr auto BlockKindWord =
      ref<BkForeach> | ref<BkFor> | ref<BkCase> | ref<BkConstruct> |
      ref<BkDisplay> | ref<BkInput> | ref<BkMenu> | ref<BkReport> |
      ref<BkWhile>;

  struct ExitStmtR
      : rule<create<ast::ExitStmt>(
                 ikw<"EXIT"> +
                 assign<&ast::ExitStmt::kind>(BlockKindWord)) |
             create<ast::ExitStmt>(
                 ikw<"EXIT"> +
                 assign<&ast::ExitStmt::kind>(ref<BkProgram>) +
                 option((lit<"("> +
                         assign<&ast::ExitStmt::programCode>(
                             ref<Expression>) +
                         lit<")">) |
                        assign<&ast::ExitStmt::programCode>(
                            ref<Expression>)))> {};

  struct ContinueStmtR
      : rule<create<ast::ContinueStmt>(
                 ikw<"CONTINUE"> +
                 assign<&ast::ContinueStmt::kind>(BlockKindWord))> {};

  struct ReturnStmtR
      : rule<create<ast::ReturnStmt> + ikw<"RETURN"> +
             option(append<&ast::ReturnStmt::values>(ref<Expression>) +
                    many(lit<","> + append<&ast::ReturnStmt::values>(
                                        ref<Expression>)))> {};

  struct DeferStmtR
      : rule<create<ast::DeferStmt>(
                 ikw<"DEFER"> +
                 (flag<&ast::DeferStmt::interrupt>(
                      ikw<"INTERRUPT">) |
                  ikw<"QUIT">))> {};

  struct IfStmtR
      : rule<create<ast::IfStmt>(
                 ikw<"IF"> + assign<&ast::IfStmt::cond>(ref<Expression>) +
                 ikw<"THEN"> +
                 many(append<&ast::IfStmt::thenBody>(ref<Statement>)) +
                 option(ikw<"ELSE"> +
                        many(append<&ast::IfStmt::elseBody>(
                            ref<Statement>))) +
                 ikw<"END"> + ikw<"IF">)> {};

  struct CaseWhenR
      : rule<create<ast::CaseWhen>(
                 ikw<"WHEN"> +
                 assign<&ast::CaseWhen::test>(ref<Expression>) +
                 many(append<&ast::CaseWhen::body>(ref<Statement>)))> {};

  struct CaseStmtR
      : rule<create<ast::CaseStmt>(
                 ikw<"CASE"> +
                 assign<&ast::CaseStmt::subject>(ref<Expression>) +
                 many(append<&ast::CaseStmt::whens>(ref<CaseWhenR>)) +
                 option(ikw<"OTHERWISE"> +
                        many(append<&ast::CaseStmt::otherwise>(
                            ref<Statement>))) +
                 ikw<"END"> + ikw<"CASE">) |
             create<ast::CaseStmt>(
                 ikw<"CASE"> +
                 many(append<&ast::CaseStmt::whens>(ref<CaseWhenR>)) +
                 option(ikw<"OTHERWISE"> +
                        many(append<&ast::CaseStmt::otherwise>(
                            ref<Statement>))) +
                 ikw<"END"> + ikw<"CASE">)> {};

  struct WhileStmtR
      : rule<create<ast::WhileStmt>(
                 ikw<"WHILE"> +
                 assign<&ast::WhileStmt::cond>(ref<Expression>) +
                 many(append<&ast::WhileStmt::body>(ref<Statement>)) +
                 ikw<"END"> + ikw<"WHILE">)> {};

  struct ForStmtR
      : rule<create<ast::ForStmt>(
                 ikw<"FOR"> + assign<&ast::ForStmt::var>(Name) + lit<"="> +
                 assign<&ast::ForStmt::from>(ref<Expression>) +
                 ikw<"TO"> + assign<&ast::ForStmt::to>(ref<Expression>) +
                 option(ikw<"STEP"> +
                        assign<&ast::ForStmt::step>(ref<Expression>)) +
                 many(append<&ast::ForStmt::body>(ref<Statement>)) +
                 ikw<"END"> + ikw<"FOR">)> {};

  struct ForEachStmtR
      : rule<create<ast::ForEachStmt>(
                 ikw<"FOREACH"> +
                 assign<&ast::ForEachStmt::cursorName>(Name) +
                 option(ikw<"USING"> +
                        append<&ast::ForEachStmt::usingVars>(
                            ref<ComponentRef>) +
                        many(lit<","> +
                             append<&ast::ForEachStmt::usingVars>(
                                 ref<ComponentRef>))) +
                 option(ikw<"INTO"> +
                        append<&ast::ForEachStmt::intoVars>(
                            ref<ComponentRef>) +
                        many(lit<","> +
                             append<&ast::ForEachStmt::intoVars>(
                                 ref<ComponentRef>))) +
                 option(flag<&ast::ForEachStmt::reoptimization>(
                     ikw<"WITH"> + ikw<"REOPTIMIZATION">)) +
                 many(append<&ast::ForEachStmt::body>(ref<Statement>)) +
                 ikw<"END"> + ikw<"FOREACH">)> {};

  // ------------------------------------------------------------ Storage

  struct AllocateArrayStmtR
      : rule<create<ast::AllocateArrayStmt>(
                 ikw<"ALLOCATE"> + ikw<"ARRAY"> +
                 assign<&ast::AllocateArrayStmt::name>(Name) +
                 assign<&ast::AllocateArrayStmt::dims>(ref<ArrayDimsR>))> {
  };

  struct DeallocateArrayStmtR
      : rule<create<ast::DeallocateArrayStmt>(
                 ikw<"DEALLOCATE"> + ikw<"ARRAY"> +
                 assign<&ast::DeallocateArrayStmt::name>(Name))> {};

  struct ResizeArrayStmtR
      : rule<create<ast::ResizeArrayStmt>(
                 ikw<"RESIZE"> + ikw<"ARRAY"> +
                 assign<&ast::ResizeArrayStmt::name>(Name) +
                 assign<&ast::ResizeArrayStmt::dims>(ref<ArrayDimsR>))> {};

  struct LocateStmtR
      : rule<create<ast::LocateStmt>(
                 ikw<"LOCATE"> +
                 append<&ast::LocateStmt::vars>(ref<ComponentRef>) +
                 many(lit<","> +
                      append<&ast::LocateStmt::vars>(ref<ComponentRef>)) +
                 ikw<"IN"> +
                 (flag<&ast::LocateStmt::inMemory>(ikw<"MEMORY">) |
                  (ikw<"FILE"> +
                   option(assign<&ast::LocateStmt::file>(
                       ref<ComponentRef> | ref<StringLitR>)))))> {};

  struct FreeVariablesStmtR
      : rule<create<ast::FreeVariablesStmt>(
                 ikw<"FREE"> +
                 append<&ast::FreeVariablesStmt::vars>(ref<ComponentRef>) +
                 many(lit<","> + append<&ast::FreeVariablesStmt::vars>(
                                      ref<ComponentRef>)))> {};

  struct InitializeStmtR
      : rule<create<ast::InitializeStmt>(
                 ikw<"INITIALIZE"> +
                 append<&ast::InitializeStmt::vars>(ref<ComponentRef>) +
                 many(lit<","> +
                      append<&ast::InitializeStmt::vars>(
                          ref<ComponentRef>)) +
                 (flag<&ast::InitializeStmt::toNull>(
                      ikw<"TO"> + ikw<"NULL">) |
                  (ikw<"LIKE"> +
                   append<&ast::InitializeStmt::likeExprs>(
                       ref<Expression>) +
                   many(lit<","> +
                        append<&ast::InitializeStmt::likeExprs>(
                            ref<Expression>)))))> {};

  struct ValidateStmtR
      : rule<create<ast::ValidateStmt>(
                 ikw<"VALIDATE"> +
                 append<&ast::ValidateStmt::vars>(ref<ComponentRef>) +
                 many(lit<","> +
                      append<&ast::ValidateStmt::vars>(ref<ComponentRef>)) +
                 ikw<"LIKE"> +
                 append<&ast::ValidateStmt::likeExprs>(ref<Expression>) +
                 many(lit<","> + append<&ast::ValidateStmt::likeExprs>(
                                      ref<Expression>)))> {};

  // ------------------------------------------------------ Attributes/UI

  static constexpr auto AttributeName =
      ikw<"BLACK"> | ikw<"BLUE"> | ikw<"CYAN"> | ikw<"GREEN"> |
      ikw<"MAGENTA"> | ikw<"RED"> | ikw<"WHITE"> | ikw<"YELLOW"> |
      ikw<"BOLD"> | ikw<"DIM"> | ikw<"NORMAL"> | ikw<"INVISIBLE"> |
      ikw<"REVERSE"> | ikw<"BLINK"> | ikw<"UNDERLINE">;

  struct AttributeListR
      : rule<create<ast::AttributeList>(
                 (ikw<"ATTRIBUTE"> | ikw<"ATTRIBUTES">) + lit<"("> +
                 append<&ast::AttributeList::names>(AttributeName) +
                 many(lit<","> + append<&ast::AttributeList::names>(
                                      AttributeName)) +
                 lit<")">)> {};

  struct KeyEventR
      : rule<create<ast::KeyEvent>(
                 ikw<"ON"> + ikw<"KEY"> + lit<"("> +
                 append<&ast::KeyEvent::keys>(ref<Expression>) +
                 many(lit<","> +
                      append<&ast::KeyEvent::keys>(ref<Expression>)) +
                 lit<")"> +
                 many(append<&ast::KeyEvent::body>(ref<Statement>)))> {};

  struct DisplayStmtR
      : rule<create<ast::DisplayStmt>(
                 ikw<"DISPLAY"> +
                 ((flag<&ast::DisplayStmt::byName>(ikw<"BY"> +
                                                        ikw<"NAME">) +
                   append<&ast::DisplayStmt::items>(ref<Expression>) +
                   many(lit<","> + append<&ast::DisplayStmt::items>(
                                       ref<Expression>))) |
                  (append<&ast::DisplayStmt::items>(ref<Expression>) +
                   many(lit<","> + append<&ast::DisplayStmt::items>(
                                       ref<Expression>)) +
                   option((ikw<"TO"> +
                           append<&ast::DisplayStmt::toFields>(
                               ref<ComponentRef>) +
                           many(lit<","> +
                                append<&ast::DisplayStmt::toFields>(
                                    ref<ComponentRef>))) |
                          (ikw<"AT"> +
                           assign<&ast::DisplayStmt::atRow>(
                               ref<Expression>) +
                           lit<","> +
                           assign<&ast::DisplayStmt::atCol>(
                               ref<Expression>))))) +
                 option(assign<&ast::DisplayStmt::attributes>(
                     ref<AttributeListR>)))> {};

  struct DisplayArrayStmtR
      : rule<create<ast::DisplayArrayStmt>(
                 ikw<"DISPLAY"> + ikw<"ARRAY"> +
                 assign<&ast::DisplayArrayStmt::array>(ref<Expression>) +
                 ikw<"TO"> +
                 assign<&ast::DisplayArrayStmt::to>(ref<Expression>) +
                 option(assign<&ast::DisplayArrayStmt::attributes>(
                     ref<AttributeListR>)) +
                 many(append<&ast::DisplayArrayStmt::events>(
                     ref<KeyEventR>)) +
                 option(ikw<"END"> + ikw<"DISPLAY">))> {};

  struct EvBeforeInput
      : rule<ikw<"BEFORE"> + ikw<"INPUT">,
             value<ast::FieldEventKind::beforeInput>> {};
  struct EvAfterInput
      : rule<ikw<"AFTER"> + ikw<"INPUT">,
             value<ast::FieldEventKind::afterInput>> {};
  struct EvBeforeRow
      : rule<ikw<"BEFORE"> + ikw<"ROW">,
             value<ast::FieldEventKind::beforeRow>> {};
  struct EvAfterRow
      : rule<ikw<"AFTER"> + ikw<"ROW">,
             value<ast::FieldEventKind::afterRow>> {};
  struct EvBeforeInsert
      : rule<ikw<"BEFORE"> + ikw<"INSERT">,
             value<ast::FieldEventKind::beforeInsert>> {};
  struct EvAfterInsert
      : rule<ikw<"AFTER"> + ikw<"INSERT">,
             value<ast::FieldEventKind::afterInsert>> {};
  struct EvBeforeDelete
      : rule<ikw<"BEFORE"> + ikw<"DELETE">,
             value<ast::FieldEventKind::beforeDelete>> {};
  struct EvAfterDelete
      : rule<ikw<"AFTER"> + ikw<"DELETE">,
             value<ast::FieldEventKind::afterDelete>> {};
  struct EvBeforeConstruct
      : rule<ikw<"BEFORE"> + ikw<"CONSTRUCT">,
             value<ast::FieldEventKind::beforeConstruct>> {};
  struct EvAfterConstruct
      : rule<ikw<"AFTER"> + ikw<"CONSTRUCT">,
             value<ast::FieldEventKind::afterConstruct>> {};
  struct EvBeforeField
      : rule<ikw<"BEFORE"> + ikw<"FIELD">,
             value<ast::FieldEventKind::beforeField>> {};
  struct EvAfterField
      : rule<ikw<"AFTER"> + ikw<"FIELD">,
             value<ast::FieldEventKind::afterField>> {};
  struct EvOnKey
      : rule<ikw<"ON"> + ikw<"KEY">, value<ast::FieldEventKind::onKey>> {};

  struct FieldEventMoment
      : rule<create<ast::FieldEvent>(
                 assign<&ast::FieldEvent::kind>(
                     ref<EvBeforeInput> | ref<EvAfterInput> |
                     ref<EvBeforeRow> | ref<EvAfterRow> |
                     ref<EvBeforeInsert> | ref<EvAfterInsert> |
                     ref<EvBeforeDelete> | ref<EvAfterDelete>) +
                 many(append<&ast::FieldEvent::body>(ref<Statement>)))> {};

  struct FieldEventConstructMoment
      : rule<create<ast::FieldEvent>(
                 assign<&ast::FieldEvent::kind>(ref<EvBeforeConstruct> |
                                                ref<EvAfterConstruct>) +
                 many(append<&ast::FieldEvent::body>(ref<Statement>)))> {};

  struct FieldEventField
      : rule<create<ast::FieldEvent>(
                 assign<&ast::FieldEvent::kind>(ref<EvBeforeField> |
                                                ref<EvAfterField>) +
                 append<&ast::FieldEvent::fields>(ref<Expression>) +
                 many(lit<","> +
                      append<&ast::FieldEvent::fields>(ref<Expression>)) +
                 many(append<&ast::FieldEvent::body>(ref<Statement>)))> {};

  struct FieldEventKey
      : rule<create<ast::FieldEvent>(
                 assign<&ast::FieldEvent::kind>(ref<EvOnKey>) + lit<"("> +
                 append<&ast::FieldEvent::keys>(ref<Expression>) +
                 many(lit<","> +
                      append<&ast::FieldEvent::keys>(ref<Expression>)) +
                 lit<")"> +
                 many(append<&ast::FieldEvent::body>(ref<Statement>)))> {};

  static constexpr auto InputEventR = ref<FieldEventField> |
                                      ref<FieldEventMoment> |
                                      ref<FieldEventKey>;
  static constexpr auto ConstructEventR = ref<FieldEventField> |
                                          ref<FieldEventConstructMoment> |
                                          ref<FieldEventKey>;

  struct InputStmtR
      : rule<create<ast::InputStmt>(
                 ikw<"INPUT"> +
                 ((flag<&ast::InputStmt::byName>(ikw<"BY"> +
                                                      ikw<"NAME">) +
                   append<&ast::InputStmt::items>(ref<Expression>) +
                   many(lit<","> + append<&ast::InputStmt::items>(
                                       ref<Expression>)) +
                   option(flag<&ast::InputStmt::withoutDefaults>(
                       ikw<"WITHOUT"> + ikw<"DEFAULTS">))) |
                  (append<&ast::InputStmt::items>(ref<Expression>) +
                   many(lit<","> + append<&ast::InputStmt::items>(
                                       ref<Expression>)) +
                   option(flag<&ast::InputStmt::withoutDefaults>(
                       ikw<"WITHOUT"> + ikw<"DEFAULTS">)) +
                   ikw<"FROM"> +
                   append<&ast::InputStmt::fromFields>(ref<ComponentRef>) +
                   many(lit<","> + append<&ast::InputStmt::fromFields>(
                                       ref<ComponentRef>)))) +
                 option(assign<&ast::InputStmt::attributes>(
                     ref<AttributeListR>)) +
                 option(ikw<"HELP"> +
                        assign<&ast::InputStmt::help>(ref<SignedNumber>)) +
                 option(some(append<&ast::InputStmt::events>(InputEventR)) +
                        ikw<"END"> + ikw<"INPUT">))> {};

  struct InputArrayStmtR
      : rule<create<ast::InputArrayStmt>(
                 ikw<"INPUT"> + ikw<"ARRAY"> +
                 assign<&ast::InputArrayStmt::array>(ref<Expression>) +
                 option(flag<&ast::InputArrayStmt::withoutDefaults>(
                     ikw<"WITHOUT"> + ikw<"DEFAULTS">)) +
                 ikw<"FROM"> +
                 append<&ast::InputArrayStmt::from>(ref<Expression>) +
                 many(lit<","> + append<&ast::InputArrayStmt::from>(
                                      ref<Expression>)) +
                 option(ikw<"HELP"> +
                        assign<&ast::InputArrayStmt::help>(
                            ref<SignedNumber>)) +
                 option(assign<&ast::InputArrayStmt::attributes>(
                     ref<AttributeListR>)) +
                 option(some(append<&ast::InputArrayStmt::events>(
                            InputEventR)) +
                        ikw<"END"> + ikw<"INPUT">))> {};

  struct NextFieldStmtR
      : rule<create<ast::NextFieldStmt>(
                 ikw<"NEXT"> + ikw<"FIELD"> +
                 (flag<&ast::NextFieldStmt::next>(ikw<"NEXT">) |
                  flag<&ast::NextFieldStmt::previous>(
                      ikw<"PREVIOUS">) |
                  assign<&ast::NextFieldStmt::field>(
                      ref<ComponentRef>)))> {};

  struct ConstructStmtR
      : rule<create<ast::ConstructStmt>(
                 ikw<"CONSTRUCT"> +
                 ((flag<&ast::ConstructStmt::byName>(ikw<"BY"> +
                                                          ikw<"NAME">) +
                   assign<&ast::ConstructStmt::target>(
                       ref<ComponentRef>) +
                   ikw<"ON"> +
                   append<&ast::ConstructStmt::columns>(
                       ref<ComponentRef>) +
                   many(lit<","> + append<&ast::ConstructStmt::columns>(
                                       ref<ComponentRef>))) |
                  (assign<&ast::ConstructStmt::target>(
                       ref<ComponentRef>) +
                   ikw<"ON"> +
                   append<&ast::ConstructStmt::columns>(
                       ref<ComponentRef>) +
                   many(lit<","> + append<&ast::ConstructStmt::columns>(
                                       ref<ComponentRef>)) +
                   ikw<"FROM"> +
                   append<&ast::ConstructStmt::fromFields>(
                       ref<Expression>) +
                   many(lit<","> + append<&ast::ConstructStmt::fromFields>(
                                       ref<Expression>)))) +
                 option(assign<&ast::ConstructStmt::attributes>(
                     ref<AttributeListR>)) +
                 option(ikw<"HELP"> +
                        assign<&ast::ConstructStmt::help>(
                            ref<SignedNumber>)) +
                 option(some(append<&ast::ConstructStmt::events>(
                            ConstructEventR)) +
                        ikw<"END"> + ikw<"CONSTRUCT">))> {};

  struct MenuEventBefore
      : rule<create<ast::MenuEvent>(
                 flag<&ast::MenuEvent::beforeMenu>(ikw<"BEFORE"> +
                                                        ikw<"MENU">))> {};

  struct MenuEventCommand
      : rule<create<ast::MenuEvent>(
                 ikw<"COMMAND"> +
                 option(ikw<"KEY"> + lit<"("> +
                        append<&ast::MenuEvent::keys>(ref<Expression>) +
                        many(lit<","> + append<&ast::MenuEvent::keys>(
                                            ref<Expression>)) +
                        lit<")">) +
                 assign<&ast::MenuEvent::description>(ref<Expression>) +
                 option(assign<&ast::MenuEvent::command>(ref<Expression>)) +
                 option(ikw<"HELP"> +
                        assign<&ast::MenuEvent::help>(ref<SignedNumber>)) +
                 many(append<&ast::MenuEvent::body>(ref<Statement>)))> {};

  struct MenuStmtR
      : rule<create<ast::MenuStmt>(
                 ikw<"MENU"> +
                 assign<&ast::MenuStmt::title>(ref<Expression>) +
                 many(append<&ast::MenuStmt::events>(
                     ref<MenuEventBefore> | ref<MenuEventCommand>)) +
                 ikw<"END"> + ikw<"MENU">)> {};

  struct MoNext : rule<ikw<"NEXT">, value<ast::MenuOptionAction::next>> {};
  struct MoShow : rule<ikw<"SHOW">, value<ast::MenuOptionAction::show>> {};
  struct MoHide : rule<ikw<"HIDE">, value<ast::MenuOptionAction::hide>> {};

  struct MenuOptionStmtR
      : rule<create<ast::MenuOptionStmt>(
                 assign<&ast::MenuOptionStmt::action>(
                     ref<MoNext> | ref<MoShow> | ref<MoHide>) +
                 ikw<"OPTION"> +
                 (flag<&ast::MenuOptionStmt::all>(ikw<"ALL">) |
                  append<&ast::MenuOptionStmt::options>(ref<Expression>)) +
                 many(lit<","> + append<&ast::MenuOptionStmt::options>(
                                      ref<Expression>)))> {};

  struct ErrorStmtR
      : rule<create<ast::ErrorStmt>(
                 ikw<"ERROR"> +
                 append<&ast::ErrorStmt::items>(ref<Expression>) +
                 many(lit<","> +
                      append<&ast::ErrorStmt::items>(ref<Expression>)) +
                 option(assign<&ast::ErrorStmt::attributes>(
                     ref<AttributeListR>)))> {};

  struct MessageStmtR
      : rule<create<ast::MessageStmt>(
                 ikw<"MESSAGE"> +
                 append<&ast::MessageStmt::items>(ref<Expression>) +
                 many(lit<","> +
                      append<&ast::MessageStmt::items>(ref<Expression>)) +
                 option(assign<&ast::MessageStmt::attributes>(
                     ref<AttributeListR>)))> {};

  struct PromptStmtR
      : rule<create<ast::PromptStmt>(
                 ikw<"PROMPT"> +
                 append<&ast::PromptStmt::items>(ref<Expression>) +
                 many(lit<","> +
                      append<&ast::PromptStmt::items>(ref<Expression>)) +
                 option(assign<&ast::PromptStmt::attributes>(
                     ref<AttributeListR>)) +
                 ikw<"FOR"> +
                 option(flag<&ast::PromptStmt::isChar>(ikw<"CHAR">)) +
                 assign<&ast::PromptStmt::target>(ref<ComponentRef>) +
                 option(ikw<"HELP"> +
                        assign<&ast::PromptStmt::help>(ref<SignedNumber>)) +
                 option(assign<&ast::PromptStmt::attributes2>(
                     ref<AttributeListR>)) +
                 option(many(append<&ast::PromptStmt::events>(
                            ref<KeyEventR>)) +
                        ikw<"END"> + ikw<"PROMPT">))> {};

  struct OptKindMessage
      : rule<ikw<"MESSAGE">, value<ast::OptionKind::messageLine>> {};
  struct OptKindPrompt
      : rule<ikw<"PROMPT">, value<ast::OptionKind::promptLine>> {};
  struct OptKindMenu
      : rule<ikw<"MENU">, value<ast::OptionKind::menuLine>> {};
  struct OptKindComment
      : rule<ikw<"COMMENT">, value<ast::OptionKind::commentLine>> {};
  struct OptKindError
      : rule<ikw<"ERROR">, value<ast::OptionKind::errorLine>> {};
  struct OptKindForm
      : rule<ikw<"FORM">, value<ast::OptionKind::formLine>> {};

  struct OptionItemLine
      : rule<create<ast::OptionItem>(
                 assign<&ast::OptionItem::kind>(
                     ref<OptKindMessage> | ref<OptKindPrompt> |
                     ref<OptKindMenu> | ref<OptKindComment> |
                     ref<OptKindError> | ref<OptKindForm>) +
                 ikw<"LINE"> +
                 assign<&ast::OptionItem::value>(ref<Expression>))> {};

  struct OptKindInputAttr
      : rule<ikw<"INPUT">, value<ast::OptionKind::inputAttr>> {};
  struct OptKindDisplayAttr
      : rule<ikw<"DISPLAY">, value<ast::OptionKind::displayAttr>> {};

  struct OptionItemAttrs
      : rule<create<ast::OptionItem>(
                 assign<&ast::OptionItem::kind>(ref<OptKindInputAttr> |
                                                ref<OptKindDisplayAttr>) +
                 assign<&ast::OptionItem::attributes>(
                     ref<AttributeListR>))> {};

  struct OptKindInputWrap
      : rule<ikw<"INPUT"> + ikw<"WRAP">,
             value<ast::OptionKind::inputWrap>> {};
  struct OptKindInputNoWrap
      : rule<ikw<"INPUT"> + ikw<"NO"> + ikw<"WRAP">,
             value<ast::OptionKind::inputNoWrap>> {};
  struct OptKindInsertKey
      : rule<ikw<"INSERT"> + ikw<"KEY">,
             value<ast::OptionKind::insertKey>> {};
  struct OptKindDeleteKey
      : rule<ikw<"DELETE"> + ikw<"KEY">,
             value<ast::OptionKind::deleteKey>> {};
  struct OptKindNextKey
      : rule<ikw<"NEXT"> + ikw<"KEY">, value<ast::OptionKind::nextKey>> {};
  struct OptKindPreviousKey
      : rule<ikw<"PREVIOUS"> + ikw<"KEY">,
             value<ast::OptionKind::previousKey>> {};
  struct OptKindAcceptKey
      : rule<ikw<"ACCEPT"> + ikw<"KEY">,
             value<ast::OptionKind::acceptKey>> {};
  struct OptKindHelpFile
      : rule<ikw<"HELP"> + ikw<"FILE">, value<ast::OptionKind::helpFile>> {
  };
  struct OptKindHelpKey
      : rule<ikw<"HELP"> + ikw<"KEY">, value<ast::OptionKind::helpKey>> {};
  struct OptKindSqlIntOn
      : rule<ikw<"SQL"> + ikw<"INTERRUPT"> + ikw<"ON">,
             value<ast::OptionKind::sqlInterruptOn>> {};
  struct OptKindSqlIntOff
      : rule<ikw<"SQL"> + ikw<"INTERRUPT"> + ikw<"OFF">,
             value<ast::OptionKind::sqlInterruptOff>> {};
  struct OptKindFieldOrderC
      : rule<ikw<"FIELD"> + ikw<"ORDER"> + ikw<"CONSTRAINED">,
             value<ast::OptionKind::fieldOrderConstrained>> {};
  struct OptKindFieldOrderU
      : rule<ikw<"FIELD"> + ikw<"ORDER"> + ikw<"UNCONSTRAINED">,
             value<ast::OptionKind::fieldOrderUnconstrained>> {};

  struct OptionItemKeyword
      : rule<create<ast::OptionItem>(
                 assign<&ast::OptionItem::kind>(
                     ref<OptKindInputWrap> | ref<OptKindInputNoWrap> |
                     ref<OptKindInsertKey> | ref<OptKindDeleteKey> |
                     ref<OptKindNextKey> | ref<OptKindPreviousKey> |
                     ref<OptKindAcceptKey> | ref<OptKindHelpFile> |
                     ref<OptKindHelpKey> | ref<OptKindSqlIntOn> |
                     ref<OptKindSqlIntOff> | ref<OptKindFieldOrderC> |
                     ref<OptKindFieldOrderU>) +
                 option(assign<&ast::OptionItem::value>(ref<Expression>)))> {
  };

  static constexpr auto OptionItemR = ref<OptionItemLine> |
                                      ref<OptionItemAttrs> |
                                      ref<OptionItemKeyword>;

  struct OptionsStmtR
      : rule<create<ast::OptionsStmt>(
                 ikw<"OPTIONS"> +
                 append<&ast::OptionsStmt::items>(OptionItemR) +
                 many(lit<","> +
                      append<&ast::OptionsStmt::items>(OptionItemR)))> {};

  struct ClearKindForm
      : rule<ikw<"FORM">, value<ast::ClearTarget::form>> {};
  struct ClearKindWindow
      : rule<ikw<"WINDOW">, value<ast::ClearTarget::window>> {};
  struct ClearKindScreen
      : rule<option(ikw<"WINDOW">) + ikw<"SCREEN">,
             value<ast::ClearTarget::screen>> {};

  struct ClearStmtR
      : rule<create<ast::ClearStmt>(
                 ikw<"CLEAR"> +
                 (assign<&ast::ClearStmt::target>(ref<ClearKindForm>) |
                  (assign<&ast::ClearStmt::target>(ref<ClearKindWindow>) +
                   assign<&ast::ClearStmt::windowName>(Name)) |
                  assign<&ast::ClearStmt::target>(ref<ClearKindScreen>) |
                  (append<&ast::ClearStmt::fields>(ref<Expression>) +
                   many(lit<","> + append<&ast::ClearStmt::fields>(
                                        ref<Expression>)))))> {};

  struct CloseWindowStmtR
      : rule<create<ast::CloseWindowStmt>(
                 ikw<"CLOSE"> + ikw<"WINDOW"> +
                 assign<&ast::CloseWindowStmt::name>(Name))> {};
  struct CloseFormStmtR
      : rule<create<ast::CloseFormStmt>(
                 ikw<"CLOSE"> + ikw<"FORM"> +
                 assign<&ast::CloseFormStmt::name>(Name))> {};

  struct CurrentWindowStmtR
      : rule<create<ast::CurrentWindowStmt>(
                 ikw<"CURRENT"> + ikw<"WINDOW"> + ikw<"IS"> +
                 (flag<&ast::CurrentWindowStmt::isScreen>(
                      ikw<"SCREEN">) |
                  assign<&ast::CurrentWindowStmt::name>(Name)))> {};

  struct DisplayFormStmtR
      : rule<create<ast::DisplayFormStmt>(
                 ikw<"DISPLAY"> + ikw<"FORM"> +
                 assign<&ast::DisplayFormStmt::name>(Name) +
                 option(assign<&ast::DisplayFormStmt::attributes>(
                     ref<AttributeListR>)))> {};

  struct OpenFormStmtR
      : rule<create<ast::OpenFormStmt>(
                 ikw<"OPEN"> + ikw<"FORM"> +
                 assign<&ast::OpenFormStmt::name>(ref<Expression>) +
                 ikw<"FROM"> +
                 assign<&ast::OpenFormStmt::from>(ref<Expression>))> {};

  struct WinKindPrompt
      : rule<ikw<"PROMPT">, value<ast::WindowClauseKind::promptLine>> {};
  struct WinKindForm
      : rule<ikw<"FORM">, value<ast::WindowClauseKind::formLine>> {};
  struct WinKindMenu
      : rule<ikw<"MENU">, value<ast::WindowClauseKind::menuLine>> {};
  struct WinKindMessage
      : rule<ikw<"MESSAGE">, value<ast::WindowClauseKind::messageLine>> {};
  struct WinKindComment
      : rule<ikw<"COMMENT">, value<ast::WindowClauseKind::commentLine>> {};

  static constexpr auto LinePos =
      assign<&ast::WindowAttrClause::line>(ref<SignedNumber>) |
      (ikw<"FIRST"> + option(lit<"+"> + SignedNumberText)) |
      (ikw<"LAST"> + option(lit<"-"> + SignedNumberText));

  struct WindowLineClause
      : rule<create<ast::WindowAttrClause>(
                 assign<&ast::WindowAttrClause::kind>(
                     ref<WinKindPrompt> | ref<WinKindForm> |
                     ref<WinKindMenu> | ref<WinKindMessage>) +
                 ikw<"LINE"> + LinePos)> {};

  struct WindowCommentClause
      : rule<create<ast::WindowAttrClause>(
                 assign<&ast::WindowAttrClause::kind>(ref<WinKindComment>) +
                 ikw<"LINE"> + (LinePos | ikw<"OFF">))> {};

  struct OpenWindowStmtR
      : rule<create<ast::OpenWindowStmt>(
                 ikw<"OPEN"> + ikw<"WINDOW"> +
                 assign<&ast::OpenWindowStmt::name>(ref<Expression>) +
                 ikw<"AT"> +
                 assign<&ast::OpenWindowStmt::atRow>(ref<Expression>) +
                 lit<","> +
                 assign<&ast::OpenWindowStmt::atCol>(ref<Expression>) +
                 ikw<"WITH"> +
                 ((ikw<"FORM"> +
                   assign<&ast::OpenWindowStmt::withForm>(
                       ref<Expression>)) |
                  (assign<&ast::OpenWindowStmt::rows>(ref<Expression>) +
                   ikw<"ROWS"> + lit<","> +
                   assign<&ast::OpenWindowStmt::cols>(ref<Expression>) +
                   ikw<"COLUMNS">)) +
                 option((ikw<"ATTRIBUTE"> | ikw<"ATTRIBUTES">) + lit<"("> +
                        (append<&ast::OpenWindowStmt::attributes>(
                             AttributeName | ikw<"BORDER">) |
                         append<&ast::OpenWindowStmt::clauses>(
                             ref<WindowLineClause> |
                             ref<WindowCommentClause>)) +
                        many(lit<","> +
                             (append<&ast::OpenWindowStmt::attributes>(
                                  AttributeName | ikw<"BORDER">) |
                              append<&ast::OpenWindowStmt::clauses>(
                                  ref<WindowLineClause> |
                                  ref<WindowCommentClause>))) +
                        lit<")">))> {};

  struct ScrollStmtR
      : rule<create<ast::ScrollStmt>(
                 ikw<"SCROLL"> +
                 append<&ast::ScrollStmt::fields>(ref<Expression>) +
                 many(lit<","> +
                      append<&ast::ScrollStmt::fields>(ref<Expression>)) +
                 (flag<&ast::ScrollStmt::up>(ikw<"UP">) |
                  ikw<"DOWN">) +
                 option(ikw<"BY"> +
                        assign<&ast::ScrollStmt::byCount>(
                            ref<SignedNumber>)))> {};

  // -------------------------------------------------------- Embedded SQL

  struct TableRefR
      : rule<create<ast::TableRef>(
                 option((assign<&ast::TableRef::qualifier>(Name) +
                         option(lit<"@"> + Name) + lit<":">) |
                        assign<&ast::TableRef::qualifier>(
                            ref<STRING_DQ> | ref<STRING_SQ>)) +
                 assign<&ast::TableRef::name>(Name))> {};

  struct ColumnDefR
      : rule<create<ast::ColumnDef>(
                 assign<&ast::ColumnDef::name>(Name) +
                 (assign<&ast::ColumnDef::type>(ref<TypeSpec>) |
                  ((flag<&ast::ColumnDef::isByte>(ikw<"BYTE">) |
                    flag<&ast::ColumnDef::isText>(ikw<"TEXT">)) +
                   option(ikw<"IN"> +
                          (ikw<"TABLE"> |
                           assign<&ast::ColumnDef::storeIn>(Name))))) +
                 option(flag<&ast::ColumnDef::notNull>(
                     ikw<"NOT"> + ikw<"NULL">)))> {};

  struct UniqueConstraintR
      : rule<create<ast::UniqueConstraint>(
                 ikw<"UNIQUE"> + lit<"("> +
                 option(append<&ast::UniqueConstraint::columns>(Name) +
                        many(lit<","> +
                             append<&ast::UniqueConstraint::columns>(
                                 Name))) +
                 lit<")"> +
                 option(ikw<"CONSTRAINT"> +
                        assign<&ast::UniqueConstraint::constraintName>(
                            Name)))> {};

  static constexpr auto ColumnItemR = ref<ColumnDefR> | ref<UniqueConstraintR>;

  struct CreateTableStmtR
      : rule<create<ast::CreateTableStmt>(
                 ikw<"CREATE"> +
                 option(flag<&ast::CreateTableStmt::temp>(
                     ikw<"TEMP">)) +
                 ikw<"TABLE"> +
                 assign<&ast::CreateTableStmt::name>(Name) + lit<"("> +
                 append<&ast::CreateTableStmt::columns>(ColumnItemR) +
                 many(lit<","> + append<&ast::CreateTableStmt::columns>(
                                      ColumnItemR)) +
                 lit<")"> +
                 option(flag<&ast::CreateTableStmt::withNoLog>(
                     ikw<"WITH"> + ikw<"NO"> + ikw<"LOG">)) +
                 option(ikw<"IN"> +
                        assign<&ast::CreateTableStmt::inDbSpace>(Name)) +
                 option(ikw<"EXTENT"> + ikw<"SIZE"> +
                        assign<&ast::CreateTableStmt::extentSize>(
                            ref<SignedNumber>)) +
                 option(ikw<"NEXT"> + ikw<"SIZE"> +
                        assign<&ast::CreateTableStmt::nextSize>(
                            ref<SignedNumber>)) +
                 option(ikw<"LOCK"> + ikw<"MODE"> + lit<"("> +
                        (flag<&ast::CreateTableStmt::lockPage>(
                             ikw<"PAGE">) |
                         flag<&ast::CreateTableStmt::lockRow>(
                             ikw<"ROW">)) +
                        lit<")">))> {};

  struct DropTableStmtR
      : rule<create<ast::DropTableStmt>(
                 ikw<"DROP"> + ikw<"TABLE"> +
                 assign<&ast::DropTableStmt::name>(Name))> {};

  struct IndexColumnR
      : rule<create<ast::IndexColumn>(
                 assign<&ast::IndexColumn::name>(Name) +
                 option(flag<&ast::IndexColumn::desc>(ikw<"DESC">) |
                        ikw<"ASC">))> {};

  struct CreateIndexStmtR
      : rule<create<ast::CreateIndexStmt>(
                 ikw<"CREATE"> +
                 option(flag<&ast::CreateIndexStmt::unique>(
                     ikw<"UNIQUE">)) +
                 option(flag<&ast::CreateIndexStmt::cluster>(
                     ikw<"CLUSTER">)) +
                 ikw<"INDEX"> +
                 assign<&ast::CreateIndexStmt::indexName>(Name) +
                 ikw<"ON"> +
                 assign<&ast::CreateIndexStmt::tableName>(Name) +
                 lit<"("> +
                 append<&ast::CreateIndexStmt::columns>(
                     ref<IndexColumnR>) +
                 many(lit<","> + append<&ast::CreateIndexStmt::columns>(
                                      ref<IndexColumnR>)) +
                 lit<")">)> {};

  struct DropIndexStmtR
      : rule<create<ast::DropIndexStmt>(
                 ikw<"DROP"> + ikw<"INDEX"> +
                 assign<&ast::DropIndexStmt::name>(Name))> {};

  struct SelectItemR
      : rule<create<ast::SelectItem>(
                 assign<&ast::SelectItem::value>(ref<Expression>) +
                 option(option(ikw<"AS">) +
                        assign<&ast::SelectItem::alias>(Name)))> {};

  struct FromItemR
      : rule<create<ast::FromItem>(
                 option(flag<&ast::FromItem::outer>(ikw<"OUTER">)) +
                 (assign<&ast::FromItem::table>(ref<TableRefR>) |
                  (lit<"("> +
                   assign<&ast::FromItem::select>(ref<SelectStmt>) +
                   lit<")">)) +
                 option(option(ikw<"AS">) +
                        assign<&ast::FromItem::alias>(Name)))> {};

  struct OrderItemR
      : rule<create<ast::OrderItem>(
                 assign<&ast::OrderItem::key>(ref<Expression>) +
                 option(flag<&ast::OrderItem::descending>(
                            ikw<"DESC">) |
                        ikw<"ASC">))> {};

  struct UnionArmR
      : rule<create<ast::UnionArm>(
                 ikw<"UNION"> +
                 option(flag<&ast::UnionArm::all>(ikw<"ALL">)) +
                 assign<&ast::UnionArm::select>(ref<SelectStmt>))> {};

  struct SelectStmt
      : rule<create<ast::SelectStmt>(
                 ikw<"SELECT"> +
                 option(flag<&ast::SelectStmt::all>(ikw<"ALL">) |
                        flag<&ast::SelectStmt::distinct>(
                            ikw<"DISTINCT"> | ikw<"UNIQUE">)) +
                 append<&ast::SelectStmt::items>(ref<SelectItemR>) +
                 many(lit<","> +
                      append<&ast::SelectStmt::items>(ref<SelectItemR>)) +
                 option(ikw<"INTO"> +
                        append<&ast::SelectStmt::intoVars>(
                            ref<ComponentRef>) +
                        many(lit<","> +
                             append<&ast::SelectStmt::intoVars>(
                                 ref<ComponentRef>))) +
                 ikw<"FROM"> +
                 append<&ast::SelectStmt::from>(ref<FromItemR>) +
                 many(lit<","> +
                      append<&ast::SelectStmt::from>(ref<FromItemR>)) +
                 option(ikw<"WHERE"> +
                        assign<&ast::SelectStmt::where>(ref<Expression>)) +
                 option(ikw<"GROUP"> + ikw<"BY"> +
                        append<&ast::SelectStmt::groupBy>(ref<Expression>) +
                        many(lit<","> + append<&ast::SelectStmt::groupBy>(
                                            ref<Expression>))) +
                 option(ikw<"HAVING"> +
                        assign<&ast::SelectStmt::having>(ref<Expression>)) +
                 many(append<&ast::SelectStmt::unions>(ref<UnionArmR>)) +
                 option(ikw<"ORDER"> + ikw<"BY"> +
                        append<&ast::SelectStmt::orderBy>(
                            ref<OrderItemR>) +
                        many(lit<","> + append<&ast::SelectStmt::orderBy>(
                                            ref<OrderItemR>))) +
                 option(ikw<"INTO"> + ikw<"TEMP"> +
                        assign<&ast::SelectStmt::intoTemp>(Name)) +
                 option(flag<&ast::SelectStmt::withNoLog>(
                     ikw<"WITH"> + ikw<"NO"> + ikw<"LOG">)))> {};

  struct InsertStmtR
      : rule<create<ast::InsertStmt>(
                 ikw<"INSERT"> + ikw<"INTO"> +
                 assign<&ast::InsertStmt::table>(ref<TableRefR>) +
                 option(lit<"("> +
                        append<&ast::InsertStmt::columns>(Name) +
                        many(lit<","> +
                             append<&ast::InsertStmt::columns>(Name)) +
                        lit<")">) +
                 ((ikw<"VALUES"> + lit<"("> +
                   append<&ast::InsertStmt::values>(ref<Expression>) +
                   many(lit<","> + append<&ast::InsertStmt::values>(
                                       ref<Expression>)) +
                   lit<")">) |
                  assign<&ast::InsertStmt::select>(ref<SelectStmt>)))> {};

  struct AssignmentR
      : rule<create<ast::Assignment>(
                 assign<&ast::Assignment::target>(ref<ComponentRef>) +
                 lit<"="> +
                 assign<&ast::Assignment::value>(ref<Expression>))> {};

  struct UpdateStmtR
      : rule<create<ast::UpdateStmt>(
                 ikw<"UPDATE"> +
                 assign<&ast::UpdateStmt::table>(ref<TableRefR>) +
                 ikw<"SET"> +
                 ((append<&ast::UpdateStmt::assignments>(ref<AssignmentR>) +
                   many(lit<","> +
                        append<&ast::UpdateStmt::assignments>(
                            ref<AssignmentR>))) |
                  (((lit<"("> +
                    append<&ast::UpdateStmt::starColumns>(Name) +
                    many(lit<","> +
                         append<&ast::UpdateStmt::starColumns>(Name)) +
                    lit<")">) |
                   flag<&ast::UpdateStmt::starTarget>(lit<"*">)) +
                      lit<"="> +
                      ((lit<"("> +
                        append<&ast::UpdateStmt::starValues>(
                            ref<Expression>) +
                        many(lit<","> +
                             append<&ast::UpdateStmt::starValues>(
                                 ref<Expression>)) +
                        lit<")">) |
                       flag<&ast::UpdateStmt::starValue>(lit<"*">)))) +
                 option(ikw<"WHERE"> +
                        (assign<&ast::UpdateStmt::where>(
                             ref<Expression>) |
                         (ikw<"CURRENT"> + ikw<"OF"> +
                          assign<&ast::UpdateStmt::cursorName>(
                              Name)))))> {};

  struct DeleteStmtR
      : rule<create<ast::DeleteStmt>(
                 ikw<"DELETE"> + ikw<"FROM"> +
                 assign<&ast::DeleteStmt::table>(ref<TableRefR>) +
                 option(ikw<"WHERE"> +
                        (assign<&ast::DeleteStmt::where>(
                             ref<Expression>) |
                         (ikw<"CURRENT"> + ikw<"OF"> +
                          assign<&ast::DeleteStmt::cursorName>(
                              Name)))))> {};

  struct LoadStmtR
      : rule<create<ast::LoadStmt>(
                 ikw<"LOAD"> + ikw<"FROM"> +
                 assign<&ast::LoadStmt::source>(ref<ComponentRef> |
                                                ref<StringLitR>) +
                 option(ikw<"DELIMITER"> +
                        assign<&ast::LoadStmt::delimiter>(
                            ref<ComponentRef> | ref<StringLitR>)) +
                 ((ikw<"INSERT"> + ikw<"INTO"> +
                   assign<&ast::LoadStmt::intoTable>(ref<TableRefR>) +
                   option(lit<"("> +
                          append<&ast::LoadStmt::columns>(Name) +
                          many(lit<","> +
                               append<&ast::LoadStmt::columns>(Name)) +
                          lit<")">)) |
                  assign<&ast::LoadStmt::insert>(ref<InsertStmtR>)))> {};

  struct UnloadStmtR
      : rule<create<ast::UnloadStmt>(
                 ikw<"UNLOAD"> + ikw<"TO"> +
                 assign<&ast::UnloadStmt::target>(ref<ComponentRef> |
                                                  ref<StringLitR>) +
                 option(ikw<"DELIMITER"> +
                        assign<&ast::UnloadStmt::delimiter>(
                            ref<ComponentRef> | ref<StringLitR>)) +
                 assign<&ast::UnloadStmt::select>(ref<SelectStmt>))> {};

  struct FetchNext : rule<ikw<"NEXT">, value<ast::FetchKind::next>> {};
  struct FetchPrevPrior
      : rule<ikw<"PREVIOUS"> | ikw<"PRIOR">,
             value<ast::FetchKind::previous>> {};
  struct FetchFirst : rule<ikw<"FIRST">, value<ast::FetchKind::first>> {};
  struct FetchLast : rule<ikw<"LAST">, value<ast::FetchKind::last>> {};
  struct FetchCurrent
      : rule<ikw<"CURRENT">, value<ast::FetchKind::current>> {};
  struct FetchRelKind
      : rule<ikw<"RELATIVE">, value<ast::FetchKind::relative>> {};
  struct FetchAbsKind
      : rule<ikw<"ABSOLUTE">, value<ast::FetchKind::absolute>> {};

  struct DeclareCursorStmtR
      : rule<create<ast::DeclareCursorStmt>(
                 ikw<"DECLARE"> +
                 assign<&ast::DeclareCursorStmt::name>(Name) +
                 option(flag<&ast::DeclareCursorStmt::scroll>(
                     ikw<"SCROLL">)) +
                 ikw<"CURSOR"> +
                 option(flag<&ast::DeclareCursorStmt::withHold>(
                     ikw<"WITH"> + ikw<"HOLD">)) +
                 ikw<"FOR"> +
                 ((assign<&ast::DeclareCursorStmt::select>(
                       ref<SelectStmt>) +
                   option(ikw<"FOR"> +
                          flag<&ast::DeclareCursorStmt::forUpdate>(
                              ikw<"UPDATE">) +
                          option(ikw<"OF"> +
                                 append<&ast::DeclareCursorStmt::
                                            updateColumns>(Name) +
                                 many(lit<","> +
                                      append<&ast::DeclareCursorStmt::
                                                 updateColumns>(Name))))) |
                  assign<&ast::DeclareCursorStmt::insert>(
                      ref<InsertStmtR>) |
                  assign<&ast::DeclareCursorStmt::statementId>(Name)))> {};

  struct CloseCursorStmtR
      : rule<create<ast::CloseCursorStmt>(
                 ikw<"CLOSE"> +
                 assign<&ast::CloseCursorStmt::name>(Name))> {};

  struct FetchStmtR
      : rule<create<ast::FetchStmt>(
                 ikw<"FETCH"> +
                 option((assign<&ast::FetchStmt::kind>(ref<FetchRelKind> |
                                                       ref<FetchAbsKind>) +
                         assign<&ast::FetchStmt::offset>(
                             ref<Expression>)) |
                        assign<&ast::FetchStmt::kind>(
                            ref<FetchNext> | ref<FetchPrevPrior> |
                            ref<FetchFirst> | ref<FetchLast> |
                            ref<FetchCurrent>)) +
                 assign<&ast::FetchStmt::cursorName>(Name) +
                 option(ikw<"INTO"> +
                        append<&ast::FetchStmt::intoVars>(
                            ref<ComponentRef>) +
                        many(lit<","> + append<&ast::FetchStmt::intoVars>(
                                            ref<ComponentRef>))))> {};

  struct FlushStmtR
      : rule<create<ast::FlushStmt>(
                 ikw<"FLUSH"> + assign<&ast::FlushStmt::name>(Name))> {};

  struct OpenCursorStmtR
      : rule<create<ast::OpenCursorStmt>(
                 ikw<"OPEN"> + assign<&ast::OpenCursorStmt::name>(Name) +
                 option(ikw<"USING"> +
                        append<&ast::OpenCursorStmt::usingVars>(
                            ref<ComponentRef>) +
                        many(lit<","> +
                             append<&ast::OpenCursorStmt::usingVars>(
                                 ref<ComponentRef>))))> {};

  struct PutStmtR
      : rule<create<ast::PutStmt>(
                 ikw<"PUT"> + assign<&ast::PutStmt::name>(Name) +
                 option(ikw<"FROM"> +
                        append<&ast::PutStmt::values>(ref<Expression>) +
                        many(lit<","> +
                             append<&ast::PutStmt::values>(
                                 ref<Expression>))))> {};

  struct PrepareStmtR
      : rule<create<ast::PrepareStmt>(
                 ikw<"PREPARE"> + assign<&ast::PrepareStmt::name>(Name) +
                 ikw<"FROM"> +
                 assign<&ast::PrepareStmt::source>(ref<Expression>))> {};

  struct ExecuteStmtR
      : rule<create<ast::ExecuteStmt>(
                 ikw<"EXECUTE"> + assign<&ast::ExecuteStmt::name>(Name) +
                 option(ikw<"USING"> +
                        append<&ast::ExecuteStmt::usingVars>(
                            ref<ComponentRef>) +
                        many(lit<","> +
                             append<&ast::ExecuteStmt::usingVars>(
                                 ref<ComponentRef>))))> {};

  struct LockTableStmtR
      : rule<create<ast::LockTableStmt>(
                 ikw<"LOCK"> + ikw<"TABLE"> +
                 assign<&ast::LockTableStmt::table>(ref<Expression>) +
                 ikw<"IN"> +
                 (flag<&ast::LockTableStmt::share>(ikw<"SHARE">) |
                  ikw<"EXCLUSIVE">) +
                 ikw<"MODE">)> {};

  struct UpdateStatisticsStmtR
      : rule<create<ast::UpdateStatisticsStmt>(
                 ikw<"UPDATE"> + ikw<"STATISTICS"> +
                 option(ikw<"FOR"> + ikw<"TABLE"> +
                        assign<&ast::UpdateStatisticsStmt::table>(
                            ref<TableRefR>)))> {};

  struct SetLockModeStmtR
      : rule<create<ast::SetLockModeStmt>(
                 ikw<"SET"> + ikw<"LOCK"> + ikw<"MODE"> + ikw<"TO"> +
                 (flag<&ast::SetLockModeStmt::wait>(
                      ikw<"WAIT"> + option(ikw<"SECONDS">)) |
                  (ikw<"NOT"> + ikw<"WAIT">)))> {};

  struct SetExplainStmtR
      : rule<create<ast::SetExplainStmt>(
                 ikw<"SET"> + ikw<"EXPLAIN"> +
                 (flag<&ast::SetExplainStmt::on>(ikw<"ON">) |
                  ikw<"OFF">))> {};

  struct IsoCursorStab
      : rule<ikw<"CURSOR"> + ikw<"STABILITY">,
             value<ast::IsolationLevel::cursorStability>> {};
  struct IsoDirty
      : rule<ikw<"DIRTY"> + ikw<"READ">,
             value<ast::IsolationLevel::dirtyRead>> {};
  struct IsoCommitted
      : rule<ikw<"COMMITTED"> + ikw<"READ">,
             value<ast::IsolationLevel::committedRead>> {};
  struct IsoRepeatable
      : rule<ikw<"REPEATABLE"> + ikw<"READ">,
             value<ast::IsolationLevel::repeatableRead>> {};

  struct SetIsolationStmtR
      : rule<create<ast::SetIsolationStmt>(
                 ikw<"SET"> + ikw<"ISOLATION"> + ikw<"TO"> +
                 assign<&ast::SetIsolationStmt::level>(
                     ref<IsoCursorStab> | ref<IsoDirty> |
                     ref<IsoCommitted> | ref<IsoRepeatable>))> {};

  struct SetLogStmtR
      : rule<create<ast::SetLogStmt>(
                 ikw<"SET"> +
                 option(flag<&ast::SetLogStmt::buffered>(
                     ikw<"BUFFERED">)) +
                 ikw<"LOG">)> {};

  struct DatabaseDeclR
      : rule<create<ast::DatabaseDecl>(
                 ikw<"DATABASE"> +
                 assign<&ast::DatabaseDecl::name>(Name) +
                 option(lit<"@"> +
                        assign<&ast::DatabaseDecl::server>(Name)) +
                 option(flag<&ast::DatabaseDecl::exclusive>(
                     ikw<"EXCLUSIVE">)))> {};

  struct CloseDatabaseStmtR
      : rule<create<ast::CloseDatabaseStmt> + ikw<"CLOSE"> +
             ikw<"DATABASE">> {};

  struct TxnBegin : rule<ikw<"BEGIN"> + ikw<"WORK">, value<ast::TxnKind::begin>> {};
  struct TxnCommit
      : rule<ikw<"COMMIT"> + ikw<"WORK">, value<ast::TxnKind::commit>> {};
  struct TxnRollback
      : rule<ikw<"ROLLBACK"> + ikw<"WORK">,
             value<ast::TxnKind::rollback>> {};

  struct TxnStmtR
      : rule<create<ast::TxnStmt>(
                 assign<&ast::TxnStmt::kind>(
                     ref<TxnBegin> | ref<TxnCommit> | ref<TxnRollback>))> {
  };

  struct WhenNotFound
      : rule<ikw<"NOT"> + ikw<"FOUND">,
             value<ast::WheneverCondition::notFound>> {};
  struct WhenSqlError
      : rule<option(ikw<"ANY">) + (ikw<"SQLERROR"> | ikw<"ERROR">),
             value<ast::WheneverCondition::sqlError>> {};
  struct WhenSqlWarning
      : rule<ikw<"SQLWARNING"> | ikw<"WARNING">,
             value<ast::WheneverCondition::sqlWarning>> {};

  struct FlowContinue
      : rule<ikw<"CONTINUE">, value<ast::WheneverAction::continue_>> {};
  struct FlowStop : rule<ikw<"STOP">, value<ast::WheneverAction::stop>> {};
  struct FlowCall : rule<ikw<"CALL">, value<ast::WheneverAction::call>> {};
  struct FlowGoto
      : rule<(ikw<"GO"> + ikw<"TO">) | ikw<"GOTO">,
             value<ast::WheneverAction::goto_>> {};

  struct WheneverStmtR
      : rule<create<ast::WheneverStmt>(
                 ikw<"WHENEVER"> +
                 assign<&ast::WheneverStmt::condition>(
                     ref<WhenNotFound> | ref<WhenSqlError> |
                     ref<WhenSqlWarning>) +
                 ((assign<&ast::WheneverStmt::action>(ref<FlowContinue> |
                                                      ref<FlowStop>)) |
                  (assign<&ast::WheneverStmt::action>(ref<FlowCall>) +
                   assign<&ast::WheneverStmt::target>(Name)) |
                  (assign<&ast::WheneverStmt::action>(ref<FlowGoto>) +
                   option(lit<":">) +
                   assign<&ast::WheneverStmt::target>(Name))))> {};

  // ---------------------------------------------------------------- Report

  struct RcMarginL
      : rule<ikw<"LEFT"> + ikw<"MARGIN">,
             value<ast::ReportClauseKind::leftMargin>> {};
  struct RcMarginR
      : rule<ikw<"RIGHT"> + ikw<"MARGIN">,
             value<ast::ReportClauseKind::rightMargin>> {};
  struct RcMarginT
      : rule<ikw<"TOP"> + ikw<"MARGIN">,
             value<ast::ReportClauseKind::topMargin>> {};
  struct RcMarginB
      : rule<ikw<"BOTTOM"> + ikw<"MARGIN">,
             value<ast::ReportClauseKind::bottomMargin>> {};
  struct RcPageLen
      : rule<ikw<"PAGE"> + ikw<"LENGTH">,
             value<ast::ReportClauseKind::pageLength>> {};
  struct RcTopOfPage
      : rule<ikw<"TOP"> + ikw<"OF"> + ikw<"PAGE">,
             value<ast::ReportClauseKind::topOfPage>> {};

  struct ReportClauseMargin
      : rule<create<ast::ReportClause>(
                 assign<&ast::ReportClause::kind>(
                     ref<RcMarginL> | ref<RcMarginR> | ref<RcMarginT> |
                     ref<RcMarginB> | ref<RcPageLen>) +
                 assign<&ast::ReportClause::value>(ref<SignedNumber>))> {};

  struct ReportClauseTopOfPage
      : rule<create<ast::ReportClause>(
                 assign<&ast::ReportClause::kind>(ref<RcTopOfPage>) +
                 assign<&ast::ReportClause::value>(ref<StringLitR>))> {};

  static constexpr auto ReportClauseR = ref<ReportClauseMargin> |
                                        ref<ReportClauseTopOfPage>;

  struct StartReportTail
      : rule<create<ast::OutputReport>(
                 option(ikw<"TO"> +
                        (assign<&ast::OutputReport::reportTo>(
                             ref<Expression>) |
                         (flag<&ast::OutputReport::toPipe>(
                              ikw<"PIPE">) +
                          assign<&ast::OutputReport::reportTo>(
                              ref<Expression>)) |
                         flag<&ast::OutputReport::toPrinter>(
                             ikw<"PRINTER">))) +
                 option(ikw<"WITH"> +
                        many(append<&ast::OutputReport::clauses>(
                            ReportClauseR))))> {};

  struct StartReportStmtR
      : rule<create<ast::StartReportStmt>(
                 ikw<"START"> + ikw<"REPORT"> +
                 assign<&ast::StartReportStmt::name>(Name) +
                 assign<&ast::StartReportStmt::output>(
                     ref<StartReportTail>))> {};

  struct TerminateReportStmtR
      : rule<create<ast::TerminateReportStmt>(
                 ikw<"TERMINATE"> + ikw<"REPORT"> +
                 assign<&ast::TerminateReportStmt::name>(Name))> {};

  struct FinishReportStmtR
      : rule<create<ast::FinishReportStmt>(
                 ikw<"FINISH"> + ikw<"REPORT"> +
                 assign<&ast::FinishReportStmt::name>(Name))> {};

  struct PauseStmtR
      : rule<create<ast::PauseStmt> + ikw<"PAUSE"> +
             option(assign<&ast::PauseStmt::message>(ref<StringLitR>))> {};

  struct NeedStmtR
      : rule<create<ast::NeedStmt>(
                 ikw<"NEED"> + assign<&ast::NeedStmt::lines>(
                                   ref<Expression>)) +
             ikw<"LINES">> {};

  struct PrintItemColumn
      : rule<create<ast::PrintItem>(
                 flag<&ast::PrintItem::isColumn>(ikw<"COLUMN">) +
                 assign<&ast::PrintItem::value>(ref<Expression>))> {};
  struct PrintItemPageno
      : rule<create<ast::PrintItem>(
                 flag<&ast::PrintItem::isPageno>(ikw<"PAGENO">))> {};
  struct PrintItemLineno
      : rule<create<ast::PrintItem>(
                 flag<&ast::PrintItem::isLineno>(ikw<"LINENO">))> {};
  struct PrintItemByte
      : rule<create<ast::PrintItem>(
                 flag<&ast::PrintItem::isByte>(ikw<"BYTE">) +
                 assign<&ast::PrintItem::value>(ref<ComponentRef>))> {};
  struct PrintItemText
      : rule<create<ast::PrintItem>(
                 flag<&ast::PrintItem::isText>(ikw<"TEXT">) +
                 assign<&ast::PrintItem::value>(ref<ComponentRef>))> {};
  struct PrintItemExpr
      : rule<create<ast::PrintItem>(
                 assign<&ast::PrintItem::value>(ref<Expression>) +
                 option(ikw<"SPACE"> | ikw<"SPACES">) +
                 option(ikw<"WORDWRAP"> +
                        option(ikw<"RIGHT"> + ikw<"MARGIN"> +
                               SignedNumberText)))> {};

  static constexpr auto PrintItemR =
      ref<PrintItemColumn> | ref<PrintItemPageno> | ref<PrintItemLineno> |
      ref<PrintItemByte> | ref<PrintItemText> | ref<PrintItemExpr>;

  struct PrintStmtR
      : rule<create<ast::PrintStmt>(
                 ikw<"PRINT"> +
                 (option(append<&ast::PrintStmt::items>(PrintItemR) +
                         many(lit<","> + append<&ast::PrintStmt::items>(
                                             PrintItemR))) |
                  (ikw<"FILE"> +
                   assign<&ast::PrintStmt::file>(ref<StringLitR>))))> {};

  struct SkipStmtR
      : rule<create<ast::SkipStmt>(
                 ikw<"SKIP"> +
                 ((assign<&ast::SkipStmt::count>(ref<Expression>) +
                   (ikw<"LINE"> |
                    flag<&ast::SkipStmt::lines>(ikw<"LINES">))) |
                  flag<&ast::SkipStmt::toTopOfPage>(
                      ikw<"TO"> + ikw<"TOP"> + ikw<"OF"> +
                      ikw<"PAGE">)))> {};

  struct OutputToReportStmtR
      : rule<create<ast::OutputToReportStmt>(
                 ikw<"OUTPUT"> + ikw<"TO"> + ikw<"REPORT"> +
                 assign<&ast::OutputToReportStmt::name>(Name) + lit<"("> +
                 option(append<&ast::OutputToReportStmt::args>(
                            ref<Expression>) +
                        many(lit<","> +
                             append<&ast::OutputToReportStmt::args>(
                                 ref<Expression>))) +
                 lit<")">)> {};

  struct RsPageHeader
      : rule<option(ikw<"FIRST">) + ikw<"PAGE"> + ikw<"HEADER">,
             value<ast::FormatSectionKind::pageHeader>> {};
  struct RsPageTrailer
      : rule<ikw<"PAGE"> + ikw<"TRAILER">,
             value<ast::FormatSectionKind::pageTrailer>> {};
  struct RsEveryRow
      : rule<ikw<"ON"> + ikw<"EVERY"> + ikw<"ROW">,
             value<ast::FormatSectionKind::everyRow>> {};
  struct RsLastRow
      : rule<ikw<"ON"> + ikw<"LAST"> + ikw<"ROW">,
             value<ast::FormatSectionKind::lastRow>> {};
  struct RsBeforeGroup
      : rule<ikw<"BEFORE"> + ikw<"GROUP"> + ikw<"OF">,
             value<ast::FormatSectionKind::beforeGroup>> {};
  struct RsAfterGroup
      : rule<ikw<"AFTER"> + ikw<"GROUP"> + ikw<"OF">,
             value<ast::FormatSectionKind::afterGroup>> {};

  struct FormatSectionR
      : rule<create<ast::FormatSection>(
                 (assign<&ast::FormatSection::kind>(
                      ref<RsPageHeader> | ref<RsPageTrailer> |
                      ref<RsEveryRow> | ref<RsLastRow>) |
                  (assign<&ast::FormatSection::kind>(ref<RsBeforeGroup> |
                                                     ref<RsAfterGroup>) +
                   assign<&ast::FormatSection::groupVar>(
                       ref<ComponentRef>))) +
                 many(append<&ast::FormatSection::body>(ref<Statement>)))> {
  };

  struct FormatReportR
      : rule<create<ast::FormatReport>(
                 ikw<"FORMAT"> +
                 (flag<&ast::FormatReport::everyRowOnly>(
                      ikw<"EVERY"> + ikw<"ROW">) |
                  some(append<&ast::FormatReport::sections>(
                      ref<FormatSectionR>))))> {};

  // -------------------------------------------------------------- Program

  struct UnlabelledStatement
      : rule<ref<LetStmtR>                    //
             | ref<CallStmtR>                  //
             | ref<CreateTableStmtR>            //
             | ref<DropTableStmtR>               //
             | ref<CreateIndexStmtR>              //
             | ref<DropIndexStmtR>                 //
             | ref<InsertStmtR>                     //
             | ref<UpdateStatisticsStmtR>            //
             | ref<UpdateStmtR>                       //
             | ref<DeleteStmtR>                        //
             | ref<SelectStmt>                          //
             | ref<LoadStmtR>                             //
             | ref<UnloadStmtR>                            //
             | ref<DeclareCursorStmtR>                      //
             | ref<CloseCursorStmtR>                         //
             | ref<FetchStmtR>                                //
             | ref<FlushStmtR>                                 //
             | ref<OpenCursorStmtR>                             //
             | ref<PutStmtR>                                     //
             | ref<PrepareStmtR>                                  //
             | ref<ExecuteStmtR>                                   //
             | ref<LockTableStmtR>                                  //
             | ref<SetLockModeStmtR>                                 //
             | ref<SetExplainStmtR>                                   //
             | ref<SetIsolationStmtR>                                  //
             | ref<SetLogStmtR>                                         //
             | ref<CloseDatabaseStmtR>                                   //
             | ref<TxnStmtR>                                              //
             | ref<WheneverStmtR>                                          //
             | ref<DatabaseDeclR>                                           //
             | ref<RunStmtR>                                                 //
             | ref<GotoStmtR>                                                 //
             | ref<SleepStmtR>                                                 //
             | ref<ExitStmtR>                                                   //
             | ref<ContinueStmtR>                                                //
             | ref<ReturnStmtR>                                                   //
             | ref<DeferStmtR>                                                     //
             | ref<IfStmtR>                                                         //
             | ref<CaseStmtR>                                                        //
             | ref<WhileStmtR>                                                        //
             | ref<ForStmtR>                                                           //
             | ref<ForEachStmtR>                                                        //
             | ref<AllocateArrayStmtR>                                                   //
             | ref<DeallocateArrayStmtR>                                                  //
             | ref<ResizeArrayStmtR>                                                       //
             | ref<LocateStmtR>                                                             //
             | ref<FreeVariablesStmtR>                                                       //
             | ref<InitializeStmtR>                                                           //
             | ref<ValidateStmtR>                                                               //
             | ref<StartReportStmtR>                                                             //
             | ref<TerminateReportStmtR>                                                          //
             | ref<FinishReportStmtR>                                                              //
             | ref<PauseStmtR>                                                                      //
             | ref<NeedStmtR>                                                                        //
             | ref<PrintStmtR>                                                                        //
             | ref<SkipStmtR>                                                                          //
             | ref<OutputToReportStmtR>                                                                 //
             | ref<ClearStmtR>                                                                           //
             | ref<CloseWindowStmtR>                                                                      //
             | ref<CloseFormStmtR>                                                                         //
             | ref<ConstructStmtR>                                                                          //
             | ref<CurrentWindowStmtR>                                                                       //
             | ref<DisplayStmtR>                                                                               //
             | ref<DisplayArrayStmtR>                                                                          //
             | ref<DisplayFormStmtR>                                                                            //
             | ref<ErrorStmtR>                                                                                   //
             | ref<MessageStmtR>                                                                                  //
             | ref<PromptStmtR>                                                                                    //
             | ref<InputStmtR>                                                                                      //
             | ref<InputArrayStmtR>                                                                                  //
             | ref<MenuStmtR>                                                                                         //
             | ref<OpenFormStmtR>                                                                                      //
             | ref<OpenWindowStmtR>                                                                                     //
             | ref<OptionsStmtR>                                                                                         //
             | ref<ScrollStmtR>                                                                                           //
             | ref<MenuOptionStmtR>                                                                                        //
             | ref<NextFieldStmtR>> {};

  struct Statement
      : rule<create<ast::LabeledStmt>(
                 assign<&ast::LabeledStmt::label>(Name) + lit<":"> +
                 assign<&ast::LabeledStmt::inner>(
                     ref<UnlabelledStatement>)) |
             ref<UnlabelledStatement>> {};

  struct FunctionDefR
      : rule<create<ast::FunctionDef>(
                 ikw<"FUNCTION"> +
                 assign<&ast::FunctionDef::name>(FuncName) +
                 option(lit<"("> +
                        option(append<&ast::FunctionDef::params>(Name) +
                               many(lit<","> +
                                    append<&ast::FunctionDef::params>(
                                        Name))) +
                        lit<")">) +
                 many(append<&ast::FunctionDef::types>(ref<TypeDeclR>)) +
                 many(append<&ast::FunctionDef::body>(ref<Statement>)) +
                 ikw<"END"> + ikw<"FUNCTION">)> {};

  struct OutputReportR
      : rule<create<ast::OutputReport>(
                 ikw<"OUTPUT"> +
                 option(ikw<"REPORT"> + ikw<"TO"> +
                        (assign<&ast::OutputReport::reportTo>(
                             ref<StringLitR>) |
                         (flag<&ast::OutputReport::toPipe>(
                              ikw<"PIPE">) +
                          assign<&ast::OutputReport::reportTo>(
                              ref<StringLitR>)) |
                         flag<&ast::OutputReport::toPrinter>(
                             ikw<"PRINTER">))) +
                 many(append<&ast::OutputReport::clauses>(
                     ReportClauseR)))> {};

  struct ReportDefR
      : rule<create<ast::ReportDef>(
                 ikw<"REPORT"> + assign<&ast::ReportDef::name>(Name) +
                 option(lit<"("> +
                        option(append<&ast::ReportDef::params>(Name) +
                               many(lit<","> +
                                    append<&ast::ReportDef::params>(
                                        Name))) +
                        lit<")">) +
                 many(append<&ast::ReportDef::types>(ref<TypeDeclR>)) +
                 option(assign<&ast::ReportDef::output>(
                     ref<OutputReportR>)) +
                 option(ikw<"ORDER"> +
                        option(flag<&ast::ReportDef::orderExternal>(
                            ikw<"EXTERNAL">)) +
                        ikw<"BY"> +
                        append<&ast::ReportDef::orderBy>(
                            ref<ComponentRef>) +
                        many(lit<","> + append<&ast::ReportDef::orderBy>(
                                            ref<ComponentRef>))) +
                 option(assign<&ast::ReportDef::format>(
                     ref<FormatReportR>)) +
                 ikw<"END"> + ikw<"REPORT">)> {};

  struct GlobalsDeclR
      : rule<create<ast::GlobalsDecl>(
                 ikw<"GLOBALS"> +
                 (assign<&ast::GlobalsDecl::fromFile>(
                      ref<STRING_DQ> | ref<STRING_SQ>) |
                  (many(append<&ast::GlobalsDecl::types>(ref<TypeDeclR>)) +
                   ikw<"END"> + ikw<"GLOBALS">)))> {};

  struct MainBlockR
      : rule<create<ast::MainBlock>(
                 ikw<"MAIN"> +
                 many(append<&ast::MainBlock::types>(ref<TypeDeclR>)) +
                 many(append<&ast::MainBlock::body>(ref<Statement>)) +
                 ikw<"END"> + ikw<"MAIN">)> {};

  struct ProgramR
      : rule<create<ast::Program>(
                 option(assign<&ast::Program::database>(
                     ref<DatabaseDeclR>)) +
                 option(assign<&ast::Program::globals>(
                     ref<GlobalsDeclR>)) +
                 many(append<&ast::Program::types>(ref<TypeDeclR>)) +
                 assign<&ast::Program::main>(ref<MainBlockR>) +
                 many(append<&ast::Program::definitions>(
                     ref<FunctionDefR> | ref<ReportDefR>)))> {};

  using entry = ProgramR;
  using ignore = list<WS, SL_COMMENT, SL_COMMENT2>;
};

}  // namespace informix
