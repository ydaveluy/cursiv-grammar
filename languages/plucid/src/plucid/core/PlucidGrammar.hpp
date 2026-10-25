// The plucid language: its grammar.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "plucid/core/ast.hpp"

namespace plucid {

inline constexpr std::string_view kNegMark = "˜";

inline constexpr auto parse_integer = [](std::string_view t) -> std::int64_t {
  bool neg = false;
  while (t.starts_with(kNegMark)) {
    neg = !neg;
    t.remove_prefix(kNegMark.size());
  }
  std::int64_t v = 0;
  for (const char c : t) v = v * 10 + (c - '0');
  return neg ? -v : v;
};
using IntConv = cursiv::text_conv<parse_integer>;

inline constexpr auto parse_real = [](std::string_view t) -> double {
  bool neg = false;
  while (t.starts_with(kNegMark)) {
    neg = !neg;
    t.remove_prefix(kNegMark.size());
  }
  double v = 0;
  std::size_t i = 0;
  for (; i < t.size() && t[i] != '.'; ++i) v = v * 10 + (t[i] - '0');
  if (i < t.size()) {
    double scale = 0.1;
    for (++i; i < t.size(); ++i) {
      v += (t[i] - '0') * scale;
      scale *= 0.1;
    }
  }
  return neg ? -v : v;
};
using RealConv = cursiv::text_conv<parse_real>;

inline constexpr auto unquote_string =
    [](std::string_view t) -> std::string_view {
  return t.substr(3, t.size() - 6);
};
using StringConv = cursiv::text_conv<unquote_string>;

struct Plucid : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \r\n\t)">)> {};

  static constexpr auto kLetter = cr<"a-zA-Z">;
  static constexpr auto kAlnum = cr<"a-zA-Z0-9">;
  static constexpr auto kSign = cr<"+*$&=<>:#ˆ-">;
  static constexpr auto kBracket =
      lit<"("> | lit<")"> | lit<"[%"> | lit<"%]"> | lit<"(%"> | lit<"%)">;

  struct ID : terminal<kLetter + many(kAlnum)> {};

  // The content of a quoted word, also reused bare as a list element: one
  // identifier-like run, a run of sign characters, one bracket spelling, or
  // one period, separator or quote character.
  struct WORD_ATOM
      : terminal<(kLetter + many(kAlnum)) | some(kSign) | kBracket |
                 lit<"."> | lit<","> | lit<";"> | lit<"\"">> {};

  struct STRING
      : terminal<through(lit<"‘">, lit<"’">), StringConv> {};

  // A leading run of `˜` negates once per occurrence; the converter
  // folds the parity into the value instead of building a chain of nodes.
  struct INTEGER : terminal<many(lit<"˜">) + some(d), IntConv> {};
  struct REAL
      : terminal<many(lit<"˜">) + some(d) + lit<"."> + many(d),
                 RealConv> {};

  struct SIN : terminal<kw<"sin">, value<ast::PrefixOp::sin>> {};
  struct COS : terminal<kw<"cos">, value<ast::PrefixOp::cos>> {};
  struct TAN : terminal<kw<"tan">, value<ast::PrefixOp::tan>> {};
  struct SQRT : terminal<kw<"sqrt">, value<ast::PrefixOp::sqrt>> {};
  struct ABS : terminal<kw<"abs">, value<ast::PrefixOp::abs>> {};
  struct LOG10 : terminal<kw<"log10">, value<ast::PrefixOp::log10>> {};
  struct LOG : terminal<kw<"log">, value<ast::PrefixOp::log>> {};
  struct ISNUMBER
      : terminal<kw<"isnumber">, value<ast::PrefixOp::isnumber>> {};
  struct ISWORD : terminal<kw<"isword">, value<ast::PrefixOp::isword>> {};
  struct NOT_OP : terminal<kw<"not">, value<ast::PrefixOp::lognot>> {};
  struct MKSTRING
      : terminal<kw<"mkstring">, value<ast::PrefixOp::mkstring>> {};
  struct ISSTRING
      : terminal<kw<"isstring">, value<ast::PrefixOp::isstring>> {};
  struct MKWORD : terminal<kw<"mkword">, value<ast::PrefixOp::mkword>> {};
  struct HD : terminal<kw<"hd">, value<ast::PrefixOp::hd>> {};
  struct TL : terminal<kw<"tl">, value<ast::PrefixOp::tl>> {};
  struct ISATOM : terminal<kw<"isatom">, value<ast::PrefixOp::isatom>> {};
  struct ISNULL : terminal<kw<"isnull">, value<ast::PrefixOp::isnull>> {};
  struct ISLIST : terminal<kw<"islist">, value<ast::PrefixOp::islist>> {};
  struct FIRST_OP : terminal<kw<"first">, value<ast::PrefixOp::first>> {};
  struct NEXT_OP : terminal<kw<"next">, value<ast::PrefixOp::next>> {};
  struct ISEOD : terminal<kw<"iseod">, value<ast::PrefixOp::iseod>> {};
  struct ISERROR : terminal<kw<"iserror">, value<ast::PrefixOp::iserror>> {};

  static constexpr auto PrefixOpAlt =
      ref<SIN> | ref<COS> | ref<TAN> | ref<SQRT> | ref<ABS> | ref<LOG10> |
      ref<LOG> | ref<ISNUMBER> | ref<ISWORD> | ref<MKSTRING> |
      ref<ISSTRING> | ref<MKWORD> | ref<HD> | ref<TL> | ref<ISATOM> |
      ref<ISNULL> | ref<ISLIST> | ref<FIRST_OP> | ref<NEXT_OP> | ref<ISEOD> |
      ref<ISERROR>;

  // One token per operation, each denoting its own constant: `eq`/`ne`
  // compare numbers, words, strings and lists alike, folded here to one
  // spelling since nothing in the grammar distinguishes their operand type.
  struct POWER : terminal<lit<"**">, value<ast::InfixOp::power>> {};
  struct MULTIPLY : terminal<lit<"*">, value<ast::InfixOp::multiply>> {};
  struct DIVIDE : terminal<lit<"/">, value<ast::InfixOp::divide>> {};
  struct INT_DIV : terminal<kw<"div">, value<ast::InfixOp::int_div>> {};
  struct MOD : terminal<kw<"mod">, value<ast::InfixOp::mod>> {};
  struct ADD : terminal<lit<"+">, value<ast::InfixOp::add>> {};
  struct SUBTRACT : terminal<lit<"-">, value<ast::InfixOp::subtract>> {};
  struct LE : terminal<lit<"<=">, value<ast::InfixOp::le>> {};
  struct GE : terminal<lit<">=">, value<ast::InfixOp::ge>> {};
  struct LT : terminal<lit<"<">, value<ast::InfixOp::lt>> {};
  struct GT : terminal<lit<">">, value<ast::InfixOp::gt>> {};
  struct EQ : terminal<kw<"eq">, value<ast::InfixOp::eq>> {};
  struct NE : terminal<kw<"ne">, value<ast::InfixOp::ne>> {};
  struct AND_OP : terminal<kw<"and">, value<ast::InfixOp::logical_and>> {};
  struct OR_OP : terminal<kw<"or">, value<ast::InfixOp::logical_or>> {};
  struct CONCAT : terminal<lit<"ˆ">, value<ast::InfixOp::concat>> {};
  struct LIST_APPEND
      : terminal<lit<"<>">, value<ast::InfixOp::list_append>> {};
  struct LIST_CONS : terminal<lit<"::">, value<ast::InfixOp::list_cons>> {};
  struct FBY : terminal<kw<"fby">, value<ast::InfixOp::fby>> {};
  struct WHENEVER
      : terminal<kw<"whenever">, value<ast::InfixOp::whenever>> {};
  struct WVR : terminal<kw<"wvr">, value<ast::InfixOp::wvr>> {};
  struct UPON : terminal<kw<"upon">, value<ast::InfixOp::upon>> {};
  struct ASA : terminal<kw<"asa">, value<ast::InfixOp::asa>> {};
  struct ATTIME : terminal<kw<"attime">, value<ast::InfixOp::attime>> {};

  struct Primary;
  struct Expression;
  struct ListConstantRule;
  struct AppliedExpr;

  struct RealLitRule
      : rule<create<ast::RealLit> +
             assign<&ast::RealLit::value>(ref<REAL>)> {};
  struct IntegerLitRule
      : rule<create<ast::IntegerLit> +
             assign<&ast::IntegerLit::value>(ref<INTEGER>)> {};
  struct WordLitRule
      : rule<create<ast::WordLit>(lit<"\""> +
                                  assign<&ast::WordLit::value>(
                                      ref<WORD_ATOM>) +
                                  lit<"\"">)> {};
  struct StringLitRule
      : rule<create<ast::StringLit> +
             assign<&ast::StringLit::value>(ref<STRING>)> {};

  // Reused bare (no quotes) as one shape of a list element.
  static constexpr auto ListElementAlt =
      ref<RealLitRule> | ref<IntegerLitRule> | ref<WORD_ATOM> |
      ref<StringLitRule> | ref<ListConstantRule>;

  struct ListConstantRule
      : rule<create<ast::NilLit> + kw<"nil">                        //
             | (create<ast::ListLit> + lit<"[]">)                   //
             | create<ast::ListLit>(
                   lit<"["> +
                   many(append<&ast::ListLit::elements>(ListElementAlt)) +
                   lit<"]">)> {};

  static constexpr auto Constant = ref<RealLitRule> | ref<IntegerLitRule> |
                                    ref<WordLitRule> | ref<StringLitRule> |
                                    ref<ListConstantRule>;

  struct IdentifierRule
      : rule<create<ast::Identifier> +
             assign<&ast::Identifier::name>(ref<ID>)> {};

  struct PrefixExprRule
      : rule<create<ast::PrefixExpr> +
             assign<&ast::PrefixExpr::op>(PrefixOpAlt) +
             assign<&ast::PrefixExpr::operand>(ref<AppliedExpr>)> {};

  struct FilterExprRule
      : rule<create<ast::FilterExpr> + kw<"filter"> +
             append<&ast::FilterExpr::args>(ref<Expression>) + lit<","> +
             append<&ast::FilterExpr::args>(ref<Expression>) + lit<","> +
             append<&ast::FilterExpr::args>(ref<Expression>)> {};

  struct SubstrExprRule
      : rule<create<ast::SubstrExpr> + kw<"substr"> +
             append<&ast::SubstrExpr::args>(ref<Expression>) + lit<","> +
             append<&ast::SubstrExpr::args>(ref<Expression>) + lit<","> +
             append<&ast::SubstrExpr::args>(ref<Expression>)> {};

  struct LengthExprRule
      : rule<create<ast::LengthExpr> + kw<"length"> +
             assign<&ast::LengthExpr::operand>(ref<AppliedExpr>)> {};

  struct ArgExprRule
      : rule<create<ast::ArgExpr> + kw<"arg"> +
             assign<&ast::ArgExpr::operand>(ref<AppliedExpr>)> {};

  // A list-expression's items are a plain comma-separated repetition: an
  // item is always an expression (a nested list included), so no separate
  // item rule is needed.
  struct ListExprRule
      : rule<(create<ast::ListExpr> + lit<"[%%]">)  //
             | create<ast::ListExpr>(
                   lit<"[%"> +
                   many(append<&ast::ListExpr::items>(ref<Expression>),
                        lit<",">) +
                   lit<"%]">)> {};

  struct IfExprRule
      : rule<create<ast::IfExpr> + kw<"if"> +
             assign<&ast::IfExpr::cond>(ref<Expression>) + kw<"then"> +
             assign<&ast::IfExpr::then_>(ref<Expression>) +
             assign<&ast::IfExpr::tail>(
                 create<ast::ElseTail>(
                     kw<"else"> +
                     assign<&ast::ElseTail::value>(ref<Expression>) +
                     kw<"fi">)  //
                 | create<ast::ElseifTail>(
                       kw<"elseif"> +
                       assign<&ast::ElseifTail::cond>(ref<Expression>) +
                       kw<"then"> +
                       assign<&ast::ElseifTail::then_>(ref<Expression>) +
                       kw<"endif">))> {};

  struct CondCaseRule
      : rule<create<ast::CondCase> +
             assign<&ast::CondCase::test>(ref<Expression>) + lit<":"> +
             assign<&ast::CondCase::result>(ref<Expression>) + lit<";">> {};

  struct CaseExprRule
      : rule<create<ast::CaseExpr> + kw<"case"> +
             assign<&ast::CaseExpr::subject>(ref<Expression>) + kw<"of"> +
             many(append<&ast::CaseExpr::cases>(ref<CondCaseRule>)) +
             kw<"default"> + lit<":"> +
             assign<&ast::CaseExpr::default_>(ref<Expression>) +
             kw<"end">> {};

  struct CondExprRule
      : rule<create<ast::CondExpr> + kw<"cond"> +
             many(append<&ast::CondExpr::cases>(ref<CondCaseRule>)) +
             kw<"default"> + lit<":"> +
             assign<&ast::CondExpr::default_>(ref<Expression>) +
             kw<"end">> {};

  struct FunctionCallRule
      : rule<create<ast::FunctionCall> +
             assign<&ast::FunctionCall::name>(ref<ID>) + lit<"("> +
             some(append<&ast::FunctionCall::args>(ref<Expression>),
                  lit<",">) +
             lit<")">> {};

  // Atoms: constants, names, the keyword-triggered constructs (each
  // self-delimiting on its own closing keyword or bracket) and a
  // parenthesized expression -- reachable from every precedence tier below.
  struct Primary
      : rule<Constant                                        //
             | ref<IdentifierRule>                            //
             | (create<ast::ErrorLit> + kw<"error">)           //
             | (create<ast::EodLit> + kw<"eod">)                //
             | ref<FilterExprRule>                             //
             | ref<SubstrExprRule>                             //
             | ref<ListExprRule>                               //
             | ref<IfExprRule>                                 //
             | ref<CaseExprRule>                                //
             | ref<CondExprRule>                                //
             | ref<FunctionCallRule>                             //
             | create<ast::GroupExpr>(
                   lit<"("> +
                   assign<&ast::GroupExpr::inner>(ref<Expression>) +
                   lit<")">)> {};

  // Tightest tier: the named unary operators plus length/arg, all mutually
  // stackable (`first arg lst`).
  struct AppliedExpr
      : rule<ref<PrefixExprRule> | ref<LengthExprRule> | ref<ArgExprRule> |
            ref<Primary>> {};

  struct ConcatExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<AppliedExpr>) +
                 assign<&ast::BinaryExpr::op>(ref<CONCAT>) +
                 assign<&ast::BinaryExpr::right>(ref<ConcatExpr>))  //
             | ref<AppliedExpr>> {};

  struct PowExpr
      : rule<ref<ConcatExpr> +
            many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                assign<&ast::BinaryExpr::op>(ref<POWER>) +
                assign<&ast::BinaryExpr::right>(ref<ConcatExpr>)))> {};

  struct MulExpr
      : rule<ref<PowExpr> +
            many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                assign<&ast::BinaryExpr::op>(ref<MULTIPLY> | ref<DIVIDE> |
                                             ref<INT_DIV> | ref<MOD>) +
                assign<&ast::BinaryExpr::right>(ref<PowExpr>)))> {};

  struct AddExpr
      : rule<ref<MulExpr> +
            many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                assign<&ast::BinaryExpr::op>(ref<ADD> | ref<SUBTRACT>) +
                assign<&ast::BinaryExpr::right>(ref<MulExpr>)))> {};

  // pLucid's associativity table leaves relation unlisted; chained here
  // (`a eq b ne c`) as a low-price over-acceptance, not a language rule.
  struct RelExpr
      : rule<ref<AddExpr> +
            many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                assign<&ast::BinaryExpr::op>(ref<LE> | ref<GE> | ref<LT> |
                                             ref<GT> | ref<EQ> | ref<NE>) +
                assign<&ast::BinaryExpr::right>(ref<AddExpr>)))> {};

  struct NotExpr
      : rule<create<ast::PrefixExpr>(
                 assign<&ast::PrefixExpr::op>(ref<NOT_OP>) +
                 assign<&ast::PrefixExpr::operand>(ref<RelExpr>))  //
             | ref<RelExpr>> {};

  struct AndExpr
      : rule<ref<NotExpr> +
            many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                assign<&ast::BinaryExpr::op>(ref<AND_OP>) +
                assign<&ast::BinaryExpr::right>(ref<NotExpr>)))> {};

  struct OrExpr
      : rule<ref<AndExpr> +
            many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                assign<&ast::BinaryExpr::op>(ref<OR_OP>) +
                assign<&ast::BinaryExpr::right>(ref<AndExpr>)))> {};

  struct ConsExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<OrExpr>) +
                 assign<&ast::BinaryExpr::op>(ref<LIST_APPEND> |
                                              ref<LIST_CONS>) +
                 assign<&ast::BinaryExpr::right>(ref<ConsExpr>))  //
             | ref<OrExpr>> {};

  struct FbyExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<ConsExpr>) +
                 assign<&ast::BinaryExpr::op>(ref<FBY>) +
                 assign<&ast::BinaryExpr::right>(ref<FbyExpr>))  //
             | ref<ConsExpr>> {};

  struct AsaExpr
      : rule<ref<FbyExpr> +
            many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                assign<&ast::BinaryExpr::op>(
                    ref<WHENEVER> | ref<WVR> | ref<UPON> | ref<ASA> |
                    ref<ATTIME>) +
                assign<&ast::BinaryExpr::right>(ref<FbyExpr>)))> {};

  struct CurrentDeclRule
      : rule<create<ast::CurrentDecl>(
            assign<&ast::CurrentDecl::name>(ref<ID>) + kw<"is"> +
            kw<"current"> +
            assign<&ast::CurrentDecl::value>(ref<Expression>))> {};

  struct DefinitionRule
      : rule<create<ast::Definition>(
            assign<&ast::Definition::name>(ref<ID>) +
            option(lit<"("> +
                   append<&ast::Definition::params>(ref<ID>) +
                   many(lit<","> +
                        append<&ast::Definition::params>(ref<ID>)) +
                   lit<")">) +
            lit<"="> + assign<&ast::Definition::value>(ref<Expression>))> {};

  // The loosest construct pLucid has: wraps whatever precedes it, and
  // stacks (`E where D1 end where D2 end`).
  struct Expression
      : rule<ref<AsaExpr> +
            many(nest<ast::WhereExpr, &ast::WhereExpr::subject>(
                kw<"where"> +
                many(append<&ast::WhereExpr::decls>(ref<CurrentDeclRule>) +
                     lit<";">) +
                many(append<&ast::WhereExpr::definitions>(ref<DefinitionRule>) +
                     lit<";">) +
                kw<"end">))> {};

  struct ProgramRule
      : rule<assign<&ast::Program::expression>(ref<Expression>)> {};

  using entry = ProgramRule;
  using ignore = WS;
};

}  // namespace plucid
