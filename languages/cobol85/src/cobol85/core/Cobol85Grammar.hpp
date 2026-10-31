// Free-format COBOL 85: no column-position lexing. COPY/REPLACE text
// substitution is a separate preprocessing pass, out of scope here.
#pragma once

#include <cstddef>
#include <memory_resource>
#include <span>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "cobol85/core/ast.hpp"

namespace cobol85 {

// Doubled delimiter -> one: `""` inside `"..."`, `''` inside `'...'`. The
// delimiter is whichever byte opened the match.
inline constexpr auto unquoteCobol = [](std::string_view t,
                                        std::pmr::memory_resource* mem) {
  const char q = t.front();
  const std::string_view body = t.substr(1, t.size() - 2);
  if (body.find(q) == std::string_view::npos) return body;
  const std::span<char> room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    room[n++] = body[i];
    if (body[i] == q) ++i;
  }
  return std::string_view(room.data(), n);
};

// A multi-word phrase (`GREATER THAN OR EQUAL TO`, `WITH TEST AFTER`) has no
// single terminal to `assign<>` from; wrapping it in a rule with this
// converter reads the whole matched span as one string_view.
inline constexpr auto rawSpan = [](std::string_view t, std::pmr::memory_resource*) {
  return t;
};

struct Cobol85 : cursiv::GrammarSpec {
  // Identifiers carry internal hyphens (WORKING-STORAGE-COUNTER is one
  // word): a keyword must not match as a prefix of such a name.
  static constexpr auto word_boundary = cr<"a-zA-Z0-9_-">;

  struct WS : terminal<some(cr<R"( \t\f\v)">)> {};
  struct NEWLINE : terminal<option(lit<"\r">) + lit<"\n">> {};
  struct LINE_COMMENT : terminal<lit<"*>"> + many(cr<R"(^\r\n)">)> {};

  // The reserved words, minus punctuation and literal-machinery tokens.
  // IDENT refuses them all rather than the curated "also a name" subset ISO
  // COBOL permits.
  static constexpr auto Reserved =
      ikw<"ABORT"> | ikw<"ACCEPT"> | ikw<"ACCESS"> | ikw<"ADD"> |
      ikw<"ADDRESS"> | ikw<"ADVANCING"> | ikw<"AFTER"> | ikw<"ALIGNED"> |
      ikw<"ALL"> | ikw<"ALPHABET"> | ikw<"ALPHABETIC"> | ikw<"ALPHABETIC-LOWER"> |
      ikw<"ALPHABETIC-UPPER"> | ikw<"ALPHANUMERIC"> | ikw<"ALPHANUMERIC-EDITED"> | ikw<"ALSO"> |
      ikw<"ALTER"> | ikw<"ALTERNATE"> | ikw<"AND"> | ikw<"ANY"> |
      ikw<"ARE"> | ikw<"AREA"> | ikw<"AREAS"> | ikw<"AS"> |
      ikw<"ASCENDING"> | ikw<"ASCII"> | ikw<"ASSIGN"> | ikw<"ASSOCIATED-DATA"> |
      ikw<"ASSOCIATED-DATA-LENGTH"> | ikw<"AT"> | ikw<"ATTRIBUTE"> | ikw<"AUTHOR"> |
      ikw<"AUTO"> | ikw<"AUTO-SKIP"> | ikw<"BACKGROUND-COLOR"> | ikw<"BACKGROUND-COLOUR"> |
      ikw<"BASIS"> | ikw<"BEEP"> | ikw<"BEFORE"> | ikw<"BEGINNING"> |
      ikw<"BELL"> | ikw<"BINARY"> | ikw<"BIT"> | ikw<"BLANK"> |
      ikw<"BLINK"> | ikw<"BLOCK"> | ikw<"BOTTOM"> | ikw<"BOUNDS"> |
      ikw<"BY"> | ikw<"BYFUNCTION"> | ikw<"BYTITLE"> | ikw<"CALL"> |
      ikw<"CANCEL"> | ikw<"CAPABLE"> | ikw<"CCSVERSION"> | ikw<"CD"> |
      ikw<"CF"> | ikw<"CH"> | ikw<"CHAINING"> | ikw<"CHANGED"> |
      ikw<"CHANNEL"> | ikw<"CHARACTER"> | ikw<"CHARACTERS"> | ikw<"CLASS"> |
      ikw<"CLASS-ID"> | ikw<"CLOCK-UNITS"> | ikw<"CLOSE"> | ikw<"CLOSE-DISPOSITION"> |
      ikw<"COBOL"> | ikw<"CODE"> | ikw<"CODE-SET"> | ikw<"COL"> |
      ikw<"COLLATING"> | ikw<"COLUMN"> | ikw<"COMMA"> | ikw<"COMMITMENT"> |
      ikw<"COMMON"> | ikw<"COMMUNICATION"> | ikw<"COMP"> | ikw<"COMP-1"> |
      ikw<"COMP-2"> | ikw<"COMP-3"> | ikw<"COMP-4"> | ikw<"COMP-5"> |
      ikw<"COMPUTATIONAL"> | ikw<"COMPUTATIONAL-1"> | ikw<"COMPUTATIONAL-2"> | ikw<"COMPUTATIONAL-3"> |
      ikw<"COMPUTATIONAL-4"> | ikw<"COMPUTATIONAL-5"> | ikw<"COMPUTE"> | ikw<"COM-REG"> |
      ikw<"CONFIGURATION"> | ikw<"CONTAINS"> | ikw<"CONTENT"> | ikw<"CONTINUE"> |
      ikw<"CONTROL"> | ikw<"CONTROL-POINT"> | ikw<"CONTROLS"> | ikw<"CONVENTION"> |
      ikw<"CONVERTING"> | ikw<"COPY"> | ikw<"CORR"> | ikw<"CORRESPONDING"> |
      ikw<"COUNT"> | ikw<"CRUNCH"> | ikw<"CURRENCY"> | ikw<"CURSOR"> |
      ikw<"DATA"> | ikw<"DATA-BASE"> | ikw<"DATE"> | ikw<"DATE-COMPILED"> |
      ikw<"DATE-WRITTEN"> | ikw<"DAY"> | ikw<"DAY-OF-WEEK"> | ikw<"DBCS"> |
      ikw<"DE"> | ikw<"DEBUG-CONTENTS"> | ikw<"DEBUGGING"> | ikw<"DEBUG-ITEM"> |
      ikw<"DEBUG-LINE"> | ikw<"DEBUG-NAME"> | ikw<"DEBUG-SUB-1"> | ikw<"DEBUG-SUB-2"> |
      ikw<"DEBUG-SUB-3"> | ikw<"DECIMAL-POINT"> | ikw<"DECLARATIVES"> | ikw<"DEFAULT"> |
      ikw<"DEFAULT-DISPLAY"> | ikw<"DEFINITION"> | ikw<"DELETE"> | ikw<"DELIMITED"> |
      ikw<"DELIMITER"> | ikw<"DEPENDING"> | ikw<"DESCENDING"> | ikw<"DESTINATION"> |
      ikw<"DETAIL"> | ikw<"DFHRESP"> | ikw<"DFHVALUE"> | ikw<"DISABLE"> |
      ikw<"DISK"> | ikw<"DISPLAY"> | ikw<"DISPLAY-1"> | ikw<"DIVIDE"> |
      ikw<"DIVISION"> | ikw<"DONTCARE"> | ikw<"DOUBLE"> | ikw<"DOWN"> |
      ikw<"DUPLICATES"> | ikw<"DYNAMIC"> | ikw<"EBCDIC"> | ikw<"EGCS"> |
      ikw<"EGI"> | ikw<"ELSE"> | ikw<"EMI"> | ikw<"EMPTY-CHECK"> |
      ikw<"ENABLE"> | ikw<"END"> | ikw<"END-ACCEPT"> | ikw<"END-ADD"> |
      ikw<"END-CALL"> | ikw<"END-COMPUTE"> | ikw<"END-DELETE"> | ikw<"END-DIVIDE"> |
      ikw<"END-EVALUATE"> | ikw<"END-IF"> | ikw<"ENDING"> | ikw<"END-MULTIPLY"> |
      ikw<"END-OF-PAGE"> | ikw<"END-PERFORM"> | ikw<"END-READ"> | ikw<"END-RECEIVE"> |
      ikw<"END-RETURN"> | ikw<"END-REWRITE"> | ikw<"END-SEARCH"> | ikw<"END-START"> |
      ikw<"END-STRING"> | ikw<"END-SUBTRACT"> | ikw<"END-UNSTRING"> | ikw<"END-WRITE"> |
      ikw<"ENTER"> | ikw<"ENTRY"> | ikw<"ENTRY-PROCEDURE"> | ikw<"ENVIRONMENT"> |
      ikw<"EOL"> | ikw<"EOP"> | ikw<"EOS"> | ikw<"EQUAL"> |
      ikw<"ERASE"> | ikw<"ERROR"> | ikw<"ESCAPE"> | ikw<"ESI"> |
      ikw<"EVALUATE"> | ikw<"EVENT"> | ikw<"EVERY"> | ikw<"EXCEPTION"> |
      ikw<"EXCLUSIVE"> | ikw<"EXHIBIT"> | ikw<"EXIT"> | ikw<"EXPORT"> |
      ikw<"EXTEND"> | ikw<"EXTENDED"> | ikw<"EXTERNAL"> | ikw<"FALSE"> |
      ikw<"FD"> | ikw<"FILE"> | ikw<"FILE-CONTROL"> | ikw<"FILLER"> |
      ikw<"FINAL"> | ikw<"FIRST"> | ikw<"FOOTING"> | ikw<"FOR"> |
      ikw<"FOREGROUND-COLOR"> | ikw<"FOREGROUND-COLOUR"> | ikw<"FROM"> | ikw<"FULL"> |
      ikw<"FUNCTION"> | ikw<"FUNCTIONNAME"> | ikw<"FUNCTION-POINTER"> | ikw<"GENERATE"> |
      ikw<"GIVING"> | ikw<"GLOBAL"> | ikw<"GO"> | ikw<"GOBACK"> |
      ikw<"GREATER"> | ikw<"GRID"> | ikw<"GROUP"> | ikw<"HEADING"> |
      ikw<"HIGHLIGHT"> | ikw<"HIGH-VALUE"> | ikw<"HIGH-VALUES"> | ikw<"ID"> |
      ikw<"IDENTIFICATION"> | ikw<"IF"> | ikw<"IMPLICIT"> | ikw<"IMPORT"> |
      ikw<"IN"> | ikw<"INDEX"> | ikw<"INDEXED"> | ikw<"INDICATE"> |
      ikw<"INITIAL"> | ikw<"INITIALIZE"> | ikw<"INITIATE"> | ikw<"INPUT"> |
      ikw<"INPUT-OUTPUT"> | ikw<"INSPECT"> | ikw<"INSTALLATION"> | ikw<"INTEGER"> |
      ikw<"INTO"> | ikw<"INVALID"> | ikw<"INVOKE"> | ikw<"I-O"> |
      ikw<"I-O-CONTROL"> | ikw<"IS"> | ikw<"JUST"> | ikw<"JUSTIFIED"> |
      ikw<"KANJI"> | ikw<"KEPT"> | ikw<"KEY"> | ikw<"KEYBOARD"> |
      ikw<"LABEL"> | ikw<"LANGUAGE"> | ikw<"LAST"> | ikw<"LB"> |
      ikw<"LD"> | ikw<"LEADING"> | ikw<"LEFT"> | ikw<"LEFTLINE"> |
      ikw<"LENGTH"> | ikw<"LENGTH-CHECK"> | ikw<"LESS"> | ikw<"LIBACCESS"> |
      ikw<"LIBPARAMETER"> | ikw<"LIBRARY"> | ikw<"LIMIT"> | ikw<"LIMITS"> |
      ikw<"LINAGE"> | ikw<"LINAGE-COUNTER"> | ikw<"LINE"> | ikw<"LINE-COUNTER"> |
      ikw<"LINES"> | ikw<"LINKAGE"> | ikw<"LIST"> | ikw<"LOCAL"> |
      ikw<"LOCAL-STORAGE"> | ikw<"LOCK"> | ikw<"LONG-DATE"> | ikw<"LONG-TIME"> |
      ikw<"LOWER"> | ikw<"LOWLIGHT"> | ikw<"LOW-VALUE"> | ikw<"LOW-VALUES"> |
      ikw<"MEMORY"> | ikw<"MERGE"> | ikw<"MESSAGE"> | ikw<"MMDDYYYY"> |
      ikw<"MODE"> | ikw<"MODULES"> | ikw<"MORE-LABELS"> | ikw<"MOVE"> |
      ikw<"MULTIPLE"> | ikw<"MULTIPLY"> | ikw<"NAMED"> | ikw<"NATIONAL"> |
      ikw<"NATIONAL-EDITED"> | ikw<"NATIVE"> | ikw<"NEGATIVE"> | ikw<"NETWORK"> |
      ikw<"NEXT"> | ikw<"NO"> | ikw<"NO-ECHO"> | ikw<"NOT"> |
      ikw<"NULL-"> | ikw<"NULLS"> | ikw<"NUMBER"> | ikw<"NUMERIC"> |
      ikw<"NUMERIC-DATE"> | ikw<"NUMERIC-EDITED"> | ikw<"NUMERIC-TIME"> | ikw<"OBJECT-COMPUTER"> |
      ikw<"OCCURS"> | ikw<"ODT"> | ikw<"OF"> | ikw<"OFF"> |
      ikw<"OMITTED"> | ikw<"ON"> | ikw<"OPEN"> | ikw<"OPTIONAL"> |
      ikw<"OR"> | ikw<"ORDER"> | ikw<"ORDERLY"> | ikw<"ORGANIZATION"> |
      ikw<"OTHER"> | ikw<"OUTPUT"> | ikw<"OVERFLOW"> | ikw<"OVERLINE"> |
      ikw<"OWN"> | ikw<"PACKED-DECIMAL"> | ikw<"PADDING"> | ikw<"PAGE"> |
      ikw<"PAGE-COUNTER"> | ikw<"PASSWORD"> | ikw<"PERFORM"> | ikw<"PF"> |
      ikw<"PH"> | ikw<"PIC"> | ikw<"PICTURE"> | ikw<"PLUS"> |
      ikw<"POINTER"> | ikw<"PORT"> | ikw<"POSITION"> | ikw<"POSITIVE"> |
      ikw<"PRINTER"> | ikw<"PRINTING"> | ikw<"PRIVATE"> | ikw<"PROCEDURE"> |
      ikw<"PROCEDURE-POINTER"> | ikw<"PROCEDURES"> | ikw<"PROCEED"> | ikw<"PROCESS"> |
      ikw<"PROGRAM"> | ikw<"PROGRAM-ID"> | ikw<"PROGRAM-LIBRARY"> | ikw<"PROMPT"> |
      ikw<"PURGE"> | ikw<"QUEUE"> | ikw<"QUOTE"> | ikw<"QUOTES"> |
      ikw<"RANDOM"> | ikw<"RD"> | ikw<"READ"> | ikw<"READER"> |
      ikw<"REAL"> | ikw<"RECEIVE"> | ikw<"RECEIVED"> | ikw<"RECORD"> |
      ikw<"RECORDING"> | ikw<"RECORDS"> | ikw<"RECURSIVE"> | ikw<"REDEFINES"> |
      ikw<"REEL"> | ikw<"REF"> | ikw<"REFERENCE"> | ikw<"REFERENCES"> |
      ikw<"RELATIVE"> | ikw<"RELEASE"> | ikw<"REMAINDER"> | ikw<"REMARKS"> |
      ikw<"REMOTE"> | ikw<"REMOVAL"> | ikw<"REMOVE"> | ikw<"RENAMES"> |
      ikw<"REPLACE"> | ikw<"REPLACING"> | ikw<"REPORT"> | ikw<"REPORTING"> |
      ikw<"REPORTS"> | ikw<"REQUIRED"> | ikw<"RERUN"> | ikw<"RESERVE"> |
      ikw<"RESET"> | ikw<"RETURN"> | ikw<"RETURN-CODE"> | ikw<"RETURNING"> |
      ikw<"REVERSED"> | ikw<"REVERSE-VIDEO"> | ikw<"REWIND"> | ikw<"REWRITE"> |
      ikw<"RF"> | ikw<"RH"> | ikw<"RIGHT"> | ikw<"ROUNDED"> |
      ikw<"RUN"> | ikw<"SAME"> | ikw<"SAVE"> | ikw<"SCREEN"> |
      ikw<"SD"> | ikw<"SEARCH"> | ikw<"SECTION"> | ikw<"SECURE"> |
      ikw<"SECURITY"> | ikw<"SEGMENT"> | ikw<"SEGMENT-LIMIT"> | ikw<"SELECT"> |
      ikw<"SEND"> | ikw<"SENTENCE"> | ikw<"SEPARATE"> | ikw<"SEQUENCE"> |
      ikw<"SEQUENTIAL"> | ikw<"SET"> | ikw<"SHARED"> | ikw<"SHAREDBYALL"> |
      ikw<"SHAREDBYRUNUNIT"> | ikw<"SHARING"> | ikw<"SHIFT-IN"> | ikw<"SHIFT-OUT"> |
      ikw<"SHORT-DATE"> | ikw<"SIGN"> | ikw<"SIZE"> | ikw<"SORT"> |
      ikw<"SORT-CONTROL"> | ikw<"SORT-CORE-SIZE"> | ikw<"SORT-FILE-SIZE"> | ikw<"SORT-MERGE"> |
      ikw<"SORT-MESSAGE"> | ikw<"SORT-MODE-SIZE"> | ikw<"SORT-RETURN"> | ikw<"SOURCE"> |
      ikw<"SOURCE-COMPUTER"> | ikw<"SPACE"> | ikw<"SPACES"> | ikw<"SPECIAL-NAMES"> |
      ikw<"STANDARD"> | ikw<"STANDARD-1"> | ikw<"STANDARD-2"> | ikw<"START"> |
      ikw<"STATUS"> | ikw<"STOP"> | ikw<"STRING"> | ikw<"SUB-QUEUE-1"> |
      ikw<"SUB-QUEUE-2"> | ikw<"SUB-QUEUE-3"> | ikw<"SUBTRACT"> | ikw<"SUM"> |
      ikw<"SUPPRESS"> | ikw<"SYMBOL"> | ikw<"SYMBOLIC"> | ikw<"SYNC"> |
      ikw<"SYNCHRONIZED"> | ikw<"TABLE"> | ikw<"TALLY"> | ikw<"TALLYING"> |
      ikw<"TAPE"> | ikw<"TASK"> | ikw<"TERMINAL"> | ikw<"TERMINATE"> |
      ikw<"TEST"> | ikw<"TEXT"> | ikw<"THAN"> | ikw<"THEN"> |
      ikw<"THREAD"> | ikw<"THREAD-LOCAL"> | ikw<"THROUGH"> | ikw<"THRU"> |
      ikw<"TIME"> | ikw<"TIMER"> | ikw<"TIMES"> | ikw<"TITLE"> |
      ikw<"TO"> | ikw<"TODAYS-DATE"> | ikw<"TODAYS-NAME"> | ikw<"TOP"> |
      ikw<"TRAILING"> | ikw<"TRUE"> | ikw<"TRUNCATED"> | ikw<"TYPE"> |
      ikw<"TYPEDEF"> | ikw<"UNDERLINE"> | ikw<"UNIT"> | ikw<"UNSTRING"> |
      ikw<"UNTIL"> | ikw<"UP"> | ikw<"UPON"> | ikw<"USAGE"> |
      ikw<"USE"> | ikw<"USING"> | ikw<"VALUE"> | ikw<"VALUES"> |
      ikw<"VARYING"> | ikw<"VIRTUAL"> | ikw<"WAIT"> | ikw<"WHEN"> |
      ikw<"WHEN-COMPILED"> | ikw<"WITH"> | ikw<"WORDS"> | ikw<"WORKING-STORAGE"> |
      ikw<"WRITE"> | ikw<"YEAR"> | ikw<"YYYYDDD"> | ikw<"YYYYMMDD"> |
      ikw<"ZERO"> | ikw<"ZEROES"> | ikw<"ZERO-FILL"> | ikw<"ZEROS">;

