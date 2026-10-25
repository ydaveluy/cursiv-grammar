// The HyperTalk language: its grammar.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "hypertalk/core/ast.hpp"

namespace hypertalk {

inline constexpr bool ci_eq(std::string_view a, std::string_view b) {
  if (a.size() != b.size()) return false;
  for (std::size_t i = 0; i < a.size(); ++i) {
    char ca = a[i];
    char cb = b[i];
    if (ca >= 'A' && ca <= 'Z') ca = static_cast<char>(ca - 'A' + 'a');
    if (cb >= 'A' && cb <= 'Z') cb = static_cast<char>(cb - 'A' + 'a');
    if (ca != cb) return false;
  }
  return true;
}

// Each of these rules matches exactly one word, so its matched text is that
// word alone -- safe ground for a case-insensitive lookup table.
inline constexpr auto ordinal_of = [](std::string_view t) -> ast::Ordinal {
  using O = ast::Ordinal;
  if (ci_eq(t, "first")) return O::First;
  if (ci_eq(t, "second")) return O::Second;
  if (ci_eq(t, "third")) return O::Third;
  if (ci_eq(t, "fourth")) return O::Fourth;
  if (ci_eq(t, "fifth")) return O::Fifth;
  if (ci_eq(t, "sixth")) return O::Sixth;
  if (ci_eq(t, "seventh")) return O::Seventh;
  if (ci_eq(t, "eighth")) return O::Eighth;
  if (ci_eq(t, "ninth")) return O::Ninth;
  if (ci_eq(t, "tenth")) return O::Tenth;
  if (ci_eq(t, "last")) return O::Last;
  if (ci_eq(t, "any")) return O::Any;
  return O::Mid;  // "mid" or "middle"
};
using OrdinalConv = cursiv::text_conv<ordinal_of>;

inline constexpr auto length_of = [](std::string_view t) -> ast::Length {
  using L = ast::Length;
  if (ci_eq(t, "short")) return L::Short;
  if (ci_eq(t, "english") || ci_eq(t, "long")) return L::English;
  return L::Abbreviated;  // "abbreviated" / "abbrev" / "abbr"
};
using LengthConv = cursiv::text_conv<length_of>;

inline constexpr auto chunk_kind_of = [](std::string_view t) -> ast::ChunkKind {
  using K = ast::ChunkKind;
  const char c0 = t.empty() ? '\0' : t.front();
  if (c0 == 'w' || c0 == 'W') return K::Word;
  if (c0 == 'i' || c0 == 'I') return K::Item;
  if (c0 == 'l' || c0 == 'L') return K::Line;
  return K::Character;
};
using ChunkKindConv = cursiv::text_conv<chunk_kind_of>;

// The words `constant`, `modifierKey`, `mouseState`, `knownType` and
// `findType` denote -- distinguished by full comparison since a few share a
// leading letter (`comma`/`colon`/`commandkey`/`cmdkey`).
inline constexpr auto named_lit_of = [](std::string_view t) -> ast::NamedLitKind {
  using K = ast::NamedLitKind;
  if (ci_eq(t, "empty")) return K::Empty;
  if (ci_eq(t, "pi")) return K::Pi;
  if (ci_eq(t, "quote")) return K::Quote;
  if (ci_eq(t, "return")) return K::ReturnChar;
  if (ci_eq(t, "space")) return K::Space;
  if (ci_eq(t, "tab")) return K::Tab;
  if (ci_eq(t, "formfeed")) return K::Formfeed;
  if (ci_eq(t, "linefeed")) return K::Linefeed;
  if (ci_eq(t, "comma")) return K::Comma;
  if (ci_eq(t, "colon")) return K::Colon;
  if (ci_eq(t, "zero")) return K::Zero;
  if (ci_eq(t, "one")) return K::One;
  if (ci_eq(t, "two")) return K::Two;
  if (ci_eq(t, "three")) return K::Three;
  if (ci_eq(t, "four")) return K::Four;
  if (ci_eq(t, "five")) return K::Five;
  if (ci_eq(t, "six")) return K::Six;
  if (ci_eq(t, "seven")) return K::Seven;
  if (ci_eq(t, "eight")) return K::Eight;
  if (ci_eq(t, "nine")) return K::Nine;
  if (ci_eq(t, "ten")) return K::Ten;
  if (ci_eq(t, "commandkey") || ci_eq(t, "cmdkey")) return K::CommandKey;
  if (ci_eq(t, "optionkey")) return K::OptionKey;
  if (ci_eq(t, "shiftkey")) return K::ShiftKey;
  if (ci_eq(t, "up")) return K::MouseUp;
  if (ci_eq(t, "down")) return K::MouseDown;
  if (ci_eq(t, "number")) return K::NumberType;
  if (ci_eq(t, "integer")) return K::IntegerType;
  if (ci_eq(t, "point")) return K::PointType;
  if (ci_eq(t, "rect") || ci_eq(t, "rectangle")) return K::RectType;
  if (ci_eq(t, "date")) return K::DateType;
  if (ci_eq(t, "word")) return K::FindWord;
  if (ci_eq(t, "chars")) return K::FindChars;
  if (ci_eq(t, "whole")) return K::FindWhole;
  if (ci_eq(t, "string")) return K::FindString;
  return K::LogicalType;  // "logical" / "boolean" / "bool"
};
using NamedLitConv = cursiv::text_conv<named_lit_of>;

inline constexpr auto unquote_ht = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using UnquotedHT = cursiv::text_conv<unquote_ht>;

struct Hypertalk : cursiv::GrammarSpec {
  static constexpr auto WsBody = some(cr<R"( \t)">);
  static constexpr auto CommentBody = lit<"--"> + many(cr<R"(^\r\n|)">);
  struct WS : terminal<WsBody> {};
  struct COMMENT : terminal<CommentBody> {};
  // `('|' | '¬') ws? comment? ws? newline`, entirely elided: a
  // HyperTalk line continuation.
  struct BREAK : terminal<(lit<"|"> | lit<"¬">) + option(WsBody) +
                          option(CommentBody) + option(WsBody) +
                          (lit<"\r\n"> | lit<"\n"> | lit<"\r">)> {};
  struct NEWLINE : terminal<some(cr<R"(\r\n)">)> {};

  struct ID : terminal<cr<"a-zA-Z"> + many(w)> {};

  struct STRING_LITERAL
      : terminal<lit<"\""> + many(cr<R"(^"\r\n)">) + lit<"\"">, UnquotedHT> {};
  struct NUMBER_LITERAL
      : terminal<(some(d) + option(lit<"."> + option(some(d)))) |
                     (lit<"."> + some(d)),
                 double> {};

  static constexpr auto Of = ikw<"of"> | ikw<"from"> | ikw<"in">;
  static constexpr auto CardKw = ikw<"card"> | ikw<"cd">;
  static constexpr auto CardsKw = ikw<"cards"> | ikw<"cds">;
  static constexpr auto BackgroundKw = ikw<"background"> | ikw<"backgrounds"> |
                                       ikw<"bkgnd"> | ikw<"bkgnds"> |
                                       ikw<"bg"> | ikw<"bgs">;
  static constexpr auto ButtonKw =
      ikw<"button"> | ikw<"buttons"> | ikw<"btn"> | ikw<"btns">;
  static constexpr auto FieldKw =
      ikw<"field"> | ikw<"fields"> | ikw<"fld"> | ikw<"flds">;
  static constexpr auto CharacterKw =
      ikw<"character"> | ikw<"characters"> | ikw<"char"> | ikw<"chars">;
  static constexpr auto WordKw = ikw<"word"> | ikw<"words">;
  static constexpr auto LineKw = ikw<"line"> | ikw<"lines">;
  static constexpr auto ItemKw = ikw<"item"> | ikw<"items">;
  static constexpr auto SecondsKw =
      ikw<"seconds"> | ikw<"secs"> | ikw<"second"> | ikw<"sec">;

  static constexpr auto CommandNameKw =
      ikw<"answer"> | ikw<"ask"> | ikw<"put"> | ikw<"get"> | ikw<"set"> |
      ikw<"send"> | ikw<"wait"> | ikw<"sort"> | ikw<"go"> | ikw<"enable"> |
      ikw<"disable"> | ikw<"read"> | ikw<"write"> | ikw<"hide"> |
      ikw<"show"> | ikw<"add"> | ikw<"subtract"> | ikw<"multiply"> |
      ikw<"divide"> | ikw<"choose"> | ikw<"click"> | ikw<"drag"> |
      ikw<"type"> | ikw<"lock"> | ikw<"unlock"> | ikw<"pass"> |
      ikw<"domenu"> | ikw<"visual"> | ikw<"reset"> | ikw<"create"> |
      ikw<"delete"> | ikw<"play"> | ikw<"dial"> | ikw<"beep"> |
      ikw<"open"> | ikw<"close"> | ikw<"select"> | ikw<"find"> |
      ikw<"import"> | ikw<"export">;

