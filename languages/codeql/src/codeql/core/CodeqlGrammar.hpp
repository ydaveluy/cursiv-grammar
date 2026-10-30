// The codeql language: its grammar. Formulas and expressions follow CodeQL's
// stated precedence table (tightest first: negation, conditional,
// conjunction, disjunction, implication; and casts, unary +/-, binary */%,
// binary +/-).
#pragma once

#include <cstdint>
#include <memory_resource>
#include <span>

#include "codeql/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace codeql {

struct Codeql : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct LINE_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  // A doc comment '/** ... */' is a '/* ... */' run too, so one hidden
  // terminal covers both forms; nothing downstream tells them apart.
  struct BLOCK_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  struct LOWERID : terminal<cr<"a-z"> + many(w)> {};
  struct UPPERID : terminal<cr<"A-Z"> + many(w)> {};
  struct ATLOWERID : terminal<lit<"@"> + cr<"a-z"> + many(w)> {};

  struct INT_LITERAL : terminal<as<std::int64_t>(some(d))> {};
  struct FLOAT_LITERAL : terminal<as<double>(some(d) + lit<"."> + some(d))> {};

  static constexpr auto escape =
      lit<"\\"> + option(lit<"u005c">) +
      (cr<R"(btnfr"'\\\\)"> |
       (option(option(cr<"0-3">) + cr<"0-7">) + cr<"0-7">));
  inline static constexpr auto unquote = [](std::string_view t,
                                            std::pmr::memory_resource* mem) {
    const auto body = t.substr(1, t.size() - 2);
    if (body.find('\\') == std::string_view::npos) return body;
    auto room = cursiv::text_room(mem, body.size());
    std::size_t n = 0;
    for (std::size_t i = 0; i < body.size(); ++i) {
      if (body[i] != '\\' || i + 1 == body.size()) {
        room[n++] = body[i];
        continue;
      }
      // QL also permits the spelling \u005c for the escape introducer.
      if (body.substr(i + 1, 5) == "u005c") i += 5;
      if (++i == body.size()) {
        room[n++] = '\\';
        break;
      }
      const char c = body[i];
      switch (c) {
        case 'b':
          room[n++] = '\b';
          break;
        case 't':
          room[n++] = '\t';
          break;
        case 'n':
          room[n++] = '\n';
          break;
        case 'f':
          room[n++] = '\f';
          break;
        case 'r':
          room[n++] = '\r';
          break;
        default:
          if (c >= '0' && c <= '7') {
            unsigned v = c - '0';
            const int max = c <= '3' ? 3 : 2;
            for (int k = 1; k < max && i + 1 < body.size() &&
                            body[i + 1] >= '0' && body[i + 1] <= '7';
                 ++k)
              v = (v << 3) + (body[++i] - '0');
            room[n++] = static_cast<char>(v);
          } else
            room[n++] = c;
      }
    }
    return std::string_view(room.data(), n);
  };
  struct STRING_LITERAL
      : terminal<as<unquote>(lit<"\""> + many(escape | cr<R"(^"\\\r\n)">) +
                             lit<"\"">)> {};

  // The 27 words the language keeps usable as a name (`varname`,
  // `predicateName`, ...) although they also open an annotation or a pragma
  // argument list: one spelling, referenced from both positions.
  struct KeywordAllowed
      : rule<kw<"unique"> | kw<"implements"> | kw<"signature"> |
             kw<"default"> | kw<"abstract"> | kw<"cached"> | kw<"external"> |
             kw<"extensible"> | kw<"final"> | kw<"transient"> |
             kw<"library"> | kw<"private"> | kw<"deprecated"> |
             kw<"override"> | kw<"additional"> | kw<"query"> | kw<"pragma"> |
             kw<"inline"> | kw<"inline_late"> | kw<"noinline"> |
             kw<"nomagic"> | kw<"noopt"> | kw<"assume_small_delta"> |
             kw<"language"> | kw<"monotonicAggregates"> | kw<"bindingset"> |
             kw<"only_bind_out"> | kw<"only_bind_into"> | kw<"overlay">> {};

  struct Varname : rule<ref<LOWERID> | ref<KeywordAllowed>> {};
  struct SimpleId : rule<ref<LOWERID> | ref<UPPERID>> {};
  struct Classname : rule<ref<UPPERID>> {};
  struct PredicateName : rule<ref<LOWERID> | ref<KeywordAllowed>> {};
  struct PredicateCallName
      : rule<ref<LOWERID> | ref<ATLOWERID> | ref<UPPERID> |
             ref<KeywordAllowed> | kw<"any"> | kw<"none">> {};
  struct Closure : rule<lit<"*"> | lit<"+">> {};

  struct ModuleExpr;
  struct Type;
  struct Formula;
  struct Expr;

  struct PredicateArity
      : rule<option(ref<ModuleExpr> + lit<"::">) +
             ref<PredicateCallName> + lit<"/"> + some(d)> {};

  struct Argument
      : rule<ref<ModuleExpr> | ref<Type> | ref<PredicateArity>> {};
  struct Arguments
      : rule<lit<"<"> + ref<Argument> + many(lit<","> + ref<Argument>) +
             lit<">">> {};
  struct ModuleExpr
      : rule<ref<SimpleId> + option(ref<Arguments>) +
             many(lit<"::"> + ref<SimpleId> + option(ref<Arguments>))> {
  };

  struct Type
      : rule<(option(ref<ModuleExpr> + lit<"::">) + ref<Classname>) |
             ref<ATLOWERID> | kw<"boolean"> | kw<"date"> | kw<"float"> |
             kw<"int"> | kw<"string">> {};

  struct Param
      : rule<assign<&ast::Param::type>(ref<Type>) +
             assign<&ast::Param::name>(ref<Varname>)> {};

  struct PragmaArg
      : rule<kw<"inline"> | kw<"inline_late"> | kw<"noinline"> |
             kw<"nomagic"> | kw<"noopt"> | kw<"assume_small_delta">> {};

  struct SimpleAnnotation
      : rule<create<ast::Annotation>(assign<&ast::Annotation::name>(
            kw<"abstract"> | kw<"cached"> | kw<"external"> |
            kw<"extensible"> | kw<"final"> | kw<"transient"> |
            kw<"library"> | kw<"private"> | kw<"deprecated"> |
            kw<"override"> | kw<"additional"> | kw<"query">))> {};

  struct ArgsAnnotation
      : rule<create<ast::Annotation>(
                 assign<&ast::Annotation::name>(kw<"pragma">) + lit<"["> +
                 append<&ast::Annotation::args>(ref<PragmaArg>) +
                 many(lit<","> + append<&ast::Annotation::args>(ref<PragmaArg>)) +
                 lit<"]">) |
             create<ast::Annotation>(
                 assign<&ast::Annotation::name>(kw<"language">) + lit<"["> +
                 append<&ast::Annotation::args>(kw<"monotonicAggregates">) +
                 lit<"]">) |
             create<ast::Annotation>(
                 assign<&ast::Annotation::name>(kw<"bindingset">) + lit<"["> +
                 option(append<&ast::Annotation::args>(ref<Varname>) +
                        many(lit<","> +
                             append<&ast::Annotation::args>(ref<Varname>))) +
                 lit<"]">) |
             create<ast::Annotation>(
                 assign<&ast::Annotation::name>(kw<"overlay">) + lit<"["> +
                 append<&ast::Annotation::args>(
                     kw<"local?"> | kw<"local"> | kw<"global"> |
                     kw<"caller?"> | kw<"caller"> | kw<"discard_entity">) +
                 lit<"]">)> {};

  struct Annotation
      : rule<ref<SimpleAnnotation> | ref<ArgsAnnotation>> {};

  // ---- formulas ----




  struct Fparen : rule<lit<"("> + ref<Formula> + lit<")">> {};

  struct Range
      : rule<create<ast::RangeExpr>(
            lit<"["> + assign<&ast::RangeExpr::lo>(ref<Expr>) +
            lit<".."> + assign<&ast::RangeExpr::hi>(ref<Expr>) +
            lit<"]">)> {};

  struct SetLiteral
      : rule<create<ast::SetLiteralExpr>(
            lit<"["> + append<&ast::SetLiteralExpr::items>(ref<Expr>) +
            many(lit<","> + append<&ast::SetLiteralExpr::items>(ref<Expr>)) +
            option(lit<",">) + lit<"]">)> {};

  struct Quantified
      : rule<create<ast::QuantifiedFormula>(
                 assign<&ast::QuantifiedFormula::kind>(as<ast::QuantKind::exists>(kw<"exists">)) +
                 lit<"("> +
                 assign<&ast::QuantifiedFormula::existsExpr>(ref<Expr>) +
                 lit<")">) |
             create<ast::QuantifiedFormula>(
                 assign<&ast::QuantifiedFormula::kind>(as<ast::QuantKind::exists>(kw<"exists">)) +
                 lit<"("> +
                 many(append<&ast::QuantifiedFormula::vars>(ref<Param>),
                      lit<",">) +
                 option(lit<"|"> + assign<&ast::QuantifiedFormula::restriction>(
                                        ref<Formula>)) +
                 option(lit<"|"> +
                        assign<&ast::QuantifiedFormula::body>(ref<Formula>)) +
                 lit<")">) |
             create<ast::QuantifiedFormula>(
                 assign<&ast::QuantifiedFormula::kind>(as<ast::QuantKind::forall>(kw<"forall">)) +
                 lit<"("> +
                 many(append<&ast::QuantifiedFormula::vars>(ref<Param>),
                      lit<",">) +
                 option(lit<"|"> + assign<&ast::QuantifiedFormula::restriction>(
                                        ref<Formula>)) +
                 lit<"|"> +
                 assign<&ast::QuantifiedFormula::body>(ref<Formula>) +
                 lit<")">) |
             create<ast::QuantifiedFormula>(
                 assign<&ast::QuantifiedFormula::kind>(as<ast::QuantKind::forex>(kw<"forex">)) +
                 lit<"("> +
                 many(append<&ast::QuantifiedFormula::vars>(ref<Param>),
                      lit<",">) +
                 option(lit<"|"> + assign<&ast::QuantifiedFormula::restriction>(
                                        ref<Formula>)) +
                 lit<"|"> +
                 assign<&ast::QuantifiedFormula::body>(ref<Formula>) +
                 lit<")">)> {};

  struct Comparison
      : rule<create<ast::ComparisonFormula>(
            assign<&ast::ComparisonFormula::left>(ref<Expr>) +
            assign<&ast::ComparisonFormula::op>(
                as<ast::CompareOp::le>(lit<"<=">) | as<ast::CompareOp::ge>(lit<">=">) | as<ast::CompareOp::ne>(lit<"!=">) |
                as<ast::CompareOp::eq>(lit<"=">) | as<ast::CompareOp::lt>(lit<"<">) | as<ast::CompareOp::gt>(lit<">">)) +
            assign<&ast::ComparisonFormula::right>(ref<Expr>))> {};

  struct Instanceof
      : rule<create<ast::InstanceofFormula>(
            assign<&ast::InstanceofFormula::operand>(ref<Expr>) +
            kw<"instanceof"> +
            assign<&ast::InstanceofFormula::type>(ref<Type>))> {};

  struct InRange
      : rule<create<ast::InRangeFormula>(
            assign<&ast::InRangeFormula::operand>(ref<Expr>) + kw<"in"> +
            assign<&ast::InRangeFormula::range>(ref<Range> |
                                                ref<SetLiteral>))> {};

  struct Primary;

  struct CallFormula
      : rule<create<ast::CallFormula>(
                 option(assign<&ast::CallFormula::modulePath>(
                            ref<ModuleExpr>) +
                        lit<"::">) +
                 assign<&ast::CallFormula::name>(ref<PredicateCallName>) +
                 option(assign<&ast::CallFormula::closure>(ref<Closure>)) +
                 lit<"("> +
                 option(some(append<&ast::CallFormula::args>(ref<Expr>),
                             lit<",">)) +
                 lit<")">) |
             create<ast::MethodCallFormula>(
                 assign<&ast::MethodCallFormula::receiver>(ref<Primary>) +
                 lit<"."> +
                 assign<&ast::MethodCallFormula::name>(ref<PredicateName>) +
                 option(assign<&ast::MethodCallFormula::closure>(
                     ref<Closure>)) +
                 lit<"("> +
                 option(some(
                     append<&ast::MethodCallFormula::args>(ref<Expr>),
                     lit<",">)) +
                 lit<")">)> {};

  struct FormulaAtom
      : rule<ref<Fparen> | ref<Quantified> | ref<Comparison> |
             ref<Instanceof> | ref<InRange> | ref<CallFormula>> {};

  struct FormulaCond
      : rule<create<ast::IfThenFormula>(
                 kw<"if"> + assign<&ast::IfThenFormula::cond>(ref<Formula>) +
                 kw<"then"> +
                 assign<&ast::IfThenFormula::then_>(ref<Formula>) +
                 kw<"else"> + assign<&ast::IfThenFormula::else_>(
                                  ref<FormulaCond>)) |
             ref<FormulaAtom>> {};

  struct FormulaNeg
      : rule<create<ast::NegatedFormula>(
                 kw<"not"> +
                 assign<&ast::NegatedFormula::operand>(ref<FormulaNeg>)) |
             ref<FormulaCond>> {};

  struct Formula
      : infix<FormulaNeg, ast::BinFormula,
              levels(left(as<ast::LogicOp::and_>(kw<"and">)),  //
                     left(as<ast::LogicOp::or_>(kw<"or">)),   //
                     nonassoc(as<ast::LogicOp::implies>(kw<"implies">)))> {};

  // ---- expressions ----


  static constexpr auto AggId =
      as<ast::AggKind::avg>(kw<"avg">) | as<ast::AggKind::concat>(kw<"concat">) | as<ast::AggKind::count>(kw<"count">) | as<ast::AggKind::max>(kw<"max">) |
      as<ast::AggKind::min>(kw<"min">) | as<ast::AggKind::rank>(kw<"rank">) | as<ast::AggKind::strictconcat>(kw<"strictconcat">) |
      as<ast::AggKind::strictcount>(kw<"strictcount">) | as<ast::AggKind::strictsum>(kw<"strictsum">) | as<ast::AggKind::sum>(kw<"sum">);

  struct Eparen : rule<lit<"("> + ref<Expr> + lit<")">> {};

  struct Literal
      : rule<create<ast::Literal>(assign<&ast::Literal::value>(
            as<false>(kw<"false">) | as<true>(kw<"true">) | ref<INT_LITERAL> |
            ref<FLOAT_LITERAL> | ref<STRING_LITERAL>))> {};

  struct Variable
      : rule<create<ast::VariableExpr>(
                 assign<&ast::VariableExpr::name>(ref<Varname>)) |
             create<ast::VariableExpr>(
                 assign<&ast::VariableExpr::kind>(as<ast::VarKind::this_>(kw<"this">))) |
             create<ast::VariableExpr>(
                 assign<&ast::VariableExpr::kind>(as<ast::VarKind::result>(kw<"result">)))> {};

  struct Super
      : rule<create<ast::SuperExpr>(kw<"super">) |
             create<ast::SuperExpr>(
                 assign<&ast::SuperExpr::type>(ref<Type>) + lit<"."> +
                 kw<"super">)> {};

  struct AsExpr
      : rule<create<ast::AsExpr>(
            assign<&ast::AsExpr::value>(ref<Expr>) +
            option(kw<"as"> + assign<&ast::AsExpr::alias>(ref<LOWERID>)))> {};

  struct AggOrderTerm
      : rule<create<ast::AggOrderTerm>(
            assign<&ast::AggOrderTerm::key>(ref<Expr>) +
            option(flag<&ast::AggOrderTerm::descending>(kw<"desc">) |
                   kw<"asc">))> {};

  struct Aggregation
      : rule<create<ast::AggregationExpr>(
            (assign<&ast::AggregationExpr::kind>(AggId) +
             option(lit<"["> +
                    assign<&ast::AggregationExpr::rankExpr>(ref<Expr>) +
                    lit<"]">) +
             lit<"("> +
             ((many(append<&ast::AggregationExpr::vars>(ref<Param>),
                    lit<",">) +
               option(lit<"|"> +
                      option(assign<&ast::AggregationExpr::restriction>(
                          ref<Formula>)) +
                      option(lit<"|"> +
                             many(append<&ast::AggregationExpr::results>(
                                      ref<AsExpr>),
                                  lit<",">) +
                             option(kw<"order"> + kw<"by"> +
                                    many(append<&ast::AggregationExpr::orderBys>(
                                             ref<AggOrderTerm>),
                                         lit<",">))))) |
              (many(append<&ast::AggregationExpr::results>(ref<AsExpr>),
                    lit<",">) +
               option(kw<"order"> + kw<"by"> +
                      many(append<&ast::AggregationExpr::orderBys>(
                               ref<AggOrderTerm>),
                           lit<",">)))) +
             lit<")">) |
            (assign<&ast::AggregationExpr::kind>(as<ast::AggKind::unique>(kw<"unique">)) +
             lit<"("> +
             many(append<&ast::AggregationExpr::vars>(ref<Param>),
                  lit<",">) +
             lit<"|"> +
             option(assign<&ast::AggregationExpr::restriction>(
                 ref<Formula>)) +
             option(lit<"|"> +
                    many(append<&ast::AggregationExpr::results>(
                             ref<AsExpr>),
                         lit<",">)) +
             lit<")">))> {};

  struct PragmaExpr
      : rule<create<ast::PragmaExpr>(
            kw<"pragma"> + lit<"["> +
            (flag<&ast::PragmaExpr::bindOut>(kw<"only_bind_out">) |
             kw<"only_bind_into">) +
            lit<"]"> + lit<"("> +
            assign<&ast::PragmaExpr::operand>(ref<Expr>) + lit<")">)> {};

  struct CallExpr
      : rule<create<ast::CallExpr>(
            option(assign<&ast::CallExpr::modulePath>(ref<ModuleExpr>) +
                   lit<"::">) +
            assign<&ast::CallExpr::name>(ref<PredicateCallName>) +
            option(assign<&ast::CallExpr::closure>(ref<Closure>)) +
            lit<"("> +
            option(some(append<&ast::CallExpr::args>(ref<Expr>), lit<",">)) +
            lit<")">)> {};

  struct Any
      : rule<create<ast::AnyExpr>(
            kw<"any"> + lit<"("> +
            many(append<&ast::AnyExpr::vars>(ref<Param>), lit<",">) +
            option(lit<"|"> +
                   option(assign<&ast::AnyExpr::restriction>(ref<Formula>)) +
                   option(lit<"|"> +
                          assign<&ast::AnyExpr::result>(ref<Expr>))) +
            lit<")">)> {};

  struct None
      : rule<create<ast::NoneExpr> + kw<"none"> + lit<"("> + lit<")">> {};

  struct PrimaryBase
      : rule<ref<Eparen> | ref<Literal> | ref<Variable> |
             ref<Super> | ref<Aggregation> | ref<PragmaExpr> |
             ref<CallExpr> | ref<Any> | ref<None> |
             ref<Range> | ref<SetLiteral>> {};

  struct Primary
      : rule<ref<PrimaryBase> +
             many(nest<ast::PostfixCastExpr, &ast::PostfixCastExpr::operand>(
                      lit<"."> + lit<"("> +
                      assign<&ast::PostfixCastExpr::type>(ref<Type>) +
                      lit<")">) |
                  nest<ast::MethodCallExpr, &ast::MethodCallExpr::receiver>(
                      lit<"."> +
                      assign<&ast::MethodCallExpr::name>(ref<PredicateName>) +
                      option(assign<&ast::MethodCallExpr::closure>(
                          ref<Closure>)) +
                      lit<"("> +
                      option(some(append<&ast::MethodCallExpr::args>(
                                      ref<Expr>),
                                  lit<",">)) +
                      lit<")">))> {};

  struct DontCare : rule<create<ast::DontCareExpr> + lit<"_">> {};

  struct UnaryExpr;
  struct CastOrPrimary
      : rule<create<ast::CastExpr>(
                 lit<"("> + assign<&ast::CastExpr::type>(ref<Type>) +
                 lit<")"> +
                 assign<&ast::CastExpr::operand>(ref<UnaryExpr>)) |
             ref<DontCare> | ref<Primary>> {};

  struct UnaryExpr
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(as<ast::SignOp::plus>(lit<"+">) | as<ast::SignOp::minus>(lit<"-">)) +
                 assign<&ast::UnaryExpr::operand>(ref<UnaryExpr>)) |
             ref<CastOrPrimary>> {};

  struct Expr
      : infix<UnaryExpr, ast::BinExpr,
              levels(left(as<ast::ArithOp::mul>(lit<"*">) | as<ast::ArithOp::div>(lit<"/">) | as<ast::ArithOp::mod>(lit<"%">)),  //
                     left(as<ast::ArithOp::add>(lit<"+">) | as<ast::ArithOp::sub>(lit<"-">)))> {};

  // ---- top-level entities ----

  struct OrderByTerm
      : rule<create<ast::OrderByTerm>(
            assign<&ast::OrderByTerm::name>(ref<LOWERID>) +
            option(flag<&ast::OrderByTerm::descending>(kw<"desc">) |
                   kw<"asc">))> {};

  struct Select
      : rule<create<ast::Select>(
            option(kw<"from"> +
                   many(append<&ast::Select::fromVars>(ref<Param>),
                        lit<",">)) +
            option(kw<"where"> +
                   assign<&ast::Select::where>(ref<Formula>)) +
            kw<"select"> +
            some(append<&ast::Select::results>(ref<AsExpr>), lit<",">) +
            option(kw<"order"> + kw<"by"> +
                   some(append<&ast::Select::orderBys>(ref<OrderByTerm>),
                        lit<",">)))> {};

  struct QualId : rule<some(ref<SimpleId>, lit<".">)> {};

  struct ImportModuleExpr
      : rule<ref<QualId> + option(ref<Arguments>) +
             many(lit<"::"> + ref<SimpleId> + option(ref<Arguments>))> {
  };

  struct Import
      : rule<create<ast::ImportDecl>(
            many(append<&ast::ImportDecl::annotations>(ref<Annotation>)) +
            kw<"import"> +
            assign<&ast::ImportDecl::modulePath>(ref<ImportModuleExpr>) +
            option(kw<"as"> +
                   assign<&ast::ImportDecl::alias>(ref<SimpleId>)))> {};

  struct SignatureExpr
      : rule<option(ref<ModuleExpr> + lit<"::">) + ref<SimpleId> +
             option((lit<"/"> + some(d)) | ref<Arguments>)> {};

  struct SignatureParam
      : rule<create<ast::Param>(
            assign<&ast::Param::type>(ref<SignatureExpr>) +
            assign<&ast::Param::name>(ref<SimpleId>))> {};

  static constexpr auto PredicateSignatureAnnotations =
      many(append<&ast::PredicateSignature::annotations>(ref<Annotation>));
  static constexpr auto PredicateSignatureContent =
      option(assign<&ast::PredicateSignature::resultType>(ref<Type>) |
             kw<"predicate">) +
      assign<&ast::PredicateSignature::name>(ref<PredicateName>) +
      lit<"("> +
      many(append<&ast::PredicateSignature::params>(ref<Param>), lit<",">) +
      lit<")"> + lit<";">;
  struct PredicateSignatureBody
      : rule<create<ast::PredicateSignature>(PredicateSignatureAnnotations +
                                             PredicateSignatureContent)> {};

  struct AliasBody
      : rule<create<ast::AliasBody>(
            assign<&ast::AliasBody::charPred>(ref<PredicateCallName>) +
            lit<"("> +
            option(append<&ast::AliasBody::charPredArgs>(
                       ref<PredicateArity>) +
                   many(lit<","> + append<&ast::AliasBody::charPredArgs>(
                                       ref<PredicateArity>))) +
            lit<")"> + lit<"("> +
            option(append<&ast::AliasBody::args>(ref<Expr>) +
                   many(lit<","> +
                        append<&ast::AliasBody::args>(ref<Expr>))) +
            lit<")">)> {};

  struct Predicate
      : rule<create<ast::Predicate>(
            many(append<&ast::Predicate::annotations>(ref<Annotation>)) +
            option(assign<&ast::Predicate::resultType>(ref<Type>) |
                   kw<"predicate">) +
            assign<&ast::Predicate::name>(ref<PredicateName>) + lit<"("> +
            many(append<&ast::Predicate::params>(ref<Param>), lit<",">) +
            lit<")"> +
            (lit<";"> |
             (lit<"{"> + assign<&ast::Predicate::body>(ref<Formula>) +
              lit<"}">) |
             (lit<"="> +
              assign<&ast::Predicate::aliasBody>(ref<AliasBody>))))> {};

  struct PredicateSignature
      : rule<create<ast::PredicateSignature>(PredicateSignatureAnnotations +
                                             kw<"signature"> +
                                             PredicateSignatureContent)> {};

  struct DefaultPredicate
      : rule<create<ast::DefaultPredicate>(
            many(append<&ast::DefaultPredicate::annotations>(
                     ref<Annotation>)) +
            kw<"default"> +
            option(assign<&ast::DefaultPredicate::resultType>(ref<Type>) |
                   kw<"predicate">) +
            assign<&ast::DefaultPredicate::name>(ref<PredicateName>) +
            lit<"("> +
            many(append<&ast::DefaultPredicate::params>(ref<Param>),
                 lit<",">) +
            lit<")"> + lit<"{"> +
            assign<&ast::DefaultPredicate::body>(ref<Formula>) + lit<"}">)> {
  };

  static constexpr auto TypeSignatureAnnotations =
      many(append<&ast::TypeSignature::annotations>(ref<Annotation>));
  static constexpr auto TypeSignatureContent =
      kw<"class"> + assign<&ast::TypeSignature::name>(ref<Classname>) +
      option(kw<"extends"> +
             some(append<&ast::TypeSignature::extends_>(ref<Type>),
                  lit<",">)) +
      (lit<";"> | (lit<"{"> +
                   many(append<&ast::TypeSignature::members>(
                       ref<PredicateSignatureBody>)) +
                   lit<"}">));
  struct SignatureType
      : rule<create<ast::TypeSignature>(TypeSignatureAnnotations +
                                        TypeSignatureContent)> {};
  struct TypeSignature : rule<create<ast::TypeSignature>(
                                 TypeSignatureAnnotations + kw<"signature"> +
                                 TypeSignatureContent)> {};

  struct ModuleSignature
      : rule<create<ast::ModuleSignature>(
            many(append<&ast::ModuleSignature::annotations>(
                     ref<Annotation>)) +
            kw<"signature"> + kw<"module"> +
            assign<&ast::ModuleSignature::name>(ref<UPPERID>) +
            option(lit<"<"> +
                   append<&ast::ModuleSignature::params>(ref<SignatureParam>) +
                   many(lit<","> + append<&ast::ModuleSignature::params>(
                                       ref<SignatureParam>)) +
                   lit<">">) +
            lit<"{"> +
            many(append<&ast::ModuleSignature::body>(
                     ref<PredicateSignatureBody>) |
                 append<&ast::ModuleSignature::body>(ref<DefaultPredicate>) |
                 append<&ast::ModuleSignature::body>(ref<SignatureType>)) +
            lit<"}">)> {};

  struct Signature
      : rule<ref<PredicateSignature> | ref<TypeSignature> |
             ref<ModuleSignature>> {};

  struct DtBranch
      : rule<create<ast::DtBranch>(
            assign<&ast::DtBranch::name>(ref<UPPERID>) + lit<"("> +
            many(append<&ast::DtBranch::params>(ref<Param>), lit<",">) +
            lit<")"> +
            option(lit<"{"> +
                   assign<&ast::DtBranch::restriction>(ref<Formula>) +
                   lit<"}">))> {};

  struct Datatype
      : rule<create<ast::Datatype>(
            many(append<&ast::Datatype::annotations>(ref<Annotation>)) +
            kw<"newtype"> + assign<&ast::Datatype::name>(ref<UPPERID>) +
            lit<"="> +
            append<&ast::Datatype::branches>(ref<DtBranch>) +
            many(kw<"or"> + append<&ast::Datatype::branches>(ref<DtBranch>)))> {
  };

  struct TypeUnion
      : rule<create<ast::TypeUnion>(
            many(append<&ast::TypeUnion::annotations>(ref<Annotation>)) +
            kw<"class"> + assign<&ast::TypeUnion::name>(ref<UPPERID>) +
            lit<"="> + append<&ast::TypeUnion::branches>(ref<Type>) +
            many(kw<"or"> + append<&ast::TypeUnion::branches>(ref<Type>)) +
            lit<";">)> {};

  struct Field
      : rule<create<ast::Field>(
            many(append<&ast::Field::annotations>(ref<Annotation>)) +
            assign<&ast::Field::type>(ref<Type>) +
            assign<&ast::Field::name>(ref<Varname>) + lit<";">)> {};

  struct CharacteristicPredicate
      : rule<create<ast::CharacteristicPredicate>(
            many(append<&ast::CharacteristicPredicate::annotations>(
                     ref<Annotation>)) +
            assign<&ast::CharacteristicPredicate::name>(ref<Classname>) +
            lit<"("> + lit<")"> + lit<"{"> +
            assign<&ast::CharacteristicPredicate::body>(ref<Formula>) +
            lit<"}">)> {};

  // A named rule collapses a choice to the alternatives' common base before
  // append<> sees it, which loses which concrete class matched; a spliced
  // expression keeps each branch's own type so it lands in the right slot
  // of the heterogeneous `members` list.
  static constexpr auto Member = ref<CharacteristicPredicate> |
                                 ref<Predicate> | ref<Field>;

  struct Class
      : rule<create<ast::Class_>(
            many(append<&ast::Class_::annotations>(ref<Annotation>)) +
            kw<"class"> + assign<&ast::Class_::name>(ref<Classname>) +
            option(kw<"extends"> +
                   append<&ast::Class_::extends_>(ref<Type>) +
                   many(lit<","> +
                        append<&ast::Class_::extends_>(ref<Type>))) +
            option(kw<"instanceof"> +
                   append<&ast::Class_::instanceof_>(ref<Type>) +
                   many(lit<","> +
                        append<&ast::Class_::instanceof_>(ref<Type>))) +
            lit<"{"> + many(append<&ast::Class_::members>(Member)) +
            lit<"}">)> {};

  struct TopLevel;

  struct ModuleSignatureExpr
      : rule<option(ref<ModuleExpr> + lit<"::">) + ref<UPPERID> +
             option(ref<Arguments>)> {};

  // Same reasoning as `Member` above: a helper that writes into the
  // enclosing node's member cannot be its own rule<> (that gives the node a
  // second head), so this is a spliced expression, not a ref<>'d rule.
  static constexpr auto ModuleImplements =
      kw<"implements"> +
      append<&ast::Module::implements_>(ref<ModuleSignatureExpr>) +
      many(lit<","> +
           append<&ast::Module::implements_>(ref<ModuleSignatureExpr>));

  struct Module
      : rule<create<ast::Module>(
            many(append<&ast::Module::annotations>(ref<Annotation>)) +
            kw<"module"> + assign<&ast::Module::name>(ref<SimpleId>) +
            option(lit<"<"> +
                   append<&ast::Module::params>(ref<SignatureParam>) +
                   many(lit<","> +
                        append<&ast::Module::params>(ref<SignatureParam>)) +
                   lit<">">) +
            option(ModuleImplements) + lit<"{"> +
            many(append<&ast::Module::body>(ref<TopLevel>)) + lit<"}">)> {
  };

  struct AliasPredicate
      : rule<create<ast::AliasPredicate>(
            many(append<&ast::AliasPredicate::annotations>(
                     ref<Annotation>)) +
            kw<"predicate">
                + assign<&ast::AliasPredicate::name>(ref<PredicateName>) +
            lit<"="> +
            assign<&ast::AliasPredicate::target>(ref<PredicateArity>) +
            lit<";">)> {};

  struct AliasClass
      : rule<create<ast::AliasClass>(
            many(append<&ast::AliasClass::annotations>(ref<Annotation>)) +
            kw<"class"> + assign<&ast::AliasClass::name>(ref<Classname>) +
            lit<"="> + assign<&ast::AliasClass::type>(ref<Type>) +
            lit<";">)> {};

  struct AliasModule
      : rule<create<ast::AliasModule>(
            many(append<&ast::AliasModule::annotations>(ref<Annotation>)) +
            kw<"module"> +
            assign<&ast::AliasModule::name>(ref<SimpleId>) + lit<"="> +
            assign<&ast::AliasModule::moduleExpr>(ref<ModuleExpr>) +
            lit<";">)> {};

  struct Alias
      : rule<ref<AliasPredicate> | ref<AliasClass> |
             ref<AliasModule>> {};

  struct TopLevel
      : rule<ref<Import> | ref<Predicate> | ref<Signature> |
             ref<Datatype> | ref<TypeUnion> | ref<Class> |
             ref<Module> | ref<Alias> | ref<Select>> {};

  struct Ql
      : rule<create<ast::Document>(
            many(append<&ast::Document::body>(ref<TopLevel>)))> {};

  using entry = Ql;
  using ignored = WS;
  using hidden = list<LINE_COMMENT, BLOCK_COMMENT>;
};

}  // namespace codeql