  struct IDENT : terminal<!Reserved + some(cr<"0-9a-zA-Z">) +
                          many(some(cr<"-_">) + some(cr<"0-9a-zA-Z">))> {};

  struct RAW_STRINGLIT
      : terminal<(lit<"\""> + many(lit<"\"\""> | cr<R"(^"\r\n)">) + lit<"\"">) |
                 (lit<"'"> + many(lit<"''"> | cr<R"(^'\r\n)">) + lit<"'">)> {};
  struct STRINGLIT : rule<as<unquoteCobol>(ref<RAW_STRINGLIT>)> {};
  struct HEXLIT : terminal<cr<"Xx"> + ((lit<"\""> + some(cr<"0-9A-F">) + lit<"\"">) |
                                       (lit<"'"> + some(cr<"0-9A-F">) + lit<"'">))> {};
  struct NULLLIT
      : terminal<cr<"Zz"> + ((lit<"\""> + many(lit<"\"\""> | lit<"'"> | cr<R"(^"\r\n)">) + lit<"\"">) |
                             (lit<"'"> + many(lit<"''"> | lit<"\""> | cr<R"(^'\r\n)">) + lit<"'">))> {};
  struct DBCSLIT
      : terminal<cr<"GN"> + ((lit<"\""> + many(lit<"\"\""> | lit<"'"> | cr<R"(^"\r\n)">) + lit<"\"">) |
                             (lit<"'"> + many(lit<"''"> | lit<"\""> | cr<R"(^'\r\n)">) + lit<"'">))> {};

  struct INTLIT : terminal<option(cr<"+-">) + some(d)> {};
  struct NUMLIT : terminal<option(cr<"+-">) + many(d) + (lit<"."> | lit<",">) + some(d) +
                           option(cr<"eE"> + option(cr<"+-">) + some(d))> {};

  // A trailing period is the sentence terminator (DOT_FS); a period inside a
  // PICTURE string or a decimal count is not -- the guard refuses DOT_FS
  // exactly where PICTURE's own dot must be read instead.
  struct DOT_FS : terminal<lit<"."> + (some(cr<R"( \t\r\n\f)">) | eof)> {};

  // A PICTURE clause's body, read as one run: letters, digits and edit
  // characters with no internal space. A dot inside it is followed by one of
  // them, so the entry's closing period is never absorbed, even at the end.
  static constexpr auto PicChar = cr<"0-9A-Za-z$/,:*()+<>-">;
  struct PICTURE : terminal<some(PicChar | (lit<"."> + PicChar))> {};

  struct EXECCICSLINE
      : terminal<lit<"*>EXECCICS"> + some(cr<R"( \t)">) + many(cr<R"(^\n\r})">) + cr<R"(\n\r})">> {};
  struct EXECSQLLINE
      : terminal<lit<"*>EXECSQL"> + some(cr<R"( \t)">) + many(cr<R"(^\n\r})">) + cr<R"(\n\r})">> {};
  struct EXECSQLIMSLINE
      : terminal<lit<"*>EXECSQLIMS"> + some(cr<R"( \t)">) + many(cr<R"(^\n\r})">) + cr<R"(\n\r})">> {};
  struct COMMENTENTRYLINE
      : terminal<lit<"*>CE"> + some(cr<R"( \t)">) + many(cr<R"(^\n\r)">)> {};

  struct CobolWord : rule<ref<IDENT>> {};

  // ---- forward declarations (mutually recursive parser rules) ----
  struct ArithExpr;
  struct Basis;
  struct SignedBasis;
  struct AllWord;
  struct AllOperand;
  struct Identifier;
  struct QualifierStep;
  struct SubscriptList;
  struct RefMod;
  struct FunctionCall;
  struct SpecialRegister;
  struct IdentifierOrRegister;
  struct SimpleLit;
  struct CicsLit;
  struct Literal;
  struct ConditionRule;
  struct AndCondition;
  struct CombinableCondition;
  struct SimpleCondition;
  struct RelationCond;
  struct AbbrevAndChain;
  struct ClassCond;
  struct CondNameRef;
  struct AbbrevCond;

  struct Statement;
  struct AcceptStatement;
  struct AddStatement;
  struct SubtractStatement;
  struct MultiplyStatement;
  struct DivideStatement;
  struct ComputeStatement;
  struct AlterStatement;
  struct CallStatement;
  struct CancelStatement;
  struct CloseStatement;
  struct ContinueStatement;
  struct DeleteStatement;
  struct EnableDisableStatement;
  struct DisplayStatement;
  struct EntryStatement;
  struct EvaluateStatement;
  struct EmbeddedStatement;
  struct ExhibitStatement;
  struct ExitStatement;
  struct GenerateStatement;
  struct GobackStatement;
  struct GoToStatement;
  struct IfStatement;
  struct InitializeStatement;
  struct InitiateStatement;
  struct InspectStatement;
  struct MergeStatement;
  struct MoveStatement;
  struct OpenStatement;
  struct PerformStatement;
  struct PurgeStatement;
  struct ReadStatement;
  struct ReceiveStatement;
  struct ReleaseStatement;
  struct ReturnStatement;
  struct RewriteStatement;
  struct SearchStatement;
  struct SendStatement;
  struct SetStatement;
  struct SortStatement;
  struct StartStatement;
  struct StopStatement;
  struct StringStatement;
  struct TerminateStatement;
  struct UnstringStatement;
  struct UseStatement;
  struct WriteStatement;
  struct Paragraph;
  struct ProcedureSection;
  struct Declarative;
  struct ProcedureDivision;

  struct DataDescriptionEntry;
  struct FileDescriptionEntry;
  struct ScreenDescriptionEntry;
  struct ReportGroupDescriptionEntry;
  struct ReportDescriptionEntry;
  struct CommunicationDescriptionEntry;
  struct LibraryDescriptionEntry;
  struct DataBaseSectionEntry;
  struct DataDivision;
  struct IdentificationDivision;

  struct ProgramUnit;
  struct CompilationUnit;

  // =========================== operands ===========================

  struct AllWord : rule<create<ast::Literal> + assign<&ast::Literal::text>(ikw<"ALL">)> {};
  struct AllOperand
      : rule<create<ast::ArithOperand> +
             assign<&ast::ArithOperand::value>(ref<AllWord>)> {};

  struct Identifier
      : rule<create<ast::Identifier> +
             assign<&ast::Identifier::name>(ref<CobolWord>) +
             many((ikw<"OF"> | ikw<"IN">) + append<&ast::Identifier::qualifiers>(ref<CobolWord>)) +
             option(lit<"("> +
                    some(append<&ast::Identifier::subscripts>(ref<AllOperand> | ref<ArithExpr>),
                         lit<",">) +
                    lit<")">) +
             option(lit<"("> + assign<&ast::Identifier::refModStart>(ref<ArithExpr>) + lit<":"> +
                    option(assign<&ast::Identifier::refModLength>(ref<ArithExpr>)) + lit<")">)> {};

  struct FunctionCall
      : rule<create<ast::FunctionCall> + ikw<"FUNCTION">+
             assign<&ast::FunctionCall::name>(
                 ikw<"INTEGER"> | ikw<"LENGTH"> | ikw<"RANDOM"> | ikw<"SUM"> |
                 ikw<"WHEN-COMPILED"> | ref<CobolWord>) +
             option(lit<"("> + some(append<&ast::FunctionCall::args>(ref<ArithExpr>), lit<",">) +
                    lit<")">) +
             option(lit<"("> + assign<&ast::FunctionCall::refModStart>(ref<ArithExpr>) + lit<":"> +
                    option(assign<&ast::FunctionCall::refModLength>(ref<ArithExpr>)) + lit<")">)> {};

  struct SpecialRegister
      : rule<create<ast::SpecialRegister> +
             ((assign<&ast::SpecialRegister::kind>(ikw<"ADDRESS">) + ikw<"OF"> +
               assign<&ast::SpecialRegister::operand>(ref<Identifier>)) |
              (assign<&ast::SpecialRegister::kind>(ikw<"LENGTH">) + option(ikw<"OF">) +
               assign<&ast::SpecialRegister::operand>(ref<Identifier>)) |
              assign<&ast::SpecialRegister::kind>(
                  ikw<"DATE"> | ikw<"DAY-OF-WEEK"> | ikw<"DAY"> | ikw<"DEBUG-CONTENTS"> |
                  ikw<"DEBUG-ITEM"> | ikw<"DEBUG-LINE"> | ikw<"DEBUG-NAME"> |
                  ikw<"DEBUG-SUB-1"> | ikw<"DEBUG-SUB-2"> | ikw<"DEBUG-SUB-3"> |
                  ikw<"LINAGE-COUNTER"> | ikw<"LINE-COUNTER"> | ikw<"PAGE-COUNTER"> |
                  ikw<"RETURN-CODE"> | ikw<"SHIFT-IN"> | ikw<"SHIFT-OUT"> |
                  ikw<"SORT-CONTROL"> | ikw<"SORT-CORE-SIZE"> | ikw<"SORT-FILE-SIZE"> |
                  ikw<"SORT-MESSAGE"> | ikw<"SORT-MODE-SIZE"> | ikw<"SORT-RETURN"> |
                  ikw<"TALLY"> | ikw<"TIME"> | ikw<"WHEN-COMPILED">))> {};

  struct IdentifierOrRegister
      : rule<ref<FunctionCall> | ref<SpecialRegister> | ref<Identifier>> {};

  struct TrueWord : rule<create<ast::Literal> + assign<&ast::Literal::text>(ikw<"TRUE">)> {};
  struct FalseWord : rule<create<ast::Literal> + assign<&ast::Literal::text>(ikw<"FALSE">)> {};

  static constexpr auto SimpleLitValue =
      ref<STRINGLIT> | ref<HEXLIT> | ref<NULLLIT> | ref<DBCSLIT> |
      ikw<"HIGH-VALUES"> | ikw<"HIGH-VALUE"> | ikw<"LOW-VALUES"> | ikw<"LOW-VALUE"> |
      ikw<"NULLS"> | ikw<"NULL"> | ikw<"QUOTES"> | ikw<"QUOTE"> |
      ikw<"SPACES"> | ikw<"SPACE"> | ikw<"ZEROES"> | ikw<"ZEROS"> | ikw<"ZERO"> |
      ref<NUMLIT> | ref<INTLIT> | ikw<"TRUE"> | ikw<"FALSE">;
  static constexpr auto CicsLitArgument =
      ref<CobolWord> | ref<STRINGLIT> | ref<HEXLIT> | ref<NULLLIT> | ref<DBCSLIT> |
      ref<NUMLIT> | ref<INTLIT>;

  struct SimpleLit
      : rule<create<ast::Literal> + option(flag<&ast::Literal::all>(ikw<"ALL">)) +
             assign<&ast::Literal::text>(SimpleLitValue)> {};

  struct CicsLit
      : rule<create<ast::Literal> +
             assign<&ast::Literal::text>(ikw<"DFHRESP"> | ikw<"DFHVALUE">) +
             (lit<"("> + CicsLitArgument + lit<")">)> {};

  struct Literal : rule<ref<SimpleLit> | ref<CicsLit>> {};
  static constexpr auto LiteralText =
      (option(ikw<"ALL">) + SimpleLitValue) |
      ((ikw<"DFHRESP"> | ikw<"DFHVALUE">) + lit<"("> + CicsLitArgument + lit<")">);

  struct Basis
      : rule<(lit<"("> + ref<ArithExpr> + lit<")">) |
             (create<ast::ArithOperand> +
              assign<&ast::ArithOperand::value>(ref<IdentifierOrRegister> | ref<Literal>))> {};

  struct SignedBasis
      : rule<(create<ast::ArithUnary> +
              assign<&ast::ArithUnary::op>(lit<"+"> | lit<"-">) +
              assign<&ast::ArithUnary::operand>(ref<Basis>)) |
             ref<Basis>> {};

  struct ArithExpr
      : infix<SignedBasis, ast::ArithBinary,
              levels(left(lit<"**">), left(lit<"*"> | lit<"/">),
                     left(lit<"+"> | lit<"-">))> {};

  // ---- conditions ----

  // A relational operator is a multi-word phrase (`GREATER THAN OR EQUAL
  // TO`) as often as a symbol: `assign<>` needs one part with a value, so
  // the whole phrase is a rule of its own, read as raw text by rawSpan.
  struct RelSignOrOp
      : rule<as<rawSpan>(ikw<"POSITIVE"> | ikw<"NEGATIVE"> | ikw<"ZERO"> |
                         (ikw<"GREATER"> + option(ikw<"THAN">) + ikw<"OR"> +
                          ikw<"EQUAL"> + option(ikw<"TO">)) |
                         lit<">="> |
                         (ikw<"LESS"> + option(ikw<"THAN">) + ikw<"OR"> +
                          ikw<"EQUAL"> + option(ikw<"TO">)) |
                         lit<"<="> | (ikw<"GREATER"> + option(ikw<"THAN">)) |
                         lit<">"> | (ikw<"LESS"> + option(ikw<"THAN">)) |
                         lit<"<"> | (ikw<"EQUAL"> + option(ikw<"TO">)) |
                         lit<"="> | lit<"<>">)> {};
  struct RelOp
      : rule<as<rawSpan>((ikw<"GREATER"> + option(ikw<"THAN">) + ikw<"OR"> +
                          ikw<"EQUAL"> + option(ikw<"TO">)) |
                         lit<">="> |
                         (ikw<"LESS"> + option(ikw<"THAN">) + ikw<"OR"> +
                          ikw<"EQUAL"> + option(ikw<"TO">)) |
                         lit<"<="> | (ikw<"GREATER"> + option(ikw<"THAN">)) |
                         lit<">"> | (ikw<"LESS"> + option(ikw<"THAN">)) |
                         lit<"<"> | (ikw<"EQUAL"> + option(ikw<"TO">)) |
                         lit<"="> | lit<"<>">)> {};