  // ---- Forward declarations: the expression/part hierarchy is mutually
  // recursive. ---------------------------------------------------------
  struct Expression;
  struct UnaryExpr;
  struct Factor;
  struct ChunkFreeFactor;
  struct FunctionCall;
  struct BuiltinFunc;
  struct Property;
  struct MenuRule;
  struct MenuItemRule;
  struct Part;
  struct CardPart;
  struct BkgndPart;
  struct StackPart;
  struct ButtonPart;
  struct FieldPart;
  struct WindowPart;
  struct ContainerBase;
  struct Statement;

  struct OrdinalWord
      : rule<ikw<"first"> | ikw<"second"> | ikw<"third"> | ikw<"fourth"> |
                 ikw<"fifth"> | ikw<"sixth"> | ikw<"seventh"> |
                 ikw<"eighth"> | ikw<"ninth"> | ikw<"tenth"> | ikw<"mid"> |
                 ikw<"middle"> | ikw<"last"> | ikw<"any">,
             OrdinalConv> {};

  struct LengthWord
      : rule<ikw<"english"> | ikw<"long"> | ikw<"abbreviated"> |
                 ikw<"abbrev"> | ikw<"abbr"> | ikw<"short">,
             LengthConv> {};

  struct ChunkKindWord : rule<CharacterKw | WordKw | ItemKw | LineKw,
                              ChunkKindConv> {};

  struct NamedLitWord
      : rule<ikw<"empty"> | ikw<"pi"> | ikw<"quote"> | ikw<"return"> |
                 ikw<"space"> | ikw<"tab"> | ikw<"formfeed"> |
                 ikw<"linefeed"> | ikw<"comma"> | ikw<"colon"> |
                 ikw<"zero"> | ikw<"one"> | ikw<"two"> | ikw<"three"> |
                 ikw<"four"> | ikw<"five"> | ikw<"six"> | ikw<"seven"> |
                 ikw<"eight"> | ikw<"nine"> | ikw<"ten"> |
                 ikw<"commandkey"> | ikw<"cmdkey"> | ikw<"optionkey"> |
                 ikw<"shiftkey"> | ikw<"up"> | ikw<"down"> |
                 ikw<"number"> | ikw<"integer"> | ikw<"point"> |
                 ikw<"rect"> | ikw<"rectangle"> | ikw<"date"> |
                 ikw<"logical"> | ikw<"boolean"> | ikw<"bool"> |
                 ikw<"word"> | ikw<"chars"> | ikw<"whole"> | ikw<"string">,
             NamedLitConv> {};

  struct HandlerName : rule<ref<ID> | CommandNameKw> {};

  // ---- Literals ---------------------------------------------------------

  struct NamedLitRule
      : rule<create<ast::NamedLit>(
            assign<&ast::NamedLit::kind>(ref<NamedLitWord>))> {};
  struct FindFormWord
      : rule<ikw<"word"> | ikw<"chars"> | ikw<"whole"> | ikw<"string">,
             NamedLitConv> {};
  struct FindFormRule
      : rule<create<ast::NamedLit>(
            assign<&ast::NamedLit::kind>(ref<FindFormWord>))> {};
  struct StringLitRule
      : rule<create<ast::StringLit>(
            assign<&ast::StringLit::value>(ref<STRING_LITERAL>))> {};
  struct NumberLitRule
      : rule<create<ast::NumberLit>(
            assign<&ast::NumberLit::value>(ref<NUMBER_LITERAL>))> {};
  static constexpr auto LiteralExpr =
      ref<StringLitRule> | ref<NumberLitRule> | ref<NamedLitRule>;

  // ---- Chunk expressions --------------------------------------------

  struct ChunkItem
      : rule<create<ast::ChunkItem>(
            (option(ikw<"the">) +
             assign<&ast::ChunkItem::ordinal>(ref<OrdinalWord>) +
             assign<&ast::ChunkItem::kind>(ref<ChunkKindWord>) + Of)  //
            | (assign<&ast::ChunkItem::kind>(ref<ChunkKindWord>) +
               assign<&ast::ChunkItem::start>(ref<Expression>) +
               option(ikw<"to"> +
                      assign<&ast::ChunkItem::end>(ref<Expression>)) +
               Of))> {};

  // ---- Properties, menus -------------------------------------------

  struct PropertyName
      : rule<ref<ID> | ikw<"marked"> | ikw<"selectedtext"> |
                 ikw<"selectedchunk"> | ikw<"selectedfield"> |
                 ikw<"selectedline"> | ikw<"number"> | ikw<"id"> |
                 ikw<"rect"> | ikw<"rectangle"> | ikw<"bottom"> |
                 ikw<"left"> | ikw<"right"> | ikw<"top"> | ikw<"center"> |
                 ikw<"scroll"> | ikw<"script"> | ikw<"pattern">> {};

  struct Property
      : rule<create<ast::PropertyRef>(
            option(ikw<"the">) +
            option(assign<&ast::PropertyRef::length>(ref<LengthWord>)) +
            assign<&ast::PropertyRef::name>(ref<PropertyName>) +
            option(Of + assign<&ast::PropertyRef::ofTarget>(ref<Factor>)))> {
  };

  struct MenuRule
      : rule<create<ast::MenuRef>(
            (ikw<"menu"> + assign<&ast::MenuRef::nameExpr>(ref<Factor>))  //
            | (option(ikw<"the">) +
               assign<&ast::MenuRef::ordinal>(ref<OrdinalWord>) +
               ikw<"menu">))> {};

  struct MenuItemRule
      : rule<create<ast::MenuItemRef>(
            (ikw<"menuitem"> +
             assign<&ast::MenuItemRef::nameExpr>(ref<Factor>) + Of +
             assign<&ast::MenuItemRef::menu>(ref<MenuRule>))  //
            | (option(ikw<"the">) +
               assign<&ast::MenuItemRef::ordinal>(ref<OrdinalWord>) +
               ikw<"menuitem"> + Of +
               assign<&ast::MenuItemRef::menu>(ref<MenuRule>)))> {};

  // ---- Parts ----------------------------------------------------------

  struct PositionNext
      : rule<option(ikw<"the">) + ikw<"next">, value<ast::Position::Next>> {};
  struct PositionPrev
      : rule<option(ikw<"the">) + (ikw<"prev"> | ikw<"previous">),
             value<ast::Position::Prev>> {};
  struct PositionThis
      : rule<ikw<"this">, value<ast::Position::This>> {};
  static constexpr auto PositionRef =
      ref<PositionNext> | ref<PositionPrev> | ref<PositionThis>;

  struct StackPart
      : rule<create<ast::StackPartRef>(
            (option(ikw<"this">) + ikw<"stack">)  //
            | (ikw<"stack"> +
               assign<&ast::StackPartRef::nameExpr>(ref<Factor>)))> {};

  struct BkgndPart
      : rule<create<ast::BkgndPartRef>(
            ((BackgroundKw + flag<&ast::BkgndPartRef::byId>(ikw<"id">) +
              assign<&ast::BkgndPartRef::expr>(ref<Factor>))  //
             | (BackgroundKw +
                option(assign<&ast::BkgndPartRef::expr>(ref<Factor>)))  //
             | (option(ikw<"the">) +
                assign<&ast::BkgndPartRef::ordinal>(ref<OrdinalWord>) +
                BackgroundKw)  //
             | (option(assign<&ast::BkgndPartRef::position>(PositionRef)) +
                BackgroundKw)) +
            option(Of + assign<&ast::BkgndPartRef::ofStack>(ref<StackPart>)))> {
  };

  struct CardPart
      : rule<create<ast::CardPartRef>(
            ((CardKw + flag<&ast::CardPartRef::byId>(ikw<"id">) +
              assign<&ast::CardPartRef::expr>(ref<Factor>))  //
             | (option(assign<&ast::CardPartRef::position>(PositionRef)) +
                CardKw + option(assign<&ast::CardPartRef::expr>(ref<Factor>)))  //
             | (option(ikw<"the">) +
                assign<&ast::CardPartRef::ordinal>(ref<OrdinalWord>) +
                CardKw)) +
            option(Of + (assign<&ast::CardPartRef::ofBkgnd>(ref<BkgndPart>) |
                         assign<&ast::CardPartRef::ofStackDirect>(
                             ref<StackPart>))))> {};

