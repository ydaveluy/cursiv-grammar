// The HyperTalk language: its AST. Plain structs -- the grammar names their
// members, and the generated builder fills them.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace hypertalk::ast {

// Expression family: every operand and operator result is an Expr.
struct Expr : cursiv::Extends<cursiv::AstNode> {};

enum class NamedLitKind : std::uint8_t {
  Empty, Pi, Quote, ReturnChar, Space, Tab, Formfeed, Linefeed, Comma, Colon,
  Zero, One, Two, Three, Four, Five, Six, Seven, Eight, Nine, Ten,
  CommandKey, OptionKey, ShiftKey,
  MouseUp, MouseDown,
  NumberType, IntegerType, PointType, RectType, DateType, LogicalType,
  FindWord, FindChars, FindWhole, FindString,
};

struct NumberLit : cursiv::Extends<Expr> {
  float64 value;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view value;
};

// A closed keyword-token used as a value in its own right: `empty`, `pi`,
// `up`, `commandkey`, `integer`... -- the spelling carries no other data.
struct NamedLit : cursiv::Extends<Expr> {
  NamedLitKind kind;
};

// A bare name: a variable, or a message sent with no arguments read as a
// value.
struct NameRef : cursiv::Extends<Expr> {
  string_view name;
};

struct SelectionRef : cursiv::Extends<Expr> {};
struct TargetRef : cursiv::Extends<Expr> {};

enum class Length : std::uint8_t { English, Abbreviated, Short };

// `the [length] propertyName [of factor]` -- `ofTarget` null is the global
// form.
struct PropertyRef : cursiv::Extends<Expr> {
  string_view name;
  optional<Length> length;
  Expr* ofTarget;
};

enum class Ordinal : std::uint8_t {
  First, Second, Third, Fourth, Fifth, Sixth, Seventh, Eighth, Ninth, Tenth,
  Mid, Last, Any,
};

struct MenuRef : cursiv::Extends<Expr> {
  optional<Ordinal> ordinal;
  Expr* nameExpr;
};

struct MenuItemRef : cursiv::Extends<Expr> {
  optional<Ordinal> ordinal;
  Expr* nameExpr;
  MenuRef* menu;
};

// `the message box`, `the msg window` -- a fixed target, no data of its own.
struct MessageBoxRef : cursiv::Extends<Expr> {};

// `me` inside a handler.
struct MeRef : cursiv::Extends<Expr> {};

// `card part 3`, `background part x` -- a part addressed only by number,
// neither a button nor a field specifically.
struct GenericPartRef : cursiv::Extends<Expr> {
  boolean background;  // false: card
  Expr* index;
};

struct StackPartRef : cursiv::Extends<Expr> {
  Expr* nameExpr;  // null: "this stack" / bare "stack"
};

enum class Position : std::uint8_t { Next, Prev, This };

struct BkgndPartRef : cursiv::Extends<Expr> {
  optional<Ordinal> ordinal;
  optional<Position> position;
  boolean byId;
  Expr* expr;  // id-expr, name-expr, or null for a bare/ordinal/positional form
  StackPartRef* ofStack;
};

// A bare `card`/`ordinal card` defaults to this scope; only an explicit
// `background` sets that scope instead.
struct CardPartRef : cursiv::Extends<Expr> {
  optional<Ordinal> ordinal;
  optional<Position> position;
  boolean byId;
  Expr* expr;
  BkgndPartRef* ofBkgnd;
  StackPartRef* ofStackDirect;
};

// `background`/`ordinal background` defaults to this scope; only an
// explicit `card` sets card scope (opposite default from a field).
struct ButtonPartRef : cursiv::Extends<Expr> {
  boolean background;
  optional<Ordinal> ordinal;
  boolean byId;
  Expr* expr;
  CardPartRef* ofCard;
};

// A bare `field` defaults to background scope; only an explicit `card` sets
// card scope (opposite default from a button).
struct FieldPartRef : cursiv::Extends<Expr> {
  boolean card;
  optional<Ordinal> ordinal;
  boolean byId;
  Expr* expr;
  CardPartRef* ofCard;
};

