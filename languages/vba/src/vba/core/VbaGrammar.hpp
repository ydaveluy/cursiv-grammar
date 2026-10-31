// The vba language: its grammar. Whitespace, line continuation and comments
// are trivia; NEWLINE and `:` stay real tokens because VBA statements are
// separated by them, not by indentation or semicolons.
#pragma once

#include <cstddef>
#include <memory_resource>
#include <span>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "vba/core/ast.hpp"

namespace vba {

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

struct Vba : cursiv::GrammarSpec {
  // -- Trivia ---------------------------------------------------------------

  struct WS : terminal<some(cr<R"( \t\x19　)">)> {};

  struct LINE_CONTINUATION
      : terminal<some(cr<R"( \t\x19　)">) + lit<"_"> +
                 option(some(cr<R"( \t\x19　)">)) +
                 (lit<"\r\n"> | lit<"\n"> | lit<"\r">)> {};

  struct COMMENT : terminal<lit<"'"> + many(cr<R"(^\r\n  )">)> {};

  struct REMCOMMENT
      : terminal<ikw<"REM"> + option(some(cr<R"( \t)">) +
                                    many(cr<R"(^\r\n  )">))> {};


  struct NEWLINE
      : terminal<some(cr<R"(\r\n)"> | lit<" "> | lit<" ">)> {};

  static constexpr auto Eos = some(ref<NEWLINE> | lit<":">);
  static constexpr auto BlankLines = many(ref<NEWLINE>);

  // -- Identifiers and keywords ----------------------------------------------

  // MS-VBAL 3.3.5 `codepage-identifier`: a letter of the source's codepage
  // spells a name as an ASCII one does (`Déjà`).
  static constexpr auto IdChar = uc::L | cr<"0-9_">;
  static constexpr auto word_boundary = IdChar;
  static constexpr auto IdBody = uc::L + many(IdChar);
  static constexpr auto ForeignBody =
      lit<"["> + some(cr<R"(^\r\n  )">) + lit<"]">;

  // A bracketed name, spelling anything a plain identifier cannot (a space, a
  // reserved word, a quote): `variable.[foreign name can be weird]`.
  struct FOREIGN_NAME : terminal<ForeignBody> {};

  // Keywords that stay legal as a plain name (MS-VBAL `ambiguousKeyword`):
  // spelled once, so a state reading a name accepts them as text and a state
  // reading the keyword accepts the same spelling -- `grammar.md`'s
  // soft-keyword rule.
  static constexpr auto AmbiguousKeyword =
      ikw<"ACCESS"> | ikw<"ALIAS"> | ikw<"APPACTIVATE"> | ikw<"APPEND"> |
      ikw<"ASSERT"> | ikw<"BASE"> | ikw<"BEGINPROPERTY"> | ikw<"BEGIN"> |
      ikw<"BINARY"> | ikw<"CLASS_INITIALIZE"> | ikw<"CLASS_TERMINATE"> |
      ikw<"CLASS"> | ikw<"CHDIR"> | ikw<"CHDRIVE"> | ikw<"COLLECTION"> |
      ikw<"COMPARE"> | ikw<"DATABASE"> | ikw<"DELETESETTING"> |
      ikw<"ENDPROPERTY"> | ikw<"ERROR"> | ikw<"FILECOPY"> | ikw<"GO"> |
      ikw<"KILL"> | ikw<"LOAD"> | ikw<"LIB"> | ikw<"LINE"> | ikw<"MIDB$"> |
      ikw<"MIDB"> | ikw<"MID$"> | ikw<"MID"> | ikw<"MKDIR"> | ikw<"MODULE"> |
      ikw<"NAME"> | ikw<"OBJECT"> | ikw<"OUTPUT"> | ikw<"PROPERTY"> |
      ikw<"RANDOMIZE"> | ikw<"RANDOM"> | ikw<"READ"> | ikw<"RESET"> |
      ikw<"RMDIR"> | ikw<"SAVEPICTURE"> | ikw<"SAVESETTING"> |
      ikw<"SENDKEYS"> | ikw<"SETATTR"> | ikw<"STEP"> | ikw<"TEXT"> |
      ikw<"TIME"> | ikw<"UNLOAD"> | ikw<"VERSION"> | ikw<"WIDTH">;

  static constexpr auto StatementKeyword =
      ikw<"CALL"> | ikw<"CASE"> | ikw<"CLOSE"> | ikw<"CONST"> |
      ikw<"DECLARE"> | ikw<"DEFBOOL"> | ikw<"DEFBYTE"> | ikw<"DEFCUR"> |
      ikw<"DEFDATE"> | ikw<"DEFDBL"> | ikw<"DEFINT"> | ikw<"DEFLNGLNG"> |
      ikw<"DEFLNGPTR"> | ikw<"DEFLNG"> | ikw<"DEFOBJ"> | ikw<"DEFSNG"> |
      ikw<"DEFSTR"> | ikw<"DEFVAR"> | ikw<"DIM"> | ikw<"DO"> | ikw<"ELSEIF"> |
      ikw<"ELSE"> | ikw<"ENDIF"> | ikw<"END"> | ikw<"ENUM"> | ikw<"ERASE"> |
      ikw<"EVENT"> | ikw<"EXIT"> | ikw<"FOR"> | ikw<"FRIEND"> |
      ikw<"FUNCTION"> | ikw<"GET"> | ikw<"GLOBAL"> | ikw<"GOSUB"> |
      ikw<"GOTO"> | ikw<"IF"> | ikw<"IMPLEMENTS"> | ikw<"INPUT"> |
      ikw<"LET"> | ikw<"LOCK"> | ikw<"LOOP"> | ikw<"LSET"> | ikw<"NEXT"> |
      ikw<"ON"> | ikw<"OPEN"> | ikw<"OPTION"> | ikw<"PRINT"> |
      ikw<"PRIVATE"> | ikw<"PUBLIC"> | ikw<"PUT"> | ikw<"RAISEEVENT"> |
      ikw<"REDIM"> | ikw<"RESUME"> | ikw<"RETURN"> | ikw<"RSET"> |
      ikw<"SEEK"> | ikw<"SELECT"> | ikw<"SET"> | ikw<"STATIC"> |
      ikw<"STOP"> | ikw<"SUB"> | ikw<"TYPE"> | ikw<"UNLOCK"> | ikw<"WEND"> |
      ikw<"WHILE"> | ikw<"WITH"> | ikw<"WRITE">;

  static constexpr auto MarkerKeyword =
      ikw<"ANY"> | ikw<"AS"> | ikw<"BYREF"> | ikw<"BYVAL"> | ikw<"CASE"> |
      ikw<"EACH"> | ikw<"ELSE"> | ikw<"IN"> | ikw<"NEW"> | ikw<"SHARED"> |
      ikw<"UNTIL"> | ikw<"WITHEVENTS"> | ikw<"WRITE"> | ikw<"OPTIONAL"> |
      ikw<"PARAMARRAY"> | ikw<"PRESERVE"> | ikw<"SPC"> | ikw<"TAB"> |
      ikw<"THEN"> | ikw<"TO">;

  static constexpr auto OperatorIdentifier =
      ikw<"ADDRESSOF"> | ikw<"AND"> | ikw<"EQV"> | ikw<"IMP"> | ikw<"IS"> |
      ikw<"LIKE"> | ikw<"NEW"> | ikw<"MOD"> | ikw<"NOT"> | ikw<"OR"> |
      ikw<"TYPEOF"> | ikw<"XOR">;

