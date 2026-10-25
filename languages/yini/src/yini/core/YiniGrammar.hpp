#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "yini/core/ast.hpp"

namespace yini {

struct Yini : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t)">)> {};
  struct NL : terminal<lit<"\r\n"> | cr<R"(\r\n)">> {};
  struct BLOCK_COMMENT : terminal<lit<"/*"> + many(!lit<"*/"> + any) + lit<"*/">> {};
  struct SHEBANG : terminal<lit<"#!"> + many(cr<R"(^\r\n)">)> {};
  struct INLINE_COMMENT : terminal<(lit<"//"> | lit<"#">) + many(cr<R"(^\r\n)">)> {};
  struct LINE_COMMENT : terminal<(lit<";"> | lit<"--">) + many(cr<R"(^\r\n)">)> {};
  struct KEY : terminal<some(cr<"a-zA-Z0-9_.">) |
      lit<"`"> + many(lit<"\\"> + any | cr<R"(^`\\\r\n)">) + lit<"`">> {};
  static constexpr auto word_boundary = cr<"a-zA-Z0-9_">;
  struct TAG : terminal<lit<"@"> + some(cr<"a-zA-Z_">)> {};
  struct END : terminal<ikw<"/END">> {};
  struct MARKER : terminal<some(cr<"§<>€_0-9^">)> {};

  static constexpr auto digits = d + many(option(lit<"_">) + d);
  static constexpr auto fraction = lit<"."> + digits;
  static constexpr auto exponent = cr<"eE"> + option(cr<"+-">) + digits;
  static constexpr auto based =
      (lit<"0x"> | lit<"0X"> | (cr<"hH"> + cr<"eE"> + cr<"xX"> + lit<":">)) + some(cr<"0-9a-fA-F_">) |
      (lit<"0b"> | lit<"0B"> | lit<"%">) + some(cr<"01_">) |
      (lit<"0o"> | lit<"0O">) + some(cr<"0-7_">) |
      (lit<"0z"> | lit<"0Z">) + some(cr<"0-9aAbBxXeE_">);
  struct NUMBER : terminal<option(cr<"+-">) + (based |
      (digits + option(fraction) | fraction) + option(exponent))> {};
  struct BOOLEAN : terminal<ikw<"true"> | ikw<"on"> | ikw<"yes"> |
      ikw<"false"> | ikw<"off"> | ikw<"no">,
      text_conv<[](std::string_view text) { return text.front() == 't' || text.front() == 'T' ||
          text.front() == 'y' || text.front() == 'Y' || text.size() == 2 &&
          (text.back() == 'n' || text.back() == 'N'); }>> {};
  struct NULL_LIT : terminal<ikw<"null">, value<std::monostate{}>> {};

  static constexpr auto escaped = lit<"\\"> + any;
  static constexpr auto triple = lit<"\"\"\"">;
  static constexpr auto raw =
      triple + many(!triple + any) + triple |
      lit<"'"> + many(cr<R"(^'\r\n)">) + lit<"'"> |
      lit<"\""> + many(cr<R"(^"\r\n)">) + lit<"\"">;
  static constexpr auto classic =
      triple + many(escaped | !triple + cr<R"(^\\)">) + triple |
      lit<"'"> + many(escaped | cr<R"(^'\\\r\n)">) + lit<"'"> |
      lit<"\""> + many(escaped | cr<R"(^"\\\r\n)">) + lit<"\"">;
  struct STRING : terminal<option(cr<"Rr">) + raw | cr<"Cc"> + classic |
      cr<"Hh"> + (lit<"'"> + many(cr<"^'">) + lit<"'"> |
                    lit<"\""> + many(cr<R"(^")">) + lit<"\"">)> {};

  static constexpr auto gap = many(ref<NL> | ref<INLINE_COMMENT>);
  static constexpr auto line_end = option(ref<INLINE_COMMENT>) + (some(ref<NL>) | !any);
  struct String : rule<assign<&ast::String::literal>(ref<STRING>)> {};
  struct Number : rule<assign<&ast::Number::literal>(ref<NUMBER>)> {};
  static constexpr auto scalar = ref<String> | ref<Number> | ref<BOOLEAN> | ref<NULL_LIT>;
  struct Concat : rule<assign<&ast::Concat::first>(ref<String>) +
      some(lit<"+"> + gap + append<&ast::Concat::rest>(scalar))> {};
  struct List;
  struct Object;
  static constexpr auto value_expr = ref<Concat> | scalar | ref<List> | ref<Object>;
  static constexpr auto elements = some(append<&ast::List::items>(value_expr) + gap,
      lit<","> + gap) + option(flag<&ast::List::trailingComma>(lit<","> + gap));
  struct List : rule<lit<"["> + gap + option(elements) + lit<"]">> {};
  struct ColonList : rule<elements> {};
  struct ObjectMember : rule<assign<&ast::ObjectMember::key>(ref<KEY>) +
      assign<&ast::ObjectMember::separator>(lit<":"> | lit<"=">) + gap +
      assign<&ast::ObjectMember::value>(value_expr)> {};
  struct Object : rule<lit<"{"> + gap +
      option(some(append<&ast::Object::members>(ref<ObjectMember>) + gap, lit<","> + gap) +
             option(flag<&ast::Object::trailingComma>(lit<","> + gap))) + lit<"}">> {};

  struct Member : rule<assign<&ast::Member::key>(ref<KEY>) +
      (lit<"="> + option(flag<&ast::Member::hasValue>(
                        assign<&ast::Member::value>(value_expr))) |
       flag<&ast::Member::colon>(lit<":">) +
         option(flag<&ast::Member::hasValue>(assign<&ast::Member::value>(ref<ColonList>)))) + line_end> {};
  struct Section : rule<assign<&ast::Section::markerText>(ref<MARKER>) +
      assign<&ast::Section::name>(ref<KEY>) + line_end> {};
  struct Directive : rule<assign<&ast::Directive::tag>(ref<TAG>) +
      many(append<&ast::Directive::arguments>(ref<STRING> | ref<KEY> | ref<NUMBER>)) + line_end> {};
  struct Comment : rule<assign<&ast::Comment::literal>(ref<LINE_COMMENT>) + option(ref<NL>)> {};
  struct Shebang : rule<assign<&ast::Shebang::literal>(ref<SHEBANG>) + line_end> {};
  struct End : rule<assign<&ast::End::literal>(ref<END>) + line_end> {};
  struct Document : rule<create<ast::Document> + option(flag<&ast::Document::bom>(lit<"\xef\xbb\xbf">)) + many(
      append<&ast::Document::entries>(ref<Shebang> | ref<Section> | ref<Member> |
                                     ref<Directive> | ref<Comment> | ref<End>) |
      ref<NL> | ref<INLINE_COMMENT>)> {};

  using entry = Document;
  using ignore = list<WS, BLOCK_COMMENT>;
};

}  // namespace yini
