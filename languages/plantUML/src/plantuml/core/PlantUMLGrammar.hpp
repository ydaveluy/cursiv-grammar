// The plantuml language: its grammar, a PlantUML class-diagram document.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "plantuml/core/ast.hpp"

namespace plantuml {

inline constexpr auto strip_stereotype = [](std::string_view t) {
  return t.substr(2, t.size() - 4);
};
using StereotypeText = cursiv::text_conv<strip_stereotype>;

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unquoted = cursiv::text_conv<unquote>;

inline constexpr auto drop_colon = [](std::string_view t) { return t.substr(1); };
using AfterColonText = cursiv::text_conv<drop_colon>;

struct PlantUML : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t)">)> {};
  struct NEWLINE : terminal<option(lit<"\r">) + lit<"\n">> {};

  struct SL_COMMENT : terminal<(lit<"//"> | lit<"'">) + many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  // Whole-line pragmas, read to end of line and hidden: `note ...` and
  // `<> ...` are never a class name followed by more text.
  struct NOTE : terminal<kw<"note"> + many(cr<R"(^\r\n)">)> {};
  struct DIAMOND : terminal<lit<"<>"> + many(cr<R"(^\r\n)">)> {};

  struct IDENT : terminal<cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9_">)> {};

  struct STEREOTYPE
      : terminal<lit<"<<"> + many(cr<R"(^\r\n>)">) + lit<">>">, StereotypeText> {};
  struct ASSOC_DETAIL
      : terminal<lit<"\""> + many(cr<R"(^\r\n")">) + lit<"\"">, Unquoted> {};
  struct AFTER_COLON
      : terminal<lit<":"> + many(cr<R"(^\r\n)">), AfterColonText> {};

  // The relation's body symbols. `o` competes with IDENT, and as a plain
  // `kw<>` loses the tie only where IDENT could not read further, so `o`
  // alone is never a usable identifier either.
  struct LT_ : terminal<lit<"<">> {};
  struct GT_ : terminal<lit<">">> {};
  struct DASH_ : terminal<lit<"-">> {};
  struct DOUBLE_DOT_ : terminal<lit<"..">> {};
  struct STAR_ : terminal<lit<"*">> {};
  struct O_ : terminal<kw<"o">> {};
  struct CARET_ : terminal<lit<"^">> {};
  struct PIPE_GT_ : terminal<lit<"|>">> {};
  struct LT_PIPE_ : terminal<lit<"<|">> {};

  // A data rule: no directive fires, so its value is its own matched text --
  // the arrow's symbols joined, e.g. `<|..`, `-->`, `o--`.
  struct Relation
      : rule<option(ref<LT_>) +
             many(ref<DOUBLE_DOT_> | ref<DASH_> | ref<PIPE_GT_> |
                  ref<LT_PIPE_> | ref<O_> | ref<CARET_> | ref<STAR_>) +
             option(ref<GT_>)> {};

  // A class or enum body is unparsed: a run of raw text per line, and
  // balanced `{...}` regions nested inside. Inside the braces no trivia is
  // dropped -- leading space on a line is content.
  static constexpr auto body_run = some(cr<R"(^{}\r\n)">);
  struct BODY_LINE : terminal<body_run> {};
  struct BODY_INLINE : terminal<lit<"{"> + body_run + lit<"}">> {};
  struct BODY_NL : terminal<cr<R"(\r\n)">> {};

  struct NestedBodyRule;
  static constexpr auto body_item =
      append<&ast::BodyBlock::items>(ref<BODY_INLINE> | ref<BODY_LINE> |
                                     ref<NestedBodyRule>) |
      ref<BODY_NL>;

  struct NestedBodyRule : rule<lit<"{"> + many(body_item) + lit<"}">> {};

  struct ClassBodyRule
      : rule<lit<"{"> + tight(many(body_item) + lit<"}">)> {
  };

  static constexpr auto class_type =
      (option(flag<&ast::ClassDecl::is_abstract>(kw<"abstract">)) +
       kw<"class">) |
      flag<&ast::ClassDecl::is_interface>(kw<"interface">);

  struct ClassDeclRule
      : rule<class_type + assign<&ast::ClassDecl::name>(ref<IDENT>) +
             option(assign<&ast::ClassDecl::stereotype>(ref<STEREOTYPE>)) +
             option(kw<"extends"> +
                    assign<&ast::ClassDecl::extends>(ref<IDENT>)) +
             option(assign<&ast::ClassDecl::body>(ref<ClassBodyRule>))> {};

  struct EnumDeclRule
      : rule<kw<"enum"> + assign<&ast::EnumDecl::name>(ref<IDENT>) +
             option(assign<&ast::EnumDecl::stereotype>(ref<STEREOTYPE>)) +
             option(assign<&ast::EnumDecl::body>(ref<ClassBodyRule>))> {};

  struct AssociationRule
      : rule<assign<&ast::Association::left>(ref<IDENT>) +
             option(assign<&ast::Association::left_detail>(
                 ref<ASSOC_DETAIL>)) +
             ((assign<&ast::Association::op>(ref<Relation>) +
               option(assign<&ast::Association::right_detail>(
                   ref<ASSOC_DETAIL>)) +
               assign<&ast::Association::right>(ref<IDENT>) +
               option(assign<&ast::Association::name>(ref<AFTER_COLON>)))  //
              | assign<&ast::Association::name>(ref<AFTER_COLON>))> {};

  struct AssociativeClassRule
      : rule<(lit<"("> + assign<&ast::AssociativeClass::left>(ref<IDENT>) +
              lit<","> + assign<&ast::AssociativeClass::right>(ref<IDENT>) +
              lit<")"> + (lit<".."> | lit<".">) +
              assign<&ast::AssociativeClass::target>(ref<IDENT>) +
              option(assign<&ast::AssociativeClass::name>(
                  ref<AFTER_COLON>)))  //
             | (assign<&ast::AssociativeClass::target>(ref<IDENT>) +
                (lit<".."> | lit<".">) + lit<"("> +
                assign<&ast::AssociativeClass::left>(ref<IDENT>) + lit<","> +
                assign<&ast::AssociativeClass::right>(ref<IDENT>) +
                lit<")"> +
                option(assign<&ast::AssociativeClass::name>(
                    ref<AFTER_COLON>)))> {};

  struct DiagramElement
      : rule<ref<ClassDeclRule> | ref<EnumDeclRule> | ref<AssociationRule> |
             ref<AssociativeClassRule>> {};

  struct UmlRule
      : rule<kw<"@startuml"> +
             option(assign<&ast::Document::name>(ref<IDENT>)) +
             many(append<&ast::Document::elements>(ref<DiagramElement>)) +
             kw<"@enduml">> {};

  using entry = UmlRule;
  // A newline is never required by any construct -- always inside a `*` or
  // behind a `?` -- so it carries no more meaning than blank trivia, and
  // folds into `ignore` with WS.
  using ignore = list<WS, NEWLINE>;
  using hidden = list<SL_COMMENT, ML_COMMENT, NOTE, DIAMOND>;
};

}  // namespace plantuml
