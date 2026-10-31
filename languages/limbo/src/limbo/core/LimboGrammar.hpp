// The Limbo language: its grammar. Basic types are keyword tokens,
// expression lists are comma-separated, and binary operators follow the
// language manual's precedence table.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "limbo/core/ast.hpp"

namespace limbo {

struct Limbo : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};
  static constexpr auto identifierHead = cr<"a-zA-Z_\u00a1-\U0010ffff">;
  static constexpr auto word_boundary = identifierHead | d;
  static constexpr auto reserved = kw<"adt"> | kw<"alt"> | kw<"array"> | kw<"big"> |
      kw<"break"> | kw<"byte"> | kw<"case"> | kw<"chan"> | kw<"con"> | kw<"continue"> |
      kw<"cyclic"> | kw<"do"> | kw<"dynamic"> | kw<"else"> | kw<"exception"> | kw<"exit"> |
      kw<"fixed"> | kw<"fn"> | kw<"for"> | kw<"hd"> | kw<"if"> | kw<"implement"> |
      kw<"import"> | kw<"include"> | kw<"int"> | kw<"len"> | kw<"list"> | kw<"load"> |
      kw<"module"> | kw<"nil"> | kw<"of"> | kw<"or"> | kw<"pick"> | kw<"raise"> |
      kw<"raises"> | kw<"real"> | kw<"ref"> | kw<"return"> | kw<"self"> | kw<"spawn"> |
      kw<"string"> | kw<"tagof"> | kw<"tl"> | kw<"to"> | kw<"type"> | kw<"while">;
  struct ID : terminal<!reserved + identifierHead + many(word_boundary)> {};
  static constexpr auto basedDigits = cr<"0-9A-Za-z_">;
  struct INT : terminal<(some(d) + cr<"rR"> + some(basedDigits)) | some(d)> {};
  static constexpr auto exponent = cr<"eE"> + option(cr<"+-">) + some(d);
  struct REAL : terminal<
      (some(d) + cr<"rR"> + many(basedDigits) + lit<"."> + many(basedDigits)) |
      (((some(d) + lit<"."> + many(d)) | (lit<"."> + some(d))) + option(exponent)) |
      (some(d) + exponent)> {};
  static constexpr auto escape = lit<"\\"> +
      (cr<R"(\\'"abtnvfr0)"> | (lit<"u"> + repeat<4>(cr<"0-9A-Fa-f">)));
  struct CHAR : terminal<lit<"'"> + (escape | cr<R"(^'\\)">) + lit<"'">> {};
  struct STRING : terminal<(lit<"\""> + many(escape | cr<R"(^"\\\n)">) + lit<"\"">) |
                           through(lit<"`">, lit<"`">)> {};

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
  struct FixedType;
  struct NamedType;
  struct Signature;
  struct FormalArg;
  struct VarDecl;
  struct ConDecl;
  struct TypeDecl;
  struct AdtDecl;
  struct AdtField;
  struct GenericParameters;
  struct ConstraintArm;
  struct RaiseSpec;
  struct Qualifier;
  struct Export;
  struct ExportSequence;
  struct TypeArgument;

  template <auto Ptr>
  static constexpr auto IdentifierListFor =
      some(append<Ptr>(ref<ID>), lit<",">);
  template <auto Ptr>
  static constexpr auto ExpressionListFor =
      some(append<Ptr>(ref<Expression>), lit<",">);
  static constexpr auto NilOrD = ref<ID> | kw<"nil">;
  template <auto First, auto Rest> static constexpr auto LabelFor = option(
      assign<First>(ref<ID>) + many(lit<","> + append<Rest>(ref<ID>)) + Colon);

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
            option(lit<"->"> + assign<&ast::NamedType::id2>(ref<ID>)) +
            many(nest<ast::AppliedType, &ast::AppliedType::base>(lit<"["> +
                     some(append<&ast::AppliedType::arguments>(ref<TypeArgument>), lit<",">) + lit<"]">) |
                 nest<ast::MemberType, &ast::MemberType::base>(
                     assign<&ast::MemberType::separator>(lit<"."> | lit<"->">) +
                     assign<&ast::MemberType::member>(ref<ID>))))> {};

  struct RefType
      : rule<create<ast::RefType>(kw<"ref"> +
                                  assign<&ast::RefType::target>(ref<Type>))> {
  };

  struct TupleType
      : rule<create<ast::TupleType>(
            lit<"("> + some(append<&ast::TupleType::elements>(ref<Type>), lit<",">) +
            lit<")">)> {};

  struct ArrayOfType
      : rule<create<ast::ArrayOfType>(
            kw<"array"> + kw<"of"> +
            assign<&ast::ArrayOfType::element>(ref<Type>))> {};

  struct ListOfType
      : rule<create<ast::ListOfType>(
            kw<"list"> + kw<"of"> +
            assign<&ast::ListOfType::element>(ref<Type>))> {};

  struct ChanOfType
      : rule<create<ast::ChanOfType>(
            kw<"chan"> + kw<"of"> +
            assign<&ast::ChanOfType::element>(ref<Type>))> {};

  struct DataType
      : rule<ref<PrimType> | ref<TupleType> | ref<ArrayOfType> |
            ref<ListOfType> | ref<ChanOfType> | ref<RefType> | ref<NamedType> | ref<FixedType>> {
  };

  struct TypeArgument : rule<create<ast::CyclicType>(kw<"cyclic"> +
      assign<&ast::CyclicType::base>(ref<Type>)) | ref<Type>> {};
  struct FnType : rule<create<ast::FnType>(kw<"fn"> +
      option(assign<&ast::FnType::parameters>(ref<GenericParameters>)) +
      assign<&ast::FnType::signature>(ref<Signature>) +
      option(assign<&ast::FnType::raises>(ref<RaiseSpec>)))> {};

  struct Type : rule<ref<DataType> | ref<FnType>> {};

  struct FixedType : rule<create<ast::FixedType>(kw<"fixed"> + lit<"("> +
      append<&ast::FixedType::parameters>(ref<Expression>) +
      option(lit<","> + append<&ast::FixedType::parameters>(ref<Expression>)) + lit<")">)> {};
  struct CastNamedType : rule<assign<&ast::NamedType::id1>(ref<ID>) +
      !(lit<"+"> | lit<"-"> | lit<"*"> | lit<"<-"> | lit<"++"> | lit<"--"> | lit<"(">)> {};
  struct CastArrayType : rule<kw<"array"> + kw<"of"> +
      assign<&ast::ArrayOfType::element>(ref<PrimType>)> {};
  struct CastLeadType : rule<ref<PrimType> | ref<CastNamedType> |
                             ref<CastArrayType> | ref<FixedType>> {};

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
                       assign<&ast::FormalArg::selfTypeExpression>(ref<NamedType>))))> {};

  struct GenericParameters : rule<lit<"["> +
      IdentifierListFor<&ast::GenericParameters::names> + lit<"]">> {};
  struct ConstraintArm : rule<IdentifierListFor<&ast::ConstraintArm::names> + lit<"=>"> +
      many(append<&ast::ConstraintArm::fields>(ref<AdtField>))> {};
  template <auto Ptr> static constexpr auto ConstraintsFor =
      kw<"for"> + lit<"{"> + some(append<Ptr>(ref<ConstraintArm>)) + lit<"}">;

  struct Signature
      : rule<create<ast::Signature>(
            lit<"("> +
            option(some(append<&ast::Signature::params>(ref<FormalArg>), lit<",">)) +
            lit<")"> +
            option(Colon + assign<&ast::Signature::result>(ref<Type>)) +
            option(ConstraintsFor<&ast::Signature::constraints>))> {};

  // -- Expressions ----

  struct Identifier : rule<assign<&ast::Ident::name>(ref<ID>)> {};
  struct RaiseTarget : rule<ref<Identifier> | create<ast::NilExpr>(kw<"nil">) |
      create<ast::ParenExpr>(lit<"("> +
          some(append<&ast::ParenExpr::items>(ref<RaiseTarget>), lit<",">) + lit<")">)> {};
  struct RaiseSpec : rule<kw<"raises"> +
      ((flag<&ast::RaiseSpec::parenthesized>(lit<"(">) +
        some(append<&ast::RaiseSpec::exceptions>(ref<RaiseTarget>), lit<",">) + lit<")">) |
       append<&ast::RaiseSpec::exceptions>(ref<Identifier> | create<ast::NilExpr>(kw<"nil">)))> {};
  struct IntLiteral : rule<assign<&ast::IntLit::text>(ref<INT> | ref<CHAR>)> {};
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
                       ((flag<&ast::IndexExpr::isSlice>(lit<":">) +
                         option(assign<&ast::IndexExpr::high>(ref<Expression>))) |
                        (assign<&ast::IndexExpr::index>(ref<Expression>) +
                         flag<&ast::IndexExpr::isSlice>(lit<":">) +
                         option(assign<&ast::IndexExpr::high>(ref<Expression>))) |
                        assign<&ast::IndexExpr::index>(ref<ExportSequence> | ref<Export>)) +
                       lit<"]">)  //
                 | nest<ast::PostfixExpr, &ast::PostfixExpr::operand>(
                       assign<&ast::PostfixExpr::op>(lit<"++"> |
                                                     lit<"--">)))> {};

  struct ExportType : rule<create<ast::TypeExpr>(assign<&ast::TypeExpr::type>(
      ref<PrimType> | ref<ArrayOfType> | ref<ListOfType> |
      create<ast::CyclicType>(kw<"cyclic"> + assign<&ast::CyclicType::base>(ref<Type>))))> {};
  struct Export : rule<ref<Expression> | ref<ExportType>> {};
  struct ExportSequence : rule<create<ast::ExportSequence>(
      append<&ast::ExportSequence::values>(ref<Export>) +
      some(lit<","> + append<&ast::ExportSequence::values>(ref<Export>)))> {};

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
                          assign<&ast::InitElem::b>(ref<Expression>))) |
             create<ast::InitElem>(append<&ast::InitElem::keys>(ref<Qualifier>) +
                 many(kw<"or"> + append<&ast::InitElem::keys>(ref<Qualifier>)) +
                 lit<"=>"> + assign<&ast::InitElem::b>(ref<Expression>))> {};

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
                   (assign<&ast::ArrayExpr::elemType>(ref<Type>)  //
                    | (flag<&ast::ArrayExpr::hasInit>(lit<"{">) +
                       some(append<&ast::ArrayExpr::elements>(ref<InitElem>), lit<",">) +
                       option(lit<",">) + lit<"}">)))  //
             | create<ast::ListLit>(
                   kw<"list"> + kw<"of"> + lit<"{"> +
                   ExpressionListFor<&ast::ListLit::elements> + option(lit<",">) + lit<"}">)  //
             | create<ast::ChanOfExpr>(
                   kw<"chan"> + option(lit<"["> + assign<&ast::ChanOfExpr::size>(ref<Expression>) + lit<"]">) +
                   kw<"of"> + assign<&ast::ChanOfExpr::elemType>(ref<Type>))  //
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

  struct SendExpression : rule<create<ast::SendExpr>(
      assign<&ast::SendExpr::target>(ref<MonadicExpr>) + lit<"<-"> +
      assign<&ast::SendExpr::value>(ref<SendExpression>)) | ref<MonadicExpr>> {};
  struct BinaryLadder
      : infix<SendExpression, ast::BinaryExpr,
              levels(right(lit<"**">), left(OpMul), left(OpAdd), left(OpShift), left(OpRel),
                     left(OpEq), left(OpBitAnd), left(OpBitXor), left(OpBitOr),
                     right(OpCons), left(OpLogAnd), left(OpLogOr))> {};

  static constexpr auto AssignOp =
      lit<"<<="> | lit<">>="> | lit<"+="> | lit<"-="> | lit<"*="> |
      lit<"**="> | lit<"/="> | lit<"%="> | lit<"&="> | lit<"^="> | lit<"|="> | Assign;

  struct Expression
      : rule<create<ast::AssignExpr>(
                 assign<&ast::AssignExpr::target>(ref<MonadicExpr>) +
                 assign<&ast::AssignExpr::op>(AssignOp) +
                 assign<&ast::AssignExpr::value>(ref<Expression>))  //
             | create<ast::SendExpr>(
                   assign<&ast::SendExpr::target>(ref<MonadicExpr>) +
                   lit<"<-"> + flag<&ast::SendExpr::equals>(Assign) +
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

  struct ExceptionDecl : rule<IdentifierListFor<&ast::ExceptionDecl::names> +
      Colon + kw<"exception"> + option(lit<"("> +
          some(append<&ast::ExceptionDecl::parameters>(ref<Type>), lit<",">) + lit<")">) + lit<";">> {};

  struct ImportDecl
      : rule<create<ast::ImportDecl>(
            IdentifierListFor<&ast::ImportDecl::names> + Colon +
            kw<"import"> +
            assign<&ast::ImportDecl::moduleExpression>(ref<Expression>) + lit<";">)> {};

  struct TypeDecl
      : rule<create<ast::TypeDecl>(
            IdentifierListFor<&ast::TypeDecl::names> + Colon + kw<"type"> +
            assign<&ast::TypeDecl::type>(ref<Type>) + lit<";">)> {};

  struct IncludeDecl
      : rule<create<ast::IncludeDecl>(
            kw<"include"> + assign<&ast::IncludeDecl::path>(ref<STRING>) +
            lit<";">)> {};

  struct Declaration
      : rule<ref<ExceptionDecl> | ref<VarDecl> | ref<ConDecl> | ref<ImportDecl> | ref<TypeDecl> |
            ref<IncludeDecl>> {};

  struct TopAssignment : rule<assign<&ast::TopAssignment::target>(ref<RaiseTarget>) +
      assign<&ast::TopAssignment::op>(lit<":="> | Assign) +
      assign<&ast::TopAssignment::value>(ref<Expression>) + lit<";">> {};

  struct AdtField
      : rule<create<ast::AdtField>(
            IdentifierListFor<&ast::AdtField::names> + Colon +
            option(flag<&ast::AdtField::cyclic>(kw<"cyclic">)) +
            assign<&ast::AdtField::type>(ref<Type>) +
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
            some(append<&ast::PickDecl::arms>(ref<PickTagArm>)) + lit<"}">)> {};

  struct AdtMember
      : rule<ref<AdtField> | ref<ConDecl> | ref<PickDecl>> {};

  static constexpr auto AdtBody = lit<"{"> +
      many(append<&ast::AdtDecl::members>(ref<AdtMember>)) + lit<"}">;
  struct AdtDecl : rule<create<ast::AdtDecl>(
      assign<&ast::AdtDecl::name>(ref<ID>) +
      many(lit<","> + append<&ast::AdtDecl::additionalNames>(ref<ID>)) + Colon + kw<"adt"> +
      option(assign<&ast::AdtDecl::parameters>(ref<GenericParameters>)) +
      ((ConstraintsFor<&ast::AdtDecl::constraints> + AdtBody) |
       (AdtBody + option(ConstraintsFor<&ast::AdtDecl::constraints>))) + lit<";">)> {};

  struct ModMember
      : rule<ref<ExceptionDecl> | ref<VarDecl> | ref<ConDecl> | ref<TypeDecl> | ref<AdtDecl>> {};

  struct ModuleDecl
      : rule<create<ast::ModuleDecl>(
            assign<&ast::ModuleDecl::name>(ref<ID>) +
            many(lit<","> + append<&ast::ModuleDecl::additionalNames>(ref<ID>)) + Colon + kw<"module"> +
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
            LabelFor<&ast::WhileStmt::label, &ast::WhileStmt::additionalLabels> +
            kw<"while"> + lit<"("> +
            option(assign<&ast::WhileStmt::cond>(ref<Expression>)) +
            lit<")"> + assign<&ast::WhileStmt::body>(ref<Statement>))> {};

  struct DoWhileStmt
      : rule<create<ast::DoWhileStmt>(
            LabelFor<&ast::DoWhileStmt::label, &ast::DoWhileStmt::additionalLabels> +
            kw<"do"> + assign<&ast::DoWhileStmt::body>(ref<Statement>) +
            kw<"while"> + lit<"("> +
            option(assign<&ast::DoWhileStmt::cond>(ref<Expression>)) +
            lit<")"> + lit<";">)> {};

  struct ForStmt
      : rule<create<ast::ForStmt>(
            LabelFor<&ast::ForStmt::label, &ast::ForStmt::additionalLabels> +
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
            LabelFor<&ast::CaseStmt::label, &ast::CaseStmt::additionalLabels> +
            kw<"case"> + assign<&ast::CaseStmt::value>(ref<Expression>) +
            lit<"{"> + some(append<&ast::CaseStmt::arms>(ref<CaseArm>)) +
            lit<"}">)> {};

  struct AltStmt
      : rule<create<ast::AltStmt>(
            LabelFor<&ast::AltStmt::label, &ast::AltStmt::additionalLabels> +
            kw<"alt"> + lit<"{"> +
            some(append<&ast::AltStmt::arms>(ref<CaseArm>)) + lit<"}">)> {};

  struct PickQual
      : rule<create<ast::PickQual>(
                 flag<&ast::PickQual::wildcard>(lit<"*">))  //
             | create<ast::PickQual>(assign<&ast::PickQual::tag>(ref<ID>))> {
  };

  struct PickArm
      : rule<create<ast::PickArm>(
            append<&ast::PickArm::tags>(ref<PickQual>) +
            many(kw<"or"> + append<&ast::PickArm::tags>(ref<PickQual>)) +
            lit<"=>"> + many(append<&ast::PickArm::body>(ref<Item>)))> {};

  struct PickStmt
      : rule<create<ast::PickStmt>(
            LabelFor<&ast::PickStmt::label, &ast::PickStmt::additionalLabels> +
            kw<"pick"> + assign<&ast::PickStmt::varName>(ref<ID>) +
            lit<":="> + assign<&ast::PickStmt::value>(ref<Expression>) +
            lit<"{"> + some(append<&ast::PickStmt::arms>(ref<PickArm>)) +
            lit<"}">)> {};

  struct TryStmt : rule<assign<&ast::TryStmt::body>(ref<Block>) + kw<"exception"> +
      option(assign<&ast::TryStmt::variable>(ref<ID>)) + lit<"{"> +
      some(append<&ast::TryStmt::handlers>(ref<CaseArm>)) + lit<"}">> {};
  struct RaiseStmt : rule<create<ast::RaiseStmt>(kw<"raise"> +
      option(assign<&ast::RaiseStmt::value>(ref<Expression>)) + lit<";">)> {};

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

  struct SpawnStmt : rule<create<ast::SpawnStmt>(kw<"spawn"> +
      assign<&ast::SpawnStmt::expression>(ref<Expression>) + lit<";">)> {};

  struct ExitStmt : rule<create<ast::ExitStmt>(kw<"exit"> + lit<";">)> {};

  struct Statement
      : rule<ref<TryStmt> | ref<RaiseStmt> | ref<Block> | ref<IfStmt> | ref<WhileStmt> | ref<DoWhileStmt> |
            ref<ForStmt> | ref<CaseStmt> | ref<AltStmt> | ref<PickStmt> |
            ref<BreakStmt> | ref<ContinueStmt> | ref<ReturnStmt> |
            ref<SpawnStmt> | ref<ExitStmt> | ref<ExprStmt>> {};

  struct Item : rule<ref<Declaration> | ref<Statement>> {};

  // -- Top level ----

  struct FunctionDef
      : rule<create<ast::FunctionDef>(
            some(append<&ast::FunctionDef::nameParts>(ref<ID>) +
                 option(append<&ast::FunctionDef::typeParameters>(ref<GenericParameters>)), lit<".">) +
            assign<&ast::FunctionDef::signature>(ref<Signature>) +
            option(assign<&ast::FunctionDef::raises>(ref<RaiseSpec>)) + lit<"{"> +
            many(append<&ast::FunctionDef::body>(ref<Item>)) + lit<"}">)> {};

  struct TopDecl
      : rule<ref<Declaration> | ref<TopAssignment> | ref<ModuleDecl> | ref<FunctionDef> |
            ref<AdtDecl>> {};

  struct Program
      : rule<create<ast::Program>(
            option(kw<"implement"> + assign<&ast::Program::name>(ref<ID>) +
                   many(lit<","> + append<&ast::Program::additionalImplementations>(ref<ID>)) + lit<";">) + some(append<&ast::Program::decls>(ref<TopDecl>)))> {};

  using entry = Program;
  using ignored = list<WS, COMMENT>;
};

}  // namespace limbo