  // `ME` is excluded here and added back in `ReservedName`: `Primary` gives
  // it its own node (`MeExpr`), so a plain name reading must not also offer
  // it -- two alternatives spelling one token into different classes is
  // refused (`twinProductions`).
  static constexpr auto ReservedNameNoMe =
      ikw<"ABS"> | ikw<"CBOOL"> | ikw<"CBYTE"> | ikw<"CCUR"> | ikw<"CDATE"> |
      ikw<"CDBL"> | ikw<"CDEC"> | ikw<"CINT"> | ikw<"CLNGLNG"> |
      ikw<"CLNGPTR"> | ikw<"CLNG"> | ikw<"CSNG"> | ikw<"CSTR"> | ikw<"CVAR"> |
      ikw<"CVERR"> | ikw<"DATE"> | ikw<"DOEVENTS"> | ikw<"FIX"> |
      ikw<"INT"> | ikw<"LENB"> | ikw<"LEN"> | ikw<"PSET"> | ikw<"SCALE"> |
      ikw<"SGN"> | ikw<"STRING">;

  static constexpr auto ReservedName =
      ReservedNameNoMe | ikw<"DEBUG"> | ikw<"ME">;

  static constexpr auto SpecialFormNoInput =
      ikw<"ARRAY"> | ikw<"CIRCLE"> | ikw<"INPUTB"> | ikw<"LBOUND"> |
      ikw<"SCALE"> | ikw<"UBOUND">;
  static constexpr auto SpecialForm = SpecialFormNoInput | ikw<"INPUT">;

  static constexpr auto ReservedTypeIdentifier =
      ikw<"BOOLEAN"> | ikw<"BYTE"> | ikw<"CURRENCY"> | ikw<"DATE"> |
      ikw<"DOUBLE"> | ikw<"INTEGER"> | ikw<"LONGLONG"> | ikw<"LONGPTR"> |
      ikw<"LONG"> | ikw<"SINGLE"> | ikw<"STRING"> | ikw<"VARIANT">;

  static constexpr auto LiteralIdentifier =
      ikw<"TRUE"> | ikw<"FALSE"> | ikw<"NOTHING"> | ikw<"EMPTY"> | ikw<"NULL">;

  static constexpr auto ReservedForImplementationUse =
      ikw<"ATTRIBUTE"> | ikw<"LINEINPUT"> | ikw<"VB_BASE"> |
      ikw<"VB_CONTROL"> | ikw<"VB_CREATABLE"> | ikw<"VB_CUSTOMIZABLE"> |
      ikw<"VB_DESCRIPTION"> | ikw<"VB_EXPOSED"> | ikw<"VB_EXT_KEY"> |
      ikw<"VB_GLOBALNAMESPACE"> | ikw<"VB_HELPID"> | ikw<"VB_INVOKE_FUNC"> |
      ikw<"VB_INVOKE_PROPERTYPUTREF"> | ikw<"VB_INVOKE_PROPERTYPUT"> |
      ikw<"VB_INVOKE_PROPERTY"> | ikw<"VB_MEMBERFLAGS"> | ikw<"VB_NAME"> |
      ikw<"VB_PREDECLAREDID"> | ikw<"VB_PROCDATA"> |
      ikw<"VB_TEMPLATEDERIVED"> | ikw<"VB_USERMEMID"> |
      ikw<"VB_VARDESCRIPTION"> | ikw<"VB_VARHELPID"> |
      ikw<"VB_VARMEMBERFLAGS"> | ikw<"VB_VARPROCDATA"> |
      ikw<"VB_VARUSERMEMID">;

  static constexpr auto FutureReserved =
      ikw<"CDECL"> | ikw<"DECIMAL"> | ikw<"DEFDEC">;

  // The words an `ID` refuses: those that open, close or part a statement or
  // an expression, which a name would otherwise stand in for.
  static constexpr auto ReservedWord =
      StatementKeyword | MarkerKeyword | OperatorIdentifier | SpecialForm |
      ikw<"ME"> | ikw<"REM">;

  struct ID : terminal<!ReservedWord + IdBody> {};

  // A name and its `:` in one token, so a label never forks with a call: an
  // `IDENTIFIER`, never `Randomize` (MS-VBAL 5.4.1.1).
  struct LABEL
      : terminal<as<label_name>(!ReservedWord + !ikw<"RANDOMIZE"> +
                                (IdBody | ForeignBody) + many(cr<R"( \t)">) +
                                lit<":", !cr<"=">>)> {};

  // A name: an ID, or one of the words that stay legal as one.
  struct NameTextRule
      : rule<ref<ID> | AmbiguousKeyword | ref<FOREIGN_NAME>> {};
  static constexpr auto NameText = ref<NameTextRule>;

  // A name usable after `.`/`!`: every reserved word besides `NameText`'s own
  // set is legal there too (`unrestrictedName`). `REM` is excluded: it is
  // always read as a comment.
  struct UnrestrictedNameTextRule
      : rule<NameText | StatementKeyword | MarkerKeyword | OperatorIdentifier |
             SpecialForm | ReservedName | ReservedTypeIdentifier |
             LiteralIdentifier | ReservedForImplementationUse | FutureReserved> {};
  static constexpr auto UnrestrictedNameText = ref<UnrestrictedNameTextRule>;

  static constexpr auto TypeSuffixClass = cr<"&%#!@$^">;

  // MS-VBAL 3.3.5.3 `TYPED-NAME`: the suffix glued to its name, and a `!`
  // glued to a name on both sides a dictionary access, never a suffix.
  static constexpr auto TypeSuffix =
      cr<"&%#@$^"> | lit<"!", !(uc::L | cr<"[">)>;

  static constexpr auto GluedSuffix = option(gap::none + TypeSuffix);

  template <auto Member>
  static constexpr auto TypeSuffixInto =
      option(gap::none + assign<Member>(TypeSuffix));

  // -- Literal terminals ------------------------------------------------------

  static constexpr auto HEXDIGIT = cr<"0-9a-fA-F">;

  struct GUID : terminal<lit<"{"> + repeat<8>(HEXDIGIT) + lit<"-"> +
                          repeat<4>(HEXDIGIT) + lit<"-"> +
                          repeat<4>(HEXDIGIT) + lit<"-"> +
                          repeat<4>(HEXDIGIT) + lit<"-"> +
                          repeat<12>(HEXDIGIT) + lit<"}">> {};

  static constexpr auto StringBody =
      lit<"\""> + many(lit<"\"\""> | cr<R"(^"\r\n)">) + lit<"\"">;

  struct STRINGLITERAL : terminal<as<undouble_dq>(StringBody)> {};

  struct FILEOFFSET : terminal<option(lit<"$">) + StringBody + lit<":"> +
                                some(HEXDIGIT)> {};

  struct INTEGERLITERAL
      : terminal<(some(d) | (lit<"&"> + cr<"Hh"> + some(HEXDIGIT)) |
                  (lit<"&"> + option(cr<"Oo">) + some(cr<"0-7">))) +
                 option(TypeSuffixClass)> {};

  static constexpr auto DecimalLit = some(d);
  static constexpr auto Exponent = cr<"DEde"> + option(cr<"+-">) + DecimalLit;
  static constexpr auto FloatBody =
      (DecimalLit + Exponent) |
      (DecimalLit + lit<"."> + option(DecimalLit) + option(Exponent)) |
      (lit<"."> + DecimalLit + option(Exponent));

  struct FLOATLITERAL
      : terminal<(FloatBody + option(cr<"!#@">)) | (DecimalLit + cr<"!#@">)> {
  };

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

  // -- Expressions ------------------------------------------------------------

  struct Expression;
  struct LExpression;
  struct NotExpr;
  struct ArithChain;
  struct NegExpr;
  struct PowExpr;
  struct AtomExpr;
  struct Postfix;
  struct Primary;
  struct WithExpr;
  struct TypeExpression;
  struct SameLineStmt;

  struct Literal
      : rule<create<ast::DateLit>(
                 assign<&ast::DateLit::text>(ref<DATELITERAL>)) |
             create<ast::FloatLit>(
                 assign<&ast::FloatLit::text>(ref<FLOATLITERAL>)) |
             create<ast::IntLit>(
                 assign<&ast::IntLit::text>(ref<INTEGERLITERAL>)) |
             assign<&ast::StringLit::value>(ref<STRINGLITERAL>) |
             (create<ast::BoolLit>((flag<&ast::BoolLit::value>(
                                        ikw<"TRUE">) |
                                    ikw<"FALSE">)) +
              GluedSuffix) |
             (create<ast::NothingLit> + ikw<"NOTHING"> + GluedSuffix) |
             (create<ast::EmptyLit> + ikw<"EMPTY"> + GluedSuffix) |
             (create<ast::NullLit> + ikw<"NULL"> + GluedSuffix)> {};

