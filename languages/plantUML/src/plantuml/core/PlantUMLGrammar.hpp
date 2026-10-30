// The plantuml language: its grammar, a PlantUML class-diagram document.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "plantuml/core/ast.hpp"

namespace plantuml {

inline constexpr auto strip_stereotype = [](std::string_view t) {
  return t.substr(2, t.size() - 4);
};

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};

inline constexpr auto drop_colon = [](std::string_view t) { return t.substr(1); };

struct PlantUML : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t)">)> {};
  struct NEWLINE : terminal<option(lit<"\r">) + lit<"\n">> {};

  struct SL_COMMENT : terminal<lit<"'"> + many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/'">, lit<"'/">)> {};

  static constexpr auto id_start = uc::L | cr<"_">;
  static constexpr auto id_part = uc::L | uc::Nd | cr<"_">;
  struct IDENT : terminal<id_start + many(id_part) + many(lit<"."> + id_start + many(id_part))> {};

  struct STEREOTYPE : terminal<as<strip_stereotype>(
                          lit<"<<"> + many(cr<R"(^\r\n>)">) + lit<">>">)> {};
  struct ASSOC_DETAIL
      : terminal<as<unquote>(lit<"\""> + many(cr<R"(^\r\n")">) + lit<"\"">)> {};
  struct AFTER_COLON
      : terminal<as<drop_colon>(lit<":"> + many(cr<R"(^\r\n)">))> {};

  // A relation is one lexical arrow, with at least one line character.
  // Heads alone, or an empty connector, are not native class relations.
  static constexpr auto direction = lit<"left"> | lit<"right"> | lit<"up"> | lit<"down"> |
      lit<"l"> | lit<"r"> | lit<"u"> | lit<"d">;
  static constexpr auto line_style = lit<"["> + some(cr<R"(^]\r\n)">) + lit<"]">;
  struct Relation : terminal<
      option(lit<"<|"> | lit<"<<"> | lit<"<"> | lit<">"> | lit<"o"> | lit<"*"> | lit<"^"> | lit<"+">) +
      some(cr<".-">) + option(line_style) + option(direction) + many(cr<".-">) +
      option(lit<"|>"> | lit<">>"> | lit<">"> | lit<"<"> | lit<"o"> | lit<"*"> | lit<"^"> | lit<"+">)> {};
  struct AssociativeLink : terminal<lit<".."> | lit<".">> {};

  // A class or enum body is unparsed: a run of raw text per line, and
  // balanced `{...}` regions nested inside. Inside the braces no trivia is
  // dropped -- leading space on a line is content.
  static constexpr auto body_run = some(cr<R"(^{}\r\n)">);
  struct BODY_LINE : terminal<body_run> {};
  struct BODY_INLINE : terminal<lit<"{"> + body_run + lit<"}">> {};
  struct BODY_NL : terminal<cr<R"(\r\n)">> {};

  struct NestedBody;
  static constexpr auto body_item =
      append<&ast::BodyBlock::items>(ref<BODY_INLINE> | ref<BODY_LINE> |
                                     ref<NestedBody> | ref<BODY_NL>);

  struct NestedBody : rule<lit<"{"> + many(body_item) + lit<"}">> {};

  struct ClassBody
      : rule<lit<"{"> + tight(many(body_item) + lit<"}">)> {
  };

  static constexpr auto class_type =
      (flag<&ast::ClassDecl::is_abstract>(kw<"abstract">) + option(kw<"class">)) |
      flag<&ast::ClassDecl::is_interface>(kw<"interface">) |
      assign<&ast::ClassDecl::kind>(kw<"class"> | kw<"annotation"> | kw<"entity"> |
          kw<"exception"> | kw<"record"> | kw<"dataclass"> | kw<"metaclass"> |
          kw<"stereotype"> | kw<"struct"> | kw<"protocol"> | kw<"circle"> | kw<"diamond"> |
          lit<"<>"> | lit<"()">);
  static constexpr auto class_name =
      (assign<&ast::ClassDecl::displayName>(ref<ASSOC_DETAIL>) + kw<"as"> + assign<&ast::ClassDecl::name>(ref<IDENT>)) |
      (assign<&ast::ClassDecl::name>(ref<IDENT>) + option(kw<"as"> + assign<&ast::ClassDecl::displayName>(ref<ASSOC_DETAIL>))) |
      assign<&ast::ClassDecl::name>(ref<ASSOC_DETAIL>);
  struct ClassDecl : rule<create<ast::ClassDecl>(class_type + class_name +
      option(assign<&ast::ClassDecl::stereotype>(ref<STEREOTYPE>)) +
      option(kw<"extends"> + some(append<&ast::ClassDecl::extends>(ref<IDENT> | ref<ASSOC_DETAIL>),lit<",">)) +
      option(kw<"implements"> + some(append<&ast::ClassDecl::implements>(ref<IDENT> | ref<ASSOC_DETAIL>),lit<",">)) +
      option(assign<&ast::ClassDecl::body>(ref<ClassBody>)))> {};
  static constexpr auto enum_name =
      (assign<&ast::EnumDecl::displayName>(ref<ASSOC_DETAIL>) + kw<"as"> + assign<&ast::EnumDecl::name>(ref<IDENT>)) |
      (assign<&ast::EnumDecl::name>(ref<IDENT>) + option(kw<"as"> + assign<&ast::EnumDecl::displayName>(ref<ASSOC_DETAIL>))) |
      assign<&ast::EnumDecl::name>(ref<ASSOC_DETAIL>);
  struct EnumDecl : rule<create<ast::EnumDecl>(kw<"enum"> + enum_name +
      option(assign<&ast::EnumDecl::stereotype>(ref<STEREOTYPE>)) +
      option(assign<&ast::EnumDecl::body>(ref<ClassBody>)))> {};

  // Note headers remain structured; note text is content, not declarations.
  static constexpr auto horizontal = cr<R"( \t)">;
  struct NoteColor : terminal<lit<"#"> + some(uc::L | uc::Nd | cr<"_-">)> {};
  static constexpr auto note_end = many(horizontal) + lit<"end"> + some(horizontal) + lit<"note">;
  struct NoteBlock : terminal<eol + many(!(note_end + (eol | eof)) + many(cr<R"(^\r\n)">) + eol) + note_end> {};
  struct NoteDecl : rule<create<ast::NoteDecl>(kw<"note"> +
      tight(option(some(horizontal) + assign<&ast::NoteDecl::position>(kw<"left"> | kw<"right"> | kw<"top"> | kw<"bottom">)) +
        option(some(horizontal) + kw<"of"> + some(horizontal) + assign<&ast::NoteDecl::target>(ref<IDENT> | ref<ASSOC_DETAIL>)) +
        option(some(horizontal) + kw<"as"> + some(horizontal) + assign<&ast::NoteDecl::name>(ref<IDENT>)) +
        option(some(horizontal) + flag<&ast::NoteDecl::onLink>(kw<"on"> + some(horizontal) + kw<"link">)) +
        option(some(horizontal) + assign<&ast::NoteDecl::color>(ref<NoteColor>)) +
        many(horizontal) + assign<&ast::NoteDecl::text>(ref<AFTER_COLON> | ref<NoteBlock>)))> {};

  struct Association
      : rule<assign<&ast::Association::left>(ref<IDENT> | ref<ASSOC_DETAIL>) +
             option(assign<&ast::Association::left_detail>(
                 ref<ASSOC_DETAIL>)) +
             ((assign<&ast::Association::op>(ref<Relation>) +
               option(assign<&ast::Association::right_detail>(
                   ref<ASSOC_DETAIL>)) +
               assign<&ast::Association::right>(ref<IDENT> | ref<ASSOC_DETAIL>) +
               option(assign<&ast::Association::name>(ref<AFTER_COLON>)))  //
              | assign<&ast::Association::name>(ref<AFTER_COLON>))> {};

  struct AssociativeClass
      : rule<(lit<"("> + assign<&ast::AssociativeClass::left>(ref<IDENT>) +
              lit<","> + assign<&ast::AssociativeClass::right>(ref<IDENT>) +
              lit<")"> + assign<&ast::AssociativeClass::op>(ref<AssociativeLink>) +
              assign<&ast::AssociativeClass::target>(ref<IDENT>) +
              option(assign<&ast::AssociativeClass::name>(
                  ref<AFTER_COLON>)))  //
             | (assign<&ast::AssociativeClass::target>(ref<IDENT>) +
                assign<&ast::AssociativeClass::op>(ref<AssociativeLink>) + flag<&ast::AssociativeClass::targetFirst>(lit<"(">) +
                assign<&ast::AssociativeClass::left>(ref<IDENT>) + lit<","> +
                assign<&ast::AssociativeClass::right>(ref<IDENT>) +
                lit<")"> +
                option(assign<&ast::AssociativeClass::name>(
                    ref<AFTER_COLON>)))> {};

  struct DiagramElement
      : rule<ref<ClassDecl> | ref<EnumDecl> | ref<Association> |
             ref<AssociativeClass> | ref<NoteDecl>> {};

  struct Uml
      : rule<kw<"@startuml"> +
             option(!gap::line + assign<&ast::Document::name>(ref<IDENT>)) +
             many(append<&ast::Document::elements>(ref<DiagramElement>)) +
             kw<"@enduml">> {};

  using entry = Uml;
  // Notes and class bodies read their own text under tight(); ordinary
  // declarations use newline trivia.
  using ignored = list<WS, NEWLINE>;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace plantuml
