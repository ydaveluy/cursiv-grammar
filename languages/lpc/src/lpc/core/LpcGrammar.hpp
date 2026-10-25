// The lpc language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "lpc/core/ast.hpp"

namespace lpc {

struct Lpc : cursiv::GrammarSpec {
  // -- Trivia --

  struct WS : terminal<some(s)> {};
  struct BlockComment : terminal<through(lit<"/*">, lit<"*/">)> {};
  struct LineComment : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};

  // `#define`/`#include`/any other `#...` line: syntactically recognised,
  // never expanded -- discarded exactly like a comment, so no gap.
  struct ComplexDefine
      : terminal<lit<"#"> + many(cr<R"( \t)">) + lit<"define"> +
                 many(cr<R"(^\\\r\n)"> |
                      (lit<"\\\\"> + option(lit<"\r">) + lit<"\n">) |
                      (lit<"\\"> + any))> {};
  struct ComplexInclude : terminal<lit<"#"> + many(cr<R"( \t)">) +
                                   lit<"include"> + many(cr<R"(^\r\n)">)> {};
  struct ComplexPreprocessor : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};

  // -- Lexical atoms --

  struct ID : terminal<cr<"a-zA-Z_"> + many(w)> {};

  static constexpr auto Digit = cr<"0-9">;
  static constexpr auto HexDigit = cr<"0-9a-fA-F">;
  static constexpr auto OctalDigit = cr<"0-7">;
  static constexpr auto NonzeroDigit = cr<"1-9">;
  static constexpr auto DigitSeq = some(Digit);

  static constexpr auto IntegerSuffix =
      (cr<"uU"> + option(lit<"ll"> | lit<"LL"> | cr<"lL">)) //
      | ((lit<"ll"> | lit<"LL"> | cr<"lL">)+option(cr<"uU">));

  // A binary literal takes no suffix -- the grammar gives BinaryConstant
  // none.
  struct NUMBER
      : terminal<((lit<"0"> + cr<"xX"> + some(HexDigit)) +
                  option(IntegerSuffix))                                  //
                 | (lit<"0"> + cr<"bB"> + some(cr<"01">))                 //
                 | ((NonzeroDigit + many(Digit)) + option(IntegerSuffix)) //
                 | ((lit<"0"> + many(OctalDigit)) + option(IntegerSuffix))> {};

  // The dot of a trailing-only fraction is guarded: unguarded, it eats the
  // first `.` of a range (`1..5`).
  struct REAL : terminal<(option(DigitSeq) + lit<"."> + DigitSeq) //
                         | (DigitSeq + lit<".", !lit<".">>)> {};

  static constexpr auto Escape =
      lit<"\\"> + (cr<R"('"?abfnrtv\\\\)">                                  //
                   | (lit<"x"> + some(HexDigit))                            //
                   | (lit<"u"> + repeat<4>(HexDigit))                       //
                   | (lit<"U"> + repeat<8>(HexDigit))                       //
                   | (OctalDigit + option(OctalDigit) + option(OctalDigit)) //
                   | cr<"^+.[{}]!@#$%&*()_=|/<>-">);

  struct STRING : terminal<option(lit<"@">) + lit<"\""> +
                           many(Escape | lit<"\r\n"> | lit<"\n"> |
                                (!cr<R"("\\\r\n)"> + any)) +
                           lit<"\"">> {};

  struct CHAR_LIT : terminal<lit<"'"> +
                             option(lit<"\""> | Escape | lit<"\r\n"> |
                                    lit<"\n"> | (!cr<R"('\n)"> + any)) +
                             lit<"'">> {};

  struct PARAMETER : terminal<lit<"$"> + DigitSeq> {};

  static constexpr auto HVSp = many(cr<R"( \t\r\n)">);
  struct MAPPING_OPEN : terminal<lit<"("> + HVSp + lit<"[">> {};
  struct ARRAY_OPEN : terminal<lit<"("> + HVSp + lit<"{">> {};
  // `(:` opens a closure only when no second `:` follows -- `(::x)` is a
  // parenthesised scope-call, not a closure.
  struct FUNCTION_OPEN
      : terminal<lit<"("> + many(cr<R"( \t)">) + lit<":", !lit<":">>> {};

  static constexpr auto TypeModifierKw = kw<"nomask"> | kw<"private"> |
                                         kw<"protected"> | kw<"public"> |
                                         kw<"static"> | kw<"varargs">;
  static constexpr auto BasicTypeKw =
      kw<"buffer"> | kw<"float"> | kw<"function"> | kw<"int"> | kw<"mapping"> |
      kw<"mixed"> | kw<"object"> | kw<"string"> | kw<"object_tx"> | kw<"void">;

  static constexpr auto OpMul = lit<"*"> | lit<"%"> | lit<"/">;
  static constexpr auto OpAdd = lit<"+"> | lit<"-">;
  static constexpr auto OpShift = lit<"<<"> | lit<">>">;
  static constexpr auto OpRel = lit<"<="> | lit<">="> | lit<">"> | lit<"<">;
  static constexpr auto OpEq = lit<"=="> | lit<"!=">;
  static constexpr auto OpBitAnd = lit<"&">;
  static constexpr auto OpBitXor = lit<"^">;
  static constexpr auto OpBitOr = lit<"|">;
  static constexpr auto OpLogAnd = lit<"&&">;
  static constexpr auto OpLogOr = lit<"||">;
  static constexpr auto OpAssign =
      lit<"<<="> | lit<">>="> | lit<"+="> | lit<"-="> | lit<"*="> | lit<"/="> |
      lit<"%="> | lit<"&="> | lit<"|="> | lit<"^="> | lit<"=">;

  // `constant` (switch case bounds) reuses BinaryExpr/PrefixExpr/IntLit but
  // needs its own operator tokens: an infix table claims a token for the
  // whole grammar, and the main ladder below already claims these spellings.
  // string_con1's own `+` -- same reasoning, its own token.
  struct StringPlusOp : terminal<lit<"+">> {};

  // -- Forward declarations --

  struct TypeSpecRule;
  struct SimpleTypeRule;
  struct LocalTypeRule;
  struct ParamRule;
  struct TypedVarRule;
  struct VarNameRule;
  struct MemberNameRule;
  struct ArgRule;
  struct ClassInitRule;
  struct AssocPairRule;
  struct NameExprRule;
  struct EfunRefRule;
  struct NewCallCalleeRule;
  struct CallRule;
  struct PrimaryRule;
  struct PostfixExprRule;
  struct UnaryExprRule;
  struct BinaryExprRule;
  struct CondExprRule;
  struct AssignExprRule;
  struct CommaExprRule;
  struct StringLitRule;
  struct StringPrimaryRule;
  struct StringExprRule;
  struct CaseLabelRule;
  struct SscanfExprRule;
  struct ParseCommandExprRule;
  struct TimeExprRule;
  struct CatchExprRule;
  struct FuncLitExprRule;
  struct ClosureExprRule;
  struct MappingExprRule;
  struct ArrayExprRule;
  struct StatementRule;
  struct CompoundStmtRule;
  struct LocalVarDeclRule;
  struct CaseStmtRule;
  struct DefaultStmtRule;
  struct Definition;

  // -- Types --

  // `data_type` (modifiers + an optional base) may derive nothing at all --
  // an untyped declaration is implicitly `mixed`. A rule may not match the
  // empty string, so the two non-empty shapes are spelled directly and the
  // call sites that may omit a type wrap this one in `option()`.
  struct TypeSpecRule
      : rule<create<ast::TypeSpec>(
                 some(append<&ast::TypeSpec::modifiers>(TypeModifierKw)) +
                 option(assign<&ast::TypeSpec::baseType>(BasicTypeKw) |
                        (kw<"class"> +
                         assign<&ast::TypeSpec::className>(ref<ID>)))) //
             |
             create<ast::TypeSpec>(
                 assign<&ast::TypeSpec::baseType>(BasicTypeKw) |
                 (kw<"class"> + assign<&ast::TypeSpec::className>(ref<ID>)))> {
  };

  struct LocalTypeRule
      : rule<create<ast::TypeSpec>(
            assign<&ast::TypeSpec::baseType>(BasicTypeKw) |
            (kw<"class"> + assign<&ast::TypeSpec::className>(ref<ID>)))> {};

  struct SimpleTypeRule
      : rule<create<ast::SimpleType>(
            (assign<&ast::SimpleType::baseType>(BasicTypeKw) |
             (kw<"class"> + assign<&ast::SimpleType::className>(ref<ID>))) +
            option(flag<&ast::SimpleType::isArray>(lit<"*">)))> {};

  struct TypedVarRule
      : rule<create<ast::Param>(assign<&ast::Param::type>(ref<SimpleTypeRule>) +
                                assign<&ast::Param::name>(ref<ID>))> {};

  struct ParamRule : rule<(assign<&ast::Param::type>(ref<SimpleTypeRule>) +
                           option(assign<&ast::Param::name>(ref<ID>))) //
                          | assign<&ast::Param::name>(ref<ID>)> {};

  // Spliced, not `ref<>`'d: a rule may not match nothing, and an empty
  // parameter list (`f()`) has to -- the parens around every use supply the
  // mandatory token instead.
  static constexpr auto ParamListBody = create<ast::ParamList>(
      many(append<&ast::ParamList::params>(ref<ParamRule>), lit<",">) +
      option(flag<&ast::ParamList::variadic>(lit<"...">)));

  struct VarNameRule
      : rule<option(flag<&ast::VarName::isArray>(lit<"*">)) +
             assign<&ast::VarName::name>(ref<ID>) +
             option(OpAssign +
                    assign<&ast::VarName::init>(ref<AssignExprRule>))> {};

  struct MemberNameRule
      : rule<option(flag<&ast::VarName::isArray>(lit<"*">)) +
             assign<&ast::VarName::name>(ref<ID>)> {};

  // -- Argument/init helpers --

  struct ArgRule : rule<create<ast::Arg>(
                       assign<&ast::Arg::value>(ref<AssignExprRule>) +
                       option(flag<&ast::Arg::spread>(lit<"...">)))> {};

  // Spliced for the same reason as ParamListBody: the brackets around every
  // use supply the token an empty argument list cannot.
  static constexpr auto ArgListBody = create<ast::ArgList>(
      many(append<&ast::ArgList::items>(ref<ArgRule>), lit<",">) +
      option(lit<",">));

  struct ClassInitRule
      : rule<create<ast::ClassInit>(
            assign<&ast::ClassInit::name>(ref<ID>) + lit<":"> +
            assign<&ast::ClassInit::value>(ref<AssignExprRule>))> {};

  struct AssocPairRule
      : rule<create<ast::AssocPair>(
            assign<&ast::AssocPair::key>(ref<AssignExprRule>) + lit<":"> +
            assign<&ast::AssocPair::value>(ref<AssignExprRule>))> {};

  // -- Call callees --

  struct NameExprRule
      : rule<create<ast::NameExpr>(assign<&ast::NameExpr::name>(ref<ID>)) //
             | create<ast::NameExpr>(
                   flag<&ast::NameExpr::hasQualifier>(lit<"::">) +
                   assign<&ast::NameExpr::name>(ref<ID>)) //
             | create<ast::NameExpr>(
                   assign<&ast::NameExpr::qualifier>(BasicTypeKw) +
                   flag<&ast::NameExpr::hasQualifier>(lit<"::">) +
                   assign<&ast::NameExpr::name>(ref<ID>)) //
             | create<ast::NameExpr>(
                   assign<&ast::NameExpr::qualifier>(ref<ID>) +
                   flag<&ast::NameExpr::hasQualifier>(lit<"::">) +
                   assign<&ast::NameExpr::name>(ref<ID>))> {};

  struct EfunRefRule : rule<create<ast::NameExpr>(
                           assign<&ast::NameExpr::qualifier>(kw<"efun">) +
                           flag<&ast::NameExpr::hasQualifier>(lit<"::">) +
                           assign<&ast::NameExpr::name>(ref<ID> | kw<"new">))> {
  };

  struct NewCallCalleeRule
      : rule<create<ast::NameExpr>(assign<&ast::NameExpr::name>(kw<"new">))> {};

  static constexpr auto ArrowCallTail =
      lit<"->"> + assign<&ast::MethodCallExpr::method>(ref<ID>) + lit<"("> +
      assign<&ast::MethodCallExpr::args>(ArgListBody) + lit<")">;

  struct CallRule
      : rule<create<ast::CallExpr>(
                 assign<&ast::CallExpr::callee>(ref<EfunRefRule>) + lit<"("> +
                 assign<&ast::CallExpr::args>(ArgListBody) + lit<")">) //
             | create<ast::CallExpr>(
                   assign<&ast::CallExpr::callee>(ref<NewCallCalleeRule>) +
                   lit<"("> + assign<&ast::CallExpr::args>(ArgListBody) +
                   lit<")">) //
             | create<ast::NewClassExpr>(
                   kw<"new"> + lit<"("> + kw<"class"> +
                   assign<&ast::NewClassExpr::className>(ref<ID>) +
                   many(lit<","> +
                        append<&ast::NewClassExpr::inits>(ref<ClassInitRule>)) +
                   lit<")">) //
             |
             create<ast::CallExpr>(
                 assign<&ast::CallExpr::callee>(ref<NameExprRule>) + lit<"("> +
                 assign<&ast::CallExpr::args>(ArgListBody) + lit<")">) //
             | create<ast::MethodCallExpr>(ArrowCallTail)              //
             | create<ast::CallExpr>(
                   lit<"("> +
                   assign<&ast::CallExpr::callee>(create<ast::PrefixExpr>(
                       assign<&ast::PrefixExpr::op>(lit<"*">) +
                       assign<&ast::PrefixExpr::operand>(ref<CommaExprRule>))) +
                   lit<")"> + lit<"("> +
                   assign<&ast::CallExpr::args>(ArgListBody) + lit<")">)> {};

  // -- Expressions, tightest to loosest --

  struct StringLitRule : rule<create<ast::StringLit>(some(
                             append<&ast::StringLit::pieces>(ref<STRING>)))> {};

  struct StringPrimaryRule
      : rule<ref<StringLitRule> //
             | (lit<"("> + ref<StringExprRule> + lit<")">)> {};

  struct StringExprRule
      : infix<StringPrimaryRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(ref<StringPlusOp>)> {};

  struct CatchExprRule
      : rule<create<ast::CatchExpr>(
            kw<"catch"> +
            (assign<&ast::CatchExpr::body>(ref<CompoundStmtRule>) //
             | (lit<"("> + assign<&ast::CatchExpr::body>(ref<CommaExprRule>) +
                lit<")">)))> {};

  struct TimeExprRule
      : rule<create<ast::TimeExpr>(
            kw<"time_expression"> +
            (assign<&ast::TimeExpr::body>(ref<CompoundStmtRule>) //
             | (lit<"("> + assign<&ast::TimeExpr::body>(ref<CommaExprRule>) +
                lit<")">)))> {};

  struct SscanfExprRule
      : rule<create<ast::SscanfExpr>(
            kw<"sscanf"> + lit<"("> +
            assign<&ast::SscanfExpr::input>(ref<AssignExprRule>) + lit<","> +
            assign<&ast::SscanfExpr::format>(ref<AssignExprRule>) +
            many(lit<","> +
                 append<&ast::SscanfExpr::lvalues>(ref<PostfixExprRule>)) +
            lit<")">)> {};

  struct ParseCommandExprRule
      : rule<create<ast::ParseCommandExpr>(
            kw<"parse_command"> + lit<"("> +
            assign<&ast::ParseCommandExpr::command>(ref<AssignExprRule>) +
            lit<","> +
            assign<&ast::ParseCommandExpr::rule1>(ref<AssignExprRule>) +
            lit<","> +
            assign<&ast::ParseCommandExpr::rule2>(ref<AssignExprRule>) +
            many(lit<","> + append<&ast::ParseCommandExpr::lvalues>(
                                ref<PostfixExprRule>)) +
            lit<")">)> {};

  struct FuncLitExprRule
      : rule<create<ast::FuncLitExpr>(
            assign<&ast::FuncLitExpr::returnType>(BasicTypeKw) + lit<"("> +
            assign<&ast::FuncLitExpr::params>(ParamListBody) + lit<")"> +
            assign<&ast::FuncLitExpr::body>(ref<CompoundStmtRule>))> {};

  struct ClosureExprRule
      : rule<create<ast::ClosureExpr>(
            ref<FUNCTION_OPEN> +
            assign<&ast::ClosureExpr::body>(ref<CommaExprRule>) + lit<":"> +
            lit<")">)> {};

  struct MappingExprRule
      : rule<create<ast::MappingExpr>(
            ref<MAPPING_OPEN> +
            option(some(append<&ast::MappingExpr::pairs>(ref<AssocPairRule>),
                        lit<",">) +
                   option(lit<",">)) +
            lit<"]"> + lit<")">)> {};

  struct ArrayExprRule
      : rule<create<ast::ArrayExpr>(
            ref<ARRAY_OPEN> + assign<&ast::ArrayExpr::items>(ArgListBody) +
            lit<"}"> + lit<")">)> {};

  struct PrimaryRule
      : rule<ref<CallRule>                                                    //
             | create<ast::IdentExpr>(assign<&ast::IdentExpr::name>(ref<ID>)) //
             | create<ast::ParamRefExpr>(
                   assign<&ast::ParamRefExpr::text>(ref<PARAMETER>)) //
             | create<ast::EvalExpr>(
                   lit<"$"> + lit<"("> +
                   assign<&ast::EvalExpr::index>(ref<CommaExprRule>) +
                   lit<")">)      //
             | ref<StringLitRule> //
             |
             create<ast::CharLit>(assign<&ast::CharLit::text>(ref<CHAR_LIT>)) //
             | (lit<"("> + ref<CommaExprRule> + lit<")">)                     //
             | ref<CatchExprRule>                                             //
             | ref<FuncLitExprRule>                                           //
             | ref<ClosureExprRule>                                           //
             | ref<MappingExprRule>                                           //
             | ref<ArrayExprRule>> {};

  struct PostfixExprRule
      : rule<ref<PrimaryRule> +
             many(
                 nest<ast::IndexExpr, &ast::IndexExpr::array>(
                     lit<"["> +
                     option(flag<&ast::IndexExpr::fromEnd>(lit<"<">)) +
                     assign<&ast::IndexExpr::index>(ref<CommaExprRule>) +
                     lit<"]">) //
                 |
                 nest<ast::SliceExpr, &ast::SliceExpr::array>(
                     lit<"["> +
                     option(flag<&ast::SliceExpr::lowFromEnd>(lit<"<">)) +
                     assign<&ast::SliceExpr::low>(ref<CommaExprRule>) +
                     lit<".."> +
                     option(option(flag<&ast::SliceExpr::exclusive>(
                                lit<"<">)) +
                            assign<&ast::SliceExpr::high>(ref<CommaExprRule>)) +
                     lit<"]">) //
                 | nest<ast::MethodCallExpr, &ast::MethodCallExpr::object>(
                       ArrowCallTail) //
                 | nest<ast::MemberExpr, &ast::MemberExpr::object>(
                       lit<"->"> + assign<&ast::MemberExpr::field>(ref<ID>)) //
                 | nest<ast::PostfixExpr, &ast::PostfixExpr::operand>(
                       assign<&ast::PostfixExpr::op>(lit<"++"> | lit<"--">)))> {
  };

  struct UnaryExprRule
      : rule<
            create<ast::PrefixExpr>(
                assign<&ast::PrefixExpr::op>(lit<"++"> | lit<"--">) +
                assign<&ast::PrefixExpr::operand>(ref<PostfixExprRule>)) //
            | create<ast::PrefixExpr>(
                  assign<&ast::PrefixExpr::op>(lit<"!"> | lit<"~"> | lit<"-">) +
                  assign<&ast::PrefixExpr::operand>(ref<UnaryExprRule>)) //
            | create<ast::CastExpr>(
                  lit<"("> + assign<&ast::CastExpr::type>(ref<SimpleTypeRule>) +
                  lit<")"> +
                  assign<&ast::CastExpr::operand>(ref<UnaryExprRule>))       //
            | create<ast::IntLit>(assign<&ast::IntLit::text>(ref<NUMBER>))   //
            | create<ast::FloatLit>(assign<&ast::FloatLit::text>(ref<REAL>)) //
            | ref<SscanfExprRule>                                            //
            | ref<ParseCommandExprRule>                                      //
            | ref<TimeExprRule>                                              //
            | ref<PostfixExprRule>> {};

  struct BinaryExprRule
      : infix<UnaryExprRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(OpMul) >> left(OpAdd) >> left(OpShift) >> left(OpRel) >>
               left(OpEq) >> left(OpBitAnd) >> left(OpBitXor) >>
               left(OpBitOr) >> left(OpLogAnd) >> left(OpLogOr))> {};

  struct CondExprRule
      : rule<create<ast::CondExpr>(
                 assign<&ast::CondExpr::cond>(ref<BinaryExprRule>) + lit<"?"> +
                 assign<&ast::CondExpr::thenValue>(ref<AssignExprRule>) +
                 lit<":"> +
                 assign<&ast::CondExpr::elseValue>(ref<CondExprRule>)) //
             | ref<BinaryExprRule>> {};

  struct AssignExprRule
      : rule<create<ast::AssignExpr>(
                 assign<&ast::AssignExpr::target>(ref<PostfixExprRule>) +
                 assign<&ast::AssignExpr::op>(OpAssign) +
                 assign<&ast::AssignExpr::value>(ref<AssignExprRule>)) //
             | ref<CondExprRule>> {};

  struct CommaExprRule
      : infix<AssignExprRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(lit<",">)> {};

  // Case labels share expression syntax; constancy is a runtime constraint.
  struct CaseLabelRule : rule<ref<AssignExprRule>> {};

  // -- Statements --

  struct ExprStmtRule
      : rule<create<ast::ExprStmt>(
            assign<&ast::ExprStmt::value>(ref<CommaExprRule>) + lit<";">)> {};
  struct EmptyStmtRule : rule<create<ast::EmptyStmt>(lit<";">)> {};
  struct BreakStmtRule : rule<create<ast::BreakStmt>(kw<"break"> + lit<";">)> {
  };
  struct ContinueStmtRule
      : rule<create<ast::ContinueStmt>(kw<"continue"> + lit<";">)> {};
  struct ReturnStmtRule
      : rule<create<ast::ReturnStmt>(
            kw<"return"> +
            option(assign<&ast::ReturnStmt::value>(ref<CommaExprRule>)) +
            lit<";">)> {};

  struct IfStmtRule
      : rule<kw<"if"> + lit<"("> +
             assign<&ast::IfStmt::cond>(ref<CommaExprRule>) + lit<")"> +
             assign<&ast::IfStmt::thenBranch>(ref<StatementRule>) +
             option(commit(kw<"else">, assign<&ast::IfStmt::elseBranch>(ref<StatementRule>)))> {};

  struct WhileStmtRule
      : rule<kw<"while"> + lit<"("> +
             assign<&ast::WhileStmt::cond>(ref<CommaExprRule>) + lit<")"> +
             assign<&ast::WhileStmt::body>(ref<StatementRule>)> {};

  struct DoWhileStmtRule
      : rule<kw<"do"> + assign<&ast::DoWhileStmt::body>(ref<StatementRule>) +
             kw<"while"> + lit<"("> +
             assign<&ast::DoWhileStmt::cond>(ref<CommaExprRule>) + lit<")"> +
             lit<";">> {};

  struct ForStmtRule
      : rule<
            kw<"for"> + lit<"("> +
            ((assign<&ast::ForStmt::initVar>(ref<TypedVarRule>) + OpAssign +
              assign<&ast::ForStmt::initVarValue>(ref<AssignExprRule>)) //
             | option(assign<&ast::ForStmt::initExpr>(ref<CommaExprRule>))) +
            lit<";"> + option(assign<&ast::ForStmt::cond>(ref<CommaExprRule>)) +
            lit<";"> + option(assign<&ast::ForStmt::step>(ref<CommaExprRule>)) +
            lit<")"> + assign<&ast::ForStmt::body>(ref<StatementRule>)> {};

  struct ForeachVarRule
      : rule<ref<TypedVarRule> //
             | create<ast::Param>(assign<&ast::Param::name>(ref<ID>))> {};

  struct ForeachStmtRule
      : rule<kw<"foreach"> + lit<"("> +
             append<&ast::ForeachStmt::vars>(ref<ForeachVarRule>) +
             option(lit<","> +
                    append<&ast::ForeachStmt::vars>(ref<ForeachVarRule>)) +
             kw<"in"> +
             assign<&ast::ForeachStmt::iterable>(ref<AssignExprRule>) +
             lit<")"> + assign<&ast::ForeachStmt::body>(ref<StatementRule>)> {};

  struct CaseStmtRule
      : rule<create<ast::CaseStmt>(
            kw<"case"> + assign<&ast::CaseStmt::low>(ref<CaseLabelRule>) +
            option(lit<".."> +
                   assign<&ast::CaseStmt::high>(ref<CaseLabelRule>)) +
            lit<":">)> {};
  struct DefaultStmtRule
      : rule<create<ast::DefaultStmt>(kw<"default"> + lit<":">)> {};

  struct SwitchStmtRule
      : rule<kw<"switch"> + lit<"("> +
             assign<&ast::SwitchStmt::value>(ref<CommaExprRule>) + lit<")"> +
             lit<"{"> +
             many(append<&ast::SwitchStmt::localDecls>(ref<LocalVarDeclRule>)) +
             append<&ast::SwitchStmt::items>(ref<CaseStmtRule> |
                                             ref<DefaultStmtRule>) +
             many(append<&ast::SwitchStmt::items>(ref<CaseStmtRule> |
                                                  ref<DefaultStmtRule> |
                                                  ref<StatementRule>)) +
             lit<"}">> {};

  struct StatementRule : rule<ref<ExprStmtRule>       //
                              | ref<IfStmtRule>       //
                              | ref<WhileStmtRule>    //
                              | ref<DoWhileStmtRule>  //
                              | ref<SwitchStmtRule>   //
                              | ref<ReturnStmtRule>   //
                              | ref<CompoundStmtRule> //
                              | ref<ForStmtRule>      //
                              | ref<ForeachStmtRule>  //
                              | ref<EmptyStmtRule>    //
                              | ref<BreakStmtRule>    //
                              | ref<ContinueStmtRule>> {};

  struct LocalVarDeclRule
      : rule<assign<&ast::VarDecl::type>(ref<LocalTypeRule>) +
             some(append<&ast::VarDecl::names>(ref<VarNameRule>), lit<",">) +
             lit<";">> {};

  struct CompoundStmtRule
      : rule<lit<"{"> +
             many(append<&ast::CompoundStmt::items>(ref<StatementRule>) |
                  append<&ast::CompoundStmt::items>(ref<LocalVarDeclRule>)) +
             lit<"}">> {};

  // -- Declarations --

  struct MemberDeclRule
      : rule<option(assign<&ast::VarDecl::type>(ref<TypeSpecRule>)) +
             some(append<&ast::VarDecl::names>(ref<MemberNameRule>), lit<",">) +
             lit<";">> {};

  struct ClassDeclRule
      : rule<many(append<&ast::ClassDecl::modifiers>(TypeModifierKw)) +
             kw<"class"> + assign<&ast::ClassDecl::name>(ref<ID>) + lit<"{"> +
             many(append<&ast::ClassDecl::members>(ref<MemberDeclRule>)) +
             lit<"}">> {};

  struct ModifierChangeRule
      : rule<many(append<&ast::ModifierChange::modifiers>(TypeModifierKw)) +
             lit<":">> {};

  struct InheritDeclRule
      : rule<many(append<&ast::InheritDecl::modifiers>(TypeModifierKw)) +
             kw<"inherit"> +
             assign<&ast::InheritDecl::path>(ref<StringExprRule>) + lit<";">> {
  };

  struct GlobalVarDeclRule
      : rule<option(assign<&ast::VarDecl::type>(ref<TypeSpecRule>)) +
             some(append<&ast::VarDecl::names>(ref<VarNameRule>), lit<",">) +
             lit<";">> {};

  struct FuncDeclRule
      : rule<option(assign<&ast::FuncDecl::returnType>(ref<TypeSpecRule>)) +
             option(flag<&ast::FuncDecl::returnsArray>(lit<"*">)) +
             assign<&ast::FuncDecl::name>(ref<ID>) + lit<"("> +
             assign<&ast::FuncDecl::params>(ParamListBody) + lit<")"> +
             (assign<&ast::FuncDecl::body>(ref<CompoundStmtRule>) | lit<";">)> {
  };

  struct Definition : rule<ref<FuncDeclRule>        //
                           | ref<GlobalVarDeclRule> //
                           | ref<InheritDeclRule>   //
                           | ref<ClassDeclRule>     //
                           | ref<ModifierChangeRule>> {};

  struct ProgramRule : rule<create<ast::Program> +
                            many(append<&ast::Program::items>(ref<Definition>) +
                                 option(lit<";">))> {};

  using entry = ProgramRule;
  using ignore = list<WS, BlockComment, LineComment, ComplexDefine,
                      ComplexInclude, ComplexPreprocessor>;
};

} // namespace lpc
