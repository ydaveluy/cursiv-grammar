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

  struct INT_LITERAL : terminal<some(d), std::int64_t> {};
  struct FLOAT_LITERAL : terminal<some(d) + lit<"."> + some(d), double> {};

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
  using Unquoted = cursiv::text_conv<unquote>;
  struct STRING_LITERAL
      : terminal<lit<"\""> + many(escape | cr<R"(^"\\\r\n)">) + lit<"\"">,
                 Unquoted> {};

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
             kw<"only_bind_out"> | kw<"only_bind_into">> {};

  struct Varname : rule<ref<LOWERID> | ref<KeywordAllowed>> {};
  struct SimpleIdRule : rule<ref<LOWERID> | ref<UPPERID>> {};
  struct ClassnameRule : rule<ref<UPPERID>> {};
  struct PredicateNameRule : rule<ref<LOWERID> | ref<KeywordAllowed>> {};
  struct PredicateCallNameRule
      : rule<ref<LOWERID> | ref<ATLOWERID> | ref<UPPERID> |
             ref<KeywordAllowed> | kw<"any"> | kw<"none">> {};
  struct ClosureRule : rule<lit<"*"> | lit<"+">> {};

  struct ModuleExprRule;
  struct TypeRule;
  struct FormulaRule;
  struct ExprRule;

  struct PredicateArityRule
      : rule<option(ref<ModuleExprRule> + lit<"::">) +
             ref<PredicateCallNameRule> + lit<"/"> + some(d)> {};

  struct ArgumentRule
      : rule<ref<ModuleExprRule> | ref<TypeRule> | ref<PredicateArityRule>> {};
  struct ArgumentsRule
      : rule<lit<"<"> + ref<ArgumentRule> + many(lit<","> + ref<ArgumentRule>) +
             lit<">">> {};
  struct ModuleExprRule
      : rule<ref<SimpleIdRule> + option(ref<ArgumentsRule>) +
             many(lit<"::"> + ref<SimpleIdRule> + option(ref<ArgumentsRule>))> {
  };

  struct TypeRule
      : rule<(option(ref<ModuleExprRule> + lit<"::">) + ref<ClassnameRule>) |
             ref<ATLOWERID> | kw<"boolean"> | kw<"date"> | kw<"float"> |
             kw<"int"> | kw<"string">> {};

  struct ParamRule
      : rule<assign<&ast::Param::type>(ref<TypeRule>) +
             assign<&ast::Param::name>(ref<Varname>)> {};

  struct PragmaArgRule
      : rule<kw<"inline"> | kw<"inline_late"> | kw<"noinline"> |
             kw<"nomagic"> | kw<"noopt"> | kw<"assume_small_delta">> {};

  struct SimpleAnnotationRule
      : rule<create<ast::Annotation>(assign<&ast::Annotation::name>(
            kw<"abstract"> | kw<"cached"> | kw<"external"> |
            kw<"extensible"> | kw<"final"> | kw<"transient"> |
            kw<"library"> | kw<"private"> | kw<"deprecated"> |
            kw<"override"> | kw<"additional"> | kw<"query">))> {};

  struct ArgsAnnotationRule
      : rule<create<ast::Annotation>(
                 assign<&ast::Annotation::name>(kw<"pragma">) + lit<"["> +
                 append<&ast::Annotation::args>(ref<PragmaArgRule>) +
                 many(lit<","> + append<&ast::Annotation::args>(ref<PragmaArgRule>)) +
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
                 lit<"]">)> {};

  struct AnnotationRule
      : rule<ref<SimpleAnnotationRule> | ref<ArgsAnnotationRule>> {};

  // ---- formulas ----

  struct KwExists : terminal<kw<"exists">, value<ast::QuantKind::exists>> {};
  struct KwForall : terminal<kw<"forall">, value<ast::QuantKind::forall>> {};
  struct KwForex : terminal<kw<"forex">, value<ast::QuantKind::forex>> {};

  struct LogicAnd : terminal<kw<"and">, value<ast::LogicOp::and_>> {};
  struct LogicOr : terminal<kw<"or">, value<ast::LogicOp::or_>> {};
  struct LogicImplies : terminal<kw<"implies">, value<ast::LogicOp::implies>> {
  };

  struct CompareEq : terminal<lit<"=">, value<ast::CompareOp::eq>> {};
  struct CompareNe : terminal<lit<"!=">, value<ast::CompareOp::ne>> {};
  struct CompareLe : terminal<lit<"<=">, value<ast::CompareOp::le>> {};
  struct CompareGe : terminal<lit<">=">, value<ast::CompareOp::ge>> {};
  struct CompareLt : terminal<lit<"<">, value<ast::CompareOp::lt>> {};
  struct CompareGt : terminal<lit<">">, value<ast::CompareOp::gt>> {};

  struct FparenRule : rule<lit<"("> + ref<FormulaRule> + lit<")">> {};

  struct RangeRule
      : rule<create<ast::RangeExpr>(
            lit<"["> + assign<&ast::RangeExpr::lo>(ref<ExprRule>) +
            lit<".."> + assign<&ast::RangeExpr::hi>(ref<ExprRule>) +
            lit<"]">)> {};

  struct SetLiteralRule
      : rule<create<ast::SetLiteralExpr>(
            lit<"["> + append<&ast::SetLiteralExpr::items>(ref<ExprRule>) +
            many(lit<","> + append<&ast::SetLiteralExpr::items>(ref<ExprRule>)) +
            option(lit<",">) + lit<"]">)> {};

  struct QuantifiedRule
      : rule<create<ast::QuantifiedFormula>(
                 assign<&ast::QuantifiedFormula::kind>(ref<KwExists>) +
                 lit<"("> +
                 assign<&ast::QuantifiedFormula::existsExpr>(ref<ExprRule>) +
                 lit<")">) |
             create<ast::QuantifiedFormula>(
                 assign<&ast::QuantifiedFormula::kind>(ref<KwExists>) +
                 lit<"("> +
                 many(append<&ast::QuantifiedFormula::vars>(ref<ParamRule>),
                      lit<",">) +
                 option(lit<"|"> + assign<&ast::QuantifiedFormula::restriction>(
                                        ref<FormulaRule>)) +
                 option(lit<"|"> +
                        assign<&ast::QuantifiedFormula::body>(ref<FormulaRule>)) +
                 lit<")">) |
             create<ast::QuantifiedFormula>(
                 assign<&ast::QuantifiedFormula::kind>(ref<KwForall>) +
                 lit<"("> +
                 many(append<&ast::QuantifiedFormula::vars>(ref<ParamRule>),
                      lit<",">) +
                 option(lit<"|"> + assign<&ast::QuantifiedFormula::restriction>(
                                        ref<FormulaRule>)) +
                 lit<"|"> +
                 assign<&ast::QuantifiedFormula::body>(ref<FormulaRule>) +
                 lit<")">) |
             create<ast::QuantifiedFormula>(
                 assign<&ast::QuantifiedFormula::kind>(ref<KwForex>) +
                 lit<"("> +
                 many(append<&ast::QuantifiedFormula::vars>(ref<ParamRule>),
                      lit<",">) +
                 option(lit<"|"> + assign<&ast::QuantifiedFormula::restriction>(
                                        ref<FormulaRule>)) +
                 lit<"|"> +
                 assign<&ast::QuantifiedFormula::body>(ref<FormulaRule>) +
                 lit<")">)> {};

  struct ComparisonRule
      : rule<create<ast::ComparisonFormula>(
            assign<&ast::ComparisonFormula::left>(ref<ExprRule>) +
            assign<&ast::ComparisonFormula::op>(
                ref<CompareLe> | ref<CompareGe> | ref<CompareNe> |
                ref<CompareEq> | ref<CompareLt> | ref<CompareGt>) +
            assign<&ast::ComparisonFormula::right>(ref<ExprRule>))> {};

  struct InstanceofRule
      : rule<create<ast::InstanceofFormula>(
            assign<&ast::InstanceofFormula::operand>(ref<ExprRule>) +
            kw<"instanceof"> +
            assign<&ast::InstanceofFormula::type>(ref<TypeRule>))> {};

  struct InRangeRule
      : rule<create<ast::InRangeFormula>(
            assign<&ast::InRangeFormula::operand>(ref<ExprRule>) + kw<"in"> +
            assign<&ast::InRangeFormula::range>(ref<RangeRule> |
                                                ref<SetLiteralRule>))> {};

  struct PrimaryRule;

  struct CallFormulaRule
      : rule<create<ast::CallFormula>(
                 option(assign<&ast::CallFormula::modulePath>(
                            ref<ModuleExprRule>) +
                        lit<"::">) +
                 assign<&ast::CallFormula::name>(ref<PredicateCallNameRule>) +
                 option(assign<&ast::CallFormula::closure>(ref<ClosureRule>)) +
                 lit<"("> +
                 option(some(append<&ast::CallFormula::args>(ref<ExprRule>),
                             lit<",">)) +
                 lit<")">) |
             create<ast::MethodCallFormula>(
                 assign<&ast::MethodCallFormula::receiver>(ref<PrimaryRule>) +
                 lit<"."> +
                 assign<&ast::MethodCallFormula::name>(ref<PredicateNameRule>) +
                 option(assign<&ast::MethodCallFormula::closure>(
                     ref<ClosureRule>)) +
                 lit<"("> +
                 option(some(
                     append<&ast::MethodCallFormula::args>(ref<ExprRule>),
                     lit<",">)) +
                 lit<")">)> {};

  struct FormulaAtomRule
      : rule<ref<FparenRule> | ref<QuantifiedRule> | ref<ComparisonRule> |
             ref<InstanceofRule> | ref<InRangeRule> | ref<CallFormulaRule>> {};

  struct FormulaCondRule
      : rule<create<ast::IfThenFormula>(
                 kw<"if"> + assign<&ast::IfThenFormula::cond>(ref<FormulaRule>) +
                 kw<"then"> +
                 assign<&ast::IfThenFormula::then_>(ref<FormulaRule>) +
                 kw<"else"> + assign<&ast::IfThenFormula::else_>(
                                  ref<FormulaCondRule>)) |
             ref<FormulaAtomRule>> {};

  struct FormulaNegRule
      : rule<create<ast::NegatedFormula>(
                 kw<"not"> +
                 assign<&ast::NegatedFormula::operand>(ref<FormulaNegRule>)) |
             ref<FormulaCondRule>> {};

  struct FormulaRule
      : infix<FormulaNegRule, &ast::BinFormula::left, &ast::BinFormula::op,
              &ast::BinFormula::right,
              (left(ref<LogicAnd>) >>       //
               left(ref<LogicOr>) >>        //
               right(ref<LogicImplies>))> {};

  // ---- expressions ----

  struct KwFalse : terminal<kw<"false">, value<false>> {};
  struct KwTrue : terminal<kw<"true">, value<true>> {};
  struct KwThis : terminal<kw<"this">, value<ast::VarKind::this_>> {};
  struct KwResult : terminal<kw<"result">, value<ast::VarKind::result>> {};
  struct SignPlus : terminal<lit<"+">, value<ast::SignOp::plus>> {};
  struct SignMinus : terminal<lit<"-">, value<ast::SignOp::minus>> {};
  struct ArithAdd : terminal<lit<"+">, value<ast::ArithOp::add>> {};
  struct ArithSub : terminal<lit<"-">, value<ast::ArithOp::sub>> {};
  struct ArithMul : terminal<lit<"*">, value<ast::ArithOp::mul>> {};
  struct ArithDiv : terminal<lit<"/">, value<ast::ArithOp::div>> {};
  struct ArithMod : terminal<lit<"%">, value<ast::ArithOp::mod>> {};

  struct AggIdAvg : terminal<kw<"avg">, value<ast::AggKind::avg>> {};
  struct AggIdConcat : terminal<kw<"concat">, value<ast::AggKind::concat>> {};
  struct AggIdCount : terminal<kw<"count">, value<ast::AggKind::count>> {};
  struct AggIdMax : terminal<kw<"max">, value<ast::AggKind::max>> {};
  struct AggIdMin : terminal<kw<"min">, value<ast::AggKind::min>> {};
  struct AggIdRank : terminal<kw<"rank">, value<ast::AggKind::rank>> {};
  struct AggIdStrictconcat
      : terminal<kw<"strictconcat">, value<ast::AggKind::strictconcat>> {};
  struct AggIdStrictcount
      : terminal<kw<"strictcount">, value<ast::AggKind::strictcount>> {};
  struct AggIdStrictsum
      : terminal<kw<"strictsum">, value<ast::AggKind::strictsum>> {};
  struct AggIdSum : terminal<kw<"sum">, value<ast::AggKind::sum>> {};
  struct AggIdUnique : terminal<kw<"unique">, value<ast::AggKind::unique>> {};
  static constexpr auto AggId =
      ref<AggIdAvg> | ref<AggIdConcat> | ref<AggIdCount> | ref<AggIdMax> |
      ref<AggIdMin> | ref<AggIdRank> | ref<AggIdStrictconcat> |
      ref<AggIdStrictcount> | ref<AggIdStrictsum> | ref<AggIdSum>;

  struct EparenRule : rule<lit<"("> + ref<ExprRule> + lit<")">> {};

  struct LiteralRule
      : rule<create<ast::Literal>(assign<&ast::Literal::value>(
            ref<KwFalse> | ref<KwTrue> | ref<INT_LITERAL> |
            ref<FLOAT_LITERAL> | ref<STRING_LITERAL>))> {};

  struct VariableRule
      : rule<create<ast::VariableExpr>(
                 assign<&ast::VariableExpr::name>(ref<Varname>)) |
             create<ast::VariableExpr>(
                 assign<&ast::VariableExpr::kind>(ref<KwThis>)) |
             create<ast::VariableExpr>(
                 assign<&ast::VariableExpr::kind>(ref<KwResult>))> {};

  struct SuperRule
      : rule<create<ast::SuperExpr>(kw<"super">) |
             create<ast::SuperExpr>(
                 assign<&ast::SuperExpr::type>(ref<TypeRule>) + lit<"."> +
                 kw<"super">)> {};

  struct AsExprRule
      : rule<create<ast::AsExpr>(
            assign<&ast::AsExpr::value>(ref<ExprRule>) +
            option(kw<"as"> + assign<&ast::AsExpr::alias>(ref<LOWERID>)))> {};

  struct AggOrderTermRule
      : rule<create<ast::AggOrderTerm>(
            assign<&ast::AggOrderTerm::key>(ref<ExprRule>) +
            option(flag<&ast::AggOrderTerm::descending>(kw<"desc">) |
                   kw<"asc">))> {};

  struct AggregationRule
      : rule<create<ast::AggregationExpr>(
            (assign<&ast::AggregationExpr::kind>(AggId) +
             option(lit<"["> +
                    assign<&ast::AggregationExpr::rankExpr>(ref<ExprRule>) +
                    lit<"]">) +
             lit<"("> +
             ((many(append<&ast::AggregationExpr::vars>(ref<ParamRule>),
                    lit<",">) +
               option(lit<"|"> +
                      option(assign<&ast::AggregationExpr::restriction>(
                          ref<FormulaRule>)) +
                      option(lit<"|"> +
                             many(append<&ast::AggregationExpr::results>(
                                      ref<AsExprRule>),
                                  lit<",">) +
                             option(kw<"order"> + kw<"by"> +
                                    many(append<&ast::AggregationExpr::orderBys>(
                                             ref<AggOrderTermRule>),
                                         lit<",">))))) |
              (many(append<&ast::AggregationExpr::results>(ref<AsExprRule>),
                    lit<",">) +
               option(kw<"order"> + kw<"by"> +
                      many(append<&ast::AggregationExpr::orderBys>(
                               ref<AggOrderTermRule>),
                           lit<",">)))) +
             lit<")">) |
            (assign<&ast::AggregationExpr::kind>(ref<AggIdUnique>) +
             lit<"("> +
             many(append<&ast::AggregationExpr::vars>(ref<ParamRule>),
                  lit<",">) +
             lit<"|"> +
             option(assign<&ast::AggregationExpr::restriction>(
                 ref<FormulaRule>)) +
             option(lit<"|"> +
                    many(append<&ast::AggregationExpr::results>(
                             ref<AsExprRule>),
                         lit<",">)) +
             lit<")">))> {};

  struct PragmaExprRule
      : rule<create<ast::PragmaExpr>(
            kw<"pragma"> + lit<"["> +
            (flag<&ast::PragmaExpr::bindOut>(kw<"only_bind_out">) |
             kw<"only_bind_into">) +
            lit<"]"> + lit<"("> +
            assign<&ast::PragmaExpr::operand>(ref<ExprRule>) + lit<")">)> {};

  struct CallExprRule
      : rule<create<ast::CallExpr>(
            option(assign<&ast::CallExpr::modulePath>(ref<ModuleExprRule>) +
                   lit<"::">) +
            assign<&ast::CallExpr::name>(ref<PredicateCallNameRule>) +
            option(assign<&ast::CallExpr::closure>(ref<ClosureRule>)) +
            lit<"("> +
            option(some(append<&ast::CallExpr::args>(ref<ExprRule>), lit<",">)) +
            lit<")">)> {};

  struct AnyRule
      : rule<create<ast::AnyExpr>(
            kw<"any"> + lit<"("> +
            many(append<&ast::AnyExpr::vars>(ref<ParamRule>), lit<",">) +
            option(lit<"|"> +
                   option(assign<&ast::AnyExpr::restriction>(ref<FormulaRule>)) +
                   option(lit<"|"> +
                          assign<&ast::AnyExpr::result>(ref<ExprRule>))) +
            lit<")">)> {};

  struct NoneRule
      : rule<create<ast::NoneExpr> + kw<"none"> + lit<"("> + lit<")">> {};

  struct PrimaryBaseRule
      : rule<ref<EparenRule> | ref<LiteralRule> | ref<VariableRule> |
             ref<SuperRule> | ref<AggregationRule> | ref<PragmaExprRule> |
             ref<CallExprRule> | ref<AnyRule> | ref<NoneRule> |
             ref<RangeRule> | ref<SetLiteralRule>> {};

  struct PrimaryRule
      : rule<ref<PrimaryBaseRule> +
             many(nest<ast::PostfixCastExpr, &ast::PostfixCastExpr::operand>(
                      lit<"."> + lit<"("> +
                      assign<&ast::PostfixCastExpr::type>(ref<TypeRule>) +
                      lit<")">) |
                  nest<ast::MethodCallExpr, &ast::MethodCallExpr::receiver>(
                      lit<"."> +
                      assign<&ast::MethodCallExpr::name>(ref<PredicateNameRule>) +
                      option(assign<&ast::MethodCallExpr::closure>(
                          ref<ClosureRule>)) +
                      lit<"("> +
                      option(some(append<&ast::MethodCallExpr::args>(
                                      ref<ExprRule>),
                                  lit<",">)) +
                      lit<")">))> {};

  struct DontCareRule : rule<create<ast::DontCareExpr> + lit<"_">> {};

  struct UnaryExprRule;
  struct CastOrPrimaryRule
      : rule<create<ast::CastExpr>(
                 lit<"("> + assign<&ast::CastExpr::type>(ref<TypeRule>) +
                 lit<")"> +
                 assign<&ast::CastExpr::operand>(ref<UnaryExprRule>)) |
             ref<DontCareRule> | ref<PrimaryRule>> {};

  struct UnaryExprRule
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ref<SignPlus> | ref<SignMinus>) +
                 assign<&ast::UnaryExpr::operand>(ref<UnaryExprRule>)) |
             ref<CastOrPrimaryRule>> {};

  struct ExprRule
      : infix<UnaryExprRule, &ast::BinExpr::left, &ast::BinExpr::op,
              &ast::BinExpr::right,
              (left(ref<ArithMul> | ref<ArithDiv> | ref<ArithMod>) >>  //
               left(ref<ArithAdd> | ref<ArithSub>))> {};

  // ---- top-level entities ----

  struct OrderByTermRule
      : rule<create<ast::OrderByTerm>(
            assign<&ast::OrderByTerm::name>(ref<LOWERID>) +
            option(flag<&ast::OrderByTerm::descending>(kw<"desc">) |
                   kw<"asc">))> {};

  struct SelectRule
      : rule<create<ast::Select>(
            option(kw<"from"> +
                   many(append<&ast::Select::fromVars>(ref<ParamRule>),
                        lit<",">)) +
            option(kw<"where"> +
                   assign<&ast::Select::where>(ref<FormulaRule>)) +
            kw<"select"> +
            some(append<&ast::Select::results>(ref<AsExprRule>), lit<",">) +
            option(kw<"order"> + kw<"by"> +
                   some(append<&ast::Select::orderBys>(ref<OrderByTermRule>),
                        lit<",">)))> {};

  struct QualIdRule : rule<some(ref<SimpleIdRule>, lit<".">)> {};

  struct ImportModuleExprRule
      : rule<ref<QualIdRule> +
             many(lit<"::"> + ref<SimpleIdRule> + option(ref<ArgumentsRule>))> {
  };

  struct ImportRule
      : rule<create<ast::ImportDecl>(
            many(append<&ast::ImportDecl::annotations>(ref<AnnotationRule>)) +
            kw<"import"> +
            assign<&ast::ImportDecl::modulePath>(ref<ImportModuleExprRule>) +
            option(kw<"as"> +
                   assign<&ast::ImportDecl::alias>(ref<SimpleIdRule>)))> {};

  struct SignatureExprRule
      : rule<option(ref<ModuleExprRule> + lit<"::">) + ref<SimpleIdRule> +
             option(lit<"/"> + some(d) | ref<ArgumentsRule>)> {};

  struct SignatureParamRule
      : rule<create<ast::Param>(
            assign<&ast::Param::type>(ref<SignatureExprRule>) +
            assign<&ast::Param::name>(ref<SimpleIdRule>))> {};

  static constexpr auto PredicateSignatureAnnotations =
      many(append<&ast::PredicateSignature::annotations>(ref<AnnotationRule>));
  static constexpr auto PredicateSignatureContent =
      option(assign<&ast::PredicateSignature::resultType>(ref<TypeRule>) |
             kw<"predicate">) +
      assign<&ast::PredicateSignature::name>(ref<PredicateNameRule>) +
      lit<"("> +
      many(append<&ast::PredicateSignature::params>(ref<ParamRule>), lit<",">) +
      lit<")"> + lit<";">;
  struct PredicateSignatureBodyRule
      : rule<create<ast::PredicateSignature>(PredicateSignatureAnnotations +
                                             PredicateSignatureContent)> {};

  struct AliasBodyRule
      : rule<create<ast::AliasBody>(
            assign<&ast::AliasBody::charPred>(ref<PredicateCallNameRule>) +
            lit<"("> +
            option(append<&ast::AliasBody::charPredArgs>(
                       ref<PredicateArityRule>) +
                   many(lit<","> + append<&ast::AliasBody::charPredArgs>(
                                       ref<PredicateArityRule>))) +
            lit<")"> + lit<"("> +
            option(append<&ast::AliasBody::args>(ref<ExprRule>) +
                   many(lit<","> +
                        append<&ast::AliasBody::args>(ref<ExprRule>))) +
            lit<")">)> {};

  struct PredicateRule
      : rule<create<ast::Predicate>(
            many(append<&ast::Predicate::annotations>(ref<AnnotationRule>)) +
            option(assign<&ast::Predicate::resultType>(ref<TypeRule>) |
                   kw<"predicate">) +
            assign<&ast::Predicate::name>(ref<PredicateNameRule>) + lit<"("> +
            many(append<&ast::Predicate::params>(ref<ParamRule>), lit<",">) +
            lit<")"> +
            (lit<";"> |
             (lit<"{"> + assign<&ast::Predicate::body>(ref<FormulaRule>) +
              lit<"}">) |
             (lit<"="> +
              assign<&ast::Predicate::aliasBody>(ref<AliasBodyRule>))))> {};

  struct PredicateSignatureRule
      : rule<create<ast::PredicateSignature>(PredicateSignatureAnnotations +
                                             kw<"signature"> +
                                             PredicateSignatureContent)> {};

  struct DefaultPredicateRule
      : rule<create<ast::DefaultPredicate>(
            many(append<&ast::DefaultPredicate::annotations>(
                     ref<AnnotationRule>)) +
            kw<"default"> +
            option(assign<&ast::DefaultPredicate::resultType>(ref<TypeRule>) |
                   kw<"predicate">) +
            assign<&ast::DefaultPredicate::name>(ref<PredicateNameRule>) +
            lit<"("> +
            many(append<&ast::DefaultPredicate::params>(ref<ParamRule>),
                 lit<",">) +
            lit<")"> + lit<"{"> +
            assign<&ast::DefaultPredicate::body>(ref<FormulaRule>) + lit<"}">)> {
  };

  static constexpr auto TypeSignatureAnnotations =
      many(append<&ast::TypeSignature::annotations>(ref<AnnotationRule>));
  static constexpr auto TypeSignatureContent =
      kw<"class"> + assign<&ast::TypeSignature::name>(ref<ClassnameRule>) +
      option(kw<"extends"> +
             some(append<&ast::TypeSignature::extends_>(ref<TypeRule>),
                  lit<",">)) +
      (lit<";"> | (lit<"{"> +
                   many(append<&ast::TypeSignature::members>(
                       ref<PredicateSignatureBodyRule>)) +
                   lit<"}">));
  struct SignatureTypeRule
      : rule<create<ast::TypeSignature>(TypeSignatureAnnotations +
                                        TypeSignatureContent)> {};
  struct TypeSignatureRule : rule<create<ast::TypeSignature>(
                                 TypeSignatureAnnotations + kw<"signature"> +
                                 TypeSignatureContent)> {};

  struct ModuleSignatureRule
      : rule<create<ast::ModuleSignature>(
            many(append<&ast::ModuleSignature::annotations>(
                     ref<AnnotationRule>)) +
            kw<"signature"> + kw<"module"> +
            assign<&ast::ModuleSignature::name>(ref<UPPERID>) +
            option(lit<"<"> +
                   append<&ast::ModuleSignature::params>(ref<SignatureParamRule>) +
                   many(lit<","> + append<&ast::ModuleSignature::params>(
                                       ref<SignatureParamRule>)) +
                   lit<">">) +
            lit<"{"> +
            many(append<&ast::ModuleSignature::body>(
                     ref<PredicateSignatureBodyRule>) |
                 append<&ast::ModuleSignature::body>(ref<DefaultPredicateRule>) |
                 append<&ast::ModuleSignature::body>(ref<SignatureTypeRule>)) +
            lit<"}">)> {};

  struct SignatureRule
      : rule<ref<PredicateSignatureRule> | ref<TypeSignatureRule> |
             ref<ModuleSignatureRule>> {};

  struct DtBranchRule
      : rule<create<ast::DtBranch>(
            assign<&ast::DtBranch::name>(ref<UPPERID>) + lit<"("> +
            many(append<&ast::DtBranch::params>(ref<ParamRule>), lit<",">) +
            lit<")"> +
            option(lit<"{"> +
                   assign<&ast::DtBranch::restriction>(ref<FormulaRule>) +
                   lit<"}">))> {};

  struct DatatypeRule
      : rule<create<ast::Datatype>(
            many(append<&ast::Datatype::annotations>(ref<AnnotationRule>)) +
            kw<"newtype"> + assign<&ast::Datatype::name>(ref<UPPERID>) +
            lit<"="> +
            append<&ast::Datatype::branches>(ref<DtBranchRule>) +
            many(kw<"or"> + append<&ast::Datatype::branches>(ref<DtBranchRule>)))> {
  };

  struct TypeUnionRule
      : rule<create<ast::TypeUnion>(
            many(append<&ast::TypeUnion::annotations>(ref<AnnotationRule>)) +
            kw<"class"> + assign<&ast::TypeUnion::name>(ref<UPPERID>) +
            lit<"="> + append<&ast::TypeUnion::branches>(ref<TypeRule>) +
            many(kw<"or"> + append<&ast::TypeUnion::branches>(ref<TypeRule>)) +
            lit<";">)> {};

  struct FieldRule
      : rule<create<ast::Field>(
            many(append<&ast::Field::annotations>(ref<AnnotationRule>)) +
            assign<&ast::Field::type>(ref<TypeRule>) +
            assign<&ast::Field::name>(ref<Varname>) + lit<";">)> {};

  struct CharacteristicPredicateRule
      : rule<create<ast::CharacteristicPredicate>(
            many(append<&ast::CharacteristicPredicate::annotations>(
                     ref<AnnotationRule>)) +
            assign<&ast::CharacteristicPredicate::name>(ref<ClassnameRule>) +
            lit<"("> + lit<")"> + lit<"{"> +
            assign<&ast::CharacteristicPredicate::body>(ref<FormulaRule>) +
            lit<"}">)> {};

  // A named rule collapses a choice to the alternatives' common base before
  // append<> sees it, which loses which concrete class matched; a spliced
  // expression keeps each branch's own type so it lands in the right slot
  // of the heterogeneous `members` list.
  static constexpr auto Member = ref<CharacteristicPredicateRule> |
                                 ref<PredicateRule> | ref<FieldRule>;

  struct ClassRule
      : rule<create<ast::Class_>(
            many(append<&ast::Class_::annotations>(ref<AnnotationRule>)) +
            kw<"class"> + assign<&ast::Class_::name>(ref<ClassnameRule>) +
            option(kw<"extends"> +
                   append<&ast::Class_::extends_>(ref<TypeRule>) +
                   many(lit<","> +
                        append<&ast::Class_::extends_>(ref<TypeRule>))) +
            option(kw<"instanceof"> +
                   append<&ast::Class_::instanceof_>(ref<TypeRule>) +
                   many(lit<","> +
                        append<&ast::Class_::instanceof_>(ref<TypeRule>))) +
            lit<"{"> + many(append<&ast::Class_::members>(Member)) +
            lit<"}">)> {};

  struct TopLevelRule;

  struct ModuleSignatureExprRule
      : rule<option(ref<ModuleExprRule> + lit<"::">) + ref<UPPERID> +
             option(ref<ArgumentsRule>)> {};

  // Same reasoning as `Member` above: a helper that writes into the
  // enclosing node's member cannot be its own rule<> (that gives the node a
  // second head), so this is a spliced expression, not a ref<>'d rule.
  static constexpr auto ModuleImplements =
      kw<"implements"> +
      append<&ast::Module::implements_>(ref<ModuleSignatureExprRule>) +
      many(lit<","> +
           append<&ast::Module::implements_>(ref<ModuleSignatureExprRule>));

  struct ModuleRule
      : rule<create<ast::Module>(
            many(append<&ast::Module::annotations>(ref<AnnotationRule>)) +
            kw<"module"> + assign<&ast::Module::name>(ref<SimpleIdRule>) +
            option(lit<"<"> +
                   append<&ast::Module::params>(ref<SignatureParamRule>) +
                   many(lit<","> +
                        append<&ast::Module::params>(ref<SignatureParamRule>)) +
                   lit<">">) +
            option(ModuleImplements) + lit<"{"> +
            many(append<&ast::Module::body>(ref<TopLevelRule>)) + lit<"}">)> {
  };

  struct AliasPredicateRule
      : rule<create<ast::AliasPredicate>(
            many(append<&ast::AliasPredicate::annotations>(
                     ref<AnnotationRule>)) +
            kw<"predicate">
                + assign<&ast::AliasPredicate::name>(ref<PredicateNameRule>) +
            lit<"="> +
            assign<&ast::AliasPredicate::target>(ref<PredicateArityRule>) +
            lit<";">)> {};

  struct AliasClassRule
      : rule<create<ast::AliasClass>(
            many(append<&ast::AliasClass::annotations>(ref<AnnotationRule>)) +
            kw<"class"> + assign<&ast::AliasClass::name>(ref<ClassnameRule>) +
            lit<"="> + assign<&ast::AliasClass::type>(ref<TypeRule>) +
            lit<";">)> {};

  struct AliasModuleRule
      : rule<create<ast::AliasModule>(
            many(append<&ast::AliasModule::annotations>(ref<AnnotationRule>)) +
            kw<"module"> +
            assign<&ast::AliasModule::name>(ref<SimpleIdRule>) + lit<"="> +
            assign<&ast::AliasModule::moduleExpr>(ref<ModuleExprRule>) +
            lit<";">)> {};

  struct AliasRule
      : rule<ref<AliasPredicateRule> | ref<AliasClassRule> |
             ref<AliasModuleRule>> {};

  struct TopLevelRule
      : rule<ref<ImportRule> | ref<PredicateRule> | ref<SignatureRule> |
             ref<DatatypeRule> | ref<TypeUnionRule> | ref<ClassRule> |
             ref<ModuleRule> | ref<AliasRule> | ref<SelectRule>> {};

  struct QlRule
      : rule<create<ast::Document>(
            many(append<&ast::Document::body>(ref<TopLevelRule>)))> {};

  using entry = QlRule;
  using ignore = WS;
  using hidden = list<LINE_COMMENT, BLOCK_COMMENT>;
};

}  // namespace codeql
