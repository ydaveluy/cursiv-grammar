#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "erlang/core/ast.hpp"
#include <cstdint>
#include <memory_resource>

namespace erlang {

// Atom fields carry semantic names; literal/string/character fields elsewhere
// deliberately retain raw spelling. Match erl_scan's escape decoding here.
inline constexpr auto unquoteAtom = [](std::string_view t,
                                      std::pmr::memory_resource* mem) {
  using Result = cursiv::ConversionResult<std::string_view>;
  if (t.empty()) return Result::err("Incomplete atom.");
  if (t.front() != '\'') return Result::ok(t);
  if (t.size() < 2 || t.back() != '\'')
    return Result::err("Incomplete quoted atom.");
  t = t.substr(1, t.size() - 2);
  if (t.find('\\') == t.npos) {
    if (t.find('\'') != t.npos)
      return Result::err("An atom quote must be escaped.");
    return Result::ok(t);
  }
  const auto room = cursiv::text_room(mem, t.size());
  std::size_t n = 0;
  const auto digit = [](char c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
  };
  for (std::size_t i = 0; i < t.size(); ++i) {
    if (t[i] != '\\') {
      if (t[i] == '\'') return Result::err("An atom quote must be escaped.");
      room[n++] = t[i];
      continue;
    }
    // Recovery may supply a fragment that no longer satisfies Escape.
    if (i + 1 == t.size()) return Result::err("Incomplete atom escape.");
    const char c = t[++i];
    unsigned cp = static_cast<unsigned char>(c);
    if (c >= '0' && c <= '7') {
      cp = c - '0';
      for (unsigned k = 1; k < 3 && i + 1 < t.size() && t[i + 1] >= '0' && t[i + 1] <= '7'; ++k)
        cp = cp * 8 + t[++i] - '0';
    } else if (c == 'x') {
      cp = 0;
      if (i + 1 < t.size() && t[i + 1] == '{') {
        i += 2;
        const auto start = i;
        for (; i < t.size() && t[i] != '}'; ++i) {
          const auto value = digit(t[i]);
          if (value < 0) return Result::err("Invalid hexadecimal atom escape.");
          if (cp > (0x10ffffu - static_cast<unsigned>(value)) / 16)
            return Result::err("Atom escape is outside Unicode.");
          cp = cp * 16 + static_cast<unsigned>(value);
        }
        if (i == t.size() || i == start)
          return Result::err("Incomplete hexadecimal atom escape.");
      } else {
        if (i + 2 >= t.size() || digit(t[i + 1]) < 0 || digit(t[i + 2]) < 0)
          return Result::err("An atom character code needs two hexadecimal digits.");
        cp = static_cast<unsigned>(digit(t[i + 1]) * 16 + digit(t[i + 2]));
        i += 2;
      }
      if (cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff))
        return Result::err("Atom escape is not a Unicode scalar.");
    } else if (c == '^') {
      if (i + 1 == t.size()) return Result::err("Incomplete control atom escape.");
      const char control = t[++i];
      if (control != '?' && !(control >= '@' && control <= '_') &&
          !(control >= 'a' && control <= 'z'))
        return Result::err("Invalid control atom escape.");
      cp = control == '?' ? 127 : control & 31;
    } else {
      switch (c) {
        case 'n': cp = '\n'; break; case 'r': cp = '\r'; break;
        case 't': cp = '\t'; break; case 'v': cp = '\v'; break;
        case 'b': cp = '\b'; break; case 'f': cp = '\f'; break;
        case 'e': cp = 27; break; case 's': cp = ' '; break;
        case 'd': cp = 127; break;
        default: room[n++] = c; continue;
      }
    }
    if (cp < 0x80) room[n++] = static_cast<char>(cp);
    else {
      if (cp >= 0x10000) room[n++] = static_cast<char>(0xf0 | (cp >> 18));
      else if (cp >= 0x800) room[n++] = static_cast<char>(0xe0 | (cp >> 12));
      else room[n++] = static_cast<char>(0xc0 | (cp >> 6));
      if (cp >= 0x10000) room[n++] = static_cast<char>(0x80 | ((cp >> 12) & 63));
      if (cp >= 0x800) room[n++] = static_cast<char>(0x80 | ((cp >> 6) & 63));
      room[n++] = static_cast<char>(0x80 | (cp & 63));
    }
  }
  return Result::ok(std::string_view(room.data(), n));
};

