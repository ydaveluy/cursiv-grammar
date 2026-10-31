// AQL (ArangoDB Query Language). LR, GLR where the ordered choices of
// COLLECT and the expr ladder overlap.
#pragma once

#include "aql/core/ast.hpp"
#include "aql/core/TextValue.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace aql {

struct Aql : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct SQL_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  struct LINE_COMMENT : terminal<lit<"//"> + many(cr<R"(^\n)">)> {};

  static constexpr auto Escape = lit<"\\"> + any;
  struct ID
      : terminal<as<text_value>(
            ((option(lit<"$">) | some(lit<"_">)) + cr<"a-zA-Z"> +
             many(cr<"a-zA-Z0-9_">)) |
            (lit<"`"> + many(Escape | cr<R"(^`\\)">) + lit<"`">) |
            (lit<"´"> + many(Escape | (!lit<"´"> + !lit<"\\"> + any)) +
             lit<"´">))> {};
  static constexpr auto ParameterName =
      many(lit<"_">) + cr<"a-zA-Z0-9"> + many(cr<"a-zA-Z0-9_">);
  struct BIND_PARAMETER
      : terminal<lit<"@"> + ParameterName> {};
  struct BIND_PARAMETER_COLL
      : terminal<lit<"@@"> + ParameterName> {};

  // Backslash escaping applies to both quote styles, including empty strings.
  struct STRING_LITERAL
      : terminal<as<text_value>(lit<"'"> + many(Escape | cr<R"(^'\\)">) +
                                lit<"'">)> {};
  struct DOUBLE_QUOTED_STRING_LITERAL
      : terminal<as<text_value>(lit<"\""> + many(Escape | cr<R"(^"\\)">) +
                                lit<"\"">)> {};

  // Decimal, float and exponent forms merged into one terminal: no other
  // rule names them apart, and the union is unambiguous by maximal munch (a
  // trailing `E[+-]digits` always reads longer).
  struct NUMBER
      : terminal<as<double>(
            (((lit<"0"> | (cr<"1-9"> + many(d))) +
              option(lit<"."> + some(d))) | (lit<"."> + some(d))) +
            option(cr<"eE"> + option(cr<"+-">) + some(d)))> {};
  struct RADIX_NUMBER
      : terminal<as<radix_number>(
            (lit<"0"> + cr<"bB"> + some(cr<"01">)) |
            (lit<"0"> + cr<"xX"> + some(cr<"0-9a-fA-F">)))> {};

  struct Atom;
  struct Postfix;
  struct Unary;
  struct InfixA;
  struct QuantCompareOrBelow;
  struct AndOrBelow;
  struct TernaryOrBelow;
  struct Expr;
  struct ExprList;
  struct Binding;
  struct BindingList;
  struct DataAccessQuery;
  struct SimpleQuery;
  struct TraversalQuery;
  struct DataModificationQuery;
  struct ArangodbQuery;

  static constexpr auto StringText = ref<STRING_LITERAL> |
                                     ref<DOUBLE_QUOTED_STRING_LITERAL>;

  struct NumberLit
      : rule<assign<&ast::NumberLit::value>(ref<NUMBER> | ref<RADIX_NUMBER>)> {};
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
             option(assign<&ast::ArrayLit::value>(ref<ExprList>) +
                    option(lit<",">)) + lit<"]">> {};

  struct Pair
      : rule<((assign<&ast::Pair::key>(StringText | ref<ID>) |
               assign<&ast::Pair::keyExpr>(ref<BindParamRef>) |
               (lit<"["> + assign<&ast::Pair::keyExpr>(ref<Expr>) + lit<"]">)) +
              lit<":"> + assign<&ast::Pair::value>(ref<Expr>)) |
             flag<&ast::Pair::shorthand>(
                 assign<&ast::Pair::key>(ref<ID>))> {};

  struct ObjectLit
      : rule<create<ast::ObjectLit> + lit<"{"> +
             option(some(append<&ast::ObjectLit::pairs>(ref<Pair>), lit<",">) +
                    option(lit<",">)) +
             lit<"}">> {};

  struct FunctionName : rule<ref<ID> + many(lit<"::"> + ref<ID>)> {};

  struct FuncCall
      : rule<create<ast::FuncCall>(
            assign<&ast::FuncCall::name>(ref<FunctionName> | ikw<"LIKE">) +
            lit<"("> + option(assign<&ast::FuncCall::args>(ref<ExprList>)) +
            lit<")">)> {};

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
                     assign<&ast::SubqueryExpr::query>(ref<ArangodbQuery>))) +
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
                        (assign<&ast::DotExpr::field>(ref<ID>) |
                         assign<&ast::DotExpr::fieldParam>(ref<BindParamRef>))))> {};

  struct Unary
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-">) +
                 assign<&ast::UnaryExpr::operand>(ref<Unary>))  //
             | create<ast::NotExpr>(
                   (lit<"!"> | ikw<"NOT">) +
                   assign<&ast::NotExpr::operand>(ref<Unary>))  //
             | ref<Postfix>> {};


  static constexpr auto OpMulDivMod = as<ast::BinOp::multiply>(lit<"*">) | as<ast::BinOp::divide>(lit<"/">) | as<ast::BinOp::modulo>(lit<"%">);
  static constexpr auto OpAddSub = as<ast::BinOp::add>(lit<"+">) | as<ast::BinOp::subtract>(lit<"-">);
  static constexpr auto OpRelational =
      as<ast::BinOp::ge>(lit<">=">) | as<ast::BinOp::le>(lit<"<=">) | as<ast::BinOp::gt>(lit<">">) | as<ast::BinOp::lt>(lit<"<">);
  // Give negated membership one token, as tokens.ll does, so its NOT prefix
  // need not share a precedence level with LIKE or equality.
  struct NOT_IN
      : terminal<((ikw<"NOT"> + some(cr<R"( \t\r\n)">)) |
                  (lit<"!"> + many(cr<R"( \t\r\n)">))) + ikw<"IN">> {};
  struct NOT_LIKE
      : terminal<ikw<"NOT"> + some(cr<R"( \t\r\n)">) + ikw<"LIKE">> {};
  struct NOT_REGEX_MATCH
      : terminal<ikw<"NOT"> + many(cr<R"( \t\r\n)">) + lit<"=~">> {};
  struct NOT_REGEX_NON_MATCH
      : terminal<ikw<"NOT"> + many(cr<R"( \t\r\n)">) + lit<"!~">> {};
  static constexpr auto OpMembership =
      as<ast::BinOp::notIn>(ref<NOT_IN>) | as<ast::BinOp::in>(ikw<"IN">);
  static constexpr auto OpEqualityLike =
      as<ast::BinOp::regexMatch>(lit<"=~"> | ref<NOT_REGEX_NON_MATCH>) |
      as<ast::BinOp::regexNotMatch>(lit<"!~"> | ref<NOT_REGEX_MATCH>) |
      as<ast::BinOp::eq>(lit<"==">) | as<ast::BinOp::ne>(lit<"!=">) |
      as<ast::BinOp::notLike>(ref<NOT_LIKE>) | as<ast::BinOp::like>(ikw<"LIKE">);

  // ArangoDB 3.12 grammar.y: arithmetic, range, relational, membership,
  // equality/LIKE, in decreasing precedence.
  struct InfixA
      : infix<Unary, ast::BinaryExpr,
              levels(left(OpMulDivMod), left(OpAddSub), left(as<ast::BinOp::range>(lit<"..">)),
                     left(OpRelational), left(OpMembership), left(OpEqualityLike))> {};

  static constexpr auto AnyQuant = as<ast::Quantifier::all>(ikw<"ALL">) | as<ast::Quantifier::none>(ikw<"NONE">) | as<ast::Quantifier::any>(ikw<"ANY">);

  static constexpr auto AnyQCmp = as<ast::CmpOp::notIn>(ref<NOT_IN>) | as<ast::CmpOp::in>(ikw<"IN">) | as<ast::CmpOp::eq>(lit<"==">) | as<ast::CmpOp::ne>(lit<"!=">) |
                                  as<ast::CmpOp::ge>(lit<">=">) | as<ast::CmpOp::le>(lit<"<=">) | as<ast::CmpOp::gt>(lit<">">) |
                                  as<ast::CmpOp::lt>(lit<"<">);

  // `expr (ALL|NONE|ANY) (IN|==|!=|>=|<=|>|<) expr`: kept out of the main
  // ladder since its operator is two independent words, not one text/value.
  struct QuantCompareOrBelow
      : rule<create<ast::QuantifiedExpr>(
                 assign<&ast::QuantifiedExpr::left>(ref<InfixA>) +
                 (assign<&ast::QuantifiedExpr::quant>(AnyQuant) |
                  (assign<&ast::QuantifiedExpr::quant>(
                       as<ast::Quantifier::atLeast>(ikw<"AT"> + ikw<"LEAST">)) +
                   lit<"("> + assign<&ast::QuantifiedExpr::minimum>(ref<Expr>) +
                   lit<")">)) +
                 assign<&ast::QuantifiedExpr::cmp>(AnyQCmp) +
                 assign<&ast::QuantifiedExpr::right>(ref<InfixA>))  //
             | ref<InfixA>> {};

  struct ANDOP : terminal<as<ast::BinOp::and_>(lit<"&&"> | ikw<"AND">)> {};
  struct OROP : terminal<as<ast::BinOp::or_>(lit<"||"> | ikw<"OR">)> {};

  struct AndOrBelow
      : infix<QuantCompareOrBelow, ast::BinaryExpr,
              levels(left(ref<ANDOP>), left(ref<OROP>))> {};

  // The `?`/`:` branches admit the full ladder; the condition only the
  // tighter levels below it.
  struct TernaryOrBelow
      : rule<create<ast::TernaryExpr>(
                 assign<&ast::TernaryExpr::cond>(ref<AndOrBelow>) +
                 lit<"?"> + option(assign<&ast::TernaryExpr::then_>(ref<Expr>)) +
                 lit<":"> + assign<&ast::TernaryExpr::else_>(ref<Expr>))  //
             | ref<AndOrBelow>> {};

  struct Expr : rule<ref<TernaryOrBelow>> {};
  // Commas separate list items; they never belong to a scalar expression.
  struct ExprList : infix<Expr, ast::BinaryExpr,
                      left(as<ast::BinOp::comma>(lit<",">))> {};
  struct Binding
      : rule<create<ast::BinaryExpr>(
            assign<&ast::BinaryExpr::left>(ref<IdRef>) +
            assign<&ast::BinaryExpr::op>(as<ast::BinOp::assign>(lit<"=">)) +
            assign<&ast::BinaryExpr::right>(ref<Expr>))> {};
  struct BindingList : infix<Binding, ast::BinaryExpr,
                             left(as<ast::BinOp::comma>(lit<",">))> {};

  struct Collection : rule<assign<&ast::Collection::name>(ref<ID> | StringText)  //
                           | assign<&ast::Collection::bindParam>(
                                 ref<BIND_PARAMETER_COLL>)> {};

  struct LetList
      : rule<some(ikw<"LET"> + append<&ast::LetList::bindings>(ref<BindingList>))> {
  };

  struct AggregateBinding
      : rule<assign<&ast::AggregateAssign::name>(ref<ID>) +
             lit<"="> + assign<&ast::AggregateAssign::value>(ref<FuncCall>)> {};
  struct AggregateAssign : rule<ikw<"AGGREGATE"> + ref<AggregateBinding>> {};

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
             option(assign<&ast::ForIn::search>(ref<Search>) |
                    assign<&ast::ForIn::options>(ref<Options>)) +
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
             ikw<"LIMIT"> + assign<&ast::Limit::count>(ref<Expr>) +
             option(lit<","> + assign<&ast::Limit::second>(ref<Expr>)) +
             option(assign<&ast::Limit::letAfter>(ref<LetList>))> {};

  static constexpr auto CountInto =
      ikw<"WITH"> + flag<&ast::Collect::withCount>(ikw<"COUNT">) +
      ikw<"INTO"> + assign<&ast::Collect::intoVar>(ref<ID>);
  static constexpr auto CollectAggregate =
      assign<&ast::Collect::aggregate>(ref<AggregateAssign>) +
      many(lit<","> + append<&ast::Collect::moreAggregates>(ref<AggregateBinding>));
  static constexpr auto CollectInto =
      ikw<"INTO"> + assign<&ast::Collect::into>(ref<Binding> | ref<IdRef>) +
      option(ikw<"KEEP"> + assign<&ast::Collect::keepVar>(ref<ID>) +
             many(lit<","> + append<&ast::Collect::keepMore>(ref<ID>)));

  struct Collect
      : rule<option(assign<&ast::Collect::letBefore>(ref<LetList>)) +
             ikw<"COLLECT"> +
             ((assign<&ast::Collect::expr_>(ref<BindingList>) +
               (CountInto | (option(CollectAggregate) + option(CollectInto)))) |
              (CollectAggregate + option(CollectInto)) | CountInto) +
             option(assign<&ast::Collect::options>(ref<Options>))> {};

  struct Window
      : rule<option(assign<&ast::Window::letBefore>(ref<LetList>)) +
             ikw<"WINDOW"> +
             option(assign<&ast::Window::expr_>(ref<Expr>) + ikw<"WITH">) +
             assign<&ast::Window::spec>(ref<ObjectLit>) +
             assign<&ast::Window::aggregate>(ref<AggregateAssign>) +
             many(lit<","> + append<&ast::Window::moreAggregates>(ref<AggregateBinding>))> {};

  static constexpr auto PipelineOperation = ref<Collect> | ref<Filter> |
                                           ref<Sort> | ref<Limit> | ref<Window>;
  struct ForOp
      : rule<(assign<&ast::ForOp::forIn>(ref<ForIn>) +
              many(append<&ast::ForOp::operations>(PipelineOperation))) |
             some(append<&ast::ForOp::operations>(PipelineOperation))> {};

  struct ReturnExpr
      : rule<option(assign<&ast::ReturnExpr::letBefore>(ref<LetList>)) +
             ikw<"RETURN"> +
             option(flag<&ast::ReturnExpr::distinct>(ikw<"DISTINCT">)) +
             assign<&ast::ReturnExpr::value>(ref<Expr>)> {};

  struct SimpleQuery
      : rule<create<ast::SimpleQuery>(
            option(ikw<"WITH"> +
                   some(append<&ast::SimpleQuery::with_>(ref<Collection>),
                        option(lit<",">))) +
            many(append<&ast::SimpleQuery::forOps>(ref<ForOp>)) +
            assign<&ast::SimpleQuery::return_>(ref<ReturnExpr>))> {};

  struct TraversalQuery
      : rule<create<ast::TraversalQuery>(
            option(ikw<"WITH"> +
                   some(append<&ast::TraversalQuery::with_>(ref<Collection>),
                        option(lit<",">))) +
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
                   option(assign<&ast::TraversalQuery::pruneVar>(ref<ID>) + lit<"=">) +
                   assign<&ast::TraversalQuery::prune>(ref<Expr>)) +
            option(assign<&ast::TraversalQuery::options>(ref<Options>)) +
            many(append<&ast::TraversalQuery::filters>(ref<Filter>)) +
            assign<&ast::TraversalQuery::return_>(ref<ReturnExpr>))> {};

  struct InsertOp
      : rule<ikw<"INSERT"> + assign<&ast::InsertOp::doc>(ref<Expr>) +
             (flag<&ast::InsertOp::isInto>(ikw<"INTO">) | ikw<"IN">) +
             assign<&ast::InsertOp::target>(ref<Collection>) +
             option(assign<&ast::InsertOp::options>(ref<Options>))> {};

  struct UpdateOp
      : rule<ikw<"UPDATE"> +
             option(assign<&ast::UpdateOp::key>(ref<Expr>) + ikw<"WITH">) +
             assign<&ast::UpdateOp::doc>(ref<Expr>) + (ikw<"IN"> | ikw<"INTO">) +
             assign<&ast::UpdateOp::target>(ref<Collection>) +
             option(assign<&ast::UpdateOp::options>(ref<Options>))> {};

  struct ReplaceOp
      : rule<ikw<"REPLACE"> +
             option(assign<&ast::ReplaceOp::key>(ref<Expr>) + ikw<"WITH">) +
             assign<&ast::ReplaceOp::doc>(ref<Expr>) + (ikw<"IN"> | ikw<"INTO">) +
             assign<&ast::ReplaceOp::target>(ref<Collection>) +
             option(assign<&ast::ReplaceOp::options>(ref<Options>))> {};

  struct RemoveOp
      : rule<ikw<"REMOVE"> + assign<&ast::RemoveOp::key>(ref<Expr>) +
             (ikw<"IN"> | ikw<"INTO">) + assign<&ast::RemoveOp::target>(ref<Collection>) +
             option(assign<&ast::RemoveOp::options>(ref<Options>))> {};

  struct UpsertOp
      : rule<ikw<"UPSERT"> +
             ((flag<&ast::UpsertOp::isFilter>(ikw<"FILTER">) +
               assign<&ast::UpsertOp::search>(ref<Expr>)) |
              assign<&ast::UpsertOp::search>(
                  ref<ObjectLit> | ref<BindParamRef> |
                  create<ast::CollectionRef>(
                      assign<&ast::CollectionRef::name>(ref<BIND_PARAMETER_COLL>)))) +
             ikw<"INSERT"> + assign<&ast::UpsertOp::insert_>(ref<Expr>) +
             (flag<&ast::UpsertOp::isUpdate>(ikw<"UPDATE">) |
              ikw<"REPLACE">) +
             assign<&ast::UpsertOp::modify>(ref<Expr>) + (ikw<"IN"> | ikw<"INTO">) +
             assign<&ast::UpsertOp::target>(ref<Collection>) +
             option(assign<&ast::UpsertOp::options>(ref<Options>))> {};

  struct DataModificationQuery
      : rule<create<ast::DataModificationQuery>(
            option(
                ikw<"WITH"> +
                some(append<&ast::DataModificationQuery::with_>(
                         ref<Collection>),
                     option(lit<",">))) +
            many(append<&ast::DataModificationQuery::forOps>(ref<ForOp>)) +
            option(assign<&ast::DataModificationQuery::letBefore>(ref<LetList>)) +
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
  using ignored = WS;
  using hidden = list<LINE_COMMENT, SQL_COMMENT>;
};

}  // namespace aql