  // One AND-run inside a parenthesized abbreviated combined relation: the
  // element right after an OR (or the whole list, if it opens on one),
  // itself followed by zero or more `AND <arith>` continuations.
  // Returns Condition: an AND step wraps the AbbrevCond in a CondBinary.
  struct AbbrevAndChain
      : rule<create<ast::AbbrevCond>(
                 assign<&ast::AbbrevCond::value>(ref<ArithExpr>) +
                 many(nest<ast::CondBinary, &ast::CondBinary::left>(
                     assign<&ast::CondBinary::op>(ikw<"AND">) +
                     assign<&ast::CondBinary::right>(create<ast::AbbrevCond>(
                         assign<&ast::AbbrevCond::value>(ref<ArithExpr>))))))> {};

  // The plain form takes one right-hand ArithExpr (or none, for a sign
  // test); the abbreviated combined form -- `a relop (b AND c OR d)`, short
  // for `((a relop b) AND (a relop c)) OR (a relop d)` -- opens on the same
  // token an ordinary parenthesized ArithExpr does, so it stays a dead end
  // for the plain form (no `AND`/`OR` before the matching `)`) rather than
  // a real ambiguity. AND binds tighter than OR inside the list too (IBM's
  // Enterprise COBOL reference ties this construct to the same precedence
  // as an ordinary combined condition), so an OR step pulls in a whole
  // AND-run of its own rather than one bare element. Returns Condition: the
  // abbreviated form wraps the RelationCond in CondBinary nodes.
  struct RelationCond
      : rule<create<ast::RelationCond> +
             assign<&ast::RelationCond::left>(ref<ArithExpr>) +
             option(ikw<"IS"> | ikw<"ARE">) +
             option(flag<&ast::RelationCond::negated>(ikw<"NOT">)) +
             assign<&ast::RelationCond::op>(ref<RelSignOrOp>) +
             (option(assign<&ast::RelationCond::right>(ref<ArithExpr>)) |
              (lit<"("> + assign<&ast::RelationCond::right>(ref<ArithExpr>) +
               some(nest<ast::CondBinary, &ast::CondBinary::left>(
                   (assign<&ast::CondBinary::op>(ikw<"AND">) +
                    assign<&ast::CondBinary::right>(create<ast::AbbrevCond>(
                        assign<&ast::AbbrevCond::value>(ref<ArithExpr>)))) |
                   (assign<&ast::CondBinary::op>(ikw<"OR">) +
                    assign<&ast::CondBinary::right>(ref<AbbrevAndChain>)))) +
               lit<")">))> {};

  struct ClassCond
      : rule<create<ast::ClassCond> +
             assign<&ast::ClassCond::subject>(ref<Identifier>) + option(ikw<"IS">) +
             option(flag<&ast::ClassCond::negated>(ikw<"NOT">)) +
             assign<&ast::ClassCond::className>(
                 ikw<"NUMERIC"> | ikw<"ALPHABETIC-LOWER"> | ikw<"ALPHABETIC-UPPER"> |
                 ikw<"ALPHABETIC"> | ikw<"DBCS"> | ikw<"KANJI"> | ref<CobolWord>)> {};

  struct CondNameRef
      : rule<create<ast::CondNameRef> +
             assign<&ast::CondNameRef::name>(ref<Identifier>)> {};

  struct SimpleCondition
      : rule<(lit<"("> + ref<ConditionRule> + lit<")">) | ref<RelationCond> |
             ref<ClassCond> | ref<CondNameRef>> {};

  struct CombinableCondition
      : rule<(create<ast::CondNot> + ikw<"NOT"> +
              assign<&ast::CondNot::operand>(ref<SimpleCondition>)) |
             ref<SimpleCondition>> {};

  struct AbbrevCond
      : rule<create<ast::AbbrevCond> +
             option(flag<&ast::AbbrevCond::negated>(ikw<"NOT">)) +
             option(assign<&ast::AbbrevCond::op>(ref<RelOp>)) +
             ((lit<"("> + assign<&ast::AbbrevCond::value>(ref<ArithExpr>) +
               assign<&ast::AbbrevCond::nested>(ref<AbbrevCond>) + lit<")">) |
              assign<&ast::AbbrevCond::value>(ref<ArithExpr>))> {};

  // Left-deepening AND/OR chains: each iteration buries the condition built
  // so far as the new node's `left`, which is what `nest<>` is for -- it
  // must sit inline after the node it buries, not behind a separate ref<>.
  // Two tiers, not one: AND binds tighter than OR, so `a OR b AND c` reads
  // `a OR (b AND c)`.
  struct AndCondition
      : rule<ref<CombinableCondition> +
             many(nest<ast::CondBinary, &ast::CondBinary::left>(
                 assign<&ast::CondBinary::op>(ikw<"AND">) +
                 (assign<&ast::CondBinary::right>(ref<CombinableCondition>) |
                  assign<&ast::CondBinary::right>(ref<AbbrevCond>))))> {};

  struct ConditionRule
      : rule<ref<AndCondition> +
             many(nest<ast::CondBinary, &ast::CondBinary::left>(
                 assign<&ast::CondBinary::op>(ikw<"OR">) +
                 (assign<&ast::CondBinary::right>(ref<AndCondition>) |
                  assign<&ast::CondBinary::right>(ref<AbbrevCond>))))> {};

  // =========================== data division ===========================

  struct UsageWord
      : rule<as<rawSpan>(
            (ikw<"BINARY"> + option(ikw<"TRUNCATED"> | ikw<"EXTENDED">)) |
            ikw<"BIT"> | ikw<"COMP-1"> | ikw<"COMP-2"> | ikw<"COMP-3"> |
            ikw<"COMP-4"> | ikw<"COMP-5"> | ikw<"COMP"> |
            ikw<"COMPUTATIONAL-1"> | ikw<"COMPUTATIONAL-2"> |
            ikw<"COMPUTATIONAL-3"> | ikw<"COMPUTATIONAL-4"> |
            ikw<"COMPUTATIONAL-5"> | ikw<"COMPUTATIONAL"> |
            ikw<"CONTROL-POINT"> | ikw<"DATE"> | ikw<"DISPLAY-1"> |
            ikw<"DISPLAY"> | ikw<"DOUBLE"> | ikw<"EVENT"> |
            ikw<"FUNCTION-POINTER"> | ikw<"INDEX"> | ikw<"KANJI"> |
            ikw<"LOCK"> | ikw<"NATIONAL"> | ikw<"PACKED-DECIMAL"> |
            ikw<"POINTER"> | ikw<"PROCEDURE-POINTER"> | ikw<"REAL"> |
            ikw<"TASK">)> {};

  // Every clause of the three formats, inlined into one alternation: a
  // separate rule per clause would each deduce ast::DataItem as its own
  // head (assign/append target it), clashing with the create<> below that
  // builds the entry itself -- one create, one node, many clauses writing
  // into it. The level number's own spelling (66/77/88) is not made a
  // distinct token either, so RENAMES and a bare VALUE clause are just two
  // more alternatives here rather than a separate production the parser
  // must commit to before reading the name. This over-accepts (e.g. a
  // 66-level with a PICTURE too) but never rejects a valid entry.
  // VALUE items retain literal quotes and both range endpoints. A parts
  // converter inserts canonical spaces: ordinary data-rule text joins tokens
  // without trivia and therefore cannot preserve "1 THRU 9" by itself.
  static constexpr auto DataValueAtom =
      ref<RAW_STRINGLIT> | ref<HEXLIT> | ref<NULLLIT> | ref<DBCSLIT> |
      ikw<"HIGH-VALUES"> | ikw<"HIGH-VALUE"> | ikw<"LOW-VALUES"> | ikw<"LOW-VALUE"> |
      ikw<"NULLS"> | ikw<"NULL"> | ikw<"QUOTES"> | ikw<"QUOTE"> |
      ikw<"SPACES"> | ikw<"SPACE"> | ikw<"ZEROES"> | ikw<"ZEROS"> | ikw<"ZERO"> |
      ref<NUMLIT> | ref<INTLIT> | ikw<"TRUE"> | ikw<"FALSE"> | ref<CobolWord>;
  struct DataValueItem : rule<as<[](std::span<const std::string_view> parts,
                                  std::pmr::memory_resource* mem) {
    std::size_t size = parts.empty() ? 0 : parts.size() - 1;
    for (const auto part : parts) size += part.size();
    const auto room = cursiv::text_room(mem, size);
    std::size_t n = 0;
    for (std::size_t i = 0; i < parts.size(); ++i) {
      if (i) room[n++] = ' ';
      for (const char c : parts[i]) room[n++] = c;
    }
    return std::string_view(room.data(), n);
  }>(option(ikw<"ALL">) + DataValueAtom +
     option((ikw<"THROUGH"> | ikw<"THRU">) + DataValueAtom))> {};

  struct DataOccursSort : rule<((ikw<"ASCENDING"> | ikw<"DESCENDING">) + option(ikw<"KEY">) +
                               option(ikw<"IS">) + some(ref<CobolWord>))> {};

    struct DataDescriptionEntry
      : rule<(create<ast::DataItem> +
              assign<&ast::DataItem::level>(ref<INTLIT>) +
              (flag<&ast::DataItem::filler>(ikw<"FILLER">) |
               option(assign<&ast::DataItem::name>(ref<CobolWord>))) +
              many((ikw<"REDEFINES">+ append<&ast::DataItem::redefines>(ref<CobolWord>)) |
             (ikw<"RENAMES">+ append<&ast::DataItem::renamesFrom>(ref<CobolWord>) +
              option((ikw<"THROUGH"> | ikw<"THRU">) +
                     append<&ast::DataItem::renamesTo>(ref<CobolWord>))) |
             (option(ikw<"IS">) + flag<&ast::DataItem::external>(ikw<"EXTERNAL">) +
              option(ikw<"BY">+ LiteralText)) |
             (option(ikw<"IS">) + flag<&ast::DataItem::global>(ikw<"GLOBAL">)) |
             ((ikw<"PICTURE"> | ikw<"PIC">) + option(ikw<"IS">) +
              append<&ast::DataItem::pictures>(ref<PICTURE>)) |
             (option(ikw<"USAGE">+ option(ikw<"IS">)) +
              append<&ast::DataItem::usages>(ref<UsageWord>)) |
             (((ikw<"VALUE">+ option(ikw<"IS">)) | (ikw<"VALUES">+ option(ikw<"ARE">))) +
              some(append<&ast::DataItem::values>(ref<DataValueItem>), lit<",">)) |
             (ikw<"OCCURS">+ append<&ast::DataItem::occursMin>(ref<INTLIT>) +
              option(ikw<"TO">+ append<&ast::DataItem::occursMax>(ref<INTLIT>)) +
              option(ikw<"TIMES">) +
              option(ikw<"DEPENDING">+ option(ikw<"ON">) +
                     append<&ast::DataItem::occursDependingOn>(ref<CobolWord>)) +
              many(ref<DataOccursSort>) +
              option(ikw<"INDEXED">+ option(ikw<"BY">) + option(ikw<"LOCAL">) +
                     some(append<&ast::DataItem::occursIndexes>(ref<CobolWord>)))) |
             (option(ikw<"SIGN">+ option(ikw<"IS">)) +
              append<&ast::DataItem::signs>(ikw<"LEADING"> | ikw<"TRAILING">) +
              option(flag<&ast::DataItem::signSeparate>(ikw<"SEPARATE">) +
                     option(ikw<"CHARACTER">))) |
             (flag<&ast::DataItem::synchronized>(ikw<"SYNCHRONIZED"> | ikw<"SYNC">) +
              option(ikw<"LEFT"> | ikw<"RIGHT">)) |
             (flag<&ast::DataItem::justified>(ikw<"JUSTIFIED"> | ikw<"JUST">) +
              option(ikw<"RIGHT">)) |
             (ikw<"BLANK">+ option(ikw<"WHEN">) +
              flag<&ast::DataItem::blankWhenZero>(ikw<"ZERO"> | ikw<"ZEROS"> |
                                                        ikw<"ZEROES">)) |
             flag<&ast::DataItem::aligned>(ikw<"ALIGNED">) |
             (option(ikw<"IS">) + flag<&ast::DataItem::thread_local_>(ikw<"THREAD-LOCAL">)) |
             (ikw<"INTEGER"> | ikw<"STRING">) | (ikw<"COMMON"> | ikw<"OWN"> | ikw<"LOCAL">) |
             (option(ikw<"IS">) + ikw<"TYPEDEF">) |
             (ikw<"TYPE">+ option(ikw<"IS">) +
              (ikw<"SHORT-DATE"> | ikw<"LONG-DATE"> | ikw<"NUMERIC-DATE"> | ikw<"NUMERIC-TIME"> |
               ikw<"LONG-TIME">)) |
             (ikw<"USING">+ (ikw<"LANGUAGE"> | ikw<"CONVENTION">) + option(ikw<"OF">) +
              ref<CobolWord>) |
             (option(ikw<"RECEIVED">) + option(ikw<"BY">) +
              (ikw<"CONTENT"> | ikw<"REFERENCE"> | ikw<"REF">)) |
             (ikw<"RECORD">+ ikw<"AREA">) |
             (option(ikw<"WITH">) + ikw<"LOWER">+ ikw<"BOUNDS">)) + ref<DOT_FS>)> {};

  static constexpr auto EmbeddedSqlLines =
      some(append<&ast::EmbeddedSql::lines>(ref<EXECSQLLINE>));
  struct EmbeddedSqlEntry
      : rule<create<ast::EmbeddedSql> + EmbeddedSqlLines + ref<DOT_FS>> {};
  struct OpenEmbeddedSqlEntry : rule<create<ast::EmbeddedSql> + EmbeddedSqlLines> {};

  // An EXEC SQL run with no period is followed by a data item or ends the list.
  template <auto Member>
  static constexpr auto DataEntriesOf =
      many((option(append<Member>(ref<OpenEmbeddedSqlEntry>)) +
            append<Member>(ref<DataDescriptionEntry>)) |
           append<Member>(ref<EmbeddedSqlEntry>)) +
      option(append<Member>(ref<OpenEmbeddedSqlEntry>));


  struct FileDescClause
      : rule<((option(ikw<"IS">) + ikw<"EXTERNAL">) | (option(ikw<"IS">) + ikw<"GLOBAL">) |
             (ikw<"BLOCK"> + option(ikw<"CONTAINS">) + ref<INTLIT> +
                  option(ikw<"TO"> + ref<INTLIT>) + option(ikw<"RECORDS"> | ikw<"CHARACTERS">)) |
             (ikw<"RECORD"> +
                  ((option(ikw<"CONTAINS">) + ref<INTLIT> + option(ikw<"CHARACTERS">)) |
                   (option(ikw<"IS">) + ikw<"VARYING"> + option(ikw<"IN">) + option(ikw<"SIZE">) +
                    option(option(ikw<"FROM">) + ref<INTLIT> +
                           option(ikw<"TO"> + ref<INTLIT>) + option(ikw<"CHARACTERS">)) +
                    option(ikw<"DEPENDING"> + option(ikw<"ON">) + ref<CobolWord>)))) |
             ((ikw<"LABEL"> + ((ikw<"RECORD"> + option(ikw<"IS">)) |
                                   (ikw<"RECORDS"> + option(ikw<"ARE">)))) +
                  (ikw<"OMITTED"> | ikw<"STANDARD"> | some(ref<CobolWord>))) |
             (ikw<"VALUE"> + ikw<"OF"> +
                  some(ref<CobolWord> + option(ikw<"IS">) +
                       (ref<CobolWord> | ref<STRINGLIT> | ref<HEXLIT> | ref<NULLLIT> |
                        ref<DBCSLIT> | ref<NUMLIT> | ref<INTLIT>))) |
             ((ikw<"DATA"> + ((ikw<"RECORD"> + option(ikw<"IS">)) |
                                  (ikw<"RECORDS"> + option(ikw<"ARE">)))) +
                  some(ref<CobolWord>)) |
             (ikw<"LINAGE"> + option(ikw<"IS">) + (ref<CobolWord> | ref<INTLIT>) +
                  option(ikw<"LINES">) +
                  many((option(ikw<"WITH">) + ikw<"FOOTING"> + option(ikw<"AT">) +
                        (ref<CobolWord> | ref<INTLIT>)) |
                       (option(ikw<"LINES">) + option(ikw<"AT">) + ikw<"TOP"> +
                        (ref<CobolWord> | ref<INTLIT>)) |
                       (option(ikw<"LINES">) + option(ikw<"AT">) + ikw<"BOTTOM"> +
                        (ref<CobolWord> | ref<INTLIT>)))) |
             (ikw<"RECORDING"> + option(ikw<"MODE">) + option(ikw<"IS">) + ref<CobolWord>) |
             (ikw<"CODE-SET"> + option(ikw<"IS">) + ref<CobolWord>) |
             (((ikw<"REPORT"> + option(ikw<"IS">)) | (ikw<"REPORTS"> + option(ikw<"ARE">))) +
                  some(ref<CobolWord>)))> {};

  struct FileDescriptionEntry
      : rule<create<ast::FileDescription> +
             (flag<&ast::FileDescription::sortMerge>(ikw<"SD">) | ikw<"FD">) +
             assign<&ast::FileDescription::fileName>(ref<CobolWord>) +
             many(option(ref<DOT_FS>) + ref<FileDescClause>) + ref<DOT_FS> +
             DataEntriesOf<&ast::FileDescription::records>> {};

  // ---- screen / report / communication / library / database sections ----