  static constexpr auto SimpleName = assign<&ast::NameExpr::name>(
      NameText | ReservedNameNoMe | SpecialFormNoInput);

  // `Debug.Print`, `Debug.Assert` and `Input #` are statements of their own
  // (MS-VBAL 5.4.5.8, 5.4.2.23, 5.4.5.10): `Debug` heads no expression, and
  // `Input` only a special form's arguments.
  struct Primary : rule<(lit<"("> + ref<Expression> + lit<")">) |
                        create<ast::MeExpr>(ikw<"ME">) |
                        create<ast::NameExpr>(
                            SimpleName +
                            TypeSuffixInto<&ast::NameExpr::typeChar>) |
                        (create<ast::NameExpr>(
                             assign<&ast::NameExpr::name>(ikw<"INPUT">) +
                             TypeSuffixInto<&ast::NameExpr::typeChar>) +
                         &lit<"(">)> {};

  // The argument of a call or index. `Arg::value` is null for an omitted
  // positional slot (`Foo(1, , 3)`).
  // A parenthesized argument may be a marked file number (`Input(5, #1)`),
  // a bare call's never: `Width #1, 80` is the file statement.
  struct MarkedArgExpr : rule<option(lit<"#">) + ref<Expression>> {};
  struct ArgExpr : rule<ref<Expression>> {};

  template <auto Value>
  static constexpr auto ArgBodyOf =
      create<ast::Arg>(option(flag<&ast::Arg::byVal>(ikw<"BYVAL">)) +
                        option(assign<&ast::Arg::value>(Value))) |
      create<ast::Arg>(flag<&ast::Arg::addressOf>(ikw<"ADDRESSOF">) +
                        assign<&ast::Arg::value>(ref<Expression>));

  static constexpr auto NamedArgBody =
      create<ast::Arg>(assign<&ast::Arg::name>(UnrestrictedNameText) +
                        lit<":="> + assign<&ast::Arg::value>(ref<Expression>));

  template <auto Value>
  static constexpr auto FilledArgOf =
      create<ast::Arg>(flag<&ast::Arg::byVal>(ikw<"BYVAL">) +
                        option(assign<&ast::Arg::value>(Value))) |
      create<ast::Arg>(assign<&ast::Arg::value>(Value)) |
      create<ast::Arg>(flag<&ast::Arg::addressOf>(ikw<"ADDRESSOF">) +
                        assign<&ast::Arg::value>(ref<Expression>));

  template <auto Member, auto Value>
  static constexpr auto NextArgInto =
      lit<","> + (append<Member>(ArgBodyOf<Value>) |
                  append<Member>(NamedArgBody));

  // A lone slot is never empty: `f()` holds no `Arg`.
  template <auto Member, auto Value = ref<MarkedArgExpr>>
  static constexpr auto ArgListInto =
      option((append<Member>(ArgBodyOf<Value>) |
              append<Member>(NamedArgBody)) +
                 some(NextArgInto<Member, Value>)  //
             | append<Member>(FilledArgOf<Value>) |
             append<Member>(NamedArgBody));

  // `lExpression`: names and the chains of `.`/`!`/`(...)` that follow them,
  // folded by hand into a loop since each may repeat and interleave. The
  // source's separate "after an explicit line continuation" spellings of `.`
  // and `!` need no mirror here: `LINE_CONTINUATION` is trivia, so a plain
  // `lit<".">`/`lit<"!">` already matches on either side of one.
  struct LExpression
      : rule<ref<Primary> +
             many(nest<ast::MemberAccessExpr, &ast::MemberAccessExpr::object>(
                      lit<"."> + assign<&ast::MemberAccessExpr::name>(
                                     UnrestrictedNameText)) |
                  nest<ast::DictionaryAccessExpr,
                       &ast::DictionaryAccessExpr::object>(
                      lit<"!"> + assign<&ast::DictionaryAccessExpr::name>(
                                     UnrestrictedNameText)) |
                  nest<ast::IndexExpr, &ast::IndexExpr::target>(
                      lit<"("> + ArgListInto<&ast::IndexExpr::args> +
                      lit<")">))> {};

  // `.field` / `!field` with no object: the implicit target inside a `With`
  // block.
  struct WithExpr
      : rule<create<ast::MemberAccessExpr>(
                 lit<"."> +
                 assign<&ast::MemberAccessExpr::name>(UnrestrictedNameText)) |
             create<ast::DictionaryAccessExpr>(
                 lit<"!"> + assign<&ast::DictionaryAccessExpr::name>(
                                UnrestrictedNameText))> {};

  // A literal is no l-expression (MS-VBAL): no call, assignment, member or
  // index opens on it.
  struct Postfix : rule<ref<WithExpr> | ref<LExpression> | ref<Literal>> {};

  // `New` and `TypeOf...Is` are not in the documented operator table
  // and are kept tight, like a primary.
  struct AtomExpr
      : rule<create<ast::NewExpr>(
                 ikw<"NEW"> + assign<&ast::NewExpr::operand>(ref<AtomExpr>)) |
             create<ast::TypeOfIsExpr>(
                 ikw<"TYPEOF"> +
                 assign<&ast::TypeOfIsExpr::operand>(ref<AtomExpr>) +
                 ikw<"IS"> + assign<&ast::TypeOfIsExpr::typeName>(
                                 ref<TypeExpression>)) |
             ref<Postfix>> {};

  // Negation is looser than exponentiation on the left (`-2^2` is `-(2^2)`,
  // not `(-2)^2`) but `^`'s own right operand may start with one
  // directly (`2^-2`). Repeated powers associate left: `(2^3)^2`.
  struct SignedPowerAtom : rule<create<ast::UnaryExpr>(assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-">) + assign<&ast::UnaryExpr::operand>(ref<SignedPowerAtom>)) | ref<AtomExpr>> {};
  struct PowExpr : rule<ref<AtomExpr> + many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(assign<&ast::BinaryExpr::op>(lit<"^">) + assign<&ast::BinaryExpr::right>(ref<SignedPowerAtom>)))> {};
  struct NegExpr : rule<create<ast::UnaryExpr>(assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-">) + assign<&ast::UnaryExpr::operand>(ref<NegExpr>)) | ref<PowExpr>> {};

  static constexpr auto OpMul = lit<"/"> | lit<"*">;
  static constexpr auto OpIntDiv = lit<"\\">;
  static constexpr auto OpModText = ikw<"MOD">;
  static constexpr auto OpAdd = lit<"+"> | lit<"-">;
  static constexpr auto OpConcat = lit<"&">;
  static constexpr auto OpRel = ikw<"IS"> | ikw<"LIKE"> | lit<">="> |
                                 lit<"=>"> | lit<"<="> | lit<"=<"> |
                                 lit<"<>"> | lit<"><"> | lit<">"> | lit<"<"> |
                                 lit<"=">;

  // */, \, Mod are each their own level, tightest to loosest.
  struct ArithChain
      : infix<NegExpr, ast::BinaryExpr,
              levels(left(OpMul), left(OpIntDiv), left(OpModText),
                     left(OpAdd), left(OpConcat), left(OpRel))> {};

  // Not: tighter than And, looser than a comparison.
  struct NotExpr
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ikw<"NOT">) +
                 assign<&ast::UnaryExpr::operand>(ref<NotExpr>))  //
             | ref<ArithChain>> {};

  static constexpr auto OpAnd = ikw<"AND">;
  static constexpr auto OpOr = ikw<"OR">;
  static constexpr auto OpXor = ikw<"XOR">;
  static constexpr auto OpEqv = ikw<"EQV">;
  static constexpr auto OpImp = ikw<"IMP">;

