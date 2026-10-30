// The gdscript language: its grammar. Godot 3.x/GDScript 1.x class bodies, statements
// and expressions, with suites delimited by indentation (`indent`/`dedent`).
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "gdscript/core/ast.hpp"

namespace gdscript {

struct Gdscript : cursiv::GrammarSpec {
  // TRIVIA
  // ======

  static constexpr auto comment_body = lit<"#"> + many(cr<R"(^\r\n)">);
  struct COMMENT : terminal<comment_body> {};

  // A backslash immediately before a line break joins the two physical
  // lines; the pair is ordinary whitespace, matched here rather than left
  // for NEWLINE to see.
  struct WS : terminal<some(cr<R"( \t)"> | (lit<"\\"> + eol))> {};

  // A line with nothing but whitespace and/or a comment does not end a
  // logical line: folded into NEWLINE so the grammar never sees it.
  static constexpr auto blank_line =
      many(cr<R"( \t)">) + option(comment_body) + eol;
  struct NEWLINE : terminal<eol + many(blank_line)> {};
  struct LeadingTrivia : terminal<some(blank_line)> {};
  static constexpr auto indent_open = lit<"("> | lit<"["> | lit<"{">;
  static constexpr auto indent_close = lit<")"> | lit<"]"> | lit<"}">;
  using indent_newline = NEWLINE;
  struct NL_TRIVIA : terminal<eol> {};

  // FORWARD DECLARATIONS
  // ====================

  struct Ident;
  struct STRING;
  struct Literal;
  struct StringLit;
  struct IntLit;
  struct FloatLit;
  struct Name;
  struct Atom;
  struct ArrayDecl;
  struct KeyValue;
  struct DictDecl;
  struct GetNode;
  struct SuperCall;
  struct Primary;
  struct IsExpr;
  struct SignExpr;
  struct ArithExpr;
  struct CompareExpr;
  struct LogicNotExpr;
  struct LogicExpr;
  struct TernaryExpr;
  struct Expression;
  struct CastPrimary;
  struct ClassNameDecl;
  struct PassDecl;

  struct Stmt;
  struct VarDeclStmt;
  struct ElifClause;
  struct IfStmt;
  struct WhileStmt;
  struct ForStmt;
  struct Pattern;
  struct MatchCase;
  struct MatchStmt;
  struct ContinueStmt;
  struct BreakStmt;
  struct ReturnStmt;
  struct AssignStmt;
  struct ExprStmt;
  struct AssertStmt;
  struct BreakpointStmt;
  struct PassStmt;

  struct WildcardPattern;
  struct BindingPattern;
  struct ArrayPattern;
  struct DictKeyValuePattern;
  struct DictPattern;

  struct Inheritance;
  struct ExportDecl;
  struct SetGet;
  struct ClassVarDecl;
  struct ConstDecl;
  struct SignalDecl;
  struct EnumItem;
  struct EnumDecl;
  struct Param;
  struct MethodDecl;
  struct InnerClassDecl;
  struct ToolDecl;
  struct TopLevelDecl;
  struct Program;

  // A suite is either a single statement on the header's own line -- every
  // statement already ends on its own `stmtEnd` -- or an indented block.
  template <auto BodyPtr>
  static constexpr auto stmtOrSuite =
      append<BodyPtr>(ref<Stmt>) |
      (ref<NEWLINE> + indent + some(append<BodyPtr>(ref<Stmt>)) + dedent);

  // NAMES AND KEYWORDS
  // ==================

  static constexpr auto identBody = cr<"_a-zA-Z"> + many(cr<"_0-9a-zA-Z">);
  struct IDENTIFIER : terminal<identBody> {};

  struct Ident : rule<ref<IDENTIFIER>> {};
  struct TypeHint : rule<as<[](std::string_view t) { return t; }>(
                        ref<Ident> + many(lit<"."> + ref<Ident>))> {};
  struct NodePath : rule<as<[](std::string_view t) {
    return t;
  }>((ref<Ident> | ref<STRING>)+many(lit<"/"> + (ref<Ident> | ref<STRING>)) +
                    option(lit<"/">))> {};

  // NUMBERS
  // =======

