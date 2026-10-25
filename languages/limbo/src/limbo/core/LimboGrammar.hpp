// The Limbo language: its grammar. Basic types are keyword tokens,
// expression lists are comma-separated, and binary operators follow the
// language manual's precedence table.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "limbo/core/ast.hpp"

namespace limbo {

struct Limbo : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \r\n\t)">)> {};
  struct COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};

  struct ID : terminal<cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9_">)> {};

  struct INT : terminal<option((some(d) + lit<"r">) | lit<"R">) + some(d)> {};
  struct REAL : terminal<some(d) + lit<"."> + option(cr<"eE">) +
                         some(cr<"0-9a-zA-Z">)> {};
  struct STRING : terminal<lit<"\""> + many(cr<R"(^")">) + lit<"\"">> {};

  static constexpr auto Assign = lit<"=">;
  static constexpr auto Colon = lit<":">;

  struct Program;
  struct Item;
  struct Statement;
  struct Declaration;
  struct Expression;
  struct MonadicExpr;
  struct Term;
  struct Type;
  struct DataType;
  struct NamedType;
  struct Signature;
  struct FormalArg;
  struct VarDecl;
  struct ConDecl;
  struct TypeDecl;
  struct AdtDecl;
  struct AdtField;

  template <auto Ptr>
  static constexpr auto IdentifierListFor =
      append<Ptr>(ref<ID>) + many(lit<","> + append<Ptr>(ref<ID>));
  template <auto Ptr>
  static constexpr auto ExpressionListFor =
      append<Ptr>(ref<Expression>) + many(lit<","> + append<Ptr>(ref<Expression>));
  static constexpr auto NilOrD = ref<ID> | kw<"nil">;

  // -- Types ----

  struct PrimType
      : rule<create<ast::PrimType>(assign<&ast::PrimType::keyword>(
            kw<"byte"> | kw<"int"> | kw<"big"> | kw<"real"> | kw<"string">))> {
  };

  // A bare name (adt/module/type, `id2` empty) or a module-qualified one
  // (`Module->Type`, `id1` the module).
  struct NamedType
      : rule<create<ast::NamedType>(
            assign<&ast::NamedType::id1>(ref<ID>) +
            option(lit<"->"> + assign<&ast::NamedType::id2>(ref<ID>)))> {};

  struct RefType
      : rule<create<ast::RefType>(kw<"ref"> +
                                  assign<&ast::RefType::target>(ref<NamedType>))> {
  };

  struct TupleType
      : rule<create<ast::TupleType>(
            lit<"("> + append<&ast::TupleType::elements>(ref<DataType>) +
            many(lit<","> + append<&ast::TupleType::elements>(ref<DataType>)) +
            lit<")">)> {};

  struct ArrayOfType
      : rule<create<ast::ArrayOfType>(
            kw<"array"> + kw<"of"> +
            assign<&ast::ArrayOfType::element>(ref<DataType>))> {};

  struct ListOfType
      : rule<create<ast::ListOfType>(
            kw<"list"> + kw<"of"> +
            assign<&ast::ListOfType::element>(ref<DataType>))> {};

  struct ChanOfType
      : rule<create<ast::ChanOfType>(
            kw<"chan"> + kw<"of"> +
            assign<&ast::ChanOfType::element>(ref<DataType>))> {};

  struct DataType
      : rule<ref<PrimType> | ref<TupleType> | ref<ArrayOfType> |
            ref<ListOfType> | ref<ChanOfType> | ref<RefType> | ref<NamedType>> {
  };

  struct FnType
      : rule<create<ast::FnType>(kw<"fn"> +
                                 assign<&ast::FnType::signature>(ref<Signature>))> {
  };

  struct Type : rule<ref<DataType> | ref<FnType>> {};

  // A cast-typed lead excludes `chan of T`, ambiguous with the value form
  // of the same spelling.
  struct CastLeadType
      : rule<ref<PrimType> | ref<TupleType> | ref<ArrayOfType> |
            ref<ListOfType> | ref<RefType> | ref<NamedType> | ref<FnType>> {};

  struct FormalArg
      : rule<create<ast::FormalArg>(
                 flag<&ast::FormalArg::isWildcard>(lit<"*">))  //
             | create<ast::FormalArg>(
                   append<&ast::FormalArg::names>(NilOrD) +
                   many(lit<","> + append<&ast::FormalArg::names>(NilOrD)) +
                   Colon +
                   (assign<&ast::FormalArg::type>(ref<Type>)  //
                    | (flag<&ast::FormalArg::isSelf>(kw<"self">) +
                       option(flag<&ast::FormalArg::refopt>(
                           kw<"ref">)) +
                       assign<&ast::FormalArg::selfType>(ref<ID>))))> {};

  struct Signature
      : rule<create<ast::Signature>(
            lit<"("> +
            option(append<&ast::Signature::params>(ref<FormalArg>) +
                   many(lit<","> +
                        append<&ast::Signature::params>(ref<FormalArg>))) +
            lit<")"> +
            option(Colon + assign<&ast::Signature::result>(ref<Type>)))> {};

  // -- Expressions ----

  struct Identifier : rule<assign<&ast::Ident::name>(ref<ID>)> {};
  struct IntLiteral : rule<assign<&ast::IntLit::text>(ref<INT>)> {};
  struct RealLiteral : rule<assign<&ast::RealLit::text>(ref<REAL>)> {};
  struct StrLiteral : rule<assign<&ast::StringLit::text>(ref<STRING>)> {};

  struct Term
      : rule<(ref<Identifier> | ref<RealLiteral> | ref<StrLiteral> |
              ref<IntLiteral> | create<ast::NilExpr>(kw<"nil">) |
              create<ast::ParenExpr>(lit<"("> +
                                     ExpressionListFor<&ast::ParenExpr::items> +
                                     lit<")">)) +
            many(nest<ast::MemberExpr, &ast::MemberExpr::object>(
                     lit<"."> + assign<&ast::MemberExpr::field>(ref<ID>))  //
                 | nest<ast::ModAccessExpr, &ast::ModAccessExpr::module_>(
                       lit<"->"> +
                       assign<&ast::ModAccessExpr::member>(ref<Term>))  //
                 | nest<ast::CallExpr, &ast::CallExpr::callee>(
                       lit<"("> +
                       option(ExpressionListFor<&ast::CallExpr::args>) +
                       lit<")">)  //
                 | nest<ast::IndexExpr, &ast::IndexExpr::array>(
                       lit<"["> +
                       assign<&ast::IndexExpr::index>(ref<Expression>) +
                       option(flag<&ast::IndexExpr::isSlice>(lit<":">) +
                              option(assign<&ast::IndexExpr::high>(
                                  ref<Expression>))) +
                       lit<"]">)  //
                 | nest<ast::PostfixExpr, &ast::PostfixExpr::operand>(
                       assign<&ast::PostfixExpr::op>(lit<"++"> |
                                                     lit<"--">)))> {};

  static constexpr auto MonadicOp =
      lit<"++"> | lit<"--"> | lit<"+"> | lit<"-"> | lit<"!"> | lit<"~"> |
      kw<"ref"> | lit<"*"> | lit<"<-"> | kw<"hd"> | kw<"tl"> | kw<"len"> |
      kw<"tagof">;

  struct InitElem
      : rule<create<ast::InitElem>(
                 flag<&ast::InitElem::wildcardKey>(lit<"*">) +
                 lit<"=>"> + assign<&ast::InitElem::b>(ref<Expression>))  //
             | create<ast::InitElem>(
                   assign<&ast::InitElem::a>(ref<Expression>) +
                   option(lit<"=>"> +
                          assign<&ast::InitElem::b>(ref<Expression>)))> {};

  struct CastExpr
      : rule<create<ast::CastExpr>(
            assign<&ast::CastExpr::type>(ref<CastLeadType>) +
            assign<&ast::CastExpr::operand>(ref<MonadicExpr>))> {};

  struct MonadicExpr
      : rule<ref<Term>  //
             | create<ast::PrefixExpr>(
                   assign<&ast::PrefixExpr::op>(MonadicOp) +
                   assign<&ast::PrefixExpr::operand>(ref<MonadicExpr>))  //
             | create<ast::ArrayExpr>(
                   kw<"array"> + lit<"["> +
                   option(assign<&ast::ArrayExpr::size>(ref<Expression>)) +
                   lit<"]"> + kw<"of"> +
                   (assign<&ast::ArrayExpr::elemType>(ref<DataType>)  //
                    | (flag<&ast::ArrayExpr::hasInit>(lit<"{">) +
                       append<&ast::ArrayExpr::elements>(ref<InitElem>) +
                       many(lit<","> +
                            append<&ast::ArrayExpr::elements>(ref<InitElem>)) +
                       lit<"}">)))  //
             | create<ast::ListLit>(
                   kw<"list"> + kw<"of"> + lit<"{"> +
                   ExpressionListFor<&ast::ListLit::elements> + lit<"}">)  //
             | create<ast::ChanOfExpr>(
                   kw<"chan"> + kw<"of"> +
                   assign<&ast::ChanOfExpr::elemType>(ref<DataType>))  //
             | ref<CastExpr>> {};

  static constexpr auto OpMul = lit<"*"> | lit<"/"> | lit<"%">;
  static constexpr auto OpAdd = lit<"+"> | lit<"-">;
  static constexpr auto OpShift = lit<"<<"> | lit<">>">;
  static constexpr auto OpRel = lit<"<="> | lit<">="> | lit<"<"> | lit<">">;
  static constexpr auto OpEq = lit<"=="> | lit<"!=">;
  static constexpr auto OpBitAnd = lit<"&">;
  static constexpr auto OpBitXor = lit<"^">;
  static constexpr auto OpBitOr = lit<"|">;
  static constexpr auto OpLogAnd = lit<"&&">;
  static constexpr auto OpLogOr = lit<"||">;
  static constexpr auto OpCons = lit<"::">;

  struct BinaryLadder
      : infix<MonadicExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(OpMul) >> left(OpAdd) >> left(OpShift) >> left(OpRel) >>
               left(OpEq) >> left(OpBitAnd) >> left(OpBitXor) >>
               left(OpBitOr) >> right(OpCons) >> left(OpLogAnd) >>
               left(OpLogOr))> {};

  static constexpr auto AssignOp =
      lit<"<<="> | lit<">>="> | lit<"+="> | lit<"-="> | lit<"*="> |
      lit<"/="> | lit<"%="> | lit<"&="> | lit<"^="> | lit<"|="> | Assign;

  struct Expression
      : rule<create<ast::AssignExpr>(
                 assign<&ast::AssignExpr::target>(ref<MonadicExpr>) +
                 assign<&ast::AssignExpr::op>(AssignOp) +
                 assign<&ast::AssignExpr::value>(ref<Expression>))  //
             | create<ast::SendExpr>(
                   assign<&ast::SendExpr::target>(ref<MonadicExpr>) +
                   lit<"<-"> + Assign +
                   assign<&ast::SendExpr::value>(ref<Expression>))  //
             | create<ast::DeclareExpr>(
                   assign<&ast::DeclareExpr::target>(ref<MonadicExpr>) +
                   lit<":="> +
                   assign<&ast::DeclareExpr::value>(ref<Expression>))  //
             | create<ast::LoadExpr>(
                   kw<"load"> +
                   assign<&ast::LoadExpr::module_>(ref<ID>) +
                   assign<&ast::LoadExpr::path>(ref<Expression>))  //
             | ref<BinaryLadder>> {};

  // -- Declarations ----

  struct VarDecl
      : rule<create<ast::VarDecl>(
            IdentifierListFor<&ast::VarDecl::names> + Colon +
            assign<&ast::VarDecl::type>(ref<Type>) +
            option(Assign + assign<&ast::VarDecl::init>(ref<Expression>)) +
            lit<";">)> {};

  struct ConDecl
      : rule<create<ast::ConDecl>(
            IdentifierListFor<&ast::ConDecl::names> + Colon + kw<"con"> +
            assign<&ast::ConDecl::value>(ref<Expression>) + lit<";">)> {};

  struct ImportDecl
      : rule<create<ast::ImportDecl>(
            IdentifierListFor<&ast::ImportDecl::names> + Colon +
            kw<"import"> +
            assign<&ast::ImportDecl::moduleName>(ref<ID>) + lit<";">)> {};

  struct TypeDecl
      : rule<create<ast::TypeDecl>(
            IdentifierListFor<&ast::TypeDecl::names> + Colon + kw<"type"> +
            assign<&ast::TypeDecl::type>(ref<Type>) + lit<";">)> {};

  struct IncludeDecl
      : rule<create<ast::IncludeDecl>(
            kw<"include"> + assign<&ast::IncludeDecl::path>(ref<STRING>) +
            lit<";">)> {};

  struct Declaration
      : rule<ref<VarDecl> | ref<ConDecl> | ref<ImportDecl> | ref<TypeDecl> |
            ref<IncludeDecl>> {};

  struct ShortVarDecl
      : rule<create<ast::ShortVarDecl>(
            IdentifierListFor<&ast::ShortVarDecl::names> + lit<":="> +
            assign<&ast::ShortVarDecl::value>(ref<Expression>) + lit<";">)> {
  };

  struct AssignDecl
      : rule<create<ast::AssignDecl>(
            IdentifierListFor<&ast::AssignDecl::names> + Assign +
            assign<&ast::AssignDecl::value>(ref<Expression>) + lit<";">)> {};

  struct TupleShortVarDecl
      : rule<create<ast::TupleShortVarDecl>(
            lit<"("> + IdentifierListFor<&ast::TupleShortVarDecl::names> +
            lit<")"> + lit<":="> +
            assign<&ast::TupleShortVarDecl::value>(ref<Expression>) +
            lit<";">)> {};

  struct AdtField
      : rule<create<ast::AdtField>(
            IdentifierListFor<&ast::AdtField::names> + Colon +
            option(flag<&ast::AdtField::cyclic>(kw<"cyclic">)) +
            assign<&ast::AdtField::type>(ref<DataType>) +
            lit<";">)> {};

  struct PickTagArm
      : rule<create<ast::PickTagArm>(
            append<&ast::PickTagArm::tags>(ref<ID>) +
            many(kw<"or"> + append<&ast::PickTagArm::tags>(ref<ID>)) +
            lit<"=>"> +
            many(append<&ast::PickTagArm::fields>(ref<AdtField>)))> {};

  struct PickDecl
      : rule<create<ast::PickDecl>(
            kw<"pick"> + lit<"{"> +
            some(append<&ast::PickDecl::arms>(ref<PickTagArm>)) + lit<"}"> +
            lit<";">)> {};

  struct AdtMember
      : rule<ref<AdtField> | ref<ConDecl> | ref<VarDecl> | ref<PickDecl>> {};

  struct AdtDecl
      : rule<create<ast::AdtDecl>(
            assign<&ast::AdtDecl::name>(ref<ID>) + Colon + kw<"adt"> +
            lit<"{"> + many(append<&ast::AdtDecl::members>(ref<AdtMember>)) +
            lit<"}"> + lit<";">)> {};

  struct ModMember
      : rule<ref<VarDecl> | ref<ConDecl> | ref<TypeDecl> | ref<AdtDecl>> {};

  struct ModuleDecl
      : rule<create<ast::ModuleDecl>(
            assign<&ast::ModuleDecl::name>(ref<ID>) + Colon + kw<"module"> +
            lit<"{"> +
            many(append<&ast::ModuleDecl::members>(ref<ModMember>)) +
            lit<"}"> + lit<";">)> {};

  // -- Statements ----

  struct ExprStmt
      : rule<create<ast::ExprStmt>(
            option(assign<&ast::ExprStmt::value>(ref<Expression>)) +
            lit<";">)> {};

  struct Block
      : rule<lit<"{"> + many(append<&ast::Block::stmts>(ref<Item>)) +
            lit<"}">> {};

  struct IfStmt
      : rule<create<ast::IfStmt>(
            kw<"if"> + lit<"("> + assign<&ast::IfStmt::cond>(ref<Expression>) +
            lit<")"> + assign<&ast::IfStmt::thenBranch>(ref<Statement>) +
            option(kw<"else"> +
                   assign<&ast::IfStmt::elseBranch>(ref<Statement>)))> {};

  struct WhileStmt
      : rule<create<ast::WhileStmt>(
            option(assign<&ast::WhileStmt::label>(ref<ID>) + Colon) +
            kw<"while"> + lit<"("> +
            option(assign<&ast::WhileStmt::cond>(ref<Expression>)) +
            lit<")"> + assign<&ast::WhileStmt::body>(ref<Statement>))> {};

  struct DoWhileStmt
      : rule<create<ast::DoWhileStmt>(
            option(assign<&ast::DoWhileStmt::label>(ref<ID>) + Colon) +
            kw<"do"> + assign<&ast::DoWhileStmt::body>(ref<Statement>) +
            kw<"while"> + lit<"("> +
            option(assign<&ast::DoWhileStmt::cond>(ref<Expression>)) +
            lit<")"> + lit<";">)> {};

  struct ForStmt
      : rule<create<ast::ForStmt>(
            option(assign<&ast::ForStmt::label>(ref<ID>) + Colon) +
            kw<"for"> + lit<"("> +
            option(assign<&ast::ForStmt::init>(ref<Expression>)) + lit<";"> +
            option(assign<&ast::ForStmt::cond>(ref<Expression>)) + lit<";"> +
            option(assign<&ast::ForStmt::post>(ref<Expression>)) + lit<")"> +
            assign<&ast::ForStmt::body>(ref<Statement>))> {};

  struct Qualifier
      : rule<create<ast::Qualifier>(
                 flag<&ast::Qualifier::wildcard>(lit<"*">))  //
             | create<ast::Qualifier>(
                   assign<&ast::Qualifier::a>(ref<Expression>) +
                   option(kw<"to"> +
                          assign<&ast::Qualifier::b>(ref<Expression>)))> {};

  struct CaseArm
      : rule<create<ast::CaseArm>(
            append<&ast::CaseArm::labels>(ref<Qualifier>) +
            many(kw<"or"> + append<&ast::CaseArm::labels>(ref<Qualifier>)) +
            lit<"=>"> + many(append<&ast::CaseArm::body>(ref<Item>)))> {};

  struct CaseStmt
      : rule<create<ast::CaseStmt>(
            option(assign<&ast::CaseStmt::label>(ref<ID>) + Colon) +
            kw<"case"> + assign<&ast::CaseStmt::value>(ref<Expression>) +
            lit<"{"> + some(append<&ast::CaseStmt::arms>(ref<CaseArm>)) +
            lit<"}">)> {};

  struct AltStmt
      : rule<create<ast::AltStmt>(
            option(assign<&ast::AltStmt::label>(ref<ID>) + Colon) +
            kw<"alt"> + lit<"{"> +
            some(append<&ast::AltStmt::arms>(ref<CaseArm>)) + lit<"}">)> {};

  struct PickQualRule
      : rule<create<ast::PickQual>(
                 flag<&ast::PickQual::wildcard>(lit<"*">))  //
             | create<ast::PickQual>(assign<&ast::PickQual::tag>(ref<ID>))> {
  };

  struct PickArm
      : rule<create<ast::PickArm>(
            append<&ast::PickArm::tags>(ref<PickQualRule>) +
            many(kw<"or"> + append<&ast::PickArm::tags>(ref<PickQualRule>)) +
            lit<"=>"> + many(append<&ast::PickArm::body>(ref<Item>)))> {};

  struct PickStmt
      : rule<create<ast::PickStmt>(
            option(assign<&ast::PickStmt::label>(ref<ID>) + Colon) +
            kw<"pick"> + assign<&ast::PickStmt::varName>(ref<ID>) +
            lit<":="> + assign<&ast::PickStmt::value>(ref<Expression>) +
            lit<"{"> + some(append<&ast::PickStmt::arms>(ref<PickArm>)) +
            lit<"}">)> {};

  struct BreakStmt
      : rule<create<ast::BreakStmt>(
            kw<"break"> + option(assign<&ast::BreakStmt::label>(ref<ID>)) +
            lit<";">)> {};

  struct ContinueStmt
      : rule<create<ast::ContinueStmt>(
            kw<"continue"> +
            option(assign<&ast::ContinueStmt::label>(ref<ID>)) + lit<";">)> {
  };

  struct ReturnStmt
      : rule<create<ast::ReturnStmt>(
            kw<"return"> + option(assign<&ast::ReturnStmt::value>(
                               ref<Expression>)) +
            lit<";">)> {};

  struct SpawnStmt
      : rule<create<ast::SpawnStmt>(
            kw<"spawn"> + assign<&ast::SpawnStmt::callee>(ref<Term>) +
            lit<"("> + option(ExpressionListFor<&ast::SpawnStmt::args>) +
            lit<")"> + lit<";">)> {};

  struct ExitStmt : rule<create<ast::ExitStmt>(kw<"exit"> + lit<";">)> {};

  struct Statement
      : rule<ref<Block> | ref<IfStmt> | ref<WhileStmt> | ref<DoWhileStmt> |
            ref<ForStmt> | ref<CaseStmt> | ref<AltStmt> | ref<PickStmt> |
            ref<BreakStmt> | ref<ContinueStmt> | ref<ReturnStmt> |
            ref<SpawnStmt> | ref<ExitStmt> | ref<ExprStmt>> {};

  struct Item : rule<ref<Declaration> | ref<Statement>> {};

  // -- Top level ----

  struct FunctionDef
      : rule<create<ast::FunctionDef>(
            append<&ast::FunctionDef::nameParts>(ref<ID>) +
            many(lit<"."> + append<&ast::FunctionDef::nameParts>(ref<ID>)) +
            assign<&ast::FunctionDef::signature>(ref<Signature>) + lit<"{"> +
            many(append<&ast::FunctionDef::body>(ref<Item>)) + lit<"}">)> {};

  struct TopDecl
      : rule<ref<Declaration> | ref<ShortVarDecl> | ref<AssignDecl> |
            ref<TupleShortVarDecl> | ref<ModuleDecl> | ref<FunctionDef> |
            ref<AdtDecl>> {};

  struct Program
      : rule<create<ast::Program>(
            kw<"implement"> + assign<&ast::Program::name>(ref<ID>) +
            lit<";"> + some(append<&ast::Program::decls>(ref<TopDecl>)))> {};

  using entry = Program;
  using ignore = list<WS, COMMENT>;
};

}  // namespace limbo