  // And, Or, Xor, Eqv, Imp are each their own level, tightest to loosest
  // -- unlike Comparison, whose members are equal.
  struct Expression
      : infix<NotExpr, ast::BinaryExpr,
              levels(left(OpAnd), left(OpOr), left(OpXor), left(OpEqv),
                     left(OpImp))> {};

  // -- Types --------------------------------------------------------------

  static constexpr auto BuiltinTypeWord = ReservedTypeIdentifier | ikw<"OBJECT">;

  struct QualifiedTypeName : rule<some(ref<ID>, lit<".">)> {};

  // Its own rule (not spliced into `assign<>` directly), like `QualifiedName`
  // in the `sql` example: a plain `rule<>` with no directives deduces to the
  // text it matched, brackets included in its span.
  struct BuiltinTypeName
      : rule<(lit<"["> + BuiltinTypeWord + lit<"]">) | BuiltinTypeWord> {};

  static constexpr auto TypeNameText =
      ref<BuiltinTypeName> | ref<QualifiedTypeName>;

  static constexpr auto StringLenExpr =
      create<ast::IntLit>(assign<&ast::IntLit::text>(ref<INTEGERLITERAL>)) |
      create<ast::NameExpr>(assign<&ast::NameExpr::name>(NameText));

  struct TypeExpression
      : rule<create<ast::TypeRef>(
                 assign<&ast::TypeRef::name>(ikw<"STRING">) + lit<"*"> +
                 assign<&ast::TypeRef::stringLength>(StringLenExpr)) |
             create<ast::TypeRef>(assign<&ast::TypeRef::name>(TypeNameText))> {
  };

  struct ParamTypeExpression
      : rule<ref<TypeExpression> | create<ast::TypeRef>(
                 assign<&ast::TypeRef::name>(ikw<"ANY">))> {};

  // -- Shared list helpers --------------------------------------------------

  struct ArrayBound
      : rule<option(assign<&ast::ArrayBound::lower>(ref<Expression>) +
                    ikw<"TO">) +
             assign<&ast::ArrayBound::upper>(ref<Expression>)> {};

  template <auto Member>
  static constexpr auto ArrayDimInto =
      lit<"("> + option(some(append<Member>(ref<ArrayBound>), lit<",">)) +
      lit<")">;

  static constexpr auto OptionalPrefix =
      (flag<&ast::Param::optionalP>(ikw<"OPTIONAL">) +
       option(flag<&ast::Param::byRef>(ikw<"BYREF">) |
              flag<&ast::Param::byVal>(ikw<"BYVAL">))) |
      ((flag<&ast::Param::byRef>(ikw<"BYREF">) |
        flag<&ast::Param::byVal>(ikw<"BYVAL">)) +
       flag<&ast::Param::optionalP>(ikw<"OPTIONAL">));

  struct Param
      : rule<create<ast::Param>(
                 flag<&ast::Param::paramArray>(ikw<"PARAMARRAY">) +
                 assign<&ast::Param::name>(NameText) +
                 flag<&ast::Param::isArray>(lit<"("> + lit<")">) +
                 option(ikw<"AS"> +
                        ((lit<"["> +
                          assign<&ast::Param::asType>(ref<TypeExpression>) +
                          lit<"]">) |
                         assign<&ast::Param::asType>(ref<TypeExpression>)))) |
             create<ast::Param>(
                 OptionalPrefix + assign<&ast::Param::name>(NameText) +
                 TypeSuffixInto<&ast::Param::typeChar> +
                 option(flag<&ast::Param::isArray>(lit<"("> +
                                                         lit<")">)) +
                 option(ikw<"AS"> +
                        assign<&ast::Param::asType>(ref<ParamTypeExpression>)) +
                 option(lit<"="> + assign<&ast::Param::defaultValue>(
                                        ref<Expression>))) |
             create<ast::Param>(
                 (flag<&ast::Param::byRef>(ikw<"BYREF">) |
                  flag<&ast::Param::byVal>(ikw<"BYVAL">)) +
                 assign<&ast::Param::name>(NameText) +
                 TypeSuffixInto<&ast::Param::typeChar> +
                 option(flag<&ast::Param::isArray>(lit<"("> +
                                                         lit<")">)) +
                 option(ikw<"AS"> +
                        assign<&ast::Param::asType>(ref<ParamTypeExpression>))) |
             create<ast::Param>(
                 assign<&ast::Param::name>(NameText) +
                 TypeSuffixInto<&ast::Param::typeChar> +
                 option(flag<&ast::Param::isArray>(lit<"("> +
                                                         lit<")">)) +
                 option(ikw<"AS"> +
                        assign<&ast::Param::asType>(ref<ParamTypeExpression>)))> {};

  template <auto Member>
  static constexpr auto ParamListInto =
      lit<"("> + option(some(append<Member>(ref<Param>), lit<",">)) +
      lit<")">;

  struct BodyItem;
  struct LabelStmt;
  struct LineNumberLabel;

  // A line number may share its line with the statement or the closer after
  // it.
  template <auto Member>
  static constexpr auto ItemInto =
      (append<Member>(ref<LineNumberLabel>) +
       option(append<Member>(ref<BodyItem>))) |
      append<Member>(ref<BodyItem>);

  // A name and its `:` opening a line are a label, never a call (MS-VBAL
  // 5.4.1): only a line break stands before one.
  template <auto Member>
  static constexpr auto SepInto =
      some((ref<NEWLINE> + option(append<Member>(ref<LabelStmt>))) |
           lit<":">);

  // MS-VBAL `statement-block`, each statement ended by its separator: a
  // block's closer is read where its next statement could open, never as a
  // name of one.
  template <auto Member>
  static constexpr auto BodyInto =
      SepInto<Member> + many(ItemInto<Member> + SepInto<Member>) +
      option(append<Member>(ref<LineNumberLabel>));

  // -- Variable, constant, type and enum declarations -----------------------

  struct VarDeclItem
      : rule<create<ast::VarDeclItem>(
                 flag<&ast::VarDeclItem::withEvents>(ikw<"WITHEVENTS">) +
                 assign<&ast::VarDeclItem::name>(NameText) + ikw<"AS"> +
                 assign<&ast::VarDeclItem::asType>(ref<TypeExpression>)) |
             create<ast::VarDeclItem>(
                 assign<&ast::VarDeclItem::name>(NameText) +
                 TypeSuffixInto<&ast::VarDeclItem::typeChar> +
                 option(flag<&ast::VarDeclItem::isArray>(
                     ArrayDimInto<&ast::VarDeclItem::bounds>)) +
                 option((ikw<"AS"> +
                         flag<&ast::VarDeclItem::asNew>(ikw<"NEW">) +
                         assign<&ast::VarDeclItem::asType>(
                             ref<TypeExpression>)) |
                        (ikw<"AS"> + assign<&ast::VarDeclItem::asType>(
                                         ref<TypeExpression>))))> {};

  struct VarDecl
      : rule<create<ast::VarDecl>(
                 assign<&ast::VarDecl::scope>(
                     ikw<"GLOBAL"> | ikw<"PUBLIC"> | ikw<"PRIVATE"> |
                     ikw<"DIM"> | ikw<"STATIC">) +
                 option(flag<&ast::VarDecl::shared>(ikw<"SHARED">)) +
                 some(append<&ast::VarDecl::items>(ref<VarDeclItem>),
                      lit<",">))> {};

  struct ConstDeclItem
      : rule<create<ast::ConstDeclItem>(
                 assign<&ast::ConstDeclItem::name>(NameText) +
                 TypeSuffixInto<&ast::ConstDeclItem::typeChar> +
                 option(ikw<"AS"> + assign<&ast::ConstDeclItem::asType>(
                                        ref<TypeExpression>)) +
                 lit<"="> +
                 assign<&ast::ConstDeclItem::value>(ref<Expression>))> {};

