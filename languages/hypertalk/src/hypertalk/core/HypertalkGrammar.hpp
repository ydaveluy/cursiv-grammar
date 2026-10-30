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

inline constexpr auto length_of = [](std::string_view t) -> ast::Length {
  using L = ast::Length;
  if (ci_eq(t, "short")) return L::Short;
  if (ci_eq(t, "english")) return L::English;
  if (ci_eq(t, "long")) return L::Long;
  return L::Abbreviated;  // "abbreviated" / "abbrev" / "abbr"
};

inline constexpr auto chunk_kind_of = [](std::string_view t) -> ast::ChunkKind {
  using K = ast::ChunkKind;
  const char c0 = t.empty() ? '\0' : t.front();
  if (c0 == 'w' || c0 == 'W') return K::Word;
  if (c0 == 'i' || c0 == 'I') return K::Item;
  if (c0 == 'l' || c0 == 'L') return K::Line;
  return K::Character;
};

// The words `constant`, `modifierKey`, `mouseState`, `knownType` and
// `findType` denote -- distinguished by full comparison since a few share a
// leading letter (`comma`/`colon`/`commandkey`/`cmdkey`).
inline constexpr auto named_lit_of = [](std::string_view t) -> ast::NamedLitKind {
  using K = ast::NamedLitKind;
  if (ci_eq(t, "empty")) return K::Empty;
  if (ci_eq(t, "true")) return K::True;
  if (ci_eq(t, "false")) return K::False;
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

inline constexpr auto unquote_ht = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};

struct Hypertalk : cursiv::GrammarSpec {
  static constexpr auto WsBody = some(cr<R"( \t)">);
  static constexpr auto CommentBody = lit<"--"> + many(cr<R"(^\r\n)">);
  struct WS : terminal<WsBody> {};
  struct COMMENT : terminal<CommentBody> {};
  // `('|' | '¬') ws? comment? ws? newline`, entirely elided: a
  // HyperTalk line continuation.
  struct BREAK : terminal<(lit<"|"> | lit<"¬">) + option(WsBody) +
                          option(CommentBody) + option(WsBody) +
                          (lit<"\r\n"> | lit<"\n"> | lit<"\r">)> {};
  struct NEWLINE : terminal<some(cr<R"(\r\n)">)> {};

  struct ID : terminal<cr<"a-zA-Z"> + many(w)> {};

  struct STRING_LITERAL : terminal<as<unquote_ht>(
                              lit<"\""> + many(cr<R"(^"\r\n)">) + lit<"\"">)> {
  };
  struct NUMBER_LITERAL
      : terminal<as<double>((some(d) + option(lit<"."> + option(some(d)))) |
                            (lit<"."> + some(d)))> {};

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
  struct Menu;
  struct MenuItem;
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
      : rule<as<ordinal_of>(ikw<"first"> | ikw<"second"> | ikw<"third"> |
                            ikw<"fourth"> | ikw<"fifth"> | ikw<"sixth"> |
                            ikw<"seventh"> | ikw<"eighth"> | ikw<"ninth"> |
                            ikw<"tenth"> | ikw<"mid"> | ikw<"middle"> |
                            ikw<"last"> | ikw<"any">)> {};

  struct LengthWord
      : rule<as<length_of>(ikw<"english"> | ikw<"long"> | ikw<"abbreviated"> |
                           ikw<"abbrev"> | ikw<"abbr"> | ikw<"short">)> {};

  struct ChunkKindWord
      : rule<as<chunk_kind_of>(CharacterKw | WordKw | ItemKw | LineKw)> {};

