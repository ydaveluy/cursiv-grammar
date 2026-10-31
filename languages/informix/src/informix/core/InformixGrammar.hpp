// The informix language: its grammar. Informix 4GL: a screen-oriented
// procedural language with embedded SQL. Keywords are case-insensitive;
// identifiers are ASCII letters, digits and underscore. EOL is trivia;
// semicolons terminate statements and retain PRINT newline suppression.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "informix/core/ast.hpp"

namespace informix {

inline constexpr auto unquote =
    [](std::string_view t, std::pmr::memory_resource* memory) {
      t = t.substr(1, t.size() - 2);
      if (t.find('\\') == t.npos) return t;
      const auto room = cursiv::text_room(memory, t.size());
      std::size_t length = 0;
      for (std::size_t i = 0; i < t.size(); ++i) {
        if (t[i] == '\\' && i + 1 < t.size()) ++i;
        room[length++] = t[i];
      }
      return std::string_view(room.data(), length);
    };

struct Informix : cursiv::GrammarSpec {
  // ---------------------------------------------------------------- Lexer

  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct SEMIS : terminal<some(lit<";">)> {};
  struct SL_COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};
  struct SL_COMMENT2 : terminal<lit<"--", !cr<"@">> + many(cr<R"(^\r\n)">)> {};
  struct BLOCK_COMMENT : terminal<through(lit<"{">, lit<"}">)> {};
  struct CLASSIC_LINE : terminal<lit<"--@">> {};

  struct ID : terminal<cr<"A-Za-z_"> + many(cr<"A-Za-z_0-9">)> {};

  struct UINT : terminal<some(d)> {};
  struct NUMBER
      : terminal<as<double>(
            ((some(d) + option(lit<"."> + many(d))) | (lit<"."> + some(d))) +
            option(cr<"eE"> + option(cr<"+-">) + some(d)))> {};

