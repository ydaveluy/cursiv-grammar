// The RuleWorks language: its grammar. RuleWorks is case-insensitive and
// almost every keyword doubles as a plain name; `word_boundary` is the
// symbol-character class itself, not `w`, since symbols routinely contain
// `-`, `$`, `.` and other punctuation.
#pragma once

#include <cstdint>
#include <charconv>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "ruleworks/core/ast.hpp"

namespace ruleworks {

inline constexpr auto strip_ends = [](std::string_view t, std::size_t head,
                                      std::size_t tail) {
  return t.substr(head, t.size() - head - tail);
};
inline constexpr auto strip_angle = [](std::string_view t) {
  return strip_ends(t, 1, 1);
};
inline constexpr auto strip_quoted_angle = [](std::string_view t) {
  return strip_ends(t, 2, 2);
};
inline constexpr auto strip_pipes = [](std::string_view t,
                                       std::pmr::memory_resource* mem) {
  const auto body = strip_ends(t, 1, 1);
  if (body.find('|') == body.npos) return body;
  auto room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    room[n++] = body[i];
    if (body[i] == '|' && i + 1 < body.size() && body[i + 1] == '|') ++i;
  }
  return std::string_view(room.data(), n);
};
inline constexpr auto integer_value = [](std::string_view t)
    -> cursiv::ConversionResult<std::int64_t> {
  if (t.ends_with('.')) t.remove_suffix(1);
  if (t.starts_with('+')) t.remove_prefix(1);
  std::int64_t value = 0;
  const auto [end, error] = std::from_chars(t.data(), t.data() + t.size(), value);
  if (error != std::errc{} || end != t.data() + t.size())
    return cursiv::ConversionResult<std::int64_t>::err("integer literal out of range");
  return cursiv::ConversionResult<std::int64_t>::ok(value);
};

