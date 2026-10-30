// The cayenne language: its grammar. Application (`expr expr`, no operator
// token) is a postfix chain over the closed `Atom` set: the pi-type/lambda/
// case/`@` forms have no terminator and read to the end of whatever holds
// them, which would make them ambiguous as a bare application argument.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "cayenne/core/ast.hpp"

namespace cayenne {

struct Cayenne : cursiv::GrammarSpec {
  static constexpr auto word_boundary = cr<"a-zA-Z0-9">;

  struct WS : terminal<some(cr<R"( \r\n\t)">)> {};
  struct ID : terminal<cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9">)> {};

  struct Expr;
  struct Atom;
  struct DataCtor;
  struct Sig;
  struct Defn;
  struct Arm;

  struct Ident : rule<assign<&ast::Ident::name>(ref<ID>)> {};
  struct Hash : rule<create<ast::Hash>(lit<"#">)> {};

  struct DataCtor
      : rule<assign<&ast::DataCtor::name>(ref<ID>) +
             many(append<&ast::DataCtor::params>(ref<Atom>)) + lit<"|">> {};
  struct DataExpr
      : rule<kw<"data"> +
             many(append<&ast::DataExpr::ctors>(ref<DataCtor>))> {};

  struct Sig : rule<assign<&ast::Sig::name>(ref<ID>) + lit<"::"> +
                    assign<&ast::Sig::type>(ref<Expr>) +
                    option(lit<"="> + assign<&ast::Sig::value>(ref<Expr>)) +
                    lit<";">> {};
  struct SigExpr
      : rule<kw<"sig"> + many(append<&ast::SigExpr::sigs>(ref<Sig>))> {};

  static constexpr auto Vis =
      kw<"private">  //
      | (flag<&ast::Defn::isPublic>(kw<"public">) +
         (flag<&ast::Defn::isAbstract>(kw<"abstract">) |
          kw<"concrete">));
  struct Defn : rule<Vis + assign<&ast::Defn::name>(ref<ID>) + lit<"::"> +
                     assign<&ast::Defn::type>(ref<Expr>) + lit<"="> +
                     assign<&ast::Defn::value>(ref<Expr>) + lit<";">> {};
  struct StructExpr
      : rule<kw<"struct"> +
             many(append<&ast::StructExpr::defns>(ref<Defn>))> {};

  struct CtorArm
      : rule<lit<"("> + assign<&ast::CtorArm::ctor>(ref<ID>) +
             many(append<&ast::CtorArm::params>(ref<ID>)) + lit<")"> +
             lit<"->"> + assign<&ast::CtorArm::body>(ref<Expr>) +
             lit<";">> {};
  struct VarArm : rule<assign<&ast::VarArm::name>(ref<ID>) + lit<"->"> +
                       assign<&ast::VarArm::body>(ref<Expr>) + lit<";">> {};
  struct Arm : rule<ref<CtorArm> | ref<VarArm>> {};
  struct CaseExpr
      : rule<kw<"case"> + assign<&ast::CaseExpr::scrutinee>(ref<ID>) +
             kw<"of"> + many(append<&ast::CaseExpr::arms>(ref<Arm>)) +
             lit<"::"> + assign<&ast::CaseExpr::type>(ref<Expr>)> {};

  // The closed set of expressions with no unbounded trailing body: safe as an
  // application argument or the target of a field access.
  struct Atom : rule<ref<Ident> | ref<Hash> | ref<DataExpr> | ref<SigExpr> |
                     ref<StructExpr>> {};

  struct Spine
      : rule<ref<Atom> +
             many(nest<ast::Application, &ast::Application::func>(
                      assign<&ast::Application::arg>(ref<Atom>))  //
                  | nest<ast::FieldAccess, &ast::FieldAccess::target>(
                        lit<"."> +
                        assign<&ast::FieldAccess::field>(ref<ID>)))> {};

  struct PiType
      : rule<lit<"("> + assign<&ast::PiType::param>(ref<ID>) + lit<"::"> +
             assign<&ast::PiType::paramType>(ref<Expr>) + lit<")"> +
             lit<"->"> + assign<&ast::PiType::body>(ref<Expr>)> {};
  struct Lambda
      : rule<lit<"\\"> + lit<"("> + assign<&ast::Lambda::param>(ref<ID>) +
             lit<"::"> + assign<&ast::Lambda::paramType>(ref<Expr>) +
             lit<")"> + lit<"->"> +
             assign<&ast::Lambda::body>(ref<Expr>)> {};
  struct ConidAt
      : rule<assign<&ast::ConidAt::name>(ref<ID>) + lit<"@"> +
             assign<&ast::ConidAt::type>(ref<Expr>)> {};

  struct Expr : rule<ref<PiType> | ref<Lambda> | ref<CaseExpr> |
                     ref<ConidAt> | ref<Spine>> {};

  using entry = Expr;
  using ignored = WS;
};

}  // namespace cayenne
