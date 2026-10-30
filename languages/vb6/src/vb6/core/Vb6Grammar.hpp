// The vb6 language: its grammar. Whitespace, line continuation and comments
// are trivia; a newline or a bare `:` are both a statement separator (`Eos`),
// since VB6 statements are line-oriented but `:` chains several on one line.
#pragma once

#include <cstddef>
#include <memory_resource>
#include <span>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "vb6/core/ast.hpp"

namespace vb6 {

inline constexpr auto undouble_dq = [](std::string_view t,
                                        std::pmr::memory_resource* mem) {
  const std::string_view body = t.substr(1, t.size() - 2);
  if (body.find('"') == std::string_view::npos) return body;
  const std::span<char> room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    room[n++] = body[i];
    if (body[i] == '"') ++i;
  }
  return std::string_view(room.data(), n);
};

inline constexpr auto label_name = [](std::string_view t,
                                      std::pmr::memory_resource*) {
  return t.substr(0, t.front() == '[' ? t.find(']') + 1
                                      : t.find_first_of(" \t:"));
};

struct Vb6 : cursiv::GrammarSpec {
  // -- Trivia -----------------------------------------------------------

  struct WS : terminal<some(cr<R"( \t)">)> {};

  struct LINE_CONTINUATION
      : terminal<some(cr<R"( \t)">) + lit<"_"> + option(some(cr<R"( \t)">)) +
                 (lit<"\r\n"> | lit<"\n"> | lit<"\r">)> {};

  struct COMMENT : terminal<lit<"'"> + many(cr<R"(^\r\n)">)> {};

  // The introducer glues to its body with no boundary of its own: spelled as
  // one terminal, like a raw string's opener (`REM` followed directly by a
  // space, or alone; `REMhello` stays an identifier).
  struct REMCOMMENT
      : terminal<ikw<"REM"> + option(lit<" "> + many(cr<R"(^\r\n)">))> {};

  struct NEWLINE : terminal<some(cr<R"(\r\n)">)> {};

  // A statement ends at a newline or a bare `:`; both compose into one
  // separator so a colon-chained line (`a = 1: b = 2`) and a label's own
  // trailing colon (`Label:`) need no separate lexer trick for the two.
  static constexpr auto Eos = some(ref<NEWLINE> | lit<":">);
  static constexpr auto BlankLines = many(ref<NEWLINE>);

  // -- Identifiers and keywords -------------------------------------------

  static constexpr auto LetterClass =
      cr<"A-Za-z_ÄÖÜÁÉÍÓÚÂÊÎÔÛÀÈÌÒÙÃẼĨÕŨÇäöüáéíóúâêîôûàèìòùãẽĩõũç">;
  static constexpr auto LetterOrDigitClass =
      cr<"A-Za-z0-9_ÄÖÜÁÉÍÓÚÂÊÎÔÛÀÈÌÒÙÃẼĨÕŨÇäöüáéíóúâêîôûàèìòùãẽĩõũç">;
  static constexpr auto word_boundary = LetterOrDigitClass;

  // MS-VBAL 3.3.5.2's reserved identifiers, but VBA 7's words (`LongPtr`,
  // `CLngLng`, ...) and the `VB_*` attribute names, which VB6 does not
  // reserve, and `Date`, a name a corpus document declares.
  static constexpr auto StatementKeyword =
      ikw<"CALL"> | ikw<"CASE"> | ikw<"CLOSE"> | ikw<"CONST"> |
      ikw<"DECLARE"> | ikw<"DEFBOOL"> | ikw<"DEFBYTE"> | ikw<"DEFCUR"> |
      ikw<"DEFDATE"> | ikw<"DEFDBL"> | ikw<"DEFINT"> | ikw<"DEFLNG"> |
      ikw<"DEFOBJ"> | ikw<"DEFSNG"> | ikw<"DEFSTR"> | ikw<"DEFVAR"> |
      ikw<"DIM"> | ikw<"DO"> | ikw<"ELSE"> | ikw<"ELSEIF"> | ikw<"END"> |
      ikw<"ENDIF"> | ikw<"ENUM"> | ikw<"ERASE"> | ikw<"EVENT"> |
      ikw<"EXIT"> | ikw<"FOR"> | ikw<"FRIEND"> | ikw<"FUNCTION"> |
      ikw<"GET"> | ikw<"GLOBAL"> | ikw<"GOSUB"> | ikw<"GOTO"> | ikw<"IF"> |
      ikw<"IMPLEMENTS"> | ikw<"INPUT"> | ikw<"LET"> | ikw<"LOCK"> |
      ikw<"LOOP"> | ikw<"LSET"> | ikw<"NEXT"> | ikw<"ON"> | ikw<"OPEN"> |
      ikw<"OPTION"> | ikw<"PRINT"> | ikw<"PRIVATE"> | ikw<"PUBLIC"> |
      ikw<"PUT"> | ikw<"RAISEEVENT"> | ikw<"REDIM"> | ikw<"RESUME"> |
      ikw<"RETURN"> | ikw<"RSET"> | ikw<"SEEK"> | ikw<"SELECT"> |
      ikw<"SET"> | ikw<"STATIC"> | ikw<"STOP"> | ikw<"SUB"> | ikw<"TYPE"> |
      ikw<"UNLOCK"> | ikw<"WEND"> | ikw<"WHILE"> | ikw<"WITH"> |
      ikw<"WRITE">;

  static constexpr auto MarkerKeyword =
      ikw<"ANY"> | ikw<"AS"> | ikw<"BYREF"> | ikw<"BYVAL"> | ikw<"EACH"> |
      ikw<"IN"> | ikw<"NEW"> | ikw<"SHARED"> | ikw<"UNTIL"> |
      ikw<"WITHEVENTS"> | ikw<"OPTIONAL"> | ikw<"PARAMARRAY"> |
      ikw<"PRESERVE"> | ikw<"SPC"> | ikw<"TAB"> | ikw<"THEN"> | ikw<"TO">;

  static constexpr auto OperatorIdentifier =
      ikw<"ADDRESSOF"> | ikw<"AND"> | ikw<"EQV"> | ikw<"IMP"> | ikw<"IS"> |
      ikw<"LIKE"> | ikw<"MOD"> | ikw<"NOT"> | ikw<"OR"> | ikw<"TYPEOF"> |
      ikw<"XOR">;

  // Reserved, yet a name in an expression (MS-VBAL 5.6.10), special forms
  // included.
  static constexpr auto ReservedName =
      ikw<"ABS"> | ikw<"CBOOL"> | ikw<"CBYTE"> | ikw<"CCUR"> | ikw<"CDATE"> |
      ikw<"CDBL"> | ikw<"CDEC"> | ikw<"CINT"> | ikw<"CLNG"> | ikw<"CSNG"> |
      ikw<"CSTR"> | ikw<"CVAR"> | ikw<"CVERR"> | ikw<"DEBUG"> |
      ikw<"DOEVENTS"> | ikw<"FIX"> | ikw<"INT"> | ikw<"LEN"> | ikw<"LENB"> |
      ikw<"ME"> | ikw<"PSET"> | ikw<"SCALE"> | ikw<"SGN"> | ikw<"STRING"> |
      ikw<"ARRAY"> | ikw<"CIRCLE"> | ikw<"INPUTB"> | ikw<"LBOUND"> |
      ikw<"UBOUND">;

  static constexpr auto ReservedTypeIdentifier =
      ikw<"BOOLEAN"> | ikw<"BYTE"> | ikw<"CURRENCY"> | ikw<"DOUBLE"> |
      ikw<"INTEGER"> | ikw<"LONG"> | ikw<"SINGLE"> | ikw<"STRING"> |
      ikw<"VARIANT">;

  static constexpr auto LiteralIdentifier =
      ikw<"TRUE"> | ikw<"FALSE"> | ikw<"NOTHING"> | ikw<"EMPTY"> | ikw<"NULL">;

  static constexpr auto ReservedWord =
      StatementKeyword | MarkerKeyword | OperatorIdentifier | ReservedName |
      ReservedTypeIdentifier | LiteralIdentifier | ikw<"REM"> |
      ikw<"ATTRIBUTE"> | ikw<"CDECL"> | ikw<"DECIMAL"> | ikw<"DEFDEC">;

  struct ID : terminal<LetterClass + many(LetterOrDigitClass)> {};

  // A name and its `:` in one token, so a label never forks with a call: an
  // `IDENTIFIER`, never `Randomize` (MS-VBAL 5.4.1.1).
  struct LABEL : terminal<as<label_name>(
                     !ikw<"RANDOMIZE"> +
                     ((LetterClass + many(LetterOrDigitClass)) |
                      (lit<"["> + many(cr<R"(^\r\n])">) + lit<"]">)) +
                     many(cr<R"( \t)">) + lit<":", !cr<"=">>)> {};