  struct ButtonPart
      : rule<create<ast::ButtonPartRef>(
            ((option(CardKw) + ButtonKw +
              flag<&ast::ButtonPartRef::byId>(ikw<"id">) +
              assign<&ast::ButtonPartRef::expr>(ref<Factor>))  //
             | (flag<&ast::ButtonPartRef::background>(BackgroundKw) +
                ButtonKw + flag<&ast::ButtonPartRef::byId>(ikw<"id">) +
                assign<&ast::ButtonPartRef::expr>(ref<Factor>))  //
             | (option(CardKw) + ButtonKw +
                assign<&ast::ButtonPartRef::expr>(ref<Factor>))  //
             | (flag<&ast::ButtonPartRef::background>(BackgroundKw) +
                ButtonKw + assign<&ast::ButtonPartRef::expr>(ref<Factor>))  //
             | (option(ikw<"the">) +
                assign<&ast::ButtonPartRef::ordinal>(ref<OrdinalWord>) +
                option(CardKw) + ButtonKw)  //
             | (option(ikw<"the">) +
                assign<&ast::ButtonPartRef::ordinal>(ref<OrdinalWord>) +
                flag<&ast::ButtonPartRef::background>(BackgroundKw) +
                ButtonKw)) +
            option(Of + assign<&ast::ButtonPartRef::ofCard>(ref<CardPart>)))> {
  };

  struct FieldPart
      : rule<create<ast::FieldPartRef>(
            ((flag<&ast::FieldPartRef::card>(CardKw) + FieldKw +
              flag<&ast::FieldPartRef::byId>(ikw<"id">) +
              assign<&ast::FieldPartRef::expr>(ref<Factor>))  //
             | (option(BackgroundKw) + FieldKw +
                flag<&ast::FieldPartRef::byId>(ikw<"id">) +
                assign<&ast::FieldPartRef::expr>(ref<Factor>))  //
             | (flag<&ast::FieldPartRef::card>(CardKw) + FieldKw +
                assign<&ast::FieldPartRef::expr>(ref<Factor>))  //
             | (option(BackgroundKw) + FieldKw +
                assign<&ast::FieldPartRef::expr>(ref<Factor>))  //
             | (option(ikw<"the">) +
                assign<&ast::FieldPartRef::ordinal>(ref<OrdinalWord>) +
                flag<&ast::FieldPartRef::card>(CardKw) + FieldKw)  //
             | (option(ikw<"the">) +
                assign<&ast::FieldPartRef::ordinal>(ref<OrdinalWord>) +
                option(BackgroundKw) + FieldKw)) +
            option(Of + assign<&ast::FieldPartRef::ofCard>(ref<CardPart>)))> {
  };

  struct WindowKindWord
      : rule<(CardKw + ikw<"window">) | (ikw<"tool"> + ikw<"window">) |
                 (ikw<"pattern"> + ikw<"window">) |
                 (ikw<"message"> + ikw<"watcher">) |
                 (ikw<"variable"> + ikw<"watcher">)> {};

  struct WindowPart
      : rule<create<ast::WindowPartRef>(
            (option(ikw<"the">) +
             assign<&ast::WindowPartRef::kind>(ref<WindowKindWord>))  //
            | (ikw<"window"> + flag<&ast::WindowPartRef::byId>(ikw<"id">) +
               assign<&ast::WindowPartRef::expr>(ref<Expression>))  //
            | (ikw<"window"> +
               assign<&ast::WindowPartRef::expr>(ref<Expression>)))> {};

  struct MessagePart
      : rule<create<ast::MessageBoxRef>(
            option(ikw<"the">) + (ikw<"message"> | ikw<"msg">) +
            option(ikw<"box"> | ikw<"window">))> {};

  struct Part
      : rule<ref<MessagePart>  //
            | create<ast::GenericPartRef>(
                  CardKw + ikw<"part"> +
                  assign<&ast::GenericPartRef::index>(ref<Factor>))  //
            | create<ast::GenericPartRef>(
                  flag<&ast::GenericPartRef::background>(BackgroundKw) +
                  ikw<"part"> +
                  assign<&ast::GenericPartRef::index>(ref<Factor>))  //
            | create<ast::MeRef>(ikw<"me">)  //
            | ref<ButtonPart>                //
            | ref<FieldPart>                 //
            | ref<BkgndPart>                 //
            | ref<CardPart>                  //
            | ref<StackPart>                 //
            | ref<WindowPart>> {};

  struct ContainerBase
      : rule<create<ast::NameRef>(assign<&ast::NameRef::name>(ref<ID>))  //
            | create<ast::SelectionRef>(option(ikw<"the">) +
                                        ikw<"selection">)  //
            | create<ast::TargetRef>(ikw<"target">)  //
            | ref<Property>                          //
            | ref<MenuRule>                           //
            | ref<MenuItemRule>                        //
            | ref<Part>> {};

  // ---- Factor, unary, and the operator ladder ------------------------

  struct ToolText : rule<ikw<"text">, value<ast::ToolKind::Text>> {};
  struct ToolSelect : rule<ikw<"select">, value<ast::ToolKind::Select>> {};
  struct ToolField : rule<FieldKw, value<ast::ToolKind::Field>> {};
  struct ToolButton : rule<ButtonKw, value<ast::ToolKind::Button>> {};
  struct ToolLine : rule<ikw<"line">, value<ast::ToolKind::Line>> {};
  struct ToolPolygon
      : rule<option(ikw<"reg"> | ikw<"regular">) +
                 (ikw<"poly"> | ikw<"polygon">),
             value<ast::ToolKind::Polygon>> {};
  struct ToolRect
      : rule<option(ikw<"round">) + (ikw<"rect"> | ikw<"rectangle">),
             value<ast::ToolKind::Rect>> {};
  struct ToolSpray
      : rule<ikw<"spray"> + option(ikw<"can">), value<ast::ToolKind::Spray>> {
  };

  struct ToolExprRule
      : rule<create<ast::ToolExpr>(
            assign<&ast::ToolExpr::kind>(
                ref<ToolPolygon> | ref<ToolRect> | ref<ToolSpray> |
                ref<ToolText> | ref<ToolSelect> | ref<ToolField> |
                ref<ToolButton> | ref<ToolLine>)  //
            | assign<&ast::ToolExpr::expr>(ref<Expression>))> {};

  struct EffDissolve : rule<ikw<"dissolve">, value<ast::EffectKind::Dissolve>> {};
  struct EffBarnDoorOpen
      : rule<ikw<"barn"> + ikw<"door"> + ikw<"open">,
             value<ast::EffectKind::BarnDoorOpen>> {};
  struct EffBarnDoorClose
      : rule<ikw<"barn"> + ikw<"door"> + ikw<"close">,
             value<ast::EffectKind::BarnDoorClose>> {};
  struct EffCheckerboard
      : rule<ikw<"checkerboard">, value<ast::EffectKind::Checkerboard>> {};
  struct EffIrisOpen
      : rule<ikw<"iris"> + ikw<"open">, value<ast::EffectKind::IrisOpen>> {};
  struct EffIrisClose
      : rule<ikw<"iris"> + ikw<"close">, value<ast::EffectKind::IrisClose>> {
  };
  struct EffPlain : rule<ikw<"plain">, value<ast::EffectKind::Plain>> {};
  struct EffPushUp
      : rule<ikw<"push"> + ikw<"up">, value<ast::EffectKind::PushUp>> {};
  struct EffPushDown
      : rule<ikw<"push"> + ikw<"down">, value<ast::EffectKind::PushDown>> {};
  struct EffPushLeft
      : rule<ikw<"push"> + ikw<"left">, value<ast::EffectKind::PushLeft>> {};
  struct EffPushRight
      : rule<ikw<"push"> + ikw<"right">, value<ast::EffectKind::PushRight>> {
  };
  struct EffScrollDown
      : rule<ikw<"scroll"> + ikw<"down">, value<ast::EffectKind::ScrollDown>> {
  };
  struct EffScrollUp
      : rule<ikw<"scroll"> + ikw<"up">, value<ast::EffectKind::ScrollUp>> {};
  struct EffScrollLeft
      : rule<ikw<"scroll"> + ikw<"left">, value<ast::EffectKind::ScrollLeft>> {
  };
  struct EffScrollRight
      : rule<ikw<"scroll"> + ikw<"right">,
             value<ast::EffectKind::ScrollRight>> {};
  struct EffShrinkToTop
      : rule<ikw<"shrink"> + ikw<"to"> + ikw<"top">,
             value<ast::EffectKind::ShrinkToTop>> {};
  struct EffShrinkToCenter
      : rule<ikw<"shrink"> + ikw<"to"> + ikw<"center">,
             value<ast::EffectKind::ShrinkToCenter>> {};
  struct EffShrinkToBottom
      : rule<ikw<"shrink"> + ikw<"to"> + ikw<"bottom">,
             value<ast::EffectKind::ShrinkToBottom>> {};
  struct EffStretchFromTop
      : rule<ikw<"stretch"> + ikw<"from"> + ikw<"top">,
             value<ast::EffectKind::StretchFromTop>> {};
  struct EffStretchFromCenter
      : rule<ikw<"stretch"> + ikw<"from"> + ikw<"center">,
             value<ast::EffectKind::StretchFromCenter>> {};
  struct EffStretchFromBottom
      : rule<ikw<"stretch"> + ikw<"from"> + ikw<"bottom">,
             value<ast::EffectKind::StretchFromBottom>> {};
  struct EffVenetianBlinds
      : rule<ikw<"venetian"> + ikw<"blinds">,
             value<ast::EffectKind::VenetianBlinds>> {};
  struct EffWipeUp
      : rule<ikw<"wipe"> + ikw<"up">, value<ast::EffectKind::WipeUp>> {};
  struct EffWipeDown
      : rule<ikw<"wipe"> + ikw<"down">, value<ast::EffectKind::WipeDown>> {};
  struct EffWipeLeft
      : rule<ikw<"wipe"> + ikw<"left">, value<ast::EffectKind::WipeLeft>> {};
  struct EffWipeRight
      : rule<ikw<"wipe"> + ikw<"right">, value<ast::EffectKind::WipeRight>> {
  };
  struct EffZoomIn
      : rule<ikw<"zoom"> + ikw<"in">, value<ast::EffectKind::ZoomIn>> {};
  struct EffZoomOut
      : rule<ikw<"zoom"> + ikw<"out">, value<ast::EffectKind::ZoomOut>> {};
  struct EffZoomOpen
      : rule<ikw<"zoom"> + ikw<"open">, value<ast::EffectKind::ZoomOpen>> {};
  struct EffZoomClose
      : rule<ikw<"zoom"> + ikw<"close">, value<ast::EffectKind::ZoomClose>> {
  };
  static constexpr auto EffectWord =
      ref<EffBarnDoorOpen> | ref<EffBarnDoorClose> | ref<EffDissolve> |
      ref<EffCheckerboard> | ref<EffIrisOpen> | ref<EffIrisClose> |
      ref<EffPlain> | ref<EffPushUp> | ref<EffPushDown> | ref<EffPushLeft> |
      ref<EffPushRight> | ref<EffScrollDown> | ref<EffScrollUp> |
      ref<EffScrollLeft> | ref<EffScrollRight> | ref<EffShrinkToTop> |
      ref<EffShrinkToCenter> | ref<EffShrinkToBottom> |
      ref<EffStretchFromTop> | ref<EffStretchFromCenter> |
      ref<EffStretchFromBottom> | ref<EffVenetianBlinds> | ref<EffWipeUp> |
      ref<EffWipeDown> | ref<EffWipeLeft> | ref<EffWipeRight> |
      ref<EffZoomIn> | ref<EffZoomOut> | ref<EffZoomOpen> | ref<EffZoomClose>;