struct WindowPartRef : cursiv::Extends<Expr> {
  string_view kind;  // "card window", "tool window", ... -- fixed phrases
  Expr* expr;        // set only for `window expr` / `window id expr`
  boolean byId;
};

enum class ChunkKind : std::uint8_t { Character, Word, Item, Line };

// `word 2`, `char 3 to 5`, `any line` -- one level of a chunk expression.
struct ChunkItem : cursiv::Extends<cursiv::AstNode> {
  ChunkKind kind;
  optional<Ordinal> ordinal;
  Expr* start;  // set when ordinal is not
  Expr* end;    // range form only
};

// `word 2 of line 3 of field "x"` -- items outermost first, wrapping target.
struct ChunkExpr : cursiv::Extends<Expr> {
  vector<ChunkItem*> items;
  Expr* target;
};

enum class UnOp : std::uint8_t { Negate, LogicalNot, ThereIsA, ThereIsNot };

struct UnaryExpr : cursiv::Extends<Expr> {
  UnOp op;
  Expr* operand;
};

enum class BinOp : std::uint8_t {
  Exponent, Multiply, Divide, IntegerDivide, Modulo, Add, Subtract, ConcatSpace, Concat,
  GreaterEq, LessEq, Less, Greater, Contains,
  IsIn, IsNotIn, IsA, IsNotA, IsWithin, IsNotWithin, IsEqual, IsNotEqual,
  Equal, NotEqual, LogicalAnd, LogicalOr,
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  BinOp op;
  Expr* right;
};

enum class ToolKind : std::uint8_t {
  Text, Select, Field, Button, Line, Polygon, Rect, Spray,
};

struct ToolExpr : cursiv::Extends<cursiv::AstNode> {
  optional<ToolKind> kind;
  Expr* expr;
};

enum class EffectKind : std::uint8_t {
  Dissolve, BarnDoorOpen, BarnDoorClose, Checkerboard, IrisOpen, IrisClose,
  Plain, PushUp, PushDown, PushLeft, PushRight, ScrollDown, ScrollUp,
  ScrollLeft, ScrollRight, ShrinkToTop, ShrinkToCenter, ShrinkToBottom,
  StretchFromTop, StretchFromCenter, StretchFromBottom, VenetianBlinds,
  WipeUp, WipeDown, WipeLeft, WipeRight, ZoomIn, ZoomOut, ZoomOpen, ZoomClose,
};
enum class SpeedKind : std::uint8_t { Fast, Slow, VeryFast, VerySlow };
enum class ImageKind : std::uint8_t { Black, Card, Gray, Inverse, White };

struct EffectExpr : cursiv::Extends<Expr> {
  EffectKind effect;
  optional<SpeedKind> speed;
  optional<ImageKind> image;
};

struct MusicExpr : cursiv::Extends<cursiv::AstNode> {
  Expr* voice;
  Expr* tempo;
  Expr* notes;
};

struct FuncCall : cursiv::Extends<Expr> {
  string_view name;
  vector<Expr*> args;
  Expr* ofTarget;  // the `singleArgFunc of factor` form
};

enum class ConversionKind : std::uint8_t { Seconds, DateItems, Date, Time };

struct ConversionFormat : cursiv::Extends<cursiv::AstNode> {
  ConversionKind kind;
  optional<Length> length;
};

struct Convertible : cursiv::Extends<cursiv::AstNode> {
  vector<ConversionFormat*> formats;
};

// ---- Statements -----------------------------------------------------

struct Stmt : cursiv::Extends<cursiv::AstNode> {};

struct ExprStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct MessageStmt : cursiv::Extends<Stmt> {
  string_view name;
  vector<Expr*> args;
};

struct GlobalStmt : cursiv::Extends<Stmt> {
  vector<string_view> names;
};

struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct IfStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  vector<Stmt*> thenBody;
  vector<Stmt*> elseBody;
  boolean hasElse;
};

// The shape is told by which fields a reader finds set: `cond` means a
// duration loop, `loopVar` a counted range, `count`/`hasTimes` a plain
// count, and none of those a bare `forever`.
struct RepeatStmt : cursiv::Extends<Stmt> {
  Expr* count;           // Count
  boolean hasTimes;       // Count: trailing `times` written
  Expr* cond;             // Until / While
  boolean isWhile;        // Duration: false is `until`, true is `while`
  string_view loopVar;    // With
  Expr* rangeStart;       // With
  Expr* rangeEnd;         // With
  boolean downTo;         // With: `down to` rather than `to`
  vector<Stmt*> body;
};

enum class CmdVerb : std::uint8_t { Delete, Dial, Domenu, Do, Get, Visual };

struct SimpleExprCmd : cursiv::Extends<Stmt> {
  CmdVerb verb;
  Expr* operand;
};

struct AddSubCmd : cursiv::Extends<Stmt> {
  boolean isAdd;  // false: subtract
  Expr* amount;
  Expr* target;
};

struct MulDivCmd : cursiv::Extends<Stmt> {
  boolean isMultiply;  // false: divide
  Expr* target;
  Expr* amount;
};

struct EnableCmd : cursiv::Extends<Stmt> {
  boolean enable;  // false: disable
  Expr* target;
};

struct ScreenLockCmd : cursiv::Extends<Stmt> {
  boolean lock;  // false: unlock
  Expr* visualEffect;
};

struct AnswerCmd : cursiv::Extends<Stmt> {
  Expr* prompt;
  vector<Expr*> buttons;  // 0-3, via optional `with ... or ... or ...`
};

struct AskCmd : cursiv::Extends<Stmt> {
  boolean file;
  Expr* prompt;
  Expr* defaultAnswer;
};

struct BeepCmd : cursiv::Extends<Stmt> {
  Expr* count;
};

struct ChooseCmd : cursiv::Extends<Stmt> {
  ToolExpr* tool;
};

struct ClickCmd : cursiv::Extends<Stmt> {
  vector<Expr*> at;
  vector<Expr*> withKeys;
};

struct CloseFileCmd : cursiv::Extends<Stmt> {
  Expr* file;
};

struct ConvertCmd : cursiv::Extends<Stmt> {
  Expr* target;
  Convertible* fromFormat;
  Convertible* toFormat;
};

struct CreateMenuCmd : cursiv::Extends<Stmt> {
  Expr* name;
};

struct DragCmd : cursiv::Extends<Stmt> {
  vector<Expr*> from;
  vector<Expr*> to;
  vector<Expr*> withKeys;
};

struct EditScriptCmd : cursiv::Extends<Stmt> {
  Expr* target;
};

// `name` empty and `toHyperCard` false means `exit repeat`.
struct ExitCmd : cursiv::Extends<Stmt> {
  string_view name;
  boolean toHyperCard;
};

struct ExportPaintCmd : cursiv::Extends<Stmt> {
  Expr* file;
};

struct FindCmd : cursiv::Extends<Stmt> {
  Expr* style;
  boolean international;
  Expr* what;
  Expr* field;
  boolean markedCardsOnly;
};

enum class RemoteNav : std::uint8_t { NewWindow, NewWindowNoDialog, NoDialog };

struct GoCmd : cursiv::Extends<Stmt> {
  boolean back;
  Expr* destination;
  Expr* visualEffect;
  optional<RemoteNav> remoteOpt;
};

struct ImportPaintCmd : cursiv::Extends<Stmt> {
  Expr* file;
};

struct NextRepeatCmd : cursiv::Extends<Stmt> {};

struct OpenFileCmd : cursiv::Extends<Stmt> {
  Expr* file;
};

struct PassCmd : cursiv::Extends<Stmt> {
  string_view name;
};

struct PlayCmd : cursiv::Extends<Stmt> {
  MusicExpr* music;
};

