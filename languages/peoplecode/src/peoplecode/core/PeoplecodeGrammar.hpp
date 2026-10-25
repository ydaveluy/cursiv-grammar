// The peoplecode language: its grammar.
#pragma once

#include <cstdint>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "peoplecode/core/ast.hpp"

namespace peoplecode {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unquoted = cursiv::text_conv<unquote>;

struct Peoplecode : cursiv::GrammarSpec {
  // GENERIC_ID admits '#' past the first letter, so a keyword's boundary
  // must count it as a word byte too, or "Get#1" would split after "Get".
  static constexpr auto word_boundary = cr<"a-zA-Z0-9_#">;

  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct BLOCK_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  struct ANGLE_COMMENT : terminal<through(lit<"<*">, lit<"*>">)> {};
  struct DOC_COMMENT
      : terminal<through(lit<"/+">, lit<"+/">) + option(lit<";">)> {};
  struct REM_COMMENT
      : terminal<option(some(cr<R"( \t\r\n)">)) + ikw<"rem"> +
                 some(cr<R"( \t\r\n)">) + many(cr<"^;">) + lit<";">> {};

  static constexpr auto IdBody = cr<"a-zA-Z"> + many(cr<"0-9a-zA-Z_#">);
  struct GENERIC_ID : terminal<IdBody> {};
  struct VAR_ID : terminal<lit<"&"> + IdBody> {};
  struct SYS_VAR_ID : terminal<lit<"%"> + IdBody> {};

  static constexpr auto IntBody = lit<"0"> | (cr<"1-9"> + many(d));
  struct IntegerLiteral : terminal<IntBody, std::int64_t> {};
  struct DecimalLiteral
      : terminal<IntBody + lit<"."> + some(d), double> {};
  struct StringLiteral
      : terminal<lit<"\""> + many(cr<R"(^")">) + lit<"\"">, Unquoted> {};

  // Any word that appears literally in the grammar below reserves its own
  // spelling only at the states that read it; elsewhere the same bytes
  // still lex as GENERIC_ID (loi du terminal).
  struct Id_ : rule<ref<SYS_VAR_ID> | ref<VAR_ID> | ref<GENERIC_ID>> {};

  struct AppClassPath
      : rule<ref<GENERIC_ID> + some(lit<":"> + ref<GENERIC_ID>)> {};
  struct AppPkgPrefix
      : rule<ref<GENERIC_ID> + many(lit<":"> + ref<GENERIC_ID>)> {};

  struct VarType;
  struct VarType
      : rule<(ref<GENERIC_ID> + option(kw<"of"> + ref<VarType>)) |
             ref<AppClassPath>> {};

  struct AppClassImport
      : rule<create<ast::ImportStmt>(
            kw<"import"> +
            ((assign<&ast::ImportStmt::target>(ref<AppPkgPrefix>) +
              lit<":"> +
              flag<&ast::ImportStmt::wildcard>(lit<"*">)) |
             assign<&ast::ImportStmt::target>(ref<AppClassPath>)))> {};

  struct EventKw : rule<kw<"FieldFormula"> | kw<"FieldChange">> {};
  struct RecDefnPath
      : rule<ref<GENERIC_ID> + lit<"."> + ref<GENERIC_ID>> {};
  struct ExtFuncImport
      : rule<create<ast::DeclareFuncStmt>(
            kw<"Declare"> + kw<"Function"> +
            assign<&ast::DeclareFuncStmt::name>(ref<GENERIC_ID>) +
            kw<"PeopleCode"> +
            assign<&ast::DeclareFuncStmt::recDefnPath>(ref<RecDefnPath>) +
            assign<&ast::DeclareFuncStmt::event>(ref<EventKw>))> {};

  struct Expr;

  struct Param
      : rule<assign<&ast::Param::name>(ref<VAR_ID>) +
             option(kw<"As"> +
                    assign<&ast::Param::varType>(ref<VarType>))> {};

  template <auto Member>
  static constexpr auto ParamListOf =
      lit<"("> + option(some(append<Member>(ref<Param>), lit<",">)) +
      lit<")">;

  struct MethodDecl
      : rule<create<ast::MethodDecl>(
            kw<"method"> + assign<&ast::MethodDecl::name>(ref<GENERIC_ID>) +
            ParamListOf<&ast::MethodDecl::params> +
            option(kw<"Returns"> + assign<&ast::MethodDecl::returnType>(
                                        ref<VarType>)))> {};