  struct NamedLitWord
      : rule<as<named_lit_of>(
            ikw<"empty"> | ikw<"true"> | ikw<"false"> | ikw<"pi"> | ikw<"quote"> | ikw<"return"> |
            ikw<"space"> | ikw<"tab"> | ikw<"formfeed"> | ikw<"linefeed"> |
            ikw<"comma"> | ikw<"colon"> | ikw<"zero"> | ikw<"one"> |
            ikw<"two"> | ikw<"three"> | ikw<"four"> | ikw<"five"> | ikw<"six"> |
            ikw<"seven"> | ikw<"eight"> | ikw<"nine"> | ikw<"ten"> |
            ikw<"commandkey"> | ikw<"cmdkey"> | ikw<"optionkey"> |
            ikw<"shiftkey"> | ikw<"up"> | ikw<"down"> | ikw<"number"> |
            ikw<"integer"> | ikw<"point"> | ikw<"rect"> | ikw<"rectangle"> |
            ikw<"date"> | ikw<"logical"> | ikw<"boolean"> | ikw<"bool"> |
            ikw<"word"> | ikw<"chars"> | ikw<"whole"> | ikw<"string">)> {};

  struct HandlerName : rule<ref<ID> | CommandNameKw> {};

  // ---- Literals ---------------------------------------------------------

  struct NamedLit
      : rule<create<ast::NamedLit>(
            assign<&ast::NamedLit::kind>(ref<NamedLitWord>))> {};
  struct FindFormWord : rule<as<named_lit_of>(ikw<"word"> | ikw<"chars"> |
                                              ikw<"whole"> | ikw<"string">)> {};
  struct FindForm
      : rule<create<ast::NamedLit>(
            assign<&ast::NamedLit::kind>(ref<FindFormWord>))> {};
  struct StringLit
      : rule<create<ast::StringLit>(
            assign<&ast::StringLit::value>(ref<STRING_LITERAL>))> {};
  struct NumberLit
      : rule<create<ast::NumberLit>(
            assign<&ast::NumberLit::value>(ref<NUMBER_LITERAL>))> {};
  static constexpr auto LiteralExpr =
      ref<StringLit> | ref<NumberLit> | ref<NamedLit>;

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

  struct Menu
      : rule<create<ast::MenuRef>(
            (ikw<"menu"> + assign<&ast::MenuRef::nameExpr>(ref<Factor>))  //
            | (option(ikw<"the">) +
               assign<&ast::MenuRef::ordinal>(ref<OrdinalWord>) +
               ikw<"menu">))> {};

  struct MenuItem
      : rule<create<ast::MenuItemRef>(
            (ikw<"menuitem"> +
             assign<&ast::MenuItemRef::nameExpr>(ref<Factor>) + Of +
             assign<&ast::MenuItemRef::menu>(ref<Menu>))  //
            | (option(ikw<"the">) +
               assign<&ast::MenuItemRef::ordinal>(ref<OrdinalWord>) +
               ikw<"menuitem"> + Of +
               assign<&ast::MenuItemRef::menu>(ref<Menu>)))> {};

  // ---- Parts ----------------------------------------------------------

  static constexpr auto PositionRef =
      as<ast::Position::Next>(option(ikw<"the">) + ikw<"next">) | as<ast::Position::Prev>(option(ikw<"the">) + (ikw<"prev"> | ikw<"previous">)) | as<ast::Position::This>(ikw<"this">);

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
            | ref<Menu>                           //
            | ref<MenuItem>                        //
            | ref<Part>> {};

  // ---- Factor, unary, and the operator ladder ------------------------


  struct ToolExpr
      : rule<create<ast::ToolExpr>(
            assign<&ast::ToolExpr::kind>(
                as<ast::ToolKind::Polygon>(option(ikw<"reg"> | ikw<"regular">) + (ikw<"poly"> | ikw<"polygon">)) | as<ast::ToolKind::Rect>(option(ikw<"round">) + (ikw<"rect"> | ikw<"rectangle">)) | as<ast::ToolKind::Spray>(ikw<"spray"> + option(ikw<"can">)) |
                as<ast::ToolKind::Text>(ikw<"text">) | as<ast::ToolKind::Select>(ikw<"select">) | as<ast::ToolKind::Field>(FieldKw) |
                as<ast::ToolKind::Button>(ButtonKw) | as<ast::ToolKind::Line>(ikw<"line">))  //
            | assign<&ast::ToolExpr::expr>(ref<Expression>))> {};

