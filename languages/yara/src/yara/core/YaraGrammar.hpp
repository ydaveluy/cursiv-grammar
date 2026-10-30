#pragma once
#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "yara/core/ast.hpp"
#include <cstdint>
#include <limits>
#include <memory_resource>
#include <string_view>

namespace yara {
inline constexpr auto hexDigit = [](char c) {
  return c >= '0' && c <= '9'   ? c - '0'
         : c >= 'a' && c <= 'f' ? c - 'a' + 10
         : c >= 'A' && c <= 'F' ? c - 'A' + 10
                                : -1;
};
inline constexpr auto stringValue =
    [](std::string_view text,
       std::pmr::memory_resource *memory) -> std::string_view {
  if (text.size() < 2) return text;
  text = text.substr(1, text.size() - 2);
  if (text.find('\\') == text.npos)
    return text;
  auto *out = static_cast<char *>(memory->allocate(text.size(), alignof(char)));
  std::size_t size = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    char c = text[i];
    if (c == '\\' && i + 1 < text.size()) {
      c = text[++i];
      switch (c) {
      case 'n':
        c = '\n';
        break;
      case 'r':
        c = '\r';
        break;
      case 't':
        c = '\t';
        break;
      case '"':
      case '\\':
        break;
      case 'x':
        if (i + 2 < text.size() && hexDigit(text[i + 1]) >= 0 &&
            hexDigit(text[i + 2]) >= 0) {
          c = static_cast<char>(hexDigit(text[i + 1]) * 16 +
                                hexDigit(text[i + 2]));
          i += 2;
          break;
        }
        [[fallthrough]];
      default:
        out[size++] = '\\';
        break;
      }
    }
    out[size++] = c;
  }
  return {out, size};
};
inline constexpr auto includePath = [](std::string_view text) {
  if (text.size() < 2) return text;
  return text.substr(1, text.size() - 2);
};
inline constexpr auto numberSource =
    [](cursiv::Parts parts) -> std::string_view {
  if (parts.empty())
    return {};
  const auto first = parts.front(), last = parts.back();
  return {first.data(),
          static_cast<std::size_t>(last.data() + last.size() - first.data())};
};

