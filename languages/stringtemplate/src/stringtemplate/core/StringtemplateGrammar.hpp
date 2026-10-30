// The stringtemplate language: its grammar. Covers the single-template
// format -- a document outside any `<...>` is literal text, and `<...>`
// opens an expression, an `if`, or a region.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "stringtemplate/core/ast.hpp"

namespace stringtemplate {

inline constexpr auto textValue = [](std::string_view text,
                                     std::pmr::memory_resource *mem) {
  if (text.find('\\') == text.npos)
    return text;
  auto room = cursiv::text_room(mem, text.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (text[i] == '\\' && i + 1 < text.size() &&
        (text[i + 1] == '\\' || text[i + 1] == '<' || text[i + 1] == '}'))
      ++i;
    room[n++] = text[i];
  }
  return std::string_view(room.data(), n);
};
inline constexpr auto stringValue = [](std::string_view text,
                                       std::pmr::memory_resource *mem) {
  text = text.substr(1, text.size() - 2);
  if (text.find('\\') == text.npos)
    return text;
  auto room = cursiv::text_room(mem, text.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    char c = text[i];
    if (c == '\\' && i + 1 < text.size()) {
      c = text[++i];
      if (c == 'n')
        c = '\n';
      else if (c == 'r')
        c = '\r';
      else if (c == 't')
        c = '\t';
    }
    room[n++] = c;
  }
  return std::string_view(room.data(), n);
};
inline constexpr auto escapeValue = [](std::string_view text,
                                       std::pmr::memory_resource *mem) {
  if (text == "<\\n>")
    return std::string_view("\n");
  if (text == "<\\t>")
    return std::string_view("\t");
  if (text == "<\\ >")
    return std::string_view(" ");
  if (text.size() != 8 || text[2] != 'u')
    return text;
  unsigned value = 0;
  for (std::size_t i = 3; i < 7; ++i) {
    const auto c = text[i];
    const unsigned digit = c >= '0' && c <= '9'   ? c - '0'
                           : c >= 'A' && c <= 'F' ? c - 'A' + 10
                           : c >= 'a' && c <= 'f' ? c - 'a' + 10
                                                  : 16;
    if (digit == 16)
      return text;
    value = value * 16 + digit;
  }
  auto room = cursiv::text_room(mem, 3);
  if (value < 128) {
    room[0] = static_cast<char>(value);
    return std::string_view(room.data(), 1);
  }
  if (value < 2048) {
    room[0] = static_cast<char>(0xc0 | (value >> 6));
    room[1] = static_cast<char>(0x80 | (value & 63));
    return std::string_view(room.data(), 2);
  }
  room[0] = static_cast<char>(0xe0 | (value >> 12));
  room[1] = static_cast<char>(0x80 | ((value >> 6) & 63));
  room[2] = static_cast<char>(0x80 | (value & 63));
  return std::string_view(room.data(), 3);
};

struct Stringtemplate : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct TMPL_COMMENT : terminal<through(lit<"<!">, lit<"!>">)> {};
  struct LINE_BREAK
      : terminal<lit<"<\\\\>"> + many(cr<R"( \t)">) + option(lit<"\r">) +
                 lit<"\n"> + many(cr<R"( \t)">)> {};

  template <bool Nested>
  static constexpr auto textChar = [] {
    if constexpr (Nested)
      return cr<R"(^<}\\)">;
    else
      return cr<R"(^<\\)">;
  }();
  template <bool Nested>
  struct TEXT : terminal<as<textValue>(
                    some((lit<"\\"> + option(any)) | textChar<Nested>))> {};
  struct PIPE : terminal<lit<"|"> + option(cr<R"( \t\r\n)">)> {};

  // Escapes outside an expression are exactly n, t, space and four hex digits.
  // The separate LINE_BREAK terminal handles <\\\\> followed by a newline.
  struct ESCAPE_TEXT : terminal<as<escapeValue>(
                           lit<"<\\"> +
                           (cr<R"(nt )"> | (lit<"u"> + repeat<4>(cr<"0-9a-fA-F">))) +
                           lit<">">)> {};

  static constexpr auto Reserved = kw<"if"> | kw<"elseif"> | kw<"else"> |
      kw<"endif"> | kw<"super"> | kw<"true"> | kw<"false">;
  static constexpr auto IdBody = some(cr<"a-zA-Z0-9_-">);
  struct ID : terminal<!Reserved + IdBody> {};
  // Native subtemplate parameters are scanned directly as IDs, even keywords.
  struct PARAM_ID : terminal<IdBody> {};

  struct STRING
      : terminal<as<stringValue>(lit<"\""> +
                                 many((lit<"\\"> + any) | cr<R"(^"\\)">) +
                                 lit<"\"">)> {};