struct PopCardCmd : cursiv::Extends<Stmt> {};

struct PushCmd : cursiv::Extends<Stmt> {
  boolean card;  // false: `push expression`
  Expr* value;
};

enum class Preposition : std::uint8_t { Before, After, Into };

struct PutCmd : cursiv::Extends<Stmt> {
  vector<Expr*> values;
  optional<Preposition> prep;
  Expr* target;
};

struct ReadFileCmd : cursiv::Extends<Stmt> {
  Expr* file;
  Expr* untilExpr;
  Expr* atExpr;
  Expr* forExpr;
};

struct ResetCmd : cursiv::Extends<Stmt> {
  boolean paint;  // false: menubar
};

enum class BeforeAfterKind : std::uint8_t { Before, After };

struct SelectCmd : cursiv::Extends<Stmt> {
  boolean empty;
  boolean text;
  optional<BeforeAfterKind> pos;
  Expr* target;
};

enum class PropValueKw : std::uint8_t { Plain, Menu, Bottom, Left, Right, Top, Center };

struct PropertyValue : cursiv::Extends<cursiv::AstNode> {
  optional<PropValueKw> kw;
  vector<Expr*> exprs;
};

struct SetCmd : cursiv::Extends<Stmt> {
  PropertyRef* property;
  PropertyValue* value;
};

struct SendCmd : cursiv::Extends<Stmt> {
  vector<Expr*> message;
  Expr* target;
};

// `titlebar` alone, or `picture` with `card`/`background` (no target) or with
// a `target` (the `picture of` form), or a bare `target` with `picture`
// unset.
struct ShowHideCmd : cursiv::Extends<Stmt> {
  boolean show;  // false: hide
  boolean titlebar;
  boolean picture;
  boolean card;
  boolean background;
  Expr* target;
};

enum class SortStyleKind : std::uint8_t { Text, Numeric, International, Datetime };
enum class Ordering : std::uint8_t { Ascending, Descending };

// `cards` covers both the `sort ... cards` and the bare `sort stack` forms
// (sorting the stack's card order); `target` set means an explicit scope
// expression rather than the implicit current stack.
struct SortCmd : cursiv::Extends<Stmt> {
  optional<ChunkKind> chunkType;
  boolean cards;
  boolean marked;
  Expr* target;
  optional<Ordering> direction;
  optional<SortStyleKind> style;
  Expr* byExpr;
};

enum class Gender : std::uint8_t { Male, Female, Neuter, Robotic };

struct SpeakCmd : cursiv::Extends<Stmt> {
  Expr* text;
  optional<Gender> gender;
  Expr* voiceExpr;
};

struct TypeCmd : cursiv::Extends<Stmt> {
  Expr* text;
  boolean withCommandKey;
};

enum class TimeUnitKind : std::uint8_t { Ticks, Seconds };

// The shape is told by which fields a reader finds set: `amount` a
// duration, `cond` an until/while (told apart by `isWhile`).
struct WaitCmd : cursiv::Extends<Stmt> {
  Expr* amount;
  optional<TimeUnitKind> unit;
  Expr* cond;
  boolean isWhile;
};

struct WriteFileCmd : cursiv::Extends<Stmt> {
  Expr* text;
  Expr* file;
  Expr* atExpr;  // set for `at expr`
  boolean atEof;  // set for `at eof` / `at end`
};

// ---- Top level --------------------------------------------------------

struct Handler : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view endName;
  vector<string_view> params;
  vector<Stmt*> body;
};

struct Function : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view endName;
  vector<string_view> params;
  vector<Stmt*> body;
};

// A cursiv document has one entry rule; HyperTalk has two (a script of
// handlers/functions, a scriptlet of bare statements), so Document unions
// both -- an item is one or the other, freely interleaved.
struct TopItem : cursiv::Extends<cursiv::AstNode> {
  Handler* handler;
  Function* function;
  Stmt* statement;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<TopItem*> items;
};

}  // namespace hypertalk::ast