  // The words a rule spells that stay plain names: spelled once, so a state
  // reading a name accepts the keyword's token as text.
  static constexpr auto SoftKeyword =
      ikw<"ACCESS"> | ikw<"ALIAS"> | ikw<"APPEND"> | ikw<"BINARY"> |
      ikw<"CLASS"> | ikw<"COLLECTION"> | ikw<"LIB"> | ikw<"MID"> |
      ikw<"NAME"> | ikw<"OBJECT"> | ikw<"OUTPUT"> | ikw<"RANDOM"> |
      ikw<"READ"> | ikw<"STEP"> | ikw<"TEXT"> | ikw<"VERSION"> |
      ikw<"WIDTH">;

  // Plain names too, but each opens a statement or a block of its own: at a
  // statement's head they never call nor assign.
  static constexpr auto StatementWord =
      ikw<"APPACTIVATE"> | ikw<"BEEP"> | ikw<"BEGIN"> | ikw<"CHDIR"> |
      ikw<"CHDRIVE"> | ikw<"DELETESETTING"> | ikw<"ERROR"> |
      ikw<"FILECOPY"> | ikw<"KILL"> | ikw<"LOAD"> | ikw<"MKDIR"> |
      ikw<"RANDOMIZE"> | ikw<"RESET"> | ikw<"RMDIR"> | ikw<"SAVEPICTURE"> |
      ikw<"SAVESETTING"> | ikw<"SENDKEYS"> | ikw<"SETATTR"> | ikw<"TIME"> |
      ikw<"UNLOAD">;

  // Keep the complete foreign-name token, including internal spaces and
  // punctuation; a data-rule token join would lose those spaces.
  struct BracketedName
      : terminal<lit<"["> + some(cr<R"(^\r\n])">) + lit<"]">> {};

  static constexpr auto NameText =
      ref<ID> | SoftKeyword | StatementWord | ref<BracketedName>;

  // A name after `.` or `!`: any word (MS-VBAL `unrestricted-name`).
  static constexpr auto UnrestrictedNameText = NameText | ReservedWord;

  // `Empty` reads as a name: no node of its own.
  static constexpr auto ExprNameText =
      NameText | ReservedName | ikw<"DATE"> | ikw<"INPUT"> | ikw<"EMPTY">;

  // `Date` and `Input` open their own statements too.
  static constexpr auto StmtNameText =
      ref<ID> | SoftKeyword | ref<BracketedName> | ReservedName |
      ikw<"EMPTY">;

  static constexpr auto TypeSuffix = cr<"&@$!#%">;

  // -- Literal terminals ----------------------------------------------------

  static constexpr auto HEXDIGIT = cr<"0-9A-Fa-f">;

  struct GUID : terminal<lit<"{"> + some(HEXDIGIT) + lit<"-"> + some(HEXDIGIT) +
                          lit<"-"> + some(HEXDIGIT) + lit<"-"> +
                          some(HEXDIGIT) + lit<"-"> + some(HEXDIGIT) +
                          lit<"}">> {};

  static constexpr auto StringBody =
      lit<"\""> + many(lit<"\"\""> | cr<R"(^"\r\n)">) + lit<"\"">;

  struct STRINGLITERAL : terminal<as<undouble_dq>(StringBody)> {};

  // MS-VBAL 3.3.3: a date, a time or both, so `Close #1, #2` holds two file
  // numbers.
  static constexpr auto Sp = many(cr<R"( \t)">);
  static constexpr auto MonthName =
      ikw<"JANUARY"> | ikw<"FEBRUARY"> | ikw<"MARCH"> | ikw<"APRIL"> |
      ikw<"MAY"> | ikw<"JUNE"> | ikw<"JULY"> | ikw<"AUGUST"> |
      ikw<"SEPTEMBER"> | ikw<"OCTOBER"> | ikw<"NOVEMBER"> | ikw<"DECEMBER"> |
      ikw<"JAN"> | ikw<"FEB"> | ikw<"MAR"> | ikw<"APR"> | ikw<"JUN"> |
      ikw<"JUL"> | ikw<"AUG"> | ikw<"SEP"> | ikw<"OCT"> | ikw<"NOV"> |
      ikw<"DEC">;
  static constexpr auto DateValuePart = some(d) | MonthName;
  static constexpr auto DateSep = some(cr<R"( \t)">) | (Sp + cr<"/,-"> + Sp);
  static constexpr auto DateValue = DateValuePart + DateSep + DateValuePart +
                                     option(DateSep + DateValuePart);
  static constexpr auto TimeSep = Sp + cr<":."> + Sp;
  static constexpr auto AmPm = Sp + (ikw<"AM"> | ikw<"PM"> | cr<"APap">);
  static constexpr auto TimeValue =
      (some(d) + AmPm) | (some(d) + TimeSep + some(d) +
                           option(TimeSep + some(d)) + option(AmPm));

  struct DATELITERAL
      : terminal<lit<"#"> +
                 ((DateValue + some(cr<R"( \t)">) + TimeValue) | DateValue |
                  TimeValue) +
                 lit<"#">> {};

  struct COLORLITERAL
      : terminal<lit<"&"> + cr<"Hh"> + some(HEXDIGIT) + option(lit<"&">)> {};

  struct OCTALLITERAL
      : terminal<lit<"&"> + cr<"Oo"> + some(cr<"0-7">) + option(lit<"&">)> {};

  static constexpr auto NumSuffix = cr<"%&">;

  static constexpr auto Exponent = cr<"EeDd"> + option(cr<"+-">) + some(d);
  struct INTEGERLITERAL : terminal<some(d) + option(NumSuffix)> {};
  struct DOUBLELITERAL
      : terminal<(((((some(d) + lit<".", !lit<".">> + many(d)) |
                     (lit<"."> + some(d))) + option(Exponent)) |
                   (some(d) + Exponent)) + option(cr<"!#@">)) |
                 (some(d) + cr<"!#@">)> {};

  // File statements accept ordinary named handles. Keep conditional-
  // compilation heads outside this terminal even when both are reachable.
  static constexpr auto FileDirective = lit<"#"> +
      (ikw<"IF"> | ikw<"ELSEIF"> | ikw<"ELSE"> | ikw<"END"> | ikw<"CONST">);
  struct FILENUMBER
      : terminal<!FileDirective + lit<"#"> +
                 (some(d) | (LetterClass + many(LetterOrDigitClass)))> {};

  // A control property value's trailing offset into the form's binary
  // resource file (`.frx`): `ItemData = "SomeForm.frx":0000`.
  struct FRX_OFFSET : terminal<lit<":"> + some(HEXDIGIT)> {};

  // -- Expressions ------------------------------------------------------

  struct Expression;
  struct Postfix;
  struct TypeExpression;

  struct Literal
      : rule<create<ast::DateLit>(assign<&ast::DateLit::text>(ref<DATELITERAL>)) |
             create<ast::ColorLit>(
                 assign<&ast::ColorLit::text>(ref<COLORLITERAL>)) |
             create<ast::OctalLit>(
                 assign<&ast::OctalLit::text>(ref<OCTALLITERAL>)) |
             create<ast::FloatLit>(
                 assign<&ast::FloatLit::text>(ref<DOUBLELITERAL>)) |
             create<ast::FileNumberLit>(
                 assign<&ast::FileNumberLit::text>(ref<FILENUMBER>)) |
             create<ast::IntLit>(assign<&ast::IntLit::text>(ref<INTEGERLITERAL>)) |
             assign<&ast::StringLit::value>(ref<STRINGLITERAL>) |
             create<ast::BoolLit>(flag<&ast::BoolLit::value>(ikw<"TRUE">) |
                                   ikw<"FALSE">) |
             (create<ast::NothingLit> + ikw<"NOTHING">) |
             (create<ast::NullLit> + ikw<"NULL">)> {};

  // A bare name, optionally carrying a type-declaration suffix (`Date$`).
  struct Atom
      : rule<ref<Literal> |
             create<ast::NameExpr>(
                 assign<&ast::NameExpr::name>(ExprNameText) +
                 option(assign<&ast::NameExpr::typeChar>(TypeSuffix)))> {};

  struct StmtAtom
      : rule<create<ast::NameExpr>(
             assign<&ast::NameExpr::name>(StmtNameText) +
             option(assign<&ast::NameExpr::typeChar>(TypeSuffix)))> {};

  // `.field` / `!field` with no object: the implicit target inside a `With`
  // block -- VB6 reaches it through the same member-chain rule as an
  // ordinary access, not a shape of its own.
  struct WithExpr
      : rule<create<ast::MemberAccessExpr>(
                 lit<"."> +
                 assign<&ast::MemberAccessExpr::name>(UnrestrictedNameText)) |
             create<ast::DictionaryAccessExpr>(
                 lit<"!"> + assign<&ast::DictionaryAccessExpr::name>(
                                UnrestrictedNameText))> {};

  static constexpr auto ArgModifier =
      flag<&ast::Arg::byVal>(ikw<"BYVAL">) |
      flag<&ast::Arg::byRef>(ikw<"BYREF">) |
      flag<&ast::Arg::paramArray>(ikw<"PARAMARRAY">);

