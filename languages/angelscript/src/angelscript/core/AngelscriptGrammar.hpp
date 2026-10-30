// The angelscript language: its grammar.
#pragma once

#include "angelscript/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace angelscript {

struct Angelscript : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};

  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  struct ID : terminal<cr<"a-zA-Z_"> + many(w)> {};

  static constexpr auto hex_digit = cr<"0-9a-fA-F">;
  static constexpr auto escape = lit<"\\"> +
      (cr<R"(0\\'"nrt)"> | (lit<"x"> + repeat<1, 4>(hex_digit)) |
       (lit<"u"> + repeat<4>(hex_digit)) |
       (lit<"U"> + repeat<8>(hex_digit)));
  struct STRING
      : terminal<through(lit<"\"\"\"">, lit<"\"\"\"">) |
                 (lit<"\""> + many(escape | cr<R"(^"\\\r\n)">) + lit<"\"">) |
                 (lit<"'"> + many(escape | cr<R"(^'\\\r\n)">) + lit<"'">)> {};

  struct PRIMTYPE
      : terminal<kw<"void"> | kw<"int"> | kw<"int8"> | kw<"int16"> |
                 kw<"int32"> | kw<"int64"> | kw<"uint"> | kw<"uint8"> |
                 kw<"uint16"> | kw<"uint32"> | kw<"uint64"> | kw<"float"> |
                 kw<"double"> | kw<"bool">> {};

  static constexpr auto NumberBody =
      ((some(d) + option(lit<"."> + many(d))) | (lit<"."> + some(d))) +
      option(cr<"eE"> + option(cr<"+-">) + some(d)) + option(cr<"fFdD">);

  static constexpr auto BitsBody =
      (lit<"0"> + cr<"bB"> + some(cr<"01">)) |
      (lit<"0"> + cr<"oO"> + some(cr<"0-7">)) |
      (lit<"0"> + cr<"dD"> + some(d)) |
      (lit<"0"> + cr<"xX"> + some(hex_digit));

  struct LITERAL
      : terminal<NumberBody                                  //
                 | BitsBody                                   //
                 | kw<"true"> | kw<"false"> | kw<"null">> {};

  // Adjacent strings form one literal, including across comments.
  struct Literal : rule<some(ref<STRING>) | ref<LITERAL>> {};

  static constexpr auto Lt = lit<"<">;
  static constexpr auto Gt = lit<">">;

  // `<<`, `>>`, `>>>` are never lexer literals: a lone `<`/`>` is always its
  // own token, so `Foo<Bar<int>>` closes two generic levels with two `>`
  // tokens instead of one maximal-munched `>>`. A shift operator is this
  // many adjacent single-char tokens read at the parser level instead.
  struct ShiftOp : rule<(Gt + gap::none + Gt + gap::none + Gt) |
                        (Lt + gap::none + Lt) | (Gt + gap::none + Gt)> {};

  static constexpr auto FuncModifier = kw<"shared"> | kw<"external">;
  static constexpr auto ClassModifier =
      kw<"shared"> | kw<"abstract"> | kw<"final"> | kw<"external">;
  static constexpr auto InterfaceModifier = kw<"external"> | kw<"shared">;
  static constexpr auto Visibility = kw<"private"> | kw<"protected">;
  static constexpr auto FuncAttr =
      kw<"override"> | kw<"final"> | kw<"explicit"> | kw<"property">;

  struct TypeRef;
  struct Expr;
  struct Assign;
  struct Statement;
  struct BlockStmt;
  struct VarDecl;
  struct ClassDecl;
  struct ScriptItem;

  // -- Types ----------------------------------------------------------

  struct GenericArgs
      : rule<create<ast::GenericArgs>(
            Lt +
            some(append<&ast::GenericArgs::args>(ref<TypeRef>), lit<",">) +
            Gt)> {};

  struct ScopeSegment
      : rule<create<ast::ScopeSegment>(
            assign<&ast::ScopeSegment::name>(ref<ID>) +
            many(append<&ast::ScopeSegment::generics>(ref<GenericArgs>)) +
            lit<"::">)> {};

  static constexpr auto DatatypeText =
      ref<ID> | ref<PRIMTYPE> | lit<"?"> | kw<"auto">;

  struct ArraySuffix
      : rule<create<ast::ArraySuffix>(lit<"["> + lit<"]">)> {};

  struct HandleSuffix
      : rule<create<ast::HandleSuffix>(
            lit<"@"> +
            option(flag<&ast::HandleSuffix::isConst>(kw<"const">)))> {};

  struct TypeSuffix
      : rule<ref<ArraySuffix> | ref<HandleSuffix>> {};

  struct TypeRef
      : rule<option(flag<&ast::TypeRef::isConst>(kw<"const">)) +
             option(flag<&ast::TypeRef::rooted>(lit<"::">)) +
             many(append<&ast::TypeRef::scope>(ref<ScopeSegment>)) +
             assign<&ast::TypeRef::name>(DatatypeText) +
             many(append<&ast::TypeRef::generics>(ref<GenericArgs>)) +
             many(append<&ast::TypeRef::suffixes>(ref<TypeSuffix>))> {};

  struct TypeMod
      : rule<create<ast::TypeMod>(option(
            flag<&ast::TypeMod::byRef>(lit<"&">) +
            option(assign<&ast::TypeMod::dir>(kw<"in"> | kw<"out"> |
                                               kw<"inout">))))> {};

  // -- Expressions ------------------------------------------------------

  struct Arg
      : rule<create<ast::Arg>(
            option(assign<&ast::Arg::name>(ref<ID>) + lit<":">) +
            assign<&ast::Arg::value>(ref<Assign>))> {};

  struct ArgList
      : rule<create<ast::ArgList>(
            lit<"("> +
            option(append<&ast::ArgList::items>(ref<Arg>) +
                   many(lit<","> +
                        append<&ast::ArgList::items>(ref<Arg>))) +
            lit<")">)> {};

  struct ConstructCall
      : rule<create<ast::ConstructExpr>(
            assign<&ast::ConstructExpr::type>(ref<TypeRef>) +
            assign<&ast::ConstructExpr::args>(ref<ArgList>))> {};

  struct FuncCall
      : rule<create<ast::CallExpr>(
            option(flag<&ast::CallExpr::rooted>(lit<"::">)) +
            many(append<&ast::CallExpr::scope>(ref<ScopeSegment>)) +
            assign<&ast::CallExpr::name>(ref<ID>) +
            assign<&ast::CallExpr::args>(ref<ArgList>))> {};

  struct VarAccess
      : rule<create<ast::VarAccessExpr>(
            option(flag<&ast::VarAccessExpr::rooted>(lit<"::">)) +
            many(append<&ast::VarAccessExpr::scope>(ref<ScopeSegment>)) +
            assign<&ast::VarAccessExpr::name>(ref<ID>))> {};

  struct Cast
      : rule<create<ast::CastExpr>(
            kw<"cast"> + Lt + assign<&ast::CastExpr::type>(ref<TypeRef>) +
            Gt + lit<"("> + assign<&ast::CastExpr::operand>(ref<Assign>) +
            lit<")">)> {};

  struct LambdaParam
      : rule<create<ast::LambdaParam>(
            option(assign<&ast::LambdaParam::type>(ref<TypeRef>) +
                   assign<&ast::LambdaParam::mod>(ref<TypeMod>)) +
            assign<&ast::LambdaParam::name>(ref<ID>))> {};

  struct Lambda
      : rule<create<ast::LambdaExpr>(
            kw<"function"> + lit<"("> +
            option(append<&ast::LambdaExpr::params>(ref<LambdaParam>) +
                   many(lit<","> + append<&ast::LambdaExpr::params>(
                                       ref<LambdaParam>))) +
            lit<")"> +
            assign<&ast::LambdaExpr::body>(ref<BlockStmt>))> {};

  // `VOID` alone is ported directly; as a *parameter list*, `(void)` already
  // reads as a one-parameter list with type `void` and no name (see
  // ParamList), so no separate alternative for it repeats there.
  struct PrimaryExpr
      : rule<create<ast::VoidExpr>(kw<"void">)  //
             | ref<ConstructCall>           //
             | ref<FuncCall>                //
             | ref<VarAccess>               //
             | ref<Cast>                    //
             | create<ast::LiteralExpr>(
                   assign<&ast::LiteralExpr::text>(ref<Literal>))  //
             | (lit<"("> + ref<Assign> + lit<")">)             //
             | ref<Lambda>> {};

  struct IndexArgFirst
      : rule<create<ast::IndexArg>(
            option(assign<&ast::IndexArg::name>(ref<ID>) + lit<":">) +
            assign<&ast::IndexArg::value>(ref<Assign>))> {};

  struct IndexArgRest
      : rule<create<ast::IndexArg>(
            option(assign<&ast::IndexArg::name>(ref<ID>) + lit<":">) +
            assign<&ast::IndexArg::value>(ref<Assign>))> {};

  struct PostfixChain
      : rule<ref<PrimaryExpr> +
             many(
                 nest<ast::MemberCallExpr, &ast::MemberCallExpr::object>(
                     lit<"."> +
                     option(flag<&ast::MemberCallExpr::rooted>(
                         lit<"::">)) +
                     many(append<&ast::MemberCallExpr::scope>(
                         ref<ScopeSegment>)) +
                     assign<&ast::MemberCallExpr::name>(ref<ID>) +
                     assign<&ast::MemberCallExpr::args>(ref<ArgList>))  //
                 | nest<ast::MemberExpr, &ast::MemberExpr::object>(
                       lit<"."> + assign<&ast::MemberExpr::name>(ref<ID>))  //
                 | nest<ast::IndexExpr, &ast::IndexExpr::object>(
                       lit<"["> +
                       append<&ast::IndexExpr::items>(ref<IndexArgFirst>) +
                       many(lit<","> + append<&ast::IndexExpr::items>(
                                           ref<IndexArgRest>)) +
                       lit<"]">)  //
                 | nest<ast::CallPostfixExpr, &ast::CallPostfixExpr::callee>(
                       assign<&ast::CallPostfixExpr::args>(
                           ref<ArgList>))  //
                 | nest<ast::PostIncDecExpr, &ast::PostIncDecExpr::operand>(
                       assign<&ast::PostIncDecExpr::op>(lit<"++"> |
                                                         lit<"--">)))> {};

  struct PrefixChain
      : rule<create<ast::PrefixExpr>(
                 assign<&ast::PrefixExpr::op>(
                     lit<"-"> | lit<"+"> | lit<"!"> | kw<"not"> | lit<"++"> |
                     lit<"--"> | lit<"~"> | lit<"@">) +
                 assign<&ast::PrefixExpr::operand>(ref<PrefixChain>))  //
             | ref<PostfixChain>> {};

  struct InitList;

  struct InitListItem
      : rule<create<ast::InitListItem>(
                 assign<&ast::InitListItem::value>(ref<Assign>))  //
             | create<ast::InitListItem>(
                   assign<&ast::InitListItem::nested>(ref<InitList>))> {
  };

  struct InitList
      : rule<create<ast::InitList>(
            lit<"{"> +
            option(append<&ast::InitList::items>(ref<InitListItem>)) +
            many(lit<","> + option(append<&ast::InitList::items>(
                                        ref<InitListItem>))) +
            lit<"}">)> {};

  struct ExprTerm
      : rule<ref<PrefixChain>  //
             | create<ast::InitListExpr>(
                   option(assign<&ast::InitListExpr::type>(ref<TypeRef>) +
                          lit<"=">) +
                   assign<&ast::InitListExpr::list>(ref<InitList>))> {};

  static constexpr auto MulOp = lit<"*"> | lit<"/"> | lit<"%">;
  static constexpr auto AddOp = lit<"+"> | lit<"-">;

  struct ArithLevel
      : infix<ExprTerm, ast::BinExpr,
              levels(left(lit<"**">), left(MulOp), left(AddOp))> {};

  // ShiftOp keeps generic closers as individual tokens while requiring
  // adjacent bytes for the operator itself.
  struct ShiftLevel
      : infix<ArithLevel, ast::BinExpr, left(ref<ShiftOp>)> {};

  struct Expr
      : infix<ShiftLevel, ast::BinExpr,
              levels(left(lit<"&">), left(lit<"^">), left(lit<"|">),
                     left(lit<"<="> | lit<">="> | Lt | Gt),
                     left(lit<"=="> | lit<"!="> | kw<"is"> | kw<"!is"> |
                          kw<"xor"> | lit<"^^">),
                     left(lit<"&&"> | kw<"and">),
                     left(lit<"||"> | kw<"or">))> {};

  struct Condition
      : rule<create<ast::CondExpr>(
                 assign<&ast::CondExpr::cond>(ref<Expr>) + lit<"?"> +
                 assign<&ast::CondExpr::thenValue>(ref<Assign>) +
                 lit<":"> +
                 assign<&ast::CondExpr::elseValue>(ref<Assign>))  //
             | ref<Expr>> {};

  static constexpr auto AssignOp =
      lit<"+="> | lit<"-="> | lit<"*="> | lit<"/="> | lit<"|="> |
      lit<"&="> | lit<"^="> | lit<"%="> | lit<"**="> | lit<"<<="> |
      lit<">>="> | lit<">>>="> | lit<"=", !cr<"=">>;

  struct Assign
      : rule<create<ast::AssignExpr>(
                 assign<&ast::AssignExpr::target>(ref<Condition>) +
                 assign<&ast::AssignExpr::op>(AssignOp) +
                 assign<&ast::AssignExpr::value>(ref<Assign>))  //
             | ref<Condition>> {};

  // -- Statements -------------------------------------------------------

  struct ExprStmt
      : rule<create<ast::ExprStmt>(
            option(assign<&ast::ExprStmt::value>(ref<Assign>)) +
            lit<";">)> {};

  struct BlockStmt
      : rule<create<ast::BlockStmt>(
            lit<"{"> +
            many(append<&ast::BlockStmt::items>(ref<VarDecl>) |
                 append<&ast::BlockStmt::items>(ref<Statement>)) +
            lit<"}">)> {};

  struct IfStmt
      : rule<kw<"if"> + lit<"("> +
             assign<&ast::IfStmt::cond>(ref<Assign>) + lit<")"> +
             assign<&ast::IfStmt::thenBranch>(ref<Statement>) +
             option(commit(kw<"else">, assign<&ast::IfStmt::elseBranch>(ref<Statement>)))> {
  };

  struct ForStmt
      : rule<kw<"for"> + lit<"("> +
             assign<&ast::ForStmt::init>(ref<VarDecl> |
                                          ref<ExprStmt>) +
             assign<&ast::ForStmt::cond>(ref<ExprStmt>) +
             option(append<&ast::ForStmt::steps>(ref<Assign>) +
                    many(lit<","> +
                         append<&ast::ForStmt::steps>(ref<Assign>))) +
             lit<")"> + assign<&ast::ForStmt::body>(ref<Statement>)> {};

  struct WhileStmt
      : rule<kw<"while"> + lit<"("> +
             assign<&ast::WhileStmt::cond>(ref<Assign>) + lit<")"> +
             assign<&ast::WhileStmt::body>(ref<Statement>)> {};

  struct DoWhileStmt
      : rule<kw<"do"> + assign<&ast::DoWhileStmt::body>(ref<Statement>) +
             kw<"while"> + lit<"("> +
             assign<&ast::DoWhileStmt::cond>(ref<Assign>) + lit<")"> +
             lit<";">> {};

  struct BreakStmt
      : rule<create<ast::BreakStmt>(kw<"break"> + lit<";">)> {};

  struct ContinueStmt
      : rule<create<ast::ContinueStmt>(kw<"continue"> + lit<";">)> {};

  struct ReturnStmt
      : rule<create<ast::ReturnStmt>(
            kw<"return"> +
            option(assign<&ast::ReturnStmt::value>(ref<Assign>)) +
            lit<";">)> {};

  struct TryStmt
      : rule<kw<"try"> + assign<&ast::TryStmt::tryBlock>(ref<BlockStmt>) +
             kw<"catch"> +
             assign<&ast::TryStmt::catchBlock>(ref<BlockStmt>)> {};

  struct CaseClause
      : rule<create<ast::CaseClause>(
            ((kw<"case"> + assign<&ast::CaseClause::value>(ref<Expr>))  //
             | flag<&ast::CaseClause::isDefault>(kw<"default">)) +
            lit<":"> +
            many(append<&ast::CaseClause::body>(ref<Statement>)))> {};

  struct SwitchStmt
      : rule<kw<"switch"> + lit<"("> +
             assign<&ast::SwitchStmt::value>(ref<Assign>) + lit<")"> +
             lit<"{"> +
             many(append<&ast::SwitchStmt::cases>(ref<CaseClause>)) +
             lit<"}">> {};

  struct Statement
      : rule<ref<IfStmt>          //
             | ref<ForStmt>       //
             | ref<WhileStmt>     //
             | ref<ReturnStmt>    //
             | ref<BlockStmt>     //
             | ref<BreakStmt>     //
             | ref<ContinueStmt>  //
             | ref<DoWhileStmt>   //
             | ref<SwitchStmt>    //
             | ref<ExprStmt>      //
             | ref<TryStmt>> {};

  // -- Declarations -------------------------------------------------------

  struct Param
      : rule<create<ast::Param>(
            assign<&ast::Param::type>(ref<TypeRef>) +
            assign<&ast::Param::mod>(ref<TypeMod>) +
            option(assign<&ast::Param::name>(ref<ID>)) +
            option(lit<"="> +
                   assign<&ast::Param::defaultValue>(ref<Expr>)))> {};

  struct ParamList
      : rule<create<ast::ParamList>(
            lit<"("> +
            option(append<&ast::ParamList::params>(ref<Param>) +
                   many(lit<","> +
                        append<&ast::ParamList::params>(ref<Param>))) +
            lit<")">)> {};

  struct VarInit
      : rule<create<ast::VarInit>(
            assign<&ast::VarInit::name>(ref<ID>) +
            option((lit<"="> +
                    (assign<&ast::VarInit::initList>(ref<InitList>)  //
                     | assign<&ast::VarInit::initExpr>(ref<Expr>)))  //
                   | assign<&ast::VarInit::ctorArgs>(ref<ArgList>)))> {};

  struct VarDecl
      : rule<create<ast::VarDecl>(
            option(assign<&ast::VarDecl::visibility>(Visibility)) +
            assign<&ast::VarDecl::type>(ref<TypeRef>) +
            append<&ast::VarDecl::declarators>(ref<VarInit>) +
            many(lit<","> +
                 append<&ast::VarDecl::declarators>(ref<VarInit>)) +
            lit<";">)> {};

  struct ImportDecl
      : rule<create<ast::ImportDecl>(
            kw<"import"> +
            assign<&ast::ImportDecl::returnType>(ref<TypeRef>) +
            option(flag<&ast::ImportDecl::returnByRef>(lit<"&">)) +
            assign<&ast::ImportDecl::name>(ref<ID>) +
            assign<&ast::ImportDecl::params>(ref<ParamList>) +
            many(append<&ast::ImportDecl::attr>(FuncAttr)) + kw<"from"> +
            assign<&ast::ImportDecl::source>(ref<STRING>) + lit<";">)> {};

  struct EnumValue
      : rule<create<ast::EnumValue>(
            assign<&ast::EnumValue::name>(ref<ID>) +
            option(lit<"="> +
                   assign<&ast::EnumValue::value>(ref<Expr>)))> {};

  struct EnumDecl
      : rule<create<ast::EnumDecl>(
            many(append<&ast::EnumDecl::modifiers>(FuncModifier)) +
            kw<"enum"> + assign<&ast::EnumDecl::name>(ref<ID>) +
            (flag<&ast::EnumDecl::forwardDecl>(lit<";">)  //
             | (lit<"{"> +
                some(append<&ast::EnumDecl::values>(ref<EnumValue>),
                     lit<",">) +
                lit<"}">)))> {};

  struct TypedefDecl
      : rule<create<ast::TypedefDecl>(
            kw<"typedef"> +
            assign<&ast::TypedefDecl::primtype>(ref<PRIMTYPE>) +
            assign<&ast::TypedefDecl::name>(ref<ID>) + lit<";">)> {};

  struct FuncdefDecl
      : rule<create<ast::FuncdefDecl>(
            many(append<&ast::FuncdefDecl::modifiers>(FuncModifier)) +
            kw<"funcdef"> +
            assign<&ast::FuncdefDecl::returnType>(ref<TypeRef>) +
            option(flag<&ast::FuncdefDecl::returnByRef>(lit<"&">)) +
            assign<&ast::FuncdefDecl::name>(ref<ID>) +
            assign<&ast::FuncdefDecl::params>(ref<ParamList>) +
            lit<";">)> {};

  struct AccessorDecl
      : rule<create<ast::AccessorDecl>(
            (flag<&ast::AccessorDecl::isSetter>(kw<"set">)  //
             | kw<"get">) +
            option(flag<&ast::AccessorDecl::isConst>(kw<"const">)) +
            many(append<&ast::AccessorDecl::attr>(FuncAttr)) +
            (flag<&ast::AccessorDecl::declOnly>(lit<";">)  //
             | assign<&ast::AccessorDecl::body>(ref<BlockStmt>)))> {};

  struct VirtPropDecl
      : rule<create<ast::VirtPropDecl>(
            option(assign<&ast::VirtPropDecl::visibility>(Visibility)) +
            assign<&ast::VirtPropDecl::type>(ref<TypeRef>) +
            option(flag<&ast::VirtPropDecl::byRef>(lit<"&">)) +
            assign<&ast::VirtPropDecl::name>(ref<ID>) + lit<"{"> +
            many(append<&ast::VirtPropDecl::accessors>(
                ref<AccessorDecl>)) +
            lit<"}">)> {};

  struct FuncDecl
      : rule<create<ast::FuncDecl>(
            many(append<&ast::FuncDecl::modifiers>(FuncModifier)) +
            option(assign<&ast::FuncDecl::visibility>(Visibility)) +
            option(
                (assign<&ast::FuncDecl::returnType>(ref<TypeRef>) +
                 option(flag<&ast::FuncDecl::returnByRef>(lit<"&">)))  //
                | flag<&ast::FuncDecl::isDestructor>(lit<"~">)) +
            assign<&ast::FuncDecl::name>(ref<ID>) +
            assign<&ast::FuncDecl::params>(ref<ParamList>) +
            option(flag<&ast::FuncDecl::isConst>(kw<"const">)) +
            many(append<&ast::FuncDecl::attr>(FuncAttr)) +
            (flag<&ast::FuncDecl::declOnly>(lit<";">)  //
             | assign<&ast::FuncDecl::body>(ref<BlockStmt>)))> {};

  struct ClassMember
      : rule<ref<VirtPropDecl>  //
             | ref<FuncDecl>    //
             | ref<VarDecl>     //
             | ref<FuncdefDecl>> {};

  struct ClassDecl
      : rule<create<ast::ClassDecl>(
            many(append<&ast::ClassDecl::modifiers>(ClassModifier)) +
            kw<"class"> + assign<&ast::ClassDecl::name>(ref<ID>) +
            (flag<&ast::ClassDecl::forwardDecl>(lit<";">)  //
             | (option(lit<":"> +
                       append<&ast::ClassDecl::bases>(ref<ID>) +
                       many(lit<","> +
                            append<&ast::ClassDecl::bases>(ref<ID>))) +
                lit<"{"> +
                many(append<&ast::ClassDecl::members>(ref<ClassMember>)) +
                lit<"}">)))> {};

  struct MixinDecl
      : rule<create<ast::MixinDecl>(
            kw<"mixin"> +
            assign<&ast::MixinDecl::class_>(ref<ClassDecl>))> {};

  struct IntfMthdDecl
      : rule<create<ast::IntfMthdDecl>(
            assign<&ast::IntfMthdDecl::returnType>(ref<TypeRef>) +
            option(flag<&ast::IntfMthdDecl::returnByRef>(lit<"&">)) +
            assign<&ast::IntfMthdDecl::name>(ref<ID>) +
            assign<&ast::IntfMthdDecl::params>(ref<ParamList>) +
            option(flag<&ast::IntfMthdDecl::isConst>(kw<"const">)) +
            lit<";">)> {};

  struct InterfaceMember
      : rule<ref<VirtPropDecl> | ref<IntfMthdDecl>> {};

  struct InterfaceDecl
      : rule<create<ast::InterfaceDecl>(
            many(append<&ast::InterfaceDecl::modifiers>(InterfaceModifier)) +
            kw<"interface"> + assign<&ast::InterfaceDecl::name>(ref<ID>) +
            (flag<&ast::InterfaceDecl::forwardDecl>(lit<";">)  //
             | (option(lit<":"> +
                       append<&ast::InterfaceDecl::bases>(ref<ID>) +
                       many(lit<","> +
                            append<&ast::InterfaceDecl::bases>(ref<ID>))) +
                lit<"{"> +
                many(append<&ast::InterfaceDecl::members>(
                    ref<InterfaceMember>)) +
                lit<"}">)))> {};

  struct NamespaceName : rule<some(ref<ID>, lit<"::">)> {};

  struct NamespaceDecl
      : rule<create<ast::NamespaceDecl>(
            kw<"namespace"> + assign<&ast::NamespaceDecl::name>(ref<NamespaceName>) +
            lit<"{"> +
            many(append<&ast::NamespaceDecl::items>(ref<ScriptItem>) |
                 lit<";">) +
            lit<"}">)> {};

  struct ScriptItem
      : rule<ref<ImportDecl>       //
             | ref<EnumDecl>       //
             | ref<TypedefDecl>    //
             | ref<ClassDecl>      //
             | ref<MixinDecl>      //
             | ref<InterfaceDecl>  //
             | ref<FuncdefDecl>    //
             | ref<VirtPropDecl>   //
             | ref<VarDecl>        //
             | ref<FuncDecl>       //
             | ref<NamespaceDecl>> {};

  struct Script
      : rule<create<ast::Script>(
            many(append<&ast::Script::items>(ref<ScriptItem>) |
                 lit<";">))> {};

  using entry = Script;
  using ignored = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace angelscript