  struct ConstantDecl
      : rule<create<ast::ConstantDecl>(
            kw<"Constant"> + assign<&ast::ConstantDecl::name>(ref<VAR_ID>) +
            lit<"="> + assign<&ast::ConstantDecl::value>(ref<Expr>))> {};

  struct PropertyDecl
      : rule<create<ast::PropertyDecl>(
            kw<"property"> +
            assign<&ast::PropertyDecl::varType>(ref<VarType>) +
            assign<&ast::PropertyDecl::name>(ref<GENERIC_ID>) +
            option(flag<&ast::PropertyDecl::hasGet>(kw<"get">)) +
            option(flag<&ast::PropertyDecl::hasSet>(kw<"set">)) +
            option(flag<&ast::PropertyDecl::readonly>(
                kw<"readonly">)))> {};

  struct InstanceDecl
      : rule<create<ast::InstanceDecl>(
            kw<"instance"> +
            assign<&ast::InstanceDecl::varType>(ref<VarType>) +
            some(append<&ast::InstanceDecl::names>(ref<VAR_ID>),
                 lit<",">))> {};

  static constexpr auto ClassMembers =
      some(append<&ast::ClassBlock::members>(
               ref<MethodDecl> | ref<ConstantDecl> | ref<PropertyDecl> |
               ref<InstanceDecl>) +
           many(lit<";">));

  struct ClassBlock
      : rule<create<ast::ClassBlock>(
            option(flag<&ast::ClassBlock::isPrivate>(kw<"private">)) +
            ClassMembers)> {};

  struct PrivateClassBlock
      : rule<create<ast::ClassBlock>(
            flag<&ast::ClassBlock::isPrivate>(kw<"private">) +
            ClassMembers)> {};

  struct ClassDeclaration
      : rule<create<ast::ClassDecl>(
            kw<"class"> + assign<&ast::ClassDecl::name>(ref<GENERIC_ID>) +
            option(append<&ast::ClassDecl::blocks>(ref<ClassBlock>) +
                   many(append<&ast::ClassDecl::blocks>(
                       ref<PrivateClassBlock>))) +
            kw<"end-class">)> {};

  struct Stmt;

  template <auto Member>
  static constexpr auto StmtListOf =
      many(append<Member>(ref<Stmt>) + some(lit<";">)) +
      option(append<Member>(ref<Stmt>));

  struct MethodImpl
      : rule<create<ast::MethodImplStmt>(
            kw<"method"> +
            assign<&ast::MethodImplStmt::name>(ref<GENERIC_ID>) +
            StmtListOf<&ast::MethodImplStmt::body> + kw<"end-method">)> {};

  struct GetImpl
      : rule<create<ast::GetImplStmt>(
            kw<"get"> + assign<&ast::GetImplStmt::name>(ref<GENERIC_ID>) +
            StmtListOf<&ast::GetImplStmt::body> + kw<"end-get">)> {};

  struct SetImpl
      : rule<create<ast::SetImplStmt>(
            kw<"set"> + assign<&ast::SetImplStmt::name>(ref<GENERIC_ID>) +
            StmtListOf<&ast::SetImplStmt::body> + kw<"end-set">)> {};

  struct FuncImpl
      : rule<create<ast::FuncImplStmt>(
            kw<"Function"> +
            assign<&ast::FuncImplStmt::name>(ref<GENERIC_ID>) +
            option(ParamListOf<&ast::FuncImplStmt::params>) +
            option(kw<"Returns"> + assign<&ast::FuncImplStmt::returnType>(
                                        ref<VarType>)) +
            option(lit<";">) + StmtListOf<&ast::FuncImplStmt::body> +
            kw<"End-Function">)> {};

  struct VarDeclarator
      : rule<create<ast::VarDeclarator>(
            assign<&ast::VarDeclarator::name>(ref<VAR_ID>) +
            option(lit<"="> +
                   assign<&ast::VarDeclarator::init>(ref<Expr>)))> {};

  struct VarDeclaration
      : rule<create<ast::VarDeclStmt>(
            assign<&ast::VarDeclStmt::scope>(ref<GENERIC_ID>) +
            assign<&ast::VarDeclStmt::varType>(ref<VarType>) +
            some(append<&ast::VarDeclStmt::declarators>(ref<VarDeclarator>),
                 lit<",">))> {};