  // A comma/semicolon-separated argument slot that may be entirely blank
  // (`Foo(1, , 3)`'s omitted middle argument).
  static constexpr auto ArgSlotBody =
      create<ast::Arg>(option(ArgModifier) +
                        option(assign<&ast::Arg::value>(ref<Expression>)));
  // The one slot in a list that must carry a value: what makes an empty
  // argument list (`Foo()`) distinguishable from a single blank slot.
  static constexpr auto ArgSlotNonBlank =
      create<ast::Arg>(option(ArgModifier) +
                        assign<&ast::Arg::value>(ref<Expression>));

  static constexpr auto ArgSep = lit<","> | lit<";">;
  static constexpr auto SeparatedArgBody = create<ast::Arg>(
      assign<&ast::Arg::separatorBefore>(ArgSep) + option(ArgModifier) +
      option(assign<&ast::Arg::value>(ref<Expression>)));

  template <auto Member>
  static constexpr auto ParenArgsInto =
      (lit<"("> + lit<")">) |
      (lit<"("> + append<Member>(ArgSlotBody) +
       many(append<Member>(SeparatedArgBody)) + lit<")">);

  template <auto Member>
  static constexpr auto BareArgsInto =
      append<Member>(ArgSlotNonBlank) +
      many(append<Member>(SeparatedArgBody));

  // A chain of `.`/`!`/`(...)` following either a name (`LExpression`) or a
  // bare `.field`/`!field` (`WithExpr`): both feed the same continuation, so
  // `.CellSet(0).Value` -- a `With` target indexed, then a further member --
  // reads as one chain rather than stopping after the first segment.
  static constexpr auto PostfixChain =
      many(nest<ast::MemberAccessExpr, &ast::MemberAccessExpr::object>(
               lit<"."> + assign<&ast::MemberAccessExpr::name>(
                              UnrestrictedNameText)) |
           nest<ast::DictionaryAccessExpr, &ast::DictionaryAccessExpr::object>(
               lit<"!"> + assign<&ast::DictionaryAccessExpr::name>(
                              UnrestrictedNameText)) |
           nest<ast::IndexExpr, &ast::IndexExpr::target>(
               ParenArgsInto<&ast::IndexExpr::args>));

  struct Postfix : rule<(ref<WithExpr> | ref<Atom>) + PostfixChain> {};

  static constexpr auto CallTarget = ref<Postfix>;

  struct StmtTarget : rule<(ref<WithExpr> | ref<StmtAtom>) + PostfixChain> {};

  // VB6 restricts a `For`/`For Each` loop's own variable to a bare name
  // with, at most, a trailing dictionary access -- no member chain, no
  // call.
  struct LoopVar
      : rule<ref<Literal> |
             (create<ast::NameExpr>(
                  assign<&ast::NameExpr::name>(NameText) +
                  option(assign<&ast::NameExpr::typeChar>(TypeSuffix))) +
              option(nest<ast::DictionaryAccessExpr,
                          &ast::DictionaryAccessExpr::object>(
                  lit<"!"> + assign<&ast::DictionaryAccessExpr::name>(
                                 UnrestrictedNameText))))> {};

  // -- Types ----------------------------------------------------------------

  // A plain `rule<>` with no directives deduces to the text it matched, like
  // `QualifiedName` in the `sql` example: a dotted chain of names covers
  // both a builtin type and a user type name.
  struct TypeNameText
      : rule<some(NameText | ReservedTypeIdentifier | ikw<"DATE"> |
                      ikw<"ANY">,
                  lit<".">)> {};

  static constexpr auto StringLenExpr =
      create<ast::IntLit>(assign<&ast::IntLit::text>(ref<INTEGERLITERAL>)) |
      create<ast::NameExpr>(assign<&ast::NameExpr::name>(NameText));

  // `fieldLength` (`String * n`) sits beside the type name in a
  // declaration's type clause, never anywhere else: folding it into
  // `TypeExpression` itself changes no accepted document.
  struct TypeExpression
      : rule<create<ast::TypeRef>(
                 assign<&ast::TypeRef::name>(ref<TypeNameText>) +
                 option(flag<&ast::TypeRef::isArrayType>(
                     lit<"("> + lit<")">)) +
                 option(lit<"*"> +
                        assign<&ast::TypeRef::stringLength>(StringLenExpr)))> {
  };
  static constexpr auto TypeExpressionText =
      ref<TypeNameText> + option(lit<"("> + lit<")">) +
      option(lit<"*"> + (ref<INTEGERLITERAL> | NameText));

  // -- Expression ladder ------------------------------------------------

  struct ValuePrimary
      : rule<create<ast::ParenExpr>(
                 lit<"("> + append<&ast::ParenExpr::items>(ref<Expression>) +
                 many(lit<","> + append<&ast::ParenExpr::items>(
                                      ref<Expression>)) +
                 lit<")">) |
             create<ast::NewExpr>(
                 ikw<"NEW"> +
                 assign<&ast::NewExpr::operand>(CallTarget)) |
             create<ast::TypeOfIsExpr>(
                 ikw<"TYPEOF"> +
                 assign<&ast::TypeOfIsExpr::operand>(CallTarget) +
                 option(ikw<"IS"> + assign<&ast::TypeOfIsExpr::typeName>(
                                         ref<TypeExpression>))) |
             create<ast::AddressOfExpr>(
                 ikw<"ADDRESSOF"> +
                 assign<&ast::AddressOfExpr::operand>(CallTarget)) |
             (CallTarget +
              option(nest<ast::AssignExpr, &ast::AssignExpr::target>(
                  lit<":="> +
                  assign<&ast::AssignExpr::value>(ref<Expression>))))> {};

  static constexpr auto OpPow = lit<"^">;
  static constexpr auto OpMul = lit<"/"> | lit<"*">;
  static constexpr auto OpIntDiv = lit<"\\">;
  static constexpr auto OpModText = ikw<"MOD">;
  static constexpr auto OpAdd = lit<"+"> | lit<"-">;
  static constexpr auto OpConcat = lit<"&">;
  static constexpr auto OpComp = lit<"<="> | lit<">="> | lit<"<>"> |
                                  lit<"="> | lit<"<"> | lit<">"> |
                                  ikw<"LIKE"> | ikw<"IS">;