struct Yara : cursiv::GrammarSpec {
  static constexpr auto word_boundary = cr<"a-zA-Z0-9_">;
  struct WHITE_SPACE : terminal<some(cr<R"( \t\r\n)">)> {};
  struct BLOCK_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  struct LINE_COMMENT : terminal<lit<"//"> + many(cr<R"(^\n)">)> {};
  static constexpr auto Id = cr<"a-zA-Z_"> + many(cr<"a-zA-Z0-9_">);
  static constexpr auto Functions =
      kw<"int8"> | kw<"int8be"> | kw<"uint8"> | kw<"uint8be"> | kw<"int16"> |
      kw<"int16be"> | kw<"uint16"> | kw<"uint16be"> | kw<"int32"> |
      kw<"int32be"> | kw<"uint32"> | kw<"uint32be">;
  static constexpr auto Reserved = Functions |
      kw<"private"> | kw<"global"> | kw<"rule"> | kw<"meta"> |
      kw<"strings"> | kw<"ascii"> | kw<"wide"> | kw<"xor"> |
      kw<"fullword"> | kw<"nocase"> | kw<"condition"> | kw<"true"> |
      kw<"false"> | kw<"not"> | kw<"and"> | kw<"or"> |
      kw<"at"> | kw<"in"> | kw<"of"> | kw<"them"> |
      kw<"for"> | kw<"all"> | kw<"any"> | kw<"none"> |
      kw<"entrypoint"> | kw<"filesize"> | kw<"matches"> | kw<"contains"> |
      kw<"startswith"> | kw<"endswith"> | kw<"icontains"> | kw<"istartswith"> |
      kw<"iendswith"> | kw<"iequals"> | kw<"import"> | kw<"defined">;
  struct IdentifierToken : terminal<Id> {};
  struct ID : rule<as<std::string_view>(!Reserved + ref<IdentifierToken>)> {};
  struct STRING_ID : terminal<lit<"$"> + many(cr<"a-zA-Z0-9_">)> {};
  struct STRING_WILD : terminal<lit<"$"> + many(cr<"a-zA-Z0-9_">) + lit<"*">> {
  };
  struct COUNT_REF : terminal<lit<"#"> + many(cr<"a-zA-Z0-9_">)> {};
  struct OFFSET_REF : terminal<lit<"@"> + many(cr<"a-zA-Z0-9_">)> {};
  struct LENGTH_REF : terminal<lit<"!"> + many(cr<"a-zA-Z0-9_">)> {};
  struct INTEGER : terminal<(some(d) + option(lit<"KB"> | lit<"MB">)) |
                            (lit<"0x"> + some(cr<"a-fA-F0-9">)) |
                            (lit<"0o"> + some(cr<"0-7">))> {};
  struct REAL : terminal<some(d) + lit<"."> + some(d)> {};
  struct IntegerValue
      : rule<as<numberSource>(option(lit<"-">) + ref<INTEGER>)> {};
  struct RealValue : rule<as<double>(option(lit<"-">) + ref<REAL>)> {};
  struct BOOLEAN : terminal<as<bool>(kw<"true"> | kw<"false">)> {};
  struct TEXT : terminal<as<stringValue>(
                    lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\\n)">) +
                    lit<"\"">)> {};
  struct INCLUDE_PATH
      : terminal<as<includePath>(lit<"\""> + many(cr<R"(^")">) + lit<"\"">)> {};
  struct REGEX : terminal<lit<"/"> + !lit<"*"> +
                          some((lit<"\\"> + any) | cr<R"(^/\\\n)">) + lit<"/"> +
                          option(lit<"i">) + option(lit<"s">)> {};
  struct HEX_BYTE : terminal<option(lit<"~">) + repeat<2>(cr<"a-fA-F0-9?">)> {};
  struct RulePattern : rule<as<std::string_view>(ref<ID> + option(lit<"*">))> {
  };

  struct Identifier : rule<assign<&ast::Literal::text>(ref<ID>)> {};
  struct Literal
      : rule<flag<&ast::Literal::quoted>(
                 assign<&ast::Literal::text>(ref<TEXT>)) |
             assign<&ast::Literal::text>(
                 ref<INTEGER> | ref<REAL> | ref<STRING_ID> | ref<COUNT_REF> |
                 ref<OFFSET_REF> | ref<LENGTH_REF> | ref<ID> | Functions |
                 kw<"entrypoint"> | kw<"filesize"> | kw<"true"> | kw<"false"> |
                 kw<"all"> | kw<"any"> | kw<"none">)> {};
  struct Expr;
  struct Arith;
  struct ForExpr;
  struct Regex : rule<assign<&ast::RegexLit::text>(ref<REGEX>)> {};
  struct Range
      : rule<lit<"("> + assign<&ast::Range::lo>(ref<Arith>) + lit<".."> +
             assign<&ast::Range::hi>(ref<Arith>) + lit<")">> {};
  struct Set
      : rule<lit<"("> + append<&ast::SetExpr::items>(ref<Expr>) +
             some(lit<","> + append<&ast::SetExpr::items>(ref<Expr>)) +
             lit<")">> {};
  struct IterableSet
      : rule<lit<"("> +
             some(append<&ast::SetExpr::items>(ref<Expr>), lit<",">) +
             lit<")">> {};
  struct Primary : rule<ref<ForExpr> | ref<Range> | ref<Set> |
                        (lit<"("> + ref<Expr> + lit<")">) | ref<Regex> |
                        ref<Literal>> {};
  struct Postfix
      : rule<ref<Primary> +
             many(nest<ast::CallExpr, &ast::CallExpr::callee>(
                      lit<"("> +
                      option(some(append<&ast::CallExpr::args>(ref<Expr>),
                                  lit<",">)) +
                      lit<")">) |
                  nest<ast::IndexExpr, &ast::IndexExpr::base>(
                      lit<"["> + assign<&ast::IndexExpr::index>(ref<Expr>) +
                      lit<"]">) |
                  nest<ast::MemberExpr, &ast::MemberExpr::object>(
                      lit<"."> +
                      assign<&ast::MemberExpr::field>(ref<Identifier>)))> {
  };
  struct Unary : rule<create<ast::PrefixExpr>(
                          assign<&ast::PrefixExpr::op>(lit<"-"> | lit<"~">) +
                          assign<&ast::PrefixExpr::operand>(ref<Unary>)) |
                      ref<Postfix>> {};
  struct Arith
      : infix<Unary, ast::BinaryExpr,
              levels(left(lit<"*"> | lit<"\\"> | lit<"%">),
                     left(lit<"+"> | lit<"-">), left(lit<"<<"> | lit<">>">),
                     left(lit<"&">), left(lit<"^">), left(lit<"|">))> {};
  static constexpr auto SetItem =
      ref<STRING_ID> | ref<STRING_WILD> | ref<RulePattern>;
  template <auto Items, auto Them>
  static constexpr auto OfSet =
      (lit<"("> + some(append<Items>(SetItem), lit<",">) + lit<")">) |
      flag<Them>(kw<"them">);
  struct Predicate
      : rule<ref<Arith> +
             option(
                 nest<ast::OfExpr, &ast::OfExpr::quantifier>(
                     option(flag<&ast::OfExpr::percentage>(lit<"%">)) +
                     kw<"of"> + OfSet<&ast::OfExpr::set, &ast::OfExpr::them> +
                     option((kw<"in"> +
                             assign<&ast::OfExpr::range>(ref<Range>)) |
                            (kw<"at"> +
                             assign<&ast::OfExpr::offset>(ref<Arith>)))) |
                 nest<ast::InExpr, &ast::InExpr::subject>(
                     kw<"in"> + assign<&ast::InExpr::range>(ref<Range>)) |
                 nest<ast::AtExpr, &ast::AtExpr::subject>(
                     kw<"at"> + assign<&ast::AtExpr::offset>(ref<Arith>)))> {};
  struct Comparison
      : infix<Predicate, ast::BinaryExpr,
              levels(left(lit<"<="> | lit<">="> | lit<"<"> | lit<">">),
                     left(lit<"=="> | lit<"!="> | kw<"contains"> |
                          kw<"icontains"> | kw<"startswith"> |
                          kw<"istartswith"> | kw<"endswith"> | kw<"iendswith"> |
                          kw<"iequals"> | kw<"matches">))> {};
  struct NotExpr
      : rule<create<ast::PrefixExpr>(
                 assign<&ast::PrefixExpr::op>(kw<"not"> | kw<"defined">) +
                 assign<&ast::PrefixExpr::operand>(ref<NotExpr>)) |
             ref<Comparison>> {};
  struct Expr : infix<NotExpr, ast::BinaryExpr,
                      levels(left(kw<"and">), left(kw<"or">))> {};
  struct ForExpr
      : rule<kw<"for"> + assign<&ast::ForExpr::quantifier>(ref<Arith>) +
             ((kw<"of"> + OfSet<&ast::ForExpr::set, &ast::ForExpr::them>) |
              (some(append<&ast::ForExpr::vars>(ref<ID>), lit<",">) + kw<"in"> +
               assign<&ast::ForExpr::iterable>(
                   ref<Range> | ref<IterableSet> | ref<Postfix>))) +
             lit<":"> + lit<"("> + assign<&ast::ForExpr::body>(ref<Expr>) +
             lit<")">> {};

  struct Alt;
  struct Jump
      : rule<lit<"["> + option(assign<&ast::Jump::lo>(ref<INTEGER>)) +
             option(flag<&ast::Jump::ranged>(lit<"-">) +
                    option(assign<&ast::Jump::hi>(ref<INTEGER>))) +
             lit<"]">> {};
  static constexpr auto HexPart = ref<HEX_BYTE> | ref<Jump> | ref<Alt>;
  struct AltBranch : rule<create<ast::AltBranch> +
                              many(append<&ast::AltBranch::items>(HexPart))> {};
  struct Alt
      : rule<lit<"("> + some(append<&ast::Alt::branches>(ref<AltBranch>), lit<"|">) +
             lit<")">> {};
  struct HexPattern
      : rule<lit<"{"> + many(append<&ast::HexPattern::items>(HexPart)) +
             lit<"}">> {};
  struct StringModifier
      : rule<(assign<&ast::StringModifier::kind>(kw<"xor">) +
              option(
                  flag<&ast::StringModifier::hasArg>(lit<"(">) +
                  assign<&ast::StringModifier::lo>(ref<IntegerValue>) +
                  option(flag<&ast::StringModifier::ranged>(lit<"-">) +
                         assign<&ast::StringModifier::hi>(ref<IntegerValue>)) +
                  lit<")">)) |
             (assign<&ast::StringModifier::kind>(kw<"base64wide"> |
                                                 kw<"base64">) +
              option(flag<&ast::StringModifier::hasArg>(lit<"(">) +
                     assign<&ast::StringModifier::arg>(ref<TEXT>) + lit<")">)) |
             assign<&ast::StringModifier::kind>(kw<"nocase"> | kw<"wide"> |
                                                kw<"ascii"> | kw<"fullword"> |
                                                kw<"private">)> {};
  struct StringDef
      : rule<assign<&ast::StringDef::id>(ref<STRING_ID>) + lit<"="> +
             assign<&ast::StringDef::value>(ref<TEXT> | ref<HexPattern> |
                                            ref<Regex>) +
             many(
                 append<&ast::StringDef::modifiers>(ref<StringModifier>))> {
  };
  struct IntegerLiteral
      : rule<create<ast::IntegerLiteral> +
             assign<&ast::IntegerLiteral::text>(ref<IntegerValue>)> {};
  struct MetaDef : rule<assign<&ast::MetaEntry::name>(ref<ID>) + lit<"="> +
                            assign<&ast::MetaEntry::value>(
                                ref<TEXT> | ref<BOOLEAN> | ref<RealValue> |
                                ref<IntegerLiteral>)> {};
  struct RuleDecl
      : rule<many(flag<&ast::RuleDecl::isPrivate>(kw<"private">) |
                  flag<&ast::RuleDecl::isGlobal>(kw<"global">)) +
             kw<"rule"> + assign<&ast::RuleDecl::name>(ref<ID>) +
             option(lit<":"> + some(append<&ast::RuleDecl::tags>(ref<ID>))) +
             lit<"{"> +
             option(kw<"meta"> + lit<":"> +
                    many(append<&ast::RuleDecl::meta>(ref<MetaDef>))) +
             option(kw<"strings"> + lit<":"> +
                    many(append<&ast::RuleDecl::strings>(ref<StringDef>))) +
             kw<"condition"> + lit<":"> +
             assign<&ast::RuleDecl::condition>(ref<Expr>) + lit<"}">> {};
  struct ImportDecl
      : rule<kw<"import"> + assign<&ast::ImportDecl::module>(ref<TEXT>)> {};
  struct IncludeDecl : rule<kw<"include"> + assign<&ast::IncludeDecl::path>(
                                                    ref<INCLUDE_PATH>)> {};
  struct Document : rule<create<ast::Document> +
                             many(append<&ast::Document::declarations>(
                                 ref<ImportDecl> | ref<IncludeDecl> |
                                 ref<RuleDecl>))> {};
  using entry = Document;
  using ignored = WHITE_SPACE;
  using hidden = list<LINE_COMMENT, BLOCK_COMMENT>;
};
} // namespace yara
