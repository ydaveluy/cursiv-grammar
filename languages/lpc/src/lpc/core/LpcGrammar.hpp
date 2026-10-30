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

  // Directives remain in CST trivia. This parser does not expand macros or
  // evaluate conditional compilation; raw-source and preprocessed validity differ.
  struct ComplexDefine
      : terminal<lit<"#"> + many(cr<R"( \t)">) + lit<"define"> +
                 many(cr<R"(^\\\r\n)"> |
                      (lit<"\\\\"> + option(lit<"\r">) + lit<"\n">) |
                      (lit<"\\"> + any))> {};
  struct ComplexInclude : terminal<lit<"#"> + many(cr<R"( \t)">) +
                                   lit<"include"> + many(cr<R"(^\r\n)">)> {};
  struct ComplexPreprocessor : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};

  // -- Lexical atoms --

  static constexpr auto reserved = kw<"asm"> | kw<"if"> | kw<"else"> | kw<"switch"> | kw<"case"> | kw<"default"> | kw<"while"> | kw<"do"> | kw<"for"> | kw<"foreach"> | kw<"in"> | kw<"break"> | kw<"continue"> | kw<"return"> | kw<"inherit"> | kw<"efun"> | kw<"sscanf"> | kw<"catch"> | kw<"acatch"> | kw<"await"> | kw<"array"> | kw<"ref"> | kw<"parse_command"> | kw<"time_expression"> | kw<"class"> | kw<"new"> | kw<"promise"> | kw<"async"> | kw<"nomask"> | kw<"private"> | kw<"protected"> | kw<"public"> | kw<"static"> | kw<"varargs"> | kw<"nosave"> | kw<"buffer"> | kw<"float"> | kw<"function"> | kw<"int"> | kw<"mapping"> | kw<"mixed"> | kw<"object"> | kw<"string"> | kw<"void">;
  struct ID : terminal<!reserved + cr<"a-zA-Z_"> + many(w)> {};

  static constexpr auto Digit = cr<"0-9">;
  static constexpr auto HexDigit = cr<"0-9a-fA-F">;
  static constexpr auto OctalDigit = cr<"0-7">;
  static constexpr auto NonzeroDigit = cr<"1-9">;
  static constexpr auto DigitSeq = Digit + many(option(lit<"_">) + Digit);

  static constexpr auto IntegerSuffix =
      (cr<"uU"> + option(lit<"ll"> | lit<"LL"> | cr<"lL">)) //
      | ((lit<"ll"> | lit<"LL"> | cr<"lL">)+option(cr<"uU">));

  // A binary literal takes no suffix -- the grammar gives BinaryConstant
  // none.
  struct NUMBER : terminal<
      (lit<"0"> + cr<"xX"> + HexDigit + many(option(lit<"_">) + HexDigit) + option(IntegerSuffix)) |
      (lit<"0"> + cr<"bB"> + cr<"01"> + many(option(lit<"_">) + cr<"01">)) |
      (DigitSeq + option(IntegerSuffix))> {};
  static constexpr auto Exponent = cr<"eE"> + option(cr<"+-">) + DigitSeq;
  // Preserve the established .5 profile and guard a trailing dot before ranges.
  struct REAL : terminal<
      ((DigitSeq + lit<"."> + option(lit<"_">) + DigitSeq) + option(Exponent)) |
      ((lit<"."> + DigitSeq) + option(Exponent)) |
      (DigitSeq + lit<".", !lit<".">>) | (DigitSeq + Exponent)> {};

  // Native unknown escapes are accepted with a warning. Unicode escapes
  // have fixed widths; octal escapes consume the complete octal run.
  static constexpr auto Escape = lit<"\\"> +
      (some(OctalDigit) | (lit<"x"> + many(HexDigit)) |
       (lit<"u"> + repeat<4>(HexDigit)) | (lit<"U"> + repeat<8>(HexDigit)) |
       eol | (!cr<R"(\x0089uU)"> + any));

  struct STRING : terminal<option(lit<"@">) + lit<"\""> +
                           many(Escape | lit<"\r\n"> | lit<"\n"> |
                                (!cr<R"("\\\r\n)"> + any)) +
                           lit<"\"">> {};

  static constexpr auto CharEscape = lit<"\\"> +
      (some(OctalDigit) | (lit<"x"> + many(HexDigit)) | eol | (!cr<R"(\x0089)"> + byte));
  struct CHAR_LIT : terminal<lit<"'"> +
      (CharEscape | (!cr<R"(\x00\\)"> + byte)) + lit<"'">> {};

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
                                         kw<"static"> | kw<"varargs"> | kw<"async"> | kw<"nosave">;
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
      lit<"??="> | lit<"%="> | lit<"&="> | lit<"|="> | lit<"^="> | lit<"=">;

  // `constant` (switch case bounds) reuses BinaryExpr/PrefixExpr/IntLit but
  // needs its own operator tokens: an infix table claims a token for the
  // whole grammar, and the main ladder below already claims these spellings.
  // string_con1's own `+` -- same reasoning, its own token.
  struct StringPlusOp : terminal<lit<"+">> {};

  // -- Forward declarations --

  struct TypeSpec;
  struct SimpleType;
  struct LocalType;
  struct Param;
  struct TypedVar;
  struct VarName;
  struct MemberName;
  struct Arg;
  struct ClassInit;
  struct AssocPair;
  struct NameExpr;
  struct EfunRef;
  struct NewCallCallee;
  struct CallRule;
  struct Primary;
  struct PostfixExpr;
  struct UnaryExpr;
  struct BinaryExprRule;
  struct CondExpr;
  struct AssignExpr;
  struct CommaExpr;
  struct StringLit;
  struct StringPrimary;
  struct StringExpr;
  struct CaseLabel;
  struct SscanfExpr;
  struct ParseCommandExpr;
  struct TimeExpr;
  struct CatchExpr;
  struct FuncLitExpr;
  struct ClosureExpr;
  struct MappingExpr;
  struct ArrayExpr;
  struct Statement;
  struct CompoundStmt;
  struct LocalVarDecl;
  struct CaseStmt;
  struct DefaultStmt;
  struct Definition;

  // -- Types --

  // An untyped declaration is implicitly `mixed`. TypeSpec builds a node
  // only for an explicit type; callers use option() where it may be absent.
  template <auto Base, auto ClassName, auto Array, auto Argument>
  static constexpr auto BasicFor =
      ((assign<Base>(BasicTypeKw) |
        (kw<"class"> + assign<ClassName>(ref<ID>)) |
        (assign<Base>(kw<"promise">) + option(lit<"<"> + assign<Argument>(ref<SimpleType>) + lit<">">))) +
       option(flag<Array>(kw<"array">))) | flag<Array>(kw<"array">);
  static constexpr auto DeclBase = BasicFor<&ast::TypeSpec::baseType, &ast::TypeSpec::className,
                                           &ast::TypeSpec::arrayKeyword, &ast::TypeSpec::typeArgument>;
  static constexpr auto SimpleBase = BasicFor<&ast::SimpleType::baseType, &ast::SimpleType::className,
                                             &ast::SimpleType::arrayKeyword, &ast::SimpleType::typeArgument>;
  struct TypeSpec : rule<create<ast::TypeSpec>(
      (some(append<&ast::TypeSpec::modifiers>(TypeModifierKw)) + option(DeclBase)) | DeclBase)> {};
  struct LocalType : rule<create<ast::TypeSpec>(DeclBase)> {};
  struct SimpleType : rule<create<ast::SimpleType>(SimpleBase +
      option(flag<&ast::SimpleType::isArray>(lit<"*">)))> {};
  struct ParameterType : rule<create<ast::SimpleType>(SimpleBase +
      option(flag<&ast::SimpleType::byReference>(kw<"ref"> | lit<"&">)) +
      option(flag<&ast::SimpleType::isArray>(lit<"*">)))> {};

  struct TypedVar
      : rule<create<ast::Param>(assign<&ast::Param::type>(ref<ParameterType>) +
                                assign<&ast::Param::name>(ref<ID>))> {};

  struct Param : rule<(assign<&ast::Param::type>(ref<ParameterType>) +
                           option(assign<&ast::Param::name>(ref<ID>) +
                                  option(lit<":"> + assign<&ast::Param::defaultValue>(ref<ClosureExpr>)))) //
                          | assign<&ast::Param::name>(ref<ID>)> {};

  // Shared contents of a parameter list, including the empty list (`f()`).
  static constexpr auto ParamListBody = create<ast::ParamList>(
      many(append<&ast::ParamList::params>(ref<Param>), lit<",">) +
      option(flag<&ast::ParamList::variadic>(lit<"...">)));

  struct VarName
      : rule<option(flag<&ast::VarName::isArray>(lit<"*">)) +
             assign<&ast::VarName::name>(ref<ID>) +
             option(assign<&ast::VarName::initOp>(OpAssign) +
                    assign<&ast::VarName::init>(ref<AssignExpr>))> {};

  struct MemberName
      : rule<option(flag<&ast::VarName::isArray>(lit<"*">)) +
             assign<&ast::VarName::name>(ref<ID>)> {};

  // -- Argument/init helpers --

  struct Arg : rule<create<ast::Arg>(
                       assign<&ast::Arg::value>(ref<AssignExpr>) +
                       option(flag<&ast::Arg::spread>(lit<"...">)))> {};

  // Shared contents of an argument list; the caller supplies its delimiters.
  static constexpr auto ArgListBody = create<ast::ArgList>(
      many(append<&ast::ArgList::items>(ref<Arg>), lit<",">) +
      option(lit<",">));

  struct ClassInit
      : rule<create<ast::ClassInit>(
            assign<&ast::ClassInit::name>(ref<ID>) + lit<":"> +
            assign<&ast::ClassInit::value>(ref<AssignExpr>))> {};

  struct AssocPair
      : rule<create<ast::AssocPair>(
            assign<&ast::AssocPair::key>(ref<AssignExpr>) + lit<":"> +
            assign<&ast::AssocPair::value>(ref<AssignExpr>))> {};

  // -- Call callees --

  struct NameExpr
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

  struct EfunRef : rule<create<ast::NameExpr>(
                           assign<&ast::NameExpr::qualifier>(kw<"efun">) +
                           flag<&ast::NameExpr::hasQualifier>(lit<"::">) +
                           assign<&ast::NameExpr::name>(ref<ID> | kw<"new">))> {
  };

  struct NewCallCallee
      : rule<create<ast::NameExpr>(assign<&ast::NameExpr::name>(kw<"new">))> {};

  static constexpr auto ArrowCallTail =
      lit<"->"> + assign<&ast::MethodCallExpr::method>(ref<ID>) + lit<"("> +
      assign<&ast::MethodCallExpr::args>(ArgListBody) + lit<")">;

  struct CallRule
      : rule<create<ast::CallExpr>(
                 assign<&ast::CallExpr::callee>(ref<EfunRef>) + lit<"("> +
                 assign<&ast::CallExpr::args>(ArgListBody) + lit<")">) //
             | create<ast::CallExpr>(
                   assign<&ast::CallExpr::callee>(ref<NewCallCallee>) +
                   lit<"("> + assign<&ast::CallExpr::args>(ArgListBody) +
                   lit<")">) //
             | create<ast::NewClassExpr>(
                   kw<"new"> + lit<"("> + kw<"class"> +
                   assign<&ast::NewClassExpr::className>(ref<ID>) +
                   many(lit<","> +
                        append<&ast::NewClassExpr::inits>(ref<ClassInit>)) +
                   lit<")">) //
             |
             create<ast::CallExpr>(
                 assign<&ast::CallExpr::callee>(ref<NameExpr>) + lit<"("> +
                 assign<&ast::CallExpr::args>(ArgListBody) + lit<")">) //
             | create<ast::MethodCallExpr>(ArrowCallTail)              //
             | create<ast::CallExpr>(
                   lit<"("> +
                   assign<&ast::CallExpr::callee>(create<ast::PrefixExpr>(
                       assign<&ast::PrefixExpr::op>(lit<"*">) +
                       assign<&ast::PrefixExpr::operand>(ref<CommaExpr>))) +
                   lit<")"> + lit<"("> +
                   assign<&ast::CallExpr::args>(ArgListBody) + lit<")">)> {};

  // -- Expressions, tightest to loosest --

  struct TemplateText : terminal<some((lit<"\\"> + any) | cr<R"(^`$\\)"> | lit<"$", !lit<"{">>)> {};
  struct TemplateString : rule<lit<"`"> + many(
      append<&ast::TemplateString::parts>(create<ast::TemplatePart>(assign<&ast::TemplatePart::text>(ref<TemplateText>))) |
      append<&ast::TemplateString::parts>(create<ast::TemplatePart>(lit<"${"> +
          assign<&ast::TemplatePart::expression>(ref<AssignExpr>) + lit<"}">))) + lit<"`">> {};
  struct StringLit : rule<create<ast::StringLit>(some(
                             append<&ast::StringLit::pieces>(ref<STRING>)))> {};

  struct StringLike : rule<(ref<StringLit> | ref<TemplateString>) +
      many(nest<ast::StringConcat, &ast::StringConcat::left>(
          assign<&ast::StringConcat::right>(ref<StringLit> | ref<TemplateString>)))> {};

  struct StringPrimary
      : rule<ref<StringLit> //
             | (lit<"("> + ref<StringExpr> + lit<")">)> {};

  struct StringExpr
      : infix<StringPrimary, ast::BinaryExpr, left(ref<StringPlusOp>)> {};

  struct CatchExpr
      : rule<create<ast::CatchExpr>(
            kw<"catch"> +
            (assign<&ast::CatchExpr::body>(ref<CompoundStmt>) //
             | (lit<"("> + assign<&ast::CatchExpr::body>(ref<CommaExpr>) +
                lit<")">)))> {};

  struct AcatchExpr : rule<create<ast::AcatchExpr>(kw<"acatch"> +
      (assign<&ast::AcatchExpr::body>(ref<CompoundStmt>) |
       (lit<"("> + assign<&ast::AcatchExpr::body>(ref<CommaExpr>) + lit<")">)))> {};

  struct TimeExpr
      : rule<create<ast::TimeExpr>(
            kw<"time_expression"> +
            (assign<&ast::TimeExpr::body>(ref<CompoundStmt>) //
             | (lit<"("> + assign<&ast::TimeExpr::body>(ref<CommaExpr>) +
                lit<")">)))> {};

  struct SscanfExpr
      : rule<create<ast::SscanfExpr>(
            kw<"sscanf"> + lit<"("> +
            assign<&ast::SscanfExpr::input>(ref<AssignExpr>) + lit<","> +
            assign<&ast::SscanfExpr::format>(ref<AssignExpr>) +
            many(lit<","> +
                 append<&ast::SscanfExpr::lvalues>(ref<PostfixExpr>)) +
            lit<")">)> {};

  struct ParseCommandExpr
      : rule<create<ast::ParseCommandExpr>(
            kw<"parse_command"> + lit<"("> +
            assign<&ast::ParseCommandExpr::command>(ref<AssignExpr>) +
            lit<","> +
            assign<&ast::ParseCommandExpr::rule1>(ref<AssignExpr>) +
            lit<","> +
            assign<&ast::ParseCommandExpr::rule2>(ref<AssignExpr>) +
            many(lit<","> + append<&ast::ParseCommandExpr::lvalues>(
                                ref<PostfixExpr>)) +
            lit<")">)> {};

  struct FuncLitExpr
      : rule<create<ast::FuncLitExpr>(
            assign<&ast::FuncLitExpr::returnType>(BasicTypeKw) + lit<"("> +
            assign<&ast::FuncLitExpr::params>(ParamListBody) + lit<")"> +
            assign<&ast::FuncLitExpr::body>(ref<CompoundStmt>))> {};

  struct ClosureExpr
      : rule<create<ast::ClosureExpr>(
            ref<FUNCTION_OPEN> +
            ((assign<&ast::ClosureExpr::callee>(ref<EfunRef>) +
              option(lit<","> + assign<&ast::ClosureExpr::arguments>(create<ast::ArgList>(
                  some(append<&ast::ArgList::items>(ref<Arg>), lit<",">))))) |
             assign<&ast::ClosureExpr::body>(ref<CommaExpr>)) +
            lit<":"> + lit<")">)> {};

  struct MappingExpr
      : rule<create<ast::MappingExpr>(
            ref<MAPPING_OPEN> +
            option(some(append<&ast::MappingExpr::pairs>(ref<AssocPair>),
                        lit<",">) +
                   option(lit<",">)) +
            lit<"]"> + lit<")">)> {};

  struct ArrayExpr
      : rule<create<ast::ArrayExpr>(
            ref<ARRAY_OPEN> + assign<&ast::ArrayExpr::items>(ArgListBody) +
            lit<"}"> + lit<")">)> {};

  struct Primary
      : rule<ref<CallRule>                                                    //
             | create<ast::IdentExpr>(assign<&ast::IdentExpr::name>(ref<ID>)) //
             | create<ast::ParamRefExpr>(
                   assign<&ast::ParamRefExpr::text>(ref<PARAMETER>)) //
             | create<ast::EvalExpr>(
                   lit<"$"> + lit<"("> +
                   assign<&ast::EvalExpr::index>(ref<CommaExpr>) +
                   lit<")">)      //
             | ref<StringLike> //
             |
             create<ast::CharLit>(assign<&ast::CharLit::text>(ref<CHAR_LIT>)) //
             | (lit<"("> + ref<CommaExpr> + lit<")">)                     //
             | ref<AcatchExpr> | ref<CatchExpr>                                             //
             | ref<FuncLitExpr>                                           //
             | ref<ClosureExpr>                                           //
             | ref<MappingExpr>                                           //
             | ref<ArrayExpr>> {};

  struct PostfixExpr
      : rule<ref<Primary> +
             many(
                 nest<ast::MemberExpr, &ast::MemberExpr::object>(
                     flag<&ast::MemberExpr::optional>(lit<"?.">) + assign<&ast::MemberExpr::field>(ref<ID>)) |
                 nest<ast::MemberExpr, &ast::MemberExpr::object>(
                     flag<&ast::MemberExpr::dot>(lit<".">) + assign<&ast::MemberExpr::field>(ref<ID>)) |
                 nest<ast::IndexExpr, &ast::IndexExpr::array>(
                     flag<&ast::IndexExpr::optional>(lit<"?."> | lit<".?">) + lit<"["> +
                     assign<&ast::IndexExpr::index>(ref<CommaExpr>) + lit<"]">) |
                 (nest<ast::IndexExpr, &ast::IndexExpr::array>(
                     lit<"["> +
                     option(flag<&ast::IndexExpr::fromEnd>(lit<"<">)) +
                     assign<&ast::IndexExpr::index>(ref<CommaExpr>) +
                     lit<"]">) +
                  option(nest<ast::CallExpr, &ast::CallExpr::callee>(
                      lit<"("> + assign<&ast::CallExpr::args>(ArgListBody) + lit<")">))) //
                 |
                 nest<ast::SliceExpr, &ast::SliceExpr::array>(
                     lit<"["> +
                     option(flag<&ast::SliceExpr::lowFromEnd>(lit<"<">)) +
                     option(assign<&ast::SliceExpr::low>(ref<CommaExpr>)) +
                     lit<".."> +
                     option(option(flag<&ast::SliceExpr::exclusive>(
                                flag<&ast::SliceExpr::highFromEnd>(lit<"<">))) +
                            assign<&ast::SliceExpr::high>(ref<CommaExpr>)) +
                     lit<"]">) //
                 | nest<ast::MethodCallExpr, &ast::MethodCallExpr::object>(
                       ArrowCallTail) //
                 | nest<ast::MemberExpr, &ast::MemberExpr::object>(
                       lit<"->"> + assign<&ast::MemberExpr::field>(ref<ID>)) //
                 | nest<ast::PostfixExpr, &ast::PostfixExpr::operand>(
                       assign<&ast::PostfixExpr::op>(lit<"++"> | lit<"--">)))> {
  };

  struct UnaryExpr
      : rule<
            create<ast::PrefixExpr>(
                assign<&ast::PrefixExpr::op>(lit<"++"> | lit<"--">) +
                assign<&ast::PrefixExpr::operand>(ref<PostfixExpr>)) //
            | create<ast::PrefixExpr>(
                  assign<&ast::PrefixExpr::op>(lit<"!"> | lit<"~"> | lit<"-"> | kw<"await"> | kw<"ref"> | lit<"&">) +
                  assign<&ast::PrefixExpr::operand>(ref<UnaryExpr>)) //
            | create<ast::CastExpr>(
                  lit<"("> + assign<&ast::CastExpr::type>(ref<SimpleType>) +
                  lit<")"> +
                  assign<&ast::CastExpr::operand>(ref<UnaryExpr>))       //
            | create<ast::IntLit>(assign<&ast::IntLit::text>(ref<NUMBER>))   //
            | create<ast::FloatLit>(assign<&ast::FloatLit::text>(ref<REAL>)) //
            | ref<SscanfExpr>                                            //
            | ref<ParseCommandExpr>                                      //
            | ref<TimeExpr>                                              //
            | ref<PostfixExpr>> {};

  struct BinaryExprRule
      : infix<UnaryExpr, ast::BinaryExpr,
              levels(left(OpMul), left(OpAdd), left(OpShift), left(OpRel),
                     left(OpEq), left(OpBitAnd), left(OpBitXor), left(OpBitOr),
                     left(OpLogAnd), left(OpLogOr), left(lit<"??">))> {};

  struct CondExpr
      : rule<create<ast::CondExpr>(
                 assign<&ast::CondExpr::cond>(ref<BinaryExprRule>) + lit<"?"> +
                 assign<&ast::CondExpr::thenValue>(ref<AssignExpr>) +
                 lit<":"> +
                 assign<&ast::CondExpr::elseValue>(ref<CondExpr>)) //
             | ref<BinaryExprRule>> {};

  struct AssignExpr
      : rule<create<ast::AssignExpr>(
                 assign<&ast::AssignExpr::target>(ref<PostfixExpr>) +
                 assign<&ast::AssignExpr::op>(OpAssign) +
                 assign<&ast::AssignExpr::value>(ref<AssignExpr>)) //
             | ref<CondExpr>> {};

  struct CommaExpr
      : infix<AssignExpr, ast::BinaryExpr, left(lit<",">)> {};

  // Case labels share expression syntax; constancy is a runtime constraint.
  struct CaseLabel : rule<ref<AssignExpr>> {};

  // -- Statements --

  struct ExprStmt
      : rule<create<ast::ExprStmt>(
            assign<&ast::ExprStmt::value>(ref<CommaExpr>) + lit<";">)> {};
  struct EmptyStmt : rule<create<ast::EmptyStmt>(lit<";">)> {};
  struct BreakStmt : rule<create<ast::BreakStmt>(kw<"break"> + lit<";">)> {
  };
  struct ContinueStmt
      : rule<create<ast::ContinueStmt>(kw<"continue"> + lit<";">)> {};
  struct ReturnStmt
      : rule<create<ast::ReturnStmt>(
            kw<"return"> +
            option(assign<&ast::ReturnStmt::value>(ref<CommaExpr>)) +
            lit<";">)> {};

  struct IfStmt
      : rule<kw<"if"> + lit<"("> +
             assign<&ast::IfStmt::cond>(ref<CommaExpr>) + lit<")"> +
             assign<&ast::IfStmt::thenBranch>(ref<Statement>) +
             option(commit(kw<"else">, assign<&ast::IfStmt::elseBranch>(ref<Statement>)))> {};

  struct WhileStmt
      : rule<kw<"while"> + lit<"("> +
             assign<&ast::WhileStmt::cond>(ref<CommaExpr>) + lit<")"> +
             assign<&ast::WhileStmt::body>(ref<Statement>)> {};

  struct DoWhileStmt
      : rule<kw<"do"> + assign<&ast::DoWhileStmt::body>(ref<Statement>) +
             kw<"while"> + lit<"("> +
             assign<&ast::DoWhileStmt::cond>(ref<CommaExpr>) + lit<")"> +
             lit<";">> {};

  struct ForStmt
      : rule<
            kw<"for"> + lit<"("> +
            ((assign<&ast::ForStmt::initVar>(ref<TypedVar>) + assign<&ast::ForStmt::initVarOp>(OpAssign) +
              assign<&ast::ForStmt::initVarValue>(ref<AssignExpr>)) //
             | option(assign<&ast::ForStmt::initExpr>(ref<CommaExpr>))) +
            lit<";"> + option(assign<&ast::ForStmt::cond>(ref<CommaExpr>)) +
            lit<";"> + option(assign<&ast::ForStmt::step>(ref<CommaExpr>)) +
            lit<")"> + assign<&ast::ForStmt::body>(ref<Statement>)> {};

  struct ForeachVar
      : rule<ref<TypedVar> //
             | create<ast::Param>(assign<&ast::Param::name>(ref<ID>))> {};

  struct ForeachStmt
      : rule<kw<"foreach"> + lit<"("> +
             append<&ast::ForeachStmt::vars>(ref<ForeachVar>) +
             option(lit<","> +
                    append<&ast::ForeachStmt::vars>(ref<ForeachVar>)) +
             kw<"in"> +
             assign<&ast::ForeachStmt::iterable>(ref<AssignExpr>) +
             lit<")"> + assign<&ast::ForeachStmt::body>(ref<Statement>)> {};

  struct CaseStmt
      : rule<create<ast::CaseStmt>(
            kw<"case"> +
            ((assign<&ast::CaseStmt::low>(ref<CaseLabel>) +
              option(flag<&ast::CaseStmt::range>(lit<"..">) +
                     option(assign<&ast::CaseStmt::high>(ref<CaseLabel>)))) |
             (flag<&ast::CaseStmt::range>(lit<"..">) + assign<&ast::CaseStmt::high>(ref<CaseLabel>))) +
            lit<":">)> {};
  struct DefaultStmt
      : rule<create<ast::DefaultStmt>(kw<"default"> + lit<":">)> {};

  struct SwitchStmt
      : rule<kw<"switch"> + lit<"("> +
             assign<&ast::SwitchStmt::value>(ref<CommaExpr>) + lit<")"> +
             lit<"{"> +
             many(append<&ast::SwitchStmt::localDecls>(ref<LocalVarDecl>)) +
             append<&ast::SwitchStmt::items>(ref<CaseStmt> |
                                             ref<DefaultStmt>) +
             many(append<&ast::SwitchStmt::items>(ref<CaseStmt> |
                                                  ref<DefaultStmt> |
                                                  ref<Statement>)) +
             lit<"}">> {};

  struct Statement : rule<ref<ExprStmt>       //
                              | ref<IfStmt>       //
                              | ref<WhileStmt>    //
                              | ref<DoWhileStmt>  //
                              | ref<SwitchStmt>   //
                              | ref<ReturnStmt>   //
                              | ref<CompoundStmt> //
                              | ref<ForStmt>      //
                              | ref<ForeachStmt>  //
                              | ref<EmptyStmt>    //
                              | ref<BreakStmt>    //
                              | ref<ContinueStmt>> {};

  struct LocalVarDecl
      : rule<assign<&ast::VarDecl::type>(ref<LocalType>) +
             some(append<&ast::VarDecl::names>(ref<VarName>), lit<",">) +
             lit<";">> {};

  struct CompoundStmt
      : rule<lit<"{"> +
             many(append<&ast::CompoundStmt::items>(ref<Statement>) |
                  append<&ast::CompoundStmt::items>(ref<LocalVarDecl>)) +
             lit<"}">> {};

  // -- Declarations --

  struct MemberDecl
      : rule<option(assign<&ast::VarDecl::type>(ref<TypeSpec>)) +
             some(append<&ast::VarDecl::names>(ref<MemberName>), lit<",">) +
             lit<";">> {};

  struct ClassDecl
      : rule<many(append<&ast::ClassDecl::modifiers>(TypeModifierKw)) +
             kw<"class"> + assign<&ast::ClassDecl::name>(ref<ID>) + lit<"{"> +
             many(append<&ast::ClassDecl::members>(ref<MemberDecl>)) +
             lit<"}">> {};

  struct ModifierChange
      : rule<many(append<&ast::ModifierChange::modifiers>(TypeModifierKw)) +
             lit<":">> {};

  struct InheritDecl
      : rule<many(append<&ast::InheritDecl::modifiers>(TypeModifierKw)) +
             kw<"inherit"> +
             assign<&ast::InheritDecl::path>(ref<StringExpr>) + lit<";">> {
  };

  struct GlobalVarDecl
      : rule<option(assign<&ast::VarDecl::type>(ref<TypeSpec>)) +
             some(append<&ast::VarDecl::names>(ref<VarName>), lit<",">) +
             lit<";">> {};

  struct FuncDecl
      : rule<option(assign<&ast::FuncDecl::returnType>(ref<TypeSpec>)) +
             option(flag<&ast::FuncDecl::returnsArray>(lit<"*">)) +
             assign<&ast::FuncDecl::name>(ref<ID>) + lit<"("> +
             assign<&ast::FuncDecl::params>(ParamListBody) + lit<")"> +
             (assign<&ast::FuncDecl::body>(ref<CompoundStmt>) | lit<";">)> {
  };

  struct Definition : rule<ref<FuncDecl>        //
                           | ref<GlobalVarDecl> //
                           | ref<InheritDecl>   //
                           | ref<ClassDecl>     //
                           | ref<ModifierChange>> {};

  struct Program : rule<create<ast::Program> +
                            many(append<&ast::Program::items>(ref<Definition>) +
                                 option(lit<";">))> {};

  using entry = Program;
  using ignored = list<WS, BlockComment, LineComment, ComplexDefine,
                       ComplexInclude, ComplexPreprocessor>;
};

} // namespace lpc
