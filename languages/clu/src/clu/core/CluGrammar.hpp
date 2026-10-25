// The CLU language: its grammar.
#pragma once

#include <cstdint>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "clu/core/ast.hpp"

namespace clu {

inline constexpr auto strip_quotes = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unquoted = cursiv::text_conv<strip_quotes>;

struct Clu : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct COMMENT : terminal<lit<"%"> + many(cr<R"(^\r\n)">)> {};

  struct ID : terminal<cr<"a-zA-Z"> + many(w)> {};
  struct INTLIT : terminal<some(d), std::int64_t> {};
  struct FLOATLIT : terminal<some(d) + lit<"."> + many(d), double> {};
  struct STRLIT : terminal<lit<"\""> + many(cr<R"(^")">) + lit<"\"">, Unquoted> {};

  // One token per operator, denoting its own constant. Longest spelling
  // first within a family so `~<=` never lexes as `~<` then `=`.
  struct OpPower : terminal<lit<"**">, value<ast::BinOp::power>> {};
  struct OpIntDiv : terminal<lit<"//">, value<ast::BinOp::intdiv>> {};
  struct OpDiv : terminal<lit<"/">, value<ast::BinOp::div>> {};
  struct OpMul : terminal<lit<"*">, value<ast::BinOp::mul>> {};
  struct OpConcat : terminal<lit<"||">, value<ast::BinOp::concat>> {};
  struct OpAdd : terminal<lit<"+">, value<ast::BinOp::add>> {};
  struct OpSub : terminal<lit<"-">, value<ast::BinOp::sub>> {};
  struct OpNle : terminal<lit<"~<=">, value<ast::BinOp::nle>> {};
  struct OpNlt : terminal<lit<"~<">, value<ast::BinOp::nlt>> {};
  struct OpNeq : terminal<lit<"~=">, value<ast::BinOp::neq>> {};
  struct OpNge : terminal<lit<"~>=">, value<ast::BinOp::nge>> {};
  struct OpNgt : terminal<lit<"~>">, value<ast::BinOp::ngt>> {};
  struct OpLe : terminal<lit<"<=">, value<ast::BinOp::le>> {};
  struct OpLt : terminal<lit<"<">, value<ast::BinOp::lt>> {};
  struct OpEq : terminal<lit<"=">, value<ast::BinOp::eq>> {};
  struct OpGe : terminal<lit<">=">, value<ast::BinOp::ge>> {};
  struct OpGt : terminal<lit<">">, value<ast::BinOp::gt>> {};
  struct OpAnd : terminal<lit<"&">, value<ast::BinOp::and_>> {};
  struct OpCand : terminal<kw<"cand">, value<ast::BinOp::cand>> {};
  struct OpOr : terminal<lit<"|">, value<ast::BinOp::or_>> {};
  struct OpCor : terminal<kw<"cor">, value<ast::BinOp::cor>> {};

  struct OpNot : terminal<lit<"~">, value<ast::UnOp::not_>> {};
  struct OpNeg : terminal<lit<"-">, value<ast::UnOp::neg>> {};

  struct KwUp : terminal<kw<"up">, value<ast::UpDown::up>> {};
  struct KwDown : terminal<kw<"down">, value<ast::UpDown::down>> {};

  struct KwNull : terminal<kw<"null">, value<ast::Builtin::null_>> {};
  struct KwBool : terminal<kw<"bool">, value<ast::Builtin::bool_>> {};
  struct KwInt : terminal<kw<"int">, value<ast::Builtin::int_>> {};
  struct KwReal : terminal<kw<"real">, value<ast::Builtin::real_>> {};
  struct KwChar : terminal<kw<"char">, value<ast::Builtin::char_>> {};
  struct KwString : terminal<kw<"string">, value<ast::Builtin::string_>> {};
  struct KwAny : terminal<kw<"any">, value<ast::Builtin::any_>> {};
  struct KwRep : terminal<kw<"rep">, value<ast::Builtin::rep_>> {};
  struct KwCvt : terminal<kw<"cvt">, value<ast::Builtin::cvt_>> {};

  struct KwRecord : terminal<kw<"record">, value<ast::RecordKind::record_>> {};
  struct KwStruct : terminal<kw<"struct">, value<ast::RecordKind::struct_>> {};
  struct KwOneof : terminal<kw<"oneof">, value<ast::RecordKind::oneof_>> {};
  struct KwVariant
      : terminal<kw<"variant">, value<ast::RecordKind::variant_>> {};