struct Ruleworks : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\n\r\f)">)> {};
  // A trailing comment with no final newline (end of file) still ends.
  struct TrailingComment
      : terminal<lit<";"> + many(cr<R"(^\r\n)">) + option(eol)> {};

  // Symbol characters, minus the angle brackets that spell a variable: a
  // bare symbol never collides with `<name>` this way, so no lexer tie
  // needs breaking between the two.
  static constexpr auto SymbolChar =
      cr<"A-Za-z0-9_$:'.,`!@*+=/?-">;
  static constexpr auto Nonmagic = cr<"A-Za-z0-9_$:'.,<>`!@*+=/?-">;
  static constexpr auto word_boundary = SymbolChar;

  static constexpr auto Digit = cr<"0-9">;
  static constexpr auto NumSign = cr<"+-">;
  static constexpr auto Exponent = cr<"Ee"> + option(NumSign) + some(Digit);
  static constexpr auto HexDigit = cr<"0-9A-Fa-f">;

  struct TOK_FLOAT_CONST
      : terminal<as<double>(
            (option(NumSign) + many(Digit) + lit<"."> + some(Digit) +
             option(Exponent)) |
            (option(NumSign) + some(Digit) + lit<"."> + Exponent))> {};
  struct TOK_INTEGER_CONST
      : terminal<as<integer_value>(option(NumSign) + some(Digit) +
                                  option(lit<".">))> {};
  struct TOK_INSTANCE_CONST : terminal<lit<"#"> + some(Digit)> {};
  struct TOK_OPAQUE_CONST
      : terminal<(lit<"%X"> | lit<"%x">) + some(HexDigit)> {};

  struct TOK_VARIABLE
      : terminal<as<strip_angle>(lit<"<"> + some(Nonmagic) + lit<">">)> {};
  struct TOK_QUOTED_VAR : terminal<as<strip_quoted_angle>(
                              lit<"'<"> + some(Nonmagic) + lit<">'">)> {};
  struct TOK_SYMBOL_CONST : terminal<some(SymbolChar)> {};
  struct TOK_QUOTED_SYMBOL
      : terminal<as<strip_pipes>(lit<"|"> + many(lit<"||"> | cr<R"(^|\n)">) + lit<"|">)> {};
  // An unmatched '|' is refused: no parser rule names a bare BAR token.

  struct TOK_LBRACE : terminal<lit<"{">> {};
  struct TOK_RBRACE : terminal<lit<"}">> {};
  struct TOK_LBRACKET : terminal<lit<"[">> {};
  struct TOK_RBRACKET : terminal<lit<"]">> {};
  struct TOK_LPAREN : terminal<lit<"(">> {};
  struct TOK_RPAREN : terminal<lit<")">> {};
  struct TOK_HAT : terminal<lit<"^">> {};
  struct TOK_START_DISJUNCTION : terminal<lit<"<<">> {};
  struct TOK_END_DISJUNCTION : terminal<lit<">>">> {};
  // These share bytes with a bare symbol at equal length (`SymbolChar`
  // includes them); `kw<>` makes the operator win the tie.
  struct TOK_TIMES : terminal<kw<"*">> {};
  struct TOK_DIVIDE : terminal<kw<"/">> {};
  struct TOK_MODULUS : terminal<lit<"\\">> {};
  struct TOK_PLUS : terminal<kw<"+">> {};
  struct TOK_MINUS : terminal<kw<"-">> {};
  struct TOK_EQUAL : terminal<kw<"=">> {};
  struct TOK_EQUAL_EQUAL : terminal<kw<"==">> {};
  struct TOK_APPROX_EQUAL : terminal<lit<"~=">> {};
  struct TOK_NOT_APPROX_EQUAL : terminal<lit<"-~=">> {};
  struct TOK_SAME_TYPE : terminal<lit<"<=>">> {};
  struct TOK_DIFF_TYPE : terminal<lit<"<->">> {};
  struct TOK_NOT_EQ : terminal<lit<"<>">> {};
  struct TOK_NOT_EQUAL : terminal<kw<"-=">> {};
  struct TOK_LESS : terminal<lit<"<">> {};
  struct TOK_LESS_EQUAL : terminal<lit<"<=">> {};
  struct TOK_GREATER : terminal<lit<">">> {};
  struct TOK_GREATER_EQUAL : terminal<lit<">=">> {};
  struct TOK_CONTAINS : terminal<lit<"[+]">> {};
  struct TOK_DOES_NOT_CONTAIN : terminal<lit<"[-]">> {};
  struct TOK_LENGTH_LESS_EQUAL : terminal<lit<"[<=]">> {};
  struct TOK_LENGTH_NOT_EQUAL : terminal<lit<"[<>]">> {};
  struct TOK_LENGTH_LESS : terminal<lit<"[<]">> {};
  struct TOK_LENGTH_EQUAL : terminal<lit<"[=]">> {};
  struct TOK_LENGTH_GREATER_EQUAL : terminal<lit<"[>=]">> {};
  struct TOK_LENGTH_GREATER : terminal<lit<"[>]">> {};
  struct TOK_ARROW : terminal<kw<"-->">> {};
  struct TOK_AT : terminal<kw<"@">> {};
  struct TOK_DOLLAR_LAST : terminal<ikw<"$LAST">> {};
  struct TOK_FAILURE : terminal<ikw<"$FAILURE">> {};
  struct TOK_SUCCESS : terminal<ikw<"$SUCCESS">> {};

  struct TOK_ACCEPT : terminal<ikw<"ACCEPT">> {};
  struct TOK_ACCEPT_ATOM : terminal<ikw<"ACCEPT-ATOM">> {};
  struct TOK_ACCEPTLINE_COMPOUND : terminal<ikw<"ACCEPTLINE-COMPOUND">> {};
  struct TOK_ACCEPTS : terminal<ikw<"ACCEPTS">> {};
  struct TOK_ACTIVATES : terminal<ikw<"ACTIVATES">> {};
  struct TOK_ADDSTATE : terminal<ikw<"ADDSTATE">> {};
  struct TOK_AFTER : terminal<ikw<"AFTER">> {};
  struct TOK_ALIAS_FOR : terminal<ikw<"ALIAS-FOR">> {};
  struct TOK_ANY : terminal<ikw<"ANY">> {};
  struct TOK_AND : terminal<ikw<"AND">> {};
  struct TOK_APPEND : terminal<ikw<"APPEND">> {};
  struct TOK_ASCID : terminal<ikw<"ASCID">> {};
  struct TOK_ASCIZ : terminal<ikw<"ASCIZ">> {};
  struct TOK_ATOM : terminal<ikw<"ATOM">> {};
  struct TOK_BIND : terminal<ikw<"BIND">> {};
  struct TOK_BY : terminal<ikw<"BY">> {};
  struct TOK_BYTE : terminal<ikw<"BYTE">> {};
  struct TOK_CALL_INHERITED : terminal<ikw<"CALL-INHERITED">> {};
  struct TOK_CATCH : terminal<ikw<"CATCH">> {};
  struct TOK_CLOSEFILE : terminal<ikw<"CLOSEFILE">> {};
  struct TOK_COMPOUND : terminal<ikw<"COMPOUND">> {};
  struct TOK_CONCAT : terminal<ikw<"CONCAT">> {};
  struct TOK_COPY : terminal<ikw<"COPY">> {};
  struct TOK_CRLF : terminal<ikw<"CRLF">> {};
  struct TOK_CS : terminal<ikw<"CS">> {};
  struct TOK_DEBUG : terminal<ikw<"DEBUG">> {};
  struct TOK_DECLARATION_BLOCK : terminal<ikw<"DECLARATION-BLOCK">> {};
  struct TOK_DEFAULT : terminal<ikw<"DEFAULT">> {};
  struct TOK_DOUBLE_FLOAT : terminal<ikw<"DOUBLE-FLOAT">> {};
  struct TOK_DO : terminal<ikw<"DO">> {};
  struct TOK_EB : terminal<ikw<"EB">> {};
  struct TOK_ELSE : terminal<ikw<"ELSE">> {};
  struct TOK_END_BLOCK : terminal<ikw<"END-BLOCK">> {};
  struct TOK_END_GROUP : terminal<ikw<"END-GROUP">> {};
  struct TOK_ENTRY_BLOCK : terminal<ikw<"ENTRY-BLOCK">> {};
  struct TOK_EVERY : terminal<ikw<"EVERY">> {};
  struct TOK_EXTERNAL_ROUTINE : terminal<ikw<"EXTERNAL-ROUTINE">> {};
  struct TOK_FILENAME : terminal<ikw<"FILENAME">> {};
  struct TOK_FILL : terminal<ikw<"FILL">> {};
  struct TOK_FLOAT : terminal<ikw<"FLOAT">> {};
  struct TOK_FOR_EACH : terminal<ikw<"FOR-EACH">> {};
  struct TOK_GENATOM : terminal<ikw<"GENATOM">> {};
  struct TOK_GENERIC_METHOD : terminal<ikw<"GENERIC-METHOD">> {};
  struct TOK_GENINT : terminal<ikw<"GENINT">> {};
  struct TOK_GET : terminal<ikw<"GET">> {};
  struct TOK_IF : terminal<ikw<"IF">> {};
  struct TOK_IN : terminal<ikw<"IN">> {};
  struct TOK_INHERITS_FROM : terminal<ikw<"INHERITS-FROM">> {};
  struct TOK_INSTANCE : terminal<ikw<"INSTANCE">> {};
  struct TOK_INTEGER : terminal<ikw<"INTEGER">> {};
  struct TOK_IS_OPEN : terminal<ikw<"IS-OPEN">> {};
  struct TOK_LENGTH : terminal<ikw<"LENGTH">> {};
  struct TOK_LEX : terminal<ikw<"LEX">> {};
  struct TOK_LONG : terminal<ikw<"LONG">> {};
  struct TOK_MAKE : terminal<ikw<"MAKE">> {};
  struct TOK_MAX : terminal<ikw<"MAX">> {};
  struct TOK_MEA : terminal<ikw<"MEA">> {};
  struct TOK_METHOD : terminal<ikw<"METHOD">> {};
  struct TOK_MIN : terminal<ikw<"MIN">> {};
  struct TOK_MODIFY : terminal<ikw<"MODIFY">> {};
  struct TOK_NOT : terminal<ikw<"NOT">> {};
  struct TOK_NTH : terminal<ikw<"NTH">> {};
  struct TOK_OBJECT_CLASS : terminal<ikw<"OBJECT-CLASS">> {};
  struct TOK_OF : terminal<ikw<"OF">> {};
  struct TOK_OFF : terminal<ikw<"OFF">> {};
  struct TOK_ON : terminal<ikw<"ON">> {};
  struct TOK_ON_EMPTY : terminal<ikw<"ON-EMPTY">> {};
  struct TOK_ON_ENTRY : terminal<ikw<"ON-ENTRY">> {};
  struct TOK_ON_EVERY : terminal<ikw<"ON-EVERY">> {};
  struct TOK_ON_EXIT : terminal<ikw<"ON-EXIT">> {};
  struct TOK_OPAQUE : terminal<ikw<"OPAQUE">> {};
  struct TOK_OPENFILE : terminal<ikw<"OPENFILE">> {};
  struct TOK_OR : terminal<ikw<"OR">> {};
  struct TOK_OUT : terminal<ikw<"OUT">> {};
  struct TOK_P : terminal<ikw<"P">> {};
  struct TOK_PATHNAME : terminal<ikw<"PATHNAME">> {};
  struct TOK_POINTER : terminal<ikw<"POINTER">> {};
  struct TOK_POSITION : terminal<ikw<"POSITION">> {};
  struct TOK_QUIT : terminal<ikw<"QUIT">> {};
  struct TOK_READ_ONLY : terminal<ikw<"READ-ONLY">> {};
  struct TOK_READ_WRITE : terminal<ikw<"READ-WRITE">> {};
  struct TOK_REFERENCE : terminal<ikw<"REFERENCE">> {};
  struct TOK_REMOVE_EVERY : terminal<ikw<"REMOVE-EVERY">> {};
  struct TOK_REMOVE : terminal<ikw<"REMOVE">> {};
  struct TOK_RESTORESTATE : terminal<ikw<"RESTORESTATE">> {};
  struct TOK_RETURN : terminal<ikw<"RETURN">> {};
  struct TOK_RETURNS : terminal<ikw<"RETURNS">> {};
  struct TOK_RG : terminal<ikw<"RG">> {};
  struct TOK_RJUST : terminal<ikw<"RJUST">> {};
  struct TOK_RULE : terminal<ikw<"RULE">> {};
  struct TOK_RULE_BLOCK : terminal<ikw<"RULE-BLOCK">> {};
  struct TOK_RULE_GROUP : terminal<ikw<"RULE-GROUP">> {};
  struct TOK_SAVESTATE : terminal<ikw<"SAVESTATE">> {};
  struct TOK_SHORT : terminal<ikw<"SHORT">> {};
  struct TOK_SINGLE_FLOAT : terminal<ikw<"SINGLE-FLOAT">> {};
  struct TOK_SCALAR : terminal<ikw<"SCALAR">> {};
  struct TOK_SPECIALIZE : terminal<ikw<"SPECIALIZE">> {};
  struct TOK_SQL_ATTACH : terminal<ikw<"SQL-ATTACH">> {};
  struct TOK_SQL_COMMIT : terminal<ikw<"SQL-COMMIT">> {};
  struct TOK_SQL_DELETE : terminal<ikw<"SQL-DELETE">> {};
  struct TOK_SQL_DETACH : terminal<ikw<"SQL-DETACH">> {};
  struct TOK_SQL_FETCH_AS_OBJECT
      : terminal<ikw<"SQL-FETCH-AS-OBJECT">> {};
  struct TOK_SQL_FETCH_EACH : terminal<ikw<"SQL-FETCH-EACH">> {};
  struct TOK_SQL_INSERT : terminal<ikw<"SQL-INSERT">> {};
  struct TOK_SQL_INSERT_FROM_OBJECT
      : terminal<ikw<"SQL-INSERT-FROM-OBJECT">> {};
  struct TOK_SQL_ROLLBACK : terminal<ikw<"SQL-ROLLBACK">> {};
  struct TOK_SQL_START : terminal<ikw<"SQL-START">> {};
  struct TOK_SQL_UPDATE : terminal<ikw<"SQL-UPDATE">> {};
  struct TOK_SQL_UPDATE_FROM_OBJECT
      : terminal<ikw<"SQL-UPDATE-FROM-OBJECT">> {};
  struct TOK_STARTUP : terminal<ikw<"STARTUP">> {};
  struct TOK_STRATEGY : terminal<ikw<"STRATEGY">> {};
  struct TOK_SUBCOMPOUND : terminal<ikw<"SUBCOMPOUND">> {};
  struct TOK_SUBSYMBOL : terminal<ikw<"SUBSYMBOL">> {};
  struct TOK_SYMBOL : terminal<ikw<"SYMBOL">> {};
  struct TOK_SYMBOL_LENGTH : terminal<ikw<"SYMBOL-LENGTH">> {};
  struct TOK_TABTO : terminal<ikw<"TABTO">> {};
  struct TOK_THEN : terminal<ikw<"THEN">> {};
  struct TOK_TRACE : terminal<ikw<"TRACE">> {};
  struct TOK_UNSIGNED_BYTE : terminal<ikw<"UNSIGNED-BYTE">> {};
  struct TOK_UNSIGNED_LONG : terminal<ikw<"UNSIGNED-LONG">> {};
  struct TOK_UNSIGNED_SHORT : terminal<ikw<"UNSIGNED-SHORT">> {};
  struct TOK_USES : terminal<ikw<"USES">> {};
  struct TOK_VALUE : terminal<ikw<"VALUE">> {};
  struct TOK_WHEN : terminal<ikw<"WHEN">> {};
  struct TOK_WHILE : terminal<ikw<"WHILE">> {};
  struct TOK_WM : terminal<ikw<"WM">> {};
  struct TOK_WRITE : terminal<ikw<"WRITE">> {};

  // Every one of these words also parses as a plain `name` -- spelled
  // once, referenced from both the specific construct and the `Name`
  // union below.
  static constexpr auto AllowableFuncKeyword =
      ref<TOK_ACCEPT> | ref<TOK_ACTIVATES> | ref<TOK_ANY> | ref<TOK_APPEND> |
      ref<TOK_ASCID> | ref<TOK_ASCIZ> | ref<TOK_ATOM> | ref<TOK_BY> |
      ref<TOK_BYTE> | ref<TOK_CATCH> | ref<TOK_CS> | ref<TOK_DO> |
      ref<TOK_DOLLAR_LAST> | ref<TOK_DOUBLE_FLOAT> | ref<TOK_EB> |
      ref<TOK_ELSE> | ref<TOK_FAILURE> | ref<TOK_FILL> | ref<TOK_IN> |
      ref<TOK_INHERITS_FROM> | ref<TOK_INSTANCE> | ref<TOK_LEX> |
      ref<TOK_LONG> | ref<TOK_MEA> | ref<TOK_OF> | ref<TOK_OUT> |
      ref<TOK_OPAQUE> | ref<TOK_POINTER> | ref<TOK_READ_ONLY> |
      ref<TOK_READ_WRITE> | ref<TOK_REFERENCE> | ref<TOK_RG> |
      ref<TOK_SHORT> | ref<TOK_SINGLE_FLOAT> | ref<TOK_SCALAR> |
      ref<TOK_STRATEGY> | ref<TOK_SUCCESS> | ref<TOK_THEN> |
      ref<TOK_UNSIGNED_BYTE> | ref<TOK_UNSIGNED_LONG> |
      ref<TOK_UNSIGNED_SHORT> | ref<TOK_USES> | ref<TOK_VALUE> |
      ref<TOK_WM>;

  static constexpr auto BuiltInKeyword =
      ref<TOK_ACCEPT_ATOM> | ref<TOK_ACCEPTLINE_COMPOUND> |
      ref<TOK_ACCEPTS> | ref<TOK_COMPOUND> | ref<TOK_CONCAT> |
      ref<TOK_CRLF> | ref<TOK_EVERY> | ref<TOK_FLOAT> | ref<TOK_GENATOM> |
      ref<TOK_GENINT> | ref<TOK_GET> | ref<TOK_INTEGER> |
      ref<TOK_IS_OPEN> | ref<TOK_LENGTH> | ref<TOK_MAX> | ref<TOK_MIN> |
      ref<TOK_NTH> | ref<TOK_POSITION> | ref<TOK_RETURNS> |
      ref<TOK_RJUST> | ref<TOK_SUBCOMPOUND> | ref<TOK_SUBSYMBOL> |
      ref<TOK_SYMBOL> | ref<TOK_SYMBOL_LENGTH> | ref<TOK_TABTO> |
      ref<TOK_AND> | ref<TOK_NOT> | ref<TOK_OR>;

  static constexpr auto ConstructKeyword =
      ref<TOK_DECLARATION_BLOCK> | ref<TOK_END_BLOCK> |
      ref<TOK_END_GROUP> | ref<TOK_ENTRY_BLOCK> |
      ref<TOK_EXTERNAL_ROUTINE> | ref<TOK_GENERIC_METHOD> |
      ref<TOK_METHOD> | ref<TOK_OBJECT_CLASS> | ref<TOK_ON_EMPTY> |
      ref<TOK_ON_ENTRY> | ref<TOK_STARTUP> | ref<TOK_ON_EVERY> |
      ref<TOK_ON_EXIT> | ref<TOK_P> | ref<TOK_RULE> |
      ref<TOK_RULE_BLOCK> | ref<TOK_RULE_GROUP>;

  static constexpr auto ActionToken =
      ref<TOK_ADDSTATE> | ref<TOK_AFTER> | ref<TOK_BIND> |
      ref<TOK_CALL_INHERITED> | ref<TOK_CLOSEFILE> | ref<TOK_COPY> |
      ref<TOK_DEBUG> | ref<TOK_DEFAULT> | ref<TOK_FOR_EACH> | ref<TOK_IF> |
      ref<TOK_MAKE> | ref<TOK_MODIFY> | ref<TOK_OPENFILE> |
      ref<TOK_QUIT> | ref<TOK_REMOVE> | ref<TOK_REMOVE_EVERY> |
      ref<TOK_RESTORESTATE> | ref<TOK_RETURN> | ref<TOK_SAVESTATE> |
      ref<TOK_SPECIALIZE> | ref<TOK_TRACE> | ref<TOK_WHILE> |
      ref<TOK_WRITE> | ref<TOK_SQL_ATTACH> | ref<TOK_SQL_COMMIT> |
      ref<TOK_SQL_DELETE> | ref<TOK_SQL_DETACH> |
      ref<TOK_SQL_FETCH_EACH> | ref<TOK_SQL_FETCH_AS_OBJECT> |
      ref<TOK_SQL_INSERT> | ref<TOK_SQL_INSERT_FROM_OBJECT> |
      ref<TOK_SQL_ROLLBACK> | ref<TOK_SQL_START> | ref<TOK_SQL_UPDATE> |
      ref<TOK_SQL_UPDATE_FROM_OBJECT>;

  static constexpr auto SqlKeyword = ref<TOK_FILENAME> | ref<TOK_PATHNAME>;

  static constexpr auto AllowableFuncName = ref<TOK_SYMBOL_CONST> |
                                            ref<TOK_QUOTED_SYMBOL> |
                                            AllowableFuncKeyword;
  static constexpr auto Name = AllowableFuncName | BuiltInKeyword |
                               ConstructKeyword | ActionToken | SqlKeyword;
  static constexpr auto DbSpecName =
      AllowableFuncName | BuiltInKeyword | ConstructKeyword | ActionToken;

  static constexpr auto AttributeText = Name | ref<TOK_VARIABLE>;

  // Forward declarations: the grammar is one mutually-recursive family
  // (a value may hold an action, an action may hold values and more
  // actions), so every rule referenced before its own definition is named
  // here first.
  struct Variable;
  struct QuotedVar;
  struct Constant;
  struct AttributeLiteral;
  struct IndexNum;
  struct Rclass;
  struct Value;
  struct ValueNonSql;
  struct ArithValue;
  struct ArithExpr;
  struct ExpreValue;
  struct CmplxExpr;
  struct Expression;
  struct CaIndex;
  struct Restriction;
  struct LhsTerm;
  struct CeForm;
  struct Ce;
  struct RhsTerm;
  struct AcceptAtomBti;
  struct AcceptlineCompoundBti;
  struct CompoundBti;
  struct ConcatBti;
  struct CrlfBti;
  struct EveryBti;
  struct FloatBti;
  struct GenatomBti;
  struct GenintBti;
  struct GetBti;
  struct IntegerBti;
  struct IsOpenBti;
  struct LengthBti;
  struct MaxBti;
  struct MinBti;
  struct NthBti;
  struct PositionBti;
  struct RjustBti;
  struct SubcompoundBti;
  struct SubsymbolBti;
  struct SymbolBti;
  struct SymbolLenBti;
  struct TabtoBti;
  struct BuiltIns;
  struct FunctionCall;
  struct Action;
  struct ActionLine;
  struct AtCommand;
  struct StartupAction;
  struct StartupActionLine;
  struct AddStateAction;
  struct AfterAction;
  struct BindAction;
  struct CallInheritedAction;
  struct CloseFileAction;
  struct CopyAction;
  struct DebugAction;
  struct DefaultAction;
  struct ForEachAction;
  struct IfAction;
  struct MakeAction;
  struct ModifyAction;
  struct OpenFileAction;
  struct QuitAction;
  struct RemoveAction;
  struct RemoveEveryAction;
  struct RestoreStateAction;
  struct ReturnAction;
  struct SaveStateAction;
  struct SpecializeAction;
  struct TraceAction;
  struct WhileAction;
  struct WriteAction;
  struct SqlAttachAction;
  struct SqlCommitAction;
  struct SqlDeleteAction;
  struct SqlDetachAction;
  struct SqlFetchEachAction;
  struct SqlFetchAsObjectAction;
  struct SqlInsertAction;
  struct SqlInsertFromObjectAction;
  struct SqlRollbackAction;
  struct SqlStartAction;
  struct SqlUpdateAction;
  struct SqlUpdateFromObjectAction;

  // ---- Values ---------------------------------------------------------
  struct Variable
      : rule<create<ast::Variable>(
            assign<&ast::Variable::name>(ref<TOK_VARIABLE>))> {};
  struct QuotedVar
      : rule<create<ast::QuotedVariable>(
            assign<&ast::QuotedVariable::name>(ref<TOK_QUOTED_VAR>))> {};

  struct Constant
      : rule<create<ast::Literal>(
                 assign<&ast::Literal::value>(ref<TOK_INTEGER_CONST>)) |
             create<ast::Literal>(
                 assign<&ast::Literal::value>(ref<TOK_FLOAT_CONST>)) |
             create<ast::Literal>(
                 assign<&ast::Literal::value>(ref<TOK_OPAQUE_CONST>)) |
             create<ast::Literal>(
                 assign<&ast::Literal::value>(ref<TOK_INSTANCE_CONST>)) |
             create<ast::Literal>(assign<&ast::Literal::value>(Name))> {};

  struct AttributeLiteral
      : rule<create<ast::Literal>(
            assign<&ast::Literal::value>(AttributeText))> {};

  struct IndexNum
      : rule<create<ast::Literal>(
                 assign<&ast::Literal::value>(ref<TOK_INTEGER_CONST>)) |
             ref<Variable> |
             (ref<TOK_LPAREN> + ref<BuiltIns> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<FunctionCall> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<ArithExpr> + ref<TOK_RPAREN>) |
             create<ast::Literal>(
                 assign<&ast::Literal::value>(ref<TOK_DOLLAR_LAST>))> {};

  struct Rclass
      : rule<create<ast::Literal>(assign<&ast::Literal::value>(Name)) |
             ref<Variable> |
             (ref<TOK_LPAREN> + ref<AcceptAtomBti> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<ConcatBti> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<GetBti> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<NthBti> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<SymbolBti> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<FunctionCall> + ref<TOK_RPAREN>)> {};

  struct ValueNonSql
      : rule<ref<Constant> | ref<Variable> |
             (ref<TOK_LPAREN> + ref<BuiltIns> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<FunctionCall> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<ArithExpr> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<CopyAction> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<MakeAction> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<ModifyAction> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<SpecializeAction> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<CallInheritedAction> +
              ref<TOK_RPAREN>)> {};

  struct Value
      : rule<ref<ValueNonSql> |
             (ref<TOK_LPAREN> + ref<SqlFetchAsObjectAction> +
              ref<TOK_RPAREN>)> {};

  // ---- Built-in functions ----------------------------------------------
  struct EveryBti
      : rule<create<ast::Call>(assign<&ast::Call::name>(ref<TOK_EVERY>) +
                               append<&ast::Call::args>(ref<Rclass>))> {
  };
  struct GetBti
      : rule<create<ast::Call>(
            assign<&ast::Call::name>(ref<TOK_GET>) +
            append<&ast::Call::args>(ref<Value>) + ref<TOK_HAT> +
            append<&ast::Call::args>(ref<AttributeLiteral>))> {};
  struct NthBti
      : rule<create<ast::Call>(assign<&ast::Call::name>(ref<TOK_NTH>) +
                               append<&ast::Call::args>(ref<Value>) +
                               append<&ast::Call::args>(ref<IndexNum>))> {
  };
  struct PositionBti
      : rule<create<ast::Call>(
            assign<&ast::Call::name>(ref<TOK_POSITION>) +
            append<&ast::Call::args>(ref<Value>) +
            option(append<&ast::Call::args>(create<ast::Literal>(
                assign<&ast::Literal::value>(
                    ref<TOK_SAME_TYPE> | ref<TOK_DIFF_TYPE> | ref<TOK_EQUAL> |
                    ref<TOK_EQUAL_EQUAL> | ref<TOK_APPROX_EQUAL> |
                    ref<TOK_NOT_APPROX_EQUAL> | ref<TOK_NOT_EQ> |
                    ref<TOK_NOT_EQUAL> | ref<TOK_GREATER> |
                    ref<TOK_GREATER_EQUAL> | ref<TOK_LESS> |
                    ref<TOK_LESS_EQUAL>)))) +
            append<&ast::Call::args>(ref<Value>))> {};
  struct SubcompoundBti
      : rule<create<ast::Call>(assign<&ast::Call::name>(ref<TOK_SUBCOMPOUND>) +
                               append<&ast::Call::args>(ref<Value>) +
                               append<&ast::Call::args>(ref<IndexNum>) +
                               append<&ast::Call::args>(ref<IndexNum>))> {
  };
  struct SubsymbolBti
      : rule<create<ast::Call>(assign<&ast::Call::name>(ref<TOK_SUBSYMBOL>) +
                               append<&ast::Call::args>(ref<Value>) +
                               append<&ast::Call::args>(ref<IndexNum>) +
                               append<&ast::Call::args>(ref<IndexNum>))> {
  };
  template <auto Keyword>
  struct BuiltinCall : rule<create<ast::Call>(
                           assign<&ast::Call::name>(Keyword) +
                           many(append<&ast::Call::args>(ref<Value>)))> {};
  // These names can also occur in the narrower rclass syntax.
  struct AcceptAtomBti : BuiltinCall<ref<TOK_ACCEPT_ATOM>> {};
  struct ConcatBti : BuiltinCall<ref<TOK_CONCAT>> {};
  struct SymbolBti : BuiltinCall<ref<TOK_SYMBOL>> {};
  struct SimpleBuiltin
      : BuiltinCall<ref<TOK_ACCEPT_ATOM> | ref<TOK_ACCEPTLINE_COMPOUND> |
                    ref<TOK_COMPOUND> | ref<TOK_CONCAT> | ref<TOK_CRLF> |
                    ref<TOK_FLOAT> | ref<TOK_GENATOM> | ref<TOK_GENINT> |
                    ref<TOK_INTEGER> | ref<TOK_IS_OPEN> | ref<TOK_LENGTH> |
                    ref<TOK_MAX> | ref<TOK_MIN> | ref<TOK_RJUST> |
                    ref<TOK_SYMBOL> | ref<TOK_SYMBOL_LENGTH> | ref<TOK_TABTO>> {
  };

  struct BuiltIns
      : rule<ref<SimpleBuiltin> | ref<EveryBti> | ref<GetBti> | ref<NthBti> |
             ref<PositionBti> | ref<SubcompoundBti> | ref<SubsymbolBti>> {};

  struct FunctionCall
      : rule<create<ast::Call>(
            assign<&ast::Call::name>(AllowableFuncName) +
            many(append<&ast::Call::args>(ref<Value>)))> {};

  // ---- Arithmetic and boolean expressions ------------------------------

  struct ArithUnaryMinus
      : rule<create<ast::ArithExpr>(
                 assign<&ast::ArithExpr::op>(as<ast::ArithOp::sub>(ref<TOK_MINUS>)) +
                 assign<&ast::ArithExpr::right>(ref<ArithValue>))> {};

  struct ArithValue
      : rule<ref<ArithUnaryMinus> |
             create<ast::Literal>(
                 assign<&ast::Literal::value>(ref<TOK_INTEGER_CONST>)) |
             create<ast::Literal>(
                 assign<&ast::Literal::value>(ref<TOK_FLOAT_CONST>)) |
             ref<Variable> |
             (ref<TOK_LPAREN> + ref<BuiltIns> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<FunctionCall> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<ArithExpr> + ref<TOK_RPAREN>)> {};

  struct ArithExpr
      : infix<ArithValue, ast::ArithExpr,
              levels(left(as<ast::ArithOp::mul>(ref<TOK_TIMES>) | as<ast::ArithOp::div>(ref<TOK_DIVIDE>) | as<ast::ArithOp::mod>(ref<TOK_MODULUS>)),
                     left(as<ast::ArithOp::add>(ref<TOK_PLUS>) | as<ast::ArithOp::sub>(ref<TOK_MINUS>)))> {};

  struct ScalarPredicate
      : rule<ref<TOK_SAME_TYPE> | ref<TOK_DIFF_TYPE> | ref<TOK_EQUAL> |
             ref<TOK_EQUAL_EQUAL> | ref<TOK_APPROX_EQUAL> |
             ref<TOK_NOT_APPROX_EQUAL> | ref<TOK_NOT_EQ> |
             ref<TOK_NOT_EQUAL> | ref<TOK_GREATER> | ref<TOK_GREATER_EQUAL> |
             ref<TOK_LESS> | ref<TOK_LESS_EQUAL>> {};

  // A predicate is one token or two: `CONTAINS ==` and `== CONTAINS` are
  // both legal. A directive cannot stand over a two-token sequence, so the
  // whole predicate is a rule of its own and the member reads `ref<>` of it
  // -- its value joins the leaves. The two-token forms come first so a
  // pair is never read as its first half alone.
  struct Predicate
      : rule<(ref<TOK_CONTAINS> + ref<ScalarPredicate>) |
             (ref<TOK_DOES_NOT_CONTAIN> + ref<ScalarPredicate>) |
             (ref<ScalarPredicate> + ref<TOK_CONTAINS>) |
             (ref<ScalarPredicate> + ref<TOK_DOES_NOT_CONTAIN>) |
             ref<TOK_LENGTH_EQUAL> | ref<TOK_LENGTH_NOT_EQUAL> |
             ref<TOK_LENGTH_GREATER> | ref<TOK_LENGTH_GREATER_EQUAL> |
             ref<TOK_LENGTH_LESS> | ref<TOK_LENGTH_LESS_EQUAL> |
             ref<TOK_CONTAINS> | ref<TOK_DOES_NOT_CONTAIN> |
             ref<ScalarPredicate>> {};

  static constexpr auto PredicateText = ref<Predicate>;

  struct Not
      : rule<ref<TOK_NOT> +
             create<ast::Not>(
                 assign<&ast::Not::operand>(ref<ExpreValue>))> {};

  struct Compare
      : rule<create<ast::Compare>(
            assign<&ast::Compare::left>(ref<ValueNonSql>) +
            assign<&ast::Compare::predicateText>(PredicateText) +
            assign<&ast::Compare::right>(ref<ValueNonSql>))> {};

  struct ExpreValue
      : rule<ref<Not> | ref<Compare> |
             (ref<TOK_LPAREN> + ref<CmplxExpr> + ref<TOK_RPAREN>)> {};


  struct CmplxExpr
      : infix<ExpreValue, ast::Logical,
              levels(left(as<ast::LogicalOp::logical_and>(ref<TOK_AND>)), left(as<ast::LogicalOp::logical_or>(ref<TOK_OR>)))> {};

  struct Expression
      : rule<ref<TOK_LPAREN> + ref<CmplxExpr> + ref<TOK_RPAREN>> {};

  // ---- LHS: conditions -------------------------------------------------
  struct CaIndex
      : rule<ref<ArithExpr> |
             create<ast::Literal>(
                 assign<&ast::Literal::value>(ref<TOK_DOLLAR_LAST>))> {};

  struct Restriction
      : rule<create<ast::Restriction>(
                 ref<TOK_START_DISJUNCTION> +
                 some(append<&ast::Restriction::values>(ref<Value>)) +
                 ref<TOK_END_DISJUNCTION>) |
             create<ast::Restriction>(
                 assign<&ast::Restriction::predicateText>(PredicateText) +
                 append<&ast::Restriction::values>(ref<Value>)) |
             create<ast::Restriction>(
                 append<&ast::Restriction::values>(ref<Value>))> {};

  struct LhsTerm
      : rule<create<ast::LhsTerm>(
            ref<TOK_HAT> + assign<&ast::LhsTerm::attribute>(AttributeText) +
            option(ref<TOK_LBRACKET> +
                   assign<&ast::LhsTerm::index>(ref<CaIndex>) +
                   ref<TOK_RBRACKET>) +
            (append<&ast::LhsTerm::restrictions>(ref<Restriction>) |
             (ref<TOK_LBRACE> +
              some(append<&ast::LhsTerm::restrictions>(ref<Restriction>)) +
              ref<TOK_RBRACE>)))> {};

  struct CeForm
      : rule<create<ast::CeForm>(
            ref<TOK_LPAREN> + assign<&ast::CeForm::className>(Name) +
            many(append<&ast::CeForm::terms>(ref<LhsTerm>)) +
            ref<TOK_RPAREN>)> {};

  struct Ce
      : rule<create<ast::Ce>(append<&ast::Ce::alternatives>(ref<CeForm>)) |
             create<ast::Ce>(ref<TOK_START_DISJUNCTION> +
                             some(append<&ast::Ce::alternatives>(
                                 ref<CeForm>)) +
                             ref<TOK_END_DISJUNCTION>) |
             create<ast::Ce>(
                 flag<&ast::Ce::negated>(ref<TOK_MINUS>) +
                 append<&ast::Ce::alternatives>(ref<CeForm>))> {};

  // ---- RHS: actions ------------------------------------------------------
  struct RhsTerm
      : rule<create<ast::RhsTerm>(
            ref<TOK_HAT> + assign<&ast::RhsTerm::attribute>(AttributeText) +
            option(ref<TOK_LBRACKET> +
                   assign<&ast::RhsTerm::index>(ref<CaIndex>) +
                   ref<TOK_RBRACKET>) +
            assign<&ast::RhsTerm::value>(ref<Value>))> {};


  struct AddStateAction
      : rule<create<ast::Action>(assign<&ast::Action::kind>(as<ast::ActionKind::add_state>(ref<TOK_ADDSTATE>)) +
                                 append<&ast::Action::values>(ref<Value>))> {
  };
  struct AfterAction
      : rule<create<ast::Action>(assign<&ast::Action::kind>(as<ast::ActionKind::after>(ref<TOK_AFTER>)) +
                                 append<&ast::Action::values>(ref<Value>) +
                                 assign<&ast::Action::name>(Name))> {};
  struct BindAction
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(as<ast::ActionKind::bind>(ref<TOK_BIND>)) +
            append<&ast::Action::values>(ref<Variable>) +
            append<&ast::Action::values>(ref<Value>))> {};
  struct CallInheritedAction
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(as<ast::ActionKind::call_inherited>(ref<TOK_CALL_INHERITED>)) +
            some(append<&ast::Action::values>(ref<Variable>)))> {};
  struct CloseFileAction
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(as<ast::ActionKind::close_file>(ref<TOK_CLOSEFILE>)) +
            some(append<&ast::Action::values>(ref<Value>)))> {};
  struct CopyAction
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(as<ast::ActionKind::copy>(ref<TOK_COPY>)) +
            append<&ast::Action::values>(ref<Variable>) +
            many(append<&ast::Action::terms>(ref<RhsTerm>)))> {};
  struct DebugAction
      : rule<create<ast::Action>(assign<&ast::Action::kind>(as<ast::ActionKind::debug>(ref<TOK_DEBUG>)))> {
  };
  struct DefaultAction
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(as<ast::ActionKind::default_>(ref<TOK_DEFAULT>)) +
            append<&ast::Action::values>(ref<Value>) +
            append<&ast::Action::values>(ref<Value>))> {};
  struct ForEachAction
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(as<ast::ActionKind::for_each>(ref<TOK_FOR_EACH>)) +
            append<&ast::Action::values>(ref<Variable>) + ref<TOK_IN> +
            append<&ast::Action::values>(ref<Value>) +
            some(append<&ast::Action::body>(ref<ActionLine>)))> {};
  struct IfAction
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(as<ast::ActionKind::if_>(ref<TOK_IF>)) +
            assign<&ast::Action::condition>(ref<Expression>) +
            ref<TOK_THEN> +
            some(append<&ast::Action::body>(ref<ActionLine>)) +
            option(ref<TOK_ELSE> +
                   some(append<&ast::Action::elseBody>(ref<ActionLine>))))> {
  };
  struct MakeAction
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(as<ast::ActionKind::make>(ref<TOK_MAKE>)) +
            append<&ast::Action::values>(ref<Rclass>) +
            many(append<&ast::Action::terms>(ref<RhsTerm>)))> {};
  struct ModifyAction
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(as<ast::ActionKind::modify>(ref<TOK_MODIFY>)) +
            append<&ast::Action::values>(ref<Variable>) +
            many(append<&ast::Action::terms>(ref<RhsTerm>)))> {};
  struct OpenFileAction
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(as<ast::ActionKind::open_file>(ref<TOK_OPENFILE>)) +
            append<&ast::Action::values>(ref<Value>) +
            append<&ast::Action::values>(ref<Value>) +
            append<&ast::Action::values>(ref<Value>))> {};
  struct QuitAction
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(as<ast::ActionKind::quit>(ref<TOK_QUIT>)) +
            option(append<&ast::Action::values>(
                ref<ArithValue> |
                create<ast::Literal>(
                    assign<&ast::Literal::value>(ref<TOK_FAILURE>)) |
                create<ast::Literal>(
                    assign<&ast::Literal::value>(ref<TOK_SUCCESS>)))))> {};
  struct RemoveAction
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(as<ast::ActionKind::remove>(ref<TOK_REMOVE>)) +
            some(append<&ast::Action::values>(ref<Variable>)))> {};
  struct RemoveEveryAction
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(as<ast::ActionKind::remove_every>(ref<TOK_REMOVE_EVERY>)) +
            some(append<&ast::Action::values>(ref<Rclass>)))> {};
  struct RestoreStateAction
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(as<ast::ActionKind::restore_state>(ref<TOK_RESTORESTATE>)) +
            append<&ast::Action::values>(ref<Value>))> {};
  struct ReturnAction
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(as<ast::ActionKind::return_>(ref<TOK_RETURN>)) +
            option(append<&ast::Action::values>(ref<Value>)))> {};
  struct SaveStateAction
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(as<ast::ActionKind::save_state>(ref<TOK_SAVESTATE>)) +
            append<&ast::Action::values>(ref<Value>))> {};
  struct SpecializeAction
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(as<ast::ActionKind::specialize>(ref<TOK_SPECIALIZE>)) +
            append<&ast::Action::values>(ref<Variable>) +
            append<&ast::Action::values>(ref<Rclass>) +
            many(append<&ast::Action::terms>(ref<RhsTerm>)))> {};
  struct TraceAction
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(as<ast::ActionKind::trace>(ref<TOK_TRACE>)) +
            assign<&ast::Action::name>(ref<TOK_ON> | ref<TOK_OFF>) +
            (append<&ast::Action::values>(create<ast::Literal>(
                 assign<&ast::Literal::value>(ref<TOK_TIMES>))) |
             some(append<&ast::Action::values>(create<ast::Literal>(
                 assign<&ast::Literal::value>(
                     ref<TOK_ENTRY_BLOCK> | ref<TOK_EB> |
                     ref<TOK_RULE_GROUP> | ref<TOK_RG> | ref<TOK_RULE> |
                     ref<TOK_WM> | ref<TOK_CS>))))))> {};
  struct WhileAction
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(as<ast::ActionKind::while_>(ref<TOK_WHILE>)) +
            assign<&ast::Action::condition>(ref<Expression>) +
            ref<TOK_DO> +
            some(append<&ast::Action::body>(ref<ActionLine>)))> {};
  struct WriteAction
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(as<ast::ActionKind::write>(ref<TOK_WRITE>)) +
            many(append<&ast::Action::values>(ref<Value>)))> {};

  static constexpr auto SqlRseElement =
      ref<Constant> | ref<Variable> | ref<QuotedVar> |
      create<ast::Literal>(assign<&ast::Literal::value>(
          ref<TOK_EQUAL> | ref<TOK_EQUAL_EQUAL> | ref<TOK_APPROX_EQUAL> |
          ref<TOK_NOT_APPROX_EQUAL> | ref<TOK_NOT_EQ> | ref<TOK_NOT_EQUAL> |
          ref<TOK_GREATER> | ref<TOK_GREATER_EQUAL> | ref<TOK_LESS> |
          ref<TOK_LESS_EQUAL> | ref<TOK_PLUS> | ref<TOK_MINUS> |
          ref<TOK_TIMES> | ref<TOK_DIVIDE> | ref<TOK_MODULUS> |
          ref<TOK_ARROW> | ref<TOK_START_DISJUNCTION> |
          ref<TOK_END_DISJUNCTION>));

  struct SqlAttachAction
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(as<ast::ActionKind::sql_attach>(ref<TOK_SQL_ATTACH>)) +
            (append<&ast::Action::values>(create<ast::Call>(
                 assign<&ast::Call::name>(ref<TOK_FILENAME>) +
                 append<&ast::Call::args>(create<ast::Literal>(
                     assign<&ast::Literal::value>(Name))))) |
             append<&ast::Action::values>(create<ast::Call>(
                 assign<&ast::Call::name>(ref<TOK_PATHNAME>) +
                 append<&ast::Call::args>(create<ast::Literal>(
                     assign<&ast::Literal::value>(Name))))) |
             append<&ast::Action::values>(
                 create<ast::Literal>(assign<&ast::Literal::value>(
                     DbSpecName)))) +
            option(append<&ast::Action::values>(create<ast::Literal>(
                assign<&ast::Literal::value>(Name)))))> {};
  struct SqlCommitAction
      : rule<create<ast::Action>(assign<&ast::Action::kind>(
            as<ast::ActionKind::sql_commit>(ref<TOK_SQL_COMMIT>)))> {};
  struct SqlDetachAction
      : rule<create<ast::Action>(assign<&ast::Action::kind>(
            as<ast::ActionKind::sql_detach>(ref<TOK_SQL_DETACH>)))> {};
  struct SqlRollbackAction
      : rule<create<ast::Action>(assign<&ast::Action::kind>(
            as<ast::ActionKind::sql_rollback>(ref<TOK_SQL_ROLLBACK>)))> {};
  struct SqlDeleteAction
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(as<ast::ActionKind::sql_delete>(ref<TOK_SQL_DELETE>)) +
            append<&ast::Action::values>(
                create<ast::Literal>(assign<&ast::Literal::value>(Name))) +
            many(append<&ast::Action::values>(SqlRseElement)))> {};
  struct SqlFetchEachAction
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(as<ast::ActionKind::sql_fetch_each>(ref<TOK_SQL_FETCH_EACH>)) +
            some(append<&ast::Action::values>(ref<Variable>)) +
            ref<TOK_LPAREN> +
            some(append<&ast::Action::values>(SqlRseElement)) +
            ref<TOK_RPAREN> +
            some(append<&ast::Action::body>(ref<ActionLine>)))> {};
  struct SqlFetchAsObjectAction
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(as<ast::ActionKind::sql_fetch_as_object>(ref<TOK_SQL_FETCH_AS_OBJECT>)) +
            some(append<&ast::Action::values>(SqlRseElement)))> {};
  struct SqlInsertAction
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(as<ast::ActionKind::sql_insert>(ref<TOK_SQL_INSERT>)) +
            append<&ast::Action::values>(
                create<ast::Literal>(assign<&ast::Literal::value>(Name))) +
            some(append<&ast::Action::values>(SqlRseElement)))> {};
  struct SqlInsertFromObjectAction
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(as<ast::ActionKind::sql_insert_from_object>(ref<TOK_SQL_INSERT_FROM_OBJECT>)) +
            append<&ast::Action::values>(ref<Variable>))> {};
  struct SqlStartAction
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(as<ast::ActionKind::sql_start>(ref<TOK_SQL_START>)) +
            many(append<&ast::Action::values>(SqlRseElement)))> {};
  struct SqlUpdateAction
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(as<ast::ActionKind::sql_update>(ref<TOK_SQL_UPDATE>)) +
            append<&ast::Action::values>(
                create<ast::Literal>(assign<&ast::Literal::value>(Name))) +
            some(append<&ast::Action::values>(SqlRseElement)))> {};
  struct SqlUpdateFromObjectAction
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(as<ast::ActionKind::sql_update_from_object>(ref<TOK_SQL_UPDATE_FROM_OBJECT>)) +
            append<&ast::Action::values>(ref<Variable>) +
            many(append<&ast::Action::values>(SqlRseElement)))> {};

  struct Action
      : rule<ref<AddStateAction> | ref<AfterAction> |
             ref<BindAction> | ref<CallInheritedAction> |
             ref<CloseFileAction> | ref<CopyAction> |
             ref<DebugAction> | ref<DefaultAction> |
             ref<ForEachAction> | ref<FunctionCall> |
             ref<IfAction> | ref<MakeAction> | ref<ModifyAction> |
             ref<OpenFileAction> | ref<QuitAction> |
             ref<RemoveAction> | ref<RemoveEveryAction> |
             ref<RestoreStateAction> | ref<ReturnAction> |
             ref<SaveStateAction> | ref<SpecializeAction> |
             ref<TraceAction> | ref<WhileAction> |
             ref<WriteAction> | ref<SqlAttachAction> |
             ref<SqlCommitAction> | ref<SqlDeleteAction> |
             ref<SqlDetachAction> | ref<SqlFetchEachAction> |
             ref<SqlFetchAsObjectAction> | ref<SqlInsertAction> |
             ref<SqlInsertFromObjectAction> | ref<SqlRollbackAction> |
             ref<SqlStartAction> | ref<SqlUpdateAction> |
             ref<SqlUpdateFromObjectAction>> {};

  // No `"(" construct_keyword` half-open alternative here: the LALR states
  // already stop this loop on any token that isn't a legal action opener.
  struct ActionLine
      : rule<ref<TOK_LPAREN> + ref<Action> + ref<TOK_RPAREN>> {};

  struct AtCommand
      : rule<create<ast::Action>(assign<&ast::Action::kind>(as<ast::ActionKind::at>(ref<TOK_AT>)) +
                                 assign<&ast::Action::name>(Name))> {};
  struct StartupAction
      : rule<ref<Action> | ref<AtCommand>> {};
  struct StartupActionLine
      : rule<ref<TOK_LPAREN> + ref<StartupAction> + ref<TOK_RPAREN>> {};

  // ---- Top-level constructs ---------------------------------------------
  struct Rule
      : rule<create<ast::Rule>(
            assign<&ast::Rule::keyword>(ref<TOK_RULE> | ref<TOK_P>) +
            assign<&ast::Rule::name>(Name) +
            append<&ast::Rule::conditions>(create<ast::Ce>(
                append<&ast::Ce::alternatives>(ref<CeForm>))) +
            many(append<&ast::Rule::conditions>(ref<Ce>)) +
            ref<TOK_ARROW> +
            many(append<&ast::Rule::actions>(ref<ActionLine>)))> {};

  struct Catcher
      : rule<create<ast::Catcher>(
            ref<TOK_CATCH> + assign<&ast::Catcher::name>(Name) +
            many(append<&ast::Catcher::actions>(ref<ActionLine>)))> {};

  struct OnEntry
      : rule<create<ast::OnEntry>(
            (ref<TOK_ON_ENTRY> | ref<TOK_STARTUP>) +
            some(append<&ast::OnEntry::actions>(ref<StartupActionLine>)))> {
  };
  struct OnEmpty
      : rule<create<ast::OnEmpty>(
            ref<TOK_ON_EMPTY> +
            some(append<&ast::OnEmpty::actions>(ref<ActionLine>)))> {};
  struct OnExit
      : rule<create<ast::OnExit>(
            ref<TOK_ON_EXIT> +
            some(append<&ast::OnExit::actions>(ref<ActionLine>)))> {};
  struct OnEvery
      : rule<create<ast::OnEvery>(
            ref<TOK_ON_EVERY> +
            some(append<&ast::OnEvery::actions>(ref<ActionLine>)))> {};

  struct DeclarationBlock
      : rule<create<ast::DeclarationBlock>(
            ref<TOK_DECLARATION_BLOCK> +
            assign<&ast::DeclarationBlock::name>(Name))> {};
  struct RuleGroupDecl
      : rule<create<ast::RuleGroupDecl>(
            ref<TOK_RULE_GROUP> + assign<&ast::RuleGroupDecl::name>(Name))> {
  };
  struct EndRuleGroup
      : rule<create<ast::EndRuleGroup>(
            ref<TOK_END_GROUP> +
            option(assign<&ast::EndRuleGroup::name>(Name)))> {};
  struct EndBlock
      : rule<create<ast::EndBlock>(
            ref<TOK_END_BLOCK> + option(assign<&ast::EndBlock::name>(Name)))> {
  };

  struct ExtRtParamDecl
      : rule<create<ast::ParamDecl>(
            option(assign<&ast::ParamDecl::name>(ref<TOK_VARIABLE>)) +
            option(ref<TOK_LBRACKET> +
                   option(assign<&ast::ParamDecl::arrayLength>(
                       create<ast::Literal>(assign<&ast::Literal::value>(
                           ref<TOK_INTEGER_CONST>)) |
                       ref<Variable>)) +
                   ref<TOK_RBRACKET>) +
            assign<&ast::ParamDecl::type>(
                ref<TOK_LONG> | ref<TOK_SHORT> | ref<TOK_BYTE> |
                ref<TOK_UNSIGNED_LONG> | ref<TOK_UNSIGNED_SHORT> |
                ref<TOK_UNSIGNED_BYTE> | ref<TOK_SINGLE_FLOAT> |
                ref<TOK_DOUBLE_FLOAT> | ref<TOK_ASCIZ> | ref<TOK_ASCID> |
                ref<TOK_POINTER> | ref<TOK_ATOM>) +
            option(ref<TOK_BY> +
                   ((ref<TOK_REFERENCE> +
                     assign<&ast::ParamDecl::mechanism>(
                         ref<TOK_READ_ONLY> | ref<TOK_READ_WRITE>)) |
                    assign<&ast::ParamDecl::mechanism>(ref<TOK_VALUE>))))> {};

  struct ExtRtReturnDecl
      : rule<create<ast::ParamDecl>(
            option(assign<&ast::ParamDecl::name>(ref<TOK_VARIABLE>)) +
            option(ref<TOK_LBRACKET> +
                   option(assign<&ast::ParamDecl::arrayLength>(
                       create<ast::Literal>(assign<&ast::Literal::value>(
                           ref<TOK_INTEGER_CONST>)) |
                       ref<Variable>)) +
                   ref<TOK_RBRACKET>) +
            assign<&ast::ParamDecl::type>(
                ref<TOK_LONG> | ref<TOK_SHORT> | ref<TOK_BYTE> |
                ref<TOK_UNSIGNED_LONG> | ref<TOK_UNSIGNED_SHORT> |
                ref<TOK_UNSIGNED_BYTE> | ref<TOK_SINGLE_FLOAT> |
                ref<TOK_DOUBLE_FLOAT> | ref<TOK_ASCIZ> | ref<TOK_ASCID> |
                ref<TOK_POINTER> | ref<TOK_ATOM>) +
            option(ref<TOK_BY> +
                   assign<&ast::ParamDecl::mechanism>(ref<TOK_REFERENCE> |
                                                       ref<TOK_VALUE>)))> {};

  static constexpr auto EntryBlockClause =
      ref<TOK_LPAREN> +
      ((ref<TOK_ACCEPTS> +
        some(append<&ast::EntryBlock::accepts>(ref<ExtRtParamDecl>))) |
       (ref<TOK_ACTIVATES> +
        some(append<&ast::EntryBlock::activates>(Name))) |
       (ref<TOK_USES> + some(append<&ast::EntryBlock::uses>(Name)))) +
      ref<TOK_RPAREN>;
  static constexpr auto EntryBlockReturns =
      ref<TOK_LPAREN> + ref<TOK_RETURNS> +
      assign<&ast::EntryBlock::returns>(ref<ExtRtReturnDecl>) +
      ref<TOK_RPAREN>;
  static constexpr auto EntryBlockStrategy =
      ref<TOK_LPAREN> + ref<TOK_STRATEGY> +
      assign<&ast::EntryBlock::strategy>(ref<TOK_LEX> | ref<TOK_MEA>) +
      ref<TOK_RPAREN>;

  struct EntryBlock
      : rule<create<ast::EntryBlock>(
            ref<TOK_ENTRY_BLOCK> + assign<&ast::EntryBlock::name>(Name) +
            many(EntryBlockClause) +
            option((EntryBlockReturns + many(EntryBlockClause) +
                    option(EntryBlockStrategy + many(EntryBlockClause))) |
                   (EntryBlockStrategy + many(EntryBlockClause) +
                    option(EntryBlockReturns +
                           many(EntryBlockClause)))))> {};

  static constexpr auto RuleBlockUses =
      ref<TOK_LPAREN> + ref<TOK_USES> +
      some(append<&ast::RuleBlock::uses>(Name)) + ref<TOK_RPAREN>;

  struct RuleBlock
      : rule<create<ast::RuleBlock>(
            ref<TOK_RULE_BLOCK> + assign<&ast::RuleBlock::name>(Name) +
            many(RuleBlockUses) +
            option(ref<TOK_LPAREN> + ref<TOK_STRATEGY> +
                   assign<&ast::RuleBlock::strategy>(ref<TOK_LEX> |
                                                      ref<TOK_MEA>) +
                   ref<TOK_RPAREN> + many(RuleBlockUses)))> {};

  struct CompoundConst
      : rule<create<ast::Call>(
            ref<TOK_LPAREN> + assign<&ast::Call::name>(ref<TOK_COMPOUND>) +
            many(append<&ast::Call::args>(ref<Constant>)) +
            ref<TOK_RPAREN>)> {};

  struct AttributeModifier
      : rule<create<ast::AttributeModifier>(
                 ref<TOK_LPAREN> +
                 assign<&ast::AttributeModifier::kind>(ref<TOK_DEFAULT>) +
                 assign<&ast::AttributeModifier::value>(ref<Constant>) +
                 ref<TOK_RPAREN>) |
             create<ast::AttributeModifier>(
                 ref<TOK_LPAREN> +
                 assign<&ast::AttributeModifier::kind>(ref<TOK_DEFAULT>) +
                 assign<&ast::AttributeModifier::value>(
                     ref<CompoundConst>) +
                 ref<TOK_RPAREN>) |
             create<ast::AttributeModifier>(
                 ref<TOK_LPAREN> +
                 assign<&ast::AttributeModifier::kind>(ref<TOK_FILL>) +
                 assign<&ast::AttributeModifier::value>(ref<Constant>) +
                 ref<TOK_RPAREN>)> {};

  struct AttributeDecl
      : rule<create<ast::AttributeDecl>(
            ref<TOK_HAT> + assign<&ast::AttributeDecl::name>(Name) +
            option(flag<&ast::AttributeDecl::compound>(
                ref<TOK_COMPOUND>)) +
            option((assign<&ast::AttributeDecl::domain>(ref<TOK_ANY>)) |
                   (assign<&ast::AttributeDecl::domain>(ref<TOK_INSTANCE>) +
                    option(ref<TOK_OF> +
                           assign<&ast::AttributeDecl::domainClass>(Name))) |
                   (assign<&ast::AttributeDecl::domain>(ref<TOK_INTEGER>)) |
                   (assign<&ast::AttributeDecl::domain>(ref<TOK_FLOAT>)) |
                   (assign<&ast::AttributeDecl::domain>(ref<TOK_OPAQUE>)) |
                   (assign<&ast::AttributeDecl::domain>(ref<TOK_SYMBOL>))) +
            many(append<&ast::AttributeDecl::modifiers>(
                ref<AttributeModifier>)))> {};

  struct ObjectClassDecl
      : rule<create<ast::ObjectClassDecl>(
            ref<TOK_OBJECT_CLASS> +
            assign<&ast::ObjectClassDecl::name>(Name) +
            option(ref<TOK_LPAREN> + ref<TOK_INHERITS_FROM> +
                   assign<&ast::ObjectClassDecl::inheritsFrom>(Name) +
                   ref<TOK_RPAREN>) +
            many(append<&ast::ObjectClassDecl::attributes>(
                ref<AttributeDecl>)))> {};

  static constexpr auto MethodParamOptions =
      option(flag<&ast::ParamDecl::compound>(ref<TOK_COMPOUND>)) +
      option((assign<&ast::ParamDecl::type>(ref<TOK_INSTANCE>) +
              option(ref<TOK_OF> + assign<&ast::ParamDecl::domainClass>(Name))) |
             assign<&ast::ParamDecl::type>(ref<TOK_ANY> | ref<TOK_INTEGER> |
                 ref<TOK_FLOAT> | ref<TOK_OPAQUE> | ref<TOK_SYMBOL>));
  struct MethodParamDecl
      : rule<create<ast::ParamDecl>(
            assign<&ast::ParamDecl::name>(ref<TOK_VARIABLE>) + MethodParamOptions)> {};
  struct MethodReturnParam
      : rule<create<ast::ParamDecl>(
            assign<&ast::ParamDecl::name>(ref<TOK_VARIABLE>) + MethodParamOptions)> {};

  struct MethodDecl
      : rule<create<ast::MethodDecl>(
            ref<TOK_METHOD> + assign<&ast::MethodDecl::name>(AllowableFuncName) +
            ref<TOK_LPAREN> + ref<TOK_ACCEPTS> +
            assign<&ast::MethodDecl::whenVar>(ref<Variable>) +
            ref<TOK_WHEN> + ref<TOK_INSTANCE> + ref<TOK_OF> +
            assign<&ast::MethodDecl::whenClassName>(Name) +
            many(append<&ast::MethodDecl::params>(ref<MethodParamDecl>)) +
            ref<TOK_RPAREN> +
            (some(append<&ast::MethodDecl::actions>(ref<ActionLine>)) |
             (ref<TOK_LPAREN> + ref<TOK_RETURNS> +
              assign<&ast::MethodDecl::returns>(ref<MethodReturnParam>) +
              ref<TOK_RPAREN> +
              many(append<&ast::MethodDecl::actions>(
                  ref<ActionLine>)))))> {};

  struct GenericMethodDecl
      : rule<create<ast::GenericMethodDecl>(
            ref<TOK_GENERIC_METHOD> +
            assign<&ast::GenericMethodDecl::name>(AllowableFuncName) +
            ref<TOK_LPAREN> + ref<TOK_ACCEPTS> +
            assign<&ast::GenericMethodDecl::whenVar>(ref<Variable>) +
            ref<TOK_WHEN> + ref<TOK_INSTANCE> +
            option(ref<TOK_OF> +
                   assign<&ast::GenericMethodDecl::whenClassName>(Name)) +
            many(append<&ast::GenericMethodDecl::params>(
                ref<MethodParamDecl>)) +
            ref<TOK_RPAREN> +
            option(ref<TOK_LPAREN> + ref<TOK_RETURNS> +
                   assign<&ast::GenericMethodDecl::returns>(
                       ref<MethodReturnParam>) +
                   ref<TOK_RPAREN>))> {};

  static constexpr auto ExtRtAccepts =
      ref<TOK_LPAREN> + ref<TOK_ACCEPTS> +
      some(append<&ast::ExternalRoutineDecl::accepts>(
          ref<ExtRtParamDecl>)) +
      ref<TOK_RPAREN>;
  static constexpr auto ExtRtAlias =
      ref<TOK_LPAREN> + ref<TOK_ALIAS_FOR> +
      assign<&ast::ExternalRoutineDecl::alias>(Name) + ref<TOK_RPAREN>;
  static constexpr auto ExtRtReturns =
      ref<TOK_LPAREN> + ref<TOK_RETURNS> +
      assign<&ast::ExternalRoutineDecl::returns>(ref<ExtRtReturnDecl>) +
      ref<TOK_RPAREN>;

  struct ExternalRoutineDecl
      : rule<create<ast::ExternalRoutineDecl>(
            ref<TOK_EXTERNAL_ROUTINE> +
            assign<&ast::ExternalRoutineDecl::name>(AllowableFuncName) +
            many(ExtRtAccepts) +
            option((ExtRtAlias + many(ExtRtAccepts) +
                    option(ExtRtReturns + many(ExtRtAccepts))) |
                   (ExtRtReturns + many(ExtRtAccepts) +
                    option(ExtRtAlias + many(ExtRtAccepts)))))> {};

  static constexpr auto ProgramLine =
      ref<DeclarationBlock> | ref<EntryBlock> | ref<RuleBlock> |
      ref<EndBlock> | ref<RuleGroupDecl> | ref<EndRuleGroup> |
      ref<ObjectClassDecl> | ref<MethodDecl> |
      ref<GenericMethodDecl> | ref<ExternalRoutineDecl> |
      ref<OnEntry> | ref<OnEmpty> | ref<OnExit> |
      ref<OnEvery> | ref<Rule> | ref<Catcher>;

  struct ProgramConstruct
      : rule<create<ast::ProgramConstruct>(
            ref<TOK_LPAREN> +
            append<&ast::ProgramConstruct::lines>(ProgramLine) +
            ref<TOK_RPAREN>)> {};

  struct Program
      : rule<create<ast::Program>(
            some(append<&ast::Program::constructs>(ref<ProgramConstruct>)))> {
  };

  using entry = Program;
  using ignored = WS;
  using hidden = list<TrailingComment>;
};

}  // namespace ruleworks
