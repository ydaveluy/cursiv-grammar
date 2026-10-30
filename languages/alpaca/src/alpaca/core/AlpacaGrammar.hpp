// The alpaca language: its grammar.
#pragma once

#include <cstdint>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "alpaca/core/ast.hpp"

namespace alpaca {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};

struct Alpaca : cursiv::GrammarSpec {
  // A keyword and a name end at the same byte: `_` ends neither.
  static constexpr auto word_boundary = cr<"a-zA-Z0-9">;

  struct WS : terminal<some(s)> {};
  struct COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  // The display glyph of a state: `" "`, `"*"`. Quotes off; the source
  // never doubles or escapes a quote inside one.
  struct QUOTED_STRING
      : terminal<as<unquote>(lit<"\""> + many(cr<R"(^")">) + lit<"\"">)> {};

  // The four compass symbols v, <, >, ^ -- named directly rather than by
  // negating everything else, which would swallow almost every byte,
  // including whitespace and letters. `^` last in the class, since leading
  // it would negate here too.
  struct ARROWCHAIN : terminal<some(cr<"v<>^">)> {};

  struct NATNUM : terminal<some(d)> {};
  struct INITIAL_CONFIGURATION : terminal<some(any)> {};

  // ALPACA 1.1 reserves a leading lowercase `v` for an arrow chain.
  struct Name
      : terminal<!lit<"v"> + cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9">)> {};

  struct Neighbourhood
      : rule<lit<"("> +
             many(append<&ast::Neighbourhood::chains>(ref<ARROWCHAIN>)) +
             lit<")">> {};

  struct ClassRef : rule<kw<"is"> + assign<&ast::ClassRef::cls>(ref<Name>)> {
  };

  struct NamedStateRef
      : rule<assign<&ast::NamedStateRef::state>(ref<Name>)> {};
  struct ArrowStateRef
      : rule<create<ast::ArrowStateRef> +
             assign<&ast::ArrowStateRef::chain>(ref<ARROWCHAIN>)> {};
  struct MeStateRef : rule<create<ast::MeStateRef> + kw<"me">> {};

  struct StateRef
      : rule<ref<NamedStateRef> | ref<ArrowStateRef> | ref<MeStateRef>> {};

  // What a relational or adjacency test compares against: a state or a
  // class.
  struct Referent : rule<ref<StateRef> | ref<ClassRef>> {};


  struct BoolPrimitive
      : rule<create<ast::BoolLit>(assign<&ast::BoolLit::kind>(
            as<ast::BoolKind::literal_true>(kw<"true">) | as<ast::BoolKind::literal_false>(kw<"false">) | as<ast::BoolKind::guess>(kw<"guess">)))> {};

  // The trailing `=` is optional and, either way, denotes equality -- `a b`
  // and `a = b` read the same, so accepting `a = is C` too costs nothing
  // real.
  struct RelationalPred
      : rule<create<ast::RelationalPred>(
            assign<&ast::RelationalPred::left>(ref<StateRef>) +
            option(lit<"=">) +
            assign<&ast::RelationalPred::right>(ref<Referent>))> {};

  struct AdjacencyPred
      : rule<create<ast::AdjacencyPred>(
            assign<&ast::AdjacencyPred::count>(ref<NATNUM>) +
            option(kw<"in"> +
                   (assign<&ast::AdjacencyPred::inlineNbhd>(
                        ref<Neighbourhood>) |
                    assign<&ast::AdjacencyPred::nbhdRef>(ref<Name>))) +
            assign<&ast::AdjacencyPred::target>(ref<Referent>))> {};

  struct Term;
  struct Expression;

  struct AND : terminal<as<ast::LogicOp::logical_and>(kw<"and">)> {};
  static constexpr auto OpLogic = ref<AND> | as<ast::LogicOp::logical_or>(kw<"or">) | as<ast::LogicOp::logical_xor>(kw<"xor">);

  struct Expression
      : infix<Term, ast::BinaryExpr, left(OpLogic)> {};

  struct Term
      : rule<create<ast::NotExpr>(kw<"not"> +
                                  assign<&ast::NotExpr::operand>(ref<Term>)) |
             (lit<"("> + ref<Expression> + lit<")">) |  //
             ref<AdjacencyPred>                         //
             | ref<BoolPrimitive>                       //
             | ref<RelationalPred>> {};

  struct TransitionRule
      : rule<kw<"to"> + assign<&ast::TransitionRule::target>(ref<StateRef>) +
             option(kw<"when"> +
                    assign<&ast::TransitionRule::guard>(ref<Expression>))> {
  };

  struct StateDefn
      : rule<kw<"state"> + assign<&ast::StateDefn::name>(ref<Name>) +
             option(assign<&ast::StateDefn::display>(ref<QUOTED_STRING>)) +
             many(append<&ast::StateDefn::isA>(ref<ClassRef>)) +
             many(append<&ast::StateDefn::rules>(ref<TransitionRule>),
                  lit<",">)> {};

  struct ClassDefn
      : rule<kw<"class"> + assign<&ast::ClassDefn::name>(ref<Name>) +
             many(append<&ast::ClassDefn::isA>(ref<ClassRef>)) +
             many(append<&ast::ClassDefn::rules>(ref<TransitionRule>),
                  lit<",">)> {};

  struct NbhdDefn
      : rule<kw<"neighbourhood"> + assign<&ast::NbhdDefn::name>(ref<Name>) +
             assign<&ast::NbhdDefn::body>(ref<Neighbourhood>)> {};

  struct Model
      : rule<create<ast::Model> +
             some((append<&ast::Model::states>(ref<StateDefn>) |
                   append<&ast::Model::classes>(ref<ClassDefn>) |
                   append<&ast::Model::nbhds>(ref<NbhdDefn>)),
                  lit<";">) +
             ((flag<&ast::Model::beginTerminated>(kw<"begin">) +
               option(assign<&ast::Model::initialConfiguration>(
                   ref<INITIAL_CONFIGURATION>))) |
              lit<".">)> {};

  using entry = Model;
  using ignored = list<WS, COMMENT>;
};

}  // namespace alpaca