  struct ScreenDescriptionEntry
      : rule<create<ast::ScreenItem> + ref<INTLIT> +
             option(ikw<"FILLER"> | assign<&ast::ScreenItem::name>(ref<CobolWord>)) +
             many((ikw<"LINE"> + option(ikw<"NUMBER">) + option(ikw<"IS">) +
              option(lit<"+"> | lit<"-">) +
              append<&ast::ScreenItem::lines>(ref<CobolWord> | ref<INTLIT>)) |
             ((ikw<"COLUMN"> | ikw<"COL">) + option(ikw<"NUMBER">) + option(ikw<"IS">) +
              option(lit<"+"> | lit<"-">) +
              append<&ast::ScreenItem::columns>(ref<CobolWord> | ref<INTLIT>)) |
             ((ikw<"PICTURE"> | ikw<"PIC">) + option(ikw<"IS">) +
              append<&ast::ScreenItem::pictures>(ref<PICTURE>)) |
             (ikw<"VALUE"> + option(ikw<"IS">) + append<&ast::ScreenItem::values>(
                  ref<STRINGLIT> | ref<NUMLIT> | ref<INTLIT>)) |
             ((ikw<"FROM"> | ikw<"USING">) +
              append<&ast::ScreenItem::usingOrFrom>(ref<CobolWord> | ref<STRINGLIT>) +
              option(ikw<"TO"> + ref<CobolWord>)) |
             (ikw<"BLANK"> + (ikw<"SCREEN"> | ikw<"LINE">)) |
             (ikw<"BELL"> | ikw<"BEEP">) | (ikw<"BLINK">) |
             (ikw<"ERASE"> + (ikw<"EOL"> | ikw<"EOS">)) |
             (ikw<"HIGHLIGHT"> | ikw<"LOWLIGHT">) |
             (ikw<"GRID"> | ikw<"LEFTLINE"> | ikw<"OVERLINE">) |
             (ikw<"REVERSE-VIDEO">) | (ikw<"UNDERLINE">) |
             (ikw<"SIZE"> + option(ikw<"IS">) + (ref<CobolWord> | ref<INTLIT>)) |
             ((ikw<"FOREGROUND-COLOR"> | ikw<"FOREGROUND-COLOUR"> |
                   ikw<"BACKGROUND-COLOR"> | ikw<"BACKGROUND-COLOUR">) +
                  option(ikw<"IS">) + (ref<CobolWord> | ref<INTLIT>)) |
             (ikw<"CONTROL"> + option(ikw<"IS">) + ref<CobolWord>) |
             (option(ikw<"USAGE"> + option(ikw<"IS">)) + (ikw<"DISPLAY-1"> | ikw<"DISPLAY">)) |
             (ikw<"BLANK"> + option(ikw<"WHEN">) + ikw<"ZERO">) |
             ((ikw<"JUSTIFIED"> | ikw<"JUST">) + option(ikw<"RIGHT">)) |
             (option(ikw<"SIGN"> + option(ikw<"IS">)) + (ikw<"LEADING"> | ikw<"TRAILING">) +
                  option(ikw<"SEPARATE"> + option(ikw<"CHARACTER">))) |
             (ikw<"AUTO"> | ikw<"AUTO-SKIP">) | (ikw<"SECURE"> | ikw<"NO-ECHO">) |
             (ikw<"REQUIRED"> | ikw<"EMPTY-CHECK">) |
             (ikw<"PROMPT"> + option(ikw<"CHARACTER">) + option(ikw<"IS">) +
                  (ref<CobolWord> | ref<STRINGLIT>) +
                  option(ikw<"OCCURS"> + ref<INTLIT> + option(ikw<"TIMES">))) |
             (ikw<"FULL"> | ikw<"LENGTH-CHECK">) | (ikw<"ZERO-FILL">)) +
             ref<DOT_FS>> {};

  struct ReportGroupTypeWord
      : rule<as<rawSpan>(ikw<"TYPE"> + option(ikw<"IS">) +
                         ((ikw<"REPORT"> + ikw<"HEADING">) | ikw<"RH"> |
                          (ikw<"PAGE"> + ikw<"HEADING">) | ikw<"PH"> |
                          (((ikw<"CONTROL"> + ikw<"HEADING">) |
                            ikw<"CH">)+(ikw<"FINAL"> | ref<CobolWord>)) |
                          ikw<"DETAIL"> | ikw<"DE"> |
                          (((ikw<"CONTROL"> + ikw<"FOOTING">) |
                            ikw<"CF">)+(ikw<"FINAL"> | ref<CobolWord>)) |
                          (ikw<"PAGE"> + ikw<"FOOTING">) | ikw<"PF"> |
                          (ikw<"REPORT"> + ikw<"FOOTING">) | ikw<"RF">))> {};

  struct ReportGroupDescriptionEntry
      : rule<create<ast::ReportGroupItem> +
             assign<&ast::ReportGroupItem::level>(ref<INTLIT>) +
             option(assign<&ast::ReportGroupItem::name>(ref<CobolWord>)) +
             many(((ikw<"PICTURE"> | ikw<"PIC">) + option(ikw<"IS">) +
                   append<&ast::ReportGroupItem::pictures>(ref<PICTURE>)) |
                  (ikw<"SOURCE">+ option(ikw<"IS">) +
                   append<&ast::ReportGroupItem::sources>(ref<CobolWord>)) |
                  (ikw<"VALUE">+ option(ikw<"IS">) +
                   append<&ast::ReportGroupItem::values>(ref<STRINGLIT> | ref<NUMLIT> |
                                                          ref<INTLIT>)) |
                  (option(ikw<"LINE">) + option(ikw<"NUMBER">) + option(ikw<"IS">) +
                   ((option(lit<"+">) + append<&ast::ReportGroupItem::lines>(ref<INTLIT>) +
                     option(option(ikw<"ON">) + ikw<"NEXT">+ ikw<"PAGE">)) |
                    (lit<"+">+ append<&ast::ReportGroupItem::lines>(ref<INTLIT>)))) |
                  ((ikw<"COLUMN"> | ikw<"COL">) + option(ikw<"NUMBER">) + option(ikw<"IS">) +
                   append<&ast::ReportGroupItem::columns>(ref<INTLIT>)) |
                  append<&ast::ReportGroupItem::types>(ref<ReportGroupTypeWord>) |
                  (option(ikw<"USAGE">+ option(ikw<"IS">)) + (ikw<"DISPLAY-1"> | ikw<"DISPLAY">)) |
                  (option(ikw<"SIGN">+ option(ikw<"IS">)) + (ikw<"LEADING"> | ikw<"TRAILING">) +
                   ikw<"SEPARATE">+ option(ikw<"CHARACTER">)) |
                  ((ikw<"JUSTIFIED"> | ikw<"JUST">) + option(ikw<"RIGHT">)) |
                  (ikw<"BLANK">+ option(ikw<"WHEN">) + ikw<"ZERO">) |
                  (ikw<"SUM">+ some(ref<CobolWord>, lit<",">) +
                   option(ikw<"UPON">+ some(ref<CobolWord>, lit<",">))) |
                  (ikw<"RESET">+ option(ikw<"ON">) + (ikw<"FINAL"> | ref<CobolWord>)) |
                  (ikw<"GROUP">+ option(ikw<"INDICATE">))) +
             ref<DOT_FS>> {};

  struct ReportDescriptionEntry
      : rule<create<ast::ReportDescription> + ikw<"RD">+
             assign<&ast::ReportDescription::reportName>(ref<CobolWord>) +
             option(option(ikw<"IS">) + ikw<"GLOBAL">) +
             option(ikw<"PAGE"> +
                    ((ikw<"LIMIT"> + option(ikw<"IS">)) | (ikw<"LIMITS"> + option(ikw<"ARE">))) +
                    ref<INTLIT> + option(ikw<"LINE"> | ikw<"LINES">) +
                    option(ikw<"HEADING"> + ref<INTLIT>) +
                    option(ikw<"FIRST"> + ikw<"DETAIL"> + ref<INTLIT>) +
                    option(ikw<"LAST"> + ikw<"DETAIL"> + ref<INTLIT>) +
                    option(ikw<"FOOTING"> + ref<INTLIT>)) +
             ref<DOT_FS> +
             some(append<&ast::ReportDescription::groups>(ref<ReportGroupDescriptionEntry>))> {};

  // Shared CD targets and keyword heads; each direction adds its own clauses.
  static constexpr auto cd_target =
      option(ikw<"IS">) + append<&ast::CommunicationClause::fields>(ref<Identifier>);
  static constexpr auto cd_length_status =
      as<ast::CommunicationClauseKind::textLength>(ikw<"TEXT"> + ikw<"LENGTH">) |
      as<ast::CommunicationClauseKind::statusKey>(ikw<"STATUS"> + ikw<"KEY">);
  static constexpr auto cd_input_common =
      cd_length_status |
      as<ast::CommunicationClauseKind::messageDate>(ikw<"MESSAGE"> + ikw<"DATE">) |
      as<ast::CommunicationClauseKind::messageTime>(ikw<"MESSAGE"> + ikw<"TIME">) |
      as<ast::CommunicationClauseKind::endKey>(ikw<"END"> + ikw<"KEY">);

  struct InputCommunicationClause
      : rule<create<ast::CommunicationClause> +
             (assign<&ast::CommunicationClause::kind>(cd_input_common |
                  as<ast::CommunicationClauseKind::messageCount>(option(ikw<"MESSAGE">) + ikw<"COUNT">)) |
              (option(ikw<"SYMBOLIC">) + assign<&ast::CommunicationClause::kind>(
                  as<ast::CommunicationClauseKind::queue>(ikw<"QUEUE">) |
                  as<ast::CommunicationClauseKind::subQueue1>(ikw<"SUB-QUEUE-1">) |
                  as<ast::CommunicationClauseKind::subQueue2>(ikw<"SUB-QUEUE-2">) |
                  as<ast::CommunicationClauseKind::subQueue3>(ikw<"SUB-QUEUE-3">) |
                  as<ast::CommunicationClauseKind::source>(ikw<"SOURCE">)))) + cd_target> {};

  struct InputOutputCommunicationClause
      : rule<create<ast::CommunicationClause> +
             (assign<&ast::CommunicationClause::kind>(cd_input_common) |
              (option(ikw<"SYMBOLIC">) + assign<&ast::CommunicationClause::kind>(
                  as<ast::CommunicationClauseKind::terminal>(ikw<"TERMINAL">)))) + cd_target> {};

  struct OutputCommunicationClause
      : rule<create<ast::CommunicationClause> +
             ((assign<&ast::CommunicationClause::kind>(cd_length_status |
                   as<ast::CommunicationClauseKind::destinationCount>(ikw<"DESTINATION"> + ikw<"COUNT">) |
                   as<ast::CommunicationClauseKind::errorKey>(ikw<"ERROR"> + ikw<"KEY">) |
                   as<ast::CommunicationClauseKind::destination>(option(ikw<"SYMBOLIC">) + ikw<"DESTINATION">)) + cd_target) |
              (assign<&ast::CommunicationClause::kind>(as<ast::CommunicationClauseKind::destinationTable>(
                   ikw<"DESTINATION"> + ikw<"TABLE">)) + ikw<"OCCURS"> +
               assign<&ast::CommunicationClause::occurs>(ref<INTLIT>) + option(ikw<"TIMES">) +
               option(ikw<"INDEXED"> + option(ikw<"BY">) +
                      some(append<&ast::CommunicationClause::indexes>(ref<CobolWord>)))))> {};

  template <std::size_t Count>
  struct PositionalCommunicationClause
      : rule<create<ast::CommunicationClause> +
             repeat<Count>(append<&ast::CommunicationClause::fields>(ref<Identifier>))> {};

  struct CommunicationDescriptionEntry
      : rule<create<ast::CommunicationEntry> + ikw<"CD"> +
             assign<&ast::CommunicationEntry::cdName>(ref<CobolWord>) + option(ikw<"FOR">) +
             ((option(flag<&ast::CommunicationEntry::initial>(ikw<"INITIAL">)) +
               assign<&ast::CommunicationEntry::direction>(ikw<"INPUT">) +
               (many(append<&ast::CommunicationEntry::clauses>(ref<InputCommunicationClause>)) |
                append<&ast::CommunicationEntry::clauses>(ref<PositionalCommunicationClause<11>>))) |
              (assign<&ast::CommunicationEntry::direction>(ikw<"OUTPUT">) +
               many(append<&ast::CommunicationEntry::clauses>(ref<OutputCommunicationClause>))) |
              (option(flag<&ast::CommunicationEntry::initial>(ikw<"INITIAL">)) +
               assign<&ast::CommunicationEntry::direction>(ikw<"I-O">) +
               (many(append<&ast::CommunicationEntry::clauses>(ref<InputOutputCommunicationClause>)) |
                append<&ast::CommunicationEntry::clauses>(ref<PositionalCommunicationClause<6>>)))) +
             ref<DOT_FS> + many(append<&ast::CommunicationEntry::records>(ref<DataDescriptionEntry>))> {};

  struct LibraryDescriptionEntry
      : rule<create<ast::LibraryEntry> +
             ((flag<&ast::LibraryEntry::export_>(ikw<"LD">) +
               assign<&ast::LibraryEntry::libraryName>(ref<CobolWord>) + ikw<"EXPORT"> +
               many(ref<CobolWord> | ref<STRINGLIT> | ref<HEXLIT> | ref<NULLLIT> |
                    ref<DBCSLIT> | ref<NUMLIT> | ref<INTLIT>)) |
              (ikw<"LB"> + assign<&ast::LibraryEntry::libraryName>(ref<CobolWord>) +
               ikw<"IMPORT"> +
               many(ref<CobolWord> | ref<STRINGLIT> | ref<HEXLIT> | ref<NULLLIT> |
                    ref<DBCSLIT> | ref<NUMLIT> | ref<INTLIT>))) +
             ref<DOT_FS>> {};

  struct DataBaseSectionEntry
      : rule<create<ast::DataBaseEntry> + (ref<INTLIT>) +
             assign<&ast::DataBaseEntry::db>(ref<STRINGLIT>) + ikw<"INVOKE">+
             assign<&ast::DataBaseEntry::invoke>(ref<STRINGLIT>)> {};

  // =========================== environment division ===========================

  // The configuration/special-names/I-O-control paragraphs parse in
  // full (every construct they accept) but write nothing to the tree: this
  // port's tools care about the files a program declares, not device or
  // collating configuration. Values inside are read as raw terminals rather
  // than through Literal/ConditionRule, which build nodes nothing holds.
  static constexpr auto SpecialNameLit = ref<STRINGLIT> | ref<HEXLIT> | ref<NULLLIT> |
                                         ref<DBCSLIT> | ref<NUMLIT> | ref<INTLIT> | ref<CobolWord>;

