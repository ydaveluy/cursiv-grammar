// The Squirrel language: its grammar.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "squirrel/core/ast.hpp"

namespace squirrel {

inline constexpr auto unquote_string = [](std::string_view t) {
  return t.front() == '@' ? t.substr(2, t.size() - 3) : t.substr(1, t.size() - 2);
};
using Unquoted = cursiv::text_conv<unquote_string>;

struct Squirrel : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\f)">)> {};
  struct SL_COMMENT : terminal<(lit<"//"> | lit<"#">) + many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  // Significant: statements end at a NL the way they end at `;` (see
  // `StatementSep`), so it is never dropped as trivia.
  struct NL : terminal<some(cr<R"(\r\n)">)> {};

  struct IDENT : terminal<cr<"a-zA-Z_"> + many(w)> {};

  static constexpr auto IntBody = (lit<"0"> + cr<"1-7"> + many(cr<"0-7">))  //
                                   | (lit<"0x"> + some(cr<"0-9a-fA-F">))    //
                                   | (lit<"'"> + cr<"a-zA-Z"> + lit<"'">)   //
                                   | lit<"0">                              //
                                   | (cr<"1-9"> + many(d));
  struct INT : terminal<IntBody> {};
  // Only reachable from a `const`/`enum` value: see `ConstLiteral`.
  struct SIGNED_INT : terminal<option(lit<"-">) + IntBody> {};

  static constexpr auto FloatExp = cr<"eE"> + option(cr<"+-">) + some(d);
  static constexpr auto FloatBody =
      (((some(d) + lit<"."> + many(d)) | (many(d) + lit<"."> + some(d))) +
       option(FloatExp))               //
      | (some(d) + FloatExp);
  struct FLOAT : terminal<FloatBody> {};
  struct SIGNED_FLOAT : terminal<option(lit<"-">) + FloatBody> {};

  static constexpr auto Escape = lit<"\\"> + any;
  struct STRING
      : terminal<(lit<"@\""> + many(cr<R"(^")"> | lit<"\"\"">) + lit<"\"">)  //
                 | (lit<"\""> + many(Escape | cr<R"(^"\r\n)">) + lit<"\"">),
                 Unquoted> {};

  struct BOOLK : terminal<kw<"true"> | kw<"false">, bool> {};

  static constexpr auto MulOp = lit<"*"> | lit<"/"> | lit<"%">;
  static constexpr auto AddOp = lit<"+"> | lit<"-">;
  static constexpr auto ShiftOp = lit<"<<"> | lit<">>>"> | lit<">>">;
  static constexpr auto RelOp =
      lit<"<="> | lit<">="> | lit<"<"> | lit<">"> | kw<"instanceof">;
  static constexpr auto EqOp = lit<"=="> | lit<"!="> | lit<"<=>">;
  static constexpr auto AndInOp = lit<"&&"> | kw<"in">;
  static constexpr auto AssignOp = lit<"<-"> | lit<"*="> | lit<"/="> |
                                    lit<"%="> | lit<"+="> | lit<"-="> |
                                    lit<"=", !cr<"=">>;
  static constexpr auto UnaryOp = lit<"-"> | lit<"!"> | lit<"~">;
  static constexpr auto UnaryAction =
      kw<"typeof"> | kw<"clone"> | kw<"delete"> | kw<"resume">;

  static constexpr auto StatementSep = some(lit<";"> | ref<NL>) | &lit<"}"> |
                                        eof;
  static constexpr auto LeadingSeps = many(lit<";"> | ref<NL>);

  // Forward declarations: the expression and statement grammars are
  // mutually recursive.
  struct Expression;
  struct AssignExprRule;
  struct Statement;
  struct StatementOrEmpty;
  struct ClassMemberRule;
  struct FunctionDeclRule;

  // ---- literals and names ----

  struct LiteralExpr
      : rule<create<ast::IntLit>(assign<&ast::IntLit::text>(ref<INT>))  //
             | create<ast::FloatLit>(
                   assign<&ast::FloatLit::text>(ref<FLOAT>))  //
             | create<ast::StringLit>(
                   assign<&ast::StringLit::value>(ref<STRING>))  //
             | create<ast::BoolLit>(
                   assign<&ast::BoolLit::value>(ref<BOOLK>))  //
             | create<ast::NullLit>(kw<"null">)> {};

  // Optional leading `-`: reachable only from a `const`/`enum` value. In an
  // ordinary expression a leading `-` is always claimed first by the unary
  // operator (`UnaryExprRule`), so the two spellings never compete for the
  // same input; folding the sign into a wider terminal here keeps that
  // true instead of introducing a real conflict.
  struct ConstLiteral
      : rule<create<ast::IntLit>(
                 assign<&ast::IntLit::text>(ref<SIGNED_INT>))  //
             | create<ast::FloatLit>(
                   assign<&ast::FloatLit::text>(ref<SIGNED_FLOAT>))  //
             | create<ast::StringLit>(
                   assign<&ast::StringLit::value>(ref<STRING>))  //
             | create<ast::BoolLit>(
                   assign<&ast::BoolLit::value>(ref<BOOLK>))  //
             | create<ast::NullLit>(kw<"null">)> {};

  struct IdentExpr
      : rule<option(flag<&ast::Ident::rootScoped>(lit<"::">)) +
             assign<&ast::Ident::name>(ref<IDENT>)> {};

  // ---- compound expressions ----

  struct ArrayExprRule
      : rule<lit<"["> +
             many(append<&ast::ArrayExpr::items>(ref<AssignExprRule>), lit<",">) +
             option(lit<",">) + many(ref<NL>) + lit<"]">> {};

  struct BracketKeyRule : rule<lit<"["> + ref<Expression> + many(ref<NL>) + lit<"]">> {
  };

  struct KeyValueRule
      : rule<assign<&ast::KeyValue::key>(ref<IDENT> | ref<BracketKeyRule>) +
             lit<"="> +
             assign<&ast::KeyValue::value>(ref<AssignExprRule>)> {};

  struct TableExprRule
      : rule<lit<"{"> + many(lit<","> | ref<NL>) +
             many(append<&ast::TableExpr::items>(ref<KeyValueRule> |
                                                 ref<FunctionDeclRule>) +
                  many(lit<","> | ref<NL>)) +
             lit<"}">> {};

  // A plain name or a defaulted one, merged into one rule: an
  // id-then-optional-`=` prefix is identical for both, so telling them
  // apart needs lookahead past the `=` that a shared prefix does not give
  // an LR table; this over-accepts a default before a plain parameter,
  // which no example reaches.
  struct ParamRule : rule<many(ref<NL>) + assign<&ast::Param::name>(ref<IDENT>) +
                          option(lit<"="> + assign<&ast::Param::defaultValue>(
                                                ref<AssignExprRule>))> {};

  struct FunctionExprRule
      : rule<kw<"function"> + lit<"("> +
             option(some(append<&ast::FunctionExpr::params>(ref<ParamRule>),
                         lit<",">) +
                        option(lit<","> +
                               flag<&ast::FunctionExpr::variadic>(lit<"...">))  //
                    | flag<&ast::FunctionExpr::variadic>(lit<"...">)) +
             many(ref<NL>) + lit<")"> + many(ref<NL>) +
             assign<&ast::FunctionExpr::body>(ref<StatementOrEmpty>)> {};

  struct LambdaExprRule
      : rule<lit<"@"> + lit<"("> +
             option(some(append<&ast::LambdaExpr::params>(ref<ParamRule>),
                         lit<",">) +
                        option(lit<","> +
                               flag<&ast::LambdaExpr::variadic>(lit<"...">))  //
                    | flag<&ast::LambdaExpr::variadic>(lit<"...">)) +
             many(ref<NL>) + lit<")"> +
             assign<&ast::LambdaExpr::body>(ref<AssignExprRule>)> {};

  struct ClassExprRule
      : rule<kw<"class"> +
             option(kw<"extends"> +
                    append<&ast::ClassExpr::base>(ref<IDENT>) +
                    many(lit<"."> + append<&ast::ClassExpr::base>(ref<IDENT>))) +
             lit<"{"> + many(ref<NL>) +
             many(append<&ast::ClassExpr::members>(ref<ClassMemberRule>) +
                  many(lit<";"> | ref<NL>)) +
             lit<"}">> {};

  struct PrimaryExprRule
      : rule<ref<LiteralExpr>                                           //
             | (lit<"("> + ref<Expression> + many(ref<NL>) + lit<")">)  //
             | ref<ClassExprRule>                                       //
             | ref<TableExprRule>                                       //
             | ref<ArrayExprRule>                                       //
             | ref<FunctionExprRule>                                    //
             | ref<LambdaExprRule>                                      //
             | ref<IdentExpr>> {};

  struct PostfixExprRule
      : rule<ref<PrimaryExprRule> +
             many(nest<ast::MemberExpr, &ast::MemberExpr::object>(
                      lit<"."> + assign<&ast::MemberExpr::field>(ref<IDENT> |
                                                                 kw<"constructor">))  //
                  | nest<ast::CallExpr, &ast::CallExpr::callee>(
                        lit<"("> +
                        many(append<&ast::CallExpr::args>(ref<AssignExprRule>),
                             lit<",">) +
                        many(ref<NL>) + lit<")">)  //
                  | nest<ast::IndexExpr, &ast::IndexExpr::object>(
                        lit<"["> + assign<&ast::IndexExpr::index>(ref<AssignExprRule>) +
                        many(ref<NL>) + lit<"]">)  //
                  | nest<ast::PostfixOpExpr, &ast::PostfixOpExpr::operand>(
                        assign<&ast::PostfixOpExpr::op>(lit<"++"> | lit<"--">)))> {};

  struct UnaryExprRule;
  struct UnaryExprRule
      : rule<many(ref<NL>) +
             (create<ast::PrefixExpr>(
                  assign<&ast::PrefixExpr::op>(UnaryOp | UnaryAction) +
                  assign<&ast::PrefixExpr::operand>(ref<UnaryExprRule>))  //
              | create<ast::PrefixExpr>(
                    assign<&ast::PrefixExpr::op>(lit<"++"> | lit<"--">) +
                    assign<&ast::PrefixExpr::operand>(ref<PostfixExprRule>))  //
              | ref<PostfixExprRule>)> {};

  struct Ladder
      : infix<UnaryExprRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(MulOp) >>      //
               left(AddOp) >>      //
               left(ShiftOp) >>    //
               left(RelOp) >>      //
               left(EqOp) >>       //
               left(lit<"&">) >>   //
               left(lit<"^">) >>   //
               left(lit<"|">) >>   //
               left(AndInOp) >>    //
               left(lit<"||">))> {};

  struct TernaryExprRule;
  struct TernaryExprRule
      : rule<create<ast::CondExpr>(
                 assign<&ast::CondExpr::cond>(ref<Ladder>) + lit<"?"> +
                 assign<&ast::CondExpr::thenValue>(ref<Expression>) +
                 lit<":"> +
                 assign<&ast::CondExpr::elseValue>(ref<TernaryExprRule>))  //
             | ref<Ladder>> {};

  struct AssignExprRule
      : rule<create<ast::AssignExpr>(
                 assign<&ast::AssignExpr::target>(ref<TernaryExprRule>) +
                 assign<&ast::AssignExpr::op>(AssignOp) +
                 assign<&ast::AssignExpr::value>(ref<AssignExprRule>))  //
             | ref<TernaryExprRule>> {};

  struct Expression
      : infix<AssignExprRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(lit<",">)> {};

  // ---- statements ----

  struct StatementOrEmpty
      : rule<ref<Statement> | create<ast::EmptyStmt>(lit<";">)> {};

  struct BlockStmtRule
      : rule<lit<"{"> + LeadingSeps +
             many(append<&ast::BlockStmt::body>(ref<Statement>) +
                  StatementSep) +
             lit<"}">> {};

  struct ExprStmtRule
      : rule<create<ast::ExprStmt>(
                 assign<&ast::ExprStmt::value>(ref<Expression>))> {};

  struct ConstDeclRule
      : rule<kw<"const"> + assign<&ast::ConstDecl::name>(ref<IDENT>) +
             lit<"="> +
             assign<&ast::ConstDecl::value>(ref<ConstLiteral>)> {};

  struct EnumItemRule
      : rule<assign<&ast::EnumItem::name>(ref<IDENT>) +
             option(lit<"="> +
                    assign<&ast::EnumItem::value>(ref<ConstLiteral>))> {};

  struct EnumDeclRule
      : rule<kw<"enum"> + assign<&ast::EnumDecl::name>(ref<IDENT>) +
             lit<"{"> +
             option(some(append<&ast::EnumDecl::items>(ref<EnumItemRule>),
                         lit<",">) +
                    option(lit<",">)) +
             lit<"}">> {};

  struct VarItemRule
      : rule<assign<&ast::VarItem::name>(ref<IDENT>) +
             option(lit<"="> +
                    assign<&ast::VarItem::init>(ref<AssignExprRule>))> {};

  struct VarDeclarationListRule
      : rule<create<ast::LocalVarDecl> +
             some(append<&ast::LocalVarDecl::vars>(ref<VarItemRule>),
                  lit<",">)> {};

  struct LocalDeclaration
      : rule<kw<"local"> +
             (ref<VarDeclarationListRule>  //
              | create<ast::LocalFunctionDecl>(
                    assign<&ast::LocalFunctionDecl::func>(
                        ref<FunctionDeclRule>)))> {};

  struct FunctionDeclRule
      : rule<kw<"function"> +
             append<&ast::FunctionDecl::path>(ref<IDENT>) +
             many(lit<"::"> + append<&ast::FunctionDecl::path>(
                                   ref<IDENT> | kw<"constructor">)) +
             lit<"("> +
             option((some(append<&ast::FunctionDecl::params>(ref<ParamRule>),
                          lit<",">) +
                     option(lit<","> + flag<&ast::FunctionDecl::variadic>(
                                           lit<"...">)))  //
                    | flag<&ast::FunctionDecl::variadic>(lit<"...">)) +
             lit<")"> + many(ref<NL>) +
             assign<&ast::FunctionDecl::body>(ref<StatementOrEmpty>)> {};

  struct ConstructorDeclRule
      : rule<kw<"constructor"> + lit<"("> +
             option(
                 (some(append<&ast::ConstructorDecl::params>(ref<ParamRule>),
                       lit<",">) +
                  option(lit<","> + flag<&ast::ConstructorDecl::variadic>(
                                        lit<"...">)))  //
                 | flag<&ast::ConstructorDecl::variadic>(lit<"...">)) +
             lit<")"> + many(ref<NL>) +
             assign<&ast::ConstructorDecl::body>(ref<StatementOrEmpty>)> {};

  struct ClassMemberRule
      : rule<option(lit<"</"> + many(lit<","> | ref<NL>) +
                    many(append<&ast::ClassMember::attrs>(ref<KeyValueRule>) +
                         many(lit<","> | ref<NL>)) +
                    lit<"/>"> + many(ref<NL>)) +
             option(flag<&ast::ClassMember::isStatic>(kw<"static">)) +
             assign<&ast::ClassMember::member>(ref<ConstructorDeclRule>  //
                                               | ref<KeyValueRule>       //
                                               | ref<FunctionDeclRule>)> {};

  struct ClassDeclRule
      : rule<kw<"class"> + append<&ast::ClassDecl::name>(ref<IDENT>) +
             many(lit<"."> + append<&ast::ClassDecl::name>(ref<IDENT>)) +
             option(kw<"extends"> +
                    append<&ast::ClassDecl::base>(ref<IDENT>) +
                    many(lit<"."> + append<&ast::ClassDecl::base>(ref<IDENT>))) +
             lit<"{"> + many(ref<NL>) +
             many(append<&ast::ClassDecl::members>(ref<ClassMemberRule>) +
                  many(lit<";"> | ref<NL>)) +
             lit<"}">> {};

  struct ForStmtRule
      : rule<kw<"for"> + lit<"("> +
             option(assign<&ast::ForStmt::init>(ref<LocalDeclaration> |
                                                ref<ExprStmtRule>)) +
             lit<";"> +
             option(assign<&ast::ForStmt::cond>(ref<Expression>)) + lit<";"> +
             option(assign<&ast::ForStmt::step>(ref<Expression>)) +
             lit<")"> + many(ref<NL>) +
             assign<&ast::ForStmt::body>(ref<StatementOrEmpty>)> {};

  struct ForeachStmtRule
      : rule<kw<"foreach"> + lit<"("> +
             option(assign<&ast::ForeachStmt::indexName>(ref<IDENT>) +
                    lit<",">) +
             assign<&ast::ForeachStmt::valueName>(ref<IDENT>) + kw<"in"> +
             assign<&ast::ForeachStmt::collection>(ref<Expression>) +
             lit<")"> + many(ref<NL>) +
             assign<&ast::ForeachStmt::body>(ref<StatementOrEmpty>)> {};

  struct WhileStmtRule
      : rule<kw<"while"> + lit<"("> +
             assign<&ast::WhileStmt::cond>(ref<Expression>) + lit<")"> +
             many(ref<NL>) +
             assign<&ast::WhileStmt::body>(ref<StatementOrEmpty>)> {};

  struct DoWhileStmtRule
      : rule<kw<"do"> + many(ref<NL>) +
             assign<&ast::DoWhileStmt::body>(ref<StatementOrEmpty>) +
             kw<"while"> + lit<"("> +
             assign<&ast::DoWhileStmt::cond>(ref<Expression>) + lit<")">> {};

  struct IfStmtRule
      : rule<kw<"if"> + lit<"("> +
             assign<&ast::IfStmt::cond>(ref<Expression>) + lit<")"> +
             many(ref<NL>) +
             assign<&ast::IfStmt::thenBranch>(ref<StatementOrEmpty>) +
             option(commit(option(lit<";">) + many(ref<NL>) + kw<"else">, many(ref<NL>) + assign<&ast::IfStmt::elseBranch>(
                          ref<StatementOrEmpty>)))> {};

  struct SwitchCaseRule
      : rule<kw<"case"> + assign<&ast::SwitchCase::value>(ref<Expression>) +
             lit<":"> + LeadingSeps +
             many(append<&ast::SwitchCase::body>(ref<Statement>) +
                  StatementSep)> {};

  struct DefaultCaseRule
      : rule<kw<"default"> + lit<":"> + LeadingSeps +
             many(append<&ast::DefaultCase::body>(ref<Statement>) +
                  StatementSep)> {};

  struct SwitchStmtRule
      : rule<kw<"switch"> + lit<"("> +
             assign<&ast::SwitchStmt::value>(ref<Expression>) + lit<")"> +
             many(ref<NL>) + lit<"{"> + many(ref<NL>) +
             many(append<&ast::SwitchStmt::cases>(ref<SwitchCaseRule>)) +
             option(assign<&ast::SwitchStmt::defaultCase>(
                 ref<DefaultCaseRule>)) +
             lit<"}">> {};

  struct TryStmtRule
      : rule<kw<"try"> + assign<&ast::TryStmt::body>(ref<StatementOrEmpty>) +
             kw<"catch"> + lit<"("> +
             assign<&ast::TryStmt::catchName>(ref<IDENT>) + lit<")"> +
             assign<&ast::TryStmt::catchBody>(ref<StatementOrEmpty>)> {};

  struct ReturnStmtRule
      : rule<kw<"return"> +
             option(assign<&ast::ReturnStmt::value>(ref<Expression>))> {};

  struct BreakStmtRule : rule<create<ast::BreakStmt>(kw<"break">)> {};
  struct ContinueStmtRule : rule<create<ast::ContinueStmt>(kw<"continue">)> {};

  struct YieldStmtRule
      : rule<kw<"yield"> +
             assign<&ast::YieldStmt::value>(ref<Expression>)> {};

  struct ThrowStmtRule
      : rule<kw<"throw"> +
             assign<&ast::ThrowStmt::value>(ref<Expression>)> {};

  struct Statement
      : rule<ref<BlockStmtRule>       //
             | ref<ConstDeclRule>     //
             | ref<EnumDeclRule>      //
             | ref<LocalDeclaration>  //
             | ref<FunctionDeclRule>  //
             | ref<ClassDeclRule>     //
             | ref<ForStmtRule>       //
             | ref<ForeachStmtRule>   //
             | ref<WhileStmtRule>     //
             | ref<DoWhileStmtRule>   //
             | ref<IfStmtRule>        //
             | ref<SwitchStmtRule>    //
             | ref<TryStmtRule>       //
             | ref<ReturnStmtRule>    //
             | ref<BreakStmtRule>     //
             | ref<ContinueStmtRule>  //
             | ref<YieldStmtRule>     //
             | ref<ThrowStmtRule>     //
             | ref<ExprStmtRule>> {};

  struct ChunkRule
      : rule<create<ast::Chunk> + LeadingSeps +
             many(append<&ast::Chunk::body>(ref<Statement>) +
                  StatementSep)> {};

  using entry = ChunkRule;
  using ignore = list<WS, SL_COMMENT, ML_COMMENT>;
};

}  // namespace squirrel