  struct ConstDecl
      : rule<create<ast::ConstDecl>(
                 option(assign<&ast::ConstDecl::scope>(
                     ikw<"GLOBAL"> | ikw<"PUBLIC"> | ikw<"PRIVATE">)) +
                 ikw<"CONST"> +
                 some(append<&ast::ConstDecl::items>(ref<ConstDeclItem>),
                      lit<",">))> {};

  struct CompilationConst
      : rule<lit<"#"> + ikw<"CONST"> +
             assign<&ast::CompilationConst::name>(NameText) +
             TypeSuffixInto<&ast::CompilationConst::typeChar> + lit<"="> +
             assign<&ast::CompilationConst::value>(ref<Expression>) +
             &(ref<NEWLINE> | eof)> {};

  struct UdtMember
      : rule<create<ast::UdtMember>(
                 assign<&ast::UdtMember::name>(UnrestrictedNameText) +
                 option(flag<&ast::UdtMember::isArray>(
                     ArrayDimInto<&ast::UdtMember::bounds>)) +
                 ikw<"AS"> +
                 assign<&ast::UdtMember::asType>(ref<TypeExpression>))> {};

  struct TypeDecl
      : rule<create<ast::TypeDecl>(
                 option(assign<&ast::TypeDecl::scope>(
                     ikw<"GLOBAL"> | ikw<"PUBLIC"> | ikw<"PRIVATE">)) +
                 ikw<"TYPE"> + assign<&ast::TypeDecl::name>(NameText) + Eos +
                 some(append<&ast::TypeDecl::members>(ref<UdtMember>),
                      Eos) +
                 Eos + ikw<"END"> + ikw<"TYPE">)> {};

  struct EnumMember
      : rule<create<ast::EnumMember>(
                 assign<&ast::EnumMember::name>(NameText) +
                 option(lit<"="> +
                        assign<&ast::EnumMember::value>(ref<Expression>)))> {
  };

  struct EnumDecl
      : rule<create<ast::EnumDecl>(
                 option(assign<&ast::EnumDecl::scope>(
                     ikw<"GLOBAL"> | ikw<"PUBLIC"> | ikw<"PRIVATE">)) +
                 ikw<"ENUM"> + assign<&ast::EnumDecl::name>(NameText) + Eos +
                 some(append<&ast::EnumDecl::members>(ref<EnumMember>),
                      Eos) +
                 Eos + ikw<"END"> + ikw<"ENUM">)> {};

  struct DeclareDecl
      : rule<create<ast::DeclareDecl>(
                 option(assign<&ast::DeclareDecl::scope>(
                     ikw<"PUBLIC"> | ikw<"PRIVATE">)) +
                 ikw<"DECLARE"> +
                 option(flag<&ast::DeclareDecl::ptrSafe>(
                     ikw<"PTRSAFE">)) +
                 ((flag<&ast::DeclareDecl::isFunction>(
                       ikw<"FUNCTION">) +
                   assign<&ast::DeclareDecl::name>(NameText) + ikw<"LIB"> +
                   assign<&ast::DeclareDecl::lib>(ref<STRINGLITERAL>) +
                   option(ikw<"ALIAS"> + assign<&ast::DeclareDecl::alias>(
                                             ref<STRINGLITERAL>)) +
                   option(ParamListInto<&ast::DeclareDecl::params>) +
                   option(ikw<"AS"> + assign<&ast::DeclareDecl::returnType>(
                                          ref<TypeExpression>))) |
                  (ikw<"SUB"> + assign<&ast::DeclareDecl::name>(NameText) +
                   ikw<"LIB"> + assign<&ast::DeclareDecl::lib>(
                                    ref<STRINGLITERAL>) +
                   option(ikw<"ALIAS"> + assign<&ast::DeclareDecl::alias>(
                                             ref<STRINGLITERAL>)) +
                   option(ParamListInto<&ast::DeclareDecl::params>))))> {};

  struct EventDecl
      : rule<create<ast::EventDecl>(
                 option(flag<&ast::EventDecl::isPublic>(
                     ikw<"PUBLIC">)) +
                 ikw<"EVENT"> + assign<&ast::EventDecl::name>(NameText) +
                 option(ParamListInto<&ast::EventDecl::params>))> {};

  struct ImplementsDecl
      : rule<create<ast::ImplementsDecl>(
                 ikw<"IMPLEMENTS"> +
                 assign<&ast::ImplementsDecl::typeName>(TypeNameText))> {};

  struct OptionStmt
      : rule<create<ast::OptionStmt>(
                 ikw<"OPTION"> + assign<&ast::OptionStmt::kind>(ikw<"COMPARE">) +
                 assign<&ast::OptionStmt::value>(ikw<"BINARY"> |
                                                  ikw<"TEXT">)) |
             create<ast::OptionStmt>(
                 ikw<"OPTION"> + assign<&ast::OptionStmt::kind>(ikw<"BASE">) +
                 assign<&ast::OptionStmt::value>(ref<INTEGERLITERAL>)) |
             create<ast::OptionStmt>(
                 ikw<"OPTION"> +
                 assign<&ast::OptionStmt::kind>(ikw<"EXPLICIT">)) |
             create<ast::OptionStmt>(
                 ikw<"OPTION"> + assign<&ast::OptionStmt::kind>(
                                     ikw<"PRIVATE">) +
                 ikw<"MODULE">)> {};

  struct LetterSpec
      : rule<create<ast::LetterSpec>(
                 assign<&ast::LetterSpec::from>(NameText) + lit<"-"> +
                 assign<&ast::LetterSpec::to>(NameText)) |
             create<ast::LetterSpec>(
                 assign<&ast::LetterSpec::from>(NameText))> {};

  struct DefDirective
      : rule<create<ast::DefDirective>(
                 assign<&ast::DefDirective::type>(
                     ikw<"DEFBOOL"> | ikw<"DEFBYTE"> | ikw<"DEFCUR"> |
                     ikw<"DEFDATE"> | ikw<"DEFDBL"> | ikw<"DEFINT"> |
                     ikw<"DEFLNGLNG"> | ikw<"DEFLNGPTR"> | ikw<"DEFLNG"> |
                     ikw<"DEFOBJ"> | ikw<"DEFSNG"> | ikw<"DEFSTR"> |
                     ikw<"DEFVAR">) +
                 some(append<&ast::DefDirective::letters>(ref<LetterSpec>),
                      lit<",">))> {};

  struct AttributeMember
      : rule<UnrestrictedNameText + option(lit<"."> + UnrestrictedNameText)> {
  };

  struct AttributeValue
      : rule<(option(lit<"-">) + ref<INTEGERLITERAL>) | ref<STRINGLITERAL> |
             LiteralIdentifier> {};

  struct Attribute
      : rule<create<ast::Attribute>(
                 ikw<"ATTRIBUTE"> + assign<&ast::Attribute::owner>(NameText) +
                 lit<"."> +
                 assign<&ast::Attribute::member>(ref<AttributeMember>) +
                 lit<"="> +
                 assign<&ast::Attribute::value>(ref<AttributeValue>)) |
             create<ast::Attribute>(
                 ikw<"ATTRIBUTE"> +
                 assign<&ast::Attribute::member>(ikw<"VB_NAME">) + lit<"="> +
                 assign<&ast::Attribute::value>(ref<STRINGLITERAL>)) |
             // The class-header attributes: no owner, a boolean value.
             create<ast::Attribute>(
                 ikw<"ATTRIBUTE"> +
                 assign<&ast::Attribute::member>(
                     ikw<"VB_GLOBALNAMESPACE"> | ikw<"VB_CREATABLE"> |
                     ikw<"VB_PREDECLAREDID"> | ikw<"VB_EXPOSED"> |
                     ikw<"VB_CUSTOMIZABLE">) +
                 lit<"="> +
                 assign<&ast::Attribute::value>(LiteralIdentifier))> {};

  // -- Statements ---------------------------------------------------------

  static constexpr auto CallTarget = ref<WithExpr> | ref<LExpression>;

