// The argus language: its grammar.
#pragma once

#include "argus/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace argus {

struct Argus : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct COMMENT : terminal<lit<"%"> + many(cr<R"(^\r\n)">)> {};

  struct ID : terminal<cr<"a-zA-Z"> + many(w)> {};
  struct INT_LITERAL : terminal<some(d)> {};
  struct REAL_LITERAL : terminal<some(d) + lit<"."> + some(d)> {};
  struct CHAR_LITERAL : terminal<through(lit<"'">, lit<"'">)> {};
  struct STRING_LITERAL : terminal<through(lit<"\"">, lit<"\"">)> {};

  // Every binary/unary operator token, each denoting its own constant --
  // the ladder below names them, `op` holds what they mean. `−` (U+2212) is
  // the language's spelling for subtraction/negation, with ASCII `-` also
  // accepted.
  struct OP_POWER : terminal<lit<"**">, value<ast::Operator::power>> {};
  struct OP_INTDIV : terminal<lit<"//">, value<ast::Operator::intdiv>> {};
  struct OP_DIV : terminal<lit<"/">, value<ast::Operator::div>> {};
  struct OP_MUL : terminal<lit<"*">, value<ast::Operator::mul>> {};
  struct OP_CONCAT : terminal<lit<"||">, value<ast::Operator::concat>> {};
  struct OP_ADD : terminal<lit<"+">, value<ast::Operator::add>> {};
  struct OP_SUB : terminal<(lit<"-"> | lit<"−">), value<ast::Operator::sub>> {};
  struct OP_LE : terminal<lit<"<=">, value<ast::Operator::le>> {};
  struct OP_LT : terminal<lit<"<">, value<ast::Operator::lt>> {};
  struct OP_EQ : terminal<lit<"=">, value<ast::Operator::eq>> {};
  struct OP_GE : terminal<lit<">=">, value<ast::Operator::ge>> {};
  struct OP_GT : terminal<lit<">">, value<ast::Operator::gt>> {};
  struct OP_ALE : terminal<lit<"~<=">, value<ast::Operator::approx_le>> {};
  struct OP_ALT : terminal<lit<"~<">, value<ast::Operator::approx_lt>> {};
  struct OP_AEQ : terminal<lit<"~=">, value<ast::Operator::approx_eq>> {};
  struct OP_AGE : terminal<lit<"~>=">, value<ast::Operator::approx_ge>> {};
  struct OP_AGT : terminal<lit<"~>">, value<ast::Operator::approx_gt>> {};
  struct OP_AND : terminal<lit<"&">, value<ast::Operator::and_>> {};
  struct OP_CAND : terminal<kw<"cand">, value<ast::Operator::cand>> {};
  struct OP_OR : terminal<lit<"|">, value<ast::Operator::or_>> {};
  struct OP_COR : terminal<kw<"cor">, value<ast::Operator::cor>> {};
  struct OP_NOT : terminal<lit<"~">, value<ast::UnaryOp::not_>> {};
  struct OP_NEG : terminal<(lit<"-"> | lit<"−">), value<ast::UnaryOp::neg>> {};

  struct KIND_NULL : terminal<kw<"null">, value<ast::SimpleTypeKind::null_>> {};
  struct KIND_NODE : terminal<kw<"node">, value<ast::SimpleTypeKind::node>> {};
  struct KIND_BOOL : terminal<kw<"bool">, value<ast::SimpleTypeKind::bool_>> {};
  struct KIND_INT : terminal<kw<"int">, value<ast::SimpleTypeKind::int_>> {};
  struct KIND_REAL : terminal<kw<"real">, value<ast::SimpleTypeKind::real>> {};
  struct KIND_CHAR : terminal<kw<"char">, value<ast::SimpleTypeKind::char_>> {};
  struct KIND_STRING : terminal<kw<"string">, value<ast::SimpleTypeKind::string>> {};
  struct KIND_ANY : terminal<kw<"any">, value<ast::SimpleTypeKind::any>> {};
  struct KIND_IMAGE : terminal<kw<"image">, value<ast::SimpleTypeKind::image>> {};
  struct KIND_REP : terminal<kw<"rep">, value<ast::SimpleTypeKind::rep>> {};
  struct KIND_CVT : terminal<kw<"cvt">, value<ast::SimpleTypeKind::cvt>> {};

  struct CTK_SEQUENCE
      : terminal<kw<"sequence">, value<ast::ContainerTypeKind::sequence>> {};
  struct CTK_ARRAY : terminal<kw<"array">, value<ast::ContainerTypeKind::array>> {};
  struct CTK_ATOMIC_ARRAY
      : terminal<kw<"atomic_array">, value<ast::ContainerTypeKind::atomic_array>> {};
  struct CTK_MUTEX : terminal<kw<"mutex">, value<ast::ContainerTypeKind::mutex>> {};

  struct FTK_STRUCT : terminal<kw<"struct">, value<ast::FieldedTypeKind::struct_>> {};
  struct FTK_RECORD : terminal<kw<"record">, value<ast::FieldedTypeKind::record>> {};
  struct FTK_ATOMIC_RECORD
      : terminal<kw<"atomic_record">, value<ast::FieldedTypeKind::atomic_record>> {};
  struct FTK_ONEOF : terminal<kw<"oneof">, value<ast::FieldedTypeKind::oneof>> {};
  struct FTK_VARIANT : terminal<kw<"variant">, value<ast::FieldedTypeKind::variant>> {};
  struct FTK_ATOMIC_VARIANT
      : terminal<kw<"atomic_variant">, value<ast::FieldedTypeKind::atomic_variant>> {};

  struct RTK_PROCTYPE
      : terminal<kw<"proctype">, value<ast::RoutineTypeKind::proctype>> {};
  struct RTK_ITERTYPE
      : terminal<kw<"itertype">, value<ast::RoutineTypeKind::itertype>> {};
  struct RTK_CREATORTYPE
      : terminal<kw<"creatortype">, value<ast::RoutineTypeKind::creatortype>> {};
  struct RTK_HANDLERTYPE
      : terminal<kw<"handlertype">, value<ast::RoutineTypeKind::handlertype>> {};

  struct ARMTAG_ACTION : terminal<kw<"action">, value<ast::ArmTag::action>> {};
  struct ARMTAG_TOPACTION
      : terminal<kw<"topaction">, value<ast::ArmTag::topaction>> {};
  struct ARMTAG_PROCESS : terminal<kw<"process">, value<ast::ArmTag::process>> {};

  struct Reference;
  struct ActualParm;
  struct TypeActual;
  struct OpBinding;
  struct FieldSpec;
  struct TypeSpecRule;
  struct FieldRule;
  struct Primary;
  struct PostfixExpr;
  struct Unary;
  struct ExprLadder;
  struct CallAt;
  struct Expression;
  struct CallRule;
  struct BindArgRule;

  struct DeclRule;
  struct ParmRule;
  struct OperDeclRule;
  struct TypeSetElement;
  struct TypeSet;
  struct Restriction;
  struct WhereRule;
  struct EquateRule;
  struct ExceptionRule;
  struct ReturnzClause;
  struct YieldsClause;
  struct SignalsClause;

  struct BodyRule;
  struct OwnVarRule;
  struct StateDeclRule;

  struct Statement;
  struct BaseStatement;
  struct ForEachClause;
  struct CoArm;
  struct IfBranch;
  struct TagArm;
  struct ATagArm;
  struct WhenHandlerRule;
  struct OthersHandlerRule;

  struct Equates_;
  struct Procedure;
  struct Iterator;
  struct Creator;
  struct Handler;
  struct Guardian;
  struct Cluster;
  struct ModuleRule;

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

  struct TypeActual
      : rule<create<ast::TypeActual>(
            assign<&ast::TypeActual::type>(ref<TypeSpecRule>) +
            option(kw<"with"> +
                   option(some(
                       append<&ast::TypeActual::bindings>(ref<OpBinding>),
                       lit<",">))))> {};

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
            ref<KIND_NULL> | ref<KIND_NODE> | ref<KIND_BOOL> | ref<KIND_INT> |
            ref<KIND_REAL> | ref<KIND_CHAR> | ref<KIND_STRING> | ref<KIND_ANY> |
            ref<KIND_IMAGE> | ref<KIND_REP> | ref<KIND_CVT>))> {};

  struct ContainerTypeSpec
      : rule<create<ast::ContainerType>(
            assign<&ast::ContainerType::kind>(
                ref<CTK_SEQUENCE> | ref<CTK_ARRAY> | ref<CTK_ATOMIC_ARRAY> |
                ref<CTK_MUTEX>) +
            lit<"["> + assign<&ast::ContainerType::element>(ref<TypeActual>) +
            lit<"]">)> {};

  struct FieldedTypeSpec
      : rule<create<ast::FieldedType>(
            assign<&ast::FieldedType::kind>(
                ref<FTK_STRUCT> | ref<FTK_RECORD> | ref<FTK_ATOMIC_RECORD> |
                ref<FTK_ONEOF> | ref<FTK_VARIANT> | ref<FTK_ATOMIC_VARIANT>) +
            lit<"["> +
            some(append<&ast::FieldedType::fields>(ref<FieldSpec>), lit<",">) +
            lit<"]">)> {};

  struct RoutineTypeSpec
      : rule<create<ast::RoutineType>(
            assign<&ast::RoutineType::kind>(
                ref<RTK_PROCTYPE> | ref<RTK_ITERTYPE> | ref<RTK_CREATORTYPE> |
                ref<RTK_HANDLERTYPE>) +
            option(lit<"("> +
                   some(append<&ast::RoutineType::params>(ref<TypeSpecRule>),
                        lit<",">) +
                   lit<")">) +
            option(assign<&ast::RoutineType::returns>(ref<ReturnzClause>)) +
            option(assign<&ast::RoutineType::yields>(ref<YieldsClause>)) +
            option(assign<&ast::RoutineType::signals>(ref<SignalsClause>)))> {};

  struct ReferenceTypeSpec
      : rule<create<ast::ReferenceType>(
            assign<&ast::ReferenceType::ref>(ref<Reference>))> {};

  struct TypeSpecRule : rule<ref<SimpleTypeSpec>      //
                             | ref<ContainerTypeSpec>  //
                             | ref<FieldedTypeSpec>    //
                             | ref<RoutineTypeSpec>    //
                             | ref<ReferenceTypeSpec>> {};

  struct FieldRule
      : rule<create<ast::Field>(
            some(append<&ast::Field::names>(ref<ID>), lit<",">) + lit<":"> +
            assign<&ast::Field::value>(ref<Expression>))> {};

  // The argument list is bracketed here too -- see ast.hpp's comment on
  // `ast::DollarExpr`.
  struct BindArgRule
      : rule<create<ast::BindArg>(
            flag<&ast::BindArg::star>(lit<"*">) |
            assign<&ast::BindArg::expression>(ref<Expression>))> {};

  struct DollarForm
      : rule<create<ast::DollarExpr>(
            assign<&ast::DollarExpr::type>(ref<TypeSpecRule>) + lit<"$"> +
            (
                (assign<&ast::DollarExpr::name>(ref<ID>) +
                 option(lit<"("> +
                        some(append<&ast::DollarExpr::actualParms>(
                                 ref<ActualParm>),
                             lit<",">) +
                        lit<")">))  //
                | (lit<"{"> +
                   many(append<&ast::DollarExpr::fields>(ref<FieldRule>),
                        lit<",">) +
                   lit<"}">)  //
                | (lit<"["> +
                   option(assign<&ast::DollarExpr::size>(ref<Expression>) +
                          lit<":">) +
                   many(append<&ast::DollarExpr::items>(ref<Expression>),
                        lit<",">) +
                   lit<"]">)))> {};

  struct Primary
      : rule<(create<ast::NilLit> + kw<"nil">)      //
             | (create<ast::TrueLit> + kw<"true">)  //
             | (create<ast::FalseLit> + kw<"false">)  //
             | (create<ast::SelfLit> + kw<"self">)     //
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
             | create<ast::UpExpr>(kw<"up"> + lit<"("> +
                                   assign<&ast::UpExpr::operand>(ref<Expression>) +
                                   lit<")">)  //
             | create<ast::DownExpr>(
                   kw<"down"> + lit<"("> +
                   assign<&ast::DownExpr::operand>(ref<Expression>) + lit<")">)  //
             | create<ast::BindExpr>(
                   kw<"bind"> + assign<&ast::BindExpr::target>(ref<PostfixExpr>) +
                   option(lit<"("> +
                          some(append<&ast::BindExpr::args>(ref<BindArgRule>),
                               lit<",">) +
                          lit<")">))  //
             | ref<DollarForm>        //
             | ref<Reference>> {};

  struct PostfixExpr
      : rule<ref<Primary> +
             many(nest<ast::Selection, &ast::Selection::target>(
                 lit<"."> + assign<&ast::Selection::field>(ref<ID>)) |
                 nest<ast::Call, &ast::Call::callee>(
                     lit<"("> +
                     many(append<&ast::Call::args>(ref<Expression>), lit<",">) +
                     lit<")">))> {};

  struct Unary
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ref<OP_NOT> | ref<OP_NEG>) +
                 assign<&ast::UnaryExpr::operand>(ref<Unary>))  //
             | ref<PostfixExpr>> {};

  struct ExprLadder
      : infix<Unary, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(ref<OP_POWER>) >>                                      //
               left(ref<OP_INTDIV> | ref<OP_DIV> | ref<OP_MUL>) >>          //
               left(ref<OP_ADD> | ref<OP_SUB> | ref<OP_CONCAT>) >>          //
               left(ref<OP_LE> | ref<OP_LT> | ref<OP_EQ> | ref<OP_GE> |    //
                    ref<OP_GT> | ref<OP_ALE> | ref<OP_ALT> | ref<OP_AEQ> |  //
                    ref<OP_AGE> | ref<OP_AGT>) >>                          //
               left(ref<OP_AND> | ref<OP_CAND>) >>                         //
               left(ref<OP_OR> | ref<OP_COR>))> {};

  struct CallRule
      : rule<create<ast::Call>(
            assign<&ast::Call::callee>(ref<PostfixExpr>) + lit<"("> +
            many(append<&ast::Call::args>(ref<Expression>), lit<",">) +
            lit<")">)> {};

  struct CallAt
      : rule<create<ast::CallAtExpr>(
            assign<&ast::CallAtExpr::call>(ref<CallRule>) + lit<"@"> +
            assign<&ast::CallAtExpr::collector>(ref<PostfixExpr>))> {};

  struct Expression : rule<ref<CallAt> | ref<ExprLadder>> {};

  // ---- declarations, restrictions, equates -----------------------------

  struct DeclRule
      : rule<create<ast::Decl>(
            some(append<&ast::Decl::names>(ref<ID>), lit<",">) + lit<":"> +
            assign<&ast::Decl::type>(ref<TypeSpecRule>))> {};

  struct ParmRule
      : rule<create<ast::Parm>(
            some(append<&ast::Parm::names>(ref<ID>), lit<",">) + lit<":"> +
            (flag<&ast::Parm::isTypeParam>(kw<"type">) |
             assign<&ast::Parm::type>(ref<TypeSpecRule>)))> {};

  struct OperDeclRule
      : rule<create<ast::OperDecl>(
                 flag<&ast::OperDecl::transmit>(kw<"transmit">))  //
             | create<ast::OperDecl>(
                   some(append<&ast::OperDecl::names>(ref<ID>), lit<",">) +
                   lit<":"> + assign<&ast::OperDecl::type>(ref<TypeSpecRule>))> {
  };

  struct TypeSetElement
      : rule<create<ast::TypeSetElement>(
                 assign<&ast::TypeSetElement::name>(ref<ID>) + kw<"has"> +
                 some(append<&ast::TypeSetElement::operDecls>(ref<OperDeclRule>),
                      lit<",">) +
                 many(append<&ast::TypeSetElement::equates>(ref<EquateRule>)))  //
             | create<ast::TypeSetElement>(
                   assign<&ast::TypeSetElement::name>(ref<ID>))> {};

  struct TypeSet
      : rule<create<ast::TypeSet>(
                 many(append<&ast::TypeSet::elements>(ref<TypeSetElement>)))  //
             | create<ast::TypeSet>(
                   assign<&ast::TypeSet::scopedRef>(ref<Reference>) + lit<"$"> +
                   assign<&ast::TypeSet::scopedName>(ref<ID>))> {};

  struct Restriction
      : rule<create<ast::Restriction>(
                 assign<&ast::Restriction::name>(ref<ID>) + kw<"has"> +
                 some(append<&ast::Restriction::operDecls>(ref<OperDeclRule>),
                      lit<",">))  //
             | create<ast::Restriction>(
                   assign<&ast::Restriction::name>(ref<ID>) + kw<"in"> +
                   assign<&ast::Restriction::typeSet>(ref<TypeSet>))> {};

  struct WhereRule
      : rule<create<ast::Where>(
            kw<"where"> +
            some(append<&ast::Where::restrictions>(ref<Restriction>), lit<",">))> {
  };

  struct EquateRule
      : rule<create<ast::Equate>(
            assign<&ast::Equate::name>(ref<ID>) + lit<"="> +
            assign<&ast::Equate::value>(ref<Expression> | ref<TypeSpecRule> |
                                        ref<TypeSet>))> {};

  struct ExceptionRule
      : rule<create<ast::Exception_>(
            assign<&ast::Exception_::name>(ref<ID>) +
            option(some(append<&ast::Exception_::types>(ref<TypeSpecRule>),
                        lit<",">)))> {};

  struct ReturnzClause
      : rule<create<ast::Returnz>(
            kw<"returns"> + lit<"("> +
            some(append<&ast::Returnz::types>(ref<TypeSpecRule>), lit<",">) +
            lit<")">)> {};

  struct YieldsClause
      : rule<create<ast::Yields>(
            kw<"yields"> + lit<"("> +
            some(append<&ast::Yields::types>(ref<TypeSpecRule>), lit<",">) +
            lit<")">)> {};

  struct SignalsClause
      : rule<create<ast::Signals>(
            kw<"signals"> + lit<"("> +
            some(append<&ast::Signals::exceptions>(ref<ExceptionRule>), lit<",">) +
            lit<")">)> {};

  // ---- statements --------------------------------------------------

  struct BodyRule
      : rule<create<ast::Body>(
            many(append<&ast::Body::equates>(ref<EquateRule>)) +
            many(append<&ast::Body::statements>(ref<Statement>)))> {};

  struct OwnVarRule
      : rule<create<ast::OwnVar>(
            kw<"own"> + some(append<&ast::OwnVar::decls>(ref<DeclRule>), lit<",">) +
            option(lit<":="> +
                   (assign<&ast::OwnVar::initCall>(ref<CallRule>)  //
                    | some(append<&ast::OwnVar::initExprs>(ref<Expression>),
                           lit<",">)) +
                   option(lit<"@"> +
                          assign<&ast::OwnVar::collector>(ref<PostfixExpr>))))> {};

  struct StateDeclRule
      : rule<create<ast::StateDecl>(
            option(flag<&ast::StateDecl::stable>(kw<"stable">)) +
            some(append<&ast::StateDecl::decls>(ref<DeclRule>), lit<",">) +
            option(lit<":="> +
                   (assign<&ast::StateDecl::initCall>(ref<CallRule>)  //
                    | assign<&ast::StateDecl::initExpr>(ref<Expression>))))> {};

  struct DeclStatementRule
      : rule<create<ast::DeclStatement>(
            some(append<&ast::DeclStatement::decls>(ref<DeclRule>), lit<",">) +
            option(lit<":="> +
                   (assign<&ast::DeclStatement::initCall>(ref<CallRule>)  //
                    | some(append<&ast::DeclStatement::initExprs>(ref<Expression>),
                           lit<",">)) +
                   option(lit<"@"> +
                          assign<&ast::DeclStatement::collector>(
                              ref<PostfixExpr>))))> {};

  struct AssignStatementRule
      : rule<create<ast::AssignStatement>(
            some(append<&ast::AssignStatement::targets>(ref<ID>), lit<",">) +
            lit<":="> +
            (assign<&ast::AssignStatement::valueCall>(ref<CallRule>)  //
             | some(append<&ast::AssignStatement::values>(ref<Expression>),
                    lit<","> )) +
            option(lit<"@"> +
                   assign<&ast::AssignStatement::collector>(ref<PostfixExpr>)))> {
  };

  struct IndexAssignStatementRule
      : rule<create<ast::IndexAssignStatement>(
            assign<&ast::IndexAssignStatement::target>(ref<PostfixExpr>) +
            option(assign<&ast::IndexAssignStatement::index>(ref<Expression>)) +
            lit<":="> +
            assign<&ast::IndexAssignStatement::value>(ref<Expression>))> {};

  struct CallStatementRule
      : rule<create<ast::CallStatement>(
            assign<&ast::CallStatement::call>(ref<CallRule>) +
            option(lit<"@"> + assign<&ast::CallStatement::collector>(
                                   ref<PostfixExpr>)))> {};

  struct ForkStatementRule
      : rule<create<ast::ForkStatement>(
            kw<"fork"> + assign<&ast::ForkStatement::call>(ref<CallRule>))> {};

  struct SeizeStatementRule
      : rule<create<ast::SeizeStatement>(
            kw<"seize"> + assign<&ast::SeizeStatement::expr>(ref<Expression>) +
            kw<"do"> + assign<&ast::SeizeStatement::body>(ref<BodyRule>) +
            kw<"end">)> {};

  struct PauseStatementRule
      : rule<create<ast::PauseStatement> + kw<"pause">> {};
  struct TerminateStatementRule
      : rule<create<ast::TerminateStatement> + kw<"terminate">> {};

  struct EnterStatementRule
      : rule<create<ast::EnterStatement>(
            kw<"enter"> +
            (flag<&ast::EnterStatement::topAction>(kw<"topaction">)  //
             | kw<"action">) +
            assign<&ast::EnterStatement::body>(ref<BodyRule>) + kw<"end">)> {};

  struct ForEachClause
      : rule<create<ast::ForEachClause>(
            kw<"foreach"> +
            some(append<&ast::ForEachClause::decls>(ref<DeclRule>), lit<",">) +
            kw<"in"> + assign<&ast::ForEachClause::call>(ref<CallRule>))> {};

  struct CoArm
      : rule<create<ast::CoArm>(
            assign<&ast::CoArm::tag>(ref<ARMTAG_ACTION> | ref<ARMTAG_TOPACTION> |
                                     ref<ARMTAG_PROCESS>) +
            many(append<&ast::CoArm::foreach_>(ref<ForEachClause>)) +
            assign<&ast::CoArm::body>(ref<BodyRule>))> {};

  struct CoenterStatementRule
      : rule<create<ast::CoenterStatement>(
            kw<"coenter"> +
            some(append<&ast::CoenterStatement::arms>(ref<CoArm>)) + kw<"end">)> {
  };

  struct LeaveStatementRule
      : rule<create<ast::LeaveStatement>(
            option(flag<&ast::LeaveStatement::abort>(kw<"abort">)) +
            kw<"leave">)> {};

  struct WhileStatementRule
      : rule<create<ast::WhileStatement>(
            kw<"while"> + assign<&ast::WhileStatement::cond>(ref<Expression>) +
            kw<"do"> + assign<&ast::WhileStatement::body>(ref<BodyRule>) +
            kw<"end">)> {};

  struct ForStatementRule
      : rule<create<ast::ForStatement>(
            kw<"for"> +
            (some(append<&ast::ForStatement::decls>(ref<DeclRule>), lit<",">)  //
             | some(append<&ast::ForStatement::names>(ref<ID>), lit<",">)) +
            kw<"in"> + assign<&ast::ForStatement::call>(ref<CallRule>) +
            kw<"do"> + assign<&ast::ForStatement::body>(ref<BodyRule>) +
            kw<"end">)> {};

  struct IfBranch
      : rule<create<ast::IfBranch>(
            assign<&ast::IfBranch::cond>(ref<Expression>) + kw<"then"> +
            assign<&ast::IfBranch::body>(ref<BodyRule>))> {};

  struct IfStatementRule
      : rule<create<ast::IfStatement>(
            kw<"if"> + append<&ast::IfStatement::branches>(ref<IfBranch>) +
            many(kw<"elseif"> +
                 append<&ast::IfStatement::branches>(ref<IfBranch>)) +
            option(kw<"else"> +
                   assign<&ast::IfStatement::elseBody>(ref<BodyRule>)) +
            kw<"end">)> {};

  struct TagArm
      : rule<create<ast::TagArm>(
            kw<"tag"> + some(append<&ast::TagArm::names>(ref<ID>), lit<",">) +
            option(assign<&ast::TagArm::declName>(ref<ID>) + lit<":"> +
                   assign<&ast::TagArm::declType>(ref<TypeSpecRule>)) +
            lit<":"> + assign<&ast::TagArm::body>(ref<BodyRule>))> {};

  struct TagcaseStatementRule
      : rule<create<ast::TagcaseStatement>(
            kw<"tagcase"> +
            assign<&ast::TagcaseStatement::expr>(ref<Expression>) +
            some(append<&ast::TagcaseStatement::arms>(ref<TagArm>)) +
            option(kw<"others"> + lit<":"> +
                   assign<&ast::TagcaseStatement::othersBody>(ref<BodyRule>)) +
            kw<"end">)> {};

  struct ATagArm
      : rule<create<ast::ATagArm>(
            (flag<&ast::ATagArm::wtag>(kw<"wtag">) | kw<"tag">) +
            some(append<&ast::ATagArm::names>(ref<ID>), lit<",">) +
            many(append<&ast::ATagArm::decls>(ref<DeclRule>)) + lit<":"> +
            assign<&ast::ATagArm::body>(ref<BodyRule>))> {};

  struct TagtestStatementRule
      : rule<create<ast::TagtestStatement>(
            kw<"tagtest"> +
            assign<&ast::TagtestStatement::expr>(ref<Expression>) +
            some(append<&ast::TagtestStatement::arms>(ref<ATagArm>)) +
            option(kw<"others"> + lit<":"> +
                   assign<&ast::TagtestStatement::othersBody>(ref<BodyRule>)) +
            kw<"end">)> {};

  struct TagwaitStatementRule
      : rule<create<ast::TagwaitStatement>(
            kw<"tagwait"> +
            assign<&ast::TagwaitStatement::expr>(ref<Expression>) +
            some(append<&ast::TagwaitStatement::arms>(ref<ATagArm>)) + kw<"end">)> {
  };

  struct ReturnStatementRule
      : rule<create<ast::ReturnStatement>(
            option(flag<&ast::ReturnStatement::abort>(kw<"abort">)) +
            kw<"return"> +
            option(some(append<&ast::ReturnStatement::values>(ref<Expression>),
                        lit<",">)))> {};

  struct YieldStatementRule
      : rule<create<ast::YieldStatement>(
            kw<"yield"> +
            option(some(append<&ast::YieldStatement::values>(ref<Expression>),
                        lit<",">)))> {};

  struct SignalStatementRule
      : rule<create<ast::SignalStatement>(
            option(flag<&ast::SignalStatement::abort>(kw<"abort">)) +
            kw<"signal"> + assign<&ast::SignalStatement::name>(ref<ID>) +
            option(some(append<&ast::SignalStatement::values>(ref<Expression>),
                        lit<",">)))> {};

  struct ExitStatementRule
      : rule<create<ast::ExitStatement>(
            option(flag<&ast::ExitStatement::abort>(kw<"abort">)) +
            kw<"exit"> + assign<&ast::ExitStatement::name>(ref<ID>) +
            option(some(append<&ast::ExitStatement::values>(ref<Expression>),
                        lit<",">)))> {};

  struct BreakStatementRule
      : rule<create<ast::BreakStatement>(
            option(flag<&ast::BreakStatement::abort>(kw<"abort">)) +
            kw<"break">)> {};

  struct ContinueStatementRule
      : rule<create<ast::ContinueStatement>(
            option(flag<&ast::ContinueStatement::abort>(kw<"abort">)) +
            kw<"continue">)> {};

  struct BeginStatementRule
      : rule<create<ast::BeginStatement>(
            kw<"begin"> + assign<&ast::BeginStatement::body>(ref<BodyRule>) +
            kw<"end">)> {};

  struct WhenHandlerRule
      : rule<create<ast::WhenHandler>(
            kw<"when"> +
            some(append<&ast::WhenHandler::names>(ref<ID>), lit<",">) +
            (many(append<&ast::WhenHandler::decls>(ref<DeclRule>))  //
             | (lit<"("> + flag<&ast::WhenHandler::star>(lit<"*">) +
                lit<")">)) +
            lit<":"> + assign<&ast::WhenHandler::body>(ref<BodyRule>))> {};

  struct OthersHandlerRule
      : rule<create<ast::OthersHandler>(
            kw<"others"> +
            many(append<&ast::OthersHandler::decls>(ref<DeclRule>)) + lit<":"> +
            assign<&ast::OthersHandler::body>(ref<BodyRule>))> {};

  struct BaseStatement : rule<ref<DeclStatementRule>        //
                              | ref<AssignStatementRule>     //
                              | ref<IndexAssignStatementRule>  //
                              | ref<CallStatementRule>         //
                              | ref<ForkStatementRule>         //
                              | ref<SeizeStatementRule>        //
                              | ref<PauseStatementRule>        //
                              | ref<TerminateStatementRule>    //
                              | ref<EnterStatementRule>        //
                              | ref<CoenterStatementRule>      //
                              | ref<LeaveStatementRule>        //
                              | ref<WhileStatementRule>        //
                              | ref<ForStatementRule>          //
                              | ref<IfStatementRule>           //
                              | ref<TagcaseStatementRule>      //
                              | ref<TagtestStatementRule>      //
                              | ref<TagwaitStatementRule>      //
                              | ref<ReturnStatementRule>       //
                              | ref<YieldStatementRule>        //
                              | ref<SignalStatementRule>       //
                              | ref<ExitStatementRule>         //
                              | ref<BreakStatementRule>        //
                              | ref<ContinueStatementRule>     //
                              | ref<BeginStatementRule>> {};

  // `resignal` and `except` each wrap a whole prior statement: left
  // recursion, natural on this engine.
  struct Statement
      : rule<ref<BaseStatement>  //
             | create<ast::ResignalStatement>(
                   assign<&ast::ResignalStatement::body>(ref<Statement>) +
                   option(flag<&ast::ResignalStatement::abort>(
                       kw<"abort">)) +
                   kw<"resignal"> +
                   some(append<&ast::ResignalStatement::names>(ref<ID>),
                        lit<",">))  //
             | create<ast::ExceptStatement>(
                   assign<&ast::ExceptStatement::body>(ref<Statement>) +
                   kw<"except"> +
                   many(append<&ast::ExceptStatement::handlers>(
                       ref<WhenHandlerRule>)) +
                   option(assign<&ast::ExceptStatement::others>(
                       ref<OthersHandlerRule>)) +
                   kw<"end">)> {};

  // ---- top-level definitions -------------------------------------------

  struct Equates_
      : rule<create<ast::Equates_>(
            assign<&ast::Equates_::name>(ref<ID>) + lit<"="> + kw<"equates"> +
            option(some(append<&ast::Equates_::parms>(ref<ParmRule>), lit<",">) +
                   option(assign<&ast::Equates_::where>(ref<WhereRule>))) +
            many(append<&ast::Equates_::equates>(ref<EquateRule>)) + kw<"end"> +
            assign<&ast::Equates_::endName>(ref<ID>))> {};

  struct Procedure
      : rule<create<ast::Procedure>(
            assign<&ast::Procedure::name>(ref<ID>) + lit<"="> + kw<"proc"> +
            option(some(append<&ast::Procedure::parms>(ref<ParmRule>), lit<",">)) +
            lit<"("> +
            option(some(append<&ast::Procedure::args>(ref<DeclRule>), lit<",">)) +
            lit<")"> +
            option(assign<&ast::Procedure::returns>(ref<ReturnzClause>)) +
            option(assign<&ast::Procedure::signals>(ref<SignalsClause>)) +
            option(assign<&ast::Procedure::where>(ref<WhereRule>)) +
            assign<&ast::Procedure::body>(ref<BodyRule>) + kw<"end"> +
            assign<&ast::Procedure::endName>(ref<ID>))> {};

  struct Iterator
      : rule<create<ast::Iterator>(
            assign<&ast::Iterator::name>(ref<ID>) + lit<"="> + kw<"iter"> +
            option(some(append<&ast::Iterator::parms>(ref<ParmRule>), lit<",">)) +
            lit<"("> +
            option(some(append<&ast::Iterator::args>(ref<DeclRule>), lit<",">)) +
            lit<")"> +
            option(assign<&ast::Iterator::yields>(ref<YieldsClause>)) +
            option(assign<&ast::Iterator::signals>(ref<SignalsClause>)) +
            option(assign<&ast::Iterator::where>(ref<WhereRule>)) +
            assign<&ast::Iterator::body>(ref<BodyRule>) + kw<"end"> +
            assign<&ast::Iterator::endName>(ref<ID>))> {};

  struct Creator
      : rule<create<ast::Creator>(
            assign<&ast::Creator::name>(ref<ID>) + lit<"="> + kw<"creator"> +
            lit<"("> +
            option(some(append<&ast::Creator::args>(ref<DeclRule>), lit<",">)) +
            lit<")"> +
            option(assign<&ast::Creator::returns>(ref<ReturnzClause>)) +
            option(assign<&ast::Creator::signals>(ref<SignalsClause>)) +
            assign<&ast::Creator::body>(ref<BodyRule>) + kw<"end"> +
            assign<&ast::Creator::endName>(ref<ID>))> {};

  struct Handler
      : rule<create<ast::Handler>(
            assign<&ast::Handler::name>(ref<ID>) + lit<"="> + kw<"handler"> +
            lit<"("> +
            option(some(append<&ast::Handler::args>(ref<DeclRule>), lit<",">)) +
            lit<")"> +
            option(assign<&ast::Handler::returns>(ref<ReturnzClause>)) +
            option(assign<&ast::Handler::signals>(ref<SignalsClause>)) +
            assign<&ast::Handler::body>(ref<BodyRule>) + kw<"end"> +
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
            assign<&ast::Guardian::name>(ref<ID>) + lit<"="> + kw<"guardian"> +
            option(some(append<&ast::Guardian::parms>(ref<ParmRule>), lit<",">)) + kw<"is"> +
            some(append<&ast::Guardian::isNames>(ref<ID>), lit<",">) +
            option(kw<"handles"> +
                   some(append<&ast::Guardian::handlesNames>(ref<ID>),
                        lit<","> )) +
            option(assign<&ast::Guardian::where>(ref<WhereRule>)) +
            many(append<&ast::Guardian::equates>(ref<EquateRule>)) +
            many(append<&ast::Guardian::stateDecls>(ref<StateDeclRule>)) +
            option(kw<"recover"> +
                   assign<&ast::Guardian::recoverBody>(ref<BodyRule>) +
                   kw<"end">) +
            option(kw<"background"> +
                   assign<&ast::Guardian::backgroundBody>(ref<BodyRule>) +
                   kw<"end">) +
            many(OperationAlt) +
            assign<&ast::Guardian::creator>(ref<Creator>) + many(OperationAfterAlt) +
            kw<"end"> + assign<&ast::Guardian::endName>(ref<ID>))> {};

  static constexpr auto RoutineAlt =
      append<&ast::Cluster::routines>(ref<Procedure>)  //
      | append<&ast::Cluster::routines>(ref<Iterator>);

  struct Cluster
      : rule<create<ast::Cluster>(
            assign<&ast::Cluster::name>(ref<ID>) + lit<"="> + kw<"cluster"> +
            option(some(append<&ast::Cluster::parms>(ref<ParmRule>), lit<",">)) + kw<"is"> +
            some(append<&ast::Cluster::opIdns>(ref<ID>), lit<",">) +
            option(assign<&ast::Cluster::where>(ref<WhereRule>)) +
            many(append<&ast::Cluster::equatesBeforeRep>(ref<EquateRule>)) +
            kw<"rep"> + lit<"="> +
            assign<&ast::Cluster::rep>(ref<TypeSpecRule>) +
            many(append<&ast::Cluster::equatesAfterRep>(ref<EquateRule>)) +
            many(append<&ast::Cluster::ownVars>(ref<OwnVarRule>)) +
            some(RoutineAlt) + kw<"end"> +
            assign<&ast::Cluster::endName>(ref<ID>))> {};

  struct ModuleRule
      : rule<create<ast::Module>(
            many(append<&ast::Module::equates>(ref<EquateRule>)) +
            assign<&ast::Module::unit>(ref<Equates_> | ref<Guardian> |
                                       ref<Procedure> | ref<Iterator> |
                                       ref<Cluster>))> {};

  using entry = ModuleRule;
  using ignore = WS;
  using hidden = list<COMMENT>;
};

}  // namespace argus