  struct MapExpr;
  struct NoCommaExpr;
  struct MapStage;
  struct MemberExpr;
  struct IncludeExpr;
  struct Primary;
  struct CallExpr;
  struct SuperCallExpr;
  struct RegionRefExpr;
  struct DynamicCallExpr;
  struct ListExpr;
  struct SubTemplateExpr;
  struct NotCond;
  struct Conditional;
  struct ArgList;
  struct Arg;
  template <bool Nested> struct Element;
  struct ExprTag;
  template <bool Nested> struct IfBranch;
  template <bool Nested> struct IfStat;
  template <bool Nested> struct Region;
  struct Option;

  // One argument: positional, named (`x=expr`), or the varargs marker
  // `...`, all one repeated element -- a strict superset that also allows
  // mixing positional and named args in one call.
  static constexpr auto SP = option(ref<WS>);
  static constexpr auto parameters =
      option(ref<WS>) + ref<PARAM_ID> +
      many(option(ref<WS>) + lit<","> + option(ref<WS>) + ref<PARAM_ID>) +
      option(ref<WS>) + lit<"|">;
  struct PARAM_OPEN : terminal<lit<"{"> + &parameters> {};
  struct BARE_OPEN : terminal<lit<"{"> + !parameters> {};

  struct Arg
      : rule<((SP + assign<&ast::Arg::name>(ref<ID>)) +
              (SP + lit<"=">)+assign<&ast::Arg::value>(ref<NoCommaExpr>)) |
             assign<&ast::Arg::value>(ref<NoCommaExpr>) |
             flag<&ast::Arg::ellipsis>((SP + lit<"...">))> {};

  struct ArgList
      : rule<some(append<&ast::ArgList::items>(ref<Arg>), (SP + lit<",">))> {};

  struct ListElement : rule<ref<NoCommaExpr> | create<ast::NullExpr>> {};

  struct ListExpr : rule<create<ast::ListExpr>(
                        SP + lit<"["> +
                        ((SP + lit<"]">) |
                         (some(append<&ast::ListExpr::items>(ref<ListElement>),
                               SP + lit<",">) +
                          SP + lit<"]">)))> {};

  struct SubTemplateExpr
      : rule<((SP + ref<PARAM_OPEN> +
                 some((SP +
                       append<&ast::SubTemplateExpr::params>(ref<PARAM_ID>)),
                      (SP + lit<",">)) +
                 (SP + ref<PIPE>)) | (SP + ref<BARE_OPEN>)) +
             many(append<&ast::SubTemplateExpr::body>(ref<Element<true>>)) +
             lit<"}">> {};

  struct DynamicCallExpr
      : rule<(SP +
              lit<"(">)+assign<&ast::DynamicCallExpr::target>(ref<MapExpr>) +
             (SP + lit<")">)+(SP + lit<"(">)+option(
                 assign<&ast::DynamicCallExpr::args>(ref<ArgList>)) +
             (SP + lit<")">)> {};

  struct Primary : rule<ref<DynamicCallExpr> |
                        ((SP + lit<"(">)+create<ast::ParenExpr>(
                             assign<&ast::ParenExpr::inner>(ref<Conditional> |
                                                            ref<MapExpr>)) +
                         (SP + lit<")">)) |
                        (SP + assign<&ast::NameExpr::name>(ref<ID>)) |
                        (SP + assign<&ast::StringExpr::value>(ref<STRING>)) |
                        (create<ast::BoolExpr> +
                         (flag<&ast::BoolExpr::value>((SP + kw<"true">)) |
                          (SP + kw<"false">))) |
                        ref<SubTemplateExpr> | ref<ListExpr>> {};

  struct QualifiedName : rule<option(SP + lit<"/">) + SP + ref<ID> +
                              many(SP + lit<"/"> + SP + ref<ID>)> {};

  struct CallExpr : rule<assign<&ast::CallExpr::name>(ref<QualifiedName>) +
                         (SP + lit<"(">)+option(
                             assign<&ast::CallExpr::args>(ref<ArgList>)) +
                         (SP + lit<")">)> {};

  struct SuperCallExpr
      : rule<(SP + kw<"super">)+(SP + lit<".">)+(
                 SP + assign<&ast::SuperCallExpr::name>(ref<ID>)) +
             (SP + lit<"(">)+option(
                 assign<&ast::SuperCallExpr::args>(ref<ArgList>)) +
             (SP + lit<")">)> {};

  // Covers both `@name()` and `@super.name()`, merged on the `isSuper`
  // flag.
  struct RegionRefExpr
      : rule<(SP + lit<"@">)+option(flag<&ast::RegionRefExpr::isSuper>(
                 (SP + kw<"super">)+(SP + lit<".">))) +
             (SP + assign<&ast::RegionRefExpr::name>(ref<ID>)) +
             (SP + lit<"(">)+(SP + lit<")">)> {};