  struct KwProctype
      : terminal<kw<"proctype">, value<ast::RoutineTypeKind::proc_>> {};
  struct KwItertype
      : terminal<kw<"itertype">, value<ast::RoutineTypeKind::iter_>> {};

  // ---- forward declarations: the grammar is mutually recursive ----
  struct Expression;
  struct Unary;
  struct Primary;
  struct AtomBase;
  struct Invocation;
  struct FieldRule;
  struct TypeSpecRule;
  struct FieldSpecRule;
  struct DeclRule;
  struct ParamRule;
  struct ExceptionRule;
  struct EquateRule;
  struct TypeSetRule;
  struct OperDeclRule;
  struct RestrictionRule;
  struct BodyRule;
  struct StatementRule;
  struct WhenHandlerRule;
  struct OthersHandlerRule;
  struct OwnVarRule;
  struct RoutineRule;
  struct ClusterRule;

  // ---- expressions ----

  struct FieldRule
      : rule<create<ast::Field> +
             some(append<&ast::Field::names>(ref<ID>), lit<",">) + lit<":"> +
             assign<&ast::Field::value>(ref<Expression>)> {};

  static constexpr auto ConstantItem = ref<Expression> | ref<TypeSpecRule>;

  // `idn` alone, or the parameterized `idn[c1, c2]`; the brackets are
  // required here, and at every other constant-list site below.
  struct AtomBase
      : rule<create<ast::NilLit>(kw<"nil">)  //
             | create<ast::BoolLit>(flag<&ast::BoolLit::value>(
                                         kw<"true">) |
                                     kw<"false">)  //
             | create<ast::IntLit>(assign<&ast::IntLit::value>(ref<INTLIT>))  //
             | create<ast::RealLit>(
                   assign<&ast::RealLit::value>(ref<FLOATLIT>))  //
             | create<ast::StringLit>(
                   assign<&ast::StringLit::value>(ref<STRLIT>))  //
             | create<ast::GroupedExpr>(
                   lit<"("> + assign<&ast::GroupedExpr::inner>(ref<Expression>) +
                   lit<")">)  //
             | create<ast::TypeRecordLit>(
                   assign<&ast::TypeRecordLit::type>(ref<TypeSpecRule>) +
                   lit<"$"> + lit<"{"> +
                   option(some(append<&ast::TypeRecordLit::fields>(
                                   ref<FieldRule>),
                               lit<",">)) +
                   lit<"}">)  //
             | create<ast::TypeSeqLit>(
                   assign<&ast::TypeSeqLit::type>(ref<TypeSpecRule>) +
                   lit<"$"> + lit<"["> +
                   option(assign<&ast::TypeSeqLit::start>(ref<Expression>) +
                          lit<":">) +
                   option(some(append<&ast::TypeSeqLit::items>(ref<Expression>),
                               lit<",">)) +
                   lit<"]">)  //
             | create<ast::TypeOpRef>(
                   assign<&ast::TypeOpRef::type>(ref<TypeSpecRule>) + lit<"$"> +
                   assign<&ast::TypeOpRef::name>(ref<ID>) +
                   option(lit<"["> +
                          option(some(append<&ast::TypeOpRef::args>(
                                          ConstantItem),
                                      lit<",">)) +
                          lit<"]">))  //
             | create<ast::ForceExpr>(
                   kw<"force"> +
                   option(assign<&ast::ForceExpr::type>(ref<TypeSpecRule>)))  //
             | create<ast::UpDownExpr>(
                   assign<&ast::UpDownExpr::kind>(ref<KwUp> | ref<KwDown>) +
                   lit<"("> +
                   assign<&ast::UpDownExpr::operand>(ref<Expression>) +
                   lit<")">)  //
             | create<ast::NameRef>(
                   assign<&ast::NameRef::name>(ref<ID>) +
                   option(lit<"["> +
                          option(some(append<&ast::NameRef::args>(
                                          ConstantItem),
                                      lit<",">)) +
                          lit<"]">))> {};