  struct CallStmt
      : rule<create<ast::CallStmt>(
                 ikw<"CALL"> + assign<&ast::CallStmt::target>(CallTarget)) |
             create<ast::CallStmt>(
                 assign<&ast::CallStmt::target>(CallTarget) +
                 option(ArgListInto<&ast::CallStmt::args, ref<ArgExpr>>))> {};

  struct AssignStmt
      : rule<create<ast::AssignStmt>(
                 flag<&ast::AssignStmt::isSet>(ikw<"SET">) +
                 assign<&ast::AssignStmt::target>(CallTarget) + lit<"="> +
                 assign<&ast::AssignStmt::value>(ref<Expression>)) |
             create<ast::AssignStmt>(
                 option(ikw<"LET">) +
                 assign<&ast::AssignStmt::target>(CallTarget) + lit<"="> +
                 assign<&ast::AssignStmt::value>(ref<Expression>))> {};

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

  struct MidStmt
      : rule<create<ast::MidStmt>(
                 assign<&ast::MidStmt::mode>(ikw<"MIDB$"> | ikw<"MIDB"> |
                                              ikw<"MID$"> | ikw<"MID">) +
                 lit<"("> + assign<&ast::MidStmt::target>(CallTarget) +
                 lit<","> + assign<&ast::MidStmt::start>(ref<Expression>) +
                 option(lit<","> +
                        assign<&ast::MidStmt::length>(ref<Expression>)) +
                 lit<")"> + lit<"="> +
                 assign<&ast::MidStmt::value>(ref<Expression>))> {};

  struct ReDimItem
      : rule<create<ast::ReDimItem>(
                 assign<&ast::ReDimItem::target>(CallTarget) +
                 TypeSuffixInto<&ast::ReDimItem::typeChar> +
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
                 some(append<&ast::EraseStmt::targets>(CallTarget),
                      lit<",">))> {};

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

  struct DoStmt
      : rule<create<ast::DoStmt>(
                 ikw<"DO"> +
                 ((flag<&ast::DoStmt::pre>(DoCondGroup) +
                   BodyInto<&ast::DoStmt::body> + ikw<"LOOP">) |
                  (BodyInto<&ast::DoStmt::body> + ikw<"LOOP"> +
                   option(DoCondGroup))))> {};

  struct ForStmt;
  struct ForEachStmt;

  // A loop owes its `Next`, or ends on an inner loop whose `Next` it shares:
  // in `Next j, i` the inner loop writes `Next j` and this one takes `i`
  // after the comma (MS-VBAL `nested-for-statement`).
  template <auto Body, auto Closed, auto CloseVars>
  static constexpr auto LoopCloseInto =
      (flag<Closed>(ikw<"NEXT">) +
       option(append<CloseVars>(CallTarget))) |
      (append<Body>(ref<ForStmt> | ref<ForEachStmt>) + lit<","> +
       append<CloseVars>(CallTarget));

  struct ForStmt
      : rule<create<ast::ForStmt>(
                 ikw<"FOR"> + assign<&ast::ForStmt::var>(CallTarget) +
                 lit<"="> + assign<&ast::ForStmt::from>(ref<Expression>) +
                 ikw<"TO"> + assign<&ast::ForStmt::to>(ref<Expression>) +
                 option(ikw<"STEP"> +
                        assign<&ast::ForStmt::step>(ref<Expression>)) +
                 BodyInto<&ast::ForStmt::body> +
                 LoopCloseInto<&ast::ForStmt::body, &ast::ForStmt::closed,
                               &ast::ForStmt::closeVars>)> {};

  struct ForEachStmt
      : rule<create<ast::ForEachStmt>(
                 ikw<"FOR"> + ikw<"EACH"> +
                 assign<&ast::ForEachStmt::var>(CallTarget) + ikw<"IN"> +
                 assign<&ast::ForEachStmt::collection>(ref<Expression>) +
                 BodyInto<&ast::ForEachStmt::body> +
                 LoopCloseInto<&ast::ForEachStmt::body,
                               &ast::ForEachStmt::closed,
                               &ast::ForEachStmt::closeVars>)> {};

  struct ExitStmt
      : rule<create<ast::ExitStmt>(
                 ikw<"EXIT"> +
                 assign<&ast::ExitStmt::kind>(
                     ikw<"FOR"> | ikw<"DO"> | ikw<"SUB"> | ikw<"FUNCTION"> |
                     ikw<"PROPERTY">))> {};

  // -- If ------------------------------------------------------------------

  struct ElseIfClause
      : rule<create<ast::ElseIfClause>(
                 ikw<"ELSEIF"> +
                 assign<&ast::ElseIfClause::cond>(ref<Expression>) +
                 ikw<"THEN"> + BodyInto<&ast::ElseIfClause::body>)> {};

  // Block form: nothing follows `Then` before the line ends.
  struct IfStmt
      : rule<create<ast::IfStmt>(
                 ikw<"IF"> + assign<&ast::IfStmt::cond>(ref<Expression>) +
                 ikw<"THEN"> + BodyInto<&ast::IfStmt::thenBody> +
                 many(append<&ast::IfStmt::elseIfs>(ref<ElseIfClause>)) +
                 option(flag<&ast::IfStmt::hasElse>(ikw<"ELSE">) +
                        BodyInto<&ast::IfStmt::elseBody>) +
                 (ikw<"ENDIF"> | (ikw<"END"> + ikw<"IF">)))> {};

  template <auto Member>
  static constexpr auto SameLineListInto =
      (append<Member>(ref<LineNumberLabel>) |
       (option(lit<":">) + append<Member>(ref<SameLineStmt>))) +
      many(lit<":"> + option(append<Member>(ref<SameLineStmt>)));

  static constexpr auto SingleLineElse =
      flag<&ast::IfStmt::hasElse>(ikw<"ELSE">) +
      option(SameLineListInto<&ast::IfStmt::elseBody>);

  // Single-line form: statements follow `Then`/`Else` on the same logical
  // line; no `ElseIf`. An empty `Then` needs its `Else`: `If c Then` alone
  // on its line opens a block.
  struct SingleLineIfStmt
      : rule<create<ast::IfStmt>(
                 ikw<"IF"> + assign<&ast::IfStmt::cond>(ref<Expression>) +
                 ikw<"THEN"> +
                 ((SameLineListInto<&ast::IfStmt::thenBody> +
                   option(SingleLineElse)) |
                  (option(lit<":">) + SingleLineElse)))> {};

  // -- Select Case ----------------------------------------------------------

  static constexpr auto ComparisonOpText =
      lit<">="> | lit<"=>"> | lit<"<="> | lit<"=<"> | lit<"<>"> |
      lit<"><"> | lit<">"> | lit<"<"> | lit<"=">;

  struct RangeClause
      : rule<create<ast::RangeClause>(
                 option(ikw<"IS">) +
                 assign<&ast::RangeClause::compareOp>(ComparisonOpText) +
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
                 some(append<&ast::CaseClause::ranges>(ref<RangeClause>),
                      lit<",">) +
                 BodyInto<&ast::CaseClause::body>)> {};

  struct SelectCaseStmt
      : rule<create<ast::SelectCaseStmt>(
                 ikw<"SELECT"> + ikw<"CASE"> +
                 assign<&ast::SelectCaseStmt::selector>(ref<Expression>) +
                 Eos +
                 many(append<&ast::SelectCaseStmt::cases>(
                     ref<CaseClause>)) +
                 option(ikw<"CASE"> +
                        flag<&ast::SelectCaseStmt::hasElse>(
                            ikw<"ELSE">) +
                        BodyInto<&ast::SelectCaseStmt::elseBody>) +
                 ikw<"END"> + ikw<"SELECT">)> {};

  struct StopStmt : rule<create<ast::StopStmt> + ikw<"STOP">> {};

  static constexpr auto LabelText = NameText | ref<INTEGERLITERAL>;