  struct IncludeExpr : rule<ref<CallExpr> | ref<SuperCallExpr> |
                            ref<RegionRefExpr> | ref<Primary>> {};

  struct MemberExpr
      : rule<ref<IncludeExpr> +
             many(nest<ast::FieldAccess, &ast::FieldAccess::object>(
                      (SP + lit<".">)+(
                          SP + assign<&ast::FieldAccess::field>(ref<ID>))) |
                  nest<ast::IndexAccess, &ast::IndexAccess::object>(
                      (SP + lit<".">)+(SP + lit<"(">)+assign<
                          &ast::IndexAccess::key>(ref<MapExpr>) +
                      (SP + lit<")">)))> {};

  struct NotCond
      : rule<create<ast::NotExpr>(
                 (SP + lit<"!">)+assign<&ast::NotExpr::operand>(ref<NotCond>)) |
             ref<MemberExpr>> {};

  struct Conditional : infix<NotCond, ast::BoolBinExpr,
                             levels(left(as<ast::BoolOp::logical_and>(option(ref<WS>) + (lit<"&&">))), left(as<ast::BoolOp::logical_or>(option(ref<WS>) + (lit<"||">))))> {};

  // A colon stage's targets: over-accepts `MemberExpr` in general (a call,
  // a subtemplate or a dynamic call are already reachable through it)
  // rather than restating the narrower set.
  struct MapStage : rule<some(append<&ast::MapStage::targets>(ref<MemberExpr>),
                              (SP + lit<",">))> {};

  struct MapExpr : rule<(some(append<&ast::MapExpr::sources>(ref<MemberExpr>),
                              (SP + lit<",">)) +
                         some((SP + lit<":">)+append<&ast::MapExpr::stages>(
                             ref<MapStage>))) |
                        append<&ast::MapExpr::sources>(ref<MemberExpr>)> {};

  struct SingleMapStage
      : rule<append<&ast::MapStage::targets>(ref<MemberExpr>)> {};

  struct NoCommaExpr
      : rule<append<&ast::MapExpr::sources>(ref<MemberExpr>) +
             many(SP + lit<":"> +
                  append<&ast::MapExpr::stages>(ref<SingleMapStage>))> {};

  struct Option : rule<(SP + assign<&ast::Option::name>(ref<ID>)) +
                           option((SP + lit<"=">)+assign<&ast::Option::value>(
                               ref<NoCommaExpr>))> {};

  struct ExprTag : rule<create<ast::ExprTag>(
                       lit<"<"> + assign<&ast::ExprTag::value>(ref<MapExpr>) +
                       option((SP + lit<";">)+some(
                           append<&ast::ExprTag::options>(ref<Option>),
                           (SP + lit<",">))) +
                       (SP + lit<">">))> {};

  template <bool Nested>
  struct IfBranch
      : rule<(SP + lit<"(">)+assign<&ast::IfBranch::cond>(ref<Conditional>) +
             (SP + lit<")">)+(SP + lit<">">)+many(
                 append<&ast::IfBranch::body>(ref<Element<Nested>>))> {};

  template <bool Nested>
  struct IfStat
      : rule<create<ast::IfStat>(
            lit<"<"> +
            (SP + kw<"if">)+append<&ast::IfStat::branches>(
                ref<IfBranch<Nested>>) +
            many(lit<"<"> + (SP + kw<"elseif">)+append<&ast::IfStat::branches>(
                                ref<IfBranch<Nested>>)) +
            option(lit<"<"> + flag<&ast::IfStat::hasElse>((SP + kw<"else">)) +
                   (SP + lit<">">)+many(append<&ast::IfStat::elseBody>(
                       ref<Element<Nested>>))) +
            lit<"<"> + (SP + kw<"endif">)+(SP + lit<">">))> {};

  template <bool Nested>
  struct Region
      : rule<create<ast::Region>(
            lit<"<"> +
            (SP + lit<"@">)+(SP + assign<&ast::Region::name>(ref<ID>)) +
            (SP + lit<">">)+many(
                append<&ast::Region::elements>(ref<Element<Nested>>)) +
            lit<"<"> + (SP + lit<"@end">)+(SP + lit<">">))> {};

  template <bool Nested>
  struct Element
      : rule<assign<&ast::Text::value>(ref<TEXT<Nested>>) |
             assign<&ast::Text::value>(ref<ESCAPE_TEXT>) | ref<ExprTag> |
             ref<IfStat<Nested>> | ref<Region<Nested>>> {};

  struct Template
      : rule<create<ast::Template>(
            many(append<&ast::Template::elements>(ref<Element<false>>)))> {
  };

  static constexpr auto word_boundary = cr<"a-zA-Z0-9_-">;
  using entry = Template;
  using ignored = list<>;
  using hidden = list<TMPL_COMMENT, LINE_BREAK>;
};

} // namespace stringtemplate