  struct SpeedVeryFast
      : rule<ikw<"very"> + ikw<"fast">, value<ast::SpeedKind::VeryFast>> {};
  struct SpeedVerySlow
      : rule<ikw<"very"> + (ikw<"slow"> | ikw<"slowly">),
             value<ast::SpeedKind::VerySlow>> {};
  struct SpeedFast : rule<ikw<"fast">, value<ast::SpeedKind::Fast>> {};
  struct SpeedSlow
      : rule<ikw<"slow"> | ikw<"slowly">, value<ast::SpeedKind::Slow>> {};
  static constexpr auto SpeedWord = ref<SpeedVeryFast> | ref<SpeedVerySlow> |
                                    ref<SpeedFast> | ref<SpeedSlow>;

  struct ImgBlack : rule<ikw<"black">, value<ast::ImageKind::Black>> {};
  struct ImgCard : rule<CardKw, value<ast::ImageKind::Card>> {};
  struct ImgGray
      : rule<ikw<"gray"> | ikw<"grey">, value<ast::ImageKind::Gray>> {};
  struct ImgInverse
      : rule<ikw<"inverse">, value<ast::ImageKind::Inverse>> {};
  struct ImgWhite : rule<ikw<"white">, value<ast::ImageKind::White>> {};
  static constexpr auto ImageWord = ref<ImgBlack> | ref<ImgCard> |
                                    ref<ImgGray> | ref<ImgInverse> |
                                    ref<ImgWhite>;

  struct EffectExprRule
      : rule<create<ast::EffectExpr>(
            option(ikw<"effect">) +
            assign<&ast::EffectExpr::effect>(EffectWord) +
            option(assign<&ast::EffectExpr::speed>(SpeedWord)) +
            option(ikw<"to"> +
                   assign<&ast::EffectExpr::image>(ImageWord)))> {};

  static constexpr auto ZeroArgFuncName =
      ikw<"mouseloc"> | ikw<"mouse"> | ikw<"result"> | ikw<"commandkey"> |
      ikw<"cmdkey"> | ikw<"shiftkey"> | ikw<"optionkey"> | ikw<"ticks"> |
      SecondsKw | (ref<LengthWord> + ikw<"time">) |
      (ref<LengthWord> + ikw<"date">) | ikw<"mouseclick"> | ikw<"tool"> |
      (ikw<"number"> + ikw<"of"> + option(CardKw) + ikw<"parts">) |
      (ikw<"number"> + ikw<"of"> + BackgroundKw + ikw<"parts">) |
      (ikw<"number"> + ikw<"of"> + option(CardKw) + ButtonKw) |
      (ikw<"number"> + ikw<"of"> + BackgroundKw + ButtonKw) |
      (ikw<"number"> + ikw<"of"> + CardKw + FieldKw) |
      (ikw<"number"> + ikw<"of"> + option(BackgroundKw) + FieldKw) |
      (ikw<"number"> + ikw<"of"> + CardsKw) |
      (ikw<"number"> + ikw<"of"> + BackgroundKw) |
      (ikw<"number"> + ikw<"of"> + ikw<"marked"> + CardsKw) |
      (ikw<"number"> + ikw<"of"> + ikw<"menus">) |
      (ikw<"number"> + ikw<"of"> + ikw<"windows">) | ikw<"menus"> |
      ikw<"diskspace"> | ikw<"params"> | ikw<"paramcount"> | ikw<"target"> |
      ikw<"speech"> | ikw<"voices"> | ikw<"windows"> | ikw<"stacks">;
  struct ZeroArgFunc : rule<ZeroArgFuncName> {};

  static constexpr auto SingleArgFuncWords =
      ikw<"average"> | ikw<"min"> | ikw<"max"> | ikw<"sum"> |
      (ikw<"number"> + ikw<"of"> + option(CardKw) + ikw<"parts">) |
      (ikw<"number"> + ikw<"of"> + BackgroundKw + ikw<"parts">) |
      (ikw<"number"> + ikw<"of"> + option(CardKw) + ButtonKw) |
      (ikw<"number"> + ikw<"of"> + BackgroundKw + ButtonKw) |
      (ikw<"number"> + ikw<"of"> + CardKw + FieldKw) |
      (ikw<"number"> + ikw<"of"> + option(BackgroundKw) + FieldKw) |
      (ikw<"number"> + ikw<"of"> + CardsKw) |
      (ikw<"number"> + ikw<"of"> + BackgroundKw) |
      (ikw<"number"> + ikw<"of"> + ikw<"marked"> + CardsKw) |
      (ikw<"number"> + ikw<"of"> + CharacterKw) |
      (ikw<"number"> + ikw<"of"> + WordKw) |
      (ikw<"number"> + ikw<"of"> + ItemKw) |
      (ikw<"number"> + ikw<"of"> + LineKw) |
      (ikw<"number"> + ikw<"of"> + ikw<"menuitems">) |
      ikw<"random"> | ikw<"sqrt"> | ikw<"trunc"> | ikw<"sin"> | ikw<"cos"> |
      ikw<"tan"> | ikw<"atan"> | ikw<"exp1"> | ikw<"exp2"> | ikw<"exp"> |
      ikw<"ln1"> | ikw<"ln"> | ikw<"log2"> | ikw<"abs"> | ikw<"chartonum"> |
      ikw<"numtochar"> | ikw<"value"> | ikw<"length"> | ikw<"diskspace"> |
      ikw<"param">;
  // `the number of <singular object>` is the property; only the counting
  // forms spelled out above are the function.
  static constexpr auto SingleArgFuncName = SingleArgFuncWords | ikw<"number">;
  struct SingleArgFunc : rule<SingleArgFuncName> {};
  struct SingleArgOfFunc : rule<SingleArgFuncWords> {};

  struct MultiArgFunc
      : rule<SingleArgFuncName | ikw<"annuity"> | ikw<"compound"> |
                 ikw<"offset">> {};

  struct BuiltinFunc
      : rule<create<ast::FuncCall>(
            (ikw<"the"> +
             assign<&ast::FuncCall::name>(ref<ZeroArgFunc>))  //
            | (option(ikw<"the">) +
               assign<&ast::FuncCall::name>(ref<SingleArgOfFunc>) + Of +
               assign<&ast::FuncCall::ofTarget>(ref<Factor>))  //
            | (assign<&ast::FuncCall::name>(ref<SingleArgFunc>) + lit<"("> +
               option(some(append<&ast::FuncCall::args>(ref<Expression>),
                           lit<",">)) +
               lit<")">)  //
            | (assign<&ast::FuncCall::name>(ref<MultiArgFunc>) + lit<"("> +
               option(some(append<&ast::FuncCall::args>(ref<Expression>),
                           lit<",">)) +
               lit<")">))> {};