inline constexpr auto checkedNumber = [](std::string_view t) {
  using Result = cursiv::ConversionResult<std::string_view>;
  const auto hash = t.find('#');
  if (hash == t.npos) return Result::ok(t);
  unsigned base = 0;
  for (const char c : t.substr(0, hash)) if (c != '_') {
    if (base > 36) return Result::err("Integer base must be from 2 to 36.");
    base = base * 10 + c - '0';
  }
  if (base < 2 || base > 36) return Result::err("Integer base must be from 2 to 36.");
  const auto exponent = t.find('#', hash + 1);
  for (const char c : t.substr(hash + 1, exponent == t.npos ? t.npos : exponent - hash - 1)) if (c != '_' && c != '.') {
    const unsigned value = c >= '0' && c <= '9' ? c - '0' :
                          c >= 'a' && c <= 'z' ? c - 'a' + 10 : c - 'A' + 10;
    if (value >= base) return Result::err("Digit is outside the integer base.");
  }
  return Result::ok(t);
};

struct Erlang : cursiv::GrammarSpec {
  static constexpr auto kLower = cr<"a-z"> | cr<"ß-öø-ÿ">;
  static constexpr auto kUpper = cr<"A-Z"> | cr<"À-ÖØ-Þ">;
  static constexpr auto kIdCont = cr<"0-9a-zA-Z_@"> | cr<"À-ÖØ-Þß-öø-ÿ">;

  static constexpr auto word_boundary = kIdCont;

  struct WS : terminal<some(cr<R"(\x00- )"> | cr<"\u0080-\u00a0">)> {};
  struct COMMENT : terminal<lit<"%"> + many(cr<R"(^\n)">)> {};

  struct VAR : terminal<(kUpper | lit<"_">) + many(kIdCont)> {};

  static constexpr auto Escape = lit<"\\"> +
      ((lit<"x{"> + some(cr<"0-9a-fA-F">) + lit<"}">) |
       (lit<"x"> + repeat<2>(cr<"0-9a-fA-F">)) |
       (lit<"^"> + cr<"?@-_a-z">) | repeat<1, 3>(cr<"0-7">) |
       (!cr<"x^"> + any));

  static constexpr auto Reserved =
      kw<"after"> | kw<"begin"> | kw<"case"> | kw<"try"> | kw<"cond"> | kw<"catch"> | kw<"andalso"> | kw<"orelse"> | kw<"end"> | kw<"fun"> | kw<"if"> | kw<"let"> | kw<"of"> | kw<"receive"> | kw<"when"> | kw<"bnot"> | kw<"not"> | kw<"div"> | kw<"rem"> | kw<"band"> | kw<"and"> | kw<"bor"> | kw<"bxor"> | kw<"bsl"> | kw<"bsr"> | kw<"or"> | kw<"xor"> | kw<"maybe"> | kw<"else">;

  struct ATOM
      : terminal<as<unquoteAtom>(
            !Reserved + ((kLower + many(kIdCont)) |
                         (lit<"'"> + many(Escape | cr<R"(^\\')">) + lit<"'">)))> {};

  static constexpr auto Digits = cr<"0-9"> + many(option(lit<"_">) + cr<"0-9">);
  static constexpr auto BasedDigits = cr<"0-9a-zA-Z"> + many(option(lit<"_">) + cr<"0-9a-zA-Z">);
  static constexpr auto Exponent = cr<"eE"> + option(cr<"+-">) + Digits;

  struct INTEGER : terminal<as<checkedNumber>(Digits + option(lit<"#"> + BasedDigits))> {};
  struct FLOATT : terminal<as<checkedNumber>(
      (Digits + lit<"."> + Digits + option(Exponent)) |
      (Digits + lit<"#"> + BasedDigits + lit<"."> + BasedDigits +
       option(lit<"#"> + Exponent)))> {};

  struct CHAR : terminal<lit<"$"> + (Escape | (!lit<"\\"> + any))> {};

  struct QuoteCount {};
  static constexpr auto TripleString =
      &lit<"\"\"\""> + mark<QuoteCount>(some(cr<"\"">)) +
      many(cr<" \t\r"> | cr<"\u0080-\u00a0">) +
      through(&lit<"\n">, lit<"\n"> +
          many(cr<" \t\r"> | cr<"\u0080-\u00a0">) + same<QuoteCount>);
  static constexpr auto NormalString =
      lit<"\""> + many(Escape | cr<R"(^\\")">) + lit<"\"">;
  struct STRING : terminal<TripleString | NormalString> {};