  struct ConfigurationSection
      : rule<ikw<"CONFIGURATION">+ ikw<"SECTION">+ ref<DOT_FS> +
             many(
                 (ikw<"SOURCE-COMPUTER">+ ref<DOT_FS> + ref<CobolWord> +
                  option(option(ikw<"WITH">) + ikw<"DEBUGGING">+ ikw<"MODE">) + ref<DOT_FS>) |
                 (ikw<"OBJECT-COMPUTER">+ ref<DOT_FS> + ref<CobolWord> +
                  many((ikw<"MEMORY">+ option(ikw<"SIZE">) + (ref<INTLIT> | ref<CobolWord>) +
                        option(ikw<"WORDS"> | ikw<"CHARACTERS"> | ikw<"MODULES">)) |
                       (ikw<"DISK">+ option(ikw<"SIZE">) + option(ikw<"IS">) +
                        (ref<INTLIT> | ref<CobolWord>) + option(ikw<"WORDS"> | ikw<"MODULES">)) |
                       (option(ikw<"PROGRAM">) + option(ikw<"COLLATING">) + ikw<"SEQUENCE">+
                        option(ikw<"IS">) + some(ref<CobolWord>) +
                        option(option(ikw<"FOR">) + ikw<"ALPHANUMERIC">+ option(ikw<"IS">) +
                               ref<CobolWord>) +
                        option(option(ikw<"FOR">) + ikw<"NATIONAL">+ option(ikw<"IS">) +
                               ref<CobolWord>)) |
                       (ikw<"SEGMENT-LIMIT">+ option(ikw<"IS">) + ref<INTLIT>) |
                       (ikw<"CHARACTER">+ ikw<"SET">)) +
                  ref<DOT_FS>) |
                 (ikw<"SPECIAL-NAMES">+ ref<DOT_FS> +
                  option(
                      some((ikw<"CHANNEL">+ ref<INTLIT> + option(ikw<"IS">) + ref<CobolWord>) |
                           (ikw<"ODT">+ option(ikw<"IS">) + ref<CobolWord>) |
                           (ikw<"ALPHABET">+ ref<CobolWord> +
                            ((option(ikw<"FOR">+ ikw<"ALPHANUMERIC">) + option(ikw<"IS">) +
                              (ikw<"EBCDIC"> | ikw<"ASCII"> | ikw<"STANDARD-1"> |
                               ikw<"STANDARD-2"> | ikw<"NATIVE"> | ref<CobolWord> |
                               some(SpecialNameLit +
                                    option((ikw<"THROUGH"> | ikw<"THRU">) + SpecialNameLit) +
                                    many(ikw<"ALSO">+ some(SpecialNameLit))))) |
                             (option(ikw<"FOR">) + ikw<"NATIONAL">+ option(ikw<"IS">) +
                              (ikw<"NATIVE"> | (ikw<"CCSVERSION">+ SpecialNameLit))))) |
                           (ikw<"CLASS">+ ref<CobolWord> +
                            option(option(ikw<"FOR">) + (ikw<"ALPHANUMERIC"> | ikw<"NATIONAL">)) +
                            option(ikw<"IS">) +
                            some(SpecialNameLit +
                                 option((ikw<"THROUGH"> | ikw<"THRU">) + SpecialNameLit))) |
                           (ikw<"CURRENCY">+ option(ikw<"SIGN">) + option(ikw<"IS">) +
                            SpecialNameLit +
                            option(option(ikw<"WITH">) + ikw<"PICTURE">+ ikw<"SYMBOL">+
                                   SpecialNameLit)) |
                           (ikw<"DECIMAL-POINT">+ option(ikw<"IS">) + ikw<"COMMA">) |
                           (ikw<"DEFAULT">+ option(ikw<"COMPUTATIONAL"> | ikw<"COMP">) +
                            option(ikw<"SIGN">+ option(ikw<"IS">)) +
                            option(ikw<"LEADING"> | ikw<"TRAILING">) + ikw<"SEPARATE">+
                            option(ikw<"CHARACTER">)) |
                           (ikw<"DEFAULT-DISPLAY">+ option(ikw<"SIGN">+ option(ikw<"IS">)) +
                            (ikw<"LEADING"> | ikw<"TRAILING">) +
                            option(ikw<"SEPARATE">+ option(ikw<"CHARACTER">))) |
                           (ikw<"RESERVE">+ option(ikw<"WORDS">) + option(ikw<"LIST">) +
                            option(ikw<"IS">) + ikw<"NETWORK">+ option(ikw<"CAPABLE">)) |
                           (ikw<"SYMBOLIC">+ option(ikw<"CHARACTERS">) +
                            option(option(ikw<"FOR">) + (ikw<"ALPHANUMERIC"> | ikw<"NATIONAL">)) +
                            some(some(ref<CobolWord>) + option(ikw<"IS"> | ikw<"ARE">) +
                                 some(ref<INTLIT>)) +
                            option(ikw<"IN">+ ref<CobolWord>)) |
                           (ref<CobolWord> + option(ikw<"IS">) + ref<CobolWord> +
                            option((ikw<"ON">+ option(ikw<"STATUS">) + option(ikw<"IS">) +
                                    some(SpecialNameLit) +
                                    option(ikw<"OFF">+ option(ikw<"STATUS">) + option(ikw<"IS">) +
                                           some(SpecialNameLit))) |
                                   (ikw<"OFF">+ option(ikw<"STATUS">) + option(ikw<"IS">) +
                                    some(SpecialNameLit) +
                                    option(ikw<"ON">+ option(ikw<"STATUS">) + option(ikw<"IS">) +
                                           some(SpecialNameLit)))))) +
                      ref<DOT_FS>)))> {};

  struct IOControlParagraph
      : rule<ikw<"I-O-CONTROL">+ ref<DOT_FS> +
             option(ref<CobolWord> + ref<DOT_FS>) +
             option(many((ikw<"RERUN">+ option(ikw<"ON">+ ref<CobolWord>) + ikw<"EVERY">+
                          ((ref<INTLIT> + ikw<"RECORDS">) |
                           (option(ikw<"END">) + option(ikw<"OF">) + (ikw<"REEL"> | ikw<"UNIT">) +
                            ikw<"OF">+ ref<CobolWord>) |
                           (ref<INTLIT> + option(ikw<"CLOCK-UNITS">)))) |
                         (ikw<"SAME">+ option(ikw<"RECORD"> | ikw<"SORT"> | ikw<"SORT-MERGE">) +
                          option(ikw<"AREA">) + option(ikw<"FOR">) + some(ref<CobolWord>)) |
                         (ikw<"MULTIPLE">+ ikw<"FILE">+ option(ikw<"TAPE">) +
                          option(ikw<"CONTAINS">) +
                          some(ref<CobolWord> + option(ikw<"POSITION">+ ref<INTLIT>))) |
                         (ikw<"COMMITMENT">+ ikw<"CONTROL">+ option(ikw<"FOR">) +
                          ref<CobolWord>)) +
                    ref<DOT_FS>)> {};

  // Every clause of a fileControlEntry inlined, for the same reason DataClause
  // is: a separate rule per clause would each deduce ast::FileControlEntry as
  // its own head, clashing with the create<> below.
  static constexpr auto FileAssignClause =
      ikw<"ASSIGN">+ option(ikw<"TO">) +
      assign<&ast::FileControlEntry::assignTo>(
          ikw<"DISK"> | ikw<"DISPLAY"> | ikw<"KEYBOARD"> | ikw<"PORT"> | ikw<"PRINTER"> |
          ikw<"READER"> | ikw<"REMOTE"> | ikw<"TAPE"> | ikw<"VIRTUAL"> | ref<CobolWord> |
          ref<STRINGLIT>);
  static constexpr auto FileOrganizationClause =
      option(ikw<"ORGANIZATION">+ option(ikw<"IS">)) +
      option(ikw<"LINE"> | (ikw<"RECORD">+ option(ikw<"BINARY">)) | ikw<"BINARY">) +
      assign<&ast::FileControlEntry::organization>(
          ikw<"SEQUENTIAL"> | ikw<"RELATIVE"> | ikw<"INDEXED">);
  static constexpr auto FileAccessClause =
      ikw<"ACCESS">+ option(ikw<"MODE">) + option(ikw<"IS">) +
      assign<&ast::FileControlEntry::accessMode>(
          ikw<"SEQUENTIAL"> | ikw<"RANDOM"> | ikw<"DYNAMIC"> | ikw<"EXCLUSIVE">);
  static constexpr auto FileControlClauses =
      many((option(ikw<"ALTERNATE">) + ikw<"RECORD">+ option(ikw<"KEY">) +
            option(ikw<"IS">) +
            append<&ast::FileControlEntry::recordKeys>(ref<CobolWord>) +
            many((ref<CobolWord> | ref<INTLIT>))) |
           (ikw<"RESERVE">+ (ikw<"NO"> | ref<INTLIT>) + option(ikw<"ALTERNATE">) +
            option(ikw<"AREA"> | ikw<"AREAS">)) |
           (ikw<"PADDING">+ option(ikw<"CHARACTER">) + option(ikw<"IS">) +
            (ref<CobolWord> | ref<STRINGLIT>)) |
           (ikw<"RECORD">+ ikw<"DELIMITER">+ option(ikw<"IS">) +
            (ikw<"STANDARD-1"> | ikw<"IMPLICIT"> | ref<CobolWord>)) |
           (ikw<"PASSWORD">+ option(ikw<"IS">) + ref<CobolWord>) |
           (option(ikw<"FILE">) + ikw<"STATUS">+ option(ikw<"IS">) + ref<CobolWord> +
            option(ref<CobolWord>)) |
           (ikw<"RELATIVE">+ option(ikw<"KEY">) + option(ikw<"IS">) + ref<CobolWord>));
  template <auto First, auto Second>
  static constexpr auto FileControlEitherOrder =
      (First + FileControlClauses + option(Second + FileControlClauses)) |
      (Second + FileControlClauses + option(First + FileControlClauses));

  // ASSIGN, ORGANIZATION and ACCESS once each, in any order among the others.
  struct FileControlEntry
      : rule<create<ast::FileControlEntry> + ikw<"SELECT">+ option(flag<&ast::FileControlEntry::optional_>(ikw<"OPTIONAL">)) +
             assign<&ast::FileControlEntry::fileName>(ref<CobolWord>) + FileControlClauses +
             option((FileAssignClause + FileControlClauses +
                     option(FileControlEitherOrder<FileOrganizationClause, FileAccessClause>)) |
                    (FileOrganizationClause + FileControlClauses +
                     option(FileControlEitherOrder<FileAssignClause, FileAccessClause>)) |
                    (FileAccessClause + FileControlClauses +
                     option(FileControlEitherOrder<FileAssignClause, FileOrganizationClause>)))> {};

  struct EnvironmentDivision
      : rule<create<ast::EnvironmentDivision> + ikw<"ENVIRONMENT">+ ikw<"DIVISION">+ ref<DOT_FS> +
             many(ref<ConfigurationSection> |
                  (ikw<"INPUT-OUTPUT">+ ikw<"SECTION">+ ref<DOT_FS> +
                   option(ikw<"FILE-CONTROL">+
                          many(option(ref<DOT_FS>) +
                               append<&ast::EnvironmentDivision::files>(
                                   ref<FileControlEntry>) +
                               ref<DOT_FS>) +
                          option(ref<DOT_FS>)) +
                   many(ref<IOControlParagraph>)))> {};

  // =========================== procedure division ===========================

  struct Statement
      : rule<ref<AcceptStatement> | ref<AddStatement> | ref<SubtractStatement> |
             ref<MultiplyStatement> | ref<DivideStatement> | ref<ComputeStatement> |
             ref<AlterStatement> | ref<CallStatement> | ref<CancelStatement> |
             ref<CloseStatement> | ref<ContinueStatement> | ref<DeleteStatement> |
             ref<EnableDisableStatement> | ref<DisplayStatement> | ref<EntryStatement> |
             ref<EvaluateStatement> | ref<EmbeddedStatement> | ref<ExhibitStatement> |
             ref<ExitStatement> | ref<GenerateStatement> | ref<GobackStatement> |
             ref<GoToStatement> | ref<IfStatement> | ref<InitializeStatement> |
             ref<InitiateStatement> | ref<InspectStatement> | ref<MergeStatement> |
             ref<MoveStatement> | ref<OpenStatement> | ref<PerformStatement> |
             ref<PurgeStatement> | ref<ReadStatement> | ref<ReceiveStatement> |
             ref<ReleaseStatement> | ref<ReturnStatement> | ref<RewriteStatement> |
             ref<SearchStatement> | ref<SendStatement> | ref<SetStatement> |
             ref<SortStatement> | ref<StartStatement> | ref<StopStatement> |
             ref<StringStatement> | ref<TerminateStatement> | ref<UnstringStatement> |
             ref<WriteStatement>> {};

  // accept

  struct AcceptFromWord
      : rule<as<rawSpan>((ikw<"DATE"> + option(ikw<"YYYYMMDD">)) |
                         (ikw<"DAY"> + option(ikw<"YYYYDDD">)) |
                         ikw<"DAY-OF-WEEK"> | ikw<"TIME"> | ikw<"TIMER"> |
                         (ikw<"TODAYS-DATE"> + option(ikw<"MMDDYYYY">)) |
                         ikw<"TODAYS-NAME"> | ikw<"YEAR"> | ikw<"YYYYMMDD"> |
                         ikw<"YYYYDDD"> | (ikw<"ESCAPE"> + ikw<"KEY">) |
                         ref<CobolWord>)> {};
  struct AcceptCountWord
      : rule<as<rawSpan>(option(ikw<"MESSAGE">) + ikw<"COUNT">)> {};

  struct AcceptStatement
      : rule<create<ast::AcceptStatement> + ikw<"ACCEPT">+
             assign<&ast::AcceptStatement::target>(ref<Identifier>) +
             option((ikw<"FROM">+ assign<&ast::AcceptStatement::source>(ref<AcceptFromWord>)) |
                    assign<&ast::AcceptStatement::source>(ref<AcceptCountWord>)) +
             option(option(ikw<"ON">) + ikw<"EXCEPTION">+
                    many(append<&ast::AcceptStatement::onException>(ref<Statement>))) +
             option(ikw<"NOT">+ option(ikw<"ON">) + ikw<"EXCEPTION">+
                    many(append<&ast::AcceptStatement::notOnException>(ref<Statement>))) +
             option(ikw<"END-ACCEPT">)> {};

  // arithmetic verbs: ADD, SUBTRACT, MULTIPLY, DIVIDE, COMPUTE

  struct RoundedOperand
      : rule<create<ast::RoundedTarget> +
             assign<&ast::RoundedTarget::target>(ref<Identifier>) +
             option(flag<&ast::RoundedTarget::rounded>(ikw<"ROUNDED">))> {};

  struct CorrespondingOperand : rule<ref<Identifier>> {};

  struct AddStatement
      : rule<create<ast::ArithStatement> + assign<&ast::ArithStatement::verb>(ikw<"ADD">) +
             ((assign<&ast::ArithStatement::form>(ikw<"CORRESPONDING"> | ikw<"CORR">) +
               append<&ast::ArithStatement::operands>(ref<CorrespondingOperand>) + ikw<"TO">+
               some(append<&ast::ArithStatement::targets>(ref<RoundedOperand>))) |
              (some(append<&ast::ArithStatement::operands>(ref<IdentifierOrRegister> | ref<Literal>)) +
               ((assign<&ast::ArithStatement::form>(ikw<"TO">) + some(append<&ast::ArithStatement::targets>(ref<RoundedOperand>))) |
                (option(ikw<"TO">+ some(append<&ast::ArithStatement::operands>(ref<IdentifierOrRegister> | ref<Literal>))) +
                 assign<&ast::ArithStatement::form>(ikw<"GIVING">) + some(append<&ast::ArithStatement::targets>(ref<RoundedOperand>)))))) +
             option(option(ikw<"ON">) + ikw<"SIZE">+ ikw<"ERROR">+
                    many(append<&ast::ArithStatement::onSizeError>(ref<Statement>))) +
             option(ikw<"NOT">+ option(ikw<"ON">) + ikw<"SIZE">+ ikw<"ERROR">+
                    many(append<&ast::ArithStatement::notOnSizeError>(ref<Statement>))) +
             option(ikw<"END-ADD">)> {};

  struct SubtractStatement
      : rule<create<ast::ArithStatement> + assign<&ast::ArithStatement::verb>(ikw<"SUBTRACT">) +
             ((assign<&ast::ArithStatement::form>(ikw<"CORRESPONDING"> | ikw<"CORR">) +
               append<&ast::ArithStatement::operands>(ref<CorrespondingOperand>) + ikw<"FROM">+
               append<&ast::ArithStatement::targets>(ref<RoundedOperand>)) |
              (some(append<&ast::ArithStatement::operands>(ref<IdentifierOrRegister> | ref<Literal>)) + ikw<"FROM">+
               ((append<&ast::ArithStatement::operands>(ref<IdentifierOrRegister> | ref<Literal>) + assign<&ast::ArithStatement::form>(ikw<"GIVING">) +
                 some(append<&ast::ArithStatement::targets>(ref<RoundedOperand>))) |
                some(append<&ast::ArithStatement::targets>(ref<RoundedOperand>))))) +
             option(option(ikw<"ON">) + ikw<"SIZE">+ ikw<"ERROR">+
                    many(append<&ast::ArithStatement::onSizeError>(ref<Statement>))) +
             option(ikw<"NOT">+ option(ikw<"ON">) + ikw<"SIZE">+ ikw<"ERROR">+
                    many(append<&ast::ArithStatement::notOnSizeError>(ref<Statement>))) +
             option(ikw<"END-SUBTRACT">)> {};

  struct MultiplyStatement
      : rule<create<ast::ArithStatement> + assign<&ast::ArithStatement::verb>(ikw<"MULTIPLY">) +
             append<&ast::ArithStatement::operands>(ref<IdentifierOrRegister> | ref<Literal>) + ikw<"BY">+
             ((append<&ast::ArithStatement::operands>(ref<IdentifierOrRegister> | ref<Literal>) + assign<&ast::ArithStatement::form>(ikw<"GIVING">) +
               some(append<&ast::ArithStatement::targets>(ref<RoundedOperand>))) |
              some(append<&ast::ArithStatement::targets>(ref<RoundedOperand>))) +
             option(option(ikw<"ON">) + ikw<"SIZE">+ ikw<"ERROR">+
                    many(append<&ast::ArithStatement::onSizeError>(ref<Statement>))) +
             option(ikw<"NOT">+ option(ikw<"ON">) + ikw<"SIZE">+ ikw<"ERROR">+
                    many(append<&ast::ArithStatement::notOnSizeError>(ref<Statement>))) +
             option(ikw<"END-MULTIPLY">)> {};

  struct DivideStatement
      : rule<create<ast::ArithStatement> + assign<&ast::ArithStatement::verb>(ikw<"DIVIDE">) +
             append<&ast::ArithStatement::operands>(ref<IdentifierOrRegister> | ref<Literal>) +
             ((assign<&ast::ArithStatement::form>(ikw<"INTO">) +
               ((append<&ast::ArithStatement::operands>(ref<IdentifierOrRegister> | ref<Literal>) + ikw<"GIVING">+ some(append<&ast::ArithStatement::targets>(ref<RoundedOperand>))) |
                some(append<&ast::ArithStatement::targets>(ref<RoundedOperand>)))) |
              (assign<&ast::ArithStatement::form>(ikw<"BY">) + append<&ast::ArithStatement::operands>(ref<IdentifierOrRegister> | ref<Literal>) +
               option(ikw<"GIVING">+ some(append<&ast::ArithStatement::targets>(ref<RoundedOperand>))))) +
             option(ikw<"REMAINDER">+ assign<&ast::ArithStatement::remainder>(ref<Identifier>)) +
             option(option(ikw<"ON">) + ikw<"SIZE">+ ikw<"ERROR">+
                    many(append<&ast::ArithStatement::onSizeError>(ref<Statement>))) +
             option(ikw<"NOT">+ option(ikw<"ON">) + ikw<"SIZE">+ ikw<"ERROR">+
                    many(append<&ast::ArithStatement::notOnSizeError>(ref<Statement>))) +
             option(ikw<"END-DIVIDE">)> {};