  struct FunctionCall
      : rule<ref<BuiltinFunc>  //
            | create<ast::FuncCall>(
                  assign<&ast::FuncCall::name>(ref<ID>) + lit<"("> +
                  option(some(append<&ast::FuncCall::args>(ref<Expression>),
                              lit<",">)) +
                  lit<")">)> {};

  struct ChunkFreeFactor
      : rule<LiteralExpr                                //
            | (lit<"("> + ref<Expression> + lit<")">)  //
            | ref<EffectExprRule>                        //
            | ref<FunctionCall>                           //
            | ref<ContainerBase>> {};

  struct Factor
      : rule<create<ast::ChunkExpr>(
                 some(append<&ast::ChunkExpr::items>(ref<ChunkItem>)) +
                 assign<&ast::ChunkExpr::target>(ref<ChunkFreeFactor>))  //
            | ref<ChunkFreeFactor>> {};

  struct OpNot : rule<ikw<"not">, value<ast::UnOp::LogicalNot>> {};
  struct OpNeg : rule<lit<"-">, value<ast::UnOp::Negate>> {};
  struct OpThereIsA
      : rule<ikw<"there"> + ikw<"is"> + (ikw<"a"> | ikw<"an">),
             value<ast::UnOp::ThereIsA>> {};
  struct OpThereIsNot
      : rule<ikw<"there"> + ikw<"is"> +
                 (ikw<"no"> | (ikw<"not"> + (ikw<"a"> | ikw<"an">))),
             value<ast::UnOp::ThereIsNot>> {};

  struct UnaryExpr
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(
                     ref<OpThereIsNot> | ref<OpThereIsA> | ref<OpNot> |
                     ref<OpNeg>) +
                 assign<&ast::UnaryExpr::operand>(ref<UnaryExpr>))  //
            | ref<Factor>> {};

  struct OpExp : terminal<lit<"^">, value<ast::BinOp::Exponent>> {};
  struct OpMul : terminal<lit<"*">, value<ast::BinOp::Multiply>> {};
  struct OpDiv : terminal<lit<"/">, value<ast::BinOp::Divide>> {};
  struct OpMod : rule<ikw<"mod">, value<ast::BinOp::Modulo>> {};
  struct OpDivWord : rule<ikw<"div">, value<ast::BinOp::IntegerDivide>> {};
  struct OpAdd : terminal<lit<"+">, value<ast::BinOp::Add>> {};
  struct OpSub : terminal<lit<"-">, value<ast::BinOp::Subtract>> {};
  struct OpConcatSp : terminal<lit<"&&">, value<ast::BinOp::ConcatSpace>> {};
  struct OpConcat : terminal<lit<"&">, value<ast::BinOp::Concat>> {};

  struct OpGe
      : terminal<lit<">="> | lit<"≥">, value<ast::BinOp::GreaterEq>> {};
  struct OpLe
      : terminal<lit<"<="> | lit<"≤">, value<ast::BinOp::LessEq>> {};
  struct OpLt : terminal<lit<"<">, value<ast::BinOp::Less>> {};
  struct OpGt : terminal<lit<">">, value<ast::BinOp::Greater>> {};
  struct OpContains : rule<ikw<"contains">, value<ast::BinOp::Contains>> {};

  // `is`/`is not` open both the relational family below (is a/an, is
  // within, is in) and the equality family below that (is, is not); an
  // infix<> level rides the token its operators open on, so the two
  // families need their own copy of each word to sit at two levels.
  struct IsRelWord : terminal<ikw<"is">> {};
  struct IsEqWord : terminal<ikw<"is">> {};

  struct OpIsNotIn
      : rule<ref<IsRelWord> + ikw<"not"> + ikw<"in">,
             value<ast::BinOp::IsNotIn>> {};
  struct OpIsIn
      : rule<ref<IsRelWord> + ikw<"in">, value<ast::BinOp::IsIn>> {};
  struct OpIsNotWithin
      : rule<ref<IsRelWord> + ikw<"not"> + ikw<"within">,
             value<ast::BinOp::IsNotWithin>> {};
  struct OpIsWithin
      : rule<ref<IsRelWord> + ikw<"within">, value<ast::BinOp::IsWithin>> {};
  struct OpIsNotA
      : rule<ref<IsRelWord> + ikw<"not"> + (ikw<"a"> | ikw<"an">),
             value<ast::BinOp::IsNotA>> {};
  struct OpIsA
      : rule<ref<IsRelWord> + (ikw<"a"> | ikw<"an">),
             value<ast::BinOp::IsA>> {};
  struct OpIsNotEqual
      : rule<ref<IsEqWord> + ikw<"not">, value<ast::BinOp::IsNotEqual>> {};
  struct OpIsEqual : rule<ref<IsEqWord>, value<ast::BinOp::IsEqual>> {};

  struct OpEqual : terminal<lit<"=">, value<ast::BinOp::Equal>> {};
  struct OpNotEqual
      : terminal<lit<"<>"> | lit<"≠">, value<ast::BinOp::NotEqual>> {};

  struct OpAnd : rule<ikw<"and">, value<ast::BinOp::LogicalAnd>> {};
  struct OpOr : rule<ikw<"or">, value<ast::BinOp::LogicalOr>> {};

  static constexpr auto RelOps =
      ref<OpIsNotWithin> | ref<OpIsWithin> | ref<OpIsNotIn> | ref<OpIsIn> |
      ref<OpIsNotA> | ref<OpIsA> | ref<OpGe> | ref<OpLe> | ref<OpLt> |
      ref<OpGt> | ref<OpContains>;
  static constexpr auto EqOps = ref<OpEqual> | ref<OpNotEqual> |
                                ref<OpIsNotEqual> | ref<OpIsEqual>;