  struct INTEGER : terminal<(lit<"0"> + cr<"xX"> + some(cr<"0-9a-fA-F_">)) |
                            (lit<"0"> + cr<"bB"> + some(cr<"01_">)) |
                            (d + many(d | lit<"_">))> {};
  static constexpr auto digits = d + many(d | lit<"_">);
  static constexpr auto floatExponent =
      cr<"eE"> + many(lit<"_">) + option(cr<"+-">) + some(d | lit<"_">);
  struct FLOAT : terminal<(digits + lit<"."> + many(d | lit<"_">) +
                           option(floatExponent)) |
                          (lit<"."> + digits + option(floatExponent)) |
                          (digits + floatExponent)> {};
  static constexpr auto escape = lit<"\\"> + any;
  struct STRING
      : terminal<(lit<"\"\"\""> + many(escape | (!lit<"\"\"\""> + any)) +
                  lit<"\"\"\"">) |
                 (lit<"\""> + many(escape | cr<R"(^\\\n")">) + lit<"\"">) |
                 (lit<"'"> + many(escape | cr<R"(^\\\n')">) + lit<"'">)> {};

  // LITERALS AND ATOMS
  // ===================

  struct StringLit : rule<assign<&ast::StringLit::text>(ref<STRING>)> {};
  struct IntLit : rule<assign<&ast::IntLit::text>(ref<INTEGER>)> {};
  struct FloatLit : rule<assign<&ast::FloatLit::text>(ref<FLOAT>)> {};
  struct Name : rule<assign<&ast::Name::id>(ref<Ident>)> {};

  struct Literal : rule<ref<StringLit> | ref<FloatLit> |
                        ref<IntLit> | ref<Name>> {};

  struct ArrayDecl
      : rule<create<ast::ListExpr>(
            lit<"["> +
            option(
                some(append<&ast::ListExpr::elts>(ref<Expression>), lit<",">) +
                option(lit<",">)) +
            lit<"]">)> {};

  struct KeyValue
      : rule<create<ast::DictItem>(
                 assign<&ast::DictItem::key>(ref<Expression>) + lit<":"> +
                 assign<&ast::DictItem::value>(ref<Expression>))  //
             | create<ast::DictItem>(
                   assign<&ast::DictItem::name>(ref<Ident>) + lit<"="> +
                   assign<&ast::DictItem::value>(ref<Expression>))> {};

  struct DictDecl
      : rule<create<ast::DictExpr>(
            lit<"{"> +
            option(some(append<&ast::DictExpr::items>(ref<KeyValue>),
                        lit<",">) +
                   option(lit<",">)) +
            lit<"}">)> {};

  struct GetNode
      : rule<create<ast::GetNodeExpr>(
            lit<"$"> + assign<&ast::GetNodeExpr::path>(ref<NodePath>))> {};

  // `.method(args)`: calls the parent class's own method of the same name.
  struct SuperCall
      : rule<create<ast::SuperCall>(
            lit<"."> + assign<&ast::SuperCall::method>(ref<Ident>) + lit<"("> +
            option(some(append<&ast::SuperCall::args>(ref<Expression>),
                        lit<",">)) +
            lit<")">)> {};

  struct Atom
      : rule<create<ast::NodePathLit>(
                 lit<"@"> + assign<&ast::NodePathLit::text>(ref<STRING>)) |
             create<ast::BoolLit>(
                 flag<&ast::BoolLit::value>(kw<"true">))  //
             | create<ast::BoolLit>(kw<"false">)               //
             | create<ast::NullLit>(kw<"null">)                //
             | create<ast::SelfExpr>(kw<"self">)               //
             | ref<Literal>                                    //
             | ref<ArrayDecl>                                  //
             | ref<DictDecl>                                   //
             | (lit<"("> + ref<Expression> + lit<")">)         //
             | ref<GetNode>                                    //
             | ref<SuperCall>> {};

  // EXPRESSIONS: POSTFIX CHAIN AND PRECEDENCE
  // ============================================
  // pure binary levels with no unary operator between them collapses into
  // one infix<> table.

