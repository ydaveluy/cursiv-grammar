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
using Ticked = cursiv::text_conv<strip_ticks>;

// Decodes `\b \t \n \f \r \" \' \\`, a 1-3 digit octal escape and `\u+HHHH`
// (one or more `u`, then four hex digits); the surrounding quote is stripped
// first.
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
      case 'u': {
        std::size_t j = i + 1;
        while (j < body.size() && body[j] == 'u') ++j;
        if (j + 4 <= body.size()) {
          unsigned cp = 0;
          for (std::size_t k = 0; k < 4; ++k)
            cp = (cp << 4) | static_cast<unsigned>(hexVal(body[j + k]));
          putUtf8(cp);
          i = j + 3;
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
using Unquoted = cursiv::text_conv<unquote>;

struct Cypher : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n\f)">)> {};
  struct COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  struct LINE_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};

  static constexpr auto word_boundary =
      cr<"a-zA-Z0-9_$"> | uc::XID_Continue;

  struct ID : terminal<(cr<"a-zA-Z_"> | uc::XID_Start) +
                       many(cr<"a-zA-Z0-9_"> | uc::XID_Continue)> {};

  struct ESC_LITERAL
      : terminal<lit<"`"> + many(lit<"``"> | cr<"^`">) + lit<"`">, Ticked> {};
  // A `\` swallows the next byte whatever it is: the exact escape shape
  // (octal run, `\u+HHHH`) is decoded by the converter, not the lexer.
  struct STRING_LITERAL
      : terminal<lit<"\""> +
                     many(lit<"\\"> + any | cr<R"(^"\\\r\n)">) + lit<"\"">,
                 Unquoted> {};
  struct CHAR_LITERAL
      : terminal<lit<"'"> + many(lit<"\\"> + any | cr<R"(^'\\\r\n)">) + lit<"'">,
                 Unquoted> {};

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
      ((Digits + lit<"."> + Digits) | (lit<"."> + Digits)) +
          option(ExponentPart) + option(FloatSuffix) |
      Digits + (ExponentPart + option(FloatSuffix) | FloatSuffix);

  struct NUMBER : terminal<HexDigits | OctalDigits | Float | Digits> {};

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
  struct AND : terminal<ikw<"AND">, value<ast::Operator::logical_and>> {};
  struct AS : terminal<ikw<"AS">> {};
  struct CONTAINS
      : terminal<ikw<"CONTAINS">, value<ast::Operator::contains>> {};
  struct DISTINCT : terminal<ikw<"DISTINCT">> {};
  struct ENDS : terminal<ikw<"ENDS">> {};
  struct IN : terminal<ikw<"IN">, value<ast::Operator::in_>> {};
  struct IS : terminal<ikw<"IS">> {};
  struct NOT : terminal<ikw<"NOT">> {};
  struct OR : terminal<ikw<"OR">, value<ast::Operator::logical_or>> {};
  struct STARTS : terminal<ikw<"STARTS">> {};
  struct XOR : terminal<ikw<"XOR">, value<ast::Operator::logical_xor>> {};
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

  struct STARTS_WITH
      : rule<ref<STARTS> + ref<WITH>, value<ast::Operator::starts_with>> {};
  struct ENDS_WITH
      : rule<ref<ENDS> + ref<WITH>, value<ast::Operator::ends_with>> {};

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

  struct Name : rule<ref<Symbol> | ReservedWord> {};

  template <auto Member>
  static constexpr auto NodeLabelsBody =
      some(lit<":"> + append<Member>(ref<Name>));

  struct Expression;
  struct ArithExpr;
  struct Atom;
  struct PropertyExpression;
  struct PropertyOrLabelExpression;
  struct AtomicExpression;
  struct UnaryAddSubExpression;
  struct RelationshipsChainPattern;
  struct RegularQuery;
  struct PatternWhere;

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

  struct RelationshipPattern
      : rule<create<ast::RelationshipPattern>(
                 option(flag<&ast::RelationshipPattern::pointsLeft>(
                     lit<"<">)) +
                 lit<"-"> +
                 option(
                     assign<&ast::RelationshipPattern::detail>(
                         ref<RelationDetail>)) +
                 lit<"-"> +
                 option(
                     flag<&ast::RelationshipPattern::pointsRight>(
                         lit<">">)))> {};

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

  struct PatternPart
      : rule<option(assign<&ast::PatternPart::variable>(ref<Symbol>) +
                    lit<"=">) +
             assign<&ast::PatternPart::elem>(ref<PatternElem>)> {};

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
             | ref<RelationshipsChainPattern>  //
             | create<ast::GroupExpr>(
                   lit<"("> + assign<&ast::GroupExpr::value>(ref<Expression>) +
                   lit<")">)            //
             | ref<FunctionInvocation>  //
             | ref<NameRef>             //
             | ref<SubqueryExist>> {};

  struct PropertyExpression
      : rule<ref<Atom> +
             many(nest<ast::PropertyExpr, &ast::PropertyExpr::object>(
                 lit<"."> +
                 assign<&ast::PropertyExpr::property>(ref<Name>)))> {};

  struct PropertyOrLabelExpression
      : rule<ref<PropertyExpression> +
             option(nest<ast::HasLabelsExpr, &ast::HasLabelsExpr::operand>(
                 NodeLabelsBody<&ast::HasLabelsExpr::labels>))> {};

  struct AtomicExpression
      : rule<ref<PropertyOrLabelExpression> +
             many(nest<ast::SliceExpr, &ast::SliceExpr::object>(
                      lit<"["> +
                      option(assign<&ast::SliceExpr::from_>(ref<Expression>)) +
                      lit<".."> +
                      option(assign<&ast::SliceExpr::to_>(ref<Expression>)) +
                      lit<"]">)  //
                  | nest<ast::IndexExpr, &ast::IndexExpr::object>(
                        lit<"["> +
                        assign<&ast::IndexExpr::index>(ref<Expression>) +
                        lit<"]">))> {};

  struct UnaryAddSubExpression
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-">) +
                 assign<&ast::UnaryExpr::operand>(ref<AtomicExpression>))  //
             | ref<AtomicExpression>> {};

  struct MULTIPLY : terminal<lit<"*">, value<ast::Operator::multiply>> {};
  struct DIVIDE : terminal<lit<"/">, value<ast::Operator::divide>> {};
  struct MODULO : terminal<lit<"%">, value<ast::Operator::modulo>> {};
  struct ADD_OP : terminal<lit<"+">, value<ast::Operator::add>> {};
  struct SUBTRACT : terminal<lit<"-">, value<ast::Operator::subtract>> {};

  struct POWER : terminal<lit<"^">, value<ast::Operator::power>> {};
  static constexpr auto CaretOp = ref<POWER>;
  static constexpr auto MulDivModOp =
      ref<MULTIPLY> | ref<DIVIDE> | ref<MODULO>;
  static constexpr auto AddSubOp = ref<ADD_OP> | ref<SUBTRACT>;

  struct ArithExpr
      : infix<UnaryAddSubExpression, &ast::BinaryExpr::left,
              &ast::BinaryExpr::op, &ast::BinaryExpr::right,
              (right(CaretOp) >> left(MulDivModOp) >> left(AddSubOp))> {};

  struct REGEX_MATCH : terminal<lit<"=~">, value<ast::Operator::regex_match>> {
  };
  static constexpr auto StringPrefixOp =
      ref<STARTS_WITH> | ref<ENDS_WITH> | ref<CONTAINS> | ref<REGEX_MATCH>;

  struct StringListNullExpression
      : rule<ref<ArithExpr> +
             option(
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

  struct EQUAL : terminal<lit<"=">, value<ast::Operator::equal>> {};
  struct LESS_EQUAL : terminal<lit<"<=">, value<ast::Operator::less_equal>> {};
  struct GREATER_EQUAL
      : terminal<lit<">=">, value<ast::Operator::greater_equal>> {};
  struct NOT_EQUAL : terminal<lit<"<>">, value<ast::Operator::not_equal>> {};
  struct LESS : terminal<lit<"<">, value<ast::Operator::less>> {};
  struct GREATER : terminal<lit<">">, value<ast::Operator::greater>> {};

  static constexpr auto ComparisonOp = ref<EQUAL> | ref<LESS_EQUAL> |
                                       ref<GREATER_EQUAL> | ref<NOT_EQUAL> |
                                       ref<LESS> | ref<GREATER>;

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
      : infix<NotExpression, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(ref<AND>)> {};

  struct XorExpression
      : infix<AndExpression, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(ref<XOR>)> {};

  struct Expression
      : infix<XorExpression, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(ref<OR>)> {};

  // reading / updating clauses

  struct MatchClause
      : rule<option(flag<&ast::MatchClause::optional_>(ref<OPTIONAL>)) +
             ref<MATCH> +
             assign<&ast::MatchClause::pattern>(ref<Pattern>) +
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

  static constexpr auto ReadingOrUpdating =
      append<&ast::QueryPart::statements>(ref<MatchClause>)   //
      | append<&ast::QueryPart::statements>(ref<UnwindClause>)  //
      | append<&ast::QueryPart::statements>(ref<QueryCallClause>)  //
      | append<&ast::QueryPart::statements>(ref<CreateClause>)  //
      | append<&ast::QueryPart::statements>(ref<MergeClause>)   //
      | append<&ast::QueryPart::statements>(ref<DeleteClause>)  //
      | append<&ast::QueryPart::statements>(ref<SetClause>)     //
      | append<&ast::QueryPart::statements>(ref<RemoveClause>);

  static constexpr auto Reading =
      append<&ast::QueryPart::statements>(ref<MatchClause>)     //
      | append<&ast::QueryPart::statements>(ref<UnwindClause>)  //
      | append<&ast::QueryPart::statements>(ref<QueryCallClause>);

  struct WithPart
      : rule<create<ast::QueryPart> + many(ReadingOrUpdating) +
             assign<&ast::QueryPart::with>(ref<WithClause>)> {};

  struct FinalPart
      : rule<create<ast::QueryPart> + many(Reading) +
             (assign<&ast::QueryPart::ret>(ref<ReturnClause>)  //
              | (some(append<&ast::QueryPart::statements>(ref<CreateClause>) |
                      append<&ast::QueryPart::statements>(ref<MergeClause>) |
                      append<&ast::QueryPart::statements>(ref<DeleteClause>) |
                      append<&ast::QueryPart::statements>(ref<SetClause>) |
                      append<&ast::QueryPart::statements>(ref<RemoveClause>)) +
                 option(assign<&ast::QueryPart::ret>(ref<ReturnClause>))))> {
  };

  struct SingleQuery
      : rule<create<ast::SingleQuery> +
             (some(append<&ast::SingleQuery::parts>(ref<WithPart>)) +
                  append<&ast::SingleQuery::parts>(ref<FinalPart>)  //
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

  struct Script
      : rule<assign<&ast::Script::query>(ref<RegularQuery> |
                                         ref<StandaloneCall>) +
             option(lit<";">)> {};

  using entry = Script;
  using ignore = WS;
  using hidden = list<COMMENT, LINE_COMMENT>;
};

}  // namespace cypher