  struct Expression
      : infix<UnaryExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(ref<OpExp>) >>  //
               left(ref<OpMul> | ref<OpDiv> | ref<OpMod> | ref<OpDivWord>) >>
               left(ref<OpAdd> | ref<OpSub>) >>          //
               left(ref<OpConcatSp> | ref<OpConcat>) >>  //
               left(RelOps) >>                            //
               left(EqOps) >>                             //
               left(ref<OpAnd>) >>                        //
               left(ref<OpOr>))> {};

  // ---- Statement-level helpers ---------------------------------------

  template <auto Ptr>
  static constexpr auto StatementListBody =
      some(append<Ptr>(ref<Statement>) + some(ref<NEWLINE>));

  template <auto Ptr>
  static constexpr auto ParamList = some(append<Ptr>(ref<ID>), lit<",">);

  struct MessageStatement
      : rule<create<ast::MessageStmt>(
            assign<&ast::MessageStmt::name>(ref<ID>) +
            option(some(append<&ast::MessageStmt::args>(ref<Expression>),
                        lit<",">)))> {};

  struct GlobalStmt
      : rule<create<ast::GlobalStmt>(
            ikw<"global"> + ParamList<&ast::GlobalStmt::names>)> {};

  struct ReturnStmt
      : rule<create<ast::ReturnStmt>(
            ikw<"return"> +
            option(assign<&ast::ReturnStmt::value>(ref<Expression>)))> {};

  struct IfStatement
      : rule<create<ast::IfStmt>(
            ikw<"if"> + assign<&ast::IfStmt::cond>(ref<Expression>) +
            ((option(ref<NEWLINE>) + ikw<"then"> +
              append<&ast::IfStmt::thenBody>(ref<Statement>) +
              option(ref<NEWLINE>) +
              option(flag<&ast::IfStmt::hasElse>(
                  (ikw<"else"> +
                   append<&ast::IfStmt::elseBody>(ref<Statement>) +
                   option(some(ref<NEWLINE>) + ikw<"end"> + ikw<"if">))  //
                  | (ikw<"else"> + some(ref<NEWLINE>) +
                     option(StatementListBody<&ast::IfStmt::elseBody>) +
                     ikw<"end"> + ikw<"if">))))  //
             | (option(ref<NEWLINE>) + ikw<"then"> + some(ref<NEWLINE>) +
                option(StatementListBody<&ast::IfStmt::thenBody>) +
                (flag<&ast::IfStmt::hasElse>(
                     (ikw<"else"> +
                      append<&ast::IfStmt::elseBody>(ref<Statement>) +
                      option(some(ref<NEWLINE>) + ikw<"end"> + ikw<"if">))  //
                     | (ikw<"else"> + some(ref<NEWLINE>) +
                        option(StatementListBody<&ast::IfStmt::elseBody>) +
                        ikw<"end"> + ikw<"if">))  //
                 | (ikw<"end"> + ikw<"if">)))))> {};

  struct RepeatStatement
      : rule<create<ast::RepeatStmt>(
            ikw<"repeat"> +
            option(
                (ikw<"until"> + assign<&ast::RepeatStmt::cond>(ref<Expression>))  //
                | (flag<&ast::RepeatStmt::isWhile>(ikw<"while">) +
                   assign<&ast::RepeatStmt::cond>(ref<Expression>))  //
                | (option(ikw<"for">) +
                   assign<&ast::RepeatStmt::count>(ref<Expression>) +
                   option(flag<&ast::RepeatStmt::hasTimes>(
                       ikw<"times">)))  //
                | (ikw<"with"> + assign<&ast::RepeatStmt::loopVar>(ref<ID>) +
                   lit<"="> +
                   assign<&ast::RepeatStmt::rangeStart>(ref<Expression>) +
                   option(flag<&ast::RepeatStmt::downTo>(ikw<"down">)) +
                   ikw<"to"> +
                   assign<&ast::RepeatStmt::rangeEnd>(ref<Expression>))  //
                | ikw<"forever">) +
            some(ref<NEWLINE>) +
            option(StatementListBody<&ast::RepeatStmt::body>) + ikw<"end"> +
            ikw<"repeat">)> {};

  // ---- Commands -------------------------------------------------------

  struct VerbDelete : rule<ikw<"delete">, value<ast::CmdVerb::Delete>> {};
  struct VerbDial : rule<ikw<"dial">, value<ast::CmdVerb::Dial>> {};
  struct VerbDomenu : rule<ikw<"domenu">, value<ast::CmdVerb::Domenu>> {};
  struct VerbDo : rule<ikw<"do">, value<ast::CmdVerb::Do>> {};
  struct VerbGet : rule<ikw<"get">, value<ast::CmdVerb::Get>> {};
  struct VerbVisual : rule<ikw<"visual">, value<ast::CmdVerb::Visual>> {};
  struct SimpleExprCmd
      : rule<create<ast::SimpleExprCmd>(
            assign<&ast::SimpleExprCmd::verb>(
                ref<VerbDelete> | ref<VerbDial> | ref<VerbDomenu> |
                ref<VerbDo> | ref<VerbGet> | ref<VerbVisual>) +
            assign<&ast::SimpleExprCmd::operand>(ref<Expression>))> {};

  struct AddSubCmd
      : rule<create<ast::AddSubCmd>(
            (flag<&ast::AddSubCmd::isAdd>(ikw<"add">) +
             assign<&ast::AddSubCmd::amount>(ref<Expression>) + ikw<"to"> +
             assign<&ast::AddSubCmd::target>(ref<Expression>))  //
            | (ikw<"subtract"> +
               assign<&ast::AddSubCmd::amount>(ref<Expression>) +
               ikw<"from"> +
               assign<&ast::AddSubCmd::target>(ref<Expression>)))> {};

  struct MulDivCmd
      : rule<create<ast::MulDivCmd>(
            (flag<&ast::MulDivCmd::isMultiply>(ikw<"multiply">) +
             assign<&ast::MulDivCmd::target>(ref<Expression>) + ikw<"by"> +
             assign<&ast::MulDivCmd::amount>(ref<Expression>))  //
            | (ikw<"divide"> +
               assign<&ast::MulDivCmd::target>(ref<Expression>) + ikw<"by"> +
               assign<&ast::MulDivCmd::amount>(ref<Expression>)))> {};

  struct EnableCmd
      : rule<create<ast::EnableCmd>(
            (flag<&ast::EnableCmd::enable>(ikw<"enable">) +
             assign<&ast::EnableCmd::target>(ref<Expression>))  //
            | (ikw<"disable"> +
               assign<&ast::EnableCmd::target>(ref<Expression>)))> {};

  struct ScreenLockCmd
      : rule<create<ast::ScreenLockCmd>(
            (flag<&ast::ScreenLockCmd::lock>(ikw<"lock">) +
             ikw<"screen">)  //
            | (ikw<"unlock"> + ikw<"screen"> +
               option(ikw<"with"> + ikw<"visual"> +
                      assign<&ast::ScreenLockCmd::visualEffect>(
                          ref<Expression>))))> {};

  struct AnswerCmd
      : rule<create<ast::AnswerCmd>(
            ikw<"answer"> + assign<&ast::AnswerCmd::prompt>(ref<Expression>) +
            option(ikw<"with"> +
                   append<&ast::AnswerCmd::buttons>(ref<Factor>) +
                   option(ikw<"or"> +
                          append<&ast::AnswerCmd::buttons>(ref<Factor>) +
                          option(ikw<"or"> + append<&ast::AnswerCmd::buttons>(
                                                  ref<Factor>)))))> {};

  struct AskCmd
      : rule<create<ast::AskCmd>(
            ikw<"ask"> + option(flag<&ast::AskCmd::file>(ikw<"file">)) +
            assign<&ast::AskCmd::prompt>(ref<Expression>) +
            option(ikw<"with"> +
                   assign<&ast::AskCmd::defaultAnswer>(ref<Expression>)))> {};

  struct BeepCmd
      : rule<create<ast::BeepCmd>(
            ikw<"beep"> +
            option(assign<&ast::BeepCmd::count>(ref<Expression>)))> {};

  struct ChooseCmd
      : rule<create<ast::ChooseCmd>(
            ikw<"choose"> +
            ((assign<&ast::ChooseCmd::tool>(ref<ToolExprRule>) +
              option(ikw<"tool">))  //
             | (ikw<"tool"> +
                assign<&ast::ChooseCmd::tool>(ref<ToolExprRule>))))> {};

  struct ClickCmd
      : rule<create<ast::ClickCmd>(
            ikw<"click"> + ikw<"at"> +
            some(append<&ast::ClickCmd::at>(ref<Expression>), lit<",">) +
            option(ikw<"with"> + some(append<&ast::ClickCmd::withKeys>(
                                          ref<Expression>),
                                      lit<",">)))> {};

  struct CloseFileCmd
      : rule<create<ast::CloseFileCmd>(
            ikw<"close"> + ikw<"file"> +
            assign<&ast::CloseFileCmd::file>(ref<Expression>))> {};

  struct ConvSeconds : rule<SecondsKw, value<ast::ConversionKind::Seconds>> {};
  struct ConvDateItems
      : rule<ikw<"dateitems">, value<ast::ConversionKind::DateItems>> {};
  struct ConvDate : rule<ikw<"date">, value<ast::ConversionKind::Date>> {};
  struct ConvTime : rule<ikw<"time">, value<ast::ConversionKind::Time>> {};
  struct ConversionFormatRule
      : rule<create<ast::ConversionFormat>(
            assign<&ast::ConversionFormat::kind>(ref<ConvSeconds> |
                                                 ref<ConvDateItems>)  //
            | (option(assign<&ast::ConversionFormat::length>(ref<LengthWord>)) +
               assign<&ast::ConversionFormat::kind>(ref<ConvDate> |
                                                    ref<ConvTime>)))> {};
  struct ConvertibleRule
      : rule<create<ast::Convertible>(
            append<&ast::Convertible::formats>(ref<ConversionFormatRule>) +
            option(ikw<"and"> + append<&ast::Convertible::formats>(
                                     ref<ConversionFormatRule>)))> {};

  struct ConvertCmd
      : rule<create<ast::ConvertCmd>(
            ikw<"convert"> + assign<&ast::ConvertCmd::target>(ref<Expression>) +
            option(ikw<"from"> +
                   assign<&ast::ConvertCmd::fromFormat>(ref<ConvertibleRule>)) +
            ikw<"to"> +
            assign<&ast::ConvertCmd::toFormat>(ref<ConvertibleRule>))> {};

  struct CreateMenuCmd
      : rule<create<ast::CreateMenuCmd>(
            ikw<"create"> + ikw<"menu"> +
            assign<&ast::CreateMenuCmd::name>(ref<Expression>))> {};

  struct DragCmd
      : rule<create<ast::DragCmd>(
            ikw<"drag"> + ikw<"from"> +
            some(append<&ast::DragCmd::from>(ref<Expression>), lit<",">) +
            ikw<"to"> +
            some(append<&ast::DragCmd::to>(ref<Expression>), lit<",">) +
            option(ikw<"with"> + some(append<&ast::DragCmd::withKeys>(
                                          ref<Expression>),
                                      lit<",">)))> {};

  struct EditScriptCmd
      : rule<create<ast::EditScriptCmd>(
            ikw<"edit"> + option(ikw<"the">) + ikw<"script"> + Of +
            assign<&ast::EditScriptCmd::target>(ref<Expression>))> {};

  struct ExitCmd
      : rule<create<ast::ExitCmd>(
            ikw<"exit"> +
            (ikw<"repeat">  //
             | flag<&ast::ExitCmd::toHyperCard>(ikw<"to"> +
                                                     ikw<"hypercard">)  //
             | assign<&ast::ExitCmd::name>(ref<HandlerName>)))> {};

  struct ExportPaintCmd
      : rule<create<ast::ExportPaintCmd>(
            ikw<"export"> + ikw<"paint"> + ikw<"to"> + ikw<"file"> +
            assign<&ast::ExportPaintCmd::file>(ref<Expression>))> {};

  struct FindCmd
      : rule<create<ast::FindCmd>(
            ikw<"find"> +
            option(assign<&ast::FindCmd::style>(ref<FindFormRule>)) +
            option(flag<&ast::FindCmd::international>(
                ikw<"international">)) +
            assign<&ast::FindCmd::what>(ref<Expression>) +
            option(Of +
                   ((assign<&ast::FindCmd::field>(ref<Expression>) +
                     option(Of +
                            flag<&ast::FindCmd::markedCardsOnly>(
                                ikw<"marked">) +
                            CardsKw))  //
                    | (flag<&ast::FindCmd::markedCardsOnly>(
                           ikw<"marked">) +
                       CardsKw))))> {};

  struct RemoteNewWindowNoDialog
      : rule<ikw<"in"> + ikw<"a"> + ikw<"new"> + ikw<"window"> +
                 ikw<"without"> + ikw<"dialog">,
             value<ast::RemoteNav::NewWindowNoDialog>> {};
  struct RemoteNewWindow
      : rule<ikw<"in"> + ikw<"a"> + ikw<"new"> + ikw<"window">,
             value<ast::RemoteNav::NewWindow>> {};
  struct RemoteNoDialog
      : rule<ikw<"without"> + ikw<"dialog">,
             value<ast::RemoteNav::NoDialog>> {};

  struct GoCmd
      : rule<create<ast::GoCmd>(
            ikw<"go"> +
            ((flag<&ast::GoCmd::back>(ikw<"back">) +
              option(ikw<"with"> + ikw<"visual"> +
                     assign<&ast::GoCmd::visualEffect>(ref<Expression>)))  //
             | (option(ikw<"to">) +
                assign<&ast::GoCmd::destination>(ref<Expression>) +
                ((ikw<"with"> + ikw<"visual"> +
                  assign<&ast::GoCmd::visualEffect>(ref<Expression>))  //
                 | option(assign<&ast::GoCmd::remoteOpt>(
                       ref<RemoteNewWindowNoDialog> | ref<RemoteNewWindow> |
                       ref<RemoteNoDialog>))))))> {};

  struct ImportPaintCmd
      : rule<create<ast::ImportPaintCmd>(
            ikw<"import"> + ikw<"paint"> + ikw<"from"> + ikw<"file"> +
            assign<&ast::ImportPaintCmd::file>(ref<Expression>))> {};

  struct NextRepeatCmd
      : rule<create<ast::NextRepeatCmd>(ikw<"next"> + ikw<"repeat">)> {};

  struct OpenFileCmd
      : rule<create<ast::OpenFileCmd>(
            ikw<"open"> + ikw<"file"> +
            assign<&ast::OpenFileCmd::file>(ref<Expression>))> {};

  struct PassCmd
      : rule<create<ast::PassCmd>(
            ikw<"pass"> + assign<&ast::PassCmd::name>(ref<HandlerName>))> {};

  struct MusicExprRule
      : rule<create<ast::MusicExpr>(
            assign<&ast::MusicExpr::voice>(ref<Expression>) +
            option(ikw<"tempo"> +
                   assign<&ast::MusicExpr::tempo>(ref<Expression>)) +
            option(assign<&ast::MusicExpr::notes>(ref<Expression>)))> {};

  struct PlayCmd
      : rule<create<ast::PlayCmd>(
            ikw<"play"> +
            assign<&ast::PlayCmd::music>(ref<MusicExprRule>))> {};

  struct PopCardCmd
      : rule<create<ast::PopCardCmd>(ikw<"pop"> + CardKw)> {};

  struct PushCmd
      : rule<create<ast::PushCmd>(
            ikw<"push"> + (flag<&ast::PushCmd::card>(CardKw)  //
                          | assign<&ast::PushCmd::value>(ref<Expression>)))> {
  };

  struct PrepBefore : rule<ikw<"before">, value<ast::Preposition::Before>> {};
  struct PrepAfter : rule<ikw<"after">, value<ast::Preposition::After>> {};
  struct PrepInto : rule<ikw<"into">, value<ast::Preposition::Into>> {};
  struct PutCmd
      : rule<create<ast::PutCmd>(
            ikw<"put"> +
            some(append<&ast::PutCmd::values>(ref<Expression>), lit<",">) +
            option(assign<&ast::PutCmd::prep>(ref<PrepBefore> | ref<PrepAfter> |
                                              ref<PrepInto>) +
                   assign<&ast::PutCmd::target>(ref<Expression>)))> {};

  struct ReadFileCmd
      : rule<create<ast::ReadFileCmd>(
            ikw<"read"> + ikw<"from"> + ikw<"file"> +
            assign<&ast::ReadFileCmd::file>(ref<Expression>) +
            option(
                (ikw<"until"> +
                 assign<&ast::ReadFileCmd::untilExpr>(ref<Expression>))  //
                | (ikw<"at"> +
                   assign<&ast::ReadFileCmd::atExpr>(ref<Expression>) +
                   ikw<"for"> +
                   assign<&ast::ReadFileCmd::forExpr>(ref<Expression>))  //
                | (ikw<"for"> +
                   assign<&ast::ReadFileCmd::forExpr>(ref<Expression>))))> {};

  struct ResetCmd
      : rule<create<ast::ResetCmd>(
            ikw<"reset"> +
            ((option(ikw<"the">) + ikw<"menubar">)  //
             | flag<&ast::ResetCmd::paint>(ikw<"paint">)))> {};

  struct PosBefore
      : rule<ikw<"before">, value<ast::BeforeAfterKind::Before>> {};
  struct PosAfter : rule<ikw<"after">, value<ast::BeforeAfterKind::After>> {};
  struct SelectCmd
      : rule<create<ast::SelectCmd>(
            ikw<"select"> +
            (flag<&ast::SelectCmd::empty>(ikw<"empty">)  //
             | (option(assign<&ast::SelectCmd::pos>(ref<PosBefore> |
                                                    ref<PosAfter>)) +
                ((flag<&ast::SelectCmd::text>(ikw<"text">) + Of +
                  assign<&ast::SelectCmd::target>(ref<Expression>))  //
                 | assign<&ast::SelectCmd::target>(ref<Expression>)))))> {};

  struct PVPlain : rule<ikw<"plain">, value<ast::PropValueKw::Plain>> {};
  struct PVMenu : rule<ikw<"menu">, value<ast::PropValueKw::Menu>> {};
  struct PVBottom : rule<ikw<"bottom">, value<ast::PropValueKw::Bottom>> {};
  struct PVLeft : rule<ikw<"left">, value<ast::PropValueKw::Left>> {};
  struct PVRight : rule<ikw<"right">, value<ast::PropValueKw::Right>> {};
  struct PVTop : rule<ikw<"top">, value<ast::PropValueKw::Top>> {};
  struct PVCenter : rule<ikw<"center">, value<ast::PropValueKw::Center>> {};
  struct PropertyValueRule
      : rule<create<ast::PropertyValue>(
            assign<&ast::PropertyValue::kw>(ref<PVPlain> | ref<PVMenu> |
                                            ref<PVBottom> | ref<PVLeft> |
                                            ref<PVRight> | ref<PVTop> |
                                            ref<PVCenter>)  //
            | some(append<&ast::PropertyValue::exprs>(ref<Expression>),
                   lit<",">))> {};

  struct SetCmd
      : rule<create<ast::SetCmd>(
            ikw<"set"> + assign<&ast::SetCmd::property>(ref<Property>) +
            ikw<"to"> +
            assign<&ast::SetCmd::value>(ref<PropertyValueRule>))> {};

  struct SendCmd
      : rule<create<ast::SendCmd>(
            ikw<"send"> +
            some(append<&ast::SendCmd::message>(ref<Expression>), lit<",">) +
            ikw<"to"> + assign<&ast::SendCmd::target>(ref<Expression>))> {};

  static constexpr auto PictureKw = ikw<"picture"> | ikw<"pict">;

  struct ShowHideCmd
      : rule<create<ast::ShowHideCmd>(
            (flag<&ast::ShowHideCmd::show>(ikw<"show">) |
             ikw<"hide">) +
            (flag<&ast::ShowHideCmd::titlebar>(ikw<"titlebar">)  //
             | (flag<&ast::ShowHideCmd::card>(CardKw) +
                flag<&ast::ShowHideCmd::picture>(PictureKw))  //
             | (flag<&ast::ShowHideCmd::background>(BackgroundKw) +
                flag<&ast::ShowHideCmd::picture>(PictureKw))  //
             | (flag<&ast::ShowHideCmd::picture>(PictureKw) + Of +
                assign<&ast::ShowHideCmd::target>(ref<Expression>))  //
             | assign<&ast::ShowHideCmd::target>(ref<Expression>)))> {};

  struct SortAsc : rule<ikw<"ascending">, value<ast::Ordering::Ascending>> {};
  struct SortDesc
      : rule<ikw<"descending">, value<ast::Ordering::Descending>> {};
  struct StyleText : rule<ikw<"text">, value<ast::SortStyleKind::Text>> {};
  struct StyleNumeric
      : rule<ikw<"numeric">, value<ast::SortStyleKind::Numeric>> {};
  struct StyleIntl
      : rule<ikw<"international">, value<ast::SortStyleKind::International>> {
  };
  struct StyleDatetime
      : rule<ikw<"datetime">, value<ast::SortStyleKind::Datetime>> {};
  static constexpr auto SortDirOpt =
      option(assign<&ast::SortCmd::direction>(ref<SortAsc> | ref<SortDesc>));
  static constexpr auto SortStyleOpt =
      option(assign<&ast::SortCmd::style>(ref<StyleText> | ref<StyleNumeric> |
                                          ref<StyleIntl> | ref<StyleDatetime>));

  struct SortCmd
      : rule<create<ast::SortCmd>(
            ikw<"sort"> +
            ((option(ikw<"the">) +
              assign<&ast::SortCmd::chunkType>(ref<ChunkKindWord>) + Of +
              assign<&ast::SortCmd::target>(ref<Expression>) + SortDirOpt +
              SortStyleOpt +
              option(ikw<"by"> +
                     assign<&ast::SortCmd::byExpr>(ref<Expression>)))  //
             | (option(ikw<"this">) +
                flag<&ast::SortCmd::cards>(ikw<"stack">) + SortDirOpt +
                SortStyleOpt + ikw<"by"> +
                assign<&ast::SortCmd::byExpr>(ref<Expression>))  //
             | (option(ikw<"the">) +
                option(flag<&ast::SortCmd::marked>(ikw<"marked">)) +
                flag<&ast::SortCmd::cards>(CardsKw) +
                option(Of + ((ikw<"this"> + ikw<"stack">)  //
                            | assign<&ast::SortCmd::target>(ref<Expression>))) +
                SortDirOpt + SortStyleOpt + ikw<"by"> +
                assign<&ast::SortCmd::byExpr>(ref<Expression>))  //
             | (SortDirOpt + SortStyleOpt + ikw<"by"> +
                assign<&ast::SortCmd::byExpr>(ref<Expression>))  //
             | (assign<&ast::SortCmd::target>(ref<Expression>) + SortDirOpt +
                SortStyleOpt + ikw<"by"> +
                assign<&ast::SortCmd::byExpr>(ref<Expression>))))> {};

  struct GenderMale : rule<ikw<"male">, value<ast::Gender::Male>> {};
  struct GenderFemale : rule<ikw<"female">, value<ast::Gender::Female>> {};
  struct GenderNeuter : rule<ikw<"neuter">, value<ast::Gender::Neuter>> {};
  struct GenderRobotic
      : rule<ikw<"robotic">, value<ast::Gender::Robotic>> {};
  struct SpeakCmd
      : rule<create<ast::SpeakCmd>(
            ikw<"speak"> + assign<&ast::SpeakCmd::text>(ref<Expression>) +
            option(ikw<"with"> +
                   ((assign<&ast::SpeakCmd::gender>(
                         ref<GenderMale> | ref<GenderFemale> |
                         ref<GenderNeuter> | ref<GenderRobotic>) +
                     ikw<"voice">)  //
                    | (ikw<"voice"> + assign<&ast::SpeakCmd::voiceExpr>(
                                          ref<Expression>)))))> {};

  struct TypeCmd
      : rule<create<ast::TypeCmd>(
            ikw<"type"> + assign<&ast::TypeCmd::text>(ref<Expression>) +
            option(ikw<"with"> +
                   flag<&ast::TypeCmd::withCommandKey>(
                       ikw<"commandkey"> | ikw<"cmdkey">)))> {};

  struct UnitTicks
      : rule<ikw<"ticks"> | ikw<"tick">, value<ast::TimeUnitKind::Ticks>> {};
  struct UnitSeconds
      : rule<SecondsKw, value<ast::TimeUnitKind::Seconds>> {};
  struct WaitCmd
      : rule<create<ast::WaitCmd>(
            ikw<"wait"> +
            ((ikw<"until"> + assign<&ast::WaitCmd::cond>(ref<Expression>))  //
             | (flag<&ast::WaitCmd::isWhile>(ikw<"while">) +
                assign<&ast::WaitCmd::cond>(ref<Expression>))  //
             | (option(ikw<"for">) +
                assign<&ast::WaitCmd::amount>(ref<Expression>) +
                assign<&ast::WaitCmd::unit>(ref<UnitTicks> |
                                            ref<UnitSeconds>))))> {};

  struct WriteFileCmd
      : rule<create<ast::WriteFileCmd>(
            ikw<"write"> + assign<&ast::WriteFileCmd::text>(ref<Expression>) +
            ikw<"to"> + ikw<"file"> +
            assign<&ast::WriteFileCmd::file>(ref<Expression>) +
            option(ikw<"at"> +
                   (flag<&ast::WriteFileCmd::atEof>(ikw<"eof"> |
                                                         ikw<"end">)  //
                    | assign<&ast::WriteFileCmd::atExpr>(
                          ref<Expression>))))> {};

  // ---- Statement, handler, function, document ------------------------

  struct Statement
      : rule<ref<IfStatement>       //
            | ref<RepeatStatement>  //
            | ref<GlobalStmt>       //
            | ref<ReturnStmt>       //
            | ref<AddSubCmd>        //
            | ref<AnswerCmd>        //
            | ref<AskCmd>           //
            | ref<BeepCmd>          //
            | ref<ChooseCmd>        //
            | ref<ClickCmd>         //
            | ref<CloseFileCmd>     //
            | ref<ConvertCmd>       //
            | ref<CreateMenuCmd>    //
            | ref<DragCmd>          //
            | ref<EditScriptCmd>    //
            | ref<EnableCmd>        //
            | ref<ExitCmd>          //
            | ref<ExportPaintCmd>   //
            | ref<FindCmd>          //
            | ref<GoCmd>            //
            | ref<ImportPaintCmd>   //
            | ref<MulDivCmd>        //
            | ref<NextRepeatCmd>    //
            | ref<OpenFileCmd>      //
            | ref<PassCmd>          //
            | ref<PlayCmd>          //
            | ref<PopCardCmd>       //
            | ref<PushCmd>          //
            | ref<PutCmd>           //
            | ref<ReadFileCmd>      //
            | ref<ResetCmd>         //
            | ref<ScreenLockCmd>    //
            | ref<SelectCmd>        //
            | ref<SendCmd>          //
            | ref<SetCmd>           //
            | ref<ShowHideCmd>      //
            | ref<SortCmd>          //
            | ref<SpeakCmd>         //
            | ref<TypeCmd>          //
            | ref<WaitCmd>          //
            | ref<WriteFileCmd>     //
            | ref<SimpleExprCmd>    //
            | ref<MessageStatement>  //
            | create<ast::ExprStmt>(
                  assign<&ast::ExprStmt::value>(ref<Expression>))> {};

  struct HandlerRule
      : rule<create<ast::Handler>(
            ikw<"on"> + assign<&ast::Handler::name>(ref<HandlerName>) +
            option(ParamList<&ast::Handler::params>) + some(ref<NEWLINE>) +
            option(StatementListBody<&ast::Handler::body>) + ikw<"end"> +
            assign<&ast::Handler::endName>(ref<HandlerName>))> {};

  struct FunctionRule
      : rule<create<ast::Function>(
            ikw<"function"> + assign<&ast::Function::name>(ref<ID>) +
            option(ParamList<&ast::Function::params>) + some(ref<NEWLINE>) +
            option(StatementListBody<&ast::Function::body>) + ikw<"end"> +
            assign<&ast::Function::endName>(ref<ID>))> {};

  struct TopItemRule
      : rule<create<ast::TopItem>(
            assign<&ast::TopItem::handler>(ref<HandlerRule>))  //
            | create<ast::TopItem>(
                  assign<&ast::TopItem::function>(ref<FunctionRule>))  //
            | create<ast::TopItem>(
                  assign<&ast::TopItem::statement>(ref<Statement>))> {};

  // HyperTalk's reference grammar has two start rules -- a script of
  // handlers/functions, a scriptlet of bare statements -- and cursiv has
  // one entry; Document unions both, freely interleaved.
  struct Document
      : rule<create<ast::Document>(
            many(ref<NEWLINE>) +
            many(append<&ast::Document::items>(ref<TopItemRule>) +
                 many(ref<NEWLINE>)))> {};

  using entry = Document;
  using ignore = list<WS, BREAK>;
  using hidden = list<COMMENT>;
};

}  // namespace hypertalk
