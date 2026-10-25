// AQL (ArangoDB Query Language). LR, GLR where the ordered choices of
// COLLECT and the expr ladder overlap.
#pragma once

#include <cstddef>
#include <memory_resource>
#include <span>
#include <string_view>

#include "aql/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace aql {

inline constexpr auto undouble_sq =
    [](std::string_view t, std::pmr::memory_resource* mem) {
      const std::string_view body = t.substr(1, t.size() - 2);
      if (body.find('\'') == std::string_view::npos) return body;
      const std::span<char> room = cursiv::text_room(mem, body.size());
      std::size_t n = 0;
      for (std::size_t i = 0; i < body.size(); ++i) {
        room[n++] = body[i];
        if (body[i] == '\'') ++i;
      }
      return std::string_view(room.data(), n);
    };
using UndoubledSQ = cursiv::text_conv<undouble_sq>;

inline constexpr auto strip1 = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Strip1 = cursiv::text_conv<strip1>;

struct Aql : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  // Block comments nest.
  struct SQL_COMMENT
      : terminal<lit<"/*"> +
                 many(ref<SQL_COMMENT> | (!lit<"/*"> + !lit<"*/"> + any)) +
                 lit<"*/">> {};
  struct LINE_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};

  struct ID : terminal<cr<"a-zA-Z_"> + many(cr<"a-zA-Z0-9_">)> {};
  struct BIND_PARAMETER
      : terminal<lit<"@"> + cr<"a-zA-Z0-9"> + many(cr<"a-zA-Z0-9_">)> {};
  struct BIND_PARAMETER_COLL
      : terminal<lit<"@@"> + cr<"a-zA-Z0-9"> + many(cr<"a-zA-Z0-9_">)> {};

  // Single quotes double to escape; double/backstick strings take no escape
  // at all and may not be empty.
  struct STRING_LITERAL
      : terminal<lit<"'"> + many(lit<"''"> | cr<"^'">) + lit<"'">,
                 UndoubledSQ> {};
  struct DOUBLE_QUOTED_STRING_LITERAL
      : terminal<lit<"\""> + some(cr<R"(^")">) + lit<"\"">, Strip1> {};
  struct BACKSTICK_STRING_LITERAL
      : terminal<lit<"`"> + some(cr<"^`">) + lit<"`">, Strip1> {};

  // Decimal, float and exponent forms merged into one terminal: no other
  // rule names them apart, and the union is unambiguous by maximal munch (a
  // trailing `E[+-]digits` always reads longer).
  struct NUMBER
      : terminal<((some(d) + option(lit<"."> + some(d))) |
                  (lit<"."> + some(d))) +
                     option(cr<"eE"> + option(cr<"+-">) + some(d)),
                 double> {};

  struct Atom;
  struct Postfix;
  struct Unary;
  struct InfixA;
  struct QuantCompareOrBelow;
  struct AndOrBelow;
  struct TernaryOrBelow;
  struct Expr;
  struct DataAccessQuery;
  struct SimpleQuery;
  struct TraversalQuery;
  struct DataModificationQuery;

  static constexpr auto StringText = ref<STRING_LITERAL> |
                                     ref<DOUBLE_QUOTED_STRING_LITERAL> |
                                     ref<BACKSTICK_STRING_LITERAL>;

  struct NumberLit : rule<assign<&ast::NumberLit::value>(ref<NUMBER>)> {};
  struct StringLit : rule<assign<&ast::StringLit::value>(StringText)> {};
  struct BoolLit
      : rule<create<ast::BoolLit> +
             (flag<&ast::BoolLit::value>(ikw<"TRUE">) | ikw<"FALSE">)> {
  };
  struct NullLit : rule<create<ast::NullLit> + ikw<"NULL">> {};
  struct IdRef : rule<assign<&ast::IdRef::name>(ref<ID>)> {};
  struct BindParamRef
      : rule<assign<&ast::BindParamRef::name>(ref<BIND_PARAMETER>)> {};
  struct NewOldRef
      : rule<create<ast::NewOldRef> +
             (flag<&ast::NewOldRef::isNew>(kw<"NEW">) | kw<"OLD">)> {};

  struct ArrayLit
      : rule<create<ast::ArrayLit> + lit<"["> +
             option(assign<&ast::ArrayLit::value>(ref<Expr>)) + lit<"]">> {};

  struct Pair : rule<assign<&ast::Pair::key>(StringText | ref<ID>) +
                     lit<":"> + assign<&ast::Pair::value>(ref<Expr>)> {};

  struct ObjectLit
      : rule<create<ast::ObjectLit> + lit<"{"> +
             option(some(append<&ast::ObjectLit::pairs>(ref<Pair>), lit<",">) |
                    some(append<&ast::ObjectLit::shorthand>(ref<ID>),
                         lit<",">)) +
             lit<"}">> {};

  struct FuncCall
      : rule<create<ast::FuncCall>(
            assign<&ast::FuncCall::name>(ref<ID>) + lit<"("> +
            option(assign<&ast::FuncCall::args>(ref<Expr>)) + lit<")">)> {};

  struct Atom
      : rule<ref<NumberLit>       //
             | ref<BoolLit>       //
             | ref<StringLit>     //
             | ref<ArrayLit>      //
             | ref<ObjectLit>     //
             | ref<NullLit>       //
             | ref<FuncCall>      //
             | ref<IdRef>         //
             | ref<BindParamRef>  //
             | ref<NewOldRef>     //
             | (lit<"("> +
                (ref<Expr> |
                 create<ast::SubqueryExpr>(
                     assign<&ast::SubqueryExpr::query>(ref<DataAccessQuery>))) +
                lit<")">)> {};

  struct Postfix
      : rule<ref<Atom> +
             many(nest<ast::IndexExpr, &ast::IndexExpr::object>(
                      lit<"["> +
                      (assign<&ast::IndexExpr::index>(ref<Expr>) |
                       flag<&ast::IndexExpr::star>(lit<"*">)) +
                      lit<"]">)  //
                  | nest<ast::DotExpr, &ast::DotExpr::object>(
                        lit<"."> +
                        assign<&ast::DotExpr::field>(ref<ID>)))> {};

  struct Unary
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-">) +
                 assign<&ast::UnaryExpr::operand>(ref<Unary>))  //
             | create<ast::NotExpr>(
                   (lit<"!"> | ikw<"NOT">) +
                   assign<&ast::NotExpr::operand>(ref<Unary>))  //
             | ref<Postfix>> {};

  struct MUL : terminal<lit<"*">, value<ast::BinOp::multiply>> {};
  struct DIVOP : terminal<lit<"/">, value<ast::BinOp::divide>> {};
  struct MODOP : terminal<lit<"%">, value<ast::BinOp::modulo>> {};
  struct ADDOP : terminal<lit<"+">, value<ast::BinOp::add>> {};
  struct SUBOP : terminal<lit<"-">, value<ast::BinOp::subtract>> {};
  struct RANGEOP : terminal<lit<"..">, value<ast::BinOp::range>> {};
  struct GEOP : terminal<lit<">=">, value<ast::BinOp::ge>> {};
  struct LEOP : terminal<lit<"<=">, value<ast::BinOp::le>> {};
  struct GTOP : terminal<lit<">">, value<ast::BinOp::gt>> {};
  struct LTOP : terminal<lit<"<">, value<ast::BinOp::lt>> {};
  struct OpIn : terminal<ikw<"IN">, value<ast::BinOp::in>> {};
  struct OpNotIn : rule<ikw<"NOT"> + ikw<"IN">, value<ast::BinOp::notIn>> {};
  struct EQOP : terminal<lit<"==">, value<ast::BinOp::eq>> {};
  struct NEOP : terminal<lit<"!=">, value<ast::BinOp::ne>> {};
  struct REMATCH : terminal<lit<"=~">, value<ast::BinOp::regexMatch>> {};
  struct RENMATCH : terminal<lit<"!~">, value<ast::BinOp::regexNotMatch>> {};
  struct OpLike : terminal<ikw<"LIKE">, value<ast::BinOp::like>> {};
  struct OpNotLike
      : rule<ikw<"NOT"> + ikw<"LIKE">, value<ast::BinOp::notLike>> {};

  static constexpr auto OpMulDivMod = ref<MUL> | ref<DIVOP> | ref<MODOP>;
  static constexpr auto OpAddSub = ref<ADDOP> | ref<SUBOP>;
  static constexpr auto OpRelational =
      ref<GEOP> | ref<LEOP> | ref<GTOP> | ref<LTOP>;
  // IN and LIKE share the NOT keyword their negated forms open on, so one
  // precedence step must hold all four (precedenceClash otherwise).
  static constexpr auto OpEqualityInLike =
      ref<REMATCH> | ref<RENMATCH> | ref<EQOP> | ref<NEOP> | ref<OpNotIn> |
      ref<OpIn> | ref<OpNotLike> | ref<OpLike>;

  // AQL's own operator table: range binds above relational, relational
  // above IN/equality/LIKE.
  struct InfixA
      : infix<Unary, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(OpMulDivMod) >> left(OpAddSub) >> left(ref<RANGEOP>) >>
               left(OpRelational) >> left(OpEqualityInLike))> {};

  struct QuantAll : terminal<ikw<"ALL">, value<ast::Quantifier::all>> {};
  struct QuantNone : terminal<ikw<"NONE">, value<ast::Quantifier::none>> {};
  struct QuantAny : terminal<ikw<"ANY">, value<ast::Quantifier::any>> {};
  static constexpr auto AnyQuant = ref<QuantAll> | ref<QuantNone> | ref<QuantAny>;

  struct QCmpIn : terminal<ikw<"IN">, value<ast::CmpOp::in>> {};
  struct QCmpEq : terminal<lit<"==">, value<ast::CmpOp::eq>> {};
  struct QCmpNe : terminal<lit<"!=">, value<ast::CmpOp::ne>> {};
  struct QCmpGe : terminal<lit<">=">, value<ast::CmpOp::ge>> {};
  struct QCmpLe : terminal<lit<"<=">, value<ast::CmpOp::le>> {};
  struct QCmpGt : terminal<lit<">">, value<ast::CmpOp::gt>> {};
  struct QCmpLt : terminal<lit<"<">, value<ast::CmpOp::lt>> {};
  static constexpr auto AnyQCmp = ref<QCmpIn> | ref<QCmpEq> | ref<QCmpNe> |
                                  ref<QCmpGe> | ref<QCmpLe> | ref<QCmpGt> |
                                  ref<QCmpLt>;

  // `expr (ALL|NONE|ANY) (IN|==|!=|>=|<=|>|<) expr`: kept out of the main
  // ladder since its operator is two independent words, not one text/value.
  struct QuantCompareOrBelow
      : rule<create<ast::QuantifiedExpr>(
                 assign<&ast::QuantifiedExpr::left>(ref<InfixA>) +
                 assign<&ast::QuantifiedExpr::quant>(AnyQuant) +
                 assign<&ast::QuantifiedExpr::cmp>(AnyQCmp) +
                 assign<&ast::QuantifiedExpr::right>(ref<InfixA>))  //
             | ref<InfixA>> {};

  struct ANDOP : terminal<lit<"&&"> | ikw<"AND">, value<ast::BinOp::and_>> {};
  struct OROP : terminal<lit<"||"> | ikw<"OR">, value<ast::BinOp::or_>> {};
  struct ASSIGNOP : terminal<lit<"=">, value<ast::BinOp::assign>> {};

  struct AndOrBelow
      : infix<QuantCompareOrBelow, &ast::BinaryExpr::left,
              &ast::BinaryExpr::op, &ast::BinaryExpr::right,
              (left(ref<ANDOP>) >> left(ref<OROP>) >>
               left(ref<ASSIGNOP>))> {};

  // The `?`/`:` branches admit the full ladder; the condition only the
  // tighter levels below it.
  struct TernaryOrBelow
      : rule<create<ast::TernaryExpr>(
                 assign<&ast::TernaryExpr::cond>(ref<AndOrBelow>) +
                 lit<"?"> + assign<&ast::TernaryExpr::then_>(ref<Expr>) +
                 lit<":"> + assign<&ast::TernaryExpr::else_>(ref<Expr>))  //
             | ref<AndOrBelow>> {};

  struct COMMAOP : terminal<lit<",">, value<ast::BinOp::comma>> {};

  struct Expr
      : infix<TernaryOrBelow, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(ref<COMMAOP>)> {};

  struct Collection : rule<assign<&ast::Collection::name>(ref<ID>)  //
                           | assign<&ast::Collection::bindParam>(
                                 ref<BIND_PARAMETER_COLL>)> {};

  struct LetList
      : rule<some(ikw<"LET"> + append<&ast::LetList::bindings>(ref<Expr>))> {
  };

  struct AggregateAssign
      : rule<ikw<"AGGREGATE"> + assign<&ast::AggregateAssign::name>(ref<ID>) +
             lit<"="> + assign<&ast::AggregateAssign::value>(ref<Expr>)> {};

  struct Options
      : rule<ikw<"OPTIONS"> + assign<&ast::Options::value>(ref<ObjectLit>)> {
  };

  struct Search
      : rule<ikw<"SEARCH"> + assign<&ast::Search::condition>(ref<Expr>) +
             option(assign<&ast::Search::options>(ref<Options>))> {};

  struct ForIn
      : rule<option(assign<&ast::ForIn::letBefore>(ref<LetList>)) +
             ikw<"FOR"> + assign<&ast::ForIn::var>(ref<ID>) + ikw<"IN"> +
             assign<&ast::ForIn::source>(
                 ref<Expr> | create<ast::CollectionRef>(
                                 assign<&ast::CollectionRef::name>(
                                     ref<BIND_PARAMETER_COLL>))) +
             option(assign<&ast::ForIn::search>(ref<Search>)) +
             option(assign<&ast::ForIn::letAfter>(ref<LetList>))> {};

  struct Filter
      : rule<option(assign<&ast::Filter::letBefore>(ref<LetList>)) +
             ikw<"FILTER"> + assign<&ast::Filter::condition>(ref<Expr>) +
             option(assign<&ast::Filter::letAfter>(ref<LetList>))> {};

  struct SortTerm
      : rule<assign<&ast::SortTerm::key>(ref<Expr>) +
             option(flag<&ast::SortTerm::descending>(ikw<"DESC">) |
                    ikw<"ASC">)> {};

  struct Sort
      : rule<option(assign<&ast::Sort::letBefore>(ref<LetList>)) +
             ikw<"SORT"> +
             some(append<&ast::Sort::terms>(ref<SortTerm>), lit<",">) +
             option(assign<&ast::Sort::letAfter>(ref<LetList>))> {};

  struct Limit
      : rule<option(assign<&ast::Limit::letBefore>(ref<LetList>)) +
             ikw<"LIMIT"> + assign<&ast::Limit::count>(ref<NUMBER>) +
             option(lit<","> + assign<&ast::Limit::second>(ref<NUMBER>)) +
             option(assign<&ast::Limit::letAfter>(ref<LetList>))> {};

  // The five shapes COLLECT admits, merged so no two branches ever read the
  // same bare `expr` (two branches sharing it were indistinguishable when
  // INTO was absent).
  struct Collect
      : rule<ikw<"COLLECT"> +
             ((assign<&ast::Collect::expr_>(ref<Expr>) +
               (option(ikw<"INTO"> +
                       ((assign<&ast::Collect::intoVar>(ref<ID>) +
                         ikw<"KEEP"> +
                         assign<&ast::Collect::keepVar>(ref<ID>))  //
                        | assign<&ast::Collect::into>(ref<Expr>)))  //
                | (ikw<"WITH"> + ikw<"COUNT"> + ikw<"INTO"> +
                   assign<&ast::Collect::intoVar>(ref<ID>))))  //
              | (option(assign<&ast::Collect::expr_>(ref<Expr>)) +
                 assign<&ast::Collect::aggregate>(ref<AggregateAssign>) +
                 option(ikw<"INTO"> +
                        assign<&ast::Collect::intoVar>(ref<ID>)))  //
              | (ikw<"WITH"> +
                 flag<&ast::Collect::withCount>(ikw<"COUNT">) +
                 ikw<"INTO"> + assign<&ast::Collect::intoVar>(ref<ID>))) +
             option(assign<&ast::Collect::options>(ref<Options>))> {};

  struct Window
      : rule<ikw<"WINDOW"> +
             option(assign<&ast::Window::expr_>(ref<Expr>) + ikw<"WITH">) +
             assign<&ast::Window::spec>(ref<ObjectLit>) +
             assign<&ast::Window::aggregate>(ref<AggregateAssign>)> {};

  struct ForOp
      : rule<assign<&ast::ForOp::forIn>(ref<ForIn>) +
             many(append<&ast::ForOp::operations>(ref<Collect> | ref<Filter> | ref<Sort> | ref<Limit> |
                          ref<Window>))> {};

  struct ReturnExpr
      : rule<option(assign<&ast::ReturnExpr::letBefore>(ref<LetList>)) +
             ikw<"RETURN"> +
             option(flag<&ast::ReturnExpr::distinct>(ikw<"DISTINCT">)) +
             assign<&ast::ReturnExpr::value>(ref<Expr>)> {};

  struct SimpleQuery
      : rule<create<ast::SimpleQuery>(
            option(ikw<"WITH"> +
                   some(append<&ast::SimpleQuery::with_>(ref<Collection>),
                        lit<",">)) +
            many(append<&ast::SimpleQuery::forOps>(ref<ForOp>)) +
            assign<&ast::SimpleQuery::return_>(ref<ReturnExpr>))> {};

  struct TraversalQuery
      : rule<create<ast::TraversalQuery>(
            option(ikw<"WITH"> +
                   some(append<&ast::TraversalQuery::with_>(ref<Collection>),
                        lit<",">)) +
            ikw<"FOR"> + assign<&ast::TraversalQuery::v>(ref<ID>) +
            option(lit<","> + assign<&ast::TraversalQuery::e>(ref<ID>) +
                   option(lit<","> +
                          assign<&ast::TraversalQuery::p>(ref<ID>))) +
            ikw<"IN"> +
            option(assign<&ast::TraversalQuery::rangeFrom>(ref<NUMBER>) +
                   option(lit<".."> +
                          assign<&ast::TraversalQuery::rangeTo>(
                              ref<NUMBER>))) +
            assign<&ast::TraversalQuery::direction>(
                ikw<"OUTBOUND"> | ikw<"INBOUND"> | ikw<"ANY">) +
            assign<&ast::TraversalQuery::target>(ref<Expr>) +
            ((ikw<"GRAPH"> +
              assign<&ast::TraversalQuery::graph>(ref<Expr>))  //
             | some(append<&ast::TraversalQuery::collections>(
                        ref<Collection>),
                    lit<",">)) +
            option(ikw<"PRUNE"> +
                   assign<&ast::TraversalQuery::prune>(ref<Expr>)) +
            many(append<&ast::TraversalQuery::filters>(ref<Filter>)) +
            option(assign<&ast::TraversalQuery::options>(ref<Options>)) +
            option(
                assign<&ast::TraversalQuery::return_>(ref<ReturnExpr>)))> {};

  struct InsertOp
      : rule<ikw<"INSERT"> + assign<&ast::InsertOp::doc>(ref<ObjectLit>) +
             (flag<&ast::InsertOp::isInto>(ikw<"INTO">) | ikw<"IN">) +
             assign<&ast::InsertOp::target>(ref<Collection>) +
             option(assign<&ast::InsertOp::options>(ref<Options>))> {};

  struct UpdateOp
      : rule<ikw<"UPDATE"> +
             option(assign<&ast::UpdateOp::key>(ref<Expr>) + ikw<"WITH">) +
             assign<&ast::UpdateOp::doc>(ref<ObjectLit>) + ikw<"IN"> +
             assign<&ast::UpdateOp::target>(ref<Collection>) +
             option(assign<&ast::UpdateOp::options>(ref<Options>))> {};

  struct ReplaceOp
      : rule<ikw<"REPLACE"> +
             option(assign<&ast::ReplaceOp::key>(ref<Expr>) + ikw<"WITH">) +
             assign<&ast::ReplaceOp::doc>(ref<ObjectLit>) + ikw<"IN"> +
             assign<&ast::ReplaceOp::target>(ref<Collection>) +
             option(assign<&ast::ReplaceOp::options>(ref<Options>))> {};

  struct RemoveOp
      : rule<ikw<"REMOVE"> + assign<&ast::RemoveOp::key>(ref<Expr>) +
             ikw<"IN"> + assign<&ast::RemoveOp::target>(ref<Collection>) +
             option(assign<&ast::RemoveOp::options>(ref<Options>))> {};

  struct UpsertOp
      : rule<ikw<"UPSERT"> + assign<&ast::UpsertOp::search>(ref<Expr>) +
             ikw<"INSERT"> + assign<&ast::UpsertOp::insert_>(ref<Expr>) +
             (flag<&ast::UpsertOp::isUpdate>(ikw<"UPDATE">) |
              ikw<"REPLACE">) +
             assign<&ast::UpsertOp::modify>(ref<Expr>) + ikw<"IN"> +
             assign<&ast::UpsertOp::target>(ref<Collection>) +
             option(assign<&ast::UpsertOp::options>(ref<Options>))> {};

  struct DataModificationQuery
      : rule<create<ast::DataModificationQuery>(
            option(
                ikw<"WITH"> +
                some(append<&ast::DataModificationQuery::with_>(
                         ref<Collection>),
                     lit<",">)) +
            many(append<&ast::DataModificationQuery::forOps>(ref<ForOp>)) +
            (assign<&ast::DataModificationQuery::insert_>(ref<InsertOp>)  //
             | assign<&ast::DataModificationQuery::update_>(ref<UpdateOp>)  //
             | assign<&ast::DataModificationQuery::replace_>(
                   ref<ReplaceOp>)  //
             | some(append<&ast::DataModificationQuery::removes>(
                   ref<RemoveOp>))  //
             | assign<&ast::DataModificationQuery::upsert_>(
                   ref<UpsertOp>)) +
            option(assign<&ast::DataModificationQuery::return_>(
                ref<ReturnExpr>)))> {};

  struct DataAccessQuery : rule<ref<SimpleQuery> | ref<TraversalQuery>> {};
  struct ArangodbQuery
      : rule<ref<DataAccessQuery> | ref<DataModificationQuery>> {};

  using entry = ArangodbQuery;
  using ignore = WS;
  using hidden = list<LINE_COMMENT, SQL_COMMENT>;
};

}  // namespace aql
