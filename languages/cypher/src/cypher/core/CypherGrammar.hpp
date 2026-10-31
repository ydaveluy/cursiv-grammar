// The cypher language: its grammar.
#pragma once

#include <cstddef>
#include <memory_resource>
#include <span>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "cypher/core/ast.hpp"

namespace cypher {

inline constexpr auto strip_ticks = [](std::string_view t,
                                       std::pmr::memory_resource* mem) {
  const auto body = t.substr(1, t.size() - 2);
  auto room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  auto hex = [](char c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
  };
  for (std::size_t i = 0; i < body.size(); ++i) {
    if (body[i] == '`' && i + 1 < body.size() && body[i + 1] == '`') {
      room[n++] = '`';
      ++i;
      continue;
    }
    if (body[i] == '\\' && i + 1 < body.size() &&
        (body[i + 1] == 'u' || body[i + 1] == 'U')) {
      const std::size_t count = body[i + 1] == 'u' ? 4 : 8;
      unsigned cp = 0;
      bool valid = i + 2 + count <= body.size();
      for (std::size_t k = 0; valid && k < count; ++k) {
        int v = hex(body[i + 2 + k]);
        if (v < 0)
          valid = false;
        else
          cp = (cp << 4) | v;
      }
      if (valid && cp <= 0x10ffff && !(cp >= 0xd800 && cp <= 0xdfff)) {
        if (cp <= 0x7f)
          room[n++] = static_cast<char>(cp);
        else if (cp <= 0x7ff) {
          room[n++] = static_cast<char>(0xc0 | (cp >> 6));
          room[n++] = static_cast<char>(0x80 | (cp & 63));
        } else if (cp <= 0xffff) {
          room[n++] = static_cast<char>(0xe0 | (cp >> 12));
          room[n++] = static_cast<char>(0x80 | ((cp >> 6) & 63));
          room[n++] = static_cast<char>(0x80 | (cp & 63));
        } else {
          room[n++] = static_cast<char>(0xf0 | (cp >> 18));
          room[n++] = static_cast<char>(0x80 | ((cp >> 12) & 63));
          room[n++] = static_cast<char>(0x80 | ((cp >> 6) & 63));
          room[n++] = static_cast<char>(0x80 | (cp & 63));
        }
        i += count + 1;
        continue;
      }
    }
    room[n++] = body[i];
  }
  return std::string_view(room.data(), n);
};

// Decode native Unicode escapes, including UTF-16 pairs and UTF-32, while
// retaining the port's historical octal/repeated-u extensions.
inline constexpr auto unquote = [](std::string_view t,
                                   std::pmr::memory_resource* mem) {
  const std::string_view body = t.substr(1, t.size() - 2);
  if (body.find('\\') == std::string_view::npos) return body;
  const std::span<char> room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  auto hexVal = [](char c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
  };
  auto putUtf8 = [&](unsigned cp) {
    if (cp <= 0x7F) {
      room[n++] = static_cast<char>(cp);
    } else if (cp <= 0x7FF) {
      room[n++] = static_cast<char>(0xC0 | (cp >> 6));
      room[n++] = static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp <= 0xFFFF) {
      room[n++] = static_cast<char>(0xE0 | (cp >> 12));
      room[n++] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
      room[n++] = static_cast<char>(0x80 | (cp & 0x3F));
    } else {
      room[n++] = static_cast<char>(0xF0 | (cp >> 18));
      room[n++] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
      room[n++] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
      room[n++] = static_cast<char>(0x80 | (cp & 0x3F));
    }
  };
  for (std::size_t i = 0; i < body.size(); ++i) {
    if (body[i] != '\\' || i + 1 >= body.size()) {
      room[n++] = body[i];
      continue;
    }
    const char e = body[++i];
    switch (e) {
      case 'b': room[n++] = '\b'; break;
      case 't': room[n++] = '\t'; break;
      case 'n': room[n++] = '\n'; break;
      case 'f': room[n++] = '\f'; break;
      case 'r': room[n++] = '\r'; break;
      case '"': room[n++] = '"'; break;
      case '\'': room[n++] = '\''; break;
      case '\\': room[n++] = '\\'; break;
      case 'u':
      case 'U': {
        std::size_t j = i + 1;
        if (e == 'u')
          while (j < body.size() && body[j] == 'u') ++j;
        const std::size_t digits = e == 'U' ? 8 : 4;
        if (j + digits <= body.size()) {
          unsigned cp = 0;
          for (std::size_t k = 0; k < digits; ++k)
            cp = (cp << 4) | static_cast<unsigned>(hexVal(body[j + k]));
          i = j + digits - 1;
          if (e == 'u' && cp >= 0xd800 && cp <= 0xdbff &&
              i + 6 < body.size() && body[i + 1] == '\\' && body[i + 2] == 'u') {
            unsigned low = 0;
            for (std::size_t k = 0; k < 4; ++k)
              low = (low << 4) | static_cast<unsigned>(hexVal(body[i + 3 + k]));
            if (low >= 0xdc00 && low <= 0xdfff) {
              cp = 0x10000 + ((cp - 0xd800) << 10) + low - 0xdc00;
              i += 6;
            }
          }
          putUtf8(cp);
        }
        break;
      }
      default:
        if (e >= '0' && e <= '7') {
          unsigned v = static_cast<unsigned>(e - '0');
          for (int k = 0; k < 2 && i + 1 < body.size() && body[i + 1] >= '0' &&
                          body[i + 1] <= '7';
               ++k)
            v = (v << 3) | static_cast<unsigned>(body[++i] - '0');
          room[n++] = static_cast<char>(v & 0xFF);
        } else {
          room[n++] = e;
        }
        break;
    }
  }
  return std::string_view(room.data(), n);
};

struct Cypher : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\n\v\f\r\x1c-\x1f\u00a0\u1680\u180e\u2000-\u200a\u2028\u2029\u202f\u205f\u3000)">)> {};
  struct COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  struct LINE_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};

  // ID_Start/ID_Continue are their Unicode category definitions plus the
  // Other_ID_* characters; openCypher also permits connector starts and
  // currency symbols after the first character.
  static constexpr auto IdentifierStart =
      uc::L | uc::Nl | cr<R"(\u1885\u1886\u2118\u212e\u309b\u309c)">;
  static constexpr auto IdentifierPart =
      IdentifierStart | uc::Mn | uc::Mc | uc::Nd | uc::Pc |
      cr<R"(\u00b7\u0387\u1369-\u1371\u19da\u200c\u200d\u30fb\uff65)"> | uc::Sc;
  static constexpr auto word_boundary = IdentifierPart;

  struct ID : terminal<(IdentifierStart | uc::Pc) + many(IdentifierPart)> {};

  struct ESC_LITERAL : terminal<as<strip_ticks>(
                           lit<"`"> + many(lit<"``"> | cr<"^`">) + lit<"`">)> {
  };
  // A `\` swallows the next byte whatever it is: the exact escape shape
  // (octal run, `\u+HHHH`) is decoded by the converter, not the lexer.
  struct STRING_LITERAL
      : terminal<as<unquote>(lit<"\""> +
                             many((lit<"\\"> + any) | cr<R"(^"\\)">) +
                             lit<"\"">)> {};
  struct CHAR_LITERAL
      : terminal<as<unquote>(lit<"'"> +
                             many((lit<"\\"> + any) | cr<R"(^'\\)">) +
                             lit<"'">)> {};

  static constexpr auto Digit = cr<"0-9">;
  static constexpr auto Digits = Digit + option(many(cr<"0-9_">) + Digit);
  static constexpr auto HexDigit = cr<"0-9a-fA-F">;
  static constexpr auto OctalDigit = cr<"0-7">;
  static constexpr auto HexDigits =
      lit<"0"> + cr<"xX"> + HexDigit +
      option(many(HexDigit | lit<"_">) + HexDigit);
  static constexpr auto OctalDigits =
      lit<"0"> + cr<"oO"> + OctalDigit +
      option(many(OctalDigit | lit<"_">) + OctalDigit);
  static constexpr auto ExponentPart =
      cr<"eE"> + option(cr<"+-">) + Digits;
  static constexpr auto FloatSuffix = cr<"fFdD">;
  static constexpr auto Float =
      (((Digits + lit<"."> + Digits) | (lit<"."> + Digits)) +
          option(ExponentPart) + option(FloatSuffix)) |
      (Digits + ((ExponentPart + option(FloatSuffix)) | FloatSuffix));

  struct NUMBER : terminal<HexDigits | OctalDigits | Float | Digits> {};
  struct INTEGER_LITERAL : terminal<HexDigits | OctalDigits | Digits> {};

  struct ALL : terminal<ikw<"ALL">> {};
  struct ASC : terminal<ikw<"ASC">> {};
  struct ASCENDING : terminal<ikw<"ASCENDING">> {};
  struct BY : terminal<ikw<"BY">> {};
  struct CREATE : terminal<ikw<"CREATE">> {};
  struct DELETE : terminal<ikw<"DELETE">> {};
  struct DESC : terminal<ikw<"DESC">> {};
  struct DESCENDING : terminal<ikw<"DESCENDING">> {};
  struct DETACH : terminal<ikw<"DETACH">> {};
  struct EXISTS : terminal<ikw<"EXISTS">> {};
  struct LIMIT : terminal<ikw<"LIMIT">> {};
  struct MATCH : terminal<ikw<"MATCH">> {};
  struct MERGE : terminal<ikw<"MERGE">> {};
  struct ON : terminal<ikw<"ON">> {};
  struct OPTIONAL : terminal<ikw<"OPTIONAL">> {};
  struct ORDER : terminal<ikw<"ORDER">> {};
  struct REMOVE : terminal<ikw<"REMOVE">> {};
  struct RETURN : terminal<ikw<"RETURN">> {};
  struct SET : terminal<ikw<"SET">> {};
  struct SKIP_KW : terminal<ikw<"SKIP">> {};
  struct WHERE : terminal<ikw<"WHERE">> {};
  struct WITH : terminal<ikw<"WITH">> {};
  struct UNION : terminal<ikw<"UNION">> {};
  struct UNWIND : terminal<ikw<"UNWIND">> {};
  struct AND : terminal<as<ast::Operator::logical_and>(ikw<"AND">)> {};
  struct AS : terminal<ikw<"AS">> {};
  struct CONTAINS : terminal<as<ast::Operator::contains>(ikw<"CONTAINS">)> {};
  struct DISTINCT : terminal<ikw<"DISTINCT">> {};
  struct ENDS : terminal<ikw<"ENDS">> {};
  struct IN : terminal<as<ast::Operator::in_>(ikw<"IN">)> {};
  struct IS : terminal<ikw<"IS">> {};
  struct NOT : terminal<ikw<"NOT">> {};
  struct OR : terminal<as<ast::Operator::logical_or>(ikw<"OR">)> {};
  struct STARTS : terminal<ikw<"STARTS">> {};
  struct XOR : terminal<as<ast::Operator::logical_xor>(ikw<"XOR">)> {};
  struct FALSE : terminal<ikw<"FALSE">> {};
  struct TRUE : terminal<ikw<"TRUE">> {};
  struct NULL_KW : terminal<ikw<"NULL">> {};
  struct CASE : terminal<ikw<"CASE">> {};
  struct WHEN : terminal<ikw<"WHEN">> {};
  struct THEN : terminal<ikw<"THEN">> {};
  struct ELSE : terminal<ikw<"ELSE">> {};
  struct END : terminal<ikw<"END">> {};
  struct COUNT : terminal<ikw<"COUNT">> {};
  struct ANY : terminal<ikw<"ANY">> {};
  struct NONE : terminal<ikw<"NONE">> {};
  struct SINGLE : terminal<ikw<"SINGLE">> {};
  struct YIELD : terminal<ikw<"YIELD">> {};
  struct CALL : terminal<ikw<"CALL">> {};


  static constexpr auto ReservedOnly =
      ikw<"CONSTRAINT"> | ikw<"DO"> | ikw<"FOR"> | ikw<"REQUIRE"> |
      ikw<"UNIQUE"> | ikw<"MANDATORY"> | ikw<"SCALAR"> | ikw<"OF"> |
      ikw<"ADD"> | ikw<"DROP">;

  static constexpr auto ReservedWord =
      ref<ALL> | ref<ASC> | ref<ASCENDING> | ref<BY> | ref<CREATE> |
      ref<DELETE> | ref<DESC> | ref<DESCENDING> | ref<DETACH> | ref<EXISTS> |
      ref<LIMIT> | ref<MATCH> | ref<MERGE> | ref<ON> | ref<OPTIONAL> |
      ref<ORDER> | ref<REMOVE> | ref<RETURN> | ref<SET> | ref<SKIP_KW> |
      ref<WHERE> | ref<WITH> | ref<UNION> | ref<UNWIND> | ref<AND> |
      ref<AS> | ref<CONTAINS> | ref<DISTINCT> | ref<ENDS> | ref<IN> |
      ref<IS> | ref<NOT> | ref<OR> | ref<STARTS> | ref<XOR> | ref<FALSE> |
      ref<TRUE> | ref<NULL_KW> | ref<CASE> | ref<WHEN> | ref<THEN> |
      ref<ELSE> | ReservedOnly;

  // `count`/`any`/`none`/`single`/`end` keep their keyword role elsewhere and
  // their identifier role here, spelled once. `filter`/`extract` carry no
  // keyword role anywhere in this grammar, so the plain ID token already
  // covers them; no separate terminal is needed.
  struct Symbol : rule<ref<ESC_LITERAL> | ref<ID> | ref<COUNT> | ref<ANY> |
                       ref<NONE> | ref<SINGLE> | ref<END>> {};

  // A name is the text it spells, though some reserved words convert.
  struct Name : rule<as<std::string_view>(ref<Symbol> | ReservedWord)> {};

  template <auto Member>
  static constexpr auto NodeLabelsBody =
      some(lit<":"> + append<Member>(ref<Name>));

  struct Expression;
  struct ArithExpr;
  struct Atom;
  struct PropertyExpression;
  struct AtomicExpression;
  struct UnaryAddSubExpression;
  struct RelationshipsChainPattern;
  struct RegularQuery;
  struct PatternWhere;
  struct ForeachClause;

  struct InvocationName
      : rule<ref<Symbol> + many(lit<"."> + ref<Symbol>)> {};

  struct Parameter
      : rule<create<ast::ParamRef>(
            lit<"$"> + assign<&ast::ParamRef::name>(ref<Symbol> |
                                                     ref<NUMBER>))> {};

  struct NullLit : rule<create<ast::NullLit> + ref<NULL_KW>> {};
  struct BoolLit
      : rule<create<ast::BoolLit> +
             (flag<&ast::BoolLit::value>(ref<TRUE>) | ref<FALSE>)> {};
  struct NumberLit : rule<assign<&ast::NumberLit::text>(ref<NUMBER>)> {};
  struct StringLit : rule<assign<&ast::StringLit::value>(ref<STRING_LITERAL> |
                                                          ref<CHAR_LITERAL>)> {
  };

  struct MapPair
      : rule<assign<&ast::MapPair::key>(ref<Name>) + lit<":"> +
             assign<&ast::MapPair::value>(ref<Expression>)> {};
  struct MapLit
      : rule<create<ast::MapLit> + lit<"{"> +
             option(some(append<&ast::MapLit::pairs>(ref<MapPair>),
                         lit<",">)) +
             lit<"}">> {};

  struct ListLit
      : rule<create<ast::ListLit> + lit<"["> +
             option(some(append<&ast::ListLit::items>(ref<Expression>),
                         lit<",">)) +
             lit<"]">> {};

  struct Literal : rule<ref<BoolLit>    //
                        | ref<NumberLit>  //
                        | ref<NullLit>    //
                        | ref<StringLit>  //
                        | ref<ListLit>    //
                        | ref<MapLit>> {};

  struct Properties : rule<ref<MapLit> | ref<Parameter>> {};

  struct NodePattern
      : rule<create<ast::NodePattern> + lit<"("> +
             option(assign<&ast::NodePattern::variable>(ref<Symbol>)) +
             option(NodeLabelsBody<&ast::NodePattern::labels>) +
             option(assign<&ast::NodePattern::properties>(ref<Properties>)) +
             lit<")">> {};

  struct RangeLit
      : rule<create<ast::RangeLit> + lit<"*"> +
             option(assign<&ast::RangeLit::min>(ref<NUMBER>)) +
             option(flag<&ast::RangeLit::hasRange>(lit<"..">) +
                    option(assign<&ast::RangeLit::max>(ref<NUMBER>)))> {};

  static constexpr auto RelationshipTypes =
      lit<":"> + append<&ast::RelationshipDetail::types>(ref<Name>) +
      many(lit<"|"> + option(lit<":">) +
           append<&ast::RelationshipDetail::types>(ref<Name>));

  struct RelationDetail
      : rule<create<ast::RelationshipDetail> + lit<"["> +
             option(assign<&ast::RelationshipDetail::variable>(ref<Symbol>)) +
             option(RelationshipTypes) +
             option(assign<&ast::RelationshipDetail::range>(ref<RangeLit>)) +
             option(
                 assign<&ast::RelationshipDetail::properties>(ref<Properties>)) +
             lit<"]">> {};

  // ASCII arrows/dashes also occur as expression literals. Spell them as
  // literals here too: at equal length literals win over character classes.
  static constexpr auto LeftArrow = lit<"<"> | cr<R"(\u27e8\u3008\ufe64\uff1c)">;
  static constexpr auto RightArrow = lit<">"> | cr<R"(\u27e9\u3009\ufe65\uff1e)">;
  static constexpr auto Dash =
      lit<"-"> | cr<R"(\u00ad\u2010-\u2015\u2212\ufe58\ufe63\uff0d)">;

  struct RelationshipPattern
      : rule<create<ast::RelationshipPattern>(
                 option(flag<&ast::RelationshipPattern::pointsLeft>(
                     LeftArrow)) +
                 Dash +
                 option(
                     assign<&ast::RelationshipPattern::detail>(
                         ref<RelationDetail>)) +
                 Dash +
                 option(
                     flag<&ast::RelationshipPattern::pointsRight>(
                         RightArrow)))> {};

  struct RelChainStep
      : rule<assign<&ast::RelChainStep::rel>(ref<RelationshipPattern>) +
             assign<&ast::RelChainStep::node>(ref<NodePattern>)> {};

  struct Quantifier
      : rule<create<ast::Quantifier> +
             (
                 (lit<"{"> +
                  (
                      (assign<&ast::Quantifier::min>(ref<NUMBER> | ref<ID>) +
                       option(flag<&ast::Quantifier::hasComma>(lit<",">) +
                              option(assign<&ast::Quantifier::max>(
                                  ref<NUMBER> | ref<ID>))))  //
                      | (flag<&ast::Quantifier::hasComma>(lit<",">) +
                         assign<&ast::Quantifier::max>(ref<NUMBER> |
                                                        ref<ID>))  //
                      | flag<&ast::Quantifier::hasComma>(lit<",">)) +
                  lit<"}">)  //
                 | flag<&ast::Quantifier::plus>(lit<"+">)  //
                 | flag<&ast::Quantifier::star>(lit<"*">))> {};

  struct PatternElem;

  struct QppChainStep
      : rule<lit<"("> + assign<&ast::QppChainStep::inner>(ref<PatternElem>) +
             lit<")"> +
             assign<&ast::QppChainStep::quantifier>(ref<Quantifier>) +
             assign<&ast::QppChainStep::node>(ref<NodePattern>)> {};

  struct PatternElem
      : rule<create<ast::SimplePatternElem>(
                 assign<&ast::SimplePatternElem::first>(ref<NodePattern>) +
                 many(append<&ast::SimplePatternElem::chain>(ref<RelChainStep>) |
                      append<&ast::SimplePatternElem::chain>(
                          ref<QppChainStep>)))  //
             | create<ast::GroupedPatternElem>(
                   lit<"("> +
                   assign<&ast::GroupedPatternElem::inner>(ref<PatternElem>) +
                   lit<")"> +
                   option(assign<&ast::GroupedPatternElem::quantifier>(
                       ref<Quantifier>)))> {};

  template<auto All, auto Pattern> static constexpr auto shortest_path =
      ((flag<All>(&ikw<"allShortestPaths"> + ref<Symbol>)) |
       (&ikw<"shortestPath"> + ref<Symbol>)) +
      lit<"("> + assign<Pattern>(ref<PatternElem>) + lit<")">;
  struct ShortestPattern : rule<shortest_path<&ast::ShortestPattern::all,
      &ast::ShortestPattern::inner>> {};
  struct ShortestPathExpr : rule<shortest_path<&ast::ShortestPathExpr::all,
      &ast::ShortestPathExpr::pattern>> {};

  struct PatternPart
      : rule<option(assign<&ast::PatternPart::variable>(ref<Symbol>) +
                    lit<"=">) +
             assign<&ast::PatternPart::elem>(ref<ShortestPattern> | ref<PatternElem>)> {};

  struct Pattern
      : rule<some(append<&ast::Pattern::parts>(ref<PatternPart>), lit<",">)> {
  };

  struct RelationshipsChainPattern
      : rule<assign<&ast::RelChainPatternExpr::first>(ref<NodePattern>) +
             some(append<&ast::RelChainPatternExpr::chain>(ref<RelChainStep>))> {
  };

  struct FilterExpr
      : rule<assign<&ast::FilterExpr::variable>(ref<Symbol>) + ref<IN> +
             assign<&ast::FilterExpr::source>(ref<Expression>) +
             option(ref<WHERE> +
                    assign<&ast::FilterExpr::where_>(ref<Expression>))> {};

  struct ListComprehension
      : rule<create<ast::ListComprehensionExpr> + lit<"["> +
             assign<&ast::ListComprehensionExpr::filter>(ref<FilterExpr>) +
             option(lit<"|"> +
                    assign<&ast::ListComprehensionExpr::projection>(
                        ref<Expression>)) +
             lit<"]">> {};

  struct FilterWith
      : rule<create<ast::FilterWithExpr> +
             assign<&ast::FilterWithExpr::kind>(
                 ref<ALL> | ref<ANY> | ref<NONE> | ref<SINGLE>) +
             lit<"("> +
             assign<&ast::FilterWithExpr::filter>(ref<FilterExpr>) +
             lit<")">> {};

  struct PatternComprehension
      : rule<create<ast::PatternComprehensionExpr> + lit<"["> +
             option(assign<&ast::PatternComprehensionExpr::variable>(
                        ref<Symbol>) +
                    lit<"=">) +
             assign<&ast::PatternComprehensionExpr::pattern>(
                 ref<RelationshipsChainPattern>) +
             option(ref<WHERE> +
                    assign<&ast::PatternComprehensionExpr::where_>(
                        ref<Expression>)) +
             lit<"|"> +
             assign<&ast::PatternComprehensionExpr::projection>(
                 ref<Expression>) +
             lit<"]">> {};

  struct CountAll
      : rule<create<ast::CountAllExpr> + ref<COUNT> + lit<"("> + lit<"*"> +
             lit<")">> {};

  struct CaseWhen
      : rule<ref<WHEN> + assign<&ast::CaseWhen::when>(ref<Expression>) +
             ref<THEN> + assign<&ast::CaseWhen::then>(ref<Expression>)> {};

  struct CaseExpression
      : rule<create<ast::CaseExpr> + ref<CASE> +
             option(assign<&ast::CaseExpr::subject>(ref<Expression>)) +
             some(append<&ast::CaseExpr::whens>(ref<CaseWhen>)) +
             option(ref<ELSE> +
                    assign<&ast::CaseExpr::elseValue>(ref<Expression>)) +
             ref<END>> {};

  struct SubqueryExist
      : rule<create<ast::ExistsExpr> + ref<EXISTS> + lit<"{"> +
             (assign<&ast::ExistsExpr::query>(ref<RegularQuery>)  //
              | (assign<&ast::ExistsExpr::pattern>(ref<Pattern>) +
                 option(ref<WHERE> +
                        assign<&ast::ExistsExpr::where_>(ref<Expression>)))) +
             lit<"}">> {};

  struct FunctionInvocation
      : rule<create<ast::FuncCallExpr>(
            assign<&ast::FuncCallExpr::name>(ref<InvocationName>) + lit<"("> +
            option(flag<&ast::FuncCallExpr::distinct>(ref<DISTINCT>)) +
            option(some(append<&ast::FuncCallExpr::args>(ref<Expression>),
                        lit<",">)) +
            lit<")">)> {};

  struct NameRef
      : rule<create<ast::NameRef> +
             assign<&ast::NameRef::name>(ref<Symbol>)> {};

  struct Atom
      : rule<ref<ListComprehension>            //
             | ref<Literal>                    //
             | ref<Parameter>                  //
             | ref<CaseExpression>             //
             | ref<CountAll>                   //
             | ref<PatternComprehension>       //
             | ref<FilterWith>                 //
             | ref<ShortestPathExpr>           //
             | ref<RelationshipsChainPattern>  //
             | create<ast::GroupExpr>(
                   lit<"("> + assign<&ast::GroupExpr::value>(ref<Expression>) +
                   lit<")">)            //
             | ref<FunctionInvocation>  //
             | ref<NameRef>             //
             | ref<SubqueryExist>> {};

  static constexpr auto PropertyStep =
      nest<ast::PropertyExpr, &ast::PropertyExpr::object>(
          lit<"."> + assign<&ast::PropertyExpr::property>(ref<Name>));
  struct PropertyExpression : rule<ref<Atom> + some(PropertyStep)> {};

  struct AtomicExpression
      : rule<ref<Atom> +
             many(PropertyStep |
                  nest<ast::SliceExpr, &ast::SliceExpr::object>(
                      lit<"["> +
                      option(assign<&ast::SliceExpr::from_>(ref<Expression>)) +
                      lit<".."> +
                      option(assign<&ast::SliceExpr::to_>(ref<Expression>)) +
                      lit<"]">)  //
                  | nest<ast::IndexExpr, &ast::IndexExpr::object>(
                        lit<"["> +
                        assign<&ast::IndexExpr::index>(ref<Expression>) +
                        lit<"]">)) +
             option(nest<ast::HasLabelsExpr, &ast::HasLabelsExpr::operand>(
                 NodeLabelsBody<&ast::HasLabelsExpr::labels>))> {};

  struct UnaryAddSubExpression
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-">) +
                 assign<&ast::UnaryExpr::operand>(ref<AtomicExpression>))  //
             | ref<AtomicExpression>> {};


  static constexpr auto CaretOp = as<ast::Operator::power>(lit<"^">);
  static constexpr auto MulDivModOp =
      as<ast::Operator::multiply>(lit<"*">) | as<ast::Operator::divide>(lit<"/">) | as<ast::Operator::modulo>(lit<"%">);
  static constexpr auto AddSubOp = as<ast::Operator::add>(lit<"+">) | as<ast::Operator::subtract>(lit<"-">);

  struct ArithExpr
      : infix<UnaryAddSubExpression, ast::BinaryExpr,
              levels(right(CaretOp), left(MulDivModOp), left(AddSubOp))> {};

  static constexpr auto StringPrefixOp =
      as<ast::Operator::starts_with>(ref<STARTS> + ref<WITH>) | as<ast::Operator::ends_with>(ref<ENDS> + ref<WITH>) | ref<CONTAINS> | as<ast::Operator::regex_match>(lit<"=~">);

  struct StringListNullExpression
      : rule<ref<ArithExpr> +
             many(
                 nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                     assign<&ast::BinaryExpr::op>(StringPrefixOp) +
                     assign<&ast::BinaryExpr::right>(ref<ArithExpr>))  //
                 | nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                       assign<&ast::BinaryExpr::op>(ref<IN>) +
                       assign<&ast::BinaryExpr::right>(ref<ArithExpr>))  //
                 | nest<ast::IsNullExpr, &ast::IsNullExpr::operand>(
                       ref<IS> +
                       option(flag<&ast::IsNullExpr::negated>(
                           ref<NOT>)) +
                       ref<NULL_KW>))> {};


  static constexpr auto ComparisonOp = as<ast::Operator::equal>(lit<"=">) | as<ast::Operator::less_equal>(lit<"<=">) |
                                       as<ast::Operator::greater_equal>(lit<">=">) | as<ast::Operator::not_equal>(lit<"<>">) |
                                       as<ast::Operator::less>(lit<"<">) | as<ast::Operator::greater>(lit<">">);

  // Preserve the chain as written: a < b < c compares adjacent operands.
  // GroupExpr distinguishes it from (a < b) < c.
  struct ComparisonExpression
      : rule<create<ast::ComparisonChain>(
                 append<&ast::ComparisonChain::operands>(
                     ref<StringListNullExpression>) +
                 some(append<&ast::ComparisonChain::operators>(ComparisonOp) +
                      append<&ast::ComparisonChain::operands>(
                          ref<StringListNullExpression>))) |
             ref<StringListNullExpression>> {};

  struct NotExpression
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ref<NOT>) +
                 assign<&ast::UnaryExpr::operand>(ref<NotExpression>))  //
             | ref<ComparisonExpression>> {};

  struct AndExpression
      : infix<NotExpression, ast::BinaryExpr, left(ref<AND>)> {};

  struct XorExpression
      : infix<AndExpression, ast::BinaryExpr, left(ref<XOR>)> {};

  struct Expression
      : infix<XorExpression, ast::BinaryExpr, left(ref<OR>)> {};

  // reading / updating clauses

  struct IndexHint : rule<ikw<"INDEX"> +
       assign<&ast::IndexHint::variable>(ref<Symbol>) + lit<":"> +
       assign<&ast::IndexHint::label>(ref<Name>) + lit<"("> +
       assign<&ast::IndexHint::property>(ref<Name>) + lit<")">> {};
  struct JoinHint : rule<ikw<"JOIN"> + ref<ON> +
      some(append<&ast::JoinHint::variables>(ref<Symbol>), lit<",">)> {};
  struct ScanHint : rule<ikw<"SCAN"> +
      assign<&ast::ScanHint::variable>(ref<Symbol>) + lit<":"> +
      assign<&ast::ScanHint::label>(ref<Name>)> {};
  struct MatchHint : rule<ikw<"USING"> +
      (ref<IndexHint> | ref<JoinHint> | ref<ScanHint>)> {};

  struct LoadCsvClause : rule<ikw<"LOAD"> + ikw<"CSV"> +
      option(ref<WITH> + flag<&ast::LoadCsvClause::headers>(ikw<"HEADERS">)) +
      ikw<"FROM"> + assign<&ast::LoadCsvClause::source>(ref<Expression>) + ref<AS> +
      assign<&ast::LoadCsvClause::variable>(ref<Symbol>) +
      option(ikw<"FIELDTERMINATOR"> + assign<&ast::LoadCsvClause::fieldTerminator>(ref<StringLit>))> {};

  struct MatchClause
      : rule<option(flag<&ast::MatchClause::optional_>(ref<OPTIONAL>)) +
             ref<MATCH> +
             assign<&ast::MatchClause::pattern>(ref<Pattern>) +
             many(append<&ast::MatchClause::hints>(ref<MatchHint>)) +
             option(ref<WHERE> +
                    assign<&ast::MatchClause::where_>(ref<Expression>))> {};

  struct UnwindClause
      : rule<ref<UNWIND> +
             assign<&ast::UnwindClause::source>(ref<Expression>) + ref<AS> +
             assign<&ast::UnwindClause::alias>(ref<Symbol>)> {};

  struct YieldItem
      : rule<option(assign<&ast::YieldItem::name>(ref<Symbol>) + ref<AS>) +
             assign<&ast::YieldItem::alias>(ref<Symbol>)> {};

  struct QueryCallClause
      : rule<ref<CALL> +
             assign<&ast::QueryCallClause::name>(ref<InvocationName>) +
             lit<"("> +
             option(some(append<&ast::QueryCallClause::args>(ref<Expression>),
                         lit<",">)) +
             lit<")"> +
             option(ref<YIELD> +
                    some(append<&ast::QueryCallClause::yieldItems>(
                             ref<YieldItem>),
                         lit<",">) +
                    option(ref<WHERE> +
                           assign<&ast::QueryCallClause::where_>(
                               ref<Expression>)))> {};

  struct CreateClause
      : rule<ref<CREATE> +
             assign<&ast::CreateClause::pattern>(ref<Pattern>)> {};

  struct SetProperty
      : rule<assign<&ast::SetProperty::target>(ref<PropertyExpression>) +
             lit<"="> +
             assign<&ast::SetProperty::value>(ref<Expression>)> {};
  struct SetVariable
      : rule<assign<&ast::SetVariable::symbol>(ref<Symbol>) +
             (flag<&ast::SetVariable::add>(lit<"+=">) | lit<"=">) +
             assign<&ast::SetVariable::value>(ref<Expression>)> {};
  struct SetLabels
      : rule<assign<&ast::SetLabels::symbol>(ref<Symbol>) +
             NodeLabelsBody<&ast::SetLabels::labels>> {};
  struct SetClause
      : rule<create<ast::SetClause> + ref<SET> +
             some(append<&ast::SetClause::items>(ref<SetProperty>)     //
                      | append<&ast::SetClause::items>(ref<SetVariable>)  //
                      | append<&ast::SetClause::items>(ref<SetLabels>),
                  lit<",">)> {};

  struct MergeAction
      : rule<ref<ON> +
             (flag<&ast::MergeAction::onCreate>(ref<CREATE>) |
              ref<MATCH>) +
             assign<&ast::MergeAction::set>(ref<SetClause>)> {};

  struct MergeClause
      : rule<ref<MERGE> +
             assign<&ast::MergeClause::part>(ref<PatternPart>) +
             many(append<&ast::MergeClause::actions>(ref<MergeAction>))> {};

  struct DeleteClause
      : rule<option(flag<&ast::DeleteClause::detach>(ref<DETACH>)) +
             ref<DELETE> +
             some(append<&ast::DeleteClause::items>(ref<Expression>),
                  lit<",">)> {};

  struct RemoveLabels
      : rule<assign<&ast::RemoveLabels::symbol>(ref<Symbol>) +
             NodeLabelsBody<&ast::RemoveLabels::labels>> {};
  struct RemoveClause
      : rule<create<ast::RemoveClause> + ref<REMOVE> +
             some(append<&ast::RemoveClause::items>(ref<RemoveLabels>)  //
                      | append<&ast::RemoveClause::items>(
                            ref<PropertyExpression>),
                  lit<",">)> {};

  template<auto Member> static constexpr auto updating =
      append<Member>(ref<CreateClause>) | append<Member>(ref<MergeClause>) |
      append<Member>(ref<DeleteClause>) | append<Member>(ref<SetClause>) |
      append<Member>(ref<RemoveClause>) | append<Member>(ref<ForeachClause>);
  struct ForeachClause : rule<ikw<"FOREACH"> + lit<"("> +
      assign<&ast::ForeachClause::variable>(ref<Symbol>) + ref<IN> +
      assign<&ast::ForeachClause::source>(ref<Expression>) + lit<"|"> +
      some(updating<&ast::ForeachClause::updates>) + lit<")">> {};

  // projection

  struct OrderItem
      : rule<assign<&ast::OrderItem::key>(ref<Expression>) +
             option(flag<&ast::OrderItem::descending>(
                        ref<DESCENDING> | ref<DESC>) |
                    ref<ASCENDING> | ref<ASC>)> {};

  struct OrderClause
      : rule<create<ast::OrderClause> + ref<ORDER> + ref<BY> +
             some(append<&ast::OrderClause::items>(ref<OrderItem>),
                  lit<",">)> {};

  struct ProjectionItem
      : rule<assign<&ast::ProjectionItem::value>(ref<Expression>) +
             option(ref<AS> +
                    assign<&ast::ProjectionItem::alias>(ref<Symbol>))> {};

  struct ProjectionBody
      : rule<create<ast::ProjectionBody> +
             option(flag<&ast::ProjectionBody::distinct>(ref<DISTINCT>)) +
             (flag<&ast::ProjectionBody::star>(lit<"*">)  //
              | append<&ast::ProjectionBody::items>(ref<ProjectionItem>)) +
             many(lit<","> +
                  append<&ast::ProjectionBody::items>(ref<ProjectionItem>)) +
             option(assign<&ast::ProjectionBody::orderBy>(ref<OrderClause>)) +
             option(ref<SKIP_KW> +
                    assign<&ast::ProjectionBody::skip_>(ref<Expression>)) +
             option(ref<LIMIT> +
                    assign<&ast::ProjectionBody::limit_>(ref<Expression>))> {
  };

  struct WithClause
      : rule<ref<WITH> +
             assign<&ast::WithClause::body>(ref<ProjectionBody>) +
             option(ref<WHERE> +
                    assign<&ast::WithClause::where_>(ref<Expression>))> {};

  struct ReturnClause
      : rule<ref<RETURN> +
             assign<&ast::ReturnClause::body>(ref<ProjectionBody>)> {};

  static constexpr auto Reading =
      append<&ast::QueryPart::statements>(ref<MatchClause>)     //
      | append<&ast::QueryPart::statements>(ref<UnwindClause>)  //
      | append<&ast::QueryPart::statements>(ref<QueryCallClause>)
      | append<&ast::QueryPart::statements>(ref<LoadCsvClause>);

  struct WithPart
      : rule<create<ast::QueryPart> + many(Reading) +
             many(updating<&ast::QueryPart::statements>) +
             assign<&ast::QueryPart::with>(ref<WithClause>)> {};

  struct FinalPart
      : rule<create<ast::QueryPart> + many(Reading) +
             (assign<&ast::QueryPart::ret>(ref<ReturnClause>)  //
              | (some(updating<&ast::QueryPart::statements>) +
                 option(assign<&ast::QueryPart::ret>(ref<ReturnClause>))))> {
  };

  struct SingleQuery
      : rule<create<ast::SingleQuery> +
             ((some(append<&ast::SingleQuery::parts>(ref<WithPart>)) +
                  append<&ast::SingleQuery::parts>(ref<FinalPart>))  //
              | append<&ast::SingleQuery::parts>(ref<FinalPart>))> {};

  struct UnionClause
      : rule<create<ast::UnionClause> + ref<UNION> +
             option(flag<&ast::UnionClause::all>(ref<ALL>)) +
             assign<&ast::UnionClause::query>(ref<SingleQuery>)> {};

  struct RegularQuery
      : rule<create<ast::RegularQuery> +
             assign<&ast::RegularQuery::first>(ref<SingleQuery>) +
             many(append<&ast::RegularQuery::unions>(ref<UnionClause>))> {};

  struct StandaloneCall
      : rule<create<ast::StandaloneCall> + ref<CALL> +
             assign<&ast::StandaloneCall::name>(ref<InvocationName>) +
             option(lit<"("> +
                    option(some(append<&ast::StandaloneCall::args>(
                                    ref<Expression>),
                                lit<",">)) +
                    lit<")">) +
             option(ref<YIELD> +
                    (flag<&ast::StandaloneCall::yieldAll>(lit<"*">)  //
                     | (some(append<&ast::StandaloneCall::yieldItems>(
                                 ref<YieldItem>),
                             lit<",">) +
                        option(ref<WHERE> +
                               assign<&ast::StandaloneCall::where_>(
                                   ref<Expression>)))))> {};

  struct BulkImportQuery : rule<ikw<"USING"> + ikw<"PERIODIC"> + ikw<"COMMIT"> +
      option(assign<&ast::BulkImportQuery::batchSize>(ref<INTEGER_LITERAL>)) +
      assign<&ast::BulkImportQuery::load>(ref<LoadCsvClause>) +
      assign<&ast::BulkImportQuery::query>(ref<SingleQuery>)> {};

  struct Script
      : rule<assign<&ast::Script::query>(ref<RegularQuery> |
                                         ref<StandaloneCall> | ref<BulkImportQuery>) +
             option(lit<";">)> {};

  using entry = Script;
  using ignored = WS;
  using hidden = list<COMMENT, LINE_COMMENT>;
};

}  // namespace cypher