  struct ComputeStatement
      : rule<create<ast::ArithStatement> + assign<&ast::ArithStatement::verb>(ikw<"COMPUTE">) +
             some(append<&ast::ArithStatement::targets>(ref<RoundedOperand>)) + (lit<"="> | ikw<"EQUAL">) +
             assign<&ast::ArithStatement::computeExpr>(ref<ArithExpr>) +
             option(option(ikw<"ON">) + ikw<"SIZE">+ ikw<"ERROR">+
                    many(append<&ast::ArithStatement::onSizeError>(ref<Statement>))) +
             option(ikw<"NOT">+ option(ikw<"ON">) + ikw<"SIZE">+ ikw<"ERROR">+
                    many(append<&ast::ArithStatement::notOnSizeError>(ref<Statement>))) +
             option(ikw<"END-COMPUTE">)> {};

  // alter

  struct AlterStatement
      : rule<create<ast::AlterStatement> + ikw<"ALTER">+
             some(append<&ast::AlterStatement::from>(ref<CobolWord>) + ikw<"TO">+
                  option(ikw<"PROCEED">+ ikw<"TO">) +
                  append<&ast::AlterStatement::to>(ref<CobolWord>))> {};

  // call

  template <ast::CallMode M>
  struct PassingMode : rule<as<M>(cursiv::Epsilon{})> {};
  struct CallAddress : rule<ikw<"ADDRESS"> + ikw<"OF">> {};
  struct CallLength : rule<ikw<"LENGTH"> + option(ikw<"OF">)> {};
  static constexpr auto ArgumentModifier =
      ref<CallAddress> | ref<CallLength> | ikw<"INTEGER"> | ikw<"STRING">;
  // Unprefixed arguments inherit the most recent BY mode. Keep even invalid
  // operand/mode combinations intact for semantic validation.
  static constexpr auto CallArgumentBody =
      flag<&ast::CallArgument::omitted>(ikw<"OMITTED">) |
      (option(assign<&ast::CallArgument::modifier>(ArgumentModifier)) +
       assign<&ast::CallArgument::value>(ref<IdentifierOrRegister> | ref<Literal>));
  template <ast::CallMode M>
  struct CallArgumentOf
      : rule<create<ast::CallArgument> +
             assign<&ast::CallArgument::mode>(ref<PassingMode<M>>) + CallArgumentBody> {
  };
  static constexpr auto CallByReferenceRun = some(append<&ast::CallStatement::using_>(
      ref<CallArgumentOf<ast::CallMode::reference>>));
  static constexpr auto CallUsingPhrase =
      (option(ikw<"BY">) + ikw<"REFERENCE"> + CallByReferenceRun) |
      (ikw<"BY"> + ikw<"VALUE"> +
       some(append<&ast::CallStatement::using_>(
           ref<CallArgumentOf<ast::CallMode::value>>))) |
      (ikw<"BY"> + ikw<"CONTENT"> +
       some(append<&ast::CallStatement::using_>(
           ref<CallArgumentOf<ast::CallMode::content>>)));
  struct CallStatement
      : rule<create<ast::CallStatement> + ikw<"CALL">+
             assign<&ast::CallStatement::program>(ref<IdentifierOrRegister> | ref<Literal>) +
             option(ikw<"USING">+ (CallByReferenceRun | CallUsingPhrase) +
                    many(CallUsingPhrase)) +
             option((ikw<"GIVING"> | ikw<"RETURNING">) +
                    assign<&ast::CallStatement::giving>(ref<Identifier>)) +
             option(option(ikw<"ON">) + ikw<"OVERFLOW">+
                    many(append<&ast::CallStatement::onOverflow>(ref<Statement>))) +
             option(option(ikw<"ON">) + ikw<"EXCEPTION">+
                    many(append<&ast::CallStatement::onException>(ref<Statement>))) +
             option(ikw<"NOT">+ option(ikw<"ON">) + ikw<"EXCEPTION">+
                    many(append<&ast::CallStatement::notOnException>(ref<Statement>))) +
             option(ikw<"END-CALL">)> {};

  // cancel

  struct CancelStatement
      : rule<create<ast::CancelStatement> + ikw<"CANCEL">+
             some(append<&ast::CancelStatement::targets>(ref<IdentifierOrRegister> | ref<Literal>) +
                  option(ikw<"BYTITLE"> | ikw<"BYFUNCTION">))> {};

  // close

  struct CloseStatement
      : rule<create<ast::CloseStatement> + ikw<"CLOSE">+
             some(append<&ast::CloseStatement::files>(ref<CobolWord>) +
                  option(((ikw<"REEL"> | ikw<"UNIT">) +
                          option(option(ikw<"FOR">) + ikw<"REMOVAL">) +
                          option(option(ikw<"WITH">) + ((ikw<"NO">+ ikw<"REWIND">) | ikw<"LOCK">))) |
                         (option(ikw<"WITH">) + ((ikw<"NO">+ ikw<"REWIND">) | ikw<"LOCK">)) |
                         ((option(ikw<"WITH">) + ikw<"NO">+ ikw<"WAIT">) |
                          (ikw<"WITH">+ ikw<"WAIT">))))> {};

  // continue

  struct ContinueStatement : rule<create<ast::ContinueStatement> + ikw<"CONTINUE">> {};

  // delete

  struct DeleteStatement
      : rule<create<ast::DeleteStatement> + ikw<"DELETE">+
             assign<&ast::DeleteStatement::fileName>(ref<CobolWord>) + option(ikw<"RECORD">) +
             option(ikw<"INVALID">+ option(ikw<"KEY">) +
                    many(append<&ast::DeleteStatement::invalidKey>(ref<Statement>))) +
             option(ikw<"NOT">+ ikw<"INVALID">+ option(ikw<"KEY">) +
                    many(append<&ast::DeleteStatement::notInvalidKey>(ref<Statement>))) +
             option(ikw<"END-DELETE">)> {};

  // enable / disable

  struct EnableDirectionWord
      : rule<as<rawSpan>((ikw<"INPUT"> + option(ikw<"TERMINAL">)) |
                         (ikw<"I-O"> + ikw<"TERMINAL">) | ikw<"OUTPUT">)> {};

  struct DisableStatement
      : rule<create<ast::EnableDisableStatement> + ikw<"DISABLE">+
             assign<&ast::EnableDisableStatement::direction>(ref<EnableDirectionWord>) +
             assign<&ast::EnableDisableStatement::cdName>(ref<CobolWord>) + option(ikw<"WITH">) +
             ikw<"KEY">+
             assign<&ast::EnableDisableStatement::key>(ref<IdentifierOrRegister> | ref<Literal>)> {};
  struct EnableStatement
      : rule<create<ast::EnableDisableStatement> +
             flag<&ast::EnableDisableStatement::enable>(ikw<"ENABLE">) +
             assign<&ast::EnableDisableStatement::direction>(ref<EnableDirectionWord>) +
             assign<&ast::EnableDisableStatement::cdName>(ref<CobolWord>) + option(ikw<"WITH">) +
             ikw<"KEY">+
             assign<&ast::EnableDisableStatement::key>(ref<IdentifierOrRegister> | ref<Literal>)> {};
  struct EnableDisableStatement : rule<ref<EnableStatement> | ref<DisableStatement>> {};

  // display

  struct DisplayStatement
      : rule<create<ast::DisplayStatement> + ikw<"DISPLAY">+
             some(append<&ast::DisplayStatement::operands>(ref<IdentifierOrRegister> |
                                                            ref<Literal>)) +
             option(ikw<"AT">+
                    assign<&ast::DisplayStatement::at>(ref<IdentifierOrRegister> | ref<Literal>)) +
             option(ikw<"UPON">+ assign<&ast::DisplayStatement::upon>(ref<CobolWord>)) +
             option(option(ikw<"WITH">) + ikw<"NO">+
                    flag<&ast::DisplayStatement::noAdvancing>(ikw<"ADVANCING">))> {};

  // entry

  struct EntryStatement
      : rule<create<ast::EntryStatement> + ikw<"ENTRY">+
             assign<&ast::EntryStatement::literalText>(ref<STRINGLIT>) +
             option(ikw<"USING">+ some(append<&ast::EntryStatement::using_>(ref<Identifier>)))> {};

  // evaluate

  struct EvalSubjectValue
      : rule<create<ast::EvaluateSubject> +
             (assign<&ast::EvaluateSubject::value>(ref<ArithExpr>) |
              assign<&ast::EvaluateSubject::cond>(ref<ConditionRule>))> {};
  struct EvalValueOne
      : rule<create<ast::EvaluateValue> +
             (flag<&ast::EvaluateValue::any>(ikw<"ANY">) |
              assign<&ast::EvaluateValue::boolLiteral>(ikw<"TRUE"> | ikw<"FALSE">) |
              (option(flag<&ast::EvaluateValue::negated>(ikw<"NOT">)) +
               assign<&ast::EvaluateValue::value>(ref<ArithExpr>) +
               option((ikw<"THROUGH"> | ikw<"THRU">) +
                      assign<&ast::EvaluateValue::through>(ref<ArithExpr>))) |
              assign<&ast::EvaluateValue::cond>(ref<ConditionRule>))> {};
  struct EvalWhen
      : rule<create<ast::EvaluateWhen> + ikw<"WHEN">+
             append<&ast::EvaluateWhen::values>(ref<EvalValueOne>) +
             many(ikw<"ALSO">+ append<&ast::EvaluateWhen::values>(ref<EvalValueOne>))> {};
  static constexpr auto EvalWhens =
      some(append<&ast::EvaluateWhenPhrase::whens>(ref<EvalWhen>));
  struct EvalWhenPhrase
      : rule<create<ast::EvaluateWhenPhrase> + EvalWhens +
             some(append<&ast::EvaluateWhenPhrase::body>(ref<Statement>))> {};
  struct EvalBareWhenPhrase : rule<create<ast::EvaluateWhenPhrase> + EvalWhens> {};
  struct EvaluateStatement
      : rule<create<ast::EvaluateStatement> + ikw<"EVALUATE">+
             append<&ast::EvaluateStatement::subjects>(ref<EvalSubjectValue>) +
             many(ikw<"ALSO">+ append<&ast::EvaluateStatement::subjects>(ref<EvalSubjectValue>)) +
             ((some(append<&ast::EvaluateStatement::whenPhrases>(ref<EvalWhenPhrase>)) +
               option(append<&ast::EvaluateStatement::whenPhrases>(ref<EvalBareWhenPhrase>))) |
              append<&ast::EvaluateStatement::whenPhrases>(ref<EvalBareWhenPhrase>)) +
             option(flag<&ast::EvaluateStatement::hasWhenOther>(ikw<"WHEN">+ ikw<"OTHER">) +
                    many(append<&ast::EvaluateStatement::whenOther>(ref<Statement>))) +
             option(ikw<"END-EVALUATE">)> {};

  // exec cics / sql / sqlims

  struct ExecCicsStatement
      : rule<create<ast::EmbeddedStatement> + assign<&ast::EmbeddedStatement::kind>(ikw<"CICS">) +
             some(append<&ast::EmbeddedStatement::lines>(ref<EXECCICSLINE>))> {};
  struct ExecSqlStatement
      : rule<create<ast::EmbeddedStatement> + assign<&ast::EmbeddedStatement::kind>(ikw<"SQL">) +
             some(append<&ast::EmbeddedStatement::lines>(ref<EXECSQLLINE>))> {};
  struct ExecSqlImsStatement
      : rule<create<ast::EmbeddedStatement> + assign<&ast::EmbeddedStatement::kind>(ikw<"SQLIMS">) +
             some(append<&ast::EmbeddedStatement::lines>(ref<EXECSQLIMSLINE>))> {};
  struct EmbeddedStatement
      : rule<ref<ExecCicsStatement> | ref<ExecSqlStatement> | ref<ExecSqlImsStatement>> {};

  // exhibit

  struct ExhibitStatement
      : rule<create<ast::ExhibitStatement> + ikw<"EXHIBIT">+
             option(flag<&ast::ExhibitStatement::named>(ikw<"NAMED">)) +
             option(flag<&ast::ExhibitStatement::changed>(ikw<"CHANGED">)) +
             some(append<&ast::ExhibitStatement::operands>(ref<IdentifierOrRegister> | ref<Literal>))> {};

  // exit

  struct ExitStatement
      : rule<create<ast::ExitStatement> + ikw<"EXIT">+
             option(flag<&ast::ExitStatement::program>(ikw<"PROGRAM">))> {};

  // generate

  struct GenerateStatement
      : rule<create<ast::GenerateStatement> + ikw<"GENERATE">+
             assign<&ast::GenerateStatement::reportName>(ref<Identifier>)> {};

  // goback

  struct GobackStatement : rule<create<ast::GobackStatement> + ikw<"GOBACK">> {};

  // go to

  struct GoToStatement
      : rule<create<ast::GoToStatement> + ikw<"GO">+ option(ikw<"TO">) +
             (flag<&ast::GoToStatement::moreLabels>(ikw<"MORE-LABELS">) |
              (some(append<&ast::GoToStatement::targets>(ref<CobolWord>)) +
               option(ikw<"DEPENDING">+ option(ikw<"ON">) +
                      assign<&ast::GoToStatement::dependingOn>(ref<Identifier>))))> {};

  // if

  struct IfStatement
      : rule<create<ast::IfStatement> + ikw<"IF">+
             assign<&ast::IfStatement::cond>(ref<ConditionRule>) + option(ikw<"THEN">) +
             (flag<&ast::IfStatement::thenNextSentence>(ikw<"NEXT">+ ikw<"SENTENCE">) |
              many(append<&ast::IfStatement::thenBranch>(ref<Statement>))) +
             option(flag<&ast::IfStatement::hasElse>(ikw<"ELSE">) +
                    (flag<&ast::IfStatement::elseNextSentence>(ikw<"NEXT">+ ikw<"SENTENCE">) |
                     many(append<&ast::IfStatement::elseBranch>(ref<Statement>)))) +
             option(ikw<"END-IF">)> {};

  // initialize

  struct InitReplaceOne
      : rule<create<ast::InitializeReplacing> +
             assign<&ast::InitializeReplacing::category>(
                 ikw<"ALPHABETIC-LOWER"> | ikw<"ALPHABETIC-UPPER"> | ikw<"ALPHABETIC"> |
                 ikw<"ALPHANUMERIC-EDITED"> | ikw<"ALPHANUMERIC"> | ikw<"NATIONAL-EDITED"> |
                 ikw<"NATIONAL"> | ikw<"NUMERIC-EDITED"> | ikw<"NUMERIC"> | ikw<"DBCS"> |
                 ikw<"EGCS">) +
             option(ikw<"DATA">) + ikw<"BY">+
             assign<&ast::InitializeReplacing::value>(ref<IdentifierOrRegister> | ref<Literal>)> {};
  struct InitializeStatement
      : rule<create<ast::InitializeStatement> + ikw<"INITIALIZE">+
             some(append<&ast::InitializeStatement::targets>(ref<Identifier>)) +
             option(ikw<"REPLACING">+
                    some(append<&ast::InitializeStatement::replacing>(ref<InitReplaceOne>)))> {};

  // initiate

  struct InitiateStatement
      : rule<create<ast::InitiateStatement> + ikw<"INITIATE">+
             some(append<&ast::InitiateStatement::reports>(ref<Identifier>))> {};

  // inspect

