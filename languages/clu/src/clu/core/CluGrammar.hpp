// The CLU language: its grammar.
#pragma once

#include <cstdint>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "clu/core/ast.hpp"

namespace clu {

inline constexpr auto strip_quotes = [](std::string_view t,
                                        std::pmr::memory_resource* memory) {
  t = t.substr(1, t.size() - 2);
  if (t.find('\\') == t.npos) return t;
  const auto room = cursiv::text_room(memory, t.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < t.size(); ++i) {
    if (t[i] != '\\') { room[n++] = t[i]; continue; }
    char c = t[++i];
    if (c >= '0' && c <= '7') {
      unsigned value = c - '0';
      value = value * 8 + t[++i] - '0';
      value = value * 8 + t[++i] - '0';
      room[n++] = static_cast<char>(value);
      continue;
    }
    if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
    switch (c) {
      case 'n': room[n++] = '\n'; break;
      case 't': room[n++] = '\t'; break;
      case 'p': room[n++] = '\f'; break;
      case 'b': room[n++] = '\b'; break;
      case 'r': room[n++] = '\r'; break;
      case 'v': room[n++] = '\v'; break;
      default: room[n++] = c; break;
    }
  }
  return std::string_view(room.data(), n);
};

struct Clu : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n\v\f)">)> {};
  struct COMMENT : terminal<lit<"%"> + many(cr<R"(^\r\n)">)> {};
  static constexpr auto Reserved =
      ikw<"any"> | ikw<"array"> | ikw<"begin"> | ikw<"bool"> | ikw<"break"> |
      ikw<"cand"> | ikw<"char"> | ikw<"cluster"> | ikw<"continue"> | ikw<"cor"> |
      ikw<"cvt"> | ikw<"do"> | ikw<"down"> | ikw<"else"> | ikw<"elseif"> |
      ikw<"end"> | ikw<"except"> | ikw<"exit"> | ikw<"false"> | ikw<"for"> |
      ikw<"force"> | ikw<"has"> | ikw<"if"> | ikw<"in"> | ikw<"int"> |
      ikw<"is"> | ikw<"iter"> | ikw<"itertype"> | ikw<"nil"> | ikw<"null"> |
      ikw<"oneof"> | ikw<"others"> | ikw<"own"> | ikw<"proc"> | ikw<"proctype"> |
      ikw<"real"> | ikw<"record"> | ikw<"rep"> | ikw<"resignal"> | ikw<"return"> |
      ikw<"returns"> | ikw<"sequence"> | ikw<"signal"> | ikw<"signals"> |
      ikw<"string"> | ikw<"struct"> | ikw<"tag"> | ikw<"tagcase"> | ikw<"then"> |
      ikw<"true"> | ikw<"type"> | ikw<"up"> | ikw<"variant"> | ikw<"when"> |
      ikw<"where"> | ikw<"while"> | ikw<"yield"> | ikw<"yields">;
  struct ID : terminal<!Reserved + cr<"a-zA-Z_"> + many(w)> {};
  struct INTLIT : terminal<as<std::int64_t>(some(d))> {};
  static constexpr auto Exponent = cr<"eE"> + option(cr<"+-">) + some(d);
  struct FLOATLIT : terminal<as<double>(
      ((some(d) + lit<"."> + many(d)) | (lit<"."> + some(d))) +
          option(Exponent) | (some(d) + Exponent))> {};
  static constexpr auto Escape = lit<"\\"> +
      (cr<R"(nNtTpPbBrRvV'"\\)"> | (cr<"0-3"> + repeat<2>(cr<"0-7">)));
  struct STRLIT : terminal<as<strip_quotes>(
      lit<"\""> + many(Escape | cr<R"(\x20-\x21\x23-\x5b\x5d-\x7e)">) + lit<"\"">)> {};
  struct CHARLIT : terminal<as<strip_quotes>(
      lit<"'"> + (Escape | cr<R"(\x20-\x26\x28-\x5b\x5d-\x7e)">) + lit<"'">)> {};

  // Longer operator spellings take precedence in the lexer.

  // ---- forward declarations: the grammar is mutually recursive ----
  struct Expression;
  struct Unary;
  struct Primary;
  struct AtomBase;
  struct Invocation;
  struct Field;
  struct TypeSpec;
  struct FieldSpec;
  struct Decl;
  struct Param;
  struct Exception;
  struct Equate;
  struct TypeSet;
  struct OperDecl;
  struct Restriction;
  struct Body;
  struct Statement;
  struct WhenHandler;
  struct OthersHandler;
  struct OwnVar;
  struct Routine;
  struct Cluster;

  // ---- expressions ----

  struct Field
      : rule<create<ast::Field> +
             some(append<&ast::Field::names>(ref<ID>), lit<",">) + lit<":"> +
             assign<&ast::Field::value>(ref<Expression>)> {};

  static constexpr auto ConstantItem = ref<Expression> | ref<TypeSpec>;

  // `idn` alone, or the parameterized `idn[c1, c2]`; the brackets are
  // required here, and at every other constant-list site below.
  struct AtomBase
      : rule<create<ast::NilLit>(ikw<"nil">)  //
             | create<ast::BoolLit>(flag<&ast::BoolLit::value>(
                                         ikw<"true">) |
                                     ikw<"false">)  //
             | create<ast::IntLit>(assign<&ast::IntLit::value>(ref<INTLIT>))  //
             | create<ast::RealLit>(
                   assign<&ast::RealLit::value>(ref<FLOATLIT>))  //
             | create<ast::StringLit>(
                   assign<&ast::StringLit::value>(ref<STRLIT>))  //
             | create<ast::CharLit>(
                   assign<&ast::CharLit::value>(ref<CHARLIT>))
             | create<ast::GroupedExpr>(
                   lit<"("> + assign<&ast::GroupedExpr::inner>(ref<Expression>) +
                   lit<")">)  //
             | create<ast::TypeRecordLit>(
                   assign<&ast::TypeRecordLit::type>(ref<TypeSpec>) +
                   lit<"$"> + lit<"{"> +
                   option(some(append<&ast::TypeRecordLit::fields>(
                                   ref<Field>),
                               lit<",">)) +
                   lit<"}">)  //
             | create<ast::TypeSeqLit>(
                   assign<&ast::TypeSeqLit::type>(ref<TypeSpec>) +
                   lit<"$"> + lit<"["> +
                   option(assign<&ast::TypeSeqLit::start>(ref<Expression>) +
                          lit<":">) +
                   option(some(append<&ast::TypeSeqLit::items>(ref<Expression>),
                               lit<",">)) +
                   lit<"]">)  //
             | create<ast::TypeOpRef>(
                   assign<&ast::TypeOpRef::type>(ref<TypeSpec>) + lit<"$"> +
                   assign<&ast::TypeOpRef::name>(ref<ID>) +
                   option(lit<"["> +
                          option(some(append<&ast::TypeOpRef::args>(
                                          ConstantItem),
                                      lit<",">)) +
                          lit<"]">))  //
             | create<ast::ForceExpr>(
                   ikw<"force"> + lit<"["> +
                   assign<&ast::ForceExpr::type>(ref<TypeSpec>) + lit<"]">)  //
             | create<ast::UpDownExpr>(
                   assign<&ast::UpDownExpr::kind>(as<ast::UpDown::up>(ikw<"up">) | as<ast::UpDown::down>(ikw<"down">)) +
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
                 assign<&ast::UnaryExpr::op>(as<ast::UnOp::not_>(lit<"~">) | as<ast::UnOp::neg>(lit<"-">)) +
                 assign<&ast::UnaryExpr::operand>(ref<Unary>))  //
             | ref<Primary>> {};

  struct Expression
      : infix<Unary, ast::BinaryExpr,
              levels(right(as<ast::BinOp::power>(lit<"**">)),
                     left(as<ast::BinOp::intdiv>(lit<"//">) | as<ast::BinOp::div>(lit<"/">) | as<ast::BinOp::mul>(lit<"*">)),
                     left(as<ast::BinOp::concat>(lit<"||">) | as<ast::BinOp::add>(lit<"+">) | as<ast::BinOp::sub>(lit<"-">)),
                     left(as<ast::BinOp::nle>(lit<"~<=">) | as<ast::BinOp::nlt>(lit<"~<">) | as<ast::BinOp::neq>(lit<"~=">) | as<ast::BinOp::nge>(lit<"~>=">) |
                          as<ast::BinOp::ngt>(lit<"~>">) | as<ast::BinOp::le>(lit<"<=">) | as<ast::BinOp::lt>(lit<"<">) | as<ast::BinOp::eq>(lit<"=">) |
                          as<ast::BinOp::ge>(lit<">=">) | as<ast::BinOp::gt>(lit<">">)),
                     left(as<ast::BinOp::and_>(lit<"&">) | as<ast::BinOp::cand>(ikw<"cand">)),
                     left(as<ast::BinOp::or_>(lit<"|">) | as<ast::BinOp::cor>(ikw<"cor">)))> {};

  // A call (including an empty argument list), rather than any Primary.
  struct Invocation
      : rule<create<ast::Call>(
                 assign<&ast::Call::callee>(ref<Primary>) + lit<"("> +
                 many(append<&ast::Call::args>(ref<Expression>), lit<",">) +
                 lit<")">)> {};

  // ---- type specs ----

  struct FieldSpec
      : rule<create<ast::FieldSpec> +
             some(append<&ast::FieldSpec::names>(ref<ID>), lit<",">) +
             lit<":"> + assign<&ast::FieldSpec::type>(ref<TypeSpec>)> {};

  struct TypeSpec
      : rule<create<ast::BuiltinTypeSpec>(assign<&ast::BuiltinTypeSpec::kind>(
                 as<ast::Builtin::null_>(ikw<"null">) | as<ast::Builtin::bool_>(ikw<"bool">) | as<ast::Builtin::int_>(ikw<"int">) | as<ast::Builtin::real_>(ikw<"real">) |
                 as<ast::Builtin::char_>(ikw<"char">) | as<ast::Builtin::string_>(ikw<"string">) | as<ast::Builtin::any_>(ikw<"any">) | as<ast::Builtin::rep_>(ikw<"rep">) |
                 as<ast::Builtin::cvt_>(ikw<"cvt">) | as<ast::Builtin::type_>(ikw<"type">)))  //
             | create<ast::ArrayTypeSpec>(ikw<"array"> + lit<"["> +
                   assign<&ast::ArrayTypeSpec::element>(ref<TypeSpec>) + lit<"]">)  //
             | create<ast::SequenceTypeSpec>(
                   ikw<"sequence"> + lit<"["> +
                   assign<&ast::SequenceTypeSpec::element>(ref<TypeSpec>) + lit<"]">)  //
             | create<ast::RecordTypeSpec>(
                   assign<&ast::RecordTypeSpec::kind>(
                       as<ast::RecordKind::record_>(ikw<"record">) | as<ast::RecordKind::struct_>(ikw<"struct">) | as<ast::RecordKind::oneof_>(ikw<"oneof">) |
                       as<ast::RecordKind::variant_>(ikw<"variant">)) +
                   lit<"["> +
                   some(append<&ast::RecordTypeSpec::fields>(ref<FieldSpec>),
                        lit<",">) + lit<"]">)  //
             | create<ast::RoutineTypeSpec>(
                   assign<&ast::RoutineTypeSpec::kind>(
                       as<ast::RoutineTypeKind::proc_>(ikw<"proctype">) | as<ast::RoutineTypeKind::iter_>(ikw<"itertype">)) +
                   lit<"("> +
                   option(some(append<&ast::RoutineTypeSpec::params>(
                                   ref<TypeSpec>),
                               lit<",">)) +
                   lit<")"> +
                   option((ikw<"returns"> | ikw<"yields">) + lit<"("> +
                          some(append<&ast::RoutineTypeSpec::results>(
                                   ref<TypeSpec>),
                               lit<",">) +
                          lit<")">) +
                   option(ikw<"signals"> + lit<"("> +
                          some(append<&ast::RoutineTypeSpec::signals>(
                                   ref<Exception>),
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

  struct Decl
      : rule<create<ast::Decl> +
             some(append<&ast::Decl::names>(ref<ID>), lit<",">) + lit<":"> +
             assign<&ast::Decl::type>(ref<TypeSpec>)> {};

  struct Param
      : rule<create<ast::Param> +
             some(append<&ast::Param::names>(ref<ID>), lit<",">) + lit<":"> +
             (flag<&ast::Param::isTypeParam>(ikw<"type">) |
              assign<&ast::Param::type>(ref<TypeSpec>))> {};

  // `name [(type_spec, ...)]`. The parens around the types are required:
  // `signals` is itself a comma-separated list of these, so an unparenthesized
  // reading is ambiguous about which comma closes which list.
  struct Exception
      : rule<create<ast::Exception> + assign<&ast::Exception::name>(ref<ID>) +
             option(lit<"("> +
                    some(append<&ast::Exception::types>(ref<TypeSpec>),
                         lit<",">) +
                    lit<")">)> {};

  struct Equate
      : rule<create<ast::Equate> + assign<&ast::Equate::name>(ref<ID>) +
             lit<"="> +
             assign<&ast::Equate::value>(ref<Expression> | ref<TypeSpec> |
                                          ref<TypeSet>) + option(lit<";">)> {};

  struct OpName
      : rule<create<ast::OpName> + assign<&ast::OpName::name>(ref<ID>) +
             option(lit<"["> +
                    some(append<&ast::OpName::args>(ConstantItem), lit<",">) +
                    lit<"]">)> {};

  struct OperDecl
      : rule<create<ast::OperDecl> +
             some(append<&ast::OperDecl::names>(ref<OpName>), lit<",">) +
             lit<":"> + assign<&ast::OperDecl::type>(ref<TypeSpec>)> {};

  struct TypeSetItem
      : rule<assign<&ast::TypeSetItem::name>(ref<ID>) + ikw<"has"> +
             some(append<&ast::TypeSetItem::ops>(ref<OperDecl>), lit<",">) +
             option(lit<";">) +
             many(append<&ast::TypeSetItem::equates>(ref<Equate>))> {};

  struct TypeSet
      : rule<lit<"{"> + assign<&ast::TypeSet::name>(ref<ID>) + lit<"|"> +
             append<&ast::TypeSet::items>(ref<TypeSetItem>) + lit<"}">> {};

  struct Restriction
      : rule<create<ast::Restriction> +
             assign<&ast::Restriction::name>(ref<ID>) +
             ((ikw<"has"> +
                  some(append<&ast::Restriction::hasOps>(ref<OperDecl>),
                       lit<",">))  //
              | (ikw<"in"> +
                    (assign<&ast::Restriction::inTypeSet>(ref<TypeSet>) |
                     assign<&ast::Restriction::inName>(ref<ID>))))> {};

  struct Body
      : rule<create<ast::Body> +
             many(append<&ast::Body::equates>(ref<Equate>)) +
             many(append<&ast::Body::statements>(ref<Statement>))> {};

  // ---- statements ----

  struct ElseIf
      : rule<create<ast::ElseIf> + ikw<"elseif"> +
             assign<&ast::ElseIf::cond>(ref<Expression>) + ikw<"then"> +
             assign<&ast::ElseIf::body>(ref<Body>)> {};

  struct TagArm
      : rule<create<ast::TagArm> + ikw<"tag"> +
             some(append<&ast::TagArm::tags>(ref<ID>), lit<",">) +
             option(lit<"("> + assign<&ast::TagArm::bindName>(ref<ID>) +
                    lit<":"> + assign<&ast::TagArm::bindType>(ref<TypeSpec>) +
                    lit<")">) +
             lit<":"> + assign<&ast::TagArm::body>(ref<Body>)> {};

  struct WhenHandler
      : rule<create<ast::WhenHandler> + ikw<"when"> +
             some(append<&ast::WhenHandler::names>(ref<ID>), lit<",">) +
             (flag<&ast::WhenHandler::star>(lit<"("> + lit<"*"> +
                                                  lit<")">)  //
              | option(lit<"("> +
                       some(append<&ast::WhenHandler::decls>(ref<Decl>), lit<",">) +
                       lit<")">)) +
             lit<":"> + assign<&ast::WhenHandler::body>(ref<Body>)> {};

  struct OthersHandler
      : rule<create<ast::OthersHandler> + ikw<"others"> +
             option(lit<"("> + assign<&ast::OthersHandler::bindName>(ref<ID>) +
                    lit<":"> +
                    assign<&ast::OthersHandler::bindType>(ref<TypeSpec>) +
                    lit<")">) +
             lit<":"> + assign<&ast::OthersHandler::body>(ref<Body>)> {};

  struct StatementBase
      : rule<
            create<ast::DeclStmt>(assign<&ast::DeclStmt::decl>(ref<Decl>))  //
            | create<ast::VarInitStmt>(
                  assign<&ast::VarInitStmt::name>(ref<ID>) + lit<":"> +
                  assign<&ast::VarInitStmt::type>(ref<TypeSpec>) +
                  lit<":="> +
                  assign<&ast::VarInitStmt::value>(ref<Expression>))  //
            | create<ast::MultiInitStmt>(
                  some(append<&ast::MultiInitStmt::decls>(ref<Decl>),
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
                  ikw<"while"> + assign<&ast::WhileStmt::cond>(ref<Expression>) +
                  ikw<"do"> + assign<&ast::WhileStmt::body>(ref<Body>) +
                  ikw<"end">)  //
            | create<ast::ForStmt>(
                  ikw<"for"> +
                  option(
                      some(append<&ast::ForStmt::decls>(ref<Decl>),
                           lit<",">)  //
                      | some(append<&ast::ForStmt::names>(ref<ID>), lit<",">)) +
                  ikw<"in"> +
                  assign<&ast::ForStmt::iterCall>(ref<Invocation>) + ikw<"do"> +
                  assign<&ast::ForStmt::body>(ref<Body>) + ikw<"end">)  //
            | create<ast::IfStmt>(
                  ikw<"if"> + assign<&ast::IfStmt::cond>(ref<Expression>) +
                  ikw<"then"> + assign<&ast::IfStmt::thenBody>(ref<Body>) +
                  many(append<&ast::IfStmt::elseIfs>(ref<ElseIf>)) +
                  option(ikw<"else"> +
                         assign<&ast::IfStmt::elseBody>(ref<Body>)) +
                  ikw<"end">)  //
            | create<ast::TagcaseStmt>(
                  ikw<"tagcase"> + assign<&ast::TagcaseStmt::expr>(ref<Expression>) +
                  many(append<&ast::TagcaseStmt::arms>(ref<TagArm>)) +
                  option(flag<&ast::TagcaseStmt::hasOthers>(ikw<"others">) +
                         lit<":"> +
                         assign<&ast::TagcaseStmt::othersBody>(ref<Body>)) +
                  ikw<"end">)  //
            | create<ast::ReturnStmt>(
                  (ikw<"return">  //
                   | flag<&ast::ReturnStmt::isYield>(ikw<"yield">)) +
                  option(lit<"("> +
                         some(append<&ast::ReturnStmt::values>(ref<Expression>),
                              lit<",">) +
                         lit<")">))  //
            | create<ast::SignalStmt>(
                  ikw<"signal"> + assign<&ast::SignalStmt::name>(ref<ID>) +
                  option(lit<"("> +
                         some(append<&ast::SignalStmt::values>(ref<Expression>),
                              lit<",">) +
                         lit<")">))  //
            | create<ast::ExitStmt>(
                  ikw<"exit"> + assign<&ast::ExitStmt::name>(ref<ID>) +
                  option(lit<"("> +
                         some(append<&ast::ExitStmt::values>(ref<Expression>),
                              lit<",">) +
                         lit<")">))  //
            | create<ast::BreakStmt>(ikw<"break">)
            | create<ast::ContinueStmt>(ikw<"continue">)  //
            | create<ast::BeginStmt>(
                  ikw<"begin"> + assign<&ast::BeginStmt::body>(ref<Body>) +
                  ikw<"end">)> {};

  struct Statement
      : rule<ref<StatementBase> + option(lit<";">) +
             many((nest<ast::ResignalStmt, &ast::ResignalStmt::inner>(
                      ikw<"resignal"> +
                      some(append<&ast::ResignalStmt::names>(ref<ID>),
                           lit<",">))  //
                  | nest<ast::ExceptStmt, &ast::ExceptStmt::inner>(
                        ikw<"except"> +
                        many(append<&ast::ExceptStmt::whens>(
                            ref<WhenHandler>)) +
                        option(assign<&ast::ExceptStmt::others>(
                            ref<OthersHandler>)) +
                        ikw<"end">)) + option(lit<";">))> {};

  // ---- routines ----

  // `decl`, `idn : type_spec := expression` or `decl_list := invocation`:
  // unified, since the first two share a decl's shape and the last differs
  // from the second only by requiring a call on the right. Accepts a
  // slightly wider set (several names before a single-value `:=`).
  struct OwnVar
      : rule<create<ast::OwnVar>(
            ikw<"own"> +
            some(append<&ast::OwnVar::decls>(ref<Decl>), lit<",">) +
            option(lit<":="> +
                   assign<&ast::OwnVar::init>(ref<Expression>)) +
            option(lit<";">))> {};

  struct Routine
      : rule<create<ast::Routine>(
            assign<&ast::Routine::name>(ref<ID>) + lit<"="> +
            ((ikw<"proc">)  //
             | flag<&ast::Routine::isIterator>(ikw<"iter">)) +
            option(lit<"["> +
                   some(append<&ast::Routine::parms>(ref<Param>), lit<",">) +
                   lit<"]">) +
            lit<"("> +
            option(some(append<&ast::Routine::args>(ref<Decl>), lit<",">)) +
            lit<")"> +
            option((ikw<"returns">  //
                    | ikw<"yields">) +
                   lit<"("> +
                   some(append<&ast::Routine::results>(ref<TypeSpec>),
                        lit<",">) +
                   lit<")">) +
            option(ikw<"signals"> + lit<"("> +
                   some(append<&ast::Routine::signals>(ref<Exception>),
                        lit<",">) +
                   lit<")">) +
            option(ikw<"where"> +
                   some(append<&ast::Routine::where>(ref<Restriction>),
                        lit<",">)) + option(lit<";">) +
            many(append<&ast::Routine::bodyEquates>(ref<Equate>)) +
            many(append<&ast::Routine::ownVars>(ref<OwnVar>)) +
            many(append<&ast::Routine::statements>(ref<Statement>)) +
            ikw<"end"> + assign<&ast::Routine::endName>(ref<ID>) + option(lit<";">))> {};

  struct Cluster
      : rule<create<ast::Cluster>(
            assign<&ast::Cluster::name>(ref<ID>) + lit<"="> + ikw<"cluster"> +
            option(lit<"["> +
                   some(append<&ast::Cluster::parms>(ref<Param>), lit<",">) +
                   lit<"]">) +
            ikw<"is"> +
            some(append<&ast::Cluster::isNames>(ref<ID>), lit<",">) +
            option(ikw<"where"> +
                   some(append<&ast::Cluster::where>(ref<Restriction>),
                        lit<",">)) + option(lit<";">) +
            many(append<&ast::Cluster::preRepEquates>(ref<Equate>)) +
            ikw<"rep"> + lit<"="> +
            assign<&ast::Cluster::rep>(ref<TypeSpec>) + option(lit<";">) +
            many(append<&ast::Cluster::postRepEquates>(ref<Equate>)) +
            many(append<&ast::Cluster::ownVars>(ref<OwnVar>)) +
            many(append<&ast::Cluster::routines>(ref<Routine>)) +
            ikw<"end"> + assign<&ast::Cluster::endName>(ref<ID>) + option(lit<";">))> {};

  struct Module
      : rule<create<ast::Module> +
             many(append<&ast::Module::equates>(ref<Equate>)) +
             assign<&ast::Module::routine>(ref<Routine> |
                                            ref<Cluster>)> {};

  using entry = Module;
  using ignored = WS;
  using hidden = list<COMMENT>;
};

}  // namespace clu