  static constexpr auto EffectWord =
      as<ast::EffectKind::BarnDoorOpen>(ikw<"barn"> + ikw<"door"> + ikw<"open">) | as<ast::EffectKind::BarnDoorClose>(ikw<"barn"> + ikw<"door"> + ikw<"close">) | as<ast::EffectKind::Dissolve>(ikw<"dissolve">) |
      as<ast::EffectKind::Checkerboard>(ikw<"checkerboard">) | as<ast::EffectKind::IrisOpen>(ikw<"iris"> + ikw<"open">) | as<ast::EffectKind::IrisClose>(ikw<"iris"> + ikw<"close">) |
      as<ast::EffectKind::Plain>(ikw<"plain">) | as<ast::EffectKind::PushUp>(ikw<"push"> + ikw<"up">) | as<ast::EffectKind::PushDown>(ikw<"push"> + ikw<"down">) | as<ast::EffectKind::PushLeft>(ikw<"push"> + ikw<"left">) |
      as<ast::EffectKind::PushRight>(ikw<"push"> + ikw<"right">) | as<ast::EffectKind::ScrollDown>(ikw<"scroll"> + ikw<"down">) | as<ast::EffectKind::ScrollUp>(ikw<"scroll"> + ikw<"up">) |
      as<ast::EffectKind::ScrollLeft>(ikw<"scroll"> + ikw<"left">) | as<ast::EffectKind::ScrollRight>(ikw<"scroll"> + ikw<"right">) | as<ast::EffectKind::ShrinkToTop>(ikw<"shrink"> + ikw<"to"> + ikw<"top">) |
      as<ast::EffectKind::ShrinkToCenter>(ikw<"shrink"> + ikw<"to"> + ikw<"center">) | as<ast::EffectKind::ShrinkToBottom>(ikw<"shrink"> + ikw<"to"> + ikw<"bottom">) |
      as<ast::EffectKind::StretchFromTop>(ikw<"stretch"> + ikw<"from"> + ikw<"top">) | as<ast::EffectKind::StretchFromCenter>(ikw<"stretch"> + ikw<"from"> + ikw<"center">) |
      as<ast::EffectKind::StretchFromBottom>(ikw<"stretch"> + ikw<"from"> + ikw<"bottom">) | as<ast::EffectKind::VenetianBlinds>(ikw<"venetian"> + ikw<"blinds">) | as<ast::EffectKind::WipeUp>(ikw<"wipe"> + ikw<"up">) |
      as<ast::EffectKind::WipeDown>(ikw<"wipe"> + ikw<"down">) | as<ast::EffectKind::WipeLeft>(ikw<"wipe"> + ikw<"left">) | as<ast::EffectKind::WipeRight>(ikw<"wipe"> + ikw<"right">) |
      as<ast::EffectKind::ZoomIn>(ikw<"zoom"> + ikw<"in">) | as<ast::EffectKind::ZoomOut>(ikw<"zoom"> + ikw<"out">) | as<ast::EffectKind::ZoomOpen>(ikw<"zoom"> + ikw<"open">) | as<ast::EffectKind::ZoomClose>(ikw<"zoom"> + ikw<"close">);

  static constexpr auto SpeedWord = as<ast::SpeedKind::VeryFast>(ikw<"very"> + ikw<"fast">) | as<ast::SpeedKind::VerySlow>(ikw<"very"> + (ikw<"slow"> | ikw<"slowly">)) |
                                    as<ast::SpeedKind::Fast>(ikw<"fast">) | as<ast::SpeedKind::Slow>(ikw<"slow"> | ikw<"slowly">);

  static constexpr auto ImageWord = as<ast::ImageKind::Black>(ikw<"black">) | as<ast::ImageKind::Card>(CardKw) |
                                    as<ast::ImageKind::Gray>(ikw<"gray"> | ikw<"grey">) | as<ast::ImageKind::Inverse>(ikw<"inverse">) |
                                    as<ast::ImageKind::White>(ikw<"white">);

  struct EffectExpr
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
      ikw<"date"> | ikw<"time"> |
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
            | (assign<&ast::FuncCall::name>(ikw<"date"> | ikw<"time">) +
               lit<"("> + lit<")">) //
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
            | ref<EffectExpr>                        //
            | ref<FunctionCall>                           //
            | ref<ContainerBase>> {};

