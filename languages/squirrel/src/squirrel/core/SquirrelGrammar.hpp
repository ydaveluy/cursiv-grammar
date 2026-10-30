// The Squirrel language: its grammar.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "squirrel/core/ast.hpp"

namespace squirrel {

inline constexpr auto unquote_string = [](std::string_view t) {
  return t.front() == '@' ? t.substr(2, t.size() - 3) : t.substr(1, t.size() - 2);
};

struct Squirrel : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\f)">)> {};
  struct SL_COMMENT : terminal<(lit<"//"> | lit<"#">) + many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  // Significant: statements end at a NL the way they end at `;` (see
  // `StatementSep`), so it is never dropped as trivia.
  struct NL : terminal<some(cr<R"(\r\n)">)> {};

  struct IDENT : terminal<cr<"a-zA-Z_"> + many(w)> {};

  static constexpr auto Hex = cr<"0-9a-fA-F">;
  static constexpr auto Escape = lit<"\\"> +
      (cr<R"(tabnrvf0\\"')"> | (lit<"x"> + repeat<1, 2>(Hex)) |
       (lit<"u"> + repeat<1, 4>(Hex)) | (lit<"U"> + repeat<1, 8>(Hex)));
  static constexpr auto IntBody = (lit<"0"> + some(cr<"0-7">))  //
                                   | (lit<"0"> + cr<"xX"> + some(Hex))    //
                                   | (lit<"'"> + (Escape | cr<R"(^'\\\r\n)">) + lit<"'">) //
                                   | lit<"0">                              //
                                   | (cr<"1-9"> + many(d));
  struct INT : terminal<IntBody> {};
  // Only reachable from a `const`/`enum` value: see `ConstLiteral`.
  struct SIGNED_INT : terminal<option(lit<"-">) + IntBody> {};

  static constexpr auto FloatExp = cr<"eE"> + option(cr<"+-">) + some(d);
  static constexpr auto FloatBody =
      ((some(d) + lit<"."> + many(d)) + option(FloatExp))               //
      | (some(d) + FloatExp);
  struct FLOAT : terminal<FloatBody> {};
  struct SIGNED_FLOAT : terminal<option(lit<"-">) + FloatBody> {};

  struct STRING
      : terminal<as<unquote_string>(
            lit<"\""> + many(Escape | cr<R"(^"\\\r\n)">) + lit<"\"">)> {};
  struct VERBATIM_STRING
      : terminal<as<unquote_string>(
            lit<"@\""> + many(cr<R"(^")"> | lit<"\"\"">) + lit<"\"">)> {};

  struct BOOLK : terminal<as<bool>(kw<"true"> | kw<"false">)> {};

  static constexpr auto MulOp = lit<"*"> | lit<"/"> | lit<"%">;
  static constexpr auto AddOp = lit<"+"> | lit<"-">;
  static constexpr auto ShiftOp = lit<"<<"> | lit<">>>"> | lit<">>">;
  static constexpr auto RelOp =
      lit<"<="> | lit<">="> | lit<"<"> | lit<">"> | kw<"instanceof"> | kw<"in">;
  static constexpr auto EqOp = lit<"=="> | lit<"!="> | lit<"<=>">;
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
  struct AssignExpr;
  struct Statement;
  struct StatementOrEmpty;
  struct ClassMember;
  struct FunctionDecl;

  // ---- literals and names ----

  struct StringExpr
      : rule<create<ast::StringLit>(
            flag<&ast::StringLit::verbatim>(assign<&ast::StringLit::value>(ref<VERBATIM_STRING>)) |
            assign<&ast::StringLit::value>(ref<STRING>))> {};

  struct LiteralExpr
      : rule<create<ast::IntLit>(assign<&ast::IntLit::text>(ref<INT>))  //
             | create<ast::FloatLit>(
                   assign<&ast::FloatLit::text>(ref<FLOAT>))  //
             | ref<StringExpr>  //
             | create<ast::BoolLit>(
                   assign<&ast::BoolLit::value>(ref<BOOLK>))  //
             | create<ast::NullLit>(kw<"null">)> {};

  // Optional leading `-`: reachable only from a `const`/`enum` value. In an
  // ordinary expression a leading `-` is always claimed first by the unary
  // operator (`UnaryExpr`), so the two spellings never compete for the
  // same input; folding the sign into a wider terminal here keeps that
  // true instead of introducing a real conflict.
  struct ConstLiteral
      : rule<create<ast::IntLit>(
                 assign<&ast::IntLit::text>(ref<SIGNED_INT>))  //
             | create<ast::FloatLit>(
                   assign<&ast::FloatLit::text>(ref<SIGNED_FLOAT>))  //
             | ref<StringExpr>  //
             | create<ast::BoolLit>(
                   assign<&ast::BoolLit::value>(ref<BOOLK>))  //
             | create<ast::NullLit>(kw<"null">)> {};

  struct IdentExpr
      : rule<option(flag<&ast::Ident::rootScoped>(lit<"::">)) +
             assign<&ast::Ident::name>(ref<IDENT>)> {};

  // ---- compound expressions ----

  struct ArrayExpr
      : rule<lit<"["> +
             many(append<&ast::ArrayExpr::items>(ref<AssignExpr>) + option(lit<",">)) + many(ref<NL>) + lit<"]">> {};

  struct BracketKey : rule<lit<"["> + ref<Expression> + many(ref<NL>) + lit<"]">> {
  };

  struct KeyValue
      : rule<assign<&ast::KeyValue::key>(ref<IDENT> | ref<BracketKey>) +
             lit<"="> +
             assign<&ast::KeyValue::value>(ref<AssignExpr>)> {};

  struct JsonKeyValue
      : rule<create<ast::KeyValue>(assign<&ast::KeyValue::key>(ref<StringExpr>) +
             lit<":"> + assign<&ast::KeyValue::value>(ref<AssignExpr>))> {};

  struct TableExpr
      : rule<lit<"{"> + many(lit<","> | ref<NL>) +
             many(append<&ast::TableExpr::items>(ref<KeyValue> | ref<JsonKeyValue> |
                                                 ref<FunctionDecl>) +
                  many(lit<","> | ref<NL>)) +
             lit<"}">> {};

  // A plain name or a defaulted one, merged into one rule: an
  // id-then-optional-`=` prefix is identical for both, so telling them
  // apart needs lookahead past the `=` that a shared prefix does not give
  // an LR table; this over-accepts a default before a plain parameter,
  // which no example reaches.
  struct Param : rule<many(ref<NL>) + assign<&ast::Param::name>(ref<IDENT>) +
                          option(lit<"="> + assign<&ast::Param::defaultValue>(
                                                ref<AssignExpr>))> {};

  struct FunctionExpr
      : rule<kw<"function"> +
             option(lit<"["> + assign<&ast::FunctionExpr::environment>(ref<AssignExpr>) + many(ref<NL>) + lit<"]">) +
             lit<"("> +
             option((some(append<&ast::FunctionExpr::params>(ref<Param>),
                         lit<",">) +
                        option(lit<","> +
                               option(flag<&ast::FunctionExpr::variadic>(lit<"...">))))  //
                    | flag<&ast::FunctionExpr::variadic>(lit<"...">)) +
             many(ref<NL>) + lit<")"> + many(ref<NL>) +
             assign<&ast::FunctionExpr::body>(ref<StatementOrEmpty>)> {};

  struct LambdaExpr
      : rule<lit<"@"> +
             option(lit<"["> + assign<&ast::LambdaExpr::environment>(ref<AssignExpr>) + many(ref<NL>) + lit<"]">) +
             lit<"("> +
             option((some(append<&ast::LambdaExpr::params>(ref<Param>),
                         lit<",">) +
                        option(lit<","> +
                               option(flag<&ast::LambdaExpr::variadic>(lit<"...">))))  //
                    | flag<&ast::LambdaExpr::variadic>(lit<"...">)) +
             many(ref<NL>) + lit<")"> +
             assign<&ast::LambdaExpr::body>(ref<AssignExpr>)> {};

  struct ClassExpr
      : rule<kw<"class"> +
             option(kw<"extends"> +
                    some(append<&ast::ClassExpr::base>(ref<IDENT>), lit<".">)) +
             lit<"{"> + many(ref<NL>) +
             many(append<&ast::ClassExpr::members>(ref<ClassMember>) +
                  many(lit<";"> | ref<NL>)) +
             lit<"}">> {};

  struct PrimaryExpr
      : rule<ref<LiteralExpr>                                           //
             | (lit<"("> + ref<Expression> + many(ref<NL>) + lit<")">)  //
             | ref<ClassExpr>                                       //
             | ref<TableExpr>                                       //
             | ref<ArrayExpr>                                       //
             | ref<FunctionExpr>                                    //
             | ref<LambdaExpr>                                      //
             | ref<IdentExpr>> {};

  struct PostfixExpr
      : rule<ref<PrimaryExpr> +
             many(nest<ast::MemberExpr, &ast::MemberExpr::object>(
                      lit<"."> + assign<&ast::MemberExpr::field>(ref<IDENT> |
                                                                 kw<"constructor">))  //
                  | nest<ast::CallExpr, &ast::CallExpr::callee>(
                        lit<"("> +
                        many(append<&ast::CallExpr::args>(ref<AssignExpr>),
                             lit<",">) +
                        many(ref<NL>) + lit<")">)  //
                  | nest<ast::IndexExpr, &ast::IndexExpr::object>(
                        lit<"["> + assign<&ast::IndexExpr::index>(ref<AssignExpr>) +
                        many(ref<NL>) + lit<"]">)  //
                  | nest<ast::PostfixOpExpr, &ast::PostfixOpExpr::operand>(
                        assign<&ast::PostfixOpExpr::op>(lit<"++"> | lit<"--">)))> {};

  struct UnaryExpr;
  struct UnaryExpr
      : rule<many(ref<NL>) +
             (create<ast::PrefixExpr>(
                  assign<&ast::PrefixExpr::op>(UnaryOp | UnaryAction) +
                  assign<&ast::PrefixExpr::operand>(ref<UnaryExpr>))  //
              | create<ast::PrefixExpr>(
                    assign<&ast::PrefixExpr::op>(lit<"++"> | lit<"--">) +
                    assign<&ast::PrefixExpr::operand>(ref<PostfixExpr>))  //
              | ref<PostfixExpr>)> {};

  struct Ladder
      : infix<UnaryExpr, ast::BinaryExpr,
              levels(left(MulOp),     //
                     left(AddOp),     //
                     left(ShiftOp),   //
                     left(RelOp),     //
                     left(EqOp),      //
                     left(lit<"&">),  //
                     left(lit<"^">),  //
                     left(lit<"|">),  //
                     right(lit<"&&">),   //
                     right(lit<"||">))> {};

  // Native Expression chooses one assignment or conditional suffix after
  // the logical ladder; both conditional arms recurse at the same level.
  struct AssignExpr
      : rule<create<ast::AssignExpr>(
                 assign<&ast::AssignExpr::target>(ref<Ladder>) +
                 assign<&ast::AssignExpr::op>(AssignOp) +
                 assign<&ast::AssignExpr::value>(ref<AssignExpr>)) |
             create<ast::CondExpr>(
                 assign<&ast::CondExpr::cond>(ref<Ladder>) + lit<"?"> +
                 assign<&ast::CondExpr::thenValue>(ref<AssignExpr>) + lit<":"> +
                 assign<&ast::CondExpr::elseValue>(ref<AssignExpr>)) |
             ref<Ladder>> {};

  struct Expression
      : infix<AssignExpr, ast::BinaryExpr, left(lit<",">)> {};

  // ---- statements ----

  struct StatementOrEmpty
      : rule<ref<Statement> | create<ast::EmptyStmt>(lit<";">)> {};

  struct BlockStmt
      : rule<lit<"{"> + LeadingSeps +
             many(append<&ast::BlockStmt::body>(ref<Statement>) +
                  StatementSep) +
             lit<"}">> {};

  struct ExprStmt
      : rule<create<ast::ExprStmt>(
                 assign<&ast::ExprStmt::value>(ref<Expression>))> {};

  struct ConstDecl
      : rule<kw<"const"> + assign<&ast::ConstDecl::name>(ref<IDENT>) +
             lit<"="> +
             assign<&ast::ConstDecl::value>(ref<ConstLiteral>)> {};

  struct EnumItem
      : rule<assign<&ast::EnumItem::name>(ref<IDENT>) +
             option(lit<"="> +
                    assign<&ast::EnumItem::value>(ref<ConstLiteral>))> {};

  struct EnumDecl
      : rule<kw<"enum"> + assign<&ast::EnumDecl::name>(ref<IDENT>) +
             lit<"{"> +
             option(some(append<&ast::EnumDecl::items>(ref<EnumItem>),
                         lit<",">) +
                    option(lit<",">)) +
             lit<"}">> {};

  struct VarItem
      : rule<assign<&ast::VarItem::name>(ref<IDENT>) +
             option(lit<"="> +
                    assign<&ast::VarItem::init>(ref<AssignExpr>))> {};

  struct VarDeclarationList
      : rule<create<ast::LocalVarDecl> +
             some(append<&ast::LocalVarDecl::vars>(ref<VarItem>),
                  lit<",">)> {};

  struct LocalDeclaration
      : rule<kw<"local"> +
             (ref<VarDeclarationList>  //
              | create<ast::LocalFunctionDecl>(
                    assign<&ast::LocalFunctionDecl::func>(
                        ref<FunctionDecl>)))> {};

  struct FunctionDecl
      : rule<kw<"function"> +
             append<&ast::FunctionDecl::path>(ref<IDENT>) +
             many(lit<"::"> + append<&ast::FunctionDecl::path>(
                                   ref<IDENT> | kw<"constructor">)) +
             option(lit<"["> + assign<&ast::FunctionDecl::environment>(ref<AssignExpr>) + many(ref<NL>) + lit<"]">) +
             lit<"("> +
             option((some(append<&ast::FunctionDecl::params>(ref<Param>),
                          lit<",">) +
                     option(lit<","> + option(flag<&ast::FunctionDecl::variadic>(
                                           lit<"...">))))  //
                    | flag<&ast::FunctionDecl::variadic>(lit<"...">)) +
             many(ref<NL>) + lit<")"> + many(ref<NL>) +
             assign<&ast::FunctionDecl::body>(ref<StatementOrEmpty>)> {};

  struct ConstructorDecl
      : rule<kw<"constructor"> +
             option(lit<"["> + assign<&ast::ConstructorDecl::environment>(ref<AssignExpr>) + many(ref<NL>) + lit<"]">) +
             lit<"("> +
             option(
                 (some(append<&ast::ConstructorDecl::params>(ref<Param>),
                       lit<",">) +
                  option(lit<","> + option(flag<&ast::ConstructorDecl::variadic>(
                                        lit<"...">))))  //
                 | flag<&ast::ConstructorDecl::variadic>(lit<"...">)) +
             many(ref<NL>) + lit<")"> + many(ref<NL>) +
             assign<&ast::ConstructorDecl::body>(ref<StatementOrEmpty>)> {};

  struct ClassMember
      : rule<option(lit<"</"> + many(lit<","> | ref<NL>) +
                    many(append<&ast::ClassMember::attrs>(ref<KeyValue>) +
                         many(lit<","> | ref<NL>)) +
                    lit<"/>"> + many(ref<NL>)) +
             option(flag<&ast::ClassMember::isStatic>(kw<"static">)) +
             assign<&ast::ClassMember::member>(ref<ConstructorDecl>  //
                                               | ref<KeyValue>       //
                                               | ref<FunctionDecl>)> {};

  struct ClassDecl
      : rule<kw<"class"> + some(append<&ast::ClassDecl::name>(ref<IDENT>), lit<".">) +
             option(kw<"extends"> +
                    some(append<&ast::ClassDecl::base>(ref<IDENT>), lit<".">)) +
             lit<"{"> + many(ref<NL>) +
             many(append<&ast::ClassDecl::members>(ref<ClassMember>) +
                  many(lit<";"> | ref<NL>)) +
             lit<"}">> {};

  struct ForStmt
      : rule<kw<"for"> + lit<"("> +
             option(assign<&ast::ForStmt::init>(ref<LocalDeclaration> |
                                                ref<ExprStmt>)) +
             lit<";"> +
             option(assign<&ast::ForStmt::cond>(ref<Expression>)) + lit<";"> +
             option(assign<&ast::ForStmt::step>(ref<Expression>)) +
             lit<")"> + many(ref<NL>) +
             assign<&ast::ForStmt::body>(ref<StatementOrEmpty>)> {};

  struct ForeachStmt
      : rule<kw<"foreach"> + lit<"("> +
             option(assign<&ast::ForeachStmt::indexName>(ref<IDENT>) +
                    lit<",">) +
             assign<&ast::ForeachStmt::valueName>(ref<IDENT>) + kw<"in"> +
             assign<&ast::ForeachStmt::collection>(ref<Expression>) +
             lit<")"> + many(ref<NL>) +
             assign<&ast::ForeachStmt::body>(ref<StatementOrEmpty>)> {};

  struct WhileStmt
      : rule<kw<"while"> + lit<"("> +
             assign<&ast::WhileStmt::cond>(ref<Expression>) + lit<")"> +
             many(ref<NL>) +
             assign<&ast::WhileStmt::body>(ref<StatementOrEmpty>)> {};

  struct DoWhileStmt
      : rule<kw<"do"> + many(ref<NL>) +
             assign<&ast::DoWhileStmt::body>(ref<StatementOrEmpty>) +
             many(ref<NL>) + kw<"while"> + lit<"("> +
             assign<&ast::DoWhileStmt::cond>(ref<Expression>) + lit<")">> {};

  struct IfStmt
      : rule<kw<"if"> + lit<"("> +
             assign<&ast::IfStmt::cond>(ref<Expression>) + lit<")"> +
             many(ref<NL>) +
             assign<&ast::IfStmt::thenBranch>(ref<StatementOrEmpty>) +
             option(commit(option(lit<";">) + many(ref<NL>) + kw<"else">, many(ref<NL>) + assign<&ast::IfStmt::elseBranch>(
                          ref<StatementOrEmpty>)))> {};

  struct SwitchCase
      : rule<kw<"case"> + assign<&ast::SwitchCase::value>(ref<Expression>) +
             lit<":"> + LeadingSeps +
             many(append<&ast::SwitchCase::body>(ref<Statement>) +
                  StatementSep)> {};

  struct DefaultCase
      : rule<kw<"default"> + lit<":"> + LeadingSeps +
             many(append<&ast::DefaultCase::body>(ref<Statement>) +
                  StatementSep)> {};

  struct SwitchStmt
      : rule<kw<"switch"> + lit<"("> +
             assign<&ast::SwitchStmt::value>(ref<Expression>) + lit<")"> +
             many(ref<NL>) + lit<"{"> + many(ref<NL>) +
             many(append<&ast::SwitchStmt::cases>(ref<SwitchCase>)) +
             option(assign<&ast::SwitchStmt::defaultCase>(
                 ref<DefaultCase>)) +
             lit<"}">> {};

  struct TryStmt
      : rule<kw<"try"> + many(ref<NL>) + assign<&ast::TryStmt::body>(ref<StatementOrEmpty>) +
             many(ref<NL>) + kw<"catch"> + lit<"("> + many(ref<NL>) +
             assign<&ast::TryStmt::catchName>(ref<IDENT>) + many(ref<NL>) + lit<")"> +
             many(ref<NL>) + assign<&ast::TryStmt::catchBody>(ref<StatementOrEmpty>)> {};

  struct ReturnStmt
      : rule<kw<"return"> +
             option(!ref<NL> + assign<&ast::ReturnStmt::value>(ref<Expression>))> {};

  struct BreakStmt : rule<create<ast::BreakStmt>(kw<"break">)> {};
  struct ContinueStmt : rule<create<ast::ContinueStmt>(kw<"continue">)> {};

  struct YieldStmt
      : rule<kw<"yield"> +
             option(!ref<NL> + assign<&ast::YieldStmt::value>(ref<Expression>))> {};

  struct ThrowStmt
      : rule<kw<"throw"> +
             assign<&ast::ThrowStmt::value>(ref<Expression>)> {};

  struct Statement
      : rule<ref<BlockStmt>       //
             | ref<ConstDecl>     //
             | ref<EnumDecl>      //
             | ref<LocalDeclaration>  //
             | ref<FunctionDecl>  //
             | ref<ClassDecl>     //
             | ref<ForStmt>       //
             | ref<ForeachStmt>   //
             | ref<WhileStmt>     //
             | ref<DoWhileStmt>   //
             | ref<IfStmt>        //
             | ref<SwitchStmt>    //
             | ref<TryStmt>       //
             | ref<ReturnStmt>    //
             | ref<BreakStmt>     //
             | ref<ContinueStmt>  //
             | ref<YieldStmt>     //
             | ref<ThrowStmt>     //
             | ref<ExprStmt>> {};

  struct Chunk
      : rule<create<ast::Chunk> + LeadingSeps +
             many(append<&ast::Chunk::body>(ref<Statement>) +
                  StatementSep)> {};

  using entry = Chunk;
  using ignored = list<WS, SL_COMMENT, ML_COMMENT>;
};

}  // namespace squirrel