  struct IfStmt
      : rule<create<ast::IfStmt>(
            kw<"If"> + assign<&ast::IfStmt::cond>(ref<Expr>) + kw<"Then"> +
            option(lit<";">) + StmtListOf<&ast::IfStmt::thenBranch> +
            option(kw<"Else"> + option(lit<";">) +
                   StmtListOf<&ast::IfStmt::elseBranch>) +
            kw<"End-If">)> {};

  struct ForStmt
      : rule<create<ast::ForStmt>(
            kw<"For"> + assign<&ast::ForStmt::var>(ref<VAR_ID>) + lit<"="> +
            assign<&ast::ForStmt::from>(ref<Expr>) + kw<"To"> +
            assign<&ast::ForStmt::to>(ref<Expr>) +
            option(lit<";"> | (kw<"Step"> + assign<&ast::ForStmt::step>(
                                                 ref<Expr>))) +
            StmtListOf<&ast::ForStmt::body> + kw<"End-For">)> {};

  struct WhileStmt
      : rule<create<ast::WhileStmt>(
            kw<"While"> + assign<&ast::WhileStmt::cond>(ref<Expr>) +
            option(lit<";">) + StmtListOf<&ast::WhileStmt::body> +
            kw<"End-While">)> {};

  struct WhenBranch
      : rule<create<ast::WhenBranch>(
            kw<"When"> +
            option(assign<&ast::WhenBranch::op>(lit<"="> | lit<">">)) +
            assign<&ast::WhenBranch::value>(ref<Expr>) +
            StmtListOf<&ast::WhenBranch::body>)> {};

  struct EvaluateStmt
      : rule<create<ast::EvaluateStmt>(
            kw<"Evaluate"> + assign<&ast::EvaluateStmt::value>(ref<Expr>) +
            some(append<&ast::EvaluateStmt::branches>(ref<WhenBranch>)) +
            option(flag<&ast::EvaluateStmt::hasOtherBranch>(
                       kw<"When-Other">) +
                   StmtListOf<&ast::EvaluateStmt::otherBranch>) +
            kw<"End-Evaluate">)> {};

  struct TryCatchStmt
      : rule<create<ast::TryCatchStmt>(
            kw<"try"> + StmtListOf<&ast::TryCatchStmt::tryBody> +
            kw<"catch"> + kw<"Exception"> +
            assign<&ast::TryCatchStmt::exceptionVar>(ref<VAR_ID>) +
            StmtListOf<&ast::TryCatchStmt::catchBody> + kw<"end-try">)> {};

  struct CreateInvocation
      : rule<create<ast::CreateExpr>(
            kw<"create"> +
            assign<&ast::CreateExpr::className>(ref<AppClassPath> |
                                                ref<GENERIC_ID>) +
            lit<"("> +
            option(some(append<&ast::CreateExpr::args>(ref<Expr>),
                        lit<",">)) +
            lit<")">)> {};

  struct IntLitRule
      : rule<assign<&ast::IntLit::value>(ref<IntegerLiteral>)> {};
  struct DecimalLitRule
      : rule<assign<&ast::DecimalLit::value>(ref<DecimalLiteral>)> {};
  struct StringLitRule
      : rule<assign<&ast::StringLit::value>(ref<StringLiteral>)> {};
  struct BoolLitRule
      : rule<create<ast::BoolLit> +
             (flag<&ast::BoolLit::value>(kw<"True">) |
              kw<"False">)> {};
  struct Literal
      : rule<ref<DecimalLitRule> | ref<IntLitRule> | ref<StringLitRule> |
             ref<BoolLitRule>> {};

  struct IdExprRule : rule<assign<&ast::IdExpr::name>(ref<Id_>)> {};

  struct Atom
      : rule<(lit<"("> + ref<Expr> + lit<")">) |
             create<ast::DynamicRefExpr>(
                 lit<"@"> +
                 assign<&ast::DynamicRefExpr::operand>(ref<Atom>)) |
             ref<Literal> | ref<IdExprRule> | ref<CreateInvocation>> {};

  struct Postfix
      : rule<ref<Atom> +
             many(nest<ast::DotExpr, &ast::DotExpr::object>(
                      lit<"."> +
                      assign<&ast::DotExpr::name>(ref<Id_>)) |
                  nest<ast::IndexExpr, &ast::IndexExpr::array>(
                      lit<"["> +
                      some(append<&ast::IndexExpr::indices>(ref<Expr>),
                           lit<",">) +
                      lit<"]">) |
                  nest<ast::CallExpr, &ast::CallExpr::callee>(
                      lit<"("> +
                      option(some(append<&ast::CallExpr::args>(ref<Expr>),
                                  lit<",">)) +
                      lit<")">))> {};