  struct Primary
      : rule<
            ref<Atom> +
            many(
                nest<ast::Subscript, &ast::Subscript::value>(
                    lit<"["> + assign<&ast::Subscript::index>(ref<Expression>) +
                    lit<"]">)  //
                | nest<ast::Attribute, &ast::Attribute::value>(
                      lit<"."> + assign<&ast::Attribute::attr>(ref<Ident>))  //
                | nest<ast::CallExpr, &ast::CallExpr::func>(
                      lit<"("> +
                      option(some(append<&ast::CallExpr::args>(ref<Expression>),
                                  lit<",">)) +
                      lit<")">))> {};

  struct CastPrimary
      : rule<ref<Primary> +
             many(nest<ast::CastExpr, &ast::CastExpr::value>(
                 kw<"as"> + assign<&ast::CastExpr::type>(ref<TypeHint>)))> {};
  struct IsExpr
      : rule<ref<CastPrimary> +
             many(nest<ast::IsExpr, &ast::IsExpr::value>(
                 kw<"is"> + assign<&ast::IsExpr::type>(ref<CastPrimary>)))> {};
  struct SignExpr
      : rule<create<ast::UnaryOp>(
                 assign<&ast::UnaryOp::op>(lit<"~"> | lit<"-"> | lit<"+">) +
                 assign<&ast::UnaryOp::operand>(ref<SignExpr>)) |
             ref<IsExpr>> {};

  static constexpr auto FactorOp = lit<"*"> | lit<"/"> | lit<"%">;
  static constexpr auto PlusOp = lit<"+">;
  static constexpr auto MinusOp = lit<"-">;
  static constexpr auto ShiftOp = lit<"<<"> | lit<">>">;
  static constexpr auto BitAndOp = lit<"&">;
  static constexpr auto BitXorOp = lit<"^">;
  static constexpr auto BitOrOp = lit<"|">;

  struct ArithExpr
      : infix<SignExpr, ast::BinOp,
              levels(left(FactorOp), left(PlusOp | MinusOp), left(ShiftOp),
                     left(BitAndOp), left(BitXorOp), left(BitOrOp))> {};

  static constexpr auto CompareOp =
      lit<"<"> | lit<">"> | lit<"<="> | lit<">="> | lit<"=="> | lit<"!=">;
  static constexpr auto InOp = kw<"in">;

  struct CompareExpr
      : infix<ArithExpr, ast::BinOp,
              levels(left(CompareOp), left(InOp))> {};

  struct LogicNotExpr
      : rule<create<ast::UnaryOp>(
                 assign<&ast::UnaryOp::op>(lit<"!"> | kw<"not">) +
                 assign<&ast::UnaryOp::operand>(ref<LogicNotExpr>))  //
             | ref<CompareExpr>> {};

  static constexpr auto LogicAndOp = kw<"and"> | lit<"&&">;
  static constexpr auto LogicOrOp = kw<"or"> | lit<"||">;

  struct LogicExpr : infix<LogicNotExpr, ast::BinOp,
                           levels(left(LogicAndOp), left(LogicOrOp))> {};

  struct TernaryExpr
      : rule<create<ast::CondExpr>(
                 assign<&ast::CondExpr::body>(ref<LogicExpr>) + kw<"if"> +
                 assign<&ast::CondExpr::test>(ref<Expression>) + kw<"else"> +
                 assign<&ast::CondExpr::orelse>(ref<TernaryExpr>))  //
             | ref<LogicExpr>> {};

  struct Expression : rule<ref<TernaryExpr>> {};

  // PATTERNS (match statement)
  // ===========================

  // wildcard.
  struct WildcardPattern : rule<create<ast::WildcardPattern>(kw<"_">)> {};

  struct BindingPattern
      : rule<create<ast::BindingPattern>(
            kw<"var"> + assign<&ast::BindingPattern::name>(ref<Ident>))> {};

  struct ArrayPattern
      : rule<create<ast::ArrayPattern>(
            lit<"["> +
            option(some(append<&ast::ArrayPattern::items>(ref<Pattern>),
                        lit<",">) +
                   option(lit<",">)) +
            option(flag<&ast::ArrayPattern::openEnded>(lit<"..">) +
                   option(lit<",">)) +
            lit<"]">)> {};