  struct GotoStmt
      : rule<create<ast::GotoStmt>(
                 ((ikw<"GO"> + ikw<"TO">) | ikw<"GOTO">) +
                 assign<&ast::GotoStmt::label>(LabelText))> {};

  struct OnGotoStmt
      : rule<create<ast::OnGotoStmt>(
                 ikw<"ON"> +
                 assign<&ast::OnGotoStmt::selector>(ref<Expression>) +
                 ikw<"GOTO"> +
                 some(append<&ast::OnGotoStmt::labels>(LabelText),
                      lit<",">))> {};

  struct GosubStmt
      : rule<create<ast::GosubStmt>(
                 ((ikw<"GO"> + ikw<"SUB">) | ikw<"GOSUB">) +
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

  struct AssertStmt
      : rule<create<ast::AssertStmt>(
                 ikw<"DEBUG"> + lit<"."> + ikw<"ASSERT"> +
                 assign<&ast::AssertStmt::cond>(ref<Expression>))> {};

  struct OnErrorStmt
      : rule<create<ast::OnErrorStmt>(
                 ikw<"ON"> + ikw<"ERROR"> +
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

  struct LabelStmt
      : rule<assign<&ast::LabelStmt::name>(ref<LABEL>)> {};

  struct LineNumberLabel
      : rule<assign<&ast::LabelStmt::name>(ref<INTEGERLITERAL>)> {};

  // -- File statements --------------------------------------------------

  struct FileNumber : rule<option(lit<"#">) + ref<Expression>> {};

  struct AccessKind
      : rule<(ikw<"READ"> + ikw<"WRITE">) | ikw<"READ"> | ikw<"WRITE">> {};

  struct LockKind
      : rule<ikw<"SHARED"> | (ikw<"LOCK"> + ikw<"READ"> + ikw<"WRITE">) |
             (ikw<"LOCK"> + ikw<"READ">) | (ikw<"LOCK"> + ikw<"WRITE">)> {};

  struct OpenStmt
      : rule<create<ast::OpenStmt>(
                 ikw<"OPEN"> + assign<&ast::OpenStmt::path>(ref<Expression>) +
                 option(ikw<"FOR"> +
                        assign<&ast::OpenStmt::mode>(
                            ikw<"APPEND"> | ikw<"BINARY"> | ikw<"INPUT"> |
                            ikw<"OUTPUT"> | ikw<"RANDOM">)) +
                 option(ikw<"ACCESS"> +
                        assign<&ast::OpenStmt::access>(
                            ref<AccessKind>)) +
                 option(assign<&ast::OpenStmt::lockKind>(
                     ref<LockKind>)) +
                 ikw<"AS"> +
                 assign<&ast::OpenStmt::fileNumber>(ref<FileNumber>) +
                 option(ikw<"LEN"> + lit<"="> +
                        assign<&ast::OpenStmt::recLength>(ref<Expression>)))> {
  };

  struct CloseStmt
      : rule<create<ast::CloseStmt>(
                 flag<&ast::CloseStmt::reset>(ikw<"RESET">)) |
             create<ast::CloseStmt>(
                 ikw<"CLOSE"> +
                 option(some(append<&ast::CloseStmt::fileNumbers>(
                                 ref<FileNumber>),
                             lit<",">)))> {};

  struct SeekStmt
      : rule<create<ast::SeekStmt>(
                 ikw<"SEEK"> +
                 assign<&ast::SeekStmt::fileNumber>(ref<FileNumber>) +
                 lit<","> +
                 assign<&ast::SeekStmt::position>(ref<Expression>))> {};

  struct LockStmt
      : rule<create<ast::LockStmt>(
                 ikw<"LOCK"> +
                 assign<&ast::LockStmt::fileNumber>(ref<FileNumber>) +
                 option(lit<","> +
                        option(assign<&ast::LockStmt::startRecord>(ref<Expression>)) +
                        option(ikw<"TO"> + assign<&ast::LockStmt::endRecord>(
                                               ref<Expression>))))> {};

  struct UnlockStmt
      : rule<create<ast::UnlockStmt>(
                 ikw<"UNLOCK"> +
                 assign<&ast::UnlockStmt::fileNumber>(ref<FileNumber>) +
                 option(lit<","> +
                        option(assign<&ast::UnlockStmt::startRecord>(
                            ref<Expression>)) +
                        option(ikw<"TO"> +
                               assign<&ast::UnlockStmt::endRecord>(
                                   ref<Expression>))))> {};

  struct LineInputStmt
      : rule<create<ast::LineInputStmt>(
                 ikw<"LINE"> + ikw<"INPUT"> +
                 assign<&ast::LineInputStmt::fileNumber>(
                     ref<FileNumber>) +
                 lit<","> +
                 assign<&ast::LineInputStmt::target>(ref<Expression>))> {};

  struct WidthStmt
      : rule<create<ast::WidthStmt>(
                 ikw<"WIDTH"> +
                 assign<&ast::WidthStmt::fileNumber>(ref<FileNumber>) +
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
                 option(flag<&ast::PrintStmt::toDebugOrMe>(
                     (ikw<"DEBUG"> | ikw<"ME">) + lit<".">)) +
                 ikw<"PRINT"> +
                 option(lit<"#"> +
                        assign<&ast::PrintStmt::fileNumber>(
                            ref<Expression>) +
                        lit<",">) +
                 many(append<&ast::PrintStmt::items>(PrintItemBody)))> {};

  struct WriteStmt
      : rule<create<ast::WriteStmt>(
                 ikw<"WRITE"> +
                 assign<&ast::WriteStmt::fileNumber>(ref<FileNumber>) +
                 lit<","> +
                 option(some(append<&ast::WriteStmt::items>(PrintItemBody),
                             lit<",">)))> {};

  struct InputStmt
      : rule<create<ast::InputStmt>(
                 ikw<"INPUT"> +
                 assign<&ast::InputStmt::fileNumber>(ref<FileNumber>) +
                 lit<","> +
                 some(append<&ast::InputStmt::targets>(ref<Expression>),
                      lit<",">))> {};

  struct PutStmt
      : rule<create<ast::PutStmt>(
                 ikw<"PUT"> +
                 assign<&ast::PutStmt::fileNumber>(ref<FileNumber>) +
                 lit<","> +
                 option(assign<&ast::PutStmt::recordNumber>(
                     ref<Expression>)) +
                 lit<","> +
                 assign<&ast::PutStmt::value>(ref<Expression>))> {};

  struct GetStmt
      : rule<create<ast::GetStmt>(
                 ikw<"GET"> +
                 assign<&ast::GetStmt::fileNumber>(ref<FileNumber>) +
                 lit<","> +
                 option(assign<&ast::GetStmt::recordNumber>(
                     ref<Expression>)) +
                 lit<","> +
                 assign<&ast::GetStmt::value>(ref<Expression>))> {};

  // A statement usable both on its own line and after `Then`/`Else`/`:` on a
  // single-line `If`.
  struct SameLineStmt
      : rule<ref<OpenStmt> | ref<CloseStmt> | ref<SeekStmt> | ref<LockStmt> |
             ref<UnlockStmt> | ref<LineInputStmt> | ref<WidthStmt> |
             ref<PrintStmt> | ref<WriteStmt> | ref<InputStmt> | ref<PutStmt> |
             ref<GetStmt> | ref<OnErrorStmt> | ref<ResumeStmt> |
             ref<ErrorStmt> | ref<AssertStmt> | ref<WhileStmt> |
             ref<ForEachStmt> | ref<ForStmt> | ref<ExitStmt> | ref<DoStmt> |
             ref<SingleLineIfStmt> | ref<SelectCaseStmt> | ref<StopStmt> |
             ref<GotoStmt> | ref<OnGotoStmt> | ref<GosubStmt> |
             ref<ReturnStmt> | ref<OnGosubStmt> | ref<RaiseEventStmt> |
             ref<WithStmt> | ref<EndStmt> | ref<LSetStmt> | ref<RSetStmt> |
             ref<MidStmt> | ref<AssignStmt> | ref<CallStmt>> {};

  struct BodyItem
      : rule<ref<CompilationConst> | ref<Attribute> | ref<VarDecl> | ref<ConstDecl> |
             ref<ReDimStmt> | ref<EraseStmt> | ref<IfStmt> |
             ref<SameLineStmt>> {};

  // -- Procedures -----------------------------------------------------------

  static constexpr auto ProcScope =
      ikw<"PRIVATE"> | ikw<"PUBLIC"> | ikw<"FRIEND"> | ikw<"GLOBAL">;

  struct Procedure
      : rule<create<ast::Procedure>(
                 option(assign<&ast::Procedure::scope>(ProcScope)) +
                 option(flag<&ast::Procedure::isStatic>(
                     ikw<"STATIC">)) +
                 assign<&ast::Procedure::kind>(ikw<"SUB">) +
                 assign<&ast::Procedure::name>(NameText) +
                 option(ParamListInto<&ast::Procedure::params>) +
                 option(flag<&ast::Procedure::isStatic>(
                     ikw<"STATIC">)) +
                 BodyInto<&ast::Procedure::body> + ikw<"END"> +
                 ikw<"SUB">) |
             create<ast::Procedure>(
                 option(assign<&ast::Procedure::scope>(ProcScope)) +
                 option(flag<&ast::Procedure::isStatic>(
                     ikw<"STATIC">)) +
                 assign<&ast::Procedure::kind>(ikw<"FUNCTION">) +
                 assign<&ast::Procedure::name>(NameText) +
                 TypeSuffixInto<&ast::Procedure::typeChar> +
                 option(ParamListInto<&ast::Procedure::params>) +
                 option(ikw<"AS"> + assign<&ast::Procedure::returnType>(
                                        ref<TypeExpression>)) +
                 option(flag<&ast::Procedure::isStatic>(
                     ikw<"STATIC">)) +
                 BodyInto<&ast::Procedure::body> + ikw<"END"> +
                 ikw<"FUNCTION">) |
             create<ast::Procedure>(
                 option(assign<&ast::Procedure::scope>(ProcScope)) +
                 option(flag<&ast::Procedure::isStatic>(
                     ikw<"STATIC">)) +
                 assign<&ast::Procedure::kind>(ikw<"PROPERTY">) +
                 assign<&ast::Procedure::accessor>(ikw<"GET">) +
                 assign<&ast::Procedure::name>(NameText) +
                 TypeSuffixInto<&ast::Procedure::typeChar> +
                 option(ParamListInto<&ast::Procedure::params>) +
                 option(ikw<"AS"> + assign<&ast::Procedure::returnType>(
                                        ref<TypeExpression>)) +
                 option(flag<&ast::Procedure::isStatic>(
                     ikw<"STATIC">)) +
                 BodyInto<&ast::Procedure::body> + ikw<"END"> +
                 ikw<"PROPERTY">) |
             create<ast::Procedure>(
                 option(assign<&ast::Procedure::scope>(ProcScope)) +
                 option(flag<&ast::Procedure::isStatic>(
                     ikw<"STATIC">)) +
                 assign<&ast::Procedure::kind>(ikw<"PROPERTY">) +
                 assign<&ast::Procedure::accessor>(ikw<"LET"> |
                                                    ikw<"SET">) +
                 assign<&ast::Procedure::name>(NameText) +
                 ParamListInto<&ast::Procedure::params> +
                 option(flag<&ast::Procedure::isStatic>(
                     ikw<"STATIC">)) +
                 BodyInto<&ast::Procedure::body> + ikw<"END"> +
                 ikw<"PROPERTY">)> {};

  // -- Module-level items -------------------------------------------------

  struct ModuleItem
      : rule<ref<CompilationConst> | ref<Attribute> | ref<OptionStmt> |
             ref<DefDirective> | ref<VarDecl> | ref<ConstDecl> |
             ref<TypeDecl> | ref<EnumDecl> | ref<DeclareDecl> |
             ref<EventDecl> | ref<ImplementsDecl> | ref<Procedure>> {};

  // A trailing separator with nothing after it is legal at the very end of a
  // module (a last line closed only by a comment or a bare `:`).
  static constexpr auto ModuleBodyExpr =
      some(append<&ast::Module::body>(ref<ModuleItem>), Eos) + option(Eos);

  // -- Class/form header configuration -------------------------------------

  struct ConfigValue
      : rule<(option(lit<"-">) +
              (ref<DATELITERAL> | ref<FLOATLITERAL> | ref<INTEGERLITERAL> |
               ref<STRINGLITERAL> | LiteralIdentifier)) |
             ref<FILEOFFSET>> {};

  struct ConfigEntry
      : rule<create<ast::ConfigEntry>(
                 option(ikw<"OBJECT"> + lit<".">) + option(lit<"_">) +
                 assign<&ast::ConfigEntry::name>(NameText) + lit<"="> +
                 assign<&ast::ConfigEntry::value>(ref<ConfigValue>))> {};

  struct FormBeginBlock;
  struct BeginPropertyBlock;

  static constexpr auto BeginBlockElement =
      append<&ast::BeginBlock::entries>(ref<ConfigEntry>) |
      append<&ast::BeginBlock::children>(ref<FormBeginBlock>) |
      append<&ast::BeginBlock::children>(ref<BeginPropertyBlock>);

  struct FormBeginBlock
      : rule<create<ast::BeginBlock>(
                 ikw<"BEGIN"> +
                 assign<&ast::BeginBlock::classId>(ref<GUID> |
                                                    ref<QualifiedTypeName>) +
                 assign<&ast::BeginBlock::name>(NameText) +
                 some(Eos + BeginBlockElement) + Eos + ikw<"END">)> {};

  struct BeginPropertyBlock
      : rule<create<ast::BeginBlock>(
                 flag<&ast::BeginBlock::isProperty>(
                     ikw<"BEGINPROPERTY">) +
                 assign<&ast::BeginBlock::name>(NameText) +
                 option(assign<&ast::BeginBlock::classId>(ref<GUID>)) +
                 some(Eos + BeginBlockElement) + Eos + ikw<"ENDPROPERTY">)> {
  };

  struct ClassBeginBlock
      : rule<create<ast::BeginBlock>(
                 ikw<"BEGIN"> + some(Eos + BeginBlockElement) + Eos +
                 ikw<"END">)> {};

  struct FormObjectAssign
      : rule<create<ast::ObjectAssign>(
                 assign<&ast::ObjectAssign::value>(ref<STRINGLITERAL>) +
                 option(lit<";"> + assign<&ast::ObjectAssign::value2>(
                                       ref<STRINGLITERAL>)))> {};

  // -- Module ---------------------------------------------------------------

  struct ModuleRule
      : rule<create<ast::Module>(
                 BlankLines +
                 ((ikw<"VERSION"> +
                   assign<&ast::Module::version>(ref<FLOATLITERAL>) +
                   ikw<"CLASS"> + Eos +
                   assign<&ast::Module::header>(ref<ClassBeginBlock>) +
                   Eos + option(ModuleBodyExpr)) |
                  flag<&ast::Module::isForm>(
                      ikw<"VERSION"> +
                      assign<&ast::Module::version>(ref<FLOATLITERAL>) +
                      many(Eos + ikw<"OBJECT"> + lit<"="> +
                           append<&ast::Module::objects>(
                               ref<FormObjectAssign>)) +
                      Eos +
                      assign<&ast::Module::header>(
                          ref<FormBeginBlock>) +
                      Eos + option(ModuleBodyExpr)) |
                  // A module of separators only resumes at its first `:`:
                  // `BlankLines` took the leading newlines.
                  option(ModuleBodyExpr | (lit<":"> + option(Eos)))))> {};

  using entry = ModuleRule;
  using ignored = list<WS, LINE_CONTINUATION>;
  using hidden = list<COMMENT, REMCOMMENT>;
};

}  // namespace vba