  struct Unary
      : rule<create<ast::NegateExpr>(
                lit<"-"> +
                assign<&ast::NegateExpr::operand>(ref<Unary>)) |
             create<ast::NotExpr>(
                 kw<"Not"> + assign<&ast::NotExpr::operand>(ref<Unary>)) |
             ref<Postfix>> {};

  struct OP_MUL : terminal<lit<"*">, value<ast::BinOp::multiply>> {};
  struct OP_DIV : terminal<lit<"/">, value<ast::BinOp::divide>> {};
  struct OP_ADD : terminal<lit<"+">, value<ast::BinOp::add>> {};
  struct OP_SUB : terminal<lit<"-">, value<ast::BinOp::subtract>> {};
  struct OP_LE : terminal<lit<"<=">, value<ast::BinOp::less_equal>> {};
  struct OP_GE
      : terminal<lit<">=">, value<ast::BinOp::greater_equal>> {};
  struct OP_LT : terminal<lit<"<">, value<ast::BinOp::less>> {};
  struct OP_GT : terminal<lit<">">, value<ast::BinOp::greater>> {};
  struct OP_EQ : terminal<lit<"=">, value<ast::BinOp::equal>> {};
  struct OP_NE : terminal<lit<"<>">, value<ast::BinOp::not_equal>> {};
  struct OP_AND : terminal<kw<"And">, value<ast::BinOp::logical_and>> {};
  struct OP_OR : terminal<kw<"Or">, value<ast::BinOp::logical_or>> {};
  struct OP_CONCAT : terminal<lit<"|">, value<ast::BinOp::concat>> {};

  static constexpr auto OpMulDiv = ref<OP_MUL> | ref<OP_DIV>;
  static constexpr auto OpAddSub = ref<OP_ADD> | ref<OP_SUB>;
  static constexpr auto OpCompare =
      ref<OP_LE> | ref<OP_GE> | ref<OP_LT> | ref<OP_GT>;
  static constexpr auto OpEquality = ref<OP_EQ> | ref<OP_NE>;
  static constexpr auto OpConcat = ref<OP_CONCAT>;

  struct Expr
      : infix<Unary, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(OpMulDiv) >>      //
               left(OpAddSub) >>     //
               left(OpCompare) >>    //
               left(OpEquality) >>   //
               right(ref<OP_AND>) >>  //
               right(ref<OP_OR>) >>   //
               left(OpConcat))> {};

  struct Stmt
      : rule<ref<AppClassImport> | ref<ExtFuncImport> |
             ref<ClassDeclaration> | ref<MethodImpl> | ref<GetImpl> |
             ref<SetImpl> | ref<FuncImpl> | ref<VarDeclaration> |
             ref<IfStmt> | ref<ForStmt> | ref<WhileStmt> |
             ref<EvaluateStmt> | ref<TryCatchStmt> |
             create<ast::ExitStmt> + kw<"Exit"> |
             create<ast::BreakStmt> + kw<"Break"> |
             create<ast::ErrorStmt>(
                 kw<"Error"> + assign<&ast::ErrorStmt::value>(ref<Expr>)) |
             create<ast::WarningStmt>(
                 kw<"Warning"> +
                 assign<&ast::WarningStmt::value>(ref<Expr>)) |
             create<ast::ReturnStmt>(
                 kw<"Return"> +
                 option(assign<&ast::ReturnStmt::value>(ref<Expr>))) |
             create<ast::ThrowStmt>(
                 kw<"throw"> + assign<&ast::ThrowStmt::value>(ref<Expr>)) |
             create<ast::AssignStmt>(
                 assign<&ast::AssignStmt::target>(ref<Expr>) + lit<"="> +
                 assign<&ast::AssignStmt::value>(ref<Expr>)) |
             create<ast::ExprStmt>(
                 assign<&ast::ExprStmt::value>(ref<Expr>))> {};

  struct Program
      : rule<create<ast::Program>(
            StmtListOf<&ast::Program::statements>)> {};

  using entry = Program;
  using ignore = WS;
  using hidden = list<BLOCK_COMMENT, ANGLE_COMMENT, DOC_COMMENT,
                      REM_COMMENT>;
};

}  // namespace peoplecode

