// The peoplecode language: its grammar.
#pragma once

#include <cstdint>
#include <memory_resource>
#include <span>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "peoplecode/core/ast.hpp"

namespace peoplecode {

inline constexpr auto unquote = [](std::string_view text,
                                   std::pmr::memory_resource* memory) {
  auto body = text.substr(1, text.size() - 2);
  const auto room = cursiv::text_room(memory, body.size());
  std::size_t size = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    room[size++] = body[i];
    if (body[i] == text.front() && i + 1 < body.size() && body[i + 1] == text.front()) ++i;
  }
  return std::string_view(room.data(), size);
};

inline constexpr auto type_text = [](std::span<const std::string_view> parts,
                                     std::pmr::memory_resource* memory) {
  const auto isOf = [](std::string_view part) {
    return part.size() == 2 && (part[0] == 'o' || part[0] == 'O') &&
           (part[1] == 'f' || part[1] == 'F');
  };
  std::size_t capacity = parts.size() * 2;
  for (auto part : parts) capacity += part.size();
  const auto room = cursiv::text_room(memory, capacity);
  std::size_t size = 0;
  const bool collection = parts.size() >= 3 && isOf(parts[1]);
  for (std::size_t i = 0; i < parts.size(); ++i) {
    if (collection && (i == 1 || i == 2)) room[size++] = ' ';
    for (char c : parts[i]) room[size++] = c;
  }
  return std::string_view(room.data(), size);
};

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
      : terminal<many(cr<R"( \t\r\n)">) + ikw<"rem"> +
                 some(cr<R"( \t\r\n)">) + many(cr<"^;">) + lit<";">> {};

  static constexpr auto IdBody = cr<"a-zA-Z"> + many(cr<"0-9a-zA-Z_#">);
  struct GENERIC_ID : terminal<IdBody> {};
  struct VAR_ID : terminal<lit<"&"> + IdBody> {};
  struct SYS_VAR_ID : terminal<lit<"%"> + IdBody> {};

  static constexpr auto IntBody = some(d);
  struct IntegerLiteral : terminal<as<std::int64_t>(IntBody)> {};
  struct DecimalLiteral : terminal<as<double>((IntBody + lit<"."> + some(d)) |
                                   (lit<"."> + some(d)))> {};
  struct StringLiteral
      : terminal<as<unquote>(
            (lit<"\""> + many(lit<"\"\""> | cr<R"(^"\r\n)">) + lit<"\"">) |
            (lit<"'"> + many(lit<"''"> | cr<R"(^'\r\n)">) + lit<"'">))> {};

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
      : rule<as<type_text>(ref<GENERIC_ID> +
             (some(lit<":"> + ref<GENERIC_ID>) |
              option(ikw<"of"> + ref<VarType>)))> {};

  struct AppClassImport
      : rule<create<ast::ImportStmt>(
            ikw<"import"> +
            ((assign<&ast::ImportStmt::target>(ref<AppPkgPrefix>) +
              lit<":"> +
              flag<&ast::ImportStmt::wildcard>(lit<"*">)) |
             assign<&ast::ImportStmt::target>(ref<AppClassPath>)))> {};

  struct EventKw : rule<ikw<"FieldFormula"> | ikw<"FieldChange">> {};
  struct RecDefnPath
      : rule<ref<GENERIC_ID> + lit<"."> + ref<GENERIC_ID>> {};
  struct ExtFuncImport
      : rule<create<ast::DeclareFuncStmt>(
            ikw<"Declare"> + ikw<"Function"> +
            assign<&ast::DeclareFuncStmt::name>(ref<GENERIC_ID>) +
            ikw<"PeopleCode"> +
            assign<&ast::DeclareFuncStmt::recDefnPath>(ref<RecDefnPath>) +
            assign<&ast::DeclareFuncStmt::event>(ref<EventKw>))> {};

  struct Expr;

  struct Param
      : rule<assign<&ast::Param::name>(ref<VAR_ID>) +
             option(ikw<"As"> +
                    assign<&ast::Param::varType>(ref<VarType>))> {};

  template <auto Member>
  static constexpr auto ParamListOf =
      lit<"("> + many(append<Member>(ref<Param>), lit<",">) +
      lit<")">;

  struct MethodDecl
      : rule<create<ast::MethodDecl>(
            ikw<"method"> + assign<&ast::MethodDecl::name>(ref<GENERIC_ID>) +
            ParamListOf<&ast::MethodDecl::params> +
            option(ikw<"Returns"> + assign<&ast::MethodDecl::returnType>(
                                        ref<VarType>)))> {};

  struct ConstantDecl
      : rule<create<ast::ConstantDecl>(
            ikw<"Constant"> + assign<&ast::ConstantDecl::name>(ref<VAR_ID>) +
            lit<"="> + assign<&ast::ConstantDecl::value>(ref<Expr>))> {};

  struct PropertyDecl
      : rule<create<ast::PropertyDecl>(
            ikw<"property"> +
            assign<&ast::PropertyDecl::varType>(ref<VarType>) +
            assign<&ast::PropertyDecl::name>(ref<GENERIC_ID>) +
            option(flag<&ast::PropertyDecl::hasGet>(ikw<"get">)) +
            option(flag<&ast::PropertyDecl::hasSet>(ikw<"set">)) +
            option(flag<&ast::PropertyDecl::readonly>(
                ikw<"readonly">)))> {};

  struct InstanceDecl
      : rule<create<ast::InstanceDecl>(
            ikw<"instance"> +
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
            option(flag<&ast::ClassBlock::isPrivate>(ikw<"private">)) +
            ClassMembers)> {};

  struct PrivateClassBlock
      : rule<create<ast::ClassBlock>(
            flag<&ast::ClassBlock::isPrivate>(ikw<"private">) +
            ClassMembers)> {};

  struct ClassDeclaration
      : rule<create<ast::ClassDecl>(
            ikw<"class"> + assign<&ast::ClassDecl::name>(ref<GENERIC_ID>) +
            option(append<&ast::ClassDecl::blocks>(ref<ClassBlock>) +
                   many(append<&ast::ClassDecl::blocks>(
                       ref<PrivateClassBlock>))) +
            ikw<"end-class">)> {};

  struct Stmt;

  template <auto Member>
  static constexpr auto StmtListOf =
      many(append<Member>(ref<Stmt>) + some(lit<";">)) +
      option(append<Member>(ref<Stmt>));

  struct MethodImpl
      : rule<create<ast::MethodImplStmt>(
            ikw<"method"> +
            assign<&ast::MethodImplStmt::name>(ref<GENERIC_ID>) +
            StmtListOf<&ast::MethodImplStmt::body> + ikw<"end-method">)> {};

  struct GetImpl
      : rule<create<ast::GetImplStmt>(
            ikw<"get"> + assign<&ast::GetImplStmt::name>(ref<GENERIC_ID>) +
            StmtListOf<&ast::GetImplStmt::body> + ikw<"end-get">)> {};

  struct SetImpl
      : rule<create<ast::SetImplStmt>(
            ikw<"set"> + assign<&ast::SetImplStmt::name>(ref<GENERIC_ID>) +
            StmtListOf<&ast::SetImplStmt::body> + ikw<"end-set">)> {};

  struct FuncImpl
      : rule<create<ast::FuncImplStmt>(
            ikw<"Function"> +
            assign<&ast::FuncImplStmt::name>(ref<GENERIC_ID>) +
            option(ParamListOf<&ast::FuncImplStmt::params>) +
            option(ikw<"Returns"> + assign<&ast::FuncImplStmt::returnType>(
                                        ref<VarType>)) +
            option(lit<";">) + StmtListOf<&ast::FuncImplStmt::body> +
            ikw<"End-Function">)> {};

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
            ikw<"If"> + assign<&ast::IfStmt::cond>(ref<Expr>) + ikw<"Then"> +
            option(lit<";">) + StmtListOf<&ast::IfStmt::thenBranch> +
            option(ikw<"Else"> + option(lit<";">) +
                   StmtListOf<&ast::IfStmt::elseBranch>) +
            ikw<"End-If">)> {};

  struct ForStmt
      : rule<create<ast::ForStmt>(
            ikw<"For"> + assign<&ast::ForStmt::var>(ref<VAR_ID>) + lit<"="> +
            assign<&ast::ForStmt::from>(ref<Expr>) + ikw<"To"> +
            assign<&ast::ForStmt::to>(ref<Expr>) +
            option(ikw<"Step"> + assign<&ast::ForStmt::step>(ref<Expr>)) +
            option(lit<";">) +
            StmtListOf<&ast::ForStmt::body> + ikw<"End-For">)> {};

  struct WhileStmt
      : rule<create<ast::WhileStmt>(
            ikw<"While"> + assign<&ast::WhileStmt::cond>(ref<Expr>) +
            option(lit<";">) + StmtListOf<&ast::WhileStmt::body> +
            ikw<"End-While">)> {};

  struct WhenBranch
      : rule<create<ast::WhenBranch>(
            ikw<"When"> +
            option(assign<&ast::WhenBranch::op>(
                as<std::string_view>(option(ikw<"Not">) +
                    (lit<"!="> | lit<"<>"> | lit<"<="> | lit<">="> |
                     lit<"="> | lit<"<"> | lit<">">)))) +
            assign<&ast::WhenBranch::value>(ref<Expr>) + option(lit<";">) +
            StmtListOf<&ast::WhenBranch::body>)> {};

  struct EvaluateStmt
      : rule<create<ast::EvaluateStmt>(
            ikw<"Evaluate"> + assign<&ast::EvaluateStmt::value>(ref<Expr>) +
            some(append<&ast::EvaluateStmt::branches>(ref<WhenBranch>)) +
            option(flag<&ast::EvaluateStmt::hasOtherBranch>(
                       ikw<"When-Other">) +
                   StmtListOf<&ast::EvaluateStmt::otherBranch>) +
            ikw<"End-Evaluate">)> {};

  struct TryCatchStmt
      : rule<create<ast::TryCatchStmt>(
            ikw<"try"> + StmtListOf<&ast::TryCatchStmt::tryBody> +
            ikw<"catch"> + ikw<"Exception"> +
            assign<&ast::TryCatchStmt::exceptionVar>(ref<VAR_ID>) +
            StmtListOf<&ast::TryCatchStmt::catchBody> + ikw<"end-try">)> {};

  struct CreateInvocation
      : rule<create<ast::CreateExpr>(
            ikw<"create"> +
            assign<&ast::CreateExpr::className>(ref<AppClassPath> |
                                                ref<GENERIC_ID>) +
            lit<"("> +
            many(append<&ast::CreateExpr::args>(ref<Expr>),
                        lit<",">) +
            lit<")">)> {};

  struct IntLit
      : rule<assign<&ast::IntLit::value>(ref<IntegerLiteral>)> {};
  struct DecimalLit
      : rule<assign<&ast::DecimalLit::value>(ref<DecimalLiteral>)> {};
  struct StringLit
      : rule<assign<&ast::StringLit::value>(ref<StringLiteral>)> {};
  struct BoolLit
      : rule<create<ast::BoolLit> +
             (flag<&ast::BoolLit::value>(ikw<"True">) |
              ikw<"False">)> {};
  struct Literal
      : rule<ref<DecimalLit> | ref<IntLit> | ref<StringLit> |
             ref<BoolLit>> {};

  struct IdExpr : rule<assign<&ast::IdExpr::name>(ref<Id_>)> {};

  struct Atom
      : rule<(lit<"("> + ref<Expr> + lit<")">) |
             create<ast::DynamicRefExpr>(
                 lit<"@"> +
                 assign<&ast::DynamicRefExpr::operand>(ref<Atom>)) |
             ref<Literal> | ref<IdExpr> | ref<CreateInvocation>> {};

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
                      many(append<&ast::CallExpr::args>(ref<Expr>),
                                  lit<",">) +
                      lit<")">))> {};

  struct Unary
      : rule<create<ast::NegateExpr>(
                lit<"-"> +
                assign<&ast::NegateExpr::operand>(ref<Unary>)) |
             create<ast::NotExpr>(
                 ikw<"Not"> + assign<&ast::NotExpr::operand>(ref<Unary>)) |
             ref<Postfix>> {};


  static constexpr auto OpPower = as<ast::BinOp::power>(lit<"**">);
  static constexpr auto OpMulDiv = as<ast::BinOp::multiply>(lit<"*">) | as<ast::BinOp::divide>(lit<"/">);
  static constexpr auto OpAddSub = as<ast::BinOp::add>(lit<"+">) | as<ast::BinOp::subtract>(lit<"-">);
  static constexpr auto OpCompare =
      as<ast::BinOp::less>(ikw<"Not"> + lit<">=">) |
      as<ast::BinOp::greater>(ikw<"Not"> + lit<"<=">) |
      as<ast::BinOp::less_equal>(ikw<"Not"> + lit<">">) |
      as<ast::BinOp::greater_equal>(ikw<"Not"> + lit<"<">) |
      as<ast::BinOp::less_equal>(lit<"<=">) | as<ast::BinOp::greater_equal>(lit<">=">) | as<ast::BinOp::less>(lit<"<">) | as<ast::BinOp::greater>(lit<">">);
  static constexpr auto OpEquality =
      as<ast::BinOp::equal>(ikw<"Not"> + (lit<"<>"> | lit<"!=">)) |
      as<ast::BinOp::not_equal>(ikw<"Not"> + lit<"=">) |
      as<ast::BinOp::equal>(lit<"=">) |
      as<ast::BinOp::not_equal>(lit<"<>"> | lit<"!=">);
  static constexpr auto OpConcat = as<ast::BinOp::concat>(lit<"|">);

  struct Expr
      : infix<Unary, ast::BinaryExpr,
              levels(left(OpPower), left(OpMulDiv),      //
                     left(OpAddSub),      //
                     left(OpCompare),     //
                     left(OpEquality),    //
                     right(as<ast::BinOp::logical_and>(ikw<"And">)),  //
                     right(as<ast::BinOp::logical_or>(ikw<"Or">)),   //
                     left(OpConcat))> {};

  struct Stmt
      : rule<ref<AppClassImport> | ref<ExtFuncImport> |
             ref<ClassDeclaration> | ref<MethodImpl> | ref<GetImpl> |
             ref<SetImpl> | ref<FuncImpl> | ref<VarDeclaration> |
             ref<IfStmt> | ref<ForStmt> | ref<WhileStmt> |
             ref<EvaluateStmt> | ref<TryCatchStmt> |
             (create<ast::ExitStmt> + ikw<"Exit">) |
             (create<ast::BreakStmt> + ikw<"Break">) |
             create<ast::ErrorStmt>(
                 ikw<"Error"> + assign<&ast::ErrorStmt::value>(ref<Expr>)) |
             create<ast::WarningStmt>(
                 ikw<"Warning"> +
                 assign<&ast::WarningStmt::value>(ref<Expr>)) |
             create<ast::ReturnStmt>(
                 ikw<"Return"> +
                 option(assign<&ast::ReturnStmt::value>(ref<Expr>))) |
             create<ast::ThrowStmt>(
                 ikw<"throw"> + assign<&ast::ThrowStmt::value>(ref<Expr>)) |
             create<ast::AssignStmt>(
                 assign<&ast::AssignStmt::target>(ref<Postfix>) + lit<"="> +
                 assign<&ast::AssignStmt::value>(ref<Expr>)) |
             create<ast::ExprStmt>(
                 assign<&ast::ExprStmt::value>(ref<Expr>))> {};

  struct Program
      : rule<create<ast::Program>(
            StmtListOf<&ast::Program::statements>)> {};

  using entry = Program;
  using ignored = WS;
  using hidden = list<BLOCK_COMMENT, ANGLE_COMMENT, DOC_COMMENT,
                      REM_COMMENT>;
};

}  // namespace peoplecode