  struct InspectCharPattern;
  struct InspectCharReplacement;
  struct InspectOperand : rule<ref<IdentifierOrRegister> | ref<Literal>> {};
  struct InspectBeforeAfterOne
      : rule<assign<&ast::InspectBoundary::kind>(ikw<"BEFORE"> | ikw<"AFTER">) +
             option(flag<&ast::InspectBoundary::initial>(ikw<"INITIAL">)) +
             assign<&ast::InspectBoundary::operand>(ref<InspectOperand>)> {};
  struct InspectCharsTallying
      : rule<assign<&ast::InspectGroup::kind>(ikw<"CHARACTERS">) +
             append<&ast::InspectGroup::patterns>(ref<InspectCharPattern>)> {};
  struct InspectCharPattern
      : rule<create<ast::InspectPattern>(many(
            append<&ast::InspectPattern::boundaries>(ref<InspectBeforeAfterOne>)))> {};
  struct InspectAllLeadOne
      : rule<assign<&ast::InspectPattern::from>(ref<InspectOperand>) +
             many(append<&ast::InspectPattern::boundaries>(
                 ref<InspectBeforeAfterOne>))> {};
  struct InspectAllLeadings
      : rule<assign<&ast::InspectGroup::kind>(ikw<"ALL"> | ikw<"LEADING">) +
             some(append<&ast::InspectGroup::patterns>(ref<InspectAllLeadOne>))> {};
  struct InspectForOne
      : rule<assign<&ast::InspectTally::counter>(ref<InspectOperand>) + ikw<"FOR"> +
             some(append<&ast::InspectTally::groups>(ref<InspectCharsTallying> |
                                                     ref<InspectAllLeadings>))> {};
  static constexpr auto InspectTallying =
      flag<&ast::InspectStatement::tallying>(ikw<"TALLYING">) +
      some(append<&ast::InspectStatement::tallies>(ref<InspectForOne>));
  struct InspectReplCharsOne
      : rule<assign<&ast::InspectGroup::kind>(ikw<"CHARACTERS">) +
             append<&ast::InspectGroup::patterns>(ref<InspectCharReplacement>)> {};
  struct InspectCharReplacement
      : rule<ikw<"BY"> + assign<&ast::InspectPattern::to>(ref<InspectOperand>) +
             many(append<&ast::InspectPattern::boundaries>(
                 ref<InspectBeforeAfterOne>))> {};
  struct InspectReplAllLeadOne
      : rule<assign<&ast::InspectPattern::from>(ref<InspectOperand>) + ikw<"BY"> +
             assign<&ast::InspectPattern::to>(ref<InspectOperand>) +
             many(append<&ast::InspectPattern::boundaries>(
                 ref<InspectBeforeAfterOne>))> {};
  struct InspectReplAllLeadings
      : rule<assign<&ast::InspectGroup::kind>(ikw<"ALL"> | ikw<"LEADING"> |
                                              ikw<"FIRST">) +
             some(append<&ast::InspectGroup::patterns>(ref<InspectReplAllLeadOne>))> {};
  static constexpr auto InspectReplacing =
      flag<&ast::InspectStatement::replacing>(ikw<"REPLACING">) + some(append<&ast::InspectStatement::replacements>(
                             ref<InspectReplCharsOne> | ref<InspectReplAllLeadings>));
  struct InspectConverting
      : rule<ikw<"CONVERTING"> +
             assign<&ast::InspectPattern::from>(ref<InspectOperand>) + ikw<"TO"> +
             assign<&ast::InspectPattern::to>(ref<InspectOperand>) +
             many(append<&ast::InspectPattern::boundaries>(
                 ref<InspectBeforeAfterOne>))> {};
  struct InspectStatement
      : rule<create<ast::InspectStatement> + ikw<"INSPECT"> +
             assign<&ast::InspectStatement::target>(ref<Identifier>) +
             ((InspectTallying + option(InspectReplacing)) | InspectReplacing |
              flag<&ast::InspectStatement::converting>(
                  assign<&ast::InspectStatement::conversion>(
                      ref<InspectConverting>)))> {};

  // merge

  struct MergeStatement
      : rule<create<ast::MergeStatement> + ikw<"MERGE">+
             assign<&ast::MergeStatement::fileName>(ref<CobolWord>) +
             some(option(ikw<"ON">) + (ikw<"ASCENDING"> | ikw<"DESCENDING">) + option(ikw<"KEY">) +
                  option(ikw<"IS">) + some(append<&ast::MergeStatement::keys>(ref<CobolWord>))) +
             option((option(ikw<"COLLATING">) + ikw<"SEQUENCE">+ option(ikw<"IS">)) +
                    some(ref<CobolWord>) +
                    option((option(ikw<"FOR">) + ikw<"ALPHANUMERIC">+ ikw<"IS">) +
                           ref<CobolWord>) +
                    option((option(ikw<"FOR">) + ikw<"NATIONAL">+ option(ikw<"IS">)) +
                           ref<CobolWord>)) +
             many(ikw<"USING">+ some(append<&ast::MergeStatement::using_>(ref<CobolWord>))) +
             option(ikw<"OUTPUT">+ ikw<"PROCEDURE">+ option(ikw<"IS">) +
                    assign<&ast::MergeStatement::outputProcedure>(ref<CobolWord>) +
                    option((ikw<"THROUGH"> | ikw<"THRU">) + ref<CobolWord>)) +
             many(ikw<"GIVING">+
                  some(append<&ast::MergeStatement::giving>(ref<CobolWord>) +
                       option(ikw<"LOCK"> | ikw<"SAVE"> | (ikw<"NO">+ ikw<"REWIND">) |
                              ikw<"CRUNCH"> | ikw<"RELEASE"> |
                              (ikw<"WITH">+ ikw<"REMOVE">+ ikw<"CRUNCH">))))> {};

  // move

  struct MoveStatement
      : rule<create<ast::MoveStatement> + ikw<"MOVE">+
             option(flag<&ast::MoveStatement::all>(ikw<"ALL">)) +
             ((flag<&ast::MoveStatement::corresponding>(ikw<"CORRESPONDING"> | ikw<"CORR">) +
               assign<&ast::MoveStatement::from>(ref<Identifier>)) |
              assign<&ast::MoveStatement::from>(ref<IdentifierOrRegister> | ref<Literal>)) +
             ikw<"TO">+ some(append<&ast::MoveStatement::to>(ref<Identifier>))> {};

  // open

  struct OpenInputGroup
      : rule<create<ast::OpenGroup> + assign<&ast::OpenGroup::mode>(ikw<"INPUT">) +
             some(append<&ast::OpenGroup::files>(ref<CobolWord>) +
                  option(ikw<"REVERSED"> | (option(ikw<"WITH">) + ikw<"NO">+ ikw<"REWIND">)))> {};
  struct OpenOutputGroup
      : rule<create<ast::OpenGroup> + assign<&ast::OpenGroup::mode>(ikw<"OUTPUT">) +
             some(append<&ast::OpenGroup::files>(ref<CobolWord>) +
                  option(option(ikw<"WITH">) + ikw<"NO">+ ikw<"REWIND">))> {};
  struct OpenIOGroup
      : rule<create<ast::OpenGroup> + assign<&ast::OpenGroup::mode>(ikw<"I-O">) +
             some(append<&ast::OpenGroup::files>(ref<CobolWord>))> {};
  struct OpenExtendGroup
      : rule<create<ast::OpenGroup> + assign<&ast::OpenGroup::mode>(ikw<"EXTEND">) +
             some(append<&ast::OpenGroup::files>(ref<CobolWord>))> {};
  struct OpenStatement
      : rule<create<ast::OpenStatement> + ikw<"OPEN">+
             some(append<&ast::OpenStatement::groups>(
                 ref<OpenInputGroup> | ref<OpenOutputGroup> | ref<OpenIOGroup> | ref<OpenExtendGroup>))> {};

  // perform

  struct PerformVaryingOne
      : rule<create<ast::PerformVarying> +
             assign<&ast::PerformVarying::index>(ref<IdentifierOrRegister> | ref<Literal>) +
             ikw<"FROM">+ assign<&ast::PerformVarying::from>(ref<ArithExpr>) + ikw<"BY">+
             assign<&ast::PerformVarying::by>(ref<ArithExpr>) + ikw<"UNTIL">+
             assign<&ast::PerformVarying::until>(ref<ConditionRule>)> {};
  // performType is inlined into PerformStatement directly (twice, once per
  // format), since its own TEST/VARYING clauses write into
  // ast::PerformStatement too; the two shared pieces are named constants,
  // not rules, so splicing them in does not itself deduce a class.
  static constexpr auto PerformTestBody =
      option(ikw<"WITH">) + ikw<"TEST">+
      (flag<&ast::PerformStatement::testAfter>(ikw<"AFTER">) | ikw<"BEFORE">);
  static constexpr auto PerformVarying_ =
      ikw<"VARYING">+ append<&ast::PerformStatement::varying>(ref<PerformVaryingOne>) +
      many(ikw<"AFTER">+ append<&ast::PerformStatement::varying>(ref<PerformVaryingOne>));
  static constexpr auto PerformType_ =
      (assign<&ast::PerformStatement::times>(ref<ArithExpr>) + ikw<"TIMES">) |
      (option(PerformTestBody) + ikw<"UNTIL">+
       assign<&ast::PerformStatement::until>(ref<ConditionRule>)) |
      (PerformTestBody + PerformVarying_) | (PerformVarying_ + option(PerformTestBody));

  struct PerformStatement
      : rule<create<ast::PerformStatement> + ikw<"PERFORM">+
             ((option(PerformType_) +
               many(append<&ast::PerformStatement::body>(ref<Statement>)) +
               flag<&ast::PerformStatement::inline_>(ikw<"END-PERFORM">)) |
              (assign<&ast::PerformStatement::procedureFrom>(ref<CobolWord>) +
               option((ikw<"THROUGH"> | ikw<"THRU">) +
                      assign<&ast::PerformStatement::procedureThrough>(ref<CobolWord>)) +
               option(PerformType_)))> {};

  // purge

  struct PurgeStatement
      : rule<create<ast::PurgeStatement> + ikw<"PURGE">+
             some(append<&ast::PurgeStatement::queues>(ref<CobolWord>))> {};

  // read

  struct ReadStatement
      : rule<create<ast::ReadStatement> + ikw<"READ">+
             assign<&ast::ReadStatement::fileName>(ref<CobolWord>) +
             option(flag<&ast::ReadStatement::next>(ikw<"NEXT">)) + option(ikw<"RECORD">) +
             option(ikw<"INTO">+ assign<&ast::ReadStatement::into>(ref<Identifier>)) +
             option((option(ikw<"WITH">) + (((ikw<"KEPT"> | ikw<"NO">) + ikw<"LOCK">) |
                                                ikw<"WAIT">))) +
             option(ikw<"KEY">+ option(ikw<"IS">) + assign<&ast::ReadStatement::key>(ref<CobolWord>)) +
             option(ikw<"INVALID">+ option(ikw<"KEY">) +
                    many(append<&ast::ReadStatement::invalidKey>(ref<Statement>))) +
             option(ikw<"NOT">+ ikw<"INVALID">+ option(ikw<"KEY">) +
                    many(append<&ast::ReadStatement::notInvalidKey>(ref<Statement>))) +
             option(option(ikw<"AT">) + ikw<"END">+
                    many(append<&ast::ReadStatement::atEnd>(ref<Statement>))) +
             option(ikw<"NOT">+ option(ikw<"AT">) + ikw<"END">+
                    many(append<&ast::ReadStatement::notAtEnd>(ref<Statement>))) +
             option(ikw<"END-READ">)> {};

  // receive

  struct ReceiveTrailing
      : rule<(((ikw<"BEFORE">+ option(ikw<"TIME">) + (ref<NUMLIT> | ref<INTLIT> | ref<CobolWord>)) |
                  (option(ikw<"WITH">) + ikw<"NO">+ ikw<"WAIT">) |
                  (ikw<"THREAD">+ option(ikw<"IN">) + ref<CobolWord>) |
                  (ikw<"SIZE">+ option(ikw<"IN">) + (ref<NUMLIT> | ref<INTLIT> | ref<CobolWord>)) |
                  (ikw<"STATUS">+ option(ikw<"IN">) + ref<CobolWord>)))> {};
  struct ReceiveStatement
      : rule<create<ast::ReceiveStatement> + ikw<"RECEIVE">+
             ((assign<&ast::ReceiveStatement::text>(ref<CobolWord>) + ikw<"FROM">+
               ((ikw<"THREAD">+ ref<CobolWord>) | (ikw<"LAST">+ ikw<"THREAD">) |
                    (ikw<"ANY">+ ikw<"THREAD">)) +
               many(ref<ReceiveTrailing>)) |
              (assign<&ast::ReceiveStatement::text>(ikw<"MESSAGE"> | ikw<"SEGMENT">) +
               option(ikw<"INTO">) + (ref<CobolWord>) +
               option(ikw<"NO">+ ikw<"DATA">+
                      many(append<&ast::ReceiveStatement::onException>(ref<Statement>))) +
               option(ikw<"WITH">+ ikw<"DATA">+
                      many(append<&ast::ReceiveStatement::notOnException>(ref<Statement>))))) +
             option(option(ikw<"ON">) + ikw<"EXCEPTION">+
                    many(append<&ast::ReceiveStatement::onException>(ref<Statement>))) +
             option(ikw<"NOT">+ option(ikw<"ON">) + ikw<"EXCEPTION">+
                    many(append<&ast::ReceiveStatement::notOnException>(ref<Statement>))) +
             option(ikw<"END-RECEIVE">)> {};

  // release

  struct ReleaseStatement
      : rule<create<ast::ReleaseStatement> + ikw<"RELEASE">+
             assign<&ast::ReleaseStatement::recordName>(ref<CobolWord>) +
             option(ikw<"FROM">+ assign<&ast::ReleaseStatement::from>(ref<Identifier>))> {};

  // return

  struct ReturnStatement
      : rule<create<ast::ReturnStatement> + ikw<"RETURN">+
             assign<&ast::ReturnStatement::fileName>(ref<CobolWord>) + option(ikw<"RECORD">) +
             option(ikw<"INTO">+ assign<&ast::ReturnStatement::into>(ref<Identifier>)) +
             option(ikw<"AT">) + ikw<"END">+
             many(append<&ast::ReturnStatement::atEnd>(ref<Statement>)) +
             option(ikw<"NOT">+ option(ikw<"AT">) + ikw<"END">+
                    many(append<&ast::ReturnStatement::notAtEnd>(ref<Statement>))) +
             option(ikw<"END-RETURN">)> {};

  // rewrite

  struct RewriteStatement
      : rule<create<ast::RewriteStatement> + ikw<"REWRITE">+
             assign<&ast::RewriteStatement::recordName>(ref<CobolWord>) +
             option(ikw<"FROM">+ assign<&ast::RewriteStatement::from>(ref<Identifier>)) +
             option(ikw<"INVALID">+ option(ikw<"KEY">) +
                    many(append<&ast::RewriteStatement::invalidKey>(ref<Statement>))) +
             option(ikw<"NOT">+ ikw<"INVALID">+ option(ikw<"KEY">) +
                    many(append<&ast::RewriteStatement::notInvalidKey>(ref<Statement>))) +
             option(ikw<"END-REWRITE">)> {};

  // search

  struct SearchWhenOne
      : rule<create<ast::SearchWhen> + ikw<"WHEN">+
             assign<&ast::SearchWhen::cond>(ref<ConditionRule>) +
             (flag<&ast::SearchWhen::nextSentence>(ikw<"NEXT">+ ikw<"SENTENCE">) |
              many(append<&ast::SearchWhen::body>(ref<Statement>)))> {};
  struct SearchStatement
      : rule<create<ast::SearchStatement> + ikw<"SEARCH">+
             option(flag<&ast::SearchStatement::all>(ikw<"ALL">)) +
             assign<&ast::SearchStatement::tableName>(ref<CobolWord>) +
             option(ikw<"VARYING">+ assign<&ast::SearchStatement::varying>(ref<Identifier>)) +
             option(option(ikw<"AT">) + ikw<"END">+
                    many(append<&ast::SearchStatement::atEnd>(ref<Statement>))) +
             some(append<&ast::SearchStatement::whens>(ref<SearchWhenOne>)) +
             option(ikw<"END-SEARCH">)> {};

  // send

  struct SendStatement
      : rule<create<ast::SendStatement> + ikw<"SEND">+
             ((ikw<"TO">+ (ikw<"TOP"> | ikw<"BOTTOM">) +
               assign<&ast::SendStatement::text>(ref<CobolWord>)) |
              (assign<&ast::SendStatement::text>(ref<CobolWord> | ref<STRINGLIT>) +
               option(ikw<"FROM">+ ref<CobolWord>) +
               option(ikw<"WITH">+ (ikw<"EGI"> | ikw<"EMI"> | ikw<"ESI"> | ref<CobolWord>)) +
               option(ikw<"REPLACING">+ option(ikw<"LINE">)) +
               option((ikw<"BEFORE"> | ikw<"AFTER">) + option(ikw<"ADVANCING">) +
                      (ikw<"PAGE"> | (ref<CobolWord> + option(ikw<"LINE"> | ikw<"LINES">)) |
                       ((ref<NUMLIT> | ref<INTLIT>) + option(ikw<"LINE"> | ikw<"LINES">)))))) +
             option((option(ikw<"ON">) + ikw<"EXCEPTION">)) +
             option((ikw<"NOT">+ option(ikw<"ON">) + ikw<"EXCEPTION">))> {};

  // set

  struct SetUpDownOp
      : rule<as<rawSpan>((ikw<"UP"> + ikw<"BY">) | (ikw<"DOWN"> + ikw<"BY">))> {
  };

  struct SetStatement
      : rule<create<ast::SetStatement> + ikw<"SET">+
             some(append<&ast::SetStatement::targets>(ref<Identifier>)) +
             ((assign<&ast::SetStatement::op>(ikw<"TO">) +
               some(append<&ast::SetStatement::values>(
                   (create<ast::Literal> + assign<&ast::Literal::text>(ikw<"ON"> | ikw<"OFF">)) |
                   (ikw<"ENTRY">+ (ref<IdentifierOrRegister> | ref<Literal>)) |
                   ref<IdentifierOrRegister> | ref<Literal>))) |
              (assign<&ast::SetStatement::op>(ref<SetUpDownOp>) +
                  append<&ast::SetStatement::values>(ref<IdentifierOrRegister> | ref<Literal>)))> {};

