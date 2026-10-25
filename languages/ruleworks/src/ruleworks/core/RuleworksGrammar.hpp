// The RuleWorks language: its grammar. RuleWorks is case-insensitive and
// almost every keyword doubles as a plain name; `word_boundary` is the
// symbol-character class itself, not `w`, since symbols routinely contain
// `-`, `$`, `.` and other punctuation.
#pragma once

#include <cstdint>
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
using StripAngle = cursiv::text_conv<strip_angle>;
inline constexpr auto strip_quoted_angle = [](std::string_view t) {
  return strip_ends(t, 2, 2);
};
using StripQuotedAngle = cursiv::text_conv<strip_quoted_angle>;
inline constexpr auto strip_pipes = [](std::string_view t) {
  return strip_ends(t, 1, 1);
};
using StripPipes = cursiv::text_conv<strip_pipes>;

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
      : terminal<(option(NumSign) + many(Digit) + lit<"."> + some(Digit) +
                  option(Exponent)) |
                     (option(NumSign) + some(Digit) + lit<"."> + Exponent),
                 double> {};
  struct TOK_INTEGER_CONST
      : terminal<option(NumSign) + some(Digit) + option(lit<".">),
                 std::int64_t> {};
  struct TOK_INSTANCE_CONST : terminal<lit<"#"> + some(Digit)> {};
  struct TOK_OPAQUE_CONST
      : terminal<(lit<"%X"> | lit<"%x">) + some(HexDigit)> {};

  struct TOK_VARIABLE
      : terminal<lit<"<"> + some(Nonmagic) + lit<">">, StripAngle> {};
  struct TOK_QUOTED_VAR
      : terminal<lit<"<\""> + some(Nonmagic) + lit<"\">">,
                 StripQuotedAngle> {};
  struct TOK_SYMBOL_CONST : terminal<some(SymbolChar)> {};
  struct TOK_QUOTED_SYMBOL
      : terminal<through(lit<"|">, lit<"|">), StripPipes> {};
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
  struct TOK_WM : terminal<ikw<"TM">> {};
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
  struct VariableRule;
  struct QuotedVarRule;
  struct ConstantRule;
  struct AttributeLiteralRule;
  struct IndexNumRule;
  struct RclassRule;
  struct ValueRule;
  struct ValueNonSqlRule;
  struct ArithValueRule;
  struct ArithExprRule;
  struct ExpreValueRule;
  struct CmplxExprRule;
  struct ExpressionRule;
  struct CaIndexRule;
  struct RestrictionRule;
  struct LhsTermRule;
  struct CeFormRule;
  struct CeRule;
  struct RhsTermRule;
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
  struct BuiltInsRule;
  struct FunctionCallRule;
  struct ActionRule;
  struct ActionLineRule;
  struct AtCommandRule;
  struct StartupActionRule;
  struct StartupActionLineRule;
  struct AddStateActionRule;
  struct AfterActionRule;
  struct BindActionRule;
  struct CallInheritedActionRule;
  struct CloseFileActionRule;
  struct CopyActionRule;
  struct DebugActionRule;
  struct DefaultActionRule;
  struct ForEachActionRule;
  struct IfActionRule;
  struct MakeActionRule;
  struct ModifyActionRule;
  struct OpenFileActionRule;
  struct QuitActionRule;
  struct RemoveActionRule;
  struct RemoveEveryActionRule;
  struct RestoreStateActionRule;
  struct ReturnActionRule;
  struct SaveStateActionRule;
  struct SpecializeActionRule;
  struct TraceActionRule;
  struct WhileActionRule;
  struct WriteActionRule;
  struct SqlAttachActionRule;
  struct SqlCommitActionRule;
  struct SqlDeleteActionRule;
  struct SqlDetachActionRule;
  struct SqlFetchEachActionRule;
  struct SqlFetchAsObjectActionRule;
  struct SqlInsertActionRule;
  struct SqlInsertFromObjectActionRule;
  struct SqlRollbackActionRule;
  struct SqlStartActionRule;
  struct SqlUpdateActionRule;
  struct SqlUpdateFromObjectActionRule;

  // ---- Values ---------------------------------------------------------
  struct VariableRule
      : rule<create<ast::Variable>(
            assign<&ast::Variable::name>(ref<TOK_VARIABLE>))> {};
  struct QuotedVarRule
      : rule<create<ast::Variable>(
            assign<&ast::Variable::name>(ref<TOK_QUOTED_VAR>))> {};

  struct ConstantRule
      : rule<create<ast::Literal>(
                 assign<&ast::Literal::value>(ref<TOK_INTEGER_CONST>)) |
             create<ast::Literal>(
                 assign<&ast::Literal::value>(ref<TOK_FLOAT_CONST>)) |
             create<ast::Literal>(
                 assign<&ast::Literal::value>(ref<TOK_OPAQUE_CONST>)) |
             create<ast::Literal>(
                 assign<&ast::Literal::value>(ref<TOK_INSTANCE_CONST>)) |
             create<ast::Literal>(assign<&ast::Literal::value>(Name))> {};

  struct AttributeLiteralRule
      : rule<create<ast::Literal>(
            assign<&ast::Literal::value>(AttributeText))> {};

  struct IndexNumRule
      : rule<create<ast::Literal>(
                 assign<&ast::Literal::value>(ref<TOK_INTEGER_CONST>)) |
             ref<VariableRule> |
             (ref<TOK_LPAREN> + ref<BuiltInsRule> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<FunctionCallRule> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<ArithExprRule> + ref<TOK_RPAREN>) |
             create<ast::Literal>(
                 assign<&ast::Literal::value>(ref<TOK_DOLLAR_LAST>))> {};

  struct RclassRule
      : rule<create<ast::Literal>(assign<&ast::Literal::value>(Name)) |
             ref<VariableRule> |
             (ref<TOK_LPAREN> + ref<AcceptAtomBti> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<ConcatBti> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<GetBti> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<NthBti> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<SymbolBti> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<FunctionCallRule> + ref<TOK_RPAREN>)> {};

  struct ValueNonSqlRule
      : rule<ref<ConstantRule> | ref<VariableRule> |
             (ref<TOK_LPAREN> + ref<BuiltInsRule> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<FunctionCallRule> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<ArithExprRule> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<CopyActionRule> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<MakeActionRule> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<ModifyActionRule> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<SpecializeActionRule> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<CallInheritedActionRule> +
              ref<TOK_RPAREN>)> {};

  struct ValueRule
      : rule<ref<ValueNonSqlRule> |
             (ref<TOK_LPAREN> + ref<SqlFetchAsObjectActionRule> +
              ref<TOK_RPAREN>)> {};

  // ---- Built-in functions ----------------------------------------------
  struct EveryBti
      : rule<create<ast::Call>(assign<&ast::Call::name>(ref<TOK_EVERY>) +
                               append<&ast::Call::args>(ref<RclassRule>))> {
  };
  struct GetBti
      : rule<create<ast::Call>(
            assign<&ast::Call::name>(ref<TOK_GET>) +
            append<&ast::Call::args>(ref<ValueRule>) + ref<TOK_HAT> +
            append<&ast::Call::args>(ref<AttributeLiteralRule>))> {};
  struct NthBti
      : rule<create<ast::Call>(assign<&ast::Call::name>(ref<TOK_NTH>) +
                               append<&ast::Call::args>(ref<ValueRule>) +
                               append<&ast::Call::args>(ref<IndexNumRule>))> {
  };
  struct PositionBti
      : rule<create<ast::Call>(
            assign<&ast::Call::name>(ref<TOK_POSITION>) +
            append<&ast::Call::args>(ref<ValueRule>) +
            option(append<&ast::Call::args>(create<ast::Literal>(
                assign<&ast::Literal::value>(
                    ref<TOK_SAME_TYPE> | ref<TOK_DIFF_TYPE> | ref<TOK_EQUAL> |
                    ref<TOK_EQUAL_EQUAL> | ref<TOK_APPROX_EQUAL> |
                    ref<TOK_NOT_APPROX_EQUAL> | ref<TOK_NOT_EQ> |
                    ref<TOK_NOT_EQUAL> | ref<TOK_GREATER> |
                    ref<TOK_GREATER_EQUAL> | ref<TOK_LESS> |
                    ref<TOK_LESS_EQUAL>)))) +
            append<&ast::Call::args>(ref<ValueRule>))> {};
  struct SubcompoundBti
      : rule<create<ast::Call>(assign<&ast::Call::name>(ref<TOK_SUBCOMPOUND>) +
                               append<&ast::Call::args>(ref<ValueRule>) +
                               append<&ast::Call::args>(ref<IndexNumRule>) +
                               append<&ast::Call::args>(ref<IndexNumRule>))> {
  };
  struct SubsymbolBti
      : rule<create<ast::Call>(assign<&ast::Call::name>(ref<TOK_SUBSYMBOL>) +
                               append<&ast::Call::args>(ref<ValueRule>) +
                               append<&ast::Call::args>(ref<IndexNumRule>) +
                               append<&ast::Call::args>(ref<IndexNumRule>))> {
  };
  template <auto Keyword>
  struct BuiltinCall : rule<create<ast::Call>(
                           assign<&ast::Call::name>(Keyword) +
                           many(append<&ast::Call::args>(ref<ValueRule>)))> {};
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

  struct BuiltInsRule
      : rule<ref<SimpleBuiltin> | ref<EveryBti> | ref<GetBti> | ref<NthBti> |
             ref<PositionBti> | ref<SubcompoundBti> | ref<SubsymbolBti>> {};

  struct FunctionCallRule
      : rule<create<ast::Call>(
            assign<&ast::Call::name>(AllowableFuncName) +
            many(append<&ast::Call::args>(ref<ValueRule>)))> {};

  // ---- Arithmetic and boolean expressions ------------------------------
  struct ArithAdd : rule<ref<TOK_PLUS>, value<ast::ArithOp::add>> {};
  struct ArithSub : rule<ref<TOK_MINUS>, value<ast::ArithOp::sub>> {};
  struct ArithMul : rule<ref<TOK_TIMES>, value<ast::ArithOp::mul>> {};
  struct ArithDiv : rule<ref<TOK_DIVIDE>, value<ast::ArithOp::div>> {};
  struct ArithMod : rule<ref<TOK_MODULUS>, value<ast::ArithOp::mod>> {};

  struct ArithUnaryMinus
      : rule<ref<TOK_MINUS> +
             create<ast::ArithExpr>(
                 assign<&ast::ArithExpr::right>(ref<ArithValueRule>))> {};

  struct ArithValueRule
      : rule<ref<ArithUnaryMinus> |
             create<ast::Literal>(
                 assign<&ast::Literal::value>(ref<TOK_INTEGER_CONST>)) |
             create<ast::Literal>(
                 assign<&ast::Literal::value>(ref<TOK_FLOAT_CONST>)) |
             ref<VariableRule> |
             (ref<TOK_LPAREN> + ref<BuiltInsRule> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<FunctionCallRule> + ref<TOK_RPAREN>) |
             (ref<TOK_LPAREN> + ref<ArithExprRule> + ref<TOK_RPAREN>)> {};

  struct ArithExprRule
      : infix<ArithValueRule, &ast::ArithExpr::left, &ast::ArithExpr::op,
              &ast::ArithExpr::right,
              (left(ref<ArithMul> | ref<ArithDiv> | ref<ArithMod>) >>
               left(ref<ArithAdd> | ref<ArithSub>))> {};

  struct ScalarPredicateRule
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
  struct PredicateRule
      : rule<(ref<TOK_CONTAINS> + ref<ScalarPredicateRule>) |
             (ref<TOK_DOES_NOT_CONTAIN> + ref<ScalarPredicateRule>) |
             (ref<ScalarPredicateRule> + ref<TOK_CONTAINS>) |
             (ref<ScalarPredicateRule> + ref<TOK_DOES_NOT_CONTAIN>) |
             ref<TOK_LENGTH_EQUAL> | ref<TOK_LENGTH_NOT_EQUAL> |
             ref<TOK_LENGTH_GREATER> | ref<TOK_LENGTH_GREATER_EQUAL> |
             ref<TOK_LENGTH_LESS> | ref<TOK_LENGTH_LESS_EQUAL> |
             ref<TOK_CONTAINS> | ref<TOK_DOES_NOT_CONTAIN> |
             ref<ScalarPredicateRule>> {};

  static constexpr auto PredicateText = ref<PredicateRule>;

  struct NotRule
      : rule<ref<TOK_NOT> +
             create<ast::Not>(
                 assign<&ast::Not::operand>(ref<ExpreValueRule>))> {};

  struct CompareRule
      : rule<create<ast::Compare>(
            assign<&ast::Compare::left>(ref<ValueNonSqlRule>) +
            assign<&ast::Compare::predicateText>(PredicateText) +
            assign<&ast::Compare::right>(ref<ValueNonSqlRule>))> {};

  struct ExpreValueRule
      : rule<ref<NotRule> | ref<CompareRule> |
             (ref<TOK_LPAREN> + ref<CmplxExprRule> + ref<TOK_RPAREN>)> {};

  struct LogicalAndRule
      : rule<ref<TOK_AND>, value<ast::LogicalOp::logical_and>> {};
  struct LogicalOrRule
      : rule<ref<TOK_OR>, value<ast::LogicalOp::logical_or>> {};

  struct CmplxExprRule
      : infix<ExpreValueRule, &ast::Logical::left, &ast::Logical::op,
              &ast::Logical::right,
              (left(ref<LogicalAndRule>) >> left(ref<LogicalOrRule>))> {};

  struct ExpressionRule
      : rule<ref<TOK_LPAREN> + ref<CmplxExprRule> + ref<TOK_RPAREN>> {};

  // ---- LHS: conditions -------------------------------------------------
  struct CaIndexRule
      : rule<ref<ArithExprRule> |
             create<ast::Literal>(
                 assign<&ast::Literal::value>(ref<TOK_DOLLAR_LAST>))> {};

  struct RestrictionRule
      : rule<create<ast::Restriction>(
                 ref<TOK_START_DISJUNCTION> +
                 some(append<&ast::Restriction::values>(ref<ValueRule>)) +
                 ref<TOK_END_DISJUNCTION>) |
             create<ast::Restriction>(
                 assign<&ast::Restriction::predicateText>(PredicateText) +
                 append<&ast::Restriction::values>(ref<ValueRule>)) |
             create<ast::Restriction>(
                 append<&ast::Restriction::values>(ref<ValueRule>))> {};

  struct LhsTermRule
      : rule<create<ast::LhsTerm>(
            ref<TOK_HAT> + assign<&ast::LhsTerm::attribute>(AttributeText) +
            option(ref<TOK_LBRACKET> +
                   assign<&ast::LhsTerm::index>(ref<CaIndexRule>) +
                   ref<TOK_RBRACKET>) +
            (append<&ast::LhsTerm::restrictions>(ref<RestrictionRule>) |
             (ref<TOK_LBRACE> +
              some(append<&ast::LhsTerm::restrictions>(ref<RestrictionRule>)) +
              ref<TOK_RBRACE>)))> {};

  struct CeFormRule
      : rule<create<ast::CeForm>(
            ref<TOK_LPAREN> + assign<&ast::CeForm::className>(Name) +
            many(append<&ast::CeForm::terms>(ref<LhsTermRule>)) +
            (ref<TOK_RPAREN> |
             flag<&ast::CeForm::openEnded>(ref<TOK_ARROW>)))> {};

  struct CeRule
      : rule<create<ast::Ce>(append<&ast::Ce::alternatives>(ref<CeFormRule>)) |
             create<ast::Ce>(ref<TOK_START_DISJUNCTION> +
                             some(append<&ast::Ce::alternatives>(
                                 ref<CeFormRule>)) +
                             ref<TOK_END_DISJUNCTION>) |
             create<ast::Ce>(
                 flag<&ast::Ce::negated>(ref<TOK_MINUS>) +
                 append<&ast::Ce::alternatives>(ref<CeFormRule>))> {};

  // ---- RHS: actions ------------------------------------------------------
  struct RhsTermRule
      : rule<create<ast::RhsTerm>(
            ref<TOK_HAT> + assign<&ast::RhsTerm::attribute>(AttributeText) +
            option(ref<TOK_LBRACKET> +
                   assign<&ast::RhsTerm::index>(ref<CaIndexRule>) +
                   ref<TOK_RBRACKET>) +
            assign<&ast::RhsTerm::value>(ref<ValueRule>))> {};

  struct KindAddState
      : rule<ref<TOK_ADDSTATE>, value<ast::ActionKind::add_state>> {};
  struct KindAfter : rule<ref<TOK_AFTER>, value<ast::ActionKind::after>> {};
  struct KindAt : rule<ref<TOK_AT>, value<ast::ActionKind::at>> {};
  struct KindBind : rule<ref<TOK_BIND>, value<ast::ActionKind::bind>> {};
  struct KindCallInherited
      : rule<ref<TOK_CALL_INHERITED>, value<ast::ActionKind::call_inherited>> {
  };
  struct KindCloseFile
      : rule<ref<TOK_CLOSEFILE>, value<ast::ActionKind::close_file>> {};
  struct KindCopy : rule<ref<TOK_COPY>, value<ast::ActionKind::copy>> {};
  struct KindDebug : rule<ref<TOK_DEBUG>, value<ast::ActionKind::debug>> {};
  struct KindDefault
      : rule<ref<TOK_DEFAULT>, value<ast::ActionKind::default_>> {};
  struct KindForEach
      : rule<ref<TOK_FOR_EACH>, value<ast::ActionKind::for_each>> {};
  struct KindIf : rule<ref<TOK_IF>, value<ast::ActionKind::if_>> {};
  struct KindMake : rule<ref<TOK_MAKE>, value<ast::ActionKind::make>> {};
  struct KindModify : rule<ref<TOK_MODIFY>, value<ast::ActionKind::modify>> {};
  struct KindOpenFile
      : rule<ref<TOK_OPENFILE>, value<ast::ActionKind::open_file>> {};
  struct KindQuit : rule<ref<TOK_QUIT>, value<ast::ActionKind::quit>> {};
  struct KindRemove : rule<ref<TOK_REMOVE>, value<ast::ActionKind::remove>> {};
  struct KindRemoveEvery
      : rule<ref<TOK_REMOVE_EVERY>, value<ast::ActionKind::remove_every>> {};
  struct KindRestoreState
      : rule<ref<TOK_RESTORESTATE>, value<ast::ActionKind::restore_state>> {};
  struct KindReturn
      : rule<ref<TOK_RETURN>, value<ast::ActionKind::return_>> {};
  struct KindSaveState
      : rule<ref<TOK_SAVESTATE>, value<ast::ActionKind::save_state>> {};
  struct KindSpecialize
      : rule<ref<TOK_SPECIALIZE>, value<ast::ActionKind::specialize>> {};
  struct KindTrace : rule<ref<TOK_TRACE>, value<ast::ActionKind::trace>> {};
  struct KindWhile : rule<ref<TOK_WHILE>, value<ast::ActionKind::while_>> {};
  struct KindWrite : rule<ref<TOK_WRITE>, value<ast::ActionKind::write>> {};
  struct KindSqlAttach
      : rule<ref<TOK_SQL_ATTACH>, value<ast::ActionKind::sql_attach>> {};
  struct KindSqlCommit
      : rule<ref<TOK_SQL_COMMIT>, value<ast::ActionKind::sql_commit>> {};
  struct KindSqlDelete
      : rule<ref<TOK_SQL_DELETE>, value<ast::ActionKind::sql_delete>> {};
  struct KindSqlDetach
      : rule<ref<TOK_SQL_DETACH>, value<ast::ActionKind::sql_detach>> {};
  struct KindSqlFetchEach
      : rule<ref<TOK_SQL_FETCH_EACH>, value<ast::ActionKind::sql_fetch_each>> {
  };
  struct KindSqlFetchAsObject
      : rule<ref<TOK_SQL_FETCH_AS_OBJECT>,
             value<ast::ActionKind::sql_fetch_as_object>> {};
  struct KindSqlInsert
      : rule<ref<TOK_SQL_INSERT>, value<ast::ActionKind::sql_insert>> {};
  struct KindSqlInsertFromObject
      : rule<ref<TOK_SQL_INSERT_FROM_OBJECT>,
             value<ast::ActionKind::sql_insert_from_object>> {};
  struct KindSqlRollback
      : rule<ref<TOK_SQL_ROLLBACK>, value<ast::ActionKind::sql_rollback>> {};
  struct KindSqlStart
      : rule<ref<TOK_SQL_START>, value<ast::ActionKind::sql_start>> {};
  struct KindSqlUpdate
      : rule<ref<TOK_SQL_UPDATE>, value<ast::ActionKind::sql_update>> {};
  struct KindSqlUpdateFromObject
      : rule<ref<TOK_SQL_UPDATE_FROM_OBJECT>,
             value<ast::ActionKind::sql_update_from_object>> {};

  struct AddStateActionRule
      : rule<create<ast::Action>(assign<&ast::Action::kind>(ref<KindAddState>) +
                                 append<&ast::Action::values>(ref<ValueRule>))> {
  };
  struct AfterActionRule
      : rule<create<ast::Action>(assign<&ast::Action::kind>(ref<KindAfter>) +
                                 append<&ast::Action::values>(ref<ValueRule>) +
                                 assign<&ast::Action::name>(Name))> {};
  struct BindActionRule
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(ref<KindBind>) +
            append<&ast::Action::values>(ref<VariableRule>) +
            append<&ast::Action::values>(ref<ValueRule>))> {};
  struct CallInheritedActionRule
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(ref<KindCallInherited>) +
            some(append<&ast::Action::values>(ref<VariableRule>)))> {};
  struct CloseFileActionRule
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(ref<KindCloseFile>) +
            some(append<&ast::Action::values>(ref<ValueRule>)))> {};
  struct CopyActionRule
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(ref<KindCopy>) +
            append<&ast::Action::values>(ref<VariableRule>) +
            many(append<&ast::Action::terms>(ref<RhsTermRule>)))> {};
  struct DebugActionRule
      : rule<create<ast::Action>(assign<&ast::Action::kind>(ref<KindDebug>))> {
  };
  struct DefaultActionRule
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(ref<KindDefault>) +
            append<&ast::Action::values>(ref<ValueRule>) +
            append<&ast::Action::values>(ref<ValueRule>))> {};
  struct ForEachActionRule
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(ref<KindForEach>) +
            append<&ast::Action::values>(ref<VariableRule>) + ref<TOK_IN> +
            append<&ast::Action::values>(ref<ValueRule>) +
            some(append<&ast::Action::body>(ref<ActionLineRule>)))> {};
  struct IfActionRule
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(ref<KindIf>) +
            assign<&ast::Action::condition>(ref<ExpressionRule>) +
            ref<TOK_THEN> +
            some(append<&ast::Action::body>(ref<ActionLineRule>)) +
            option(ref<TOK_ELSE> +
                   some(append<&ast::Action::elseBody>(ref<ActionLineRule>))))> {
  };
  struct MakeActionRule
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(ref<KindMake>) +
            append<&ast::Action::values>(ref<RclassRule>) +
            many(append<&ast::Action::terms>(ref<RhsTermRule>)))> {};
  struct ModifyActionRule
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(ref<KindModify>) +
            append<&ast::Action::values>(ref<VariableRule>) +
            many(append<&ast::Action::terms>(ref<RhsTermRule>)))> {};
  struct OpenFileActionRule
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(ref<KindOpenFile>) +
            append<&ast::Action::values>(ref<ValueRule>) +
            append<&ast::Action::values>(ref<ValueRule>) +
            append<&ast::Action::values>(ref<ValueRule>))> {};
  struct QuitActionRule
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(ref<KindQuit>) +
            option(append<&ast::Action::values>(
                ref<ArithValueRule> |
                create<ast::Literal>(
                    assign<&ast::Literal::value>(ref<TOK_FAILURE>)) |
                create<ast::Literal>(
                    assign<&ast::Literal::value>(ref<TOK_SUCCESS>)))))> {};
  struct RemoveActionRule
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(ref<KindRemove>) +
            some(append<&ast::Action::values>(ref<VariableRule>)))> {};
  struct RemoveEveryActionRule
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(ref<KindRemoveEvery>) +
            some(append<&ast::Action::values>(ref<RclassRule>)))> {};
  struct RestoreStateActionRule
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(ref<KindRestoreState>) +
            append<&ast::Action::values>(ref<ValueRule>))> {};
  struct ReturnActionRule
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(ref<KindReturn>) +
            option(append<&ast::Action::values>(ref<ValueRule>)))> {};
  struct SaveStateActionRule
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(ref<KindSaveState>) +
            append<&ast::Action::values>(ref<ValueRule>))> {};
  struct SpecializeActionRule
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(ref<KindSpecialize>) +
            append<&ast::Action::values>(ref<VariableRule>) +
            append<&ast::Action::values>(ref<RclassRule>) +
            many(append<&ast::Action::terms>(ref<RhsTermRule>)))> {};
  struct TraceActionRule
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(ref<KindTrace>) +
            assign<&ast::Action::name>(ref<TOK_ON> | ref<TOK_OFF>) +
            (append<&ast::Action::values>(create<ast::Literal>(
                 assign<&ast::Literal::value>(ref<TOK_TIMES>))) |
             some(append<&ast::Action::values>(create<ast::Literal>(
                 assign<&ast::Literal::value>(
                     ref<TOK_ENTRY_BLOCK> | ref<TOK_EB> |
                     ref<TOK_RULE_GROUP> | ref<TOK_RG> | ref<TOK_RULE> |
                     ref<TOK_WM> | ref<TOK_CS>))))))> {};
  struct WhileActionRule
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(ref<KindWhile>) +
            assign<&ast::Action::condition>(ref<ExpressionRule>) +
            ref<TOK_DO> +
            some(append<&ast::Action::body>(ref<ActionLineRule>)))> {};
  struct WriteActionRule
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(ref<KindWrite>) +
            many(append<&ast::Action::values>(ref<ValueRule>)))> {};

  static constexpr auto SqlRseElement =
      ref<ConstantRule> | ref<VariableRule> | ref<QuotedVarRule> |
      create<ast::Literal>(assign<&ast::Literal::value>(
          ref<TOK_EQUAL> | ref<TOK_EQUAL_EQUAL> | ref<TOK_APPROX_EQUAL> |
          ref<TOK_NOT_APPROX_EQUAL> | ref<TOK_NOT_EQ> | ref<TOK_NOT_EQUAL> |
          ref<TOK_GREATER> | ref<TOK_GREATER_EQUAL> | ref<TOK_LESS> |
          ref<TOK_LESS_EQUAL> | ref<TOK_PLUS> | ref<TOK_MINUS> |
          ref<TOK_TIMES> | ref<TOK_DIVIDE> | ref<TOK_MODULUS> |
          ref<TOK_ARROW> | ref<TOK_START_DISJUNCTION> |
          ref<TOK_END_DISJUNCTION>));

  struct SqlAttachActionRule
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(ref<KindSqlAttach>) +
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
  struct SqlCommitActionRule
      : rule<create<ast::Action>(assign<&ast::Action::kind>(
            ref<KindSqlCommit>))> {};
  struct SqlDetachActionRule
      : rule<create<ast::Action>(assign<&ast::Action::kind>(
            ref<KindSqlDetach>))> {};
  struct SqlRollbackActionRule
      : rule<create<ast::Action>(assign<&ast::Action::kind>(
            ref<KindSqlRollback>))> {};
  struct SqlDeleteActionRule
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(ref<KindSqlDelete>) +
            append<&ast::Action::values>(
                create<ast::Literal>(assign<&ast::Literal::value>(Name))) +
            many(append<&ast::Action::values>(SqlRseElement)))> {};
  struct SqlFetchEachActionRule
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(ref<KindSqlFetchEach>) +
            some(append<&ast::Action::values>(ref<VariableRule>)) +
            ref<TOK_LPAREN> +
            some(append<&ast::Action::values>(SqlRseElement)) +
            ref<TOK_RPAREN> +
            some(append<&ast::Action::body>(ref<ActionLineRule>)))> {};
  struct SqlFetchAsObjectActionRule
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(ref<KindSqlFetchAsObject>) +
            some(append<&ast::Action::values>(SqlRseElement)))> {};
  struct SqlInsertActionRule
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(ref<KindSqlInsert>) +
            append<&ast::Action::values>(
                create<ast::Literal>(assign<&ast::Literal::value>(Name))) +
            some(append<&ast::Action::values>(SqlRseElement)))> {};
  struct SqlInsertFromObjectActionRule
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(ref<KindSqlInsertFromObject>) +
            append<&ast::Action::values>(ref<VariableRule>))> {};
  struct SqlStartActionRule
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(ref<KindSqlStart>) +
            many(append<&ast::Action::values>(SqlRseElement)))> {};
  struct SqlUpdateActionRule
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(ref<KindSqlUpdate>) +
            append<&ast::Action::values>(
                create<ast::Literal>(assign<&ast::Literal::value>(Name))) +
            some(append<&ast::Action::values>(SqlRseElement)))> {};
  struct SqlUpdateFromObjectActionRule
      : rule<create<ast::Action>(
            assign<&ast::Action::kind>(ref<KindSqlUpdateFromObject>) +
            append<&ast::Action::values>(ref<VariableRule>) +
            many(append<&ast::Action::values>(SqlRseElement)))> {};

  struct ActionRule
      : rule<ref<AddStateActionRule> | ref<AfterActionRule> |
             ref<BindActionRule> | ref<CallInheritedActionRule> |
             ref<CloseFileActionRule> | ref<CopyActionRule> |
             ref<DebugActionRule> | ref<DefaultActionRule> |
             ref<ForEachActionRule> | ref<FunctionCallRule> |
             ref<IfActionRule> | ref<MakeActionRule> | ref<ModifyActionRule> |
             ref<OpenFileActionRule> | ref<QuitActionRule> |
             ref<RemoveActionRule> | ref<RemoveEveryActionRule> |
             ref<RestoreStateActionRule> | ref<ReturnActionRule> |
             ref<SaveStateActionRule> | ref<SpecializeActionRule> |
             ref<TraceActionRule> | ref<WhileActionRule> |
             ref<WriteActionRule> | ref<SqlAttachActionRule> |
             ref<SqlCommitActionRule> | ref<SqlDeleteActionRule> |
             ref<SqlDetachActionRule> | ref<SqlFetchEachActionRule> |
             ref<SqlFetchAsObjectActionRule> | ref<SqlInsertActionRule> |
             ref<SqlInsertFromObjectActionRule> | ref<SqlRollbackActionRule> |
             ref<SqlStartActionRule> | ref<SqlUpdateActionRule> |
             ref<SqlUpdateFromObjectActionRule>> {};

  // No `"(" construct_keyword` half-open alternative here: the LALR states
  // already stop this loop on any token that isn't a legal action opener.
  struct ActionLineRule
      : rule<ref<TOK_LPAREN> + ref<ActionRule> + ref<TOK_RPAREN>> {};

  struct AtCommandRule
      : rule<create<ast::Action>(assign<&ast::Action::kind>(ref<KindAt>) +
                                 assign<&ast::Action::name>(Name))> {};
  struct StartupActionRule
      : rule<ref<ActionRule> | ref<AtCommandRule>> {};
  struct StartupActionLineRule
      : rule<ref<TOK_LPAREN> + ref<StartupActionRule> + ref<TOK_RPAREN>> {};

  // ---- Top-level constructs ---------------------------------------------
  struct RuleRule
      : rule<create<ast::Rule>(
            assign<&ast::Rule::keyword>(ref<TOK_RULE> | ref<TOK_P>) +
            assign<&ast::Rule::name>(Name) +
            append<&ast::Rule::conditions>(create<ast::Ce>(
                append<&ast::Ce::alternatives>(ref<CeFormRule>))) +
            many(append<&ast::Rule::conditions>(ref<CeRule>)) +
            ref<TOK_ARROW> +
            many(append<&ast::Rule::actions>(ref<ActionLineRule>)))> {};

  struct CatcherRule
      : rule<create<ast::Catcher>(
            ref<TOK_CATCH> + assign<&ast::Catcher::name>(Name) +
            many(append<&ast::Catcher::actions>(ref<ActionLineRule>)))> {};

  struct OnEntryRule
      : rule<create<ast::OnEntry>(
            (ref<TOK_ON_ENTRY> | ref<TOK_STARTUP>) +
            some(append<&ast::OnEntry::actions>(ref<StartupActionLineRule>)))> {
  };
  struct OnEmptyRule
      : rule<create<ast::OnEmpty>(
            ref<TOK_ON_EMPTY> +
            some(append<&ast::OnEmpty::actions>(ref<ActionLineRule>)))> {};
  struct OnExitRule
      : rule<create<ast::OnExit>(
            ref<TOK_ON_EXIT> +
            some(append<&ast::OnExit::actions>(ref<ActionLineRule>)))> {};
  struct OnEveryRule
      : rule<create<ast::OnEvery>(
            ref<TOK_ON_EVERY> +
            some(append<&ast::OnEvery::actions>(ref<ActionLineRule>)))> {};

  struct DeclarationBlockRule
      : rule<create<ast::DeclarationBlock>(
            ref<TOK_DECLARATION_BLOCK> +
            assign<&ast::DeclarationBlock::name>(Name))> {};
  struct RuleGroupDeclRule
      : rule<create<ast::RuleGroupDecl>(
            ref<TOK_RULE_GROUP> + assign<&ast::RuleGroupDecl::name>(Name))> {
  };
  struct EndRuleGroupRule
      : rule<create<ast::EndRuleGroup>(
            ref<TOK_END_GROUP> +
            option(assign<&ast::EndRuleGroup::name>(Name)))> {};
  struct EndBlockRule
      : rule<create<ast::EndBlock>(
            ref<TOK_END_BLOCK> + option(assign<&ast::EndBlock::name>(Name)))> {
  };

  struct ExtRtParamDeclRule
      : rule<create<ast::ParamDecl>(
            option(assign<&ast::ParamDecl::name>(ref<TOK_VARIABLE>)) +
            option(ref<TOK_LBRACKET> +
                   option(assign<&ast::ParamDecl::arrayLength>(
                       create<ast::Literal>(assign<&ast::Literal::value>(
                           ref<TOK_INTEGER_CONST>)) |
                       ref<VariableRule>)) +
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

  struct ExtRtReturnDeclRule
      : rule<create<ast::ParamDecl>(
            option(assign<&ast::ParamDecl::name>(ref<TOK_VARIABLE>)) +
            option(ref<TOK_LBRACKET> +
                   option(assign<&ast::ParamDecl::arrayLength>(
                       create<ast::Literal>(assign<&ast::Literal::value>(
                           ref<TOK_INTEGER_CONST>)) |
                       ref<VariableRule>)) +
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
        some(append<&ast::EntryBlock::accepts>(ref<ExtRtParamDeclRule>))) |
       (ref<TOK_ACTIVATES> +
        some(append<&ast::EntryBlock::activates>(Name))) |
       (ref<TOK_USES> + some(append<&ast::EntryBlock::uses>(Name)))) +
      ref<TOK_RPAREN>;
  static constexpr auto EntryBlockReturns =
      ref<TOK_LPAREN> + ref<TOK_RETURNS> +
      assign<&ast::EntryBlock::returns>(ref<ExtRtReturnDeclRule>) +
      ref<TOK_RPAREN>;
  static constexpr auto EntryBlockStrategy =
      ref<TOK_LPAREN> + ref<TOK_STRATEGY> +
      assign<&ast::EntryBlock::strategy>(ref<TOK_LEX> | ref<TOK_MEA>) +
      ref<TOK_RPAREN>;

  struct EntryBlockRule
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

  struct RuleBlockRule
      : rule<create<ast::RuleBlock>(
            ref<TOK_RULE_BLOCK> + assign<&ast::RuleBlock::name>(Name) +
            many(RuleBlockUses) +
            option(ref<TOK_LPAREN> + ref<TOK_STRATEGY> +
                   assign<&ast::RuleBlock::strategy>(ref<TOK_LEX> |
                                                      ref<TOK_MEA>) +
                   ref<TOK_RPAREN> + many(RuleBlockUses)))> {};

  struct CompoundConstRule
      : rule<create<ast::Call>(
            ref<TOK_LPAREN> + assign<&ast::Call::name>(ref<TOK_COMPOUND>) +
            many(append<&ast::Call::args>(ref<ConstantRule>)) +
            ref<TOK_RPAREN>)> {};

  struct AttributeModifierRule
      : rule<create<ast::AttributeModifier>(
                 ref<TOK_LPAREN> +
                 assign<&ast::AttributeModifier::kind>(ref<TOK_DEFAULT>) +
                 assign<&ast::AttributeModifier::value>(ref<ConstantRule>) +
                 ref<TOK_RPAREN>) |
             create<ast::AttributeModifier>(
                 ref<TOK_LPAREN> +
                 assign<&ast::AttributeModifier::kind>(ref<TOK_DEFAULT>) +
                 assign<&ast::AttributeModifier::value>(
                     ref<CompoundConstRule>) +
                 ref<TOK_RPAREN>) |
             create<ast::AttributeModifier>(
                 ref<TOK_LPAREN> +
                 assign<&ast::AttributeModifier::kind>(ref<TOK_FILL>) +
                 assign<&ast::AttributeModifier::value>(ref<ConstantRule>) +
                 ref<TOK_RPAREN>)> {};

  struct AttributeDeclRule
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
                ref<AttributeModifierRule>)))> {};

  struct ObjectClassDeclRule
      : rule<create<ast::ObjectClassDecl>(
            ref<TOK_OBJECT_CLASS> +
            assign<&ast::ObjectClassDecl::name>(Name) +
            option(ref<TOK_LPAREN> + ref<TOK_INHERITS_FROM> +
                   assign<&ast::ObjectClassDecl::inheritsFrom>(Name) +
                   ref<TOK_RPAREN>) +
            many(append<&ast::ObjectClassDecl::attributes>(
                ref<AttributeDeclRule>)))> {};

  struct MethodParamDeclRule
      : rule<create<ast::ParamDecl>(
            assign<&ast::ParamDecl::name>(ref<TOK_VARIABLE>) +
            option(flag<&ast::ParamDecl::compound>(ref<TOK_COMPOUND>)) +
            option(assign<&ast::ParamDecl::type>(
                ref<TOK_ANY> | ref<TOK_INSTANCE> | ref<TOK_INTEGER> |
                ref<TOK_FLOAT> | ref<TOK_OPAQUE> | ref<TOK_SYMBOL>)))> {};

  struct MethodReturnParamRule
      : rule<create<ast::ParamDecl>(
            assign<&ast::ParamDecl::name>(ref<TOK_VARIABLE>) +
            option(flag<&ast::ParamDecl::compound>(ref<TOK_COMPOUND>)) +
            option(assign<&ast::ParamDecl::type>(
                ref<TOK_ANY> | ref<TOK_INSTANCE> | ref<TOK_INTEGER> |
                ref<TOK_FLOAT> | ref<TOK_OPAQUE> | ref<TOK_SYMBOL>)))> {};

  struct MethodDeclRule
      : rule<create<ast::MethodDecl>(
            ref<TOK_METHOD> + assign<&ast::MethodDecl::name>(AllowableFuncName) +
            ref<TOK_LPAREN> + ref<TOK_ACCEPTS> +
            assign<&ast::MethodDecl::whenVar>(ref<VariableRule>) +
            ref<TOK_WHEN> + ref<TOK_INSTANCE> + ref<TOK_OF> +
            assign<&ast::MethodDecl::whenClassName>(Name) +
            many(append<&ast::MethodDecl::params>(ref<MethodParamDeclRule>)) +
            ref<TOK_RPAREN> +
            (some(append<&ast::MethodDecl::actions>(ref<ActionLineRule>)) |
             (ref<TOK_LPAREN> + ref<TOK_RETURNS> +
              assign<&ast::MethodDecl::returns>(ref<MethodReturnParamRule>) +
              ref<TOK_RPAREN> +
              many(append<&ast::MethodDecl::actions>(
                  ref<ActionLineRule>)))))> {};

  struct GenericMethodDeclRule
      : rule<create<ast::GenericMethodDecl>(
            ref<TOK_GENERIC_METHOD> +
            assign<&ast::GenericMethodDecl::name>(AllowableFuncName) +
            ref<TOK_LPAREN> + ref<TOK_ACCEPTS> +
            assign<&ast::GenericMethodDecl::whenVar>(ref<VariableRule>) +
            ref<TOK_WHEN> + ref<TOK_INSTANCE> +
            option(ref<TOK_OF> +
                   assign<&ast::GenericMethodDecl::whenClassName>(Name)) +
            many(append<&ast::GenericMethodDecl::params>(
                ref<MethodParamDeclRule>)) +
            ref<TOK_RPAREN> +
            option(ref<TOK_LPAREN> + ref<TOK_RETURNS> +
                   assign<&ast::GenericMethodDecl::returns>(
                       ref<MethodReturnParamRule>) +
                   ref<TOK_RPAREN>))> {};

  static constexpr auto ExtRtAccepts =
      ref<TOK_LPAREN> + ref<TOK_ACCEPTS> +
      some(append<&ast::ExternalRoutineDecl::accepts>(
          ref<ExtRtParamDeclRule>)) +
      ref<TOK_RPAREN>;
  static constexpr auto ExtRtAlias =
      ref<TOK_LPAREN> + ref<TOK_ALIAS_FOR> +
      assign<&ast::ExternalRoutineDecl::alias>(Name) + ref<TOK_RPAREN>;
  static constexpr auto ExtRtReturns =
      ref<TOK_LPAREN> + ref<TOK_RETURNS> +
      assign<&ast::ExternalRoutineDecl::returns>(ref<ExtRtReturnDeclRule>) +
      ref<TOK_RPAREN>;

  struct ExternalRoutineDeclRule
      : rule<create<ast::ExternalRoutineDecl>(
            ref<TOK_EXTERNAL_ROUTINE> +
            assign<&ast::ExternalRoutineDecl::name>(AllowableFuncName) +
            many(ExtRtAccepts) +
            option((ExtRtAlias + many(ExtRtAccepts) +
                    option(ExtRtReturns + many(ExtRtAccepts))) |
                   (ExtRtReturns + many(ExtRtAccepts) +
                    option(ExtRtAlias + many(ExtRtAccepts)))))> {};

  static constexpr auto ProgramLine =
      ref<DeclarationBlockRule> | ref<EntryBlockRule> | ref<RuleBlockRule> |
      ref<EndBlockRule> | ref<RuleGroupDeclRule> | ref<EndRuleGroupRule> |
      ref<ObjectClassDeclRule> | ref<MethodDeclRule> |
      ref<GenericMethodDeclRule> | ref<ExternalRoutineDeclRule> |
      ref<OnEntryRule> | ref<OnEmptyRule> | ref<OnExitRule> |
      ref<OnEveryRule> | ref<RuleRule> | ref<CatcherRule>;

  struct ProgramConstructRule
      : rule<create<ast::ProgramConstruct>(
            ref<TOK_LPAREN> +
            some(append<&ast::ProgramConstruct::lines>(ProgramLine)) +
            ref<TOK_RPAREN>)> {};

  struct ProgramRule
      : rule<create<ast::Program>(
            some(append<&ast::Program::constructs>(ref<ProgramConstructRule>)))> {
  };

  using entry = ProgramRule;
  using ignore = WS;
  using hidden = list<TrailingComment>;
};

}  // namespace ruleworks
