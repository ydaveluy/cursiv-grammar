// The argus language: its grammar.
#pragma once

#include "argus/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace argus {

struct Argus : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct COMMENT : terminal<lit<"%"> + many(cr<R"(^\r\n)">)> {};

  struct ID : terminal<cr<"a-zA-Z_"> + many(w)> {};
  struct INT_LITERAL
      : terminal<some(d) | (lit<"\\"> + some(cr<"0-7">)) |
                 (lit<"\\#"> + some(cr<"0-9a-fA-F">))> {};
  static constexpr auto Exponent = cr<"eE"> + option(cr<R"(+-)">) + some(d);
  struct REAL_LITERAL
      : terminal<(((some(d) + lit<"."> + many(d)) |
                    (lit<"."> + some(d))) + option(Exponent)) |
                 (some(d) + Exponent)> {};
  static constexpr auto Escape =
      lit<"\\"> +
      (cr<R"('"\\bBtTnNvVpPrR)"> | repeat<3>(cr<"0-7">) |
       (lit<"#"> + repeat<2>(cr<"0-9a-fA-F">)) |
       (cr<"^!&"> + cr<" -~">));
  struct CHAR_LITERAL
      : terminal<lit<"'"> + (Escape | cr<R"(^'\\\r\n)">) + lit<"'">> {};
  struct STRING_LITERAL
      : terminal<lit<"\""> + many(Escape | cr<R"(^"\\\r\n)">) + lit<"\"">> {};

  // Every binary/unary operator token, each denoting its own constant --
  // the ladder below names them, `op` holds what they mean. `−` (U+2212) is
  // the language's spelling for subtraction/negation, with ASCII `-` also
  // accepted.
  struct OP_SUB : terminal<as<ast::Operator::sub>((lit<"-"> | lit<"−">))> {};
  struct OP_NEG : terminal<as<ast::UnaryOp::neg>((lit<"-"> | lit<"−">))> {};

  struct Reference;
  struct ActualParm;
  struct TypeActual;
  struct OpBinding;
  struct FieldSpec;
  struct TypeSpec;
  struct Field;
  struct Primary;
  struct Entity;
  struct PostfixExpr;
  struct Unary;
  struct ExprLadder;
  struct CallAt;
  struct Expression;
  struct Call;
  struct BindArg;

  struct Decl;
  struct Parm;
  struct OperDecl;
  struct TypeSetElement;
  struct TypeSet;
  struct Restriction;
  struct Where;
  struct Equate;
  struct Exception;
  struct ReturnzClause;
  struct YieldsClause;
  struct SignalsClause;

  struct Body;
  struct OwnVar;
  struct StateDecl;

  struct Statement;
  struct BaseStatement;
  struct ForEachClause;
  struct CoArm;
  struct IfBranch;
  struct TagArm;
  struct ATagArm;
  struct WhenHandler;
  struct OthersHandler;

  struct Equates_;
  struct Procedure;
  struct Iterator;
  struct Creator;
  struct Handler;
  struct Guardian;
  struct Cluster;
  struct Module;

  // ---- expressions ----------------------------------------------------

  // Parametrization is bracketed -- see ast.hpp's comment on `ast::Reference`.
  struct Reference
      : rule<create<ast::Reference>(
            assign<&ast::Reference::name>(ref<ID>) +
            option(lit<"["> +
                   some(append<&ast::Reference::actualParms>(ref<ActualParm>),
                        lit<",">) +
                   lit<"]">) +
            many(lit<"$"> + append<&ast::Reference::selectors>(ref<ID>)))> {};

  static constexpr auto OpBindings =
      many(append<&ast::TypeActual::bindings>(ref<OpBinding>), lit<",">);

  struct TypeActual
      : rule<create<ast::TypeActual>(
            assign<&ast::TypeActual::type>(ref<TypeSpec>) +
            option(ikw<"with"> +
                   ((lit<"{"> + OpBindings + lit<"}">) | OpBindings)))> {};

  struct ActualParm
      : rule<create<ast::ActualParm>(
            assign<&ast::ActualParm::value>(ref<Expression> | ref<TypeActual>))> {
  };

  struct OpBinding
      : rule<create<ast::OpBinding>(
            some(append<&ast::OpBinding::names>(ref<ID>), lit<",">) + lit<":"> +
            assign<&ast::OpBinding::value>(ref<PostfixExpr>))> {};

  struct FieldSpec
      : rule<create<ast::FieldSpec>(
            some(append<&ast::FieldSpec::names>(ref<ID>), lit<",">) + lit<":"> +
            assign<&ast::FieldSpec::type>(ref<TypeActual>))> {};

  struct SimpleTypeSpec
      : rule<create<ast::SimpleType>(assign<&ast::SimpleType::kind>(
            as<ast::SimpleTypeKind::null_>(ikw<"null">) | as<ast::SimpleTypeKind::node>(ikw<"node">) | as<ast::SimpleTypeKind::bool_>(ikw<"bool">) | as<ast::SimpleTypeKind::int_>(ikw<"int">) |
            as<ast::SimpleTypeKind::real>(ikw<"real">) | as<ast::SimpleTypeKind::char_>(ikw<"char">) | as<ast::SimpleTypeKind::string>(ikw<"string">) | as<ast::SimpleTypeKind::any>(ikw<"any">) |
            as<ast::SimpleTypeKind::image>(ikw<"image">) | as<ast::SimpleTypeKind::rep>(ikw<"rep">) | as<ast::SimpleTypeKind::cvt>(ikw<"cvt">)))> {};

  struct ContainerTypeSpec
      : rule<create<ast::ContainerType>(
            assign<&ast::ContainerType::kind>(
                as<ast::ContainerTypeKind::sequence>(ikw<"sequence">) | as<ast::ContainerTypeKind::array>(ikw<"array">) | as<ast::ContainerTypeKind::atomic_array>(ikw<"atomic_array"> | (ikw<"atomic"> + ikw<"array">)) |
                as<ast::ContainerTypeKind::mutex>(ikw<"mutex">)) +
            lit<"["> + assign<&ast::ContainerType::element>(ref<TypeActual>) +
            lit<"]">)> {};

  struct FieldedTypeSpec
      : rule<create<ast::FieldedType>(
            assign<&ast::FieldedType::kind>(
                as<ast::FieldedTypeKind::struct_>(ikw<"struct">) | as<ast::FieldedTypeKind::record>(ikw<"record">) | as<ast::FieldedTypeKind::atomic_record>(ikw<"atomic_record"> | (ikw<"atomic"> + ikw<"record">)) |
                as<ast::FieldedTypeKind::oneof>(ikw<"oneof">) | as<ast::FieldedTypeKind::variant>(ikw<"variant">) | as<ast::FieldedTypeKind::atomic_variant>(ikw<"atomic_variant"> | (ikw<"atomic"> + ikw<"variant">))) +
            lit<"["> +
            some(append<&ast::FieldedType::fields>(ref<FieldSpec>), lit<",">) +
            lit<"]">)> {};

  struct RoutineTypeSpec
      : rule<create<ast::RoutineType>(
            assign<&ast::RoutineType::kind>(
                as<ast::RoutineTypeKind::proctype>(ikw<"proctype">) | as<ast::RoutineTypeKind::itertype>(ikw<"itertype">) | as<ast::RoutineTypeKind::creatortype>(ikw<"creatortype">) |
                as<ast::RoutineTypeKind::handlertype>(ikw<"handlertype">)) +
            option(lit<"("> +
                   many(append<&ast::RoutineType::params>(ref<TypeSpec>),
                        lit<",">) +
                   lit<")">) +
            option(assign<&ast::RoutineType::returns>(ref<ReturnzClause>)) +
            option(assign<&ast::RoutineType::yields>(ref<YieldsClause>)) +
            option(assign<&ast::RoutineType::signals>(ref<SignalsClause>)))> {};

  struct ReferenceTypeSpec
      : rule<create<ast::ReferenceType>(
            assign<&ast::ReferenceType::ref>(ref<Reference>))> {};

  struct TypeSpec : rule<ref<SimpleTypeSpec>      //
                             | ref<ContainerTypeSpec>  //
                             | ref<FieldedTypeSpec>    //
                             | ref<RoutineTypeSpec>    //
                             | ref<ReferenceTypeSpec>> {};

  struct Field
      : rule<create<ast::Field>(
            some(append<&ast::Field::names>(ref<ID>), lit<",">) + lit<":"> +
            assign<&ast::Field::value>(ref<Expression>))> {};

  // The argument list is bracketed here too -- see ast.hpp's comment on
  // `ast::DollarExpr`.
  struct BindArg
      : rule<create<ast::BindArg>(
            flag<&ast::BindArg::star>(lit<"*">) |
            assign<&ast::BindArg::expression>(ref<Expression>))> {};

  struct DollarForm
      : rule<create<ast::DollarExpr>(
            assign<&ast::DollarExpr::type>(ref<TypeSpec>) + lit<"$"> +
            (
                (assign<&ast::DollarExpr::name>(ref<ID>) +
                 option((lit<"["> +
                         some(append<&ast::DollarExpr::actualParms>(ref<ActualParm>),
                              lit<",">) + lit<"]">) |
                        (lit<"("> +
                         some(append<&ast::DollarExpr::actualParms>(ref<ActualParm>),
                              lit<",">) + lit<")">)))  //
                | (lit<"{"> +
                   many(append<&ast::DollarExpr::fields>(ref<Field>),
                        lit<",">) +
                   lit<"}">)  //
                | (lit<"["> +
                   option(assign<&ast::DollarExpr::size>(ref<Expression>) +
                          lit<":">) +
                   many(append<&ast::DollarExpr::items>(ref<Expression>),
                        lit<",">) +
                   lit<"]">)))> {};

  struct Primary
      : rule<(create<ast::NilLit> + ikw<"nil">)      //
             | (create<ast::TrueLit> + ikw<"true">)  //
             | (create<ast::FalseLit> + ikw<"false">)  //
             | (create<ast::SelfLit> + ikw<"self">)     //
             | create<ast::IntLit>(assign<&ast::IntLit::value>(ref<INT_LITERAL>))  //
             | create<ast::RealLit>(
                   assign<&ast::RealLit::value>(ref<REAL_LITERAL>))  //
             | create<ast::CharLit>(
                   assign<&ast::CharLit::value>(ref<CHAR_LITERAL>))  //
             | create<ast::StringLit>(
                   assign<&ast::StringLit::value>(ref<STRING_LITERAL>))  //
             | (lit<"("> +
                create<ast::ParenExpr>(
                    assign<&ast::ParenExpr::inner>(ref<Expression>)) +
                lit<")">)  //
             | create<ast::UpExpr>(ikw<"up"> + lit<"("> +
                                   assign<&ast::UpExpr::operand>(ref<Expression>) +
                                   lit<")">)  //
             | create<ast::DownExpr>(
                   ikw<"down"> + lit<"("> +
                   assign<&ast::DownExpr::operand>(ref<Expression>) + lit<")">)  //
             | create<ast::BindExpr>(
                   ikw<"bind"> + assign<&ast::BindExpr::target>(ref<Entity>) +
                   ((lit<"("> +
                     many(append<&ast::BindExpr::args>(ref<BindArg>), lit<",">) +
                     lit<")">) |
                    !lit<"(">))  // bare bind remains a permissive extension
             | ref<DollarForm>        //
             | ref<Reference>> {};

  static constexpr auto SelectionStep =
      nest<ast::Selection, &ast::Selection::target>(
          lit<"."> + assign<&ast::Selection::field>(ref<ID>));
  static constexpr auto IndexStep =
      nest<ast::IndexExpr, &ast::IndexExpr::target>(
          lit<"["> + assign<&ast::IndexExpr::index>(ref<Expression>) + lit<"]">);

  // A bind target is an entity; consuming a call here would turn
  // `bind f(1)` into a closure over the result of calling f.
  struct Entity : rule<ref<Primary> + many(SelectionStep | IndexStep)> {};

  struct PostfixExpr
      : rule<ref<Entity> +
             many(SelectionStep | IndexStep |
                  nest<ast::Call, &ast::Call::callee>(
                     lit<"("> +
                     many(append<&ast::Call::args>(ref<Expression>), lit<",">) +
                     lit<")">))> {};

  struct Unary
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(as<ast::UnaryOp::not_>(lit<"~">) | ref<OP_NEG>) +
                 assign<&ast::UnaryExpr::operand>(ref<Unary>))  //
             | ref<PostfixExpr>> {};

  struct ExprLadder
      : infix<Unary, ast::BinaryExpr,
              levels(right(as<ast::Operator::power>(lit<"**">)),                              //
                     left(as<ast::Operator::intdiv>(lit<"//">) | as<ast::Operator::div>(lit<"/">) | as<ast::Operator::mul>(lit<"*">)),  //
                     left(as<ast::Operator::add>(lit<"+">) | ref<OP_SUB> | as<ast::Operator::concat>(lit<"||">)),  //
                     left(as<ast::Operator::le>(lit<"<=">) | as<ast::Operator::lt>(lit<"<">) | as<ast::Operator::eq>(lit<"=">) |        //
                          as<ast::Operator::ge>(lit<">=">) | as<ast::Operator::gt>(lit<">">) | as<ast::Operator::approx_le>(lit<"~<=">) |       //
                          as<ast::Operator::approx_lt>(lit<"~<">) | as<ast::Operator::approx_eq>(lit<"~=">) | as<ast::Operator::approx_ge>(lit<"~>=">) |     //
                          as<ast::Operator::approx_gt>(lit<"~>">)),                                 //
                     left(as<ast::Operator::and_>(lit<"&">) | as<ast::Operator::cand>(ikw<"cand">)),                  //
                     left(as<ast::Operator::or_>(lit<"|">) | as<ast::Operator::cor>(ikw<"cor">)))> {};

  struct Call
      : rule<create<ast::Call>(
            assign<&ast::Call::callee>(ref<PostfixExpr>) + lit<"("> +
            many(append<&ast::Call::args>(ref<Expression>), lit<",">) +
            lit<")">)> {};

  struct CallAt
      : rule<create<ast::CallAtExpr>(
            assign<&ast::CallAtExpr::call>(ref<Call>) + lit<"@"> +
            assign<&ast::CallAtExpr::collector>(ref<PostfixExpr>))> {};

  struct Expression : rule<ref<CallAt> | ref<ExprLadder>> {};

  // ---- declarations, restrictions, equates -----------------------------

  struct Decl
      : rule<create<ast::Decl>(
            some(append<&ast::Decl::names>(ref<ID>), lit<",">) + lit<":"> +
            assign<&ast::Decl::type>(ref<TypeSpec>))> {};

  struct Parm
      : rule<create<ast::Parm>(
            some(append<&ast::Parm::names>(ref<ID>), lit<",">) + lit<":"> +
            (flag<&ast::Parm::isTypeParam>(ikw<"type">) |
             assign<&ast::Parm::type>(ref<TypeSpec>)))> {};

  // Native generator parameters are bracketed. Keep the earlier bare
  // spelling as a permissive extension, sharing the same captures.
  template <auto Member>
  static constexpr auto FormalParms =
      (lit<"["> + some(append<Member>(ref<Parm>), lit<",">) + lit<"]">) |
      some(append<Member>(ref<Parm>), lit<",">);

  struct OperDecl
      : rule<create<ast::OperDecl>(
                 flag<&ast::OperDecl::transmit>(ikw<"transmit">))  //
             | create<ast::OperDecl>(
                   some(append<&ast::OperDecl::names>(ref<ID>), lit<",">) +
                   lit<":"> + assign<&ast::OperDecl::type>(ref<TypeSpec>))> {
  };

  struct TypeSetElement
      : rule<create<ast::TypeSetElement>(
                 assign<&ast::TypeSetElement::name>(ref<ID>) + ikw<"has"> +
                 some(append<&ast::TypeSetElement::operDecls>(ref<OperDecl>),
                      lit<",">) +
                 many(append<&ast::TypeSetElement::equates>(ref<Equate>)))  //
             | create<ast::TypeSetElement>(
                   assign<&ast::TypeSetElement::name>(ref<ID>))> {};

  struct TypeSet
      : rule<create<ast::TypeSet>(
                 lit<"{"> + assign<&ast::TypeSet::binder>(ref<ID>) + lit<"|"> +
                 append<&ast::TypeSet::elements>(ref<TypeSetElement>) + lit<"}">) |
             create<ast::TypeSet>(
                 many(append<&ast::TypeSet::elements>(ref<TypeSetElement>)))  //
             | create<ast::TypeSet>(
                   assign<&ast::TypeSet::scopedRef>(ref<Reference>) + lit<"$"> +
                   assign<&ast::TypeSet::scopedName>(ref<ID>))> {};

  struct Restriction
      : rule<create<ast::Restriction>(
                 assign<&ast::Restriction::name>(ref<ID>) + ikw<"has"> +
                 some(append<&ast::Restriction::operDecls>(ref<OperDecl>),
                      lit<",">))  //
             | create<ast::Restriction>(
                   assign<&ast::Restriction::name>(ref<ID>) + ikw<"in"> +
                   assign<&ast::Restriction::typeSet>(ref<TypeSet>))> {};

  struct Where
      : rule<create<ast::Where>(
            ikw<"where"> +
            some(append<&ast::Where::restrictions>(ref<Restriction>), lit<",">))> {
  };

  struct Equate
      : rule<create<ast::Equate>(
            assign<&ast::Equate::name>(ref<ID>) + lit<"="> +
            assign<&ast::Equate::value>(ref<Expression> | ref<TypeSpec> |
                                        ref<TypeSet>))> {};

  struct Exception
      : rule<create<ast::Exception_>(
            assign<&ast::Exception_::name>(ref<ID>) +
            option((lit<"("> +
                    many(append<&ast::Exception_::types>(ref<TypeSpec>), lit<",">) +
                    lit<")">) |
                   some(append<&ast::Exception_::types>(ref<TypeSpec>), lit<",">)))> {};

  struct ReturnzClause
      : rule<create<ast::Returnz>(
            ikw<"returns"> + lit<"("> +
            some(append<&ast::Returnz::types>(ref<TypeSpec>), lit<",">) +
            lit<")">)> {};

  struct YieldsClause
      : rule<create<ast::Yields>(
            ikw<"yields"> + lit<"("> +
            some(append<&ast::Yields::types>(ref<TypeSpec>), lit<",">) +
            lit<")">)> {};

  struct SignalsClause
      : rule<create<ast::Signals>(
            ikw<"signals"> + lit<"("> +
            some(append<&ast::Signals::exceptions>(ref<Exception>), lit<",">) +
            lit<")">)> {};

  // ---- statements --------------------------------------------------

  struct Body
      : rule<create<ast::Body>(
            many(append<&ast::Body::equates>(ref<Equate>)) +
            many(append<&ast::Body::ownVars>(ref<OwnVar>)) +
            many(append<&ast::Body::statements>(ref<Statement>)))> {};

  struct OwnVar
      : rule<create<ast::OwnVar>(
            ikw<"own"> + some(append<&ast::OwnVar::decls>(ref<Decl>), lit<",">) +
            option(lit<":="> +
                   (assign<&ast::OwnVar::initCall>(ref<Call>)  //
                    | some(append<&ast::OwnVar::initExprs>(ref<Expression>),
                           lit<",">)) +
                   option(lit<"@"> +
                          assign<&ast::OwnVar::collector>(ref<PostfixExpr>))))> {};

  struct StateDecl
      : rule<create<ast::StateDecl>(
            option(flag<&ast::StateDecl::stable>(ikw<"stable">)) +
            some(append<&ast::StateDecl::decls>(ref<Decl>), lit<",">) +
            option(lit<":="> +
                   (assign<&ast::StateDecl::initCall>(ref<Call>)  //
                    | assign<&ast::StateDecl::initExpr>(ref<Expression>))))> {};

  struct DeclStatement
      : rule<create<ast::DeclStatement>(
            some(append<&ast::DeclStatement::decls>(ref<Decl>), lit<",">) +
            option(lit<":="> +
                   (assign<&ast::DeclStatement::initCall>(ref<Call>)  //
                    | some(append<&ast::DeclStatement::initExprs>(ref<Expression>),
                           lit<",">)) +
                   option(lit<"@"> +
                          assign<&ast::DeclStatement::collector>(
                              ref<PostfixExpr>))))> {};

  struct AssignStatement
      : rule<create<ast::AssignStatement>(
            some(append<&ast::AssignStatement::targets>(ref<ID>), lit<",">) +
            lit<":="> +
            (assign<&ast::AssignStatement::valueCall>(ref<Call>)  //
             | some(append<&ast::AssignStatement::values>(ref<Expression>),
                    lit<","> )) +
            option(lit<"@"> +
                   assign<&ast::AssignStatement::collector>(ref<PostfixExpr>)))> {
  };

  struct IndexAssignStatement
      : rule<create<ast::IndexAssignStatement>(
            assign<&ast::IndexAssignStatement::target>(ref<PostfixExpr>) +
            option(assign<&ast::IndexAssignStatement::index>(ref<Expression>)) +
            lit<":="> +
            assign<&ast::IndexAssignStatement::value>(ref<Expression>))> {};

  struct CallStatement
      : rule<create<ast::CallStatement>(
            assign<&ast::CallStatement::call>(ref<Call>) +
            option(lit<"@"> + assign<&ast::CallStatement::collector>(
                                   ref<PostfixExpr>)))> {};

  struct ForkStatement
      : rule<create<ast::ForkStatement>(
            ikw<"fork"> + assign<&ast::ForkStatement::call>(ref<Call>))> {};

  struct SeizeStatement
      : rule<create<ast::SeizeStatement>(
            ikw<"seize"> + assign<&ast::SeizeStatement::expr>(ref<Expression>) +
            ikw<"do"> + assign<&ast::SeizeStatement::body>(ref<Body>) +
            ikw<"end">)> {};

  struct PauseStatement
      : rule<create<ast::PauseStatement> + ikw<"pause">> {};
  struct TerminateStatement
      : rule<create<ast::TerminateStatement> + ikw<"terminate">> {};

  struct EnterStatement
      : rule<create<ast::EnterStatement>(
            ikw<"enter"> +
            (flag<&ast::EnterStatement::topAction>(ikw<"topaction">)  //
             | ikw<"action">) +
            assign<&ast::EnterStatement::body>(ref<Body>) + ikw<"end">)> {};

  struct ForEachClause
      : rule<create<ast::ForEachClause>(
            ikw<"foreach"> +
            some(append<&ast::ForEachClause::decls>(ref<Decl>), lit<",">) +
            ikw<"in"> + assign<&ast::ForEachClause::call>(ref<Call>))> {};

  struct CoArm
      : rule<create<ast::CoArm>(
            assign<&ast::CoArm::tag>(as<ast::ArmTag::action>(ikw<"action">) | as<ast::ArmTag::topaction>(ikw<"topaction">) |
                                     as<ast::ArmTag::process>(ikw<"process">)) +
            many(append<&ast::CoArm::foreach_>(ref<ForEachClause>)) +
            assign<&ast::CoArm::body>(ref<Body>))> {};

  struct CoenterStatement
      : rule<create<ast::CoenterStatement>(
            ikw<"coenter"> +
            some(append<&ast::CoenterStatement::arms>(ref<CoArm>)) + ikw<"end">)> {
  };

  struct LeaveStatement
      : rule<create<ast::LeaveStatement>(
            option(flag<&ast::LeaveStatement::abort>(ikw<"abort">)) +
            ikw<"leave">)> {};

  struct WhileStatement
      : rule<create<ast::WhileStatement>(
            ikw<"while"> + assign<&ast::WhileStatement::cond>(ref<Expression>) +
            ikw<"do"> + assign<&ast::WhileStatement::body>(ref<Body>) +
            ikw<"end">)> {};

  struct ForStatement
      : rule<create<ast::ForStatement>(
            ikw<"for"> +
            (some(append<&ast::ForStatement::decls>(ref<Decl>), lit<",">)  //
             | some(append<&ast::ForStatement::names>(ref<ID>), lit<",">)) +
            ikw<"in"> + assign<&ast::ForStatement::call>(ref<Call>) +
            ikw<"do"> + assign<&ast::ForStatement::body>(ref<Body>) +
            ikw<"end">)> {};

  struct IfBranch
      : rule<create<ast::IfBranch>(
            assign<&ast::IfBranch::cond>(ref<Expression>) + ikw<"then"> +
            assign<&ast::IfBranch::body>(ref<Body>))> {};

  struct IfStatement
      : rule<create<ast::IfStatement>(
            ikw<"if"> + append<&ast::IfStatement::branches>(ref<IfBranch>) +
            many(ikw<"elseif"> +
                 append<&ast::IfStatement::branches>(ref<IfBranch>)) +
            option(ikw<"else"> +
                   assign<&ast::IfStatement::elseBody>(ref<Body>)) +
            ikw<"end">)> {};

  static constexpr auto TagDecl =
      assign<&ast::TagArm::declName>(ref<ID>) + lit<":"> +
      assign<&ast::TagArm::declType>(ref<TypeSpec>);

  struct TagArm
      : rule<create<ast::TagArm>(
            ikw<"tag"> + some(append<&ast::TagArm::names>(ref<ID>), lit<",">) +
            option((lit<"("> + TagDecl + lit<")">) | TagDecl) +
            lit<":"> + assign<&ast::TagArm::body>(ref<Body>))> {};

  struct TagcaseStatement
      : rule<create<ast::TagcaseStatement>(
            ikw<"tagcase"> +
            assign<&ast::TagcaseStatement::expr>(ref<Expression>) +
            some(append<&ast::TagcaseStatement::arms>(ref<TagArm>)) +
            option(ikw<"others"> + lit<":"> +
                   assign<&ast::TagcaseStatement::othersBody>(ref<Body>)) +
            ikw<"end">)> {};

  struct ATagArm
      : rule<create<ast::ATagArm>(
            (flag<&ast::ATagArm::wtag>(ikw<"wtag">) | ikw<"tag">) +
            some(append<&ast::ATagArm::names>(ref<ID>), lit<",">) +
            ((lit<"("> + many(append<&ast::ATagArm::decls>(ref<Decl>), lit<",">) +
              lit<")">) | many(append<&ast::ATagArm::decls>(ref<Decl>))) + lit<":"> +
            assign<&ast::ATagArm::body>(ref<Body>))> {};

  struct TagtestStatement
      : rule<create<ast::TagtestStatement>(
            ikw<"tagtest"> +
            assign<&ast::TagtestStatement::expr>(ref<Expression>) +
            some(append<&ast::TagtestStatement::arms>(ref<ATagArm>)) +
            option(ikw<"others"> + lit<":"> +
                   assign<&ast::TagtestStatement::othersBody>(ref<Body>)) +
            ikw<"end">)> {};

  struct TagwaitStatement
      : rule<create<ast::TagwaitStatement>(
            ikw<"tagwait"> +
            assign<&ast::TagwaitStatement::expr>(ref<Expression>) +
            some(append<&ast::TagwaitStatement::arms>(ref<ATagArm>)) + ikw<"end">)> {
  };

  template <auto Member>
  static constexpr auto ResultValues =
      (lit<"("> + many(append<Member>(ref<Expression>), lit<",">) + lit<")">) |
      some(append<Member>(ref<Expression>), lit<",">);

  struct ReturnStatement
      : rule<create<ast::ReturnStatement>(
            option(flag<&ast::ReturnStatement::abort>(ikw<"abort">)) +
            ikw<"return"> +
            option(ResultValues<&ast::ReturnStatement::values>))> {};

  struct YieldStatement
      : rule<create<ast::YieldStatement>(
            ikw<"yield"> +
            option(ResultValues<&ast::YieldStatement::values>))> {};

  struct SignalStatement
      : rule<create<ast::SignalStatement>(
            option(flag<&ast::SignalStatement::abort>(ikw<"abort">)) +
            ikw<"signal"> + assign<&ast::SignalStatement::name>(ref<ID>) +
            option(ResultValues<&ast::SignalStatement::values>))> {};

  struct ExitStatement
      : rule<create<ast::ExitStatement>(
            option(flag<&ast::ExitStatement::abort>(ikw<"abort">)) +
            ikw<"exit"> + assign<&ast::ExitStatement::name>(ref<ID>) +
            option(ResultValues<&ast::ExitStatement::values>))> {};

  struct BreakStatement
      : rule<create<ast::BreakStatement>(
            option(flag<&ast::BreakStatement::abort>(ikw<"abort">)) +
            ikw<"break">)> {};

  struct ContinueStatement
      : rule<create<ast::ContinueStatement>(
            option(flag<&ast::ContinueStatement::abort>(ikw<"abort">)) +
            ikw<"continue">)> {};

  struct BeginStatement
      : rule<create<ast::BeginStatement>(
            ikw<"begin"> + assign<&ast::BeginStatement::body>(ref<Body>) +
            ikw<"end">)> {};

  struct WhenHandler
      : rule<create<ast::WhenHandler>(
            ikw<"when"> +
            some(append<&ast::WhenHandler::names>(ref<ID>), lit<",">) +
            ((lit<"("> +
              (flag<&ast::WhenHandler::star>(lit<"*">) |
               many(append<&ast::WhenHandler::decls>(ref<Decl>), lit<",">)) +
              lit<")">) | many(append<&ast::WhenHandler::decls>(ref<Decl>))) +
            lit<":"> + assign<&ast::WhenHandler::body>(ref<Body>))> {};

  struct OthersHandler
      : rule<create<ast::OthersHandler>(
            ikw<"others"> +
            ((lit<"("> + many(append<&ast::OthersHandler::decls>(ref<Decl>), lit<",">) +
              lit<")">) | many(append<&ast::OthersHandler::decls>(ref<Decl>))) + lit<":"> +
            assign<&ast::OthersHandler::body>(ref<Body>))> {};

  struct BaseStatement : rule<ref<DeclStatement>        //
                              | ref<AssignStatement>     //
                              | ref<IndexAssignStatement>  //
                              | ref<CallStatement>         //
                              | ref<ForkStatement>         //
                              | ref<SeizeStatement>        //
                              | ref<PauseStatement>        //
                              | ref<TerminateStatement>    //
                              | ref<EnterStatement>        //
                              | ref<CoenterStatement>      //
                              | ref<LeaveStatement>        //
                              | ref<WhileStatement>        //
                              | ref<ForStatement>          //
                              | ref<IfStatement>           //
                              | ref<TagcaseStatement>      //
                              | ref<TagtestStatement>      //
                              | ref<TagwaitStatement>      //
                              | ref<ReturnStatement>       //
                              | ref<YieldStatement>        //
                              | ref<SignalStatement>       //
                              | ref<ExitStatement>         //
                              | ref<BreakStatement>        //
                              | ref<ContinueStatement>     //
                              | ref<BeginStatement>> {};

  // `resignal` and `except` each wrap a whole prior statement: left
  // recursion, natural on this engine.
  struct Statement
      : rule<ref<BaseStatement>  //
             | create<ast::ResignalStatement>(
                   assign<&ast::ResignalStatement::body>(ref<Statement>) +
                   option(flag<&ast::ResignalStatement::abort>(
                       ikw<"abort">)) +
                   ikw<"resignal"> +
                   some(append<&ast::ResignalStatement::names>(ref<ID>),
                        lit<",">))  //
             | create<ast::ExceptStatement>(
                   assign<&ast::ExceptStatement::body>(ref<Statement>) +
                   ikw<"except"> +
                   many(append<&ast::ExceptStatement::handlers>(
                       ref<WhenHandler>)) +
                   option(assign<&ast::ExceptStatement::others>(
                       ref<OthersHandler>)) +
                   ikw<"end">)> {};

  // ---- top-level definitions -------------------------------------------

  struct Equates_
      : rule<create<ast::Equates_>(
            assign<&ast::Equates_::name>(ref<ID>) + lit<"="> + ikw<"equates"> +
            option(FormalParms<&ast::Equates_::parms> +
                   option(assign<&ast::Equates_::where>(ref<Where>))) +
            many(append<&ast::Equates_::equates>(ref<Equate>)) + ikw<"end"> +
            assign<&ast::Equates_::endName>(ref<ID>))> {};

  struct Procedure
      : rule<create<ast::Procedure>(
            assign<&ast::Procedure::name>(ref<ID>) + lit<"="> + ikw<"proc"> +
            option(FormalParms<&ast::Procedure::parms>) +
            lit<"("> +
            option(some(append<&ast::Procedure::args>(ref<Decl>), lit<",">)) +
            lit<")"> +
            option(assign<&ast::Procedure::returns>(ref<ReturnzClause>)) +
            option(assign<&ast::Procedure::signals>(ref<SignalsClause>)) +
            option(assign<&ast::Procedure::where>(ref<Where>)) +
            assign<&ast::Procedure::body>(ref<Body>) + ikw<"end"> +
            assign<&ast::Procedure::endName>(ref<ID>))> {};

  struct Iterator
      : rule<create<ast::Iterator>(
            assign<&ast::Iterator::name>(ref<ID>) + lit<"="> + ikw<"iter"> +
            option(FormalParms<&ast::Iterator::parms>) +
            lit<"("> +
            option(some(append<&ast::Iterator::args>(ref<Decl>), lit<",">)) +
            lit<")"> +
            option(assign<&ast::Iterator::yields>(ref<YieldsClause>)) +
            option(assign<&ast::Iterator::signals>(ref<SignalsClause>)) +
            option(assign<&ast::Iterator::where>(ref<Where>)) +
            assign<&ast::Iterator::body>(ref<Body>) + ikw<"end"> +
            assign<&ast::Iterator::endName>(ref<ID>))> {};

  struct Creator
      : rule<create<ast::Creator>(
            assign<&ast::Creator::name>(ref<ID>) + lit<"="> + ikw<"creator"> +
            lit<"("> +
            option(some(append<&ast::Creator::args>(ref<Decl>), lit<",">)) +
            lit<")"> +
            option(assign<&ast::Creator::returns>(ref<ReturnzClause>)) +
            option(assign<&ast::Creator::signals>(ref<SignalsClause>)) +
            assign<&ast::Creator::body>(ref<Body>) + ikw<"end"> +
            assign<&ast::Creator::endName>(ref<ID>))> {};

  struct Handler
      : rule<create<ast::Handler>(
            assign<&ast::Handler::name>(ref<ID>) + lit<"="> + ikw<"handler"> +
            lit<"("> +
            option(some(append<&ast::Handler::args>(ref<Decl>), lit<",">)) +
            lit<")"> +
            option(assign<&ast::Handler::returns>(ref<ReturnzClause>)) +
            option(assign<&ast::Handler::signals>(ref<SignalsClause>)) +
            assign<&ast::Handler::body>(ref<Body>) + ikw<"end"> +
            assign<&ast::Handler::endName>(ref<ID>))> {};

  static constexpr auto OperationAlt =
      append<&ast::Guardian::opsBefore>(ref<Creator>)  //
      | append<&ast::Guardian::opsBefore>(ref<Handler>)   //
      | append<&ast::Guardian::opsBefore>(ref<Procedure>)  //
      | append<&ast::Guardian::opsBefore>(ref<Iterator>);
  static constexpr auto OperationAfterAlt =
      append<&ast::Guardian::opsAfter>(ref<Creator>)  //
      | append<&ast::Guardian::opsAfter>(ref<Handler>)   //
      | append<&ast::Guardian::opsAfter>(ref<Procedure>)  //
      | append<&ast::Guardian::opsAfter>(ref<Iterator>);

  struct Guardian
      : rule<create<ast::Guardian>(
            assign<&ast::Guardian::name>(ref<ID>) + lit<"="> + ikw<"guardian"> +
            option(FormalParms<&ast::Guardian::parms>) + ikw<"is"> +
            some(append<&ast::Guardian::isNames>(ref<ID>), lit<",">) +
            option(ikw<"handles"> +
                   some(append<&ast::Guardian::handlesNames>(ref<ID>),
                        lit<","> )) +
            option(assign<&ast::Guardian::where>(ref<Where>)) +
            many(append<&ast::Guardian::equates>(ref<Equate>)) +
            many(append<&ast::Guardian::stateDecls>(ref<StateDecl>)) +
            option(ikw<"recover"> +
                   assign<&ast::Guardian::recoverBody>(ref<Body>) +
                   ikw<"end">) +
            option(ikw<"background"> +
                   assign<&ast::Guardian::backgroundBody>(ref<Body>) +
                   ikw<"end">) +
            many(OperationAlt) +
            assign<&ast::Guardian::creator>(ref<Creator>) + many(OperationAfterAlt) +
            ikw<"end"> + assign<&ast::Guardian::endName>(ref<ID>))> {};

  static constexpr auto RoutineAlt =
      append<&ast::Cluster::routines>(ref<Procedure>)  //
      | append<&ast::Cluster::routines>(ref<Iterator>);

  struct Cluster
      : rule<create<ast::Cluster>(
            assign<&ast::Cluster::name>(ref<ID>) + lit<"="> + ikw<"cluster"> +
            option(FormalParms<&ast::Cluster::parms>) + ikw<"is"> +
            some(append<&ast::Cluster::opIdns>(ref<ID>), lit<",">) +
            option(assign<&ast::Cluster::where>(ref<Where>)) +
            many(append<&ast::Cluster::equatesBeforeRep>(ref<Equate>)) +
            ikw<"rep"> + lit<"="> +
            assign<&ast::Cluster::rep>(ref<TypeSpec>) +
            many(append<&ast::Cluster::equatesAfterRep>(ref<Equate>)) +
            many(append<&ast::Cluster::ownVars>(ref<OwnVar>)) +
            some(RoutineAlt) + ikw<"end"> +
            assign<&ast::Cluster::endName>(ref<ID>))> {};

  struct Module
      : rule<create<ast::Module>(
            many(append<&ast::Module::equates>(ref<Equate>)) +
            assign<&ast::Module::unit>(ref<Equates_> | ref<Guardian> |
                                       ref<Procedure> | ref<Iterator> |
                                       ref<Cluster>))> {};

  using entry = Module;
  using ignored = WS;
  using hidden = list<COMMENT>;
};

}  // namespace argus