  struct Factor
      : rule<create<ast::ChunkExpr>(
                 some(append<&ast::ChunkExpr::items>(ref<ChunkItem>)) +
                 assign<&ast::ChunkExpr::target>(ref<ChunkFreeFactor>))  //
            | ref<ChunkFreeFactor>> {};


  struct UnaryExpr
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(
                     as<ast::UnOp::ThereIsNot>(ikw<"there"> + ikw<"is"> + (ikw<"no"> | (ikw<"not"> + (ikw<"a"> | ikw<"an">)))) | as<ast::UnOp::ThereIsA>(ikw<"there"> + ikw<"is"> + (ikw<"a"> | ikw<"an">)) | as<ast::UnOp::LogicalNot>(ikw<"not">) |
                     as<ast::UnOp::Negate>(lit<"-">)) +
                 assign<&ast::UnaryExpr::operand>(ref<UnaryExpr>))  //
            | ref<Factor>> {};

  struct OpGe : terminal<as<ast::BinOp::GreaterEq>(lit<">="> | lit<"≥">)> {};
  struct OpLe : terminal<as<ast::BinOp::LessEq>(lit<"<="> | lit<"≤">)> {};

  // `is`/`is not` open both the relational family below (is a/an, is
  // within, is in) and the equality family below that (is, is not); an
  // infix<> level rides the token its operators open on, so the two
  // families need their own copy of each word to sit at two levels.
  struct IsRelWord : terminal<ikw<"is">> {};
  struct IsEqWord : terminal<ikw<"is">> {};

  struct OpNotEqual : terminal<as<ast::BinOp::NotEqual>(lit<"<>"> | lit<"≠">)> {
  };

  static constexpr auto RelOps =
      as<ast::BinOp::IsNotWithin>(ref<IsRelWord> + ikw<"not"> + ikw<"within">) | as<ast::BinOp::IsWithin>(ref<IsRelWord> + ikw<"within">) | as<ast::BinOp::IsNotIn>(ref<IsRelWord> + ikw<"not"> + ikw<"in">) | as<ast::BinOp::IsIn>(ref<IsRelWord> + ikw<"in">) |
      as<ast::BinOp::IsNotA>(ref<IsRelWord> + ikw<"not"> + (ikw<"a"> | ikw<"an">)) | as<ast::BinOp::IsA>(ref<IsRelWord> + (ikw<"a"> | ikw<"an">)) | ref<OpGe> | ref<OpLe> | as<ast::BinOp::Less>(lit<"<">) |
      as<ast::BinOp::Greater>(lit<">">) | as<ast::BinOp::Contains>(ikw<"contains">);
  static constexpr auto EqOps = as<ast::BinOp::Equal>(lit<"=">) | ref<OpNotEqual> |
                                as<ast::BinOp::IsNotEqual>(ref<IsEqWord> + ikw<"not">) | as<ast::BinOp::IsEqual>(ref<IsEqWord>);

  struct Expression
      : infix<UnaryExpr, ast::BinaryExpr,
              levels(left(as<ast::BinOp::Exponent>(lit<"^">)),                       //
                     left(as<ast::BinOp::Multiply>(lit<"*">) | as<ast::BinOp::Divide>(lit<"/">) | as<ast::BinOp::Modulo>(ikw<"mod">) |
                          as<ast::BinOp::IntegerDivide>(ikw<"div">)),                   //
                     left(as<ast::BinOp::Add>(lit<"+">) | as<ast::BinOp::Subtract>(lit<"-">)),          //
                     left(as<ast::BinOp::ConcatSpace>(lit<"&&">) | as<ast::BinOp::Concat>(lit<"&">)),  //
                     left(RelOps),                           //
                     left(EqOps),                            //
                     left(as<ast::BinOp::LogicalAnd>(ikw<"and">)),                       //
                     left(as<ast::BinOp::LogicalOr>(ikw<"or">)))> {};

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

  struct SimpleExprCmd
      : rule<create<ast::SimpleExprCmd>(
          (assign<&ast::SimpleExprCmd::verb>(
               as<ast::CmdVerb::Delete>(ikw<"delete">) |
               as<ast::CmdVerb::Get>(ikw<"get">) | as<ast::CmdVerb::Visual>(ikw<"visual">)) +
           assign<&ast::SimpleExprCmd::operand>(ref<Expression>)) |
          (assign<&ast::SimpleExprCmd::verb>(as<ast::CmdVerb::Do>(ikw<"do">)) +
           assign<&ast::SimpleExprCmd::operand>(ref<Expression>) +
           option(ikw<"as"> + assign<&ast::SimpleExprCmd::language>(ref<Expression>))) |
          (assign<&ast::SimpleExprCmd::verb>(as<ast::CmdVerb::Dial>(ikw<"dial">)) +
           assign<&ast::SimpleExprCmd::operand>(ref<Expression>) +
           option(ikw<"with"> + flag<&ast::SimpleExprCmd::withModem>(ikw<"modem">) +
                  option(assign<&ast::SimpleExprCmd::modemCommands>(ref<Expression>)))) |
          (assign<&ast::SimpleExprCmd::verb>(as<ast::CmdVerb::Domenu>(ikw<"domenu">)) +
           assign<&ast::SimpleExprCmd::operand>(ref<Expression>) +
           option(lit<","> + assign<&ast::SimpleExprCmd::menuName>(ref<Expression>)) +
           option(flag<&ast::SimpleExprCmd::withoutDialog>(ikw<"without"> + ikw<"dialog">)) +
           option(ikw<"with"> + some(append<&ast::SimpleExprCmd::withKeys>(ref<Expression>), lit<",">))))> {};

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

  static constexpr auto LockTarget =
      as<ast::LockTarget::Screen>(ikw<"screen">) |
      as<ast::LockTarget::Messages>(ikw<"messages">) |
      as<ast::LockTarget::Recent>(ikw<"recent">) |
      as<ast::LockTarget::ErrorDialogs>(ikw<"error"> + ikw<"dialogs">);
  struct ScreenLockCmd
      : rule<create<ast::ScreenLockCmd>(
            (flag<&ast::ScreenLockCmd::lock>(ikw<"lock">) | ikw<"unlock">) +
            assign<&ast::ScreenLockCmd::target>(LockTarget) +
            option(ikw<"with"> + option(ikw<"visual">) +
                   assign<&ast::ScreenLockCmd::visualEffect>(ref<Expression>)))> {};

  struct AnswerCmd
      : rule<create<ast::AnswerCmd>(ikw<"answer"> +
          (((flag<&ast::AnswerCmd::file>(ikw<"file">) |
             flag<&ast::AnswerCmd::program>(ikw<"program">)) +
            assign<&ast::AnswerCmd::prompt>(ref<Expression>) +
            option(ikw<"of"> + ikw<"type"> + assign<&ast::AnswerCmd::type>(ref<Expression>))) |
           (assign<&ast::AnswerCmd::prompt>(ref<Expression>) +
            option(ikw<"with"> + append<&ast::AnswerCmd::buttons>(ref<Factor>) +
                   option(ikw<"or"> + append<&ast::AnswerCmd::buttons>(ref<Factor>) +
                          option(ikw<"or"> + append<&ast::AnswerCmd::buttons>(ref<Factor>)))))))> {};

  struct AskCmd
      : rule<create<ast::AskCmd>(
            ikw<"ask"> + option(flag<&ast::AskCmd::file>(ikw<"file">) |
                (flag<&ast::AskCmd::password>(ikw<"password">) +
                 option(flag<&ast::AskCmd::clear>(ikw<"clear">)))) +
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
            ((assign<&ast::ChooseCmd::tool>(ref<ToolExpr>) +
              option(ikw<"tool">))  //
             | (ikw<"tool"> +
                assign<&ast::ChooseCmd::tool>(ref<ToolExpr>))))> {};

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

  struct ConversionFormat
      : rule<create<ast::ConversionFormat>(
            assign<&ast::ConversionFormat::kind>(as<ast::ConversionKind::Seconds>(SecondsKw) |
                                                 as<ast::ConversionKind::DateItems>(ikw<"dateitems">))  //
            | (option(assign<&ast::ConversionFormat::length>(ref<LengthWord>)) +
               assign<&ast::ConversionFormat::kind>(as<ast::ConversionKind::Date>(ikw<"date">) |
                                                    as<ast::ConversionKind::Time>(ikw<"time">))))> {};
  struct Convertible
      : rule<create<ast::Convertible>(
            append<&ast::Convertible::formats>(ref<ConversionFormat>) +
            option(ikw<"and"> + append<&ast::Convertible::formats>(
                                     ref<ConversionFormat>)))> {};

  struct ConvertCmd
      : rule<create<ast::ConvertCmd>(
            ikw<"convert"> + assign<&ast::ConvertCmd::target>(ref<Expression>) +
            option(ikw<"from"> +
                   assign<&ast::ConvertCmd::fromFormat>(ref<Convertible>)) +
            ikw<"to"> +
            assign<&ast::ConvertCmd::toFormat>(ref<Convertible>))> {};

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
            option(assign<&ast::FindCmd::style>(ref<FindForm>)) +
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


  struct GoDestination
      : rule<ref<Expression> |
             create<ast::CardPartRef>(assign<&ast::CardPartRef::ordinal>(ref<OrdinalWord>)) |
             create<ast::CardPartRef>(assign<&ast::CardPartRef::position>(PositionRef))> {};

  struct GoCmd
      : rule<create<ast::GoCmd>(
            ikw<"go"> +
            ((flag<&ast::GoCmd::back>(ikw<"back">) +
              option(ikw<"with"> + ikw<"visual"> +
                     assign<&ast::GoCmd::visualEffect>(ref<Expression>)))  //
             | (option(ikw<"to">) +
                assign<&ast::GoCmd::destination>(ref<GoDestination>) +
                ((ikw<"with"> + ikw<"visual"> +
                  assign<&ast::GoCmd::visualEffect>(ref<Expression>))  //
                 | option(assign<&ast::GoCmd::remoteOpt>(
                       as<ast::RemoteNav::NewWindowNoDialog>(ikw<"in"> + ikw<"a"> + ikw<"new"> + ikw<"window"> + ikw<"without"> + ikw<"dialog">) | as<ast::RemoteNav::NewWindow>(ikw<"in"> + ikw<"a"> + ikw<"new"> + ikw<"window">) |
                       as<ast::RemoteNav::NoDialog>(ikw<"without"> + ikw<"dialog">)))))))> {};

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

  struct MusicExpr
      : rule<create<ast::MusicExpr>(
            assign<&ast::MusicExpr::voice>(ref<Expression>) +
            option(ikw<"tempo"> +
                   assign<&ast::MusicExpr::tempo>(ref<Expression>)) +
            option(assign<&ast::MusicExpr::notes>(ref<Expression>)))> {};

  struct PlayCmd
      : rule<create<ast::PlayCmd>(
            ikw<"play"> +
            assign<&ast::PlayCmd::music>(ref<MusicExpr>))> {};

  struct PopCardCmd
      : rule<create<ast::PopCardCmd>(ikw<"pop"> + CardKw +
             option(assign<&ast::PopCardCmd::prep>(
                        as<ast::Preposition::Into>(ikw<"into">) |
                        as<ast::Preposition::Before>(ikw<"before">) |
                        as<ast::Preposition::After>(ikw<"after">)) +
                    assign<&ast::PopCardCmd::target>(ref<Expression>)))> {};

  struct PushCmd
      : rule<create<ast::PushCmd>(
            ikw<"push"> + (flag<&ast::PushCmd::card>(CardKw) + &(ref<NEWLINE> | eof) //
                          | assign<&ast::PushCmd::value>(ref<Expression>)))> {
  };

  struct PutCmd
      : rule<create<ast::PutCmd>(
            ikw<"put"> +
            some(append<&ast::PutCmd::values>(ref<Expression>), lit<",">) +
            option(assign<&ast::PutCmd::prep>(as<ast::Preposition::Before>(ikw<"before">) | as<ast::Preposition::After>(ikw<"after">) |
                                              as<ast::Preposition::Into>(ikw<"into">)) +
                   assign<&ast::PutCmd::target>(ref<Expression>)))> {};

  struct ReadFileCmd
      : rule<create<ast::ReadFileCmd>(
            ikw<"read"> + ikw<"from"> + ikw<"file"> +
            assign<&ast::ReadFileCmd::file>(ref<Expression>) +
            option(ikw<"at"> +
                   assign<&ast::ReadFileCmd::atExpr>(ref<Expression>)) +
            option((ikw<"until"> +
                    assign<&ast::ReadFileCmd::untilExpr>(ref<Expression>)) |
                   (ikw<"for"> +
                    assign<&ast::ReadFileCmd::forExpr>(ref<Expression>))))> {};

  struct ResetCmd
      : rule<create<ast::ResetCmd>(
            ikw<"reset"> +
            ((option(ikw<"the">) + ikw<"menubar">)  //
             | flag<&ast::ResetCmd::paint>(ikw<"paint">) |
             flag<&ast::ResetCmd::printing>(ikw<"printing">)))> {};

  struct SelectCmd
      : rule<create<ast::SelectCmd>(
            ikw<"select"> +
            (flag<&ast::SelectCmd::empty>(ikw<"empty">)  //
             | (option(assign<&ast::SelectCmd::pos>(as<ast::BeforeAfterKind::Before>(ikw<"before">) |
                                                    as<ast::BeforeAfterKind::After>(ikw<"after">))) +
                ((flag<&ast::SelectCmd::text>(ikw<"text">) + Of +
                  assign<&ast::SelectCmd::target>(ref<Expression>))  //
                 | assign<&ast::SelectCmd::target>(ref<Expression>)))))> {};

  struct PropertyValue
      : rule<create<ast::PropertyValue>(
            assign<&ast::PropertyValue::kw>(as<ast::PropValueKw::Plain>(ikw<"plain">) | as<ast::PropValueKw::Menu>(ikw<"menu">) |
                                            as<ast::PropValueKw::Bottom>(ikw<"bottom">) | as<ast::PropValueKw::Left>(ikw<"left">) |
                                            as<ast::PropValueKw::Right>(ikw<"right">) | as<ast::PropValueKw::Top>(ikw<"top">) |
                                            as<ast::PropValueKw::Center>(ikw<"center">))  //
            | some(append<&ast::PropertyValue::exprs>(ref<Expression>),
                   lit<",">))> {};

  struct SetCmd
      : rule<create<ast::SetCmd>(
            ikw<"set"> + assign<&ast::SetCmd::property>(ref<Property>) +
            ikw<"to"> +
            assign<&ast::SetCmd::value>(ref<PropertyValue>))> {};

  struct SendCmd
      : rule<create<ast::SendCmd>(
            ikw<"send"> +
            some(append<&ast::SendCmd::message>(ref<Expression>), lit<",">) +
            option(ikw<"to"> + assign<&ast::SendCmd::target>(ref<Expression>)))> {};

  static constexpr auto PictureKw = ikw<"picture"> | ikw<"pict">;

  struct ShowHideCmd
      : rule<create<ast::ShowHideCmd>(
            (flag<&ast::ShowHideCmd::show>(ikw<"show">) |
             ikw<"hide">) +
            ((flag<&ast::ShowHideCmd::allCards>(ikw<"all">) + CardsKw) |
             (flag<&ast::ShowHideCmd::markedCards>(ikw<"marked">) + CardsKw) |
             (assign<&ast::ShowHideCmd::cardCount>(ref<Expression>) + CardsKw) |
             flag<&ast::ShowHideCmd::titlebar>(ikw<"titlebar">)  //
             | (flag<&ast::ShowHideCmd::card>(CardKw) +
                flag<&ast::ShowHideCmd::picture>(PictureKw))  //
             | (flag<&ast::ShowHideCmd::background>(BackgroundKw) +
                flag<&ast::ShowHideCmd::picture>(PictureKw))  //
             | (flag<&ast::ShowHideCmd::picture>(PictureKw) + Of +
                assign<&ast::ShowHideCmd::target>(ref<Expression>))  //
             | assign<&ast::ShowHideCmd::target>(ref<Expression>)) +
            option(ikw<"at"> + some(append<&ast::ShowHideCmd::at>(ref<Expression>), lit<",">)))> {};

  static constexpr auto SortDirOpt =
      option(assign<&ast::SortCmd::direction>(as<ast::Ordering::Ascending>(ikw<"ascending">) | as<ast::Ordering::Descending>(ikw<"descending">)));
  static constexpr auto SortStyleOpt =
      option(assign<&ast::SortCmd::style>(as<ast::SortStyleKind::Text>(ikw<"text">) | as<ast::SortStyleKind::Numeric>(ikw<"numeric">) |
                                          as<ast::SortStyleKind::International>(ikw<"international">) | as<ast::SortStyleKind::Datetime>(ikw<"datetime">)));

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

  struct SpeakCmd
      : rule<create<ast::SpeakCmd>(
            ikw<"speak"> + assign<&ast::SpeakCmd::text>(ref<Expression>) +
            option(ikw<"with"> +
                   ((assign<&ast::SpeakCmd::gender>(
                         as<ast::Gender::Male>(ikw<"male">) | as<ast::Gender::Female>(ikw<"female">) |
                         as<ast::Gender::Neuter>(ikw<"neuter">) | as<ast::Gender::Robotic>(ikw<"robotic">)) +
                     ikw<"voice">)  //
                    | (ikw<"voice"> + assign<&ast::SpeakCmd::voiceExpr>(
                                          ref<Expression>)))))> {};

  struct TypeCmd
      : rule<create<ast::TypeCmd>(
            ikw<"type"> + assign<&ast::TypeCmd::text>(ref<Expression>) +
            option(ikw<"with"> +
                   flag<&ast::TypeCmd::withCommandKey>(
                       ikw<"commandkey"> | ikw<"cmdkey">)))> {};

  struct WaitCmd
      : rule<create<ast::WaitCmd>(
            ikw<"wait"> +
            ((ikw<"until"> + assign<&ast::WaitCmd::cond>(ref<Expression>))  //
             | (flag<&ast::WaitCmd::isWhile>(ikw<"while">) +
                assign<&ast::WaitCmd::cond>(ref<Expression>))  //
             | (option(ikw<"for">) +
                assign<&ast::WaitCmd::amount>(ref<Expression>) +
                option(assign<&ast::WaitCmd::unit>(as<ast::TimeUnitKind::Ticks>(ikw<"ticks"> | ikw<"tick">) |
                                            as<ast::TimeUnitKind::Seconds>(SecondsKw))))))> {};

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

  struct Handler
      : rule<create<ast::Handler>(
            ikw<"on"> + assign<&ast::Handler::name>(ref<HandlerName>) +
            option(ParamList<&ast::Handler::params>) + some(ref<NEWLINE>) +
            option(StatementListBody<&ast::Handler::body>) + ikw<"end"> +
            assign<&ast::Handler::endName>(ref<HandlerName>))> {};

  struct Function
      : rule<create<ast::Function>(
            ikw<"function"> + assign<&ast::Function::name>(ref<ID>) +
            option(ParamList<&ast::Function::params>) + some(ref<NEWLINE>) +
            option(StatementListBody<&ast::Function::body>) + ikw<"end"> +
            assign<&ast::Function::endName>(ref<ID>))> {};

  struct TopItem
      : rule<create<ast::TopItem>(
            assign<&ast::TopItem::handler>(ref<Handler>))  //
            | create<ast::TopItem>(
                  assign<&ast::TopItem::function>(ref<Function>))  //
            | create<ast::TopItem>(
                  assign<&ast::TopItem::statement>(ref<Statement>))> {};

  // HyperTalk's reference grammar has two start rules -- a script of
  // handlers/functions, a scriptlet of bare statements -- and cursiv has
  // one entry; Document unions both, freely interleaved.
  struct Document
      : rule<create<ast::Document>(
            many(ref<NEWLINE>) +
            many(append<&ast::Document::items>(ref<TopItem>) +
                 many(ref<NEWLINE>)))> {};

  using entry = Document;
  using ignored = list<WS, BREAK>;
  using hidden = list<COMMENT>;
};

}  // namespace hypertalk