  template<auto Open, auto Close, auto Body>
  static constexpr auto SigilBody = Open + many(Escape | Body) + Close;
  static constexpr auto EscapedSigil = TripleString |
      SigilBody<lit<"\"">, lit<"\"">, cr<R"(^\\")">> |
      SigilBody<lit<"'">, lit<"'">, cr<R"(^\\')">> |
      SigilBody<lit<"/">, lit<"/">, cr<R"(^\\/)">> |
      SigilBody<lit<"|">, lit<"|">, cr<R"(^\\|)">> |
      SigilBody<lit<"#">, lit<"#">, cr<R"(^\\#)">> |
      SigilBody<lit<"`">, lit<"`">, cr<R"(^\\`)">> |
      SigilBody<lit<"(">, lit<")">, cr<R"(^\\))">> |
      SigilBody<lit<"[">, lit<"]">, cr<R"(^\\])">> |
      SigilBody<lit<"{">, lit<"}">, cr<R"(^\\})">> |
      SigilBody<lit<"<">, lit<">">, cr<R"(^\\>)">>;
  static constexpr auto VerbatimSigil = TripleString |
      through(lit<"\"">, lit<"\"">) | through(lit<"'">, lit<"'">) |
      through(lit<"/">, lit<"/">) | through(lit<"|">, lit<"|">) |
      through(lit<"#">, lit<"#">) | through(lit<"`">, lit<"`">) |
      through(lit<"(">, lit<")">) | through(lit<"[">, lit<"]">) |
      through(lit<"{">, lit<"}">) | through(lit<"<">, lit<">">);
  struct SIGIL : terminal<lit<"~"> +
      ((option(cr<"bs">) + EscapedSigil) |
       (some(kIdCont) + VerbatimSigil)) + many(kIdCont)> {};

  struct ATTR_NAME : terminal<as<[](std::string_view t) {
    using Result = cursiv::ConversionResult<std::string_view>;
    if (t.ends_with("callback")) return Result::ok(t.substr(t.size() - 8));
    if (t.ends_with("spec")) return Result::ok(t.substr(t.size() - 4));
    return Result::err("Incomplete spec or callback attribute.");
  }>(lit<"-"> + many(ref<WS> | ref<COMMENT>) +
                     (lit<"spec", !kIdCont> | lit<"callback", !kIdCont>))> {};

  static constexpr auto OpMult = as<ast::Operator::divide>(lit<"/">) | as<ast::Operator::multiply>(lit<"*">) |
                                 as<ast::Operator::int_div>(kw<"div">) | as<ast::Operator::rem_>(kw<"rem">) | as<ast::Operator::band_>(kw<"band">) |
                                 as<ast::Operator::and_>(kw<"and">);
  static constexpr auto OpAddLevel = as<ast::Operator::add>(lit<"+">) | as<ast::Operator::sub>(lit<"-">) | as<ast::Operator::bor_>(kw<"bor">) |
                                     as<ast::Operator::bxor_>(kw<"bxor">) | as<ast::Operator::bsl_>(kw<"bsl">) | as<ast::Operator::bsr_>(kw<"bsr">) |
                                     as<ast::Operator::or_>(kw<"or">) | as<ast::Operator::xor_>(kw<"xor">);
  static constexpr auto OpListLevel = as<ast::Operator::list_append>(lit<"++">) | as<ast::Operator::list_subtract>(lit<"--">);
  static constexpr auto OpCompareLevel = as<ast::Operator::exact_eq>(lit<"=:=">) | as<ast::Operator::exact_neq>(lit<"=/=">) |
                                         as<ast::Operator::le>(lit<"=<">) | as<ast::Operator::ge>(lit<">=">) | as<ast::Operator::eq>(lit<"==">) |
                                         as<ast::Operator::neq>(lit<"/=">) | as<ast::Operator::lt>(lit<"<">) | as<ast::Operator::gt>(lit<">">);
  static constexpr auto PrefixOpText =
      lit<"+"> | lit<"-"> | kw<"bnot"> | kw<"not">;

  struct Expr;
  struct Unary;
  struct PrimaryChain;
  struct ExprMax;

  struct ExprInfix
      : infix<Unary, ast::BinaryExpr,
             levels(left(OpMult),           //
                    left(OpAddLevel),       //
                    right(OpListLevel),     //
                    nonassoc(OpCompareLevel),   //
                    right(as<ast::Operator::andalso_>(kw<"andalso">)),  //
                    right(as<ast::Operator::orelse_>(kw<"orelse">)),   //
                    right(as<ast::Operator::match>(lit<"=">) | as<ast::Operator::send>(lit<"!">)))> {};

  struct Expr : rule<ref<ExprInfix>> {};

  // Yecc's lowest-priority catch prefix may start any operator operand.
  // Calling Expr here lets it absorb the full following expression; a
  // separate root-only alternative would reject `A = catch B`.
  struct Unary
      : rule<create<ast::CatchExpr>(
                 kw<"catch"> + assign<&ast::CatchExpr::operand>(ref<Expr>)) |
             create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(PrefixOpText) +
                 assign<&ast::UnaryExpr::operand>(ref<Unary>)) |
             ref<PrimaryChain>> {};

  static constexpr auto MapFieldBody =
      create<ast::MapField>(
          assign<&ast::MapField::key>(ref<Expr>) +
          ((lit<"=>"> + assign<&ast::MapField::value>(ref<Expr>))
           | (lit<":="> +
              flag<&ast::MapField::exact>(
                  assign<&ast::MapField::value>(ref<Expr>)))));

  struct MapField : rule<MapFieldBody> {};

  static constexpr auto MapTupleBody =
      lit<"{"> +
      many(append<&ast::MapExpr::fields>(ref<MapField>), lit<",">) +
      lit<"}">;

  struct RecordField
      : rule<create<ast::RecordField>(
                 assign<&ast::RecordField::name>(ref<VAR> | ref<ATOM>) +
                 lit<"="> +
                 assign<&ast::RecordField::value>(ref<Expr>))> {};

  static constexpr auto RecordBody =
      assign<&ast::RecordExpr::name>(ref<ATOM>) +
      ((lit<"."> + assign<&ast::RecordExpr::accessField>(ref<ATOM>)) |
       (lit<"{"> +
        many(append<&ast::RecordExpr::fields>(ref<RecordField>),
             lit<",">) +
        lit<"}">));

  struct PrimaryChain
      : rule<(ref<ExprMax> +
              option(nest<ast::ModuleRef, &ast::ModuleRef::module>(
                  lit<":"> +
                  assign<&ast::ModuleRef::member>(ref<ExprMax>))) +
              many(nest<ast::Call, &ast::Call::target>(
                  lit<"("> +
                  many(append<&ast::Call::args>(ref<Expr>), lit<",">) +
                  lit<")">)) +
              many(nest<ast::MapExpr, &ast::MapExpr::base>(lit<"#"> +
                                                            MapTupleBody) |
                   nest<ast::RecordExpr, &ast::RecordExpr::base>(
                       lit<"#"> + RecordBody)))
             | ((create<ast::MapExpr>(lit<"#"> + MapTupleBody) |
                 create<ast::RecordExpr>(lit<"#"> + RecordBody)) +
                many(nest<ast::MapExpr, &ast::MapExpr::base>(lit<"#"> +
                                                              MapTupleBody) |
                     nest<ast::RecordExpr, &ast::RecordExpr::base>(
                         lit<"#"> + RecordBody)))> {};

  struct ListLit
      : rule<create<ast::ListLit>(
                 lit<"["> + lit<"]">) |
             create<ast::ListLit>(
                 lit<"["> +
                 some(append<&ast::ListLit::items>(ref<Expr>), lit<",">) +
                 option(lit<"|"> +
                        assign<&ast::ListLit::tail>(ref<Expr>)) +
                 lit<"]">)> {};

  struct TupleLit
      : rule<create<ast::TupleLit>(
                 lit<"{"> +
                 many(append<&ast::TupleLit::items>(ref<Expr>), lit<",">) +
                 lit<"}">)> {};

  static constexpr auto BitTypeOne =
      ref<ATOM> + option(lit<":"> + ref<INTEGER>);
  struct BitTypeList : rule<as<std::string_view>(some(BitTypeOne, lit<"-">))> {
  };

  static constexpr auto BitExprValue =
      (create<ast::UnaryExpr>(
           assign<&ast::UnaryExpr::op>(PrefixOpText) +
           assign<&ast::UnaryExpr::operand>(ref<ExprMax>))
       | ref<ExprMax>);

  struct BinElement
      : rule<create<ast::BinElement>(
                 assign<&ast::BinElement::value>(BitExprValue) +
                 option(lit<":"> +
                        assign<&ast::BinElement::size>(ref<ExprMax>)) +
                 option(lit<"/"> + assign<&ast::BinElement::bitTypes>(ref<BitTypeList>)))> {};

  struct BinaryLit
      : rule<create<ast::BinaryLit>(lit<"<<"> + lit<">>">) |
             create<ast::BinaryLit>(
                 lit<"<<"> +
                 some(append<&ast::BinaryLit::elements>(ref<BinElement>),
                      lit<",">) +
                 lit<">>">)> {};

  struct LcClause
      : rule<create<ast::LcClause>(
          assign<&ast::LcClause::left>(ref<Expr>) +
          option(lit<":="> + assign<&ast::LcClause::mapValue>(ref<Expr>)) +
          option(assign<&ast::LcClause::op>(
                     lit<"<:-"> | lit<"<:="> | lit<"<-"> | lit<"<=">) +
                 assign<&ast::LcClause::right>(ref<Expr>)))> {};

  struct Qualifier
      : rule<create<ast::ZipQualifier>(
          append<&ast::ZipQualifier::generators>(ref<LcClause>) +
          lit<"&&"> + some(append<&ast::ZipQualifier::generators>(ref<LcClause>), lit<"&&">)) |
          ref<LcClause>> {};

  struct ListCompr
      : rule<create<ast::ListCompr>(
          lit<"["> + assign<&ast::ListCompr::head>(ref<Expr>) + lit<"||"> +
          some(append<&ast::ListCompr::quals>(ref<Qualifier>), lit<",">) + lit<"]">)> {};

  struct BinCompr
      : rule<create<ast::BinCompr>(
          lit<"<<"> + assign<&ast::BinCompr::head>(ref<ExprMax>) + lit<"||"> +
          some(append<&ast::BinCompr::quals>(ref<Qualifier>), lit<",">) + lit<">>">)> {};

  struct MapCompr
      : rule<create<ast::MapCompr>(
          lit<"#"> + lit<"{"> + assign<&ast::MapCompr::head>(ref<MapField>) + lit<"||"> +
          some(append<&ast::MapCompr::quals>(ref<Qualifier>), lit<",">) + lit<"}">)> {};

  struct GuardGroup
      : rule<create<ast::GuardGroup>(
                 some(append<&ast::GuardGroup::exprs>(ref<Expr>),
                      lit<",">))> {};

  struct Guard
      : rule<create<ast::Guard>(
                 some(append<&ast::Guard::groups>(ref<GuardGroup>),
                      lit<";">))> {};

  struct IfClause
      : rule<create<ast::IfClause>(
                 assign<&ast::IfClause::guard>(ref<Guard>) + lit<"->"> +
                 some(append<&ast::IfClause::body>(ref<Expr>),
                      lit<",">))> {};

  struct IfExpr
      : rule<create<ast::IfExpr>(
                 kw<"if"> +
                 some(append<&ast::IfExpr::clauses>(ref<IfClause>),
                      lit<";">) +
                 kw<"end">)> {};

  struct CaseClause
      : rule<create<ast::CaseClause>(
                 assign<&ast::CaseClause::pattern>(ref<Expr>) +
                 option(kw<"when"> +
                        assign<&ast::CaseClause::guard>(ref<Guard>)) +
                 lit<"->"> +
                 some(append<&ast::CaseClause::body>(ref<Expr>),
                      lit<",">))> {};

  struct MaybeItem
      : rule<create<ast::MaybeMatch>(
          assign<&ast::MaybeMatch::left>(ref<Expr>) + lit<"?="> +
          assign<&ast::MaybeMatch::right>(ref<Expr>)) | ref<Expr>> {};

  struct MaybeExpr
      : rule<create<ast::MaybeExpr>(
          kw<"maybe"> + some(append<&ast::MaybeExpr::body>(ref<MaybeItem>), lit<",">) +
          option(flag<&ast::MaybeExpr::hasElse>(kw<"else">) +
              some(append<&ast::MaybeExpr::elseClauses>(ref<CaseClause>), lit<";">)) + kw<"end">)> {};

  struct CaseExpr
      : rule<create<ast::CaseExpr>(
                 kw<"case"> + assign<&ast::CaseExpr::subject>(ref<Expr>) +
                 kw<"of"> +
                 some(append<&ast::CaseExpr::clauses>(ref<CaseClause>),
                      lit<";">) +
                 kw<"end">)> {};

  struct ReceiveExpr
      : rule<create<ast::ReceiveExpr>(
                 kw<"receive"> +
                 option(some(
                     append<&ast::ReceiveExpr::clauses>(ref<CaseClause>),
                     lit<";">)) +
                 option(flag<&ast::ReceiveExpr::hasAfter>(kw<"after">) +
                        assign<&ast::ReceiveExpr::afterTimeout>(
                            ref<Expr>) +
                        lit<"->"> +
                        some(append<&ast::ReceiveExpr::afterBody>(
                                 ref<Expr>),
                             lit<",">)) +
                 kw<"end">)> {};

  struct FunClause
      : rule<create<ast::FunClause>(
                 option(assign<&ast::FunClause::name>(ref<VAR>)) + lit<"("> +
                 many(append<&ast::FunClause::params>(ref<Expr>),
                      lit<",">) +
                 lit<")"> +
                 option(kw<"when"> +
                        assign<&ast::FunClause::guard>(ref<Guard>)) +
                 lit<"->"> +
                 some(append<&ast::FunClause::body>(ref<Expr>),
                      lit<",">))> {};

  struct FunExpr
      : rule<create<ast::FunNameRef>(
                 kw<"fun"> + assign<&ast::FunNameRef::name>(ref<ATOM>) +
                 lit<"/"> +
                 assign<&ast::FunNameRef::arity>(ref<INTEGER>)) |
             create<ast::FunRemoteRef>(
                 kw<"fun"> +
                 assign<&ast::FunRemoteRef::module>(ref<ATOM> | ref<VAR>) +
                 lit<":"> +
                 assign<&ast::FunRemoteRef::function>(ref<ATOM> | ref<VAR>) +
                 lit<"/"> +
                 assign<&ast::FunRemoteRef::arity>(ref<INTEGER> |
                                                    ref<VAR>)) |
             create<ast::FunExpr>(
                 kw<"fun"> +
                 some(append<&ast::FunExpr::clauses>(ref<FunClause>),
                      lit<";">) +
                 kw<"end">)> {};

  struct CatchClause
      : rule<create<ast::CatchClause>(
                 option(assign<&ast::CatchClause::exClass>(ref<ATOM> |
                                                            ref<VAR>) +
                        lit<":">) +
                 assign<&ast::CatchClause::pattern>(ref<Expr>) +
                 option(lit<":"> +
                        assign<&ast::CatchClause::stackVar>(ref<VAR>)) +
                 option(kw<"when"> +
                        assign<&ast::CatchClause::guard>(ref<Guard>)) +
                 lit<"->"> +
                 some(append<&ast::CatchClause::body>(ref<Expr>),
                      lit<",">))> {};

  struct TryExpr
      : rule<create<ast::TryExpr>(
                 kw<"try"> +
                 some(append<&ast::TryExpr::body>(ref<Expr>), lit<",">) +
                 option(flag<&ast::TryExpr::hasOf>(kw<"of">) +
                        some(append<&ast::TryExpr::ofClauses>(
                                 ref<CaseClause>),
                             lit<";">)) +
                 ((kw<"catch"> +
                   some(append<&ast::TryExpr::catchClauses>(
                            ref<CatchClause>),
                        lit<";">) +
                   option(flag<&ast::TryExpr::hasAfter>(kw<"after">) +
                          some(append<&ast::TryExpr::afterBody>(
                                   ref<Expr>),
                               lit<",">))) |
                  (flag<&ast::TryExpr::hasAfter>(kw<"after">) +
                   some(append<&ast::TryExpr::afterBody>(ref<Expr>),
                        lit<",">))) +
                 kw<"end">)> {};

  struct ExprMax
      : rule<assign<&ast::VarRef::name>(ref<VAR>) |
             assign<&ast::CharLit::value>(ref<CHAR>) |
             assign<&ast::IntegerLit::value>(ref<INTEGER>) |
             assign<&ast::FloatLit::value>(ref<FLOATT>) |
             assign<&ast::AtomLit::name>(ref<ATOM>) |
             some(append<&ast::StringLit::parts>(ref<STRING>)) |
             ref<ListLit> | ref<BinaryLit> | ref<ListCompr> |
             ref<BinCompr> | ref<MapCompr> | ref<TupleLit> |
             (lit<"("> + ref<Expr> + lit<")">) |
             create<ast::BeginEnd>(
                 kw<"begin"> +
                 some(append<&ast::BeginEnd::body>(ref<Expr>), lit<",">) +
                 kw<"end">) |
             ref<IfExpr> | ref<CaseExpr> | ref<ReceiveExpr> |
             ref<FunExpr> | ref<TryExpr> | ref<MaybeExpr> |
             assign<&ast::SigilLit::spelling>(ref<SIGIL>) |
             create<ast::MacroExpr>(
                 (flag<&ast::MacroExpr::stringify>(lit<"??">) | lit<"?">) +
                 assign<&ast::MacroExpr::name>(ref<VAR> | ref<ATOM>))> {};

  struct TypeExpr;

  template <auto Part>
  static constexpr auto TypeLeaf = append<&ast::TypeExpr::parts>(Part);

  static constexpr auto NestedType =
      append<&ast::TypeExpr::parts>(ref<TypeExpr>);

  static constexpr auto TypeAtom =
      (TypeLeaf<lit<"(">> + NestedType + TypeLeaf<lit<")">>) | TypeLeaf<ref<VAR>> |
      (TypeLeaf<ref<ATOM>> + TypeLeaf<lit<"(">> + TypeLeaf<lit<")">>) |
      (TypeLeaf<ref<ATOM>> + TypeLeaf<lit<"(">> + some(NestedType, TypeLeaf<lit<",">>) + TypeLeaf<lit<")">>) |
      (TypeLeaf<ref<ATOM>> + TypeLeaf<lit<":">> + TypeLeaf<ref<ATOM>> + TypeLeaf<lit<"(">> + TypeLeaf<lit<")">>) |
      (TypeLeaf<ref<ATOM>> + TypeLeaf<lit<":">> + TypeLeaf<ref<ATOM>> + TypeLeaf<lit<"(">> +
       some(NestedType, TypeLeaf<lit<",">>) + TypeLeaf<lit<")">>) |
      (TypeLeaf<lit<"[">> + TypeLeaf<lit<"]">>) | (TypeLeaf<lit<"[">> + NestedType + TypeLeaf<lit<"]">>) |
      (TypeLeaf<lit<"[">> + NestedType + TypeLeaf<lit<",">> + TypeLeaf<lit<"...">> + TypeLeaf<lit<"]">>) |
      (TypeLeaf<lit<"#">> + TypeLeaf<lit<"{">> + TypeLeaf<lit<"}">>) |
      (TypeLeaf<lit<"#">> + TypeLeaf<lit<"{">> +
       some(NestedType + (TypeLeaf<lit<"=>">> | TypeLeaf<lit<":=">>) + NestedType, TypeLeaf<lit<",">>) +
       TypeLeaf<lit<"}">>) |
      (TypeLeaf<lit<"{">> + TypeLeaf<lit<"}">>) |
      (TypeLeaf<lit<"{">> + some(NestedType, TypeLeaf<lit<",">>) + TypeLeaf<lit<"}">>) |
      (TypeLeaf<lit<"#">> + TypeLeaf<ref<ATOM>> + TypeLeaf<lit<"{">> + TypeLeaf<lit<"}">>) |
      (TypeLeaf<lit<"#">> + TypeLeaf<ref<ATOM>> + TypeLeaf<lit<"{">> +
       some(TypeLeaf<ref<ATOM>> + TypeLeaf<lit<"::">> + NestedType, TypeLeaf<lit<",">>) + TypeLeaf<lit<"}">>) |
      (TypeLeaf<lit<"<<">> + TypeLeaf<lit<">>">>) |
      (TypeLeaf<lit<"<<">> + TypeLeaf<ref<VAR>> + TypeLeaf<lit<":">> + NestedType + TypeLeaf<lit<">>">>) |
      (TypeLeaf<lit<"<<">> + TypeLeaf<ref<VAR>> + TypeLeaf<lit<":">> + TypeLeaf<ref<VAR>> + TypeLeaf<lit<"*">> + NestedType +
       TypeLeaf<lit<">>">>) |
      (TypeLeaf<lit<"<<">> + TypeLeaf<ref<VAR>> + TypeLeaf<lit<":">> + NestedType + TypeLeaf<lit<",">> + TypeLeaf<ref<VAR>> +
       TypeLeaf<lit<":">> + TypeLeaf<ref<VAR>> + TypeLeaf<lit<"*">> + NestedType + TypeLeaf<lit<">>">>) |
      TypeLeaf<ref<INTEGER>> | TypeLeaf<ref<CHAR>> | (TypeLeaf<kw<"fun">> + TypeLeaf<lit<"(">> + TypeLeaf<lit<")">>) |
      (TypeLeaf<kw<"fun">> + TypeLeaf<lit<"(">> + TypeLeaf<lit<"(">> + TypeLeaf<lit<"...">> + TypeLeaf<lit<")">> + TypeLeaf<lit<"->">> +
       NestedType + TypeLeaf<lit<")">>) |
      (TypeLeaf<kw<"fun">> + TypeLeaf<lit<"(">> +
       ((TypeLeaf<lit<"(">> + option(some(NestedType, TypeLeaf<lit<",">>)) + TypeLeaf<lit<")">> +
         TypeLeaf<lit<"->">> + NestedType)) +
       TypeLeaf<lit<")">>) |
      TypeLeaf<ref<ATOM>>;

  static constexpr auto TypeTerm = option(TypeLeaf<PrefixOpText>) + TypeAtom;
  static constexpr auto TypeArith =
      TypeTerm + many((TypeLeaf<lit<"*">> | TypeLeaf<lit<"/">> | TypeLeaf<kw<"div">> |
                       TypeLeaf<kw<"rem">> | TypeLeaf<kw<"band">> | TypeLeaf<kw<"and">> | TypeLeaf<lit<"+">> |
                       TypeLeaf<lit<"-">> | TypeLeaf<kw<"bor">> | TypeLeaf<kw<"bxor">> | TypeLeaf<kw<"bsl">> |
                       TypeLeaf<kw<"bsr">> | TypeLeaf<kw<"or">> | TypeLeaf<kw<"xor">>) +
                          TypeTerm);
  static constexpr auto TypeRange = TypeArith + option(TypeLeaf<lit<"..">> + TypeArith);
  static constexpr auto TypeUnion =
      TypeRange + many(TypeLeaf<lit<"|">> + TypeRange);

  struct TypeExpr
      : rule<create<ast::TypeExpr>(
            option(TypeLeaf<ref<VAR>> + TypeLeaf<lit<"::">>) + TypeUnion)> {};

  struct TypeGuard
      : rule<create<ast::TypeExpr>(
                 (TypeLeaf<ref<ATOM>> + TypeLeaf<lit<"(">> + some(NestedType, TypeLeaf<lit<",">>) +
                  TypeLeaf<lit<")">>) |
                 (TypeLeaf<ref<VAR>> + TypeLeaf<lit<"::">> + NestedType))> {};

  struct FunType
      : rule<create<ast::TypeExpr>(
                 TypeLeaf<lit<"(">> + option(some(NestedType, TypeLeaf<lit<",">>)) + TypeLeaf<lit<")">> +
                 TypeLeaf<lit<"->">> + NestedType)> {};

  static constexpr auto TypeSigOne =
      append<&ast::TypeSig::signatures>(ref<FunType>) +
      option(kw<"when"> +
             some(append<&ast::TypeSig::signatures>(ref<TypeGuard>),
                  lit<",">));

  struct TypeSig
      : rule<create<ast::TypeSig>(
                 lit<"("> +
                 ((assign<&ast::TypeSig::module>(ref<ATOM>) + lit<":"> +
                   assign<&ast::TypeSig::function>(ref<ATOM>)) |
                  assign<&ast::TypeSig::function>(ref<ATOM>)) +
                 some(TypeSigOne, lit<";">) + lit<")">) |
             create<ast::TypeSig>(
                 ((assign<&ast::TypeSig::module>(ref<ATOM>) + lit<":"> +
                   assign<&ast::TypeSig::function>(ref<ATOM>)) |
                  assign<&ast::TypeSig::function>(ref<ATOM>)) +
                 some(TypeSigOne, lit<";">))> {};

  struct TypedField
      : rule<create<ast::TypedField>(
                 assign<&ast::TypedField::field>(ref<Expr>) +
                 option(lit<"::"> +
                        assign<&ast::TypedField::type>(ref<TypeExpr>)))> {
  };

  struct TypedRecord
      : rule<create<ast::TypedRecord>(
                 assign<&ast::TypedRecord::head>(ref<Expr>) +
                 ((lit<"::"> +
                   assign<&ast::TypedRecord::type>(ref<TypeExpr>)) |
                  (lit<","> + lit<"{"> +
                   many(append<&ast::TypedRecord::fields>(
                            ref<TypedField>),
                        lit<",">) +
                   lit<"}">)))> {};

  struct Attribute
      : rule<create<ast::Attribute>(
                 lit<"-"> + assign<&ast::Attribute::name>(ref<ATOM> | kw<"if"> | kw<"else">) +
                 option(((lit<"("> + some(append<&ast::Attribute::args>(ref<Expr>), lit<",">) + lit<")">) |
                   some(append<&ast::Attribute::args>(ref<Expr>), lit<",">)) |
                  assign<&ast::Attribute::typedRecord>(
                      ref<TypedRecord>) |
                  (lit<"("> +
                   assign<&ast::Attribute::typedRecord>(
                       ref<TypedRecord>) +
                   lit<")">))) |
             create<ast::Attribute>(
                 assign<&ast::Attribute::name>(ref<ATTR_NAME>) +
                 assign<&ast::Attribute::spec>(ref<TypeSig>))> {};

  struct FunctionClause
      : rule<create<ast::FunctionClause>(
                 assign<&ast::FunctionClause::name>(ref<ATOM>) + lit<"("> +
                 many(append<&ast::FunctionClause::params>(ref<Expr>),
                      lit<",">) +
                 lit<")"> +
                 option(kw<"when"> +
                        assign<&ast::FunctionClause::guard>(ref<Guard>)) +
                 lit<"->"> +
                 some(append<&ast::FunctionClause::body>(ref<Expr>),
                      lit<",">))> {};

  struct FunctionDecl
      : rule<create<ast::FunctionDecl>(
                 some(append<&ast::FunctionDecl::clauses>(
                          ref<FunctionClause>),
                      lit<";">))> {};

  struct Forms
      : rule<create<ast::Forms>(many(
                 append<&ast::Forms::forms>(ref<Attribute> |
                                             ref<FunctionDecl>) +
                 lit<".">))> {};

  using entry = Forms;
  using ignored = WS;
  using hidden = list<COMMENT>;
};

}  // namespace erlang