  struct PowExpr;
  struct SignedPowerAtom : rule<create<ast::UnaryExpr>(assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-">) + assign<&ast::UnaryExpr::operand>(ref<SignedPowerAtom>)) | ref<ValuePrimary>> {};
  struct PowExpr : rule<ref<ValuePrimary> + many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(assign<&ast::BinaryExpr::op>(OpPow) + assign<&ast::BinaryExpr::right>(ref<SignedPowerAtom>)))> {};
  struct UnaryPM : rule<create<ast::UnaryExpr>(assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-">) + assign<&ast::UnaryExpr::operand>(ref<UnaryPM>)) | ref<PowExpr>> {};

  // Multiplicative `*`/`/`, integer division `\` and `Mod` are three
  // distinct tiers, tightest to loosest in that order, not one.
  struct ExprArith
      : infix<UnaryPM, ast::BinaryExpr,
              levels(left(OpMul), left(OpIntDiv), left(OpModText),
                     left(OpAdd), left(OpConcat), left(OpComp))> {};

  // `Not` binds looser than comparison but tighter than `And`: `Not a = b`
  // reads as `Not (a = b)`.
  struct NotExpr
      : rule<create<ast::UnaryExpr>(assign<&ast::UnaryExpr::op>(ikw<"NOT">) +
                                     assign<&ast::UnaryExpr::operand>(
                                         ref<NotExpr>)) |
             ref<ExprArith>> {};

  static constexpr auto OpAnd = ikw<"AND">;
  static constexpr auto OpOr = ikw<"OR">;
  static constexpr auto OpXor = ikw<"XOR">;
  static constexpr auto OpEqv = ikw<"EQV">;
  static constexpr auto OpImp = ikw<"IMP">;

  struct Expression
      : infix<NotExpr, ast::BinaryExpr,
              levels(left(OpAnd), left(OpOr), left(OpXor), left(OpEqv),
                     left(OpImp))> {};

  // -- Shared list helpers ----------------------------------------------

  struct ArrayBound
      : rule<option(assign<&ast::ArrayBound::lower>(ref<Expression>) +
                    ikw<"TO">) +
             assign<&ast::ArrayBound::upper>(ref<Expression>)> {};

  template <auto Member>
  static constexpr auto ArrayDimTail =
      option(some(append<Member>(ref<ArrayBound>), lit<",">)) + lit<")">;

  template <auto Member>
  static constexpr auto ArrayDimInto = lit<"("> + ArrayDimTail<Member>;

  template <auto Flag, auto Member>
  static constexpr auto FlaggedArrayDimInto =
      flag<Flag>(lit<"(">) + ArrayDimTail<Member>;

  struct Param
      : rule<create<ast::Param>(
                 option(flag<&ast::Param::optionalP>(ikw<"OPTIONAL">)) +
                 option(flag<&ast::Param::byVal>(ikw<"BYVAL">) |
                        flag<&ast::Param::byRef>(ikw<"BYREF">)) +
                 option(flag<&ast::Param::paramArray>(
                     ikw<"PARAMARRAY">)) +
                 assign<&ast::Param::name>(NameText) +
                 option(assign<&ast::Param::typeChar>(TypeSuffix)) +
                 option(flag<&ast::Param::isArray>(lit<"("> +
                                                         lit<")">)) +
                 option(ikw<"AS"> +
                        assign<&ast::Param::asType>(ref<TypeExpression>)) +
                 option(lit<"="> + assign<&ast::Param::defaultValue>(
                                        ref<Expression>)))> {};

  template <auto Member>
  static constexpr auto ParamListInto =
      lit<"("> + option(some(append<Member>(ref<Param>), lit<",">)) +
      lit<")">;

  struct StatementRule;

  struct LabelRule
      : rule<create<ast::LabelStmt>(
             assign<&ast::LabelStmt::name>(ref<LABEL> | ref<INTEGERLITERAL>))> {};

  // A label opens a line, right behind its line break (MS-VBAL 5.4.1.1).
  template <auto Member>
  static constexpr auto SepInto =
      some((ref<NEWLINE> + option(append<Member>(ref<LabelRule>))) |
           lit<":">);

  // MS-VBAL `statement-block`: each statement ended by its separator, so a
  // label on a block's last line is the block's, and its closer follows.
  template <auto Member>
  static constexpr auto BodyInto =
      SepInto<Member> +
      many(append<Member>(ref<StatementRule>) + SepInto<Member>);

  // Like `BodyInto`, but its first item needs no leading separator: an
  // `ElseIf ... Then` clause's body may start right on the same line
  // (`ElseIf i = 2 Then Statement`) -- VB6 accepts either whitespace or a
  // newline between `Then` and the body, not only a newline.
  template <auto Member>
  static constexpr auto FlexBodyInto =
      option(append<Member>(ref<StatementRule>)) + BodyInto<Member>;

  // -- Declarations -------------------------------------------------------

  static constexpr auto ProcVisibility =
      ikw<"PRIVATE"> | ikw<"PUBLIC"> | ikw<"FRIEND"> | ikw<"GLOBAL">;
  static constexpr auto TypeEnumVisibility = ikw<"PRIVATE"> | ikw<"PUBLIC">;
  static constexpr auto ConstVisibility =
      ikw<"PRIVATE"> | ikw<"PUBLIC"> | ikw<"GLOBAL">;

  struct VarDeclItem
      : rule<create<ast::VarDeclItem>(
                 assign<&ast::VarDeclItem::name>(NameText) +
                 option(assign<&ast::VarDeclItem::typeChar>(TypeSuffix)) +
                 option(FlaggedArrayDimInto<&ast::VarDeclItem::isArray,
                                            &ast::VarDeclItem::bounds>) +
                 option(ikw<"AS"> +
                        option(flag<&ast::VarDeclItem::asNew>(
                            ikw<"NEW">)) +
                        assign<&ast::VarDeclItem::asType>(
                            ref<TypeExpression>)))> {};

  struct VarDecl
      : rule<create<ast::VarDecl>(
                 assign<&ast::VarDecl::scope>(ikw<"DIM"> | ikw<"STATIC"> |
                                               ProcVisibility) +
                 option(flag<&ast::VarDecl::withEvents>(
                     ikw<"WITHEVENTS">)) +
                 some(append<&ast::VarDecl::items>(ref<VarDeclItem>),
                      lit<",">))> {};

  struct ConstDeclItem
      : rule<create<ast::ConstDeclItem>(
                 assign<&ast::ConstDeclItem::name>(NameText) +
                 option(assign<&ast::ConstDeclItem::typeChar>(TypeSuffix)) +
                 option(ikw<"AS"> + assign<&ast::ConstDeclItem::asType>(
                                        ref<TypeExpression>)) +
                 lit<"="> +
                 assign<&ast::ConstDeclItem::value>(ref<Expression>))> {};

  struct ConstDecl
      : rule<create<ast::ConstDecl>(
                 option(assign<&ast::ConstDecl::scope>(ConstVisibility)) +
                 ikw<"CONST"> +
                 some(append<&ast::ConstDecl::items>(ref<ConstDeclItem>),
                      lit<",">))> {};

  struct LetterSpec
      : rule<create<ast::LetterSpec>(
                 assign<&ast::LetterSpec::from>(NameText) +
                 option(lit<"-"> +
                        assign<&ast::LetterSpec::to>(NameText)))> {};

  struct DefDirective
      : rule<create<ast::DefDirective>(
                 assign<&ast::DefDirective::type>(
                     ikw<"DEFBOOL"> | ikw<"DEFBYTE"> | ikw<"DEFINT"> |
                     ikw<"DEFLNG"> | ikw<"DEFCUR"> | ikw<"DEFSNG"> |
                     ikw<"DEFDBL"> | ikw<"DEFDEC"> | ikw<"DEFDATE"> |
                     ikw<"DEFSTR"> | ikw<"DEFOBJ"> | ikw<"DEFVAR">) +
                 some(append<&ast::DefDirective::letters>(ref<LetterSpec>),
                      lit<",">))> {};

  struct UdtMember
      : rule<create<ast::UdtMember>(
                 assign<&ast::UdtMember::name>(UnrestrictedNameText) +
                 option(FlaggedArrayDimInto<&ast::UdtMember::isArray,
                                            &ast::UdtMember::bounds>) +
                 option(ikw<"AS"> +
                        assign<&ast::UdtMember::asType>(
                            ref<TypeExpression>)))> {};

  struct TypeDecl
      : rule<create<ast::TypeDecl>(
                 option(assign<&ast::TypeDecl::scope>(TypeEnumVisibility)) +
                 ikw<"TYPE"> + assign<&ast::TypeDecl::name>(NameText) +
                 many(Eos + append<&ast::TypeDecl::members>(
                                ref<UdtMember>)) +
                 Eos + ikw<"END"> + ikw<"TYPE">)> {};

  struct EnumMember
      : rule<create<ast::EnumMember>(
                 assign<&ast::EnumMember::name>(NameText) +
                 option(lit<"="> +
                        assign<&ast::EnumMember::value>(ref<Expression>)))> {
  };

  struct EnumDecl
      : rule<create<ast::EnumDecl>(
                 option(assign<&ast::EnumDecl::scope>(TypeEnumVisibility)) +
                 ikw<"ENUM"> + assign<&ast::EnumDecl::name>(NameText) +
                 many(Eos + append<&ast::EnumDecl::members>(
                                ref<EnumMember>)) +
                 Eos + ikw<"END"> + ikw<"ENUM">)> {};

  struct DeclareDecl
      : rule<create<ast::DeclareDecl>(
                 option(assign<&ast::DeclareDecl::scope>(ProcVisibility)) +
                 ikw<"DECLARE"> +
                 ((flag<&ast::DeclareDecl::isFunction>(
                       ikw<"FUNCTION">) +
                   assign<&ast::DeclareDecl::name>(NameText) +
                   option(assign<&ast::DeclareDecl::typeChar>(TypeSuffix)) +
                   ikw<"LIB"> +
                   assign<&ast::DeclareDecl::lib>(ref<STRINGLITERAL>) +
                   option(ikw<"ALIAS"> + assign<&ast::DeclareDecl::alias>(
                                             ref<STRINGLITERAL>)) +
                   option(ParamListInto<&ast::DeclareDecl::params>) +
                   option(ikw<"AS"> + assign<&ast::DeclareDecl::returnType>(
                                          ref<TypeExpression>))) |
                  (ikw<"SUB"> + assign<&ast::DeclareDecl::name>(NameText) +
                   ikw<"LIB"> +
                   assign<&ast::DeclareDecl::lib>(ref<STRINGLITERAL>) +
                   option(ikw<"ALIAS"> + assign<&ast::DeclareDecl::alias>(
                                             ref<STRINGLITERAL>)) +
                   option(ParamListInto<&ast::DeclareDecl::params>))))> {};

  struct EventDecl
      : rule<create<ast::EventDecl>(
                 option(assign<&ast::EventDecl::scope>(ProcVisibility)) +
                 ikw<"EVENT"> + assign<&ast::EventDecl::name>(NameText) +
                 ParamListInto<&ast::EventDecl::params>)> {};

  struct ImplementsDecl
      : rule<create<ast::ImplementsDecl>(
                 ikw<"IMPLEMENTS"> +
                 assign<&ast::ImplementsDecl::typeName>(
                     ref<TypeNameText>))> {};

  struct OptionStmt
      : rule<create<ast::OptionStmt>(
                 ikw<"OPTION"> + assign<&ast::OptionStmt::kind>(
                                     ikw<"BASE">) +
                 assign<&ast::OptionStmt::value>(ref<INTEGERLITERAL>)) |
             create<ast::OptionStmt>(
                 ikw<"OPTION"> +
                 assign<&ast::OptionStmt::kind>(ikw<"COMPARE">) +
                 assign<&ast::OptionStmt::value>(ikw<"BINARY"> |
                                                  ikw<"TEXT">)) |
             create<ast::OptionStmt>(
                 ikw<"OPTION"> +
                 assign<&ast::OptionStmt::kind>(ikw<"EXPLICIT">)) |
             create<ast::OptionStmt>(
                 ikw<"OPTION"> + ikw<"PRIVATE"> +
                 assign<&ast::OptionStmt::kind>(ikw<"MODULE">))> {};

  struct Attribute
      : rule<create<ast::Attribute>(
                 ikw<"ATTRIBUTE"> +
                 assign<&ast::Attribute::target>(CallTarget) + lit<"="> +
                 some(append<&ast::Attribute::values>(ref<Literal>), lit<",">))> {};

  // -- Statements -----------------------------------------------------------

  struct CallStmt
      : rule<create<ast::CallStmt>(
                 ikw<"CALL"> + assign<&ast::CallStmt::target>(CallTarget)) |
             create<ast::CallStmt>(
                 assign<&ast::CallStmt::target>(ref<StmtTarget>) +
                 option(BareArgsInto<&ast::CallStmt::args>))> {};

  static constexpr auto LetOp = lit<"+="> | lit<"-="> | lit<"=">;

  struct LetStmt
      : rule<create<ast::LetStmt>(
                 ((ikw<"LET"> + assign<&ast::LetStmt::target>(CallTarget)) |
                  assign<&ast::LetStmt::target>(ref<StmtTarget>)) +
                 assign<&ast::LetStmt::op>(LetOp) +
                 assign<&ast::LetStmt::value>(ref<Expression>))> {};

  struct SetStmt
      : rule<create<ast::SetStmt>(
                 ikw<"SET"> + assign<&ast::SetStmt::target>(CallTarget) +
                 lit<"="> +
                 assign<&ast::SetStmt::value>(ref<Expression>))> {};

  struct LSetStmt
      : rule<create<ast::LSetStmt>(
                 ikw<"LSET"> + assign<&ast::LSetStmt::target>(CallTarget) +
                 lit<"="> +
                 assign<&ast::LSetStmt::value>(ref<Expression>))> {};

  struct RSetStmt
      : rule<create<ast::RSetStmt>(
                 ikw<"RSET"> + assign<&ast::RSetStmt::target>(CallTarget) +
                 lit<"="> +
                 assign<&ast::RSetStmt::value>(ref<Expression>))> {};

  struct WhileStmt
      : rule<create<ast::WhileStmt>(
                 ikw<"WHILE"> +
                 assign<&ast::WhileStmt::cond>(ref<Expression>) +
                 BodyInto<&ast::WhileStmt::body> + ikw<"WEND">)> {};

  struct DoStmt;
  static constexpr auto DoCondGroup =
      (assign<&ast::DoStmt::condKind>(ikw<"WHILE">) +
       assign<&ast::DoStmt::cond>(ref<Expression>)) |
      (assign<&ast::DoStmt::condKind>(ikw<"UNTIL">) +
       assign<&ast::DoStmt::cond>(ref<Expression>));

  static constexpr auto DoBodyLoop =
      BodyInto<&ast::DoStmt::body> + ikw<"LOOP">;

  // `DO` opens each branch: factored out of the choice, it adds table
  // ambiguities.
  struct DoStmt
      : rule<create<ast::DoStmt>(
                 (flag<&ast::DoStmt::pre>(ikw<"DO">) + DoCondGroup +
                  DoBodyLoop) |
                 (ikw<"DO"> + DoBodyLoop + option(DoCondGroup)))> {};

  // The optional inline type hint / `As` clause on the loop variable itself
  // is accepted but not stored: no example uses it, and it decorates a
  // variable declared elsewhere.
  static constexpr auto LoopVarTail =
      option(TypeSuffix) + option(ikw<"AS"> + TypeExpressionText);

  struct ForStmt;
  struct ForEachStmt;

  // `Next j, i` closes two loops: the inner one writes `Next j`, and the
  // loop it ends takes its own name after the comma.
  template <auto Member>
  static constexpr auto NextOrInnerLoop =
      ikw<"NEXT"> |
      (append<Member>(ref<ForStmt> | ref<ForEachStmt>) + lit<",">);

  struct ForStmt
      : rule<create<ast::ForStmt>(
                 ikw<"FOR"> + assign<&ast::ForStmt::var>(ref<LoopVar>) +
                 LoopVarTail + lit<"="> +
                 assign<&ast::ForStmt::from>(ref<Expression>) + ikw<"TO"> +
                 assign<&ast::ForStmt::to>(ref<Expression>) +
                 option(ikw<"STEP"> +
                        assign<&ast::ForStmt::step>(ref<Expression>)) +
                 BodyInto<&ast::ForStmt::body> +
                 NextOrInnerLoop<&ast::ForStmt::body> +
                 option(assign<&ast::ForStmt::closeVar>(NameText) +
                        option(TypeSuffix)))> {};

  struct ForEachStmt
      : rule<create<ast::ForEachStmt>(
                 ikw<"FOR"> + ikw<"EACH"> +
                 assign<&ast::ForEachStmt::var>(ref<LoopVar>) +
                 LoopVarTail + ikw<"IN"> +
                 assign<&ast::ForEachStmt::collection>(ref<Expression>) +
                 BodyInto<&ast::ForEachStmt::body> +
                 NextOrInnerLoop<&ast::ForEachStmt::body> +
                 option(assign<&ast::ForEachStmt::closeVar>(NameText)))> {};

  struct ExitStmt
      : rule<create<ast::ExitStmt>(
                 ikw<"EXIT"> +
                 assign<&ast::ExitStmt::kind>(
                     ikw<"DO"> | ikw<"FOR"> | ikw<"FUNCTION"> |
                     ikw<"PROPERTY"> | ikw<"SUB">))> {};

  // -- If -------------------------------------------------------------------

  struct ElseIfClause
      : rule<create<ast::ElseIfClause>(
                 ikw<"ELSEIF"> +
                 assign<&ast::ElseIfClause::cond>(ref<Expression>) +
                 ikw<"THEN"> + FlexBodyInto<&ast::ElseIfClause::body>)> {};

  // Block form: nothing but a line break follows `Then`.
  struct IfStmt
      : rule<create<ast::IfStmt>(
                 ikw<"IF"> + assign<&ast::IfStmt::cond>(ref<Expression>) +
                 ikw<"THEN"> + BodyInto<&ast::IfStmt::thenBody> +
                 many(append<&ast::IfStmt::elseIfs>(ref<ElseIfClause>)) +
                 option(flag<&ast::IfStmt::hasElse>(ikw<"ELSE">) +
                        BodyInto<&ast::IfStmt::elseBody>) +
                 ikw<"END"> + ikw<"IF">)> {};

  // Single-line form: colon-chained statements follow `Then`/`Else` on the
  // same logical line; no `ElseIf`. Every colon-chained statement groups as
  // the `If`'s own body, not only the first with the rest falling to the
  // enclosing block's own separator -- that grouping is what a
  // colon-chained `Then` means in real VB6. An empty `Then` needs its `Else`
  // or a `:` behind it: `If c Then` alone on its line opens a block.
  struct SingleLineIfStmt
      : rule<create<ast::IfStmt>(
                 ikw<"IF"> + assign<&ast::IfStmt::cond>(ref<Expression>) +
                 ikw<"THEN"> +
                 ((some(append<&ast::IfStmt::thenBody>(ref<StatementRule>),
                        lit<":">) +
                   option(lit<":">)) |
                  (option(lit<":">) + &ikw<"ELSE">) | &lit<":">) +
                 option(flag<&ast::IfStmt::hasElse>(ikw<"ELSE">) +
                        many(append<&ast::IfStmt::elseBody>(ref<StatementRule>),
                             lit<":">)))> {};

  // -- Select Case ------------------------------------------------------

  struct RangeClause
      : rule<create<ast::RangeClause>(
                 option(ikw<"IS">) +
                 assign<&ast::RangeClause::compareOp>(OpComp) +
                 assign<&ast::RangeClause::value>(ref<Expression>)) |
             create<ast::RangeClause>(
                 assign<&ast::RangeClause::value>(ref<Expression>) +
                 ikw<"TO"> +
                 assign<&ast::RangeClause::rangeTo>(ref<Expression>)) |
             create<ast::RangeClause>(
                 assign<&ast::RangeClause::value>(ref<Expression>))> {};

  struct CaseClause
      : rule<create<ast::CaseClause>(
                 ikw<"CASE"> +
                 (flag<&ast::CaseClause::isElse>(ikw<"ELSE">) |
                  some(append<&ast::CaseClause::ranges>(ref<RangeClause>),
                       lit<",">)) +
                 BodyInto<&ast::CaseClause::body>)> {};

  struct SelectCaseStmt
      : rule<create<ast::SelectCaseStmt>(
                 ikw<"SELECT"> + ikw<"CASE"> +
                 assign<&ast::SelectCaseStmt::selector>(ref<Expression>) +
                 Eos +
                 many(append<&ast::SelectCaseStmt::cases>(
                     ref<CaseClause>)) +
                 ikw<"END"> + ikw<"SELECT">)> {};

  struct StopStmt : rule<create<ast::StopStmt> + ikw<"STOP">> {};

  static constexpr auto LabelText = NameText | ref<INTEGERLITERAL>;

  struct GotoStmt
      : rule<create<ast::GotoStmt>(
                 ikw<"GOTO"> + assign<&ast::GotoStmt::label>(LabelText))> {};

  struct OnGotoStmt
      : rule<create<ast::OnGotoStmt>(
                 ikw<"ON"> +
                 assign<&ast::OnGotoStmt::selector>(ref<Expression>) +
                 ikw<"GOTO"> +
                 some(append<&ast::OnGotoStmt::labels>(LabelText),
                      lit<",">))> {};

  struct GosubStmt
      : rule<create<ast::GosubStmt>(
                 ikw<"GOSUB"> +
                 assign<&ast::GosubStmt::label>(LabelText))> {};

  struct ReturnStmt : rule<create<ast::ReturnStmt> + ikw<"RETURN">> {};

  struct OnGosubStmt
      : rule<create<ast::OnGosubStmt>(
                 ikw<"ON"> +
                 assign<&ast::OnGosubStmt::selector>(ref<Expression>) +
                 ikw<"GOSUB"> +
                 some(append<&ast::OnGosubStmt::labels>(LabelText),
                      lit<",">))> {};

  struct RaiseEventStmt
      : rule<create<ast::RaiseEventStmt>(
                 ikw<"RAISEEVENT"> +
                 assign<&ast::RaiseEventStmt::name>(NameText) +
                 option(lit<"("> +
                        option(some(append<&ast::RaiseEventStmt::args>(
                                        ref<Expression>),
                                    lit<",">)) +
                        lit<")">))> {};

  struct WithStmt
      : rule<create<ast::WithStmt>(
                 ikw<"WITH"> +
                 assign<&ast::WithStmt::target>(ref<Expression>) +
                 BodyInto<&ast::WithStmt::body> + ikw<"END"> +
                 ikw<"WITH">)> {};

  struct EndStmt : rule<create<ast::EndStmt> + ikw<"END">> {};

  // A trailing colon after the `GoTo` target (`On Error GoTo Label:`) needs
  // no rule of its own: the surrounding body's own separator reads it.
  struct OnErrorStmt
      : rule<create<ast::OnErrorStmt>(
                 ikw<"ON"> +
                 option(flag<&ast::OnErrorStmt::local>(ikw<"LOCAL">)) +
                 ikw<"ERROR"> +
                 (flag<&ast::OnErrorStmt::resumeNext>(
                      ikw<"RESUME"> + ikw<"NEXT">) |
                  (ikw<"GOTO"> +
                   assign<&ast::OnErrorStmt::label>(LabelText))))> {};

  struct ResumeStmt
      : rule<create<ast::ResumeStmt>(
                 ikw<"RESUME"> +
                 option(flag<&ast::ResumeStmt::next>(ikw<"NEXT">) |
                        assign<&ast::ResumeStmt::label>(LabelText)))> {};

  struct ErrorStmt
      : rule<create<ast::ErrorStmt>(
                 ikw<"ERROR"> +
                 assign<&ast::ErrorStmt::number>(ref<Expression>))> {};

  // -- Array / file statements ---------------------------------------------

  struct ReDimItem
      : rule<create<ast::ReDimItem>(
                 assign<&ast::ReDimItem::target>(CallTarget) +
                 option(assign<&ast::ReDimItem::typeChar>(TypeSuffix)) +
                 ArrayDimInto<&ast::ReDimItem::bounds> +
                 option(ikw<"AS"> + assign<&ast::ReDimItem::asType>(ref<TypeExpression>)))> {};

  struct ReDimStmt
      : rule<create<ast::ReDimStmt>(
                 ikw<"REDIM"> +
                 option(flag<&ast::ReDimStmt::preserve>(
                     ikw<"PRESERVE">)) +
                 some(append<&ast::ReDimStmt::items>(ref<ReDimItem>),
                      lit<",">))> {};

  struct EraseStmt
      : rule<create<ast::EraseStmt>(
                 ikw<"ERASE"> +
                 some(append<&ast::EraseStmt::targets>(ref<Expression>),
                      lit<",">))> {};

  struct AccessKind
      : rule<(ikw<"READ"> + ikw<"WRITE">) | ikw<"READ"> | ikw<"WRITE">> {};

  struct LockKind
      : rule<ikw<"SHARED"> | (ikw<"LOCK"> + ikw<"READ"> + ikw<"WRITE">) |
             (ikw<"LOCK"> + ikw<"READ">) | (ikw<"LOCK"> + ikw<"WRITE">)> {};

  struct OpenStmt
      : rule<create<ast::OpenStmt>(
                 ikw<"OPEN"> + assign<&ast::OpenStmt::path>(ref<Expression>) +
                 option(ikw<"FOR"> +
                        assign<&ast::OpenStmt::mode>(ikw<"APPEND"> | ikw<"BINARY"> |
                                                    ikw<"INPUT"> | ikw<"OUTPUT"> |
                                                    ikw<"RANDOM">)) +
                 option(ikw<"ACCESS"> +
                        assign<&ast::OpenStmt::access>(
                            ref<AccessKind>)) +
                 option(assign<&ast::OpenStmt::lockKind>(
                     ref<LockKind>)) +
                 ikw<"AS"> +
                 assign<&ast::OpenStmt::fileNumber>(ref<Expression>) +
                 option(ikw<"LEN"> + lit<"="> +
                        assign<&ast::OpenStmt::recLength>(ref<Expression>)))> {
  };

  struct CloseStmt
      : rule<create<ast::CloseStmt>(
                 ikw<"CLOSE"> +
                 option(some(append<&ast::CloseStmt::fileNumbers>(
                                 ref<Expression>),
                             lit<",">)))> {};

  struct ResetStmt : rule<create<ast::ResetStmt> + ikw<"RESET">> {};

  struct SeekStmt
      : rule<create<ast::SeekStmt>(
                 ikw<"SEEK"> +
                 assign<&ast::SeekStmt::fileNumber>(ref<Expression>) +
                 lit<","> +
                 assign<&ast::SeekStmt::position>(ref<Expression>))> {};

  struct LockStmt
      : rule<create<ast::LockStmt>(
                 ikw<"LOCK"> +
                 assign<&ast::LockStmt::fileNumber>(ref<Expression>) +
                 option(lit<","> +
                        option(assign<&ast::LockStmt::startRecord>(
                            ref<Expression>)) +
                        option(ikw<"TO"> +
                               assign<&ast::LockStmt::endRecord>(
                                   ref<Expression>))))> {};

  struct UnlockStmt
      : rule<create<ast::UnlockStmt>(
                 ikw<"UNLOCK"> +
                 assign<&ast::UnlockStmt::fileNumber>(ref<Expression>) +
                 option(lit<","> +
                        option(assign<&ast::UnlockStmt::startRecord>(
                            ref<Expression>)) +
                        option(ikw<"TO"> +
                               assign<&ast::UnlockStmt::endRecord>(
                                   ref<Expression>))))> {};

  struct LineInputStmt
      : rule<create<ast::LineInputStmt>(
                 ikw<"LINE"> + ikw<"INPUT"> +
                 assign<&ast::LineInputStmt::fileNumber>(ref<Expression>) +
                 lit<","> +
                 assign<&ast::LineInputStmt::target>(ref<Expression>))> {};

  struct WidthStmt
      : rule<create<ast::WidthStmt>(
                 ikw<"WIDTH"> +
                 assign<&ast::WidthStmt::fileNumber>(ref<Expression>) +
                 lit<","> +
                 assign<&ast::WidthStmt::width>(ref<Expression>))> {};

  static constexpr auto PrintSep =
      assign<&ast::PrintItem::sep>(lit<";"> | lit<",">);

  static constexpr auto PrintItemBody =
      create<ast::PrintItem>(
          flag<&ast::PrintItem::isSpc>(ikw<"SPC">) + lit<"("> +
          assign<&ast::PrintItem::value>(ref<Expression>) + lit<")"> +
          option(PrintSep)) |
      create<ast::PrintItem>(
          flag<&ast::PrintItem::isTab>(ikw<"TAB">) + lit<"("> +
          assign<&ast::PrintItem::value>(ref<Expression>) + lit<")"> +
          option(PrintSep)) |
      create<ast::PrintItem>(assign<&ast::PrintItem::value>(ref<Expression>) +
                              option(PrintSep)) |
      create<ast::PrintItem>(PrintSep);

  struct PrintStmt
      : rule<create<ast::PrintStmt>(
                 ikw<"PRINT"> +
                 assign<&ast::PrintStmt::fileNumber>(ref<Expression>) +
                 lit<","> +
                 many(append<&ast::PrintStmt::items>(PrintItemBody)))> {};

  struct WriteStmt
      : rule<create<ast::WriteStmt>(
                 ikw<"WRITE"> +
                 assign<&ast::WriteStmt::fileNumber>(ref<Expression>) +
                 lit<","> +
                 many(append<&ast::WriteStmt::items>(PrintItemBody)))> {};

  struct InputStmt
      : rule<create<ast::InputStmt>(
                 ikw<"INPUT"> +
                 assign<&ast::InputStmt::fileNumber>(ref<Expression>) +
                 some(lit<","> + append<&ast::InputStmt::targets>(
                                      ref<Expression>)))> {};

  struct PutStmt
      : rule<create<ast::PutStmt>(
                 ikw<"PUT"> +
                 assign<&ast::PutStmt::fileNumber>(ref<Expression>) +
                 lit<","> +
                 option(assign<&ast::PutStmt::recordNumber>(
                     ref<Expression>)) +
                 lit<","> +
                 assign<&ast::PutStmt::value>(ref<Expression>))> {};

  struct GetStmt
      : rule<create<ast::GetStmt>(
                 ikw<"GET"> +
                 assign<&ast::GetStmt::fileNumber>(ref<Expression>) +
                 lit<","> +
                 option(assign<&ast::GetStmt::recordNumber>(
                     ref<Expression>)) +
                 lit<","> +
                 assign<&ast::GetStmt::value>(ref<Expression>))> {};

  // -- Miscellaneous single-word-led statements ----------------------------

  struct AppActivateStmt
      : rule<create<ast::AppActivateStmt>(
                 ikw<"APPACTIVATE"> +
                 assign<&ast::AppActivateStmt::title>(ref<Expression>) +
                 option(lit<","> + assign<&ast::AppActivateStmt::wait>(
                                        ref<Expression>)))> {};

  struct BeepStmt : rule<create<ast::BeepStmt> + ikw<"BEEP">> {};

  struct ChDirStmt
      : rule<create<ast::ChDirStmt>(
                 ikw<"CHDIR"> +
                 assign<&ast::ChDirStmt::path>(ref<Expression>))> {};

  struct ChDriveStmt
      : rule<create<ast::ChDriveStmt>(
                 ikw<"CHDRIVE"> +
                 assign<&ast::ChDriveStmt::drive>(ref<Expression>))> {};

  struct DateStmt
      : rule<create<ast::DateStmt>(
                 ikw<"DATE"> + lit<"="> +
                 assign<&ast::DateStmt::value>(ref<Expression>))> {};

  struct DeleteSettingStmt
      : rule<create<ast::DeleteSettingStmt>(
                 ikw<"DELETESETTING"> +
                 assign<&ast::DeleteSettingStmt::appName>(ref<Expression>) +
                 lit<","> +
                 assign<&ast::DeleteSettingStmt::section>(ref<Expression>) +
                 option(lit<","> + assign<&ast::DeleteSettingStmt::key>(
                                        ref<Expression>)))> {};

  struct FileCopyStmt
      : rule<create<ast::FileCopyStmt>(
                 ikw<"FILECOPY"> +
                 assign<&ast::FileCopyStmt::source>(ref<Expression>) +
                 lit<","> +
                 assign<&ast::FileCopyStmt::dest>(ref<Expression>))> {};

  struct KillStmt
      : rule<create<ast::KillStmt>(
                 ikw<"KILL"> +
                 assign<&ast::KillStmt::path>(ref<Expression>))> {};

  struct LoadStmt
      : rule<create<ast::LoadStmt>(
                 ikw<"LOAD"> +
                 assign<&ast::LoadStmt::target>(ref<Expression>))> {};

  struct MkDirStmt
      : rule<create<ast::MkDirStmt>(
                 ikw<"MKDIR"> +
                 assign<&ast::MkDirStmt::path>(ref<Expression>))> {};

  struct NameStmt
      : rule<create<ast::NameStmt>(
                 ikw<"NAME"> +
                 assign<&ast::NameStmt::oldPath>(ref<Expression>) +
                 ikw<"AS"> +
                 assign<&ast::NameStmt::newPath>(ref<Expression>))> {};

  struct RandomizeStmt
      : rule<create<ast::RandomizeStmt>(
                 ikw<"RANDOMIZE"> +
                 option(assign<&ast::RandomizeStmt::seed>(
                     ref<Expression>)))> {};

  struct RmDirStmt
      : rule<create<ast::RmDirStmt>(
                 ikw<"RMDIR"> +
                 assign<&ast::RmDirStmt::path>(ref<Expression>))> {};

  struct SavePictureStmt
      : rule<create<ast::SavePictureStmt>(
                 ikw<"SAVEPICTURE"> +
                 assign<&ast::SavePictureStmt::picture>(ref<Expression>) +
                 lit<","> +
                 assign<&ast::SavePictureStmt::path>(ref<Expression>))> {};

  struct SaveSettingStmt
      : rule<create<ast::SaveSettingStmt>(
                 ikw<"SAVESETTING"> +
                 assign<&ast::SaveSettingStmt::appName>(ref<Expression>) +
                 lit<","> +
                 assign<&ast::SaveSettingStmt::section>(ref<Expression>) +
                 lit<","> +
                 assign<&ast::SaveSettingStmt::key>(ref<Expression>) +
                 lit<","> +
                 assign<&ast::SaveSettingStmt::value>(ref<Expression>))> {};

  struct SendKeysStmt
      : rule<create<ast::SendKeysStmt>(
                 ikw<"SENDKEYS"> +
                 assign<&ast::SendKeysStmt::keys>(ref<Expression>) +
                 option(lit<","> + assign<&ast::SendKeysStmt::wait>(
                                        ref<Expression>)))> {};

  struct SetAttrStmt
      : rule<create<ast::SetAttrStmt>(
                 ikw<"SETATTR"> +
                 assign<&ast::SetAttrStmt::path>(ref<Expression>) +
                 lit<","> +
                 assign<&ast::SetAttrStmt::attrs>(ref<Expression>))> {};

  struct TimeStmt
      : rule<create<ast::TimeStmt>(
                 ikw<"TIME"> + lit<"="> +
                 assign<&ast::TimeStmt::value>(ref<Expression>))> {};

  struct UnloadStmt
      : rule<create<ast::UnloadStmt>(
                 ikw<"UNLOAD"> +
                 assign<&ast::UnloadStmt::target>(ref<Expression>))> {};

  // -- Conditional compilation ------------------------------------------

  struct MacroElseIfClause
      : rule<create<ast::ElseIfClause>(
                 lit<"#"> + ikw<"ELSEIF"> +
                 assign<&ast::ElseIfClause::cond>(ref<Expression>) +
                 ikw<"THEN"> + BodyInto<&ast::ElseIfClause::body>)> {};

  struct MacroIfStmt
      : rule<create<ast::MacroIfStmt>(
                 lit<"#"> + ikw<"IF"> +
                 assign<&ast::MacroIfStmt::ifCond>(ref<Expression>) +
                 ikw<"THEN"> + BodyInto<&ast::MacroIfStmt::ifBody> +
                 many(append<&ast::MacroIfStmt::elseIfs>(
                     ref<MacroElseIfClause>)) +
                 option(lit<"#"> +
                        flag<&ast::MacroIfStmt::hasElse>(
                            ikw<"ELSE">) +
                        BodyInto<&ast::MacroIfStmt::elseBody>) +
                 lit<"#"> + ikw<"END"> + ikw<"IF">)> {};

  // -- Procedures -----------------------------------------------------------

  struct Procedure
      : rule<create<ast::Procedure>(
                 option(assign<&ast::Procedure::scope>(ProcVisibility)) +
                 option(flag<&ast::Procedure::isStatic>(
                     ikw<"STATIC">)) +
                 assign<&ast::Procedure::kind>(ikw<"SUB">) +
                 assign<&ast::Procedure::name>(NameText) +
                 option(assign<&ast::Procedure::typeChar>(TypeSuffix)) +
                 option(ParamListInto<&ast::Procedure::params>) +
                 BodyInto<&ast::Procedure::body> + ikw<"END"> +
                 ikw<"SUB">) |
             create<ast::Procedure>(
                 option(assign<&ast::Procedure::scope>(ProcVisibility)) +
                 option(flag<&ast::Procedure::isStatic>(
                     ikw<"STATIC">)) +
                 assign<&ast::Procedure::kind>(ikw<"FUNCTION">) +
                 assign<&ast::Procedure::name>(NameText) +
                 option(assign<&ast::Procedure::typeChar>(TypeSuffix)) +
                 option(ParamListInto<&ast::Procedure::params>) +
                 option(ikw<"AS"> + assign<&ast::Procedure::returnType>(
                                        ref<TypeExpression>)) +
                 BodyInto<&ast::Procedure::body> + ikw<"END"> +
                 ikw<"FUNCTION">) |
             create<ast::Procedure>(
                 option(assign<&ast::Procedure::scope>(ProcVisibility)) +
                 option(flag<&ast::Procedure::isStatic>(
                     ikw<"STATIC">)) +
                 assign<&ast::Procedure::kind>(ikw<"PROPERTY">) +
                 assign<&ast::Procedure::accessor>(ikw<"GET">) +
                 assign<&ast::Procedure::name>(NameText) +
                 option(assign<&ast::Procedure::typeChar>(TypeSuffix)) +
                 option(ParamListInto<&ast::Procedure::params>) +
                 option(ikw<"AS"> + assign<&ast::Procedure::returnType>(
                                        ref<TypeExpression>)) +
                 BodyInto<&ast::Procedure::body> + ikw<"END"> +
                 ikw<"PROPERTY">) |
             create<ast::Procedure>(
                 option(assign<&ast::Procedure::scope>(ProcVisibility)) +
                 option(flag<&ast::Procedure::isStatic>(
                     ikw<"STATIC">)) +
                 assign<&ast::Procedure::kind>(ikw<"PROPERTY">) +
                 assign<&ast::Procedure::accessor>(ikw<"SET"> |
                                                    ikw<"LET">) +
                 assign<&ast::Procedure::name>(NameText) +
                 option(assign<&ast::Procedure::typeChar>(TypeSuffix)) +
                 ParamListInto<&ast::Procedure::params> +
                 BodyInto<&ast::Procedure::body> + ikw<"END"> +
                 ikw<"PROPERTY">)> {};

  // Module-only forms (`Function`/`Sub`/`Property`/`Declare`/`Enum`/`Type`/
  // `Event`, `Option ...`) belong only at module level in VB6, and
  // everything else only inside a procedure body; here one shared
  // vocabulary covers both; module/procedure context restrictions are not
  // enforced here (for example, nested `Sub` declarations are accepted).
  struct StatementRule
      : rule<ref<Attribute> | ref<OptionStmt> |
             ref<DefDirective> | ref<VarDecl> | ref<ConstDecl> |
             ref<TypeDecl> | ref<EnumDecl> | ref<DeclareDecl> |
             ref<EventDecl> | ref<ImplementsDecl> | ref<Procedure> |
             ref<MacroIfStmt> |
             ref<AppActivateStmt> | ref<BeepStmt> | ref<ChDirStmt> |
             ref<ChDriveStmt> | ref<CloseStmt> | ref<DateStmt> |
             ref<DeleteSettingStmt> | ref<DoStmt> | ref<EndStmt> |
             ref<EraseStmt> | ref<ErrorStmt> | ref<ExitStmt> |
             ref<FileCopyStmt> | ref<ForEachStmt> | ref<ForStmt> |
             ref<GetStmt> | ref<GosubStmt> | ref<GotoStmt> |
             ref<SingleLineIfStmt> | ref<IfStmt> | ref<InputStmt> |
             ref<KillStmt> | ref<LetStmt> | ref<LineInputStmt> |
             ref<LoadStmt> | ref<LockStmt> | ref<LSetStmt> |
             ref<MkDirStmt> | ref<NameStmt> |
             ref<OnErrorStmt> | ref<OnGotoStmt> | ref<OnGosubStmt> |
             ref<OpenStmt> | ref<PrintStmt> | ref<PutStmt> |
             ref<RaiseEventStmt> | ref<RandomizeStmt> | ref<ReDimStmt> |
             ref<ResetStmt> | ref<ResumeStmt> | ref<ReturnStmt> |
             ref<RmDirStmt> | ref<RSetStmt> | ref<SavePictureStmt> |
             ref<SaveSettingStmt> | ref<SeekStmt> | ref<SelectCaseStmt> |
             ref<SendKeysStmt> | ref<SetAttrStmt> | ref<SetStmt> |
             ref<StopStmt> | ref<TimeStmt> | ref<UnloadStmt> |
             ref<UnlockStmt> | ref<WhileStmt> | ref<WidthStmt> |
             ref<WithStmt> | ref<WriteStmt> | ref<CallStmt>> {};

  // -- Header / design-time configuration -----------------------------------

  struct ObjectAssign
      : rule<create<ast::ObjectAssign>(
                 assign<&ast::ObjectAssign::value>(ref<STRINGLITERAL>) +
                 option(lit<";"> + assign<&ast::ObjectAssign::value2>(
                                       ref<STRINGLITERAL>)))> {};

  // A literal, signed as a number may be (`MultiUse = -1`).
  static constexpr auto ConfigValueText =
      many(cr<"+-">) +
      (ref<DATELITERAL> | ref<COLORLITERAL> | ref<OCTALLITERAL> |
       ref<DOUBLELITERAL> | ref<FILENUMBER> | ref<INTEGERLITERAL> |
       ref<STRINGLITERAL> | ikw<"TRUE"> | ikw<"FALSE"> | ikw<"NOTHING"> |
       ikw<"NULL">);

  struct ConfigLiteral : rule<ConfigValueText> {};

  struct ModuleConfigElement
      : rule<create<ast::ConfigEntry>(
                 assign<&ast::ConfigEntry::target>(
                     create<ast::NameExpr>(
                         assign<&ast::NameExpr::name>(NameText))) +
                 lit<"="> +
                 assign<&ast::ConfigEntry::value>(ref<ConfigLiteral>))> {};

  struct ModuleConfig
      : rule<create<ast::BeginBlock>(
                 ikw<"BEGIN"> +
                 some(Eos + append<&ast::BeginBlock::entries>(
                                ref<ModuleConfigElement>)) +
                 Eos + ikw<"END">)> {};

  // A design-time property value: `$"text":offset`, `{Ident}`, `^Ident`, or
  // a plain literal -- a bare `rule<>` with no directives, so it deduces to
  // the text it matched.
  struct PropertyValue
      : rule<option(lit<"$">) +
             (ConfigValueText | (lit<"{"> + NameText + lit<"}">) |
              (lit<"^"> + NameText)) +
             option(ref<FRX_OFFSET>)> {};

  // A design-time property is no code: any word names it (`Tab(0)`).
  struct ConfigAtom
      : rule<create<ast::NameExpr>(
             assign<&ast::NameExpr::name>(UnrestrictedNameText))> {};

  struct ConfigTarget : rule<ref<ConfigAtom> + PostfixChain> {};

  struct ControlBlock;
  struct BeginPropertyBlock;

  static constexpr auto ConfigBlockElement =
      append<&ast::BeginBlock::entries>(
          create<ast::ConfigEntry>(
              assign<&ast::ConfigEntry::target>(ref<ConfigTarget>) + lit<"="> +
              assign<&ast::ConfigEntry::value>(ref<PropertyValue>))) |
      append<&ast::BeginBlock::children>(ref<ControlBlock>) |
      append<&ast::BeginBlock::children>(ref<BeginPropertyBlock>);

  struct ControlBlock
      : rule<create<ast::BeginBlock>(
                 ikw<"BEGIN"> +
                 assign<&ast::BeginBlock::controlType>(ref<TypeNameText>) +
                 assign<&ast::BeginBlock::name>(NameText) +
                 some(Eos + ConfigBlockElement) + Eos + ikw<"END">)> {};

  struct BeginPropertyBlock
      : rule<create<ast::BeginBlock>(
                 flag<&ast::BeginBlock::isProperty>(
                     ikw<"BEGINPROPERTY">) +
                 assign<&ast::BeginBlock::name>(NameText) +
                 option(lit<"("> +
                        assign<&ast::BeginBlock::index>(
                            ref<INTEGERLITERAL>) +
                        lit<")">) +
                 option(assign<&ast::BeginBlock::classId>(ref<GUID>)) +
                 many(Eos + ConfigBlockElement) + Eos +
                 ikw<"ENDPROPERTY">)> {};

  // -- Module -----------------------------------------------------------

  struct ModuleRule
      : rule<create<ast::Module>(
                 BlankLines +
                 option(ikw<"VERSION"> +
                        assign<&ast::Module::version>(ref<DOUBLELITERAL>) +
                        option(flag<&ast::Module::isClass>(
                            ikw<"CLASS">)) +
                        BlankLines) +
                 many(ikw<"OBJECT"> + lit<"="> +
                      append<&ast::Module::objects>(ref<ObjectAssign>) +
                      BlankLines) +
                 option(assign<&ast::Module::controls>(
                            ref<ControlBlock>) +
                        BlankLines) +
                 option(assign<&ast::Module::config>(ref<ModuleConfig>) +
                        BlankLines) +
                 many(append<&ast::Module::body>(ref<StatementRule>),
                      SepInto<&ast::Module::body>) +
                 option(SepInto<&ast::Module::body>) + BlankLines)> {};

  using entry = ModuleRule;
  using ignored = list<WS, LINE_CONTINUATION>;
  using hidden = list<COMMENT, REMCOMMENT>;
};

}  // namespace vb6