  // sort

  struct SortStatement
      : rule<create<ast::SortStatement> + ikw<"SORT">+
             assign<&ast::SortStatement::fileName>(ref<CobolWord>) +
             some(option(ikw<"ON">) + (ikw<"ASCENDING"> | ikw<"DESCENDING">) + option(ikw<"KEY">) +
                  option(ikw<"IS">) + some(append<&ast::SortStatement::keys>(ref<CobolWord>))) +
             option((option(ikw<"WITH">) + ikw<"DUPLICATES">+ option(ikw<"IN">) +
                         option(ikw<"ORDER">))) +
             option((option(ikw<"COLLATING">) + ikw<"SEQUENCE">+ option(ikw<"IS">)) +
                    some(ref<CobolWord>) +
                    option((option(ikw<"FOR">) + ikw<"ALPHANUMERIC">+ ikw<"IS">) +
                           ref<CobolWord>) +
                    option((option(ikw<"FOR">) + ikw<"NATIONAL">+ option(ikw<"IS">)) +
                           ref<CobolWord>)) +
             option(ikw<"INPUT">+ ikw<"PROCEDURE">+ option(ikw<"IS">) +
                    assign<&ast::SortStatement::inputProcedure>(ref<CobolWord>) +
                    option((ikw<"THROUGH"> | ikw<"THRU">) + ref<CobolWord>)) +
             many(ikw<"USING">+ some(append<&ast::SortStatement::using_>(ref<CobolWord>))) +
             option(ikw<"OUTPUT">+ ikw<"PROCEDURE">+ option(ikw<"IS">) +
                    assign<&ast::SortStatement::outputProcedure>(ref<CobolWord>) +
                    option((ikw<"THROUGH"> | ikw<"THRU">) + ref<CobolWord>)) +
             many(ikw<"GIVING">+
                  some(append<&ast::SortStatement::giving>(ref<CobolWord>) +
                       option(ikw<"LOCK"> | ikw<"SAVE"> | (ikw<"NO">+ ikw<"REWIND">) |
                              ikw<"CRUNCH"> | ikw<"RELEASE"> |
                              (ikw<"WITH">+ ikw<"REMOVE">+ ikw<"CRUNCH">))))> {};

  // start

  struct StartKeyOp
      : rule<as<rawSpan>(
            (ikw<"GREATER"> + option(ikw<"THAN">) + ikw<"OR"> + ikw<"EQUAL"> +
             option(ikw<"TO">)) |
            lit<">="> |
            (ikw<"NOT"> + ((ikw<"LESS"> + option(ikw<"THAN">)) | lit<"<">)) |
            (ikw<"GREATER"> + option(ikw<"THAN">)) | lit<">"> |
            (ikw<"EQUAL"> + option(ikw<"TO">)) | lit<"=">)> {};

  struct StartStatement
      : rule<create<ast::StartStatement> + ikw<"START">+
             assign<&ast::StartStatement::fileName>(ref<CobolWord>) +
             option(ikw<"KEY">+ option(ikw<"IS">) +
                    assign<&ast::StartStatement::op>(ref<StartKeyOp>) +
                    assign<&ast::StartStatement::key>(ref<CobolWord>)) +
             option(ikw<"INVALID">+ option(ikw<"KEY">) +
                    many(append<&ast::StartStatement::invalidKey>(ref<Statement>))) +
             option(ikw<"NOT">+ ikw<"INVALID">+ option(ikw<"KEY">) +
                    many(append<&ast::StartStatement::notInvalidKey>(ref<Statement>))) +
             option(ikw<"END-START">)> {};

  // stop

  struct StopStatement
      : rule<create<ast::StopStatement> + ikw<"STOP">+
             (flag<&ast::StopStatement::run>(ikw<"RUN">) |
              assign<&ast::StopStatement::value>(ref<Literal>))> {};

  // string

  struct StringSendItem
      : rule<create<ast::StringSendItem> +
             some(append<&ast::StringSendItem::operands>(ref<IdentifierOrRegister> |
                                                          ref<Literal>)) +
             ((ikw<"DELIMITED">+ option(ikw<"BY">) +
               assign<&ast::StringSendItem::delimitedBy>(ikw<"SIZE"> | ref<CobolWord> |
                                                          ref<STRINGLIT>)) |
              (ikw<"FOR">+
               assign<&ast::StringSendItem::forPhrase>(ref<IdentifierOrRegister> | ref<Literal>)))> {};
  struct StringStatement
      : rule<create<ast::StringStatement> + ikw<"STRING">+
             some(append<&ast::StringStatement::sending>(ref<StringSendItem>)) + ikw<"INTO">+
             assign<&ast::StringStatement::into>(ref<Identifier>) +
             option(option(ikw<"WITH">) + ikw<"POINTER">+
                    assign<&ast::StringStatement::pointer>(ref<Identifier>)) +
             option(option(ikw<"ON">) + ikw<"OVERFLOW">+
                    many(append<&ast::StringStatement::onOverflow>(ref<Statement>))) +
             option(ikw<"NOT">+ option(ikw<"ON">) + ikw<"OVERFLOW">+
                    many(append<&ast::StringStatement::notOnOverflow>(ref<Statement>))) +
             option(ikw<"END-STRING">)> {};

  // subtract already defined above with add; terminate

  struct TerminateStatement
      : rule<create<ast::TerminateStatement> + ikw<"TERMINATE">+
             assign<&ast::TerminateStatement::reportName>(ref<Identifier>)> {};

  // unstring

  struct UnstringIntoOne
      : rule<create<ast::UnstringInto> + assign<&ast::UnstringInto::target>(ref<Identifier>) +
             option(ikw<"DELIMITER">+ option(ikw<"IN">) +
                    assign<&ast::UnstringInto::delimiterIn>(ref<Identifier>)) +
             option(ikw<"COUNT">+ option(ikw<"IN">) +
                    assign<&ast::UnstringInto::countIn>(ref<Identifier>))> {};
  struct UnstringStatement
      : rule<create<ast::UnstringStatement> + ikw<"UNSTRING">+
             assign<&ast::UnstringStatement::sending>(ref<Identifier>) +
             option(ikw<"DELIMITED">+ option(ikw<"BY">) + option(ikw<"ALL">) +
                    append<&ast::UnstringStatement::delimiters>(ref<IdentifierOrRegister> |
                                                                ref<Literal>) +
                    many(ikw<"OR">+ option(ikw<"ALL">) +
                         append<&ast::UnstringStatement::delimiters>(ref<IdentifierOrRegister> |
                                                                     ref<Literal>))) +
             ikw<"INTO">+ some(append<&ast::UnstringStatement::into>(ref<UnstringIntoOne>)) +
             option(option(ikw<"WITH">) + ikw<"POINTER">+
                    assign<&ast::UnstringStatement::pointer>(ref<Identifier>)) +
             option(ikw<"TALLYING">+ option(ikw<"IN">) +
                    assign<&ast::UnstringStatement::tallying>(ref<Identifier>)) +
             option(option(ikw<"ON">) + ikw<"OVERFLOW">+
                    many(append<&ast::UnstringStatement::onOverflow>(ref<Statement>))) +
             option(ikw<"NOT">+ option(ikw<"ON">) + ikw<"OVERFLOW">+
                    many(append<&ast::UnstringStatement::notOnOverflow>(ref<Statement>))) +
             option(ikw<"END-UNSTRING">)> {};

  // use (declaratives only, not a general statement)

  struct UseWord
      : rule<as<rawSpan>(
            (option(ikw<"GLOBAL">) + ikw<"AFTER"> + option(ikw<"STANDARD">) +
             (ikw<"EXCEPTION"> | ikw<"ERROR">)+ikw<"PROCEDURE"> +
             option(ikw<"ON">) +
             (ikw<"INPUT"> | ikw<"OUTPUT"> | ikw<"I-O"> | ikw<"EXTEND"> |
              some(ref<CobolWord>))) |
            (option(ikw<"FOR">) + ikw<"DEBUGGING"> + option(ikw<"ON">) +
             some((ikw<"ALL"> + ikw<"PROCEDURES">) |
                  (ikw<"ALL"> + option(ikw<"REFERENCES">) + option(ikw<"OF">) +
                   ref<CobolWord>) |
                  ref<CobolWord>)))> {};

  struct UseStatement
      : rule<create<ast::UseStatement> + ikw<"USE">+
             assign<&ast::UseStatement::text>(ref<UseWord>)> {};

  // write

  struct AdvancingWord
      : rule<as<rawSpan>(
            ikw<"PAGE"> |
            (ref<CobolWord> + option(ikw<"LINE"> | ikw<"LINES">)) |
            ((ref<NUMLIT> | ref<INTLIT>)+option(ikw<"LINE"> | ikw<"LINES">)))> {
  };

  struct WriteStatement
      : rule<create<ast::WriteStatement> + ikw<"WRITE">+
             assign<&ast::WriteStatement::recordName>(ref<CobolWord>) +
             option(ikw<"FROM">+
                    assign<&ast::WriteStatement::from>(ref<IdentifierOrRegister> | ref<Literal>)) +
             option((ikw<"BEFORE"> | ikw<"AFTER">) + option(ikw<"ADVANCING">) +
                    assign<&ast::WriteStatement::advancing>(ref<AdvancingWord>)) +
             option(ikw<"AT">) +
             option((ikw<"END-OF-PAGE"> | ikw<"EOP">) +
                    many(append<&ast::WriteStatement::atEndOfPage>(ref<Statement>))) +
             option(ikw<"NOT">+ option(ikw<"AT">) + (ikw<"END-OF-PAGE"> | ikw<"EOP">) +
                    many(append<&ast::WriteStatement::notAtEndOfPage>(ref<Statement>))) +
             option(ikw<"INVALID">+ option(ikw<"KEY">) +
                    many(append<&ast::WriteStatement::invalidKey>(ref<Statement>))) +
             option(ikw<"NOT">+ ikw<"INVALID">+ option(ikw<"KEY">) +
                    many(append<&ast::WriteStatement::notInvalidKey>(ref<Statement>))) +
             option(ikw<"END-WRITE">)> {};

  // paragraphs, sections, declaratives

  struct Paragraph
      : rule<create<ast::Paragraph> + assign<&ast::Paragraph::name>(ref<CobolWord> | ref<INTLIT>) +
             ref<DOT_FS> +
             many(append<&ast::Paragraph::statements>(ref<Statement>) | (ref<DOT_FS>))> {};

  struct ProcedureSection
      : rule<create<ast::ProcedureSection> +
             assign<&ast::ProcedureSection::name>(ref<CobolWord> | ref<INTLIT>) + ikw<"SECTION">+
             option(ref<INTLIT>) + ref<DOT_FS> +
             many(append<&ast::ProcedureSection::paragraphs>(ref<Paragraph>))> {};

  struct Declarative
      : rule<create<ast::Declarative> +
             assign<&ast::Declarative::sectionName>(ref<CobolWord> | ref<INTLIT>) + ikw<"SECTION">+
             option(ref<INTLIT>) + ref<DOT_FS> +
             assign<&ast::Declarative::use>(ref<UseStatement>) + ref<DOT_FS> +
             many(append<&ast::Declarative::paragraphs>(ref<Paragraph>))> {};

  struct ProcedureDivision
      : rule<create<ast::ProcedureDivision> + ikw<"PROCEDURE">+ ikw<"DIVISION">+
             option((ikw<"USING"> | ikw<"CHAINING">) +
                    some((option(option(ikw<"BY">) + ikw<"REFERENCE">) +
                          (ikw<"ANY"> |
                           (option(ikw<"OPTIONAL">) +
                            append<&ast::ProcedureDivision::using_>(ref<Identifier>)))) |
                         (ikw<"BY">+ ikw<"VALUE">+
                          (ikw<"ANY"> |
                           append<&ast::ProcedureDivision::using_>(ref<Identifier>) |
                           LiteralText)))) +
             option((ikw<"GIVING"> | ikw<"RETURNING">) +
                    assign<&ast::ProcedureDivision::giving>(ref<CobolWord>)) +
             ref<DOT_FS> +
             option(ikw<"DECLARATIVES">+ ref<DOT_FS> +
                    some(append<&ast::ProcedureDivision::declaratives>(ref<Declarative>)) +
                    ikw<"END">+ ikw<"DECLARATIVES">+ ref<DOT_FS>) +
             // paragraphs: sentence* paragraph* -- a leading run of sentences
             // with no paragraph name is a program's normal, common shape
             // ("PROCEDURE DIVISION. DISPLAY ... STOP RUN." with nothing
             // named at all).
             many(append<&ast::ProcedureDivision::statements>(ref<Statement>) |
                  (ref<DOT_FS>)) +
             many(append<&ast::ProcedureDivision::paragraphs>(ref<Paragraph>)) +
             many(append<&ast::ProcedureDivision::sections>(ref<ProcedureSection>))> {};

  // ---- identification division, data division assembly, program ----

  struct IdBodySkip
      : rule<(((ikw<"AUTHOR"> | ikw<"INSTALLATION"> | ikw<"DATE-WRITTEN"> |
                  ikw<"DATE-COMPILED"> | ikw<"SECURITY"> | ikw<"REMARKS">) +
                 ref<DOT_FS> + many(ref<COMMENTENTRYLINE>)))> {};

  // Each section is inlined into the one DataDivision that creates
  // ast::DataDivision: a per-section rule whose only job is `append<>` into
  // that same class would deduce ast::DataDivision as its own head too,
  // clashing with the outer create<>.
  struct DataDivision
      : rule<create<ast::DataDivision> + ikw<"DATA">+ ikw<"DIVISION">+ ref<DOT_FS> +
             many((ikw<"FILE">+ ikw<"SECTION">+ ref<DOT_FS> +
                   many(append<&ast::DataDivision::files>(ref<FileDescriptionEntry>))) |
                  (ikw<"DATA-BASE">+ ikw<"SECTION">+ ref<DOT_FS> +
                   many(append<&ast::DataDivision::databases>(ref<DataBaseSectionEntry>) +
                        ref<DOT_FS>)) |
                  (ikw<"WORKING-STORAGE">+ ikw<"SECTION">+ ref<DOT_FS> +
                   DataEntriesOf<&ast::DataDivision::workingStorage>) |
                  (ikw<"LINKAGE">+ ikw<"SECTION">+ ref<DOT_FS> +
                   DataEntriesOf<&ast::DataDivision::linkage>) |
                  (ikw<"COMMUNICATION">+ ikw<"SECTION">+ ref<DOT_FS> +
                   many(append<&ast::DataDivision::communications>(
                       ref<CommunicationDescriptionEntry>))) |
                  (ikw<"LOCAL-STORAGE">+ ikw<"SECTION">+ ref<DOT_FS> +
                   option(ikw<"LD">+ ref<CobolWord> + ref<DOT_FS>) +
                   DataEntriesOf<&ast::DataDivision::localStorage>) |
                  (ikw<"SCREEN">+ ikw<"SECTION">+ ref<DOT_FS> +
                   many(append<&ast::DataDivision::screens>(ref<ScreenDescriptionEntry>))) |
                  (ikw<"REPORT">+ ikw<"SECTION">+ ref<DOT_FS> +
                   many(append<&ast::DataDivision::reports>(ref<ReportDescriptionEntry>))) |
                  (ikw<"PROGRAM-LIBRARY">+ ikw<"SECTION">+ ref<DOT_FS> +
                   many(append<&ast::DataDivision::libraries>(ref<LibraryDescriptionEntry>))))> {};

  struct ProgramUnit
      : rule<create<ast::ProgramUnit> +
             (ikw<"IDENTIFICATION"> | ikw<"ID">)+ikw<"DIVISION"> + ref<DOT_FS> +
             ikw<"PROGRAM-ID"> + ref<DOT_FS> +
             assign<&ast::ProgramUnit::name>(ref<STRINGLIT> | ref<CobolWord>) +
             option(option(ikw<"IS">) + (ikw<"COMMON"> | ikw<"INITIAL"> |
                                         ikw<"LIBRARY"> | ikw<"DEFINITION"> |
                                         ikw<"RECURSIVE">)+option(ikw<"PROGRAM">)) +
             option(ref<DOT_FS>) + many(ref<COMMENTENTRYLINE>) + many(ref<IdBodySkip>) +
             option(
                 assign<&ast::ProgramUnit::environment>(ref<EnvironmentDivision>)) +
             option(assign<&ast::ProgramUnit::data>(ref<DataDivision>)) +
             option(assign<&ast::ProgramUnit::procedure>(ref<ProcedureDivision>)) +
             many(append<&ast::ProgramUnit::nested>(ref<ProgramUnit>)) +
             option(ikw<"END"> + flag<&ast::ProgramUnit::hasEndProgram>(ikw<"PROGRAM">) +
                    assign<&ast::ProgramUnit::endName>(ref<STRINGLIT> | ref<CobolWord>) +
                    ref<DOT_FS>)> {};

  struct CompilationUnit
      : rule<create<ast::CompilationUnit> +
             some(append<&ast::CompilationUnit::programs>(ref<ProgramUnit>))> {};

  using entry = CompilationUnit;
  using ignored = list<WS, NEWLINE>;
  using hidden = list<LINE_COMMENT>;
};

}  // namespace cobol85
