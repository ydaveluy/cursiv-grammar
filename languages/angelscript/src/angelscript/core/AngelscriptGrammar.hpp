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

  struct STRING
      : terminal<(lit<"\""> + many(cr<R"(^")">) + lit<"\"">)  //
                 | (lit<"'"> + many(cr<"^'">) + lit<"'">)> {};

  struct PRIMTYPE
      : terminal<kw<"void"> | kw<"int"> | kw<"int8"> | kw<"int16"> |
                 kw<"int32"> | kw<"int64"> | kw<"uint"> | kw<"uint8"> |
                 kw<"uint16"> | kw<"uint32"> | kw<"uint64"> | kw<"float"> |
                 kw<"double"> | kw<"bool">> {};

  static constexpr auto NumberBody =
      ((some(d) + option(lit<"."> + many(d))) | (lit<"."> + some(d))) +
      option(cr<"eE"> + option(cr<"+-">) + some(d)) + option(cr<"fFdD">);

  static constexpr auto BitsBody =
      (lit<"0b"> | lit<"0o"> | lit<"0d"> | lit<"0x"> | lit<"0B"> |
       lit<"0O"> | lit<"0D"> | lit<"0X">) +
      some(cr<"0-9a-z">);

  struct LITERAL
      : terminal<NumberBody                                  //
                 | (lit<"\""> + many(cr<R"(^")">) + lit<"\"">)  //
                 | (lit<"'"> + many(cr<"^'">) + lit<"'">)     //
                 | BitsBody                                   //
                 | kw<"true"> | kw<"false"> | kw<"null">> {};

  static constexpr auto Lt = lit<"<">;
  static constexpr auto Gt = lit<">">;

  // `<<`, `>>`, `>>>` are never lexer literals: a lone `<`/`>` is always its
  // own token, so `Foo<Bar<int>>` closes two generic levels with two `>`
  // tokens instead of one maximal-munched `>>`. A shift operator is this
  // many adjacent single-char tokens read at the parser level instead.
  struct ShiftOpRule : rule<(Gt + Gt + Gt) | (Lt + Lt) | (Gt + Gt)> {};

  static constexpr auto FuncModifier = kw<"shared"> | kw<"external">;
  static constexpr auto ClassModifier =
      kw<"shared"> | kw<"abstract"> | kw<"final"> | kw<"external">;
  static constexpr auto InterfaceModifier = kw<"external"> | kw<"shared">;
  static constexpr auto Visibility = kw<"private"> | kw<"protected">;
  static constexpr auto FuncAttr =
      kw<"override"> | kw<"final"> | kw<"explicit"> | kw<"property">;

  struct TypeRefRule;
  struct ExprRule;
  struct AssignRule;
  struct StatementRule;
  struct BlockStmtRule;
  struct VarDeclRule;
  struct ClassDeclRule;
  struct ScriptItemRule;

  // -- Types ----------------------------------------------------------

  struct GenericArgsRule
      : rule<create<ast::GenericArgs>(
            Lt +
            some(append<&ast::GenericArgs::args>(ref<TypeRefRule>), lit<",">) +
            Gt)> {};

  struct ScopeSegmentRule
      : rule<create<ast::ScopeSegment>(
            assign<&ast::ScopeSegment::name>(ref<ID>) +
            many(append<&ast::ScopeSegment::generics>(ref<GenericArgsRule>)) +
            lit<"::">)> {};

  static constexpr auto DatatypeText =
      ref<ID> | ref<PRIMTYPE> | lit<"?"> | kw<"auto">;

  struct ArraySuffixRule
      : rule<create<ast::ArraySuffix>(lit<"["> + lit<"]">)> {};

  struct HandleSuffixRule
      : rule<create<ast::HandleSuffix>(
            lit<"@"> +
            option(flag<&ast::HandleSuffix::isConst>(kw<"const">)))> {};

  struct TypeSuffixRule
      : rule<ref<ArraySuffixRule> | ref<HandleSuffixRule>> {};

  struct TypeRefRule
      : rule<option(flag<&ast::TypeRef::isConst>(kw<"const">)) +
             option(flag<&ast::TypeRef::rooted>(lit<"::">)) +
             many(append<&ast::TypeRef::scope>(ref<ScopeSegmentRule>)) +
             assign<&ast::TypeRef::name>(DatatypeText) +
             many(append<&ast::TypeRef::generics>(ref<GenericArgsRule>)) +
             many(append<&ast::TypeRef::suffixes>(ref<TypeSuffixRule>))> {};

  struct TypeModRule
      : rule<create<ast::TypeMod>(option(
            flag<&ast::TypeMod::byRef>(lit<"&">) +
            option(assign<&ast::TypeMod::dir>(kw<"in"> | kw<"out"> |
                                               kw<"inout">))))> {};

  // -- Expressions ------------------------------------------------------

  struct ArgRule
      : rule<create<ast::Arg>(
            option(assign<&ast::Arg::name>(ref<ID>) + lit<":">) +
            assign<&ast::Arg::value>(ref<AssignRule>))> {};

  struct ArgListRule
      : rule<create<ast::ArgList>(
            lit<"("> +
            option(append<&ast::ArgList::items>(ref<ArgRule>) +
                   many(lit<","> +
                        append<&ast::ArgList::items>(ref<ArgRule>))) +
            lit<")">)> {};

  struct ConstructCallRule
      : rule<create<ast::ConstructExpr>(
            assign<&ast::ConstructExpr::type>(ref<TypeRefRule>) +
            assign<&ast::ConstructExpr::args>(ref<ArgListRule>))> {};

  struct FuncCallRule
      : rule<create<ast::CallExpr>(
            option(flag<&ast::CallExpr::rooted>(lit<"::">)) +
            many(append<&ast::CallExpr::scope>(ref<ScopeSegmentRule>)) +
            assign<&ast::CallExpr::name>(ref<ID>) +
            assign<&ast::CallExpr::args>(ref<ArgListRule>))> {};

  struct VarAccessRule
      : rule<create<ast::VarAccessExpr>(
            option(flag<&ast::VarAccessExpr::rooted>(lit<"::">)) +
            many(append<&ast::VarAccessExpr::scope>(ref<ScopeSegmentRule>)) +
            assign<&ast::VarAccessExpr::name>(ref<ID>))> {};

  struct CastRule
      : rule<create<ast::CastExpr>(
            kw<"cast"> + Lt + assign<&ast::CastExpr::type>(ref<TypeRefRule>) +
            Gt + lit<"("> + assign<&ast::CastExpr::operand>(ref<AssignRule>) +
            lit<")">)> {};

  struct LambdaParamRule
      : rule<create<ast::LambdaParam>(
            option(assign<&ast::LambdaParam::type>(ref<TypeRefRule>) +
                   assign<&ast::LambdaParam::mod>(ref<TypeModRule>)) +
            assign<&ast::LambdaParam::name>(ref<ID>))> {};

  struct LambdaRule
      : rule<create<ast::LambdaExpr>(
            kw<"function"> + lit<"("> +
            option(append<&ast::LambdaExpr::params>(ref<LambdaParamRule>) +
                   many(lit<","> + append<&ast::LambdaExpr::params>(
                                       ref<LambdaParamRule>))) +
            lit<")"> +
            assign<&ast::LambdaExpr::body>(ref<BlockStmtRule>))> {};

  // `VOID` alone is ported directly; as a *parameter list*, `(void)` already
  // reads as a one-parameter list with type `void` and no name (see
  // ParamListRule), so no separate alternative for it repeats there.
  struct PrimaryExprRule
      : rule<create<ast::VoidExpr>(kw<"void">)  //
             | ref<ConstructCallRule>           //
             | ref<FuncCallRule>                //
             | ref<VarAccessRule>               //
             | ref<CastRule>                    //
             | create<ast::LiteralExpr>(
                   assign<&ast::LiteralExpr::text>(ref<LITERAL>))  //
             | (lit<"("> + ref<AssignRule> + lit<")">)             //
             | ref<LambdaRule>> {};

  struct IndexArgFirstRule
      : rule<create<ast::IndexArg>(
            option(assign<&ast::IndexArg::name>(ref<ID>) + lit<":">) +
            assign<&ast::IndexArg::value>(ref<AssignRule>))> {};

  struct IndexArgRestRule
      : rule<create<ast::IndexArg>(
            option(assign<&ast::IndexArg::name>(ref<ID>)) + lit<":"> +
            assign<&ast::IndexArg::value>(ref<AssignRule>))> {};

  struct PostfixChainRule
      : rule<ref<PrimaryExprRule> +
             many(
                 nest<ast::MemberCallExpr, &ast::MemberCallExpr::object>(
                     lit<"."> +
                     option(flag<&ast::MemberCallExpr::rooted>(
                         lit<"::">)) +
                     many(append<&ast::MemberCallExpr::scope>(
                         ref<ScopeSegmentRule>)) +
                     assign<&ast::MemberCallExpr::name>(ref<ID>) +
                     assign<&ast::MemberCallExpr::args>(ref<ArgListRule>))  //
                 | nest<ast::MemberExpr, &ast::MemberExpr::object>(
                       lit<"."> + assign<&ast::MemberExpr::name>(ref<ID>))  //
                 | nest<ast::IndexExpr, &ast::IndexExpr::object>(
                       lit<"["> +
                       append<&ast::IndexExpr::items>(ref<IndexArgFirstRule>) +
                       many(lit<","> + append<&ast::IndexExpr::items>(
                                           ref<IndexArgRestRule>)) +
                       lit<"]">)  //
                 | nest<ast::CallPostfixExpr, &ast::CallPostfixExpr::callee>(
                       assign<&ast::CallPostfixExpr::args>(
                           ref<ArgListRule>))  //
                 | nest<ast::PostIncDecExpr, &ast::PostIncDecExpr::operand>(
                       assign<&ast::PostIncDecExpr::op>(lit<"++"> |
                                                         lit<"--">)))> {};

  struct PrefixChainRule
      : rule<create<ast::PrefixExpr>(
                 assign<&ast::PrefixExpr::op>(
                     lit<"-"> | lit<"+"> | lit<"!"> | kw<"not"> | lit<"++"> |
                     lit<"--"> | lit<"~"> | lit<"@">) +
                 assign<&ast::PrefixExpr::operand>(ref<PrefixChainRule>))  //
             | ref<PostfixChainRule>> {};

  struct InitListRule;

  struct InitListItemRule
      : rule<create<ast::InitListItem>(
                 assign<&ast::InitListItem::value>(ref<AssignRule>))  //
             | create<ast::InitListItem>(
                   assign<&ast::InitListItem::nested>(ref<InitListRule>))> {
  };

  struct InitListRule
      : rule<create<ast::InitList>(
            lit<"{"> +
            option(append<&ast::InitList::items>(ref<InitListItemRule>)) +
            many(lit<","> + option(append<&ast::InitList::items>(
                                        ref<InitListItemRule>))) +
            lit<"}">)> {};

  struct ExprTermRule
      : rule<ref<PrefixChainRule>  //
             | create<ast::InitListExpr>(
                   option(assign<&ast::InitListExpr::type>(ref<TypeRefRule>) +
                          lit<"=">) +
                   assign<&ast::InitListExpr::list>(ref<InitListRule>))> {};

  static constexpr auto MulOp = lit<"*"> | lit<"/"> | lit<"%">;
  static constexpr auto AddOp = lit<"+"> | lit<"-">;

  struct ArithLevelRule
      : infix<ExprTermRule, &ast::BinExpr::left, &ast::BinExpr::op,
              &ast::BinExpr::right,
              (left(lit<"**">) >> left(MulOp) >> left(AddOp))> {};

  // `<<`/`>>`/`>>>` open on the same tokens as the relational operators
  // below: kept out of the infix tables and hand-rolled, like a nested
  // generic close splitting `>>` into two adjacent `>`.
  struct ShiftLevelRule
      : rule<ref<ArithLevelRule> +
             many(nest<ast::BinExpr, &ast::BinExpr::left>(
                 assign<&ast::BinExpr::op>(ref<ShiftOpRule>) +
                 assign<&ast::BinExpr::right>(ref<ArithLevelRule>)))> {};

  struct ExprRule
      : infix<ShiftLevelRule, &ast::BinExpr::left, &ast::BinExpr::op,
              &ast::BinExpr::right,
              (left(lit<"&">) >> left(lit<"^">) >> left(lit<"|">) >>
               left(lit<"<="> | lit<">="> | Lt | Gt) >>
               left(lit<"=="> | lit<"!="> | kw<"is"> | kw<"!is"> |
                    kw<"xor"> | lit<"^^">) >>
               left(lit<"&&"> | kw<"and">) >> left(lit<"||"> | kw<"or">))> {};

  struct ConditionRule
      : rule<create<ast::CondExpr>(
                 assign<&ast::CondExpr::cond>(ref<ExprRule>) + lit<"?"> +
                 assign<&ast::CondExpr::thenValue>(ref<AssignRule>) +
                 lit<":"> +
                 assign<&ast::CondExpr::elseValue>(ref<AssignRule>))  //
             | ref<ExprRule>> {};

  static constexpr auto AssignOp =
      lit<"+="> | lit<"-="> | lit<"*="> | lit<"/="> | lit<"|="> |
      lit<"&="> | lit<"^="> | lit<"%="> | lit<"**="> | lit<"<<="> |
      lit<">>="> | lit<">>>="> | lit<"=", !cr<"=">>;

  struct AssignRule
      : rule<create<ast::AssignExpr>(
                 assign<&ast::AssignExpr::target>(ref<ConditionRule>) +
                 assign<&ast::AssignExpr::op>(AssignOp) +
                 assign<&ast::AssignExpr::value>(ref<AssignRule>))  //
             | ref<ConditionRule>> {};

  // -- Statements -------------------------------------------------------

  struct ExprStmtRule
      : rule<create<ast::ExprStmt>(
            option(assign<&ast::ExprStmt::value>(ref<AssignRule>)) +
            lit<";">)> {};

  struct BlockStmtRule
      : rule<create<ast::BlockStmt>(
            lit<"{"> +
            many(append<&ast::BlockStmt::items>(ref<VarDeclRule>) |
                 append<&ast::BlockStmt::items>(ref<StatementRule>)) +
            lit<"}">)> {};

  struct IfStmtRule
      : rule<kw<"if"> + lit<"("> +
             assign<&ast::IfStmt::cond>(ref<AssignRule>) + lit<")"> +
             assign<&ast::IfStmt::thenBranch>(ref<StatementRule>) +
             option(commit(kw<"else">, assign<&ast::IfStmt::elseBranch>(ref<StatementRule>)))> {
  };

  struct ForStmtRule
      : rule<kw<"for"> + lit<"("> +
             assign<&ast::ForStmt::init>(ref<VarDeclRule> |
                                          ref<ExprStmtRule>) +
             assign<&ast::ForStmt::cond>(ref<ExprStmtRule>) +
             option(append<&ast::ForStmt::steps>(ref<AssignRule>) +
                    many(lit<","> +
                         append<&ast::ForStmt::steps>(ref<AssignRule>))) +
             lit<")"> + assign<&ast::ForStmt::body>(ref<StatementRule>)> {};

  struct WhileStmtRule
      : rule<kw<"while"> + lit<"("> +
             assign<&ast::WhileStmt::cond>(ref<AssignRule>) + lit<")"> +
             assign<&ast::WhileStmt::body>(ref<StatementRule>)> {};

  struct DoWhileStmtRule
      : rule<kw<"do"> + assign<&ast::DoWhileStmt::body>(ref<StatementRule>) +
             kw<"while"> + lit<"("> +
             assign<&ast::DoWhileStmt::cond>(ref<AssignRule>) + lit<")"> +
             lit<";">> {};

  struct BreakStmtRule
      : rule<create<ast::BreakStmt>(kw<"break"> + lit<";">)> {};

  struct ContinueStmtRule
      : rule<create<ast::ContinueStmt>(kw<"continue"> + lit<";">)> {};

  struct ReturnStmtRule
      : rule<create<ast::ReturnStmt>(
            kw<"return"> +
            option(assign<&ast::ReturnStmt::value>(ref<AssignRule>)) +
            lit<";">)> {};

  struct TryStmtRule
      : rule<kw<"try"> + assign<&ast::TryStmt::tryBlock>(ref<BlockStmtRule>) +
             kw<"catch"> +
             assign<&ast::TryStmt::catchBlock>(ref<BlockStmtRule>)> {};

  struct CaseClauseRule
      : rule<create<ast::CaseClause>(
            ((kw<"case"> + assign<&ast::CaseClause::value>(ref<ExprRule>))  //
             | flag<&ast::CaseClause::isDefault>(kw<"default">)) +
            lit<":"> +
            many(append<&ast::CaseClause::body>(ref<StatementRule>)))> {};

  struct SwitchStmtRule
      : rule<kw<"switch"> + lit<"("> +
             assign<&ast::SwitchStmt::value>(ref<AssignRule>) + lit<")"> +
             lit<"{"> +
             many(append<&ast::SwitchStmt::cases>(ref<CaseClauseRule>)) +
             lit<"}">> {};

  struct StatementRule
      : rule<ref<IfStmtRule>          //
             | ref<ForStmtRule>       //
             | ref<WhileStmtRule>     //
             | ref<ReturnStmtRule>    //
             | ref<BlockStmtRule>     //
             | ref<BreakStmtRule>     //
             | ref<ContinueStmtRule>  //
             | ref<DoWhileStmtRule>   //
             | ref<SwitchStmtRule>    //
             | ref<ExprStmtRule>      //
             | ref<TryStmtRule>> {};

  // -- Declarations -------------------------------------------------------

  struct ParamRule
      : rule<create<ast::Param>(
            assign<&ast::Param::type>(ref<TypeRefRule>) +
            assign<&ast::Param::mod>(ref<TypeModRule>) +
            option(assign<&ast::Param::name>(ref<ID>)) +
            option(lit<"="> +
                   assign<&ast::Param::defaultValue>(ref<ExprRule>)))> {};

  struct ParamListRule
      : rule<create<ast::ParamList>(
            lit<"("> +
            option(append<&ast::ParamList::params>(ref<ParamRule>) +
                   many(lit<","> +
                        append<&ast::ParamList::params>(ref<ParamRule>))) +
            lit<")">)> {};

  struct VarInitRule
      : rule<create<ast::VarInit>(
            assign<&ast::VarInit::name>(ref<ID>) +
            option((lit<"="> +
                    (assign<&ast::VarInit::initList>(ref<InitListRule>)  //
                     | assign<&ast::VarInit::initExpr>(ref<ExprRule>)))  //
                   | assign<&ast::VarInit::ctorArgs>(ref<ArgListRule>)))> {};

  struct VarDeclRule
      : rule<create<ast::VarDecl>(
            option(assign<&ast::VarDecl::visibility>(Visibility)) +
            assign<&ast::VarDecl::type>(ref<TypeRefRule>) +
            append<&ast::VarDecl::declarators>(ref<VarInitRule>) +
            many(lit<","> +
                 append<&ast::VarDecl::declarators>(ref<VarInitRule>)) +
            lit<";">)> {};

  struct ImportDeclRule
      : rule<create<ast::ImportDecl>(
            kw<"import"> +
            assign<&ast::ImportDecl::returnType>(ref<TypeRefRule>) +
            option(flag<&ast::ImportDecl::returnByRef>(lit<"&">)) +
            assign<&ast::ImportDecl::name>(ref<ID>) +
            assign<&ast::ImportDecl::params>(ref<ParamListRule>) +
            assign<&ast::ImportDecl::attr>(FuncAttr) + kw<"from"> +
            assign<&ast::ImportDecl::source>(ref<STRING>) + lit<";">)> {};

  struct EnumValueRule
      : rule<create<ast::EnumValue>(
            assign<&ast::EnumValue::name>(ref<ID>) +
            option(lit<"="> +
                   assign<&ast::EnumValue::value>(ref<ExprRule>)))> {};

  struct EnumDeclRule
      : rule<create<ast::EnumDecl>(
            many(append<&ast::EnumDecl::modifiers>(FuncModifier)) +
            kw<"enum"> + assign<&ast::EnumDecl::name>(ref<ID>) +
            (flag<&ast::EnumDecl::forwardDecl>(lit<";">)  //
             | (lit<"{"> +
                some(append<&ast::EnumDecl::values>(ref<EnumValueRule>),
                     lit<",">) +
                lit<"}">)))> {};

  struct TypedefDeclRule
      : rule<create<ast::TypedefDecl>(
            kw<"typedef"> +
            assign<&ast::TypedefDecl::primtype>(ref<PRIMTYPE>) +
            assign<&ast::TypedefDecl::name>(ref<ID>) + lit<";">)> {};

  struct FuncdefDeclRule
      : rule<create<ast::FuncdefDecl>(
            many(append<&ast::FuncdefDecl::modifiers>(FuncModifier)) +
            kw<"funcdef"> +
            assign<&ast::FuncdefDecl::returnType>(ref<TypeRefRule>) +
            option(flag<&ast::FuncdefDecl::returnByRef>(lit<"&">)) +
            assign<&ast::FuncdefDecl::name>(ref<ID>) +
            assign<&ast::FuncdefDecl::params>(ref<ParamListRule>) +
            lit<";">)> {};

  struct AccessorDeclRule
      : rule<create<ast::AccessorDecl>(
            (flag<&ast::AccessorDecl::isSetter>(kw<"set">)  //
             | kw<"get">) +
            option(flag<&ast::AccessorDecl::isConst>(kw<"const">)) +
            assign<&ast::AccessorDecl::attr>(FuncAttr) +
            (flag<&ast::AccessorDecl::declOnly>(lit<";">)  //
             | assign<&ast::AccessorDecl::body>(ref<BlockStmtRule>)))> {};

  struct VirtPropDeclRule
      : rule<create<ast::VirtPropDecl>(
            option(assign<&ast::VirtPropDecl::visibility>(Visibility)) +
            assign<&ast::VirtPropDecl::type>(ref<TypeRefRule>) +
            option(flag<&ast::VirtPropDecl::byRef>(lit<"&">)) +
            assign<&ast::VirtPropDecl::name>(ref<ID>) + lit<"{"> +
            many(append<&ast::VirtPropDecl::accessors>(
                ref<AccessorDeclRule>)) +
            lit<"}">)> {};

  struct FuncDeclRule
      : rule<create<ast::FuncDecl>(
            many(append<&ast::FuncDecl::modifiers>(FuncModifier)) +
            option(assign<&ast::FuncDecl::visibility>(Visibility)) +
            option(
                (assign<&ast::FuncDecl::returnType>(ref<TypeRefRule>) +
                 option(flag<&ast::FuncDecl::returnByRef>(lit<"&">)))  //
                | flag<&ast::FuncDecl::isDestructor>(lit<"~">)) +
            assign<&ast::FuncDecl::name>(ref<ID>) +
            assign<&ast::FuncDecl::params>(ref<ParamListRule>) +
            option(flag<&ast::FuncDecl::isConst>(kw<"const">)) +
            option(assign<&ast::FuncDecl::attr>(FuncAttr)) +
            (flag<&ast::FuncDecl::declOnly>(lit<";">)  //
             | assign<&ast::FuncDecl::body>(ref<BlockStmtRule>)))> {};

  struct ClassMemberRule
      : rule<ref<VirtPropDeclRule>  //
             | ref<FuncDeclRule>    //
             | ref<VarDeclRule>     //
             | ref<FuncdefDeclRule>> {};

  struct ClassDeclRule
      : rule<create<ast::ClassDecl>(
            many(append<&ast::ClassDecl::modifiers>(ClassModifier)) +
            kw<"class"> + assign<&ast::ClassDecl::name>(ref<ID>) +
            (flag<&ast::ClassDecl::forwardDecl>(lit<";">)  //
             | (option(lit<":"> +
                       append<&ast::ClassDecl::bases>(ref<ID>) +
                       many(lit<","> +
                            append<&ast::ClassDecl::bases>(ref<ID>))) +
                lit<"{"> +
                many(append<&ast::ClassDecl::members>(ref<ClassMemberRule>)) +
                lit<"}">)))> {};

  struct MixinDeclRule
      : rule<create<ast::MixinDecl>(
            kw<"mixin"> +
            assign<&ast::MixinDecl::class_>(ref<ClassDeclRule>))> {};

  struct IntfMthdDeclRule
      : rule<create<ast::IntfMthdDecl>(
            assign<&ast::IntfMthdDecl::returnType>(ref<TypeRefRule>) +
            option(flag<&ast::IntfMthdDecl::returnByRef>(lit<"&">)) +
            assign<&ast::IntfMthdDecl::name>(ref<ID>) +
            assign<&ast::IntfMthdDecl::params>(ref<ParamListRule>) +
            option(flag<&ast::IntfMthdDecl::isConst>(kw<"const">)) +
            lit<";">)> {};

  struct InterfaceMemberRule
      : rule<ref<VirtPropDeclRule> | ref<IntfMthdDeclRule>> {};

  struct InterfaceDeclRule
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
                    ref<InterfaceMemberRule>)) +
                lit<"}">)))> {};

  struct NamespaceDeclRule
      : rule<create<ast::NamespaceDecl>(
            kw<"namespace"> + assign<&ast::NamespaceDecl::name>(ref<ID>) +
            lit<"{"> +
            some(append<&ast::NamespaceDecl::items>(ref<ScriptItemRule>) |
                 lit<";">) +
            lit<"}">)> {};

  struct ScriptItemRule
      : rule<ref<ImportDeclRule>       //
             | ref<EnumDeclRule>       //
             | ref<TypedefDeclRule>    //
             | ref<ClassDeclRule>      //
             | ref<MixinDeclRule>      //
             | ref<InterfaceDeclRule>  //
             | ref<FuncdefDeclRule>    //
             | ref<VirtPropDeclRule>   //
             | ref<VarDeclRule>        //
             | ref<FuncDeclRule>       //
             | ref<NamespaceDeclRule>> {};

  struct ScriptRule
      : rule<create<ast::Script>(
            some(append<&ast::Script::items>(ref<ScriptItemRule>) |
                 lit<";">))> {};

  using entry = ScriptRule;
  using ignore = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace angelscript