  struct STRING_DQ
      : terminal<as<unquote>(lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\)">) + lit<"\"">)> {};
  struct STRING_SQ
      : terminal<as<unquote>(lit<"'"> + many((lit<"\\"> + any) | cr<R"(^'\\)">) + lit<"'">)> {};

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
      ikw<"MOD"> | ikw<"DATE"> | ikw<"ORD">;
  static constexpr auto FuncName = Name | FuncKeyword;

  // -------------------------------------------------------------- Operators

  struct OP_NE : terminal<as<ast::Op::ne>(lit<"<>"> | lit<"!="> | lit<"^=">)> {
  };

  static constexpr auto OpMul =
      as<ast::Op::multiply>(lit<"*">) | as<ast::Op::divide>(lit<"/">) | as<ast::Op::int_div>(ikw<"DIV">);
  static constexpr auto OpAdd = as<ast::Op::add>(lit<"+">) | as<ast::Op::subtract>(lit<"-">);
  static constexpr auto OpConcat = as<ast::Op::concat>(lit<"||">);
  static constexpr auto OpCompare = ref<OP_NE> | as<ast::Op::le>(lit<"<=">) | as<ast::Op::ge>(lit<">=">) |
                                    as<ast::Op::eq>(lit<"=="> | lit<"=">) | as<ast::Op::lt>(lit<"<">) | as<ast::Op::gt>(lit<">">);
  static constexpr auto OpAnd = as<ast::Op::logical_and>(ikw<"AND">);
  static constexpr auto OpOr = as<ast::Op::logical_or>(ikw<"OR">);

  struct CmpGe : rule<as<ast::CompareOp::ge>(lit<">=">)> {};
  struct CmpGt : rule<as<ast::CompareOp::gt>(lit<">">)> {};

  // ------------------------------------------------------------------ Expr

  struct Expression;
  struct Unary;
  struct ArithConcat;
  struct LikeExpression;
  struct Postfix;
  struct Atom;
  struct ComponentRef;
  struct Primary;
  struct FuncCall;
  struct TemporalExprR;
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
                     | ref<TemporalExprR>                     //
                     | ref<FuncCall>                          //
                     | ref<SubqueryExprR>                     //
                     | ref<TupleExprR>                        //
                     | ref<ComponentRef>> {};

  static constexpr auto AnyUnit = as<ast::TimeUnit::year>(ikw<"YEAR">) | as<ast::TimeUnit::month>(ikw<"MONTH">) |
                                  as<ast::TimeUnit::day>(ikw<"DAY">) | as<ast::TimeUnit::hour>(ikw<"HOUR">) |
                                  as<ast::TimeUnit::minute>(ikw<"MINUTE">) | as<ast::TimeUnit::second>(ikw<"SECOND">) |
                                  as<ast::TimeUnit::fraction>(ikw<"FRACTION">);

  template<class T> static constexpr auto FromUnit =
      assign<&T::from>(AnyUnit) + option(lit<"("> +
          assign<&T::fromPrecision>(ref<UINT>) + lit<")">);
  template<class T> static constexpr auto ToUnit =
      assign<&T::to>(AnyUnit) + option(lit<"("> +
          assign<&T::toPrecision>(ref<UINT>) + lit<")">);
  static constexpr auto TemporalChar = cr<R"(0-9:.+ \t\r\n-)">;
  struct TEMPORAL_BODY : terminal<lit<"("> + many(TemporalChar) + d +
      many(TemporalChar) + lit<")">> {};
  struct TemporalExprR : rule<create<ast::TemporalExpr>(
      ((assign<&ast::TemporalExpr::kind>(
            as<ast::TemporalKind::datetime_literal>(ikw<"DATETIME">) |
            as<ast::TemporalKind::interval_literal>(ikw<"INTERVAL">)) +
        assign<&ast::TemporalExpr::payload>(ref<TEMPORAL_BODY>) +
        FromUnit<ast::TemporalExpr> + ikw<"TO"> + ToUnit<ast::TemporalExpr>) |
       (assign<&ast::TemporalExpr::kind>(as<ast::TemporalKind::current>(ikw<"CURRENT">)) +
        option(FromUnit<ast::TemporalExpr> + ikw<"TO"> + ToUnit<ast::TemporalExpr>))))> {};

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
            assign<&ast::QuantifiedExpr::left>(ref<LikeExpression>) +
            assign<&ast::QuantifiedExpr::op>(
                as<ast::CompareOp::ne>(ref<OP_NE>) |
                as<ast::CompareOp::le>(lit<"<=">) | ref<CmpGe> |
                as<ast::CompareOp::eq>(lit<"=="> | lit<"=">) |
                as<ast::CompareOp::lt>(lit<"<">) | ref<CmpGt>) +
            option(flag<&ast::QuantifiedExpr::all>(ikw<"ALL">) |
                   flag<&ast::QuantifiedExpr::any>(ikw<"ANY">) |
                   flag<&ast::QuantifiedExpr::some>(ikw<"SOME">)) +
            lit<"("> + assign<&ast::QuantifiedExpr::subquery>(ref<SelectStmt>) +
            lit<")">)> {};

  struct Unary
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"-"> | lit<"+">) +
                 assign<&ast::UnaryExpr::operand>(ref<Unary>)) |
             ref<UnitsAtom>> {};

  static constexpr auto OpPower = as<ast::Op::power>(lit<"**">) |
                                  as<ast::Op::mod>(ikw<"MOD">);
  struct ArithConcat
      : infix<Unary, ast::BinaryExpr,
              levels(left(OpPower), left(OpMul), left(OpAdd), left(OpConcat))> {};

  struct LikeExpression : rule<ref<ArithConcat> + option(
      nest<ast::LikeExpr, &ast::LikeExpr::operand>(
          option(flag<&ast::LikeExpr::negated>(ikw<"NOT">)) +
          (ikw<"LIKE"> | flag<&ast::LikeExpr::matches>(ikw<"MATCHES">)) +
          assign<&ast::LikeExpr::pattern>(ref<LikeExpression>) +
          option(ikw<"ESCAPE"> + assign<&ast::LikeExpr::escape>(ref<StringLitR>))))> {};

  struct ComparisonBase : rule<ref<QuantifiedExprR> | ref<ExistsExprR> | ref<LikeExpression>> {};
  struct Comparison : infix<ComparisonBase, ast::BinaryExpr, left(OpCompare)> {};
  struct Membership : rule<ref<Comparison> + option(
      nest<ast::InExpr, &ast::InExpr::operand>(
          option(flag<&ast::InExpr::negated>(ikw<"NOT">)) + ikw<"IN"> + lit<"("> +
          (assign<&ast::InExpr::subquery>(ref<SelectStmt>) |
           some(append<&ast::InExpr::values>(ref<Expression>), lit<",">)) + lit<")">))> {};
  struct BetweenExpression : rule<ref<Membership> + many(
      nest<ast::BetweenExpr, &ast::BetweenExpr::operand>(
          option(flag<&ast::BetweenExpr::negated>(ikw<"NOT">)) + ikw<"BETWEEN"> +
          assign<&ast::BetweenExpr::low>(ref<Membership>) + ikw<"AND"> +
          assign<&ast::BetweenExpr::high>(ref<Membership>)))> {};
  struct Postfix : rule<ref<BetweenExpression> + many(
      nest<ast::IsNullExpr, &ast::IsNullExpr::operand>(
          ikw<"IS"> + option(flag<&ast::IsNullExpr::negated>(ikw<"NOT">)) + ikw<"NULL">))> {};
  struct LogicalNot : rule<ref<Postfix> |
      create<ast::UnaryExpr>(assign<&ast::UnaryExpr::op>(ikw<"NOT">) +
                            assign<&ast::UnaryExpr::operand>(ref<LogicalNot>))> {};
  struct LogicalExpression : infix<LogicalNot, ast::BinaryExpr,
      levels(left(OpAnd), left(OpOr))> {};

  static constexpr auto LineNumber = ref<SignedNumber> | ref<ComponentRef>;
  static constexpr auto PrefixFormat =
      (as<ast::FormatOperator::ascii>(ikw<"ASCII">) |
       as<ast::FormatOperator::column>(ikw<"COLUMN">) |
       as<ast::FormatOperator::ord>(ikw<"ORD">)) + !lit<"(">;
  struct Expression : rule<
      create<ast::FormatExpr>(assign<&ast::FormatExpr::op>(PrefixFormat) +
                             assign<&ast::FormatExpr::operand>(ref<Expression>)) |
      (ref<LogicalExpression> + many(nest<ast::FormatExpr, &ast::FormatExpr::operand>(
          assign<&ast::FormatExpr::op>(as<ast::FormatOperator::clipped>(ikw<"CLIPPED">) |
              as<ast::FormatOperator::spaces>(ikw<"SPACE"> | ikw<"SPACES">)) |
          (assign<&ast::FormatExpr::op>(as<ast::FormatOperator::wordwrap>(ikw<"WORDWRAP">)) +
           option(ikw<"RIGHT"> + ikw<"MARGIN"> + assign<&ast::FormatExpr::rightMargin>(LineNumber))) |
          (assign<&ast::FormatExpr::op>(as<ast::FormatOperator::using_>(ikw<"USING">)) +
           assign<&ast::FormatExpr::format>(ref<Expression>)))) )> {};

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
                 ikw<"SMALLINT"> | ikw<"SMALLFLOAT"> | ikw<"REAL">) |
             (assign<&ast::ScalarType::keyword>(
                  ikw<"DECIMAL"> | ikw<"DEC"> | ikw<"NUMERIC"> |
                  ikw<"MONEY">) +
              option(SizeParen)) |
             (assign<&ast::ScalarType::keyword>(ikw<"FLOAT"> |
                                                ikw<"DOUBLE">) +
              option(lit<"("> +
                     assign<&ast::ScalarType::size1>(ref<UINT>) +
                     lit<")">))> {};

  static constexpr auto FromUnitExpr = FromUnit<ast::DateTimeType>;
  static constexpr auto ToUnitExpr = ToUnit<ast::DateTimeType>;

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

  struct QualifiedTypeName : rule<Name + many(lit<"."> + Name)> {};
  struct IndirectType
      : rule<ikw<"LIKE"> + assign<&ast::LikeType::table>(ref<QualifiedTypeName>) + lit<"."> +
             assign<&ast::LikeType::column>(Name)> {};

  struct ArrayDimsR
      : rule<create<ast::ArrayDims>(
                 lit<"["> + some(append<&ast::ArrayDims::sizes>(ref<UINT>), lit<",">) +
                 lit<"]">)> {};

  struct RecordType
      : rule<ikw<"RECORD"> + ikw<"LIKE"> +
             assign<&ast::RecordType::likeTable>(ref<QualifiedTypeName>) + lit<"."> +
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
                 some(append<&ast::TypeDecl::groups>(ref<VarGroup>), lit<",">)) + many(ref<SEMIS>)> {};

  // -------------------------------------------------------------- Statement

  struct LetStmtR
      : rule<create<ast::LetStmt>(
                 ikw<"LET"> +
                 assign<&ast::LetStmt::target>(ref<ComponentRef>) +
                 lit<"="> + some(append<&ast::LetStmt::values>(ref<Expression>), lit<",">))> {};

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

  static constexpr auto BlockKindWord =
      as<ast::BlockKind::foreach_>(ikw<"FOREACH">) | as<ast::BlockKind::for_>(ikw<"FOR">) | as<ast::BlockKind::case_>(ikw<"CASE">) | as<ast::BlockKind::construct>(ikw<"CONSTRUCT">) |
      as<ast::BlockKind::display>(ikw<"DISPLAY">) | as<ast::BlockKind::input>(ikw<"INPUT">) | as<ast::BlockKind::menu>(ikw<"MENU">) | as<ast::BlockKind::report>(ikw<"REPORT">) |
      as<ast::BlockKind::while_>(ikw<"WHILE">);

  struct ExitStmtR
      : rule<create<ast::ExitStmt>(
                 ikw<"EXIT"> +
                 assign<&ast::ExitStmt::kind>(BlockKindWord)) |
             create<ast::ExitStmt>(
                 ikw<"EXIT"> +
                 assign<&ast::ExitStmt::kind>(as<ast::BlockKind::program>(ikw<"PROGRAM">)) +
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
                 some(append<&ast::LocateStmt::vars>(ref<ComponentRef>), lit<",">) +
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
                 some(append<&ast::ValidateStmt::vars>(ref<ComponentRef>), lit<",">) +
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
                 some(append<&ast::KeyEvent::keys>(ref<Expression>), lit<",">) +
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


  struct FieldEventMoment
      : rule<create<ast::FieldEvent>(
                 assign<&ast::FieldEvent::kind>(
                     as<ast::FieldEventKind::beforeInput>(ikw<"BEFORE"> + ikw<"INPUT">) | as<ast::FieldEventKind::afterInput>(ikw<"AFTER"> + ikw<"INPUT">) |
                     as<ast::FieldEventKind::beforeRow>(ikw<"BEFORE"> + ikw<"ROW">) | as<ast::FieldEventKind::afterRow>(ikw<"AFTER"> + ikw<"ROW">) |
                     as<ast::FieldEventKind::beforeInsert>(ikw<"BEFORE"> + ikw<"INSERT">) | as<ast::FieldEventKind::afterInsert>(ikw<"AFTER"> + ikw<"INSERT">) |
                     as<ast::FieldEventKind::beforeDelete>(ikw<"BEFORE"> + ikw<"DELETE">) | as<ast::FieldEventKind::afterDelete>(ikw<"AFTER"> + ikw<"DELETE">)) +
                 many(append<&ast::FieldEvent::body>(ref<Statement>)))> {};

  struct FieldEventConstructMoment
      : rule<create<ast::FieldEvent>(
                 assign<&ast::FieldEvent::kind>(as<ast::FieldEventKind::beforeConstruct>(ikw<"BEFORE"> + ikw<"CONSTRUCT">) |
                                                as<ast::FieldEventKind::afterConstruct>(ikw<"AFTER"> + ikw<"CONSTRUCT">)) +
                 many(append<&ast::FieldEvent::body>(ref<Statement>)))> {};

  struct FieldEventField
      : rule<create<ast::FieldEvent>(
                 assign<&ast::FieldEvent::kind>(as<ast::FieldEventKind::beforeField>(ikw<"BEFORE"> + ikw<"FIELD">) |
                                                as<ast::FieldEventKind::afterField>(ikw<"AFTER"> + ikw<"FIELD">)) +
                 some(append<&ast::FieldEvent::fields>(ref<Expression>), lit<",">) +
                 many(append<&ast::FieldEvent::body>(ref<Statement>)))> {};

  struct FieldEventKey
      : rule<create<ast::FieldEvent>(
                 assign<&ast::FieldEvent::kind>(as<ast::FieldEventKind::onKey>(ikw<"ON"> + ikw<"KEY">)) + lit<"("> +
                 some(append<&ast::FieldEvent::keys>(ref<Expression>), lit<",">) +
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
                                                        ikw<"MENU">) +
                 many(append<&ast::MenuEvent::body>(ref<Statement>)))> {};

  static constexpr auto MenuOptionText =
      assign<&ast::MenuEvent::optionName>(ref<Expression>) +
      option(assign<&ast::MenuEvent::description>(ref<Expression>)) +
      option(ikw<"HELP"> + assign<&ast::MenuEvent::help>(ref<SignedNumber>));
  struct MenuEventCommand
      : rule<create<ast::MenuEvent>(
                 ikw<"COMMAND"> +
                 ((ikw<"KEY"> + lit<"("> +
                   some(append<&ast::MenuEvent::keys>(ref<Expression>), lit<",">) +
                   lit<")"> + option(MenuOptionText)) | MenuOptionText) +
                 many(append<&ast::MenuEvent::body>(ref<Statement>)))> {};

  struct MenuStmtR
      : rule<create<ast::MenuStmt>(
                 ikw<"MENU"> +
                 assign<&ast::MenuStmt::title>(ref<Expression>) +
                 many(append<&ast::MenuStmt::events>(
                     ref<MenuEventBefore> | ref<MenuEventCommand>)) +
                 ikw<"END"> + ikw<"MENU">)> {};


  struct MenuOptionStmtR
      : rule<create<ast::MenuOptionStmt>(
                 assign<&ast::MenuOptionStmt::action>(
                     as<ast::MenuOptionAction::next>(ikw<"NEXT">) | as<ast::MenuOptionAction::show>(ikw<"SHOW">) | as<ast::MenuOptionAction::hide>(ikw<"HIDE">)) +
                 ikw<"OPTION"> +
                 (flag<&ast::MenuOptionStmt::all>(ikw<"ALL">) |
                  append<&ast::MenuOptionStmt::options>(ref<Expression>)) +
                 many(lit<","> + append<&ast::MenuOptionStmt::options>(
                                      ref<Expression>)))> {};

  struct ErrorStmtR
      : rule<create<ast::ErrorStmt>(
                 ikw<"ERROR"> +
                 some(append<&ast::ErrorStmt::items>(ref<Expression>), lit<",">) +
                 option(assign<&ast::ErrorStmt::attributes>(
                     ref<AttributeListR>)))> {};

  struct MessageStmtR
      : rule<create<ast::MessageStmt>(
                 ikw<"MESSAGE"> +
                 some(append<&ast::MessageStmt::items>(ref<Expression>), lit<",">) +
                 option(assign<&ast::MessageStmt::attributes>(
                     ref<AttributeListR>)))> {};

  struct PromptStmtR
      : rule<create<ast::PromptStmt>(
                 ikw<"PROMPT"> +
                 some(append<&ast::PromptStmt::items>(ref<Expression>), lit<",">) +
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


  struct OptionItemLine
      : rule<create<ast::OptionItem>(
                 assign<&ast::OptionItem::kind>(
                     as<ast::OptionKind::messageLine>(ikw<"MESSAGE">) | as<ast::OptionKind::promptLine>(ikw<"PROMPT">) |
                     as<ast::OptionKind::menuLine>(ikw<"MENU">) | as<ast::OptionKind::commentLine>(ikw<"COMMENT">) |
                     as<ast::OptionKind::errorLine>(ikw<"ERROR">) | as<ast::OptionKind::formLine>(ikw<"FORM">)) +
                 ikw<"LINE"> +
                 assign<&ast::OptionItem::value>(ref<Expression>))> {};


  struct OptionItemAttrs
      : rule<create<ast::OptionItem>(
                 assign<&ast::OptionItem::kind>(as<ast::OptionKind::inputAttr>(ikw<"INPUT">) |
                                                as<ast::OptionKind::displayAttr>(ikw<"DISPLAY">)) +
                 assign<&ast::OptionItem::attributes>(
                     ref<AttributeListR>))> {};


  struct OptionItemKeyword
      : rule<create<ast::OptionItem>(
                 assign<&ast::OptionItem::kind>(
                     as<ast::OptionKind::inputWrap>(ikw<"INPUT"> + ikw<"WRAP">) | as<ast::OptionKind::inputNoWrap>(ikw<"INPUT"> + ikw<"NO"> + ikw<"WRAP">) |
                     as<ast::OptionKind::insertKey>(ikw<"INSERT"> + ikw<"KEY">) | as<ast::OptionKind::deleteKey>(ikw<"DELETE"> + ikw<"KEY">) |
                     as<ast::OptionKind::nextKey>(ikw<"NEXT"> + ikw<"KEY">) | as<ast::OptionKind::previousKey>(ikw<"PREVIOUS"> + ikw<"KEY">) |
                     as<ast::OptionKind::acceptKey>(ikw<"ACCEPT"> + ikw<"KEY">) | as<ast::OptionKind::helpFile>(ikw<"HELP"> + ikw<"FILE">) |
                     as<ast::OptionKind::helpKey>(ikw<"HELP"> + ikw<"KEY">) | as<ast::OptionKind::sqlInterruptOn>(ikw<"SQL"> + ikw<"INTERRUPT"> + ikw<"ON">) |
                     as<ast::OptionKind::sqlInterruptOff>(ikw<"SQL"> + ikw<"INTERRUPT"> + ikw<"OFF">) | as<ast::OptionKind::fieldOrderConstrained>(ikw<"FIELD"> + ikw<"ORDER"> + ikw<"CONSTRAINED">) |
                     as<ast::OptionKind::fieldOrderUnconstrained>(ikw<"FIELD"> + ikw<"ORDER"> + ikw<"UNCONSTRAINED">)) +
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


  struct ClearStmtR
      : rule<create<ast::ClearStmt>(
                 ikw<"CLEAR"> +
                 (assign<&ast::ClearStmt::target>(as<ast::ClearTarget::form>(ikw<"FORM">)) |
                  (assign<&ast::ClearStmt::target>(as<ast::ClearTarget::window>(ikw<"WINDOW">)) +
                   assign<&ast::ClearStmt::windowName>(Name)) |
                  assign<&ast::ClearStmt::target>(as<ast::ClearTarget::screen>(option(ikw<"WINDOW">) + ikw<"SCREEN">)) |
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


  static constexpr auto LinePos =
      (assign<&ast::WindowAttrClause::position>(as<ast::WindowLinePosition::first>(ikw<"FIRST">)) +
       option(lit<"+"> + assign<&ast::WindowAttrClause::offset>(LineNumber))) |
      (assign<&ast::WindowAttrClause::position>(as<ast::WindowLinePosition::last>(ikw<"LAST">)) +
       option(lit<"-"> + assign<&ast::WindowAttrClause::offset>(LineNumber))) |
      (!(ikw<"FIRST"> | ikw<"LAST">) + assign<&ast::WindowAttrClause::line>(LineNumber));

  struct WindowLineClause
      : rule<create<ast::WindowAttrClause>(
                 assign<&ast::WindowAttrClause::kind>(
                     as<ast::WindowClauseKind::promptLine>(ikw<"PROMPT">) | as<ast::WindowClauseKind::formLine>(ikw<"FORM">) |
                     as<ast::WindowClauseKind::menuLine>(ikw<"MENU">) | as<ast::WindowClauseKind::messageLine>(ikw<"MESSAGE">)) +
                 ikw<"LINE"> + LinePos)> {};

  struct WindowCommentClause
      : rule<create<ast::WindowAttrClause>(
                 assign<&ast::WindowAttrClause::kind>(as<ast::WindowClauseKind::commentLine>(ikw<"COMMENT">)) +
                 ikw<"LINE"> + (LinePos |
                     assign<&ast::WindowAttrClause::position>(as<ast::WindowLinePosition::off>(ikw<"OFF">))))> {};

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
                 some(append<&ast::ScrollStmt::fields>(ref<Expression>), lit<",">) +
                 (flag<&ast::ScrollStmt::up>(ikw<"UP">) |
                  ikw<"DOWN">) +
                 option(ikw<"BY"> +
                        assign<&ast::ScrollStmt::byCount>(
                            ref<SignedNumber>)))> {};

  // -------------------------------------------------------- Embedded SQL

  struct TableRefR
      : rule<create<ast::TableRef>(
                 option(assign<&ast::TableRef::qualifier>(Name) +
                        option(lit<"@"> + assign<&ast::TableRef::server>(Name)) + lit<":">) +
                 option(assign<&ast::TableRef::owner>(Name | ref<STRING_DQ> | ref<STRING_SQ>) + lit<".">) +
                 assign<&ast::TableRef::name>(Name))> {};

  struct ColumnDefR
      : rule<create<ast::ColumnDef>(
                 assign<&ast::ColumnDef::name>(Name) +
                 (assign<&ast::ColumnDef::type>(ref<TypeSpec>) |
                  ((flag<&ast::ColumnDef::isByte>(ikw<"BYTE">) |
                    flag<&ast::ColumnDef::isText>(ikw<"TEXT">)) +
                   option(ikw<"IN"> +
                          (flag<&ast::ColumnDef::inTable>(ikw<"TABLE">) |
                           assign<&ast::ColumnDef::storeIn>(Name))))) +
                 option(flag<&ast::ColumnDef::notNull>(
                     ikw<"NOT"> + ikw<"NULL">)))> {};

  struct UniqueConstraintR
      : rule<create<ast::UniqueConstraint>(
                 ikw<"UNIQUE"> + lit<"("> +
                 some(append<&ast::UniqueConstraint::columns>(Name), lit<",">) +
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
                 some(append<&ast::SelectStmt::items>(ref<SelectItemR>), lit<",">) +
                 option(ikw<"INTO"> +
                        append<&ast::SelectStmt::intoVars>(
                            ref<ComponentRef>) +
                        many(lit<","> +
                             append<&ast::SelectStmt::intoVars>(
                                 ref<ComponentRef>))) +
                 ikw<"FROM"> +
                 some(append<&ast::SelectStmt::from>(ref<FromItemR>), lit<",">) +
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
                 option((assign<&ast::FetchStmt::kind>(as<ast::FetchKind::relative>(ikw<"RELATIVE">) |
                                                       as<ast::FetchKind::absolute>(ikw<"ABSOLUTE">)) +
                         assign<&ast::FetchStmt::offset>(
                             ref<Expression>)) |
                        assign<&ast::FetchStmt::kind>(
                            as<ast::FetchKind::next>(ikw<"NEXT">) | as<ast::FetchKind::previous>(ikw<"PREVIOUS"> | ikw<"PRIOR">) |
                            as<ast::FetchKind::first>(ikw<"FIRST">) | as<ast::FetchKind::last>(ikw<"LAST">) |
                            as<ast::FetchKind::current>(ikw<"CURRENT">))) +
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
                 ((flag<&ast::SetLockModeStmt::wait>(ikw<"WAIT">) +
                   option(assign<&ast::SetLockModeStmt::seconds>(ref<Expression>))) |
                  (ikw<"NOT"> + ikw<"WAIT">)))> {};

  struct SetExplainStmtR
      : rule<create<ast::SetExplainStmt>(
                 ikw<"SET"> + ikw<"EXPLAIN"> +
                 (flag<&ast::SetExplainStmt::on>(ikw<"ON">) |
                  ikw<"OFF">))> {};


  struct SetIsolationStmtR
      : rule<create<ast::SetIsolationStmt>(
                 ikw<"SET"> + ikw<"ISOLATION"> + ikw<"TO"> +
                 assign<&ast::SetIsolationStmt::level>(
                     as<ast::IsolationLevel::cursorStability>(ikw<"CURSOR"> + ikw<"STABILITY">) | as<ast::IsolationLevel::dirtyRead>(ikw<"DIRTY"> + ikw<"READ">) |
                     as<ast::IsolationLevel::committedRead>(ikw<"COMMITTED"> + ikw<"READ">) | as<ast::IsolationLevel::repeatableRead>(ikw<"REPEATABLE"> + ikw<"READ">)))> {};

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


  struct TxnStmtR
      : rule<create<ast::TxnStmt>(
                 assign<&ast::TxnStmt::kind>(
                     as<ast::TxnKind::begin>(ikw<"BEGIN"> + ikw<"WORK">) | as<ast::TxnKind::commit>(ikw<"COMMIT"> + ikw<"WORK">) | as<ast::TxnKind::rollback>(ikw<"ROLLBACK"> + ikw<"WORK">)))> {
  };



  struct WheneverStmtR
      : rule<create<ast::WheneverStmt>(
                 ikw<"WHENEVER"> +
                 assign<&ast::WheneverStmt::condition>(
                     as<ast::WheneverCondition::notFound>(ikw<"NOT"> + ikw<"FOUND">) | as<ast::WheneverCondition::sqlError>(option(ikw<"ANY">) + (ikw<"SQLERROR"> | ikw<"ERROR">)) |
                     as<ast::WheneverCondition::sqlWarning>(ikw<"SQLWARNING"> | ikw<"WARNING">)) +
                 ((assign<&ast::WheneverStmt::action>(as<ast::WheneverAction::continue_>(ikw<"CONTINUE">) |
                                                      as<ast::WheneverAction::stop>(ikw<"STOP">))) |
                  (assign<&ast::WheneverStmt::action>(as<ast::WheneverAction::call>(ikw<"CALL">)) +
                   assign<&ast::WheneverStmt::target>(Name)) |
                  (assign<&ast::WheneverStmt::action>(as<ast::WheneverAction::goto_>((ikw<"GO"> + ikw<"TO">) | ikw<"GOTO">)) +
                   option(lit<":">) +
                   assign<&ast::WheneverStmt::target>(Name))))> {};

  // ---------------------------------------------------------------- Report


  struct ReportClauseMargin
      : rule<create<ast::ReportClause>(
                 assign<&ast::ReportClause::kind>(
                     as<ast::ReportClauseKind::leftMargin>(ikw<"LEFT"> + ikw<"MARGIN">) | as<ast::ReportClauseKind::rightMargin>(ikw<"RIGHT"> + ikw<"MARGIN">) | as<ast::ReportClauseKind::topMargin>(ikw<"TOP"> + ikw<"MARGIN">) |
                     as<ast::ReportClauseKind::bottomMargin>(ikw<"BOTTOM"> + ikw<"MARGIN">) | as<ast::ReportClauseKind::pageLength>(ikw<"PAGE"> + ikw<"LENGTH">)) +
                 assign<&ast::ReportClause::value>(ref<SignedNumber>))> {};

  struct ReportClauseTopOfPage
      : rule<create<ast::ReportClause>(
                 assign<&ast::ReportClause::kind>(as<ast::ReportClauseKind::topOfPage>(ikw<"TOP"> + ikw<"OF"> + ikw<"PAGE">)) +
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
  struct PrintItemExpr : rule<assign<&ast::PrintItem::value>(ref<Expression>)> {};

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
                   assign<&ast::PrintStmt::file>(ref<StringLitR>))) + option(flag<&ast::PrintStmt::suppressNewline>(some(ref<SEMIS>))))> {};

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


  static constexpr auto FormatSectionHeader =
      assign<&ast::FormatSection::kind>(
          as<ast::FormatSectionKind::pageHeader>(ikw<"PAGE"> + ikw<"HEADER">) |
          as<ast::FormatSectionKind::pageTrailer>(ikw<"PAGE"> + ikw<"TRAILER">) |
          as<ast::FormatSectionKind::everyRow>(ikw<"ON"> + ikw<"EVERY"> + ikw<"ROW">) |
          as<ast::FormatSectionKind::lastRow>(ikw<"ON"> + ikw<"LAST"> + ikw<"ROW">)) |
      (flag<&ast::FormatSection::firstPage>(ikw<"FIRST">) +
       assign<&ast::FormatSection::kind>(as<ast::FormatSectionKind::pageHeader>(ikw<"PAGE"> + ikw<"HEADER">))) |
      (assign<&ast::FormatSection::kind>(
           as<ast::FormatSectionKind::beforeGroup>(ikw<"BEFORE"> + ikw<"GROUP"> + ikw<"OF">) |
           as<ast::FormatSectionKind::afterGroup>(ikw<"AFTER"> + ikw<"GROUP"> + ikw<"OF">)) +
       assign<&ast::FormatSection::groupVar>(ref<ComponentRef>));
  struct FormatSectionR : rule<create<ast::FormatSection>(FormatSectionHeader +
      many(append<&ast::FormatSection::body>(ref<Statement>)))> {};

  struct FormatReportR
      : rule<create<ast::FormatReport>(
                 ikw<"FORMAT"> +
                 (flag<&ast::FormatReport::everyRowOnly>(
                      ikw<"EVERY"> + ikw<"ROW">) |
                  some(append<&ast::FormatReport::sections>(
                      ref<FormatSectionR>))))> {};

  // -------------------------------------------------------------- Program

  struct NonPrintStatement
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

  struct UnlabelledStatement : rule<ref<PrintStmtR> |
      (ref<NonPrintStatement> + many(ref<SEMIS>))> {};

  struct Statement
      : rule<create<ast::LabeledStmt>(
                 option(ikw<"LABEL">) + assign<&ast::LabeledStmt::label>(Name) + lit<":"> +
                 assign<&ast::LabeledStmt::inner>(ref<Statement>)) |
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
                 ikw<"END"> + ikw<"FUNCTION">) + many(ref<SEMIS>)> {};

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
                 ikw<"END"> + ikw<"REPORT">) + many(ref<SEMIS>)> {};

  struct GlobalsDeclR
      : rule<create<ast::GlobalsDecl>(
                 ikw<"GLOBALS"> +
                 (assign<&ast::GlobalsDecl::fromFile>(
                      ref<STRING_DQ> | ref<STRING_SQ>) |
                  (many(append<&ast::GlobalsDecl::types>(ref<TypeDeclR>)) +
                   ikw<"END"> + ikw<"GLOBALS">))) + many(ref<SEMIS>)> {};

  struct MainBlockR
      : rule<create<ast::MainBlock>(
                 ikw<"MAIN"> +
                 many(append<&ast::MainBlock::types>(ref<TypeDeclR>)) +
                 many(append<&ast::MainBlock::body>(ref<Statement>)) +
                 ikw<"END"> + ikw<"MAIN">) + many(ref<SEMIS>)> {};

  struct ProgramR
      : rule<create<ast::Program>(many(ref<SEMIS>) +
                 option(assign<&ast::Program::database>(
                     ref<DatabaseDeclR>) + many(ref<SEMIS>)) +
                 option(assign<&ast::Program::globals>(
                     ref<GlobalsDeclR>)) +
                 many(append<&ast::Program::types>(ref<TypeDeclR>)) +
                 many(append<&ast::Program::definitions>(ref<FunctionDefR> | ref<ReportDefR>)) +
                 option(assign<&ast::Program::main>(ref<MainBlockR>)) +
                 many(append<&ast::Program::definitions>(
                     ref<FunctionDefR> | ref<ReportDefR>)))> {};

  using entry = ProgramR;
  using ignored = list<WS, SL_COMMENT, SL_COMMENT2, BLOCK_COMMENT, CLASSIC_LINE>;
};

}  // namespace informix