  struct DictKeyValuePattern
      : rule<create<ast::DictKeyValuePattern>(
            assign<&ast::DictKeyValuePattern::key>(ref<Expression>) +
            option(lit<":"> + assign<&ast::DictKeyValuePattern::value>(
                                  ref<Pattern>)))> {};

  struct DictPattern
      : rule<create<ast::DictPattern>(
            lit<"{"> +
            option(some(append<&ast::DictPattern::items>(
                            ref<DictKeyValuePattern>),
                        lit<",">) +
                   option(lit<",">)) +
            option(flag<&ast::DictPattern::openEnded>(lit<"..">) +
                   option(lit<",">)) +
            lit<"}">)> {};

  struct Pattern
      : rule<ref<WildcardPattern>   //
             | ref<BindingPattern>  //
             | ref<ArrayPattern>    //
             | ref<DictPattern>     //
             | create<ast::LiteralPattern>(
                   assign<&ast::LiteralPattern::value>(ref<Expression>))> {};

  // STATEMENTS
  // ==========

  static constexpr auto stmtEnd = ref<NEWLINE> | lit<";"> | eof;

  struct VarDeclStmt
      : rule<create<ast::LocalVarDecl>(
            kw<"var"> + assign<&ast::LocalVarDecl::name>(ref<Ident>) +
            ((option(lit<":"> +
                     assign<&ast::LocalVarDecl::typeHint>(ref<TypeHint>)) +
              option(lit<"="> +
                     assign<&ast::LocalVarDecl::value>(ref<Expression>))) |
             (flag<&ast::LocalVarDecl::isInferred>(lit<":=">) +
              assign<&ast::LocalVarDecl::value>(ref<Expression>))) +
            stmtEnd)> {};

  struct ElifClause
      : rule<create<ast::ElifClause>(
            kw<"elif"> + assign<&ast::ElifClause::test>(ref<Expression>) +
            lit<":"> + stmtOrSuite<&ast::ElifClause::body>)> {};

  struct IfStmt
      : rule<create<ast::IfStmt>(
            kw<"if"> + assign<&ast::IfStmt::test>(ref<Expression>) + lit<":"> +
            stmtOrSuite<&ast::IfStmt::body> +
            many(append<&ast::IfStmt::elifs>(ref<ElifClause>)) +
            option(kw<"else"> + lit<":"> +
                   stmtOrSuite<&ast::IfStmt::orelse>))> {};

  struct WhileStmt
      : rule<create<ast::WhileStmt>(
            kw<"while"> + assign<&ast::WhileStmt::test>(ref<Expression>) +
            lit<":"> + stmtOrSuite<&ast::WhileStmt::body>)> {};

  struct ForStmt
      : rule<create<ast::ForStmt>(
            kw<"for"> + assign<&ast::ForStmt::target>(ref<Ident>) + kw<"in"> +
            assign<&ast::ForStmt::iter>(ref<Expression>) + lit<":"> +
            stmtOrSuite<&ast::ForStmt::body>)> {};

  struct MatchCase
      : rule<create<ast::MatchCase>(
            some(append<&ast::MatchCase::patterns>(ref<Pattern>),
                 lit<",">) +
            lit<":"> + stmtOrSuite<&ast::MatchCase::body>)> {};

  // Given the same `:` every other compound statement has before its suite
  struct MatchStmt
      : rule<create<ast::MatchStmt>(
            kw<"match"> + assign<&ast::MatchStmt::subject>(ref<Expression>) +
            lit<":"> + ref<NEWLINE> + indent +
            some(append<&ast::MatchStmt::cases>(ref<MatchCase>)) +
            dedent)> {};

  struct ContinueStmt
      : rule<create<ast::ContinueStmt>(kw<"continue"> + stmtEnd)> {};
  struct BreakStmt : rule<create<ast::BreakStmt>(kw<"break"> + stmtEnd)> {};

  struct ReturnStmt
      : rule<create<ast::ReturnStmt>(
            kw<"return"> +
            option(assign<&ast::ReturnStmt::value>(ref<Expression>)) +
            stmtEnd)> {};

  static constexpr auto AssignOp =
      lit<"="> | lit<"+="> | lit<"-="> | lit<"*="> | lit<"/="> | lit<"%="> |
      lit<"&="> | lit<"|="> | lit<"^="> | lit<"<<="> | lit<">>=">;

