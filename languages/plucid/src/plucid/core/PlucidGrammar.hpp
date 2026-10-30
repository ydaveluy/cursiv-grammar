// The plucid language: its grammar.
#pragma once

#include <string>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "plucid/core/ast.hpp"

namespace plucid {

inline constexpr std::string_view kNegMark = "˜";

inline constexpr auto parse_integer = [](std::string_view t) {
  bool neg = false;
  while (t.starts_with(kNegMark) || t.starts_with("~")) {
    neg = !neg;
    t.remove_prefix(t.starts_with("~") ? 1 : kNegMark.size());
  }
  if (!neg) return cursiv::default_convert<std::int64_t>(t);
  return cursiv::default_convert<std::int64_t>("-" + std::string(t));
};

inline constexpr auto parse_real = [](std::string_view t) {
  bool neg = false;
  while (t.starts_with(kNegMark) || t.starts_with("~")) {
    neg = !neg;
    t.remove_prefix(t.starts_with("~") ? 1 : kNegMark.size());
  }
  auto result = cursiv::default_convert<double>(t);
  if (neg && result.value) *result.value = -*result.value;
  return result;
};

inline constexpr auto unquote_string =
    [](std::string_view t) -> std::string_view {
  return t.starts_with("'") ? t.substr(1, t.size() - 2) : t.substr(3, t.size() - 6);
};

struct Plucid : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \r\n\t)">)> {};
  struct LINE_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};

  static constexpr auto kLetter = cr<"a-zA-Z">;
  static constexpr auto kAlnum = cr<"a-zA-Z0-9">;
  static constexpr auto kSign = cr<"+*$&=<>:#^ˆ-">;
  static constexpr auto kBracket =
      lit<"("> | lit<")"> | lit<"[%"> | lit<"%]"> | lit<"(%"> | lit<"%)">;

  static constexpr auto reserved = kw<"if"> | kw<"then"> | kw<"else"> | kw<"elseif"> | kw<"fi"> |
      kw<"case"> | kw<"of"> | kw<"default"> | kw<"where"> | kw<"end"> | kw<"cond"> |
      kw<"is"> | kw<"current"> | kw<"include"> | kw<"true"> | kw<"false"> |
      kw<"error"> | kw<"eod"> | kw<"nil"> | kw<"sin"> | kw<"cos"> | kw<"tan"> | kw<"sqrt"> |
      kw<"abs"> | kw<"log10"> | kw<"log"> | kw<"isnumber"> | kw<"isword"> | kw<"not"> |
      kw<"mkstring"> | kw<"isstring"> | kw<"mkword"> | kw<"hd"> | kw<"tl"> | kw<"isatom"> |
      kw<"isnull"> | kw<"islist"> | kw<"first"> | kw<"next"> | kw<"iseod"> | kw<"iserror"> |
      kw<"filter"> | kw<"substr"> | kw<"length"> | kw<"arg"> | kw<"div"> | kw<"mod"> |
      kw<"and"> | kw<"or"> | kw<"eq"> | kw<"ne"> | kw<"fby"> | kw<"asa"> | kw<"attime"> |
      kw<"upon"> | kw<"whenever"> | kw<"wvr">;
  struct ID : terminal<!reserved + kLetter + many(kAlnum)> {};

  // The content of a quoted word, also reused bare as a list element: one
  // identifier-like run, a run of sign characters, one bracket spelling, or
  // one period, separator or quote character.
  struct WORD_ATOM
      : terminal<(kLetter + many(kAlnum)) | some(kSign) | kBracket |
                 lit<"."> | lit<","> | lit<";"> | lit<"\"">> {};

  struct STRING : terminal<as<unquote_string>(through(lit<"‘">, lit<"’">) | through(lit<"'">, lit<"'">))> {};

  // A leading run of `˜` negates once per occurrence; the converter
  // folds the parity into the value instead of building a chain of nodes.
  struct INTEGER : terminal<as<parse_integer>(many(lit<"~"> | lit<"˜">) + some(d))> {};
  struct REAL : terminal<as<parse_real>(many(lit<"~"> | lit<"˜">) + some(d) + lit<"."> +
                                        many(d))> {};

  static constexpr auto PrefixOpAlt =
      as<ast::PrefixOp::sin>(kw<"sin">) | as<ast::PrefixOp::cos>(kw<"cos">) | as<ast::PrefixOp::tan>(kw<"tan">) | as<ast::PrefixOp::sqrt>(kw<"sqrt">) | as<ast::PrefixOp::abs>(kw<"abs">) | as<ast::PrefixOp::log10>(kw<"log10">) |
      as<ast::PrefixOp::log>(kw<"log">) | as<ast::PrefixOp::isnumber>(kw<"isnumber">) | as<ast::PrefixOp::isword>(kw<"isword">) | as<ast::PrefixOp::mkstring>(kw<"mkstring">) |
      as<ast::PrefixOp::isstring>(kw<"isstring">) | as<ast::PrefixOp::mkword>(kw<"mkword">) | as<ast::PrefixOp::hd>(kw<"hd">) | as<ast::PrefixOp::tl>(kw<"tl">) | as<ast::PrefixOp::isatom>(kw<"isatom">) |
      as<ast::PrefixOp::isnull>(kw<"isnull">) | as<ast::PrefixOp::islist>(kw<"islist">) | as<ast::PrefixOp::first>(kw<"first">) | as<ast::PrefixOp::next>(kw<"next">) | as<ast::PrefixOp::iseod>(kw<"iseod">) |
      as<ast::PrefixOp::iserror>(kw<"iserror">);

  // One token per operation, each denoting its own constant: `eq`/`ne`
  // compare numbers, words, strings and lists alike, folded here to one
  // spelling since nothing in the grammar distinguishes their operand type.

  struct Primary;
  struct Expression;
  struct ListConstant;
  struct AppliedExpr;

  struct RealLit
      : rule<create<ast::RealLit> +
             assign<&ast::RealLit::value>(ref<REAL>)> {};
  struct IntegerLit
      : rule<create<ast::IntegerLit> +
             assign<&ast::IntegerLit::value>(ref<INTEGER>)> {};
  struct WordLit
      : rule<create<ast::WordLit>(lit<"\""> +
                                  assign<&ast::WordLit::value>(
                                      ref<WORD_ATOM>) +
                                  lit<"\"">)> {};
  struct StringLit
      : rule<create<ast::StringLit> +
             assign<&ast::StringLit::value>(ref<STRING>)> {};

  // Reused bare (no quotes) as one shape of a list element.
  static constexpr auto ListElementAlt =
      ref<RealLit> | ref<IntegerLit> | ref<WORD_ATOM> |
      ref<StringLit> | ref<ListConstant>;

  struct ListConstant
      : rule<(create<ast::NilLit> + kw<"nil">)                        //
             | (create<ast::ListLit> + lit<"[]">)                   //
             | create<ast::ListLit>(
                   lit<"["> +
                   many(append<&ast::ListLit::elements>(ListElementAlt)) +
                   lit<"]">)> {};

  static constexpr auto Constant = ref<RealLit> | ref<IntegerLit> |
                                    ref<WordLit> | ref<StringLit> |
                                    ref<ListConstant>;

  struct Identifier
      : rule<create<ast::Identifier> +
             assign<&ast::Identifier::name>(ref<ID> | kw<"true"> | kw<"false">)> {};

  struct PrefixExpr
      : rule<create<ast::PrefixExpr> +
             assign<&ast::PrefixExpr::op>(PrefixOpAlt) +
             assign<&ast::PrefixExpr::operand>(ref<AppliedExpr>)> {};

  struct FilterExpr
      : rule<create<ast::FilterExpr> + kw<"filter"> + lit<"("> +
             append<&ast::FilterExpr::args>(ref<Expression>) + lit<","> +
             append<&ast::FilterExpr::args>(ref<Expression>) + lit<","> +
             append<&ast::FilterExpr::args>(ref<Expression>) + lit<")">> {};

  struct SubstrExpr
      : rule<create<ast::SubstrExpr> + kw<"substr"> + lit<"("> +
             append<&ast::SubstrExpr::args>(ref<Expression>) + lit<","> +
             append<&ast::SubstrExpr::args>(ref<Expression>) + lit<","> +
             append<&ast::SubstrExpr::args>(ref<Expression>) + lit<")">> {};

  struct LengthExpr
      : rule<create<ast::LengthExpr> + kw<"length"> +
             assign<&ast::LengthExpr::operand>(ref<AppliedExpr>)> {};

  struct ArgExpr
      : rule<create<ast::ArgExpr> + kw<"arg"> +
             assign<&ast::ArgExpr::operand>(ref<AppliedExpr>)> {};

  // A list-expression's items are a plain comma-separated repetition: an
  // item is always an expression (a nested list included), so no separate
  // item rule is needed.
  struct ListExpr
      : rule<(create<ast::ListExpr> + lit<"[%%]">)  //
             | create<ast::ListExpr>(
                   lit<"[%"> +
                   many(append<&ast::ListExpr::items>(ref<Expression>),
                        lit<",">) +
                   lit<"%]">)> {};

  struct ConditionalTail : rule<
      create<ast::ElseTail>(kw<"else"> + assign<&ast::ElseTail::value>(ref<Expression>) + kw<"fi">) |
      create<ast::ElseifTail>(kw<"elseif"> + assign<&ast::ElseifTail::cond>(ref<Expression>) +
          kw<"then"> + assign<&ast::ElseifTail::then_>(ref<Expression>) +
          assign<&ast::ElseifTail::tail>(ref<ConditionalTail>))> {};
  struct IfExpr : rule<create<ast::IfExpr>(kw<"if"> +
      assign<&ast::IfExpr::cond>(ref<Expression>) + kw<"then"> +
      assign<&ast::IfExpr::then_>(ref<Expression>) +
      assign<&ast::IfExpr::tail>(ref<ConditionalTail>))> {};

  struct CondCase
      : rule<create<ast::CondCase> +
             assign<&ast::CondCase::test>(ref<Expression>) + lit<":"> +
             assign<&ast::CondCase::result>(ref<Expression>) + lit<";">> {};

  struct CaseExpr
      : rule<create<ast::CaseExpr> + kw<"case"> +
             assign<&ast::CaseExpr::subject>(ref<Expression>) + kw<"of"> +
             many(append<&ast::CaseExpr::cases>(ref<CondCase>)) +
             kw<"default"> + lit<":"> +
             assign<&ast::CaseExpr::default_>(ref<Expression>) + lit<";"> +
             kw<"end">> {};

  struct CondExpr
      : rule<create<ast::CondExpr> + kw<"cond"> +
             many(append<&ast::CondExpr::cases>(ref<CondCase>)) +
             kw<"default"> + lit<":"> +
             assign<&ast::CondExpr::default_>(ref<Expression>) + lit<";"> +
             kw<"end">> {};

  struct FunctionCall
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
             | ref<Identifier>                            //
             | (create<ast::ErrorLit> + kw<"error">)           //
             | (create<ast::EodLit> + kw<"eod">)                //
             | ref<FilterExpr>                             //
             | ref<SubstrExpr>                             //
             | ref<ListExpr>                               //
             | ref<IfExpr>                                 //
             | ref<CaseExpr>                                //
             | ref<CondExpr>                                //
             | ref<FunctionCall>                             //
             | create<ast::GroupExpr>(
                   lit<"("> +
                   assign<&ast::GroupExpr::inner>(ref<Expression>) +
                   lit<")">)> {};

  // Tightest tier: the named unary operators plus length/arg, all mutually
  // stackable (`first arg lst`).
  struct AppliedExpr
      : rule<ref<PrefixExpr> | ref<LengthExpr> | ref<ArgExpr> |
            ref<Primary>> {};

  struct ConcatExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<AppliedExpr>) +
                 assign<&ast::BinaryExpr::op>(as<ast::InfixOp::concat>(lit<"^"> | lit<"ˆ">)) +
                 assign<&ast::BinaryExpr::right>(ref<ConcatExpr>))  //
             | ref<AppliedExpr>> {};

  struct PowExpr
      : rule<ref<ConcatExpr> +
            many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                assign<&ast::BinaryExpr::op>(as<ast::InfixOp::power>(lit<"**">)) +
                assign<&ast::BinaryExpr::right>(ref<ConcatExpr>)))> {};

  struct MulExpr
      : rule<ref<PowExpr> +
            many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                assign<&ast::BinaryExpr::op>(as<ast::InfixOp::multiply>(lit<"*">) | as<ast::InfixOp::divide>(lit<"/">) |
                                             as<ast::InfixOp::int_div>(kw<"div">) | as<ast::InfixOp::mod>(kw<"mod">)) +
                assign<&ast::BinaryExpr::right>(ref<PowExpr>)))> {};

  struct AddExpr
      : rule<ref<MulExpr> +
            many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                assign<&ast::BinaryExpr::op>(as<ast::InfixOp::add>(lit<"+">) | as<ast::InfixOp::subtract>(lit<"-">)) +
                assign<&ast::BinaryExpr::right>(ref<MulExpr>)))> {};

  // pLucid's associativity table leaves relation unlisted; chained here
  // (`a eq b ne c`) as a low-price over-acceptance, not a language rule.
  struct RelExpr
      : rule<ref<AddExpr> +
            many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                assign<&ast::BinaryExpr::op>(as<ast::InfixOp::le>(lit<"<=">) | as<ast::InfixOp::ge>(lit<">=">) | as<ast::InfixOp::lt>(lit<"<">) |
                                             as<ast::InfixOp::gt>(lit<">">) | as<ast::InfixOp::eq>(kw<"eq">) | as<ast::InfixOp::ne>(kw<"ne">)) +
                assign<&ast::BinaryExpr::right>(ref<AddExpr>)))> {};

  struct NotExpr
      : rule<create<ast::PrefixExpr>(
                 assign<&ast::PrefixExpr::op>(as<ast::PrefixOp::lognot>(kw<"not">)) +
                 assign<&ast::PrefixExpr::operand>(ref<RelExpr>))  //
             | ref<RelExpr>> {};

  struct AndExpr
      : rule<ref<NotExpr> +
            many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                assign<&ast::BinaryExpr::op>(as<ast::InfixOp::logical_and>(kw<"and">)) +
                assign<&ast::BinaryExpr::right>(ref<NotExpr>)))> {};

  struct OrExpr
      : rule<ref<AndExpr> +
            many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                assign<&ast::BinaryExpr::op>(as<ast::InfixOp::logical_or>(kw<"or">)) +
                assign<&ast::BinaryExpr::right>(ref<AndExpr>)))> {};

  struct ConsExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<OrExpr>) +
                 assign<&ast::BinaryExpr::op>(as<ast::InfixOp::list_append>(lit<"<>">) |
                                              as<ast::InfixOp::list_cons>(lit<"::">)) +
                 assign<&ast::BinaryExpr::right>(ref<ConsExpr>))  //
             | ref<OrExpr>> {};

  struct FbyExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<ConsExpr>) +
                 assign<&ast::BinaryExpr::op>(as<ast::InfixOp::fby>(kw<"fby">)) +
                 assign<&ast::BinaryExpr::right>(ref<FbyExpr>))  //
             | ref<ConsExpr>> {};

  struct AsaExpr
      : rule<ref<FbyExpr> +
            many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                assign<&ast::BinaryExpr::op>(
                    as<ast::InfixOp::whenever>(kw<"whenever">) | as<ast::InfixOp::wvr>(kw<"wvr">) | as<ast::InfixOp::upon>(kw<"upon">) | as<ast::InfixOp::asa>(kw<"asa">) |
                    as<ast::InfixOp::attime>(kw<"attime">)) +
                assign<&ast::BinaryExpr::right>(ref<FbyExpr>)))> {};

  struct CurrentDecl
      : rule<create<ast::CurrentDecl>(
            assign<&ast::CurrentDecl::name>(ref<ID>) + kw<"is"> +
            kw<"current"> +
            assign<&ast::CurrentDecl::value>(ref<Expression>))> {};

  struct Definition
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
                many(append<&ast::WhereExpr::decls>(ref<CurrentDecl>) +
                     lit<";">) +
                many(append<&ast::WhereExpr::definitions>(ref<Definition>) +
                     lit<";">) +
                kw<"end">))> {};

  struct Program
      : rule<assign<&ast::Program::expression>(ref<Expression>)> {};

  using entry = Program;
  using ignored = list<WS, LINE_COMMENT>;
};

}  // namespace plucid