  // `primary . name`, `primary [ expression ]`, `primary ( args )`. The
  // brackets around the index are required: two adjacent statements are a
  // primary immediately followed by another expression, so an unbracketed
  // reading forks every statement boundary.
  struct Primary
      : rule<ref<AtomBase> +
             many(nest<ast::Select, &ast::Select::object>(
                      lit<"."> + assign<&ast::Select::name>(ref<ID>))  //
                  | nest<ast::Index, &ast::Index::object>(
                        lit<"["> +
                        assign<&ast::Index::index>(ref<Expression>) +
                        lit<"]">)  //
                  | nest<ast::Call, &ast::Call::callee>(
                        lit<"("> +
                        option(some(append<&ast::Call::args>(ref<Expression>),
                                    lit<",">)) +
                        lit<")">))> {};

  struct Unary
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ref<OpNot> | ref<OpNeg>) +
                 assign<&ast::UnaryExpr::operand>(ref<Unary>))  //
             | ref<Primary>> {};

  struct Expression
      : infix<Unary, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (right(ref<OpPower>) >>
               left(ref<OpIntDiv> | ref<OpDiv> | ref<OpMul>) >>
               left(ref<OpConcat> | ref<OpAdd> | ref<OpSub>) >>
               left(ref<OpNle> | ref<OpNlt> | ref<OpNeq> | ref<OpNge> |
                    ref<OpNgt> | ref<OpLe> | ref<OpLt> | ref<OpEq> | ref<OpGe> |
                    ref<OpGt>) >>
               left(ref<OpAnd> | ref<OpCand>) >>
               left(ref<OpOr> | ref<OpCor>))> {};

  // A call with at least one argument: `for`, `own`, and a bare-statement
  // call all require this shape rather than any `Primary`.
  struct Invocation
      : rule<create<ast::Call>(
                 assign<&ast::Call::callee>(ref<Primary>) + lit<"("> +
                 some(append<&ast::Call::args>(ref<Expression>), lit<",">) +
                 lit<")">)> {};

  // ---- type specs ----

  struct FieldSpecRule
      : rule<create<ast::FieldSpec> +
             some(append<&ast::FieldSpec::names>(ref<ID>), lit<",">) +
             lit<":"> + assign<&ast::FieldSpec::type>(ref<TypeSpecRule>)> {};

  struct TypeSpecRule
      : rule<create<ast::BuiltinTypeSpec>(assign<&ast::BuiltinTypeSpec::kind>(
                 ref<KwNull> | ref<KwBool> | ref<KwInt> | ref<KwReal> |
                 ref<KwChar> | ref<KwString> | ref<KwAny> | ref<KwRep> |
                 ref<KwCvt>))  //
             | create<ast::ArrayTypeSpec>(kw<"array">)  //
             | create<ast::SequenceTypeSpec>(
                   kw<"sequence"> +
                   option(assign<&ast::SequenceTypeSpec::element>(
                       ref<TypeSpecRule>)))  //
             | create<ast::RecordTypeSpec>(
                   assign<&ast::RecordTypeSpec::kind>(
                       ref<KwRecord> | ref<KwStruct> | ref<KwOneof> |
                       ref<KwVariant>) +
                   option(lit<"["> +
                          option(some(append<&ast::RecordTypeSpec::fields>(
                                          ref<FieldSpecRule>),
                                      lit<",">)) +
                          lit<"]">))  //
             | create<ast::RoutineTypeSpec>(
                   assign<&ast::RoutineTypeSpec::kind>(
                       ref<KwProctype> | ref<KwItertype>) +
                   lit<"("> +
                   option(some(append<&ast::RoutineTypeSpec::params>(
                                   ref<TypeSpecRule>),
                               lit<",">)) +
                   lit<")"> +
                   option((kw<"returns"> | kw<"yields">) + lit<"("> +
                          some(append<&ast::RoutineTypeSpec::results>(
                                   ref<TypeSpecRule>),
                               lit<",">) +
                          lit<")">) +
                   option(kw<"signals"> + lit<"("> +
                          some(append<&ast::RoutineTypeSpec::signals>(
                                   ref<ExceptionRule>),
                               lit<",">) +
                          lit<")">))  //
             | create<ast::NamedTypeSpec>(
                   assign<&ast::NamedTypeSpec::name>(ref<ID>) +
                   option(lit<"["> +
                          option(some(append<&ast::NamedTypeSpec::args>(
                                          ConstantItem),
                                      lit<",">)) +
                          lit<"]">))> {};

  // ---- shared shapes ----

  struct DeclRule
      : rule<create<ast::Decl> +
             some(append<&ast::Decl::names>(ref<ID>), lit<",">) + lit<":"> +
             assign<&ast::Decl::type>(ref<TypeSpecRule>)> {};

  struct ParamRule
      : rule<create<ast::Param> +
             some(append<&ast::Param::names>(ref<ID>), lit<",">) + lit<":"> +
             (flag<&ast::Param::isTypeParam>(kw<"type">) |
              assign<&ast::Param::type>(ref<TypeSpecRule>))> {};

  // `name [(type_spec, ...)]`. The parens around the types are required:
  // `signals` is itself a comma-separated list of these, so an unparenthesized
  // reading is ambiguous about which comma closes which list.
  struct ExceptionRule
      : rule<create<ast::Exception> + assign<&ast::Exception::name>(ref<ID>) +
             option(lit<"("> +
                    some(append<&ast::Exception::types>(ref<TypeSpecRule>),
                         lit<",">) +
                    lit<")">)> {};

  struct EquateRule
      : rule<create<ast::Equate> + assign<&ast::Equate::name>(ref<ID>) +
             lit<"="> +
             assign<&ast::Equate::value>(ref<Expression> | ref<TypeSpecRule> |
                                          ref<TypeSetRule>)> {};

  struct OpNameRule
      : rule<create<ast::OpName> + assign<&ast::OpName::name>(ref<ID>) +
             lit<"["> +
             option(some(append<&ast::OpName::args>(ConstantItem), lit<",">)) +
             lit<"]">> {};

  struct OperDeclRule
      : rule<create<ast::OperDecl> +
             some(append<&ast::OperDecl::names>(ref<OpNameRule>), lit<",">) +
             lit<":"> + assign<&ast::OperDecl::type>(ref<TypeSpecRule>)> {};

  struct TypeSetItemRule
      : rule<create<ast::TypeSetItem> +
             assign<&ast::TypeSetItem::name>(ref<ID>) +
             option(kw<"has"> +
                    some(append<&ast::TypeSetItem::ops>(ref<OperDeclRule>),
                         lit<",">) +
                    many(append<&ast::TypeSetItem::equates>(ref<EquateRule>)))> {
  };

  struct TypeSetRule
      : rule<create<ast::TypeSet> +
             many(append<&ast::TypeSet::items>(ref<TypeSetItemRule>))> {};

  struct RestrictionRule
      : rule<create<ast::Restriction> +
             assign<&ast::Restriction::name>(ref<ID>) +
             (kw<"has"> +
                  some(append<&ast::Restriction::hasOps>(ref<OperDeclRule>),
                       lit<",">)  //
              | kw<"in"> +
                    assign<&ast::Restriction::inTypeSet>(ref<TypeSetRule>))> {};

  struct BodyRule
      : rule<create<ast::Body> +
             many(append<&ast::Body::equates>(ref<EquateRule>)) +
             many(append<&ast::Body::statements>(ref<StatementRule>))> {};

  // ---- statements ----

  struct ElseIfRule
      : rule<create<ast::ElseIf> + kw<"elseif"> +
             assign<&ast::ElseIf::cond>(ref<Expression>) + kw<"then"> +
             assign<&ast::ElseIf::body>(ref<BodyRule>)> {};

  struct TagArmRule
      : rule<create<ast::TagArm> + kw<"tag"> +
             some(append<&ast::TagArm::tags>(ref<ID>), lit<",">) +
             option(lit<"("> + assign<&ast::TagArm::bindName>(ref<ID>) +
                    lit<":"> + assign<&ast::TagArm::bindType>(ref<TypeSpecRule>) +
                    lit<")">) +
             lit<":"> + assign<&ast::TagArm::body>(ref<BodyRule>)> {};

  struct WhenHandlerRule
      : rule<create<ast::WhenHandler> + kw<"when"> +
             some(append<&ast::WhenHandler::names>(ref<ID>), lit<",">) +
             (flag<&ast::WhenHandler::star>(lit<"("> + lit<"*"> +
                                                  lit<")">)  //
              | option(some(append<&ast::WhenHandler::decls>(ref<DeclRule>),
                            lit<",">))) +
             lit<":"> + assign<&ast::WhenHandler::body>(ref<BodyRule>)> {};

  struct OthersHandlerRule
      : rule<create<ast::OthersHandler> + kw<"others"> +
             option(lit<"("> + assign<&ast::OthersHandler::bindName>(ref<ID>) +
                    lit<":"> +
                    assign<&ast::OthersHandler::bindType>(ref<TypeSpecRule>) +
                    lit<")">) +
             lit<":"> + assign<&ast::OthersHandler::body>(ref<BodyRule>)> {};

  struct StatementBase
      : rule<
            create<ast::DeclStmt>(assign<&ast::DeclStmt::decl>(ref<DeclRule>))  //
            | create<ast::VarInitStmt>(
                  assign<&ast::VarInitStmt::name>(ref<ID>) + lit<":"> +
                  assign<&ast::VarInitStmt::type>(ref<TypeSpecRule>) +
                  lit<":="> +
                  assign<&ast::VarInitStmt::value>(ref<Expression>))  //
            | create<ast::MultiInitStmt>(
                  some(append<&ast::MultiInitStmt::decls>(ref<DeclRule>),
                       lit<",">) +
                  lit<":="> +
                  assign<&ast::MultiInitStmt::value>(ref<Invocation>))  //
            | create<ast::AssignStmt>(
                  some(append<&ast::AssignStmt::names>(ref<ID>), lit<",">) +
                  lit<":="> +
                  (append<&ast::AssignStmt::values>(ref<Invocation>)  //
                   | some(append<&ast::AssignStmt::values>(ref<Expression>),
                          lit<",">)))  //
            | create<ast::SelectAssignStmt>(
                  assign<&ast::SelectAssignStmt::target>(ref<Primary>) +
                  lit<"."> + assign<&ast::SelectAssignStmt::name>(ref<ID>) +
                  lit<":="> +
                  assign<&ast::SelectAssignStmt::value>(ref<Expression>))  //
            | create<ast::IndexAssignStmt>(
                  assign<&ast::IndexAssignStmt::target>(ref<Primary>) +
                  lit<"["> +
                  assign<&ast::IndexAssignStmt::index>(ref<Expression>) +
                  lit<"]"> + lit<":="> +
                  assign<&ast::IndexAssignStmt::value>(ref<Expression>))  //
            | create<ast::ExprStmt>(
                  assign<&ast::ExprStmt::call>(ref<Invocation>))  //
            | create<ast::WhileStmt>(
                  kw<"while"> + assign<&ast::WhileStmt::cond>(ref<Expression>) +
                  kw<"do"> + assign<&ast::WhileStmt::body>(ref<BodyRule>) +
                  kw<"end">)  //
            | create<ast::ForStmt>(
                  kw<"for"> +
                  option(
                      some(append<&ast::ForStmt::decls>(ref<DeclRule>),
                           lit<",">)  //
                      | some(append<&ast::ForStmt::names>(ref<ID>), lit<",">)) +
                  kw<"in"> +
                  assign<&ast::ForStmt::iterCall>(ref<Invocation>) + kw<"do"> +
                  assign<&ast::ForStmt::body>(ref<BodyRule>) + kw<"end">)  //
            | create<ast::IfStmt>(
                  kw<"if"> + assign<&ast::IfStmt::cond>(ref<Expression>) +
                  kw<"then"> + assign<&ast::IfStmt::thenBody>(ref<BodyRule>) +
                  many(append<&ast::IfStmt::elseIfs>(ref<ElseIfRule>)) +
                  option(kw<"else"> +
                         assign<&ast::IfStmt::elseBody>(ref<BodyRule>)) +
                  kw<"end">)  //
            | create<ast::TagcaseStmt>(
                  kw<"tagcase"> + assign<&ast::TagcaseStmt::expr>(ref<Expression>) +
                  many(append<&ast::TagcaseStmt::arms>(ref<TagArmRule>)) +
                  option(flag<&ast::TagcaseStmt::hasOthers>(kw<"others">) +
                         lit<":"> +
                         assign<&ast::TagcaseStmt::othersBody>(ref<BodyRule>)) +
                  kw<"end">)  //
            | create<ast::ReturnStmt>(
                  (kw<"return">  //
                   | flag<&ast::ReturnStmt::isYield>(kw<"yield">)) +
                  option(lit<"("> +
                         some(append<&ast::ReturnStmt::values>(ref<Expression>),
                              lit<",">) +
                         lit<")">))  //
            | create<ast::SignalStmt>(
                  kw<"signal"> + assign<&ast::SignalStmt::name>(ref<ID>) +
                  option(lit<"("> +
                         some(append<&ast::SignalStmt::values>(ref<Expression>),
                              lit<",">) +
                         lit<")">))  //
            | create<ast::ExitStmt>(
                  kw<"exit"> + assign<&ast::ExitStmt::name>(ref<ID>) +
                  option(lit<"("> +
                         some(append<&ast::ExitStmt::values>(ref<Expression>),
                              lit<",">) +
                         lit<")">))  //
            | create<ast::BreakStmt>(kw<"break">)  //
            | create<ast::BeginStmt>(
                  kw<"begin"> + assign<&ast::BeginStmt::body>(ref<BodyRule>) +
                  kw<"end">)> {};

  struct StatementRule
      : rule<ref<StatementBase> +
             many(nest<ast::ResignalStmt, &ast::ResignalStmt::inner>(
                      kw<"resignal"> +
                      some(append<&ast::ResignalStmt::names>(ref<ID>),
                           lit<",">))  //
                  | nest<ast::ExceptStmt, &ast::ExceptStmt::inner>(
                        kw<"except"> +
                        many(append<&ast::ExceptStmt::whens>(
                            ref<WhenHandlerRule>)) +
                        option(assign<&ast::ExceptStmt::others>(
                            ref<OthersHandlerRule>)) +
                        kw<"end">))> {};

  // ---- routines ----

  // `decl`, `idn : type_spec := expression` or `decl_list := invocation`:
  // unified, since the first two share a decl's shape and the last differs
  // from the second only by requiring a call on the right. Accepts a
  // slightly wider set (several names before a single-value `:=`).
  struct OwnVarRule
      : rule<create<ast::OwnVar>(
            kw<"own"> +
            some(append<&ast::OwnVar::decls>(ref<DeclRule>), lit<",">) +
            option(lit<":="> +
                   assign<&ast::OwnVar::init>(ref<Expression>)))> {};

  struct RoutineRule
      : rule<create<ast::Routine>(
            assign<&ast::Routine::name>(ref<ID>) + lit<"="> +
            ((kw<"proc">)  //
             | flag<&ast::Routine::isIterator>(kw<"iter">)) +
            option(some(append<&ast::Routine::parms>(ref<ParamRule>),
                        lit<",">)) +
            lit<"("> +
            option(some(append<&ast::Routine::args>(ref<DeclRule>), lit<",">)) +
            lit<")"> +
            option((kw<"returns">  //
                    | kw<"yields">) +
                   lit<"("> +
                   some(append<&ast::Routine::results>(ref<TypeSpecRule>),
                        lit<",">) +
                   lit<")">) +
            option(kw<"signals"> + lit<"("> +
                   some(append<&ast::Routine::signals>(ref<ExceptionRule>),
                        lit<",">) +
                   lit<")">) +
            option(kw<"where"> +
                   some(append<&ast::Routine::where>(ref<RestrictionRule>),
                        lit<",">)) +
            many(append<&ast::Routine::bodyEquates>(ref<EquateRule>)) +
            many(append<&ast::Routine::ownVars>(ref<OwnVarRule>)) +
            many(append<&ast::Routine::statements>(ref<StatementRule>)) +
            kw<"end"> + assign<&ast::Routine::endName>(ref<ID>))> {};

  struct ClusterRule
      : rule<create<ast::Cluster>(
            assign<&ast::Cluster::name>(ref<ID>) + lit<"="> + kw<"cluster"> +
            option(some(append<&ast::Cluster::parms>(ref<ParamRule>),
                        lit<",">)) +
            kw<"is"> +
            some(append<&ast::Cluster::isNames>(ref<ID>), lit<",">) +
            option(kw<"where"> +
                   some(append<&ast::Cluster::where>(ref<RestrictionRule>),
                        lit<",">)) +
            many(append<&ast::Cluster::preRepEquates>(ref<EquateRule>)) +
            kw<"rep"> + lit<"="> +
            assign<&ast::Cluster::rep>(ref<TypeSpecRule>) +
            many(append<&ast::Cluster::postRepEquates>(ref<EquateRule>)) +
            many(append<&ast::Cluster::ownVars>(ref<OwnVarRule>)) +
            kw<"routine"> +
            many(append<&ast::Cluster::routines>(ref<RoutineRule>)) +
            kw<"end"> + assign<&ast::Cluster::endName>(ref<ID>))> {};

  struct ModuleRule
      : rule<create<ast::Module> +
             many(append<&ast::Module::equates>(ref<EquateRule>)) +
             assign<&ast::Module::routine>(ref<RoutineRule> |
                                            ref<ClusterRule>)> {};

  using entry = ModuleRule;
  using ignore = WS;
  using hidden = list<COMMENT>;
};

}  // namespace clu