  struct AssignStmt
      : rule<create<ast::AssignStmt>(
            assign<&ast::AssignStmt::target>(ref<Expression>) +
            assign<&ast::AssignStmt::op>(AssignOp) +
            assign<&ast::AssignStmt::value>(ref<Expression>) + stmtEnd)> {};

  struct ExprStmt
      : rule<assign<&ast::ExprStmt::value>(ref<Expression>) + stmtEnd> {};

  struct AssertStmt
      : rule<create<ast::AssertStmt>(
            kw<"assert"> + lit<"("> +
            assign<&ast::AssertStmt::test>(ref<Expression>) +
            option(lit<","> + assign<&ast::AssertStmt::msg>(ref<Expression>)) +
            lit<")"> + stmtEnd)> {};

  struct BreakpointStmt
      : rule<create<ast::BreakpointStmt>(kw<"breakpoint"> + stmtEnd)> {};
  struct PassStmt : rule<create<ast::PassStmt>(kw<"pass"> + stmtEnd)> {};

  struct Stmt
      : rule<ref<VarDeclStmt> | ref<IfStmt> | ref<ForStmt> |
             ref<WhileStmt> | ref<MatchStmt> | ref<ContinueStmt> |
             ref<BreakStmt> | ref<ReturnStmt> | ref<AssignStmt> |
             ref<AssertStmt> | ref<ExprStmt> | ref<BreakpointStmt> |
             ref<PassStmt>> {};

  // TOP-LEVEL DECLARATIONS (class members)
  // ========================================

  struct Inheritance
      : rule<create<ast::Inheritance>(
            kw<"extends"> +
            assign<&ast::Inheritance::base>(ref<Ident> | ref<STRING>) +
            many(lit<"."> + append<&ast::Inheritance::path>(ref<Ident>)))> {};

  struct ExportDecl
      : rule<create<ast::ExportDecl>(
            kw<"export"> +
            option(lit<"("> +
                   option(assign<&ast::ExportDecl::hint>(ref<Ident>) +
                          many(lit<","> + append<&ast::ExportDecl::args>(
                                              ref<Expression>))) +
                   lit<")">))> {};

  struct SetGet
      : rule<create<ast::SetGet>(
            kw<"setget"> + option(assign<&ast::SetGet::setter>(ref<Ident>)) +
            option(lit<","> + assign<&ast::SetGet::getter>(ref<Ident>)))> {};

  static constexpr auto RpcMode = kw<"remote"> | kw<"master"> | kw<"puppet"> |
                                  kw<"remotesync"> | kw<"mastersync"> |
                                  kw<"puppetsync"> | kw<"sync"> | kw<"slave">;

  struct ClassVarDecl
      : rule<create<ast::ClassVarDecl>(
            option(
                assign<&ast::ClassVarDecl::exportDecl>(ref<ExportDecl>)) +
            option(flag<&ast::ClassVarDecl::isOnready>(kw<"onready">) |
                   assign<&ast::ClassVarDecl::rpcMode>(RpcMode)) +
            kw<"var"> + assign<&ast::ClassVarDecl::name>(ref<Ident>) +
            ((option(lit<":"> +
                     assign<&ast::ClassVarDecl::typeHint>(ref<TypeHint>)) +
              option(lit<"="> +
                     assign<&ast::ClassVarDecl::value>(ref<Expression>)))  //
             | (flag<&ast::ClassVarDecl::isInferred>(lit<":=">) +
                assign<&ast::ClassVarDecl::value>(ref<Expression>))) +
            option(assign<&ast::ClassVarDecl::setget>(ref<SetGet>)) +
            stmtEnd)> {};

  struct ConstDecl
      : rule<create<ast::ConstDecl>(
            kw<"const"> + assign<&ast::ConstDecl::name>(ref<Ident>) +
            option(
                flag<&ast::ConstDecl::isInferred>(lit<":"> + &lit<"=">) |
                (lit<":"> + assign<&ast::ConstDecl::typeHint>(ref<TypeHint>))) +
            lit<"="> + assign<&ast::ConstDecl::value>(ref<Expression>) +
            stmtEnd)> {};

  struct SignalDecl
      : rule<create<ast::SignalDecl>(
            kw<"signal"> + assign<&ast::SignalDecl::name>(ref<Ident>) +
            option(lit<"("> +
                   option(some(append<&ast::SignalDecl::params>(ref<Ident>),
                               lit<",">) +
                          option(lit<",">)) +
                   lit<")">) +
            stmtEnd)> {};

  struct EnumItem : rule<create<ast::EnumItem>(
                            assign<&ast::EnumItem::name>(ref<Ident>) +
                            option(lit<"="> + assign<&ast::EnumItem::value>(
                                                  ref<Expression>)))> {};

  struct EnumDecl
      : rule<create<ast::EnumDecl>(
            kw<"enum"> + option(assign<&ast::EnumDecl::name>(ref<Ident>)) +
            lit<"{"> +
            many(append<&ast::EnumDecl::items>(ref<EnumItem>), lit<",">) +
            option(lit<",">) + lit<"}"> + stmtEnd)> {};

  struct Param
      : rule<create<ast::Param>(
            option(flag<&ast::Param::hasVarKeyword>(kw<"var">)) +
            assign<&ast::Param::name>(ref<Ident>) +
            option(flag<&ast::Param::isInferred>(lit<":"> + &lit<"=">) |
                   (lit<":"> + assign<&ast::Param::typeHint>(ref<TypeHint>))) +
            option(lit<"="> +
                   assign<&ast::Param::defaultValue>(ref<Expression>)))> {};

  struct MethodDecl
      : rule<create<ast::MethodDecl>(
            option(assign<&ast::MethodDecl::rpcMode>(RpcMode)) +
            option(flag<&ast::MethodDecl::isStatic>(kw<"static">)) +
            kw<"func"> + assign<&ast::MethodDecl::name>(ref<Ident>) + lit<"("> +
            option(some(append<&ast::MethodDecl::params>(ref<Param>),
                        lit<",">) +
                   option(lit<",">)) +
            lit<")"> +
            option(flag<&ast::MethodDecl::hasSuperCall>(lit<".">) +
                   lit<"("> +
                   many(append<&ast::MethodDecl::superArgs>(ref<Expression>),
                        lit<",">) +
                   option(lit<",">) + lit<")">) +
            option(lit<"->"> +
                   assign<&ast::MethodDecl::returnType>(ref<TypeHint>)) +
            lit<":"> + stmtOrSuite<&ast::MethodDecl::body>)> {};

  struct InnerClassDecl
      : rule<create<ast::InnerClassDecl>(
            kw<"class"> + assign<&ast::InnerClassDecl::name>(ref<Ident>) +
            option(assign<&ast::InnerClassDecl::headerInherits>(
                ref<Inheritance>)) +
            lit<":"> +
            (append<&ast::InnerClassDecl::body>(ref<TopLevelDecl>) |
             (ref<NEWLINE> + indent +
              many(append<&ast::InnerClassDecl::body>(ref<TopLevelDecl>)) +
              dedent)))> {};
  struct ToolDecl
      : rule<create<ast::ToolDecl>(kw<"tool"> + option(stmtEnd))> {};
  struct ClassNameDecl
      : rule<kw<"class_name"> + assign<&ast::ClassNameDecl::name>(ref<Ident>) +
             option(lit<","> + assign<&ast::ClassNameDecl::icon>(ref<STRING>)) +
             option(stmtEnd)> {};
  struct PassDecl : rule<create<ast::PassDecl>(kw<"pass"> + stmtEnd)> {};
  struct TopLevelDecl
      : rule<(ref<Inheritance> + stmtEnd) | ref<ClassNameDecl> |
             ref<ToolDecl> | ref<PassDecl> | ref<ClassVarDecl> |
             ref<ConstDecl> | ref<SignalDecl> | ref<EnumDecl> |
             ref<MethodDecl> | ref<InnerClassDecl>> {};
  struct Program
      : rule<create<ast::Program>(
            option(lit<"\xef\xbb\xbf">) + option(ref<LeadingTrivia>) +
            many(append<&ast::Program::body>(ref<TopLevelDecl>)))> {};

  using entry = Program;
  using ignored = list<WS, NL_TRIVIA>;
  using hidden = list<COMMENT>;
};

}  // namespace gdscript
