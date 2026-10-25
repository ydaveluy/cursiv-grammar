#pragma once
#include "abb/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace abb {
inline constexpr auto unquote = [](std::string_view text) {
  return text.substr(1, text.size() - 2);
};
using Unquoted = cursiv::text_conv<unquote>;
struct Abb : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct LINE_COMMENT : terminal<lit<"!"> + many(cr<R"(^\n\r)">)> {};
  struct IDENTIFIER : terminal<cr<"A-Za-z_"> + many(cr<"A-Za-z0-9_">)> {};
  static constexpr auto control =
      ikw<"MODULE"> | ikw<"ENDMODULE"> | ikw<"PROC"> | ikw<"ENDPROC"> |
      ikw<"FUNC"> | ikw<"ENDFUNC"> | ikw<"TRAP"> | ikw<"ENDTRAP"> |
      ikw<"RECORD"> | ikw<"ENDRECORD"> | ikw<"VAR"> | ikw<"PERS"> |
      ikw<"CONST"> | ikw<"LOCAL"> | ikw<"TASK"> | ikw<"IF"> | ikw<"THEN"> |
      ikw<"ELSEIF"> | ikw<"ELSE"> | ikw<"ENDIF"> | ikw<"WHILE"> | ikw<"DO"> |
      ikw<"ENDWHILE"> | ikw<"FOR"> | ikw<"ENDFOR"> | ikw<"TEST"> | ikw<"CASE"> |
      ikw<"DEFAULT"> | ikw<"ENDTEST"> | ikw<"RETURN"> | ikw<"CONNECT"> |
      ikw<"ERROR"> | ikw<"UNDO"> | ikw<"BACKWARD">;
  struct CALL_NAME : terminal<!control + ref<IDENTIFIER>> {};
  struct BOOLLITERAL
      : terminal<
            ikw<"TRUE"> | ikw<"FALSE">,
            text_conv<[](std::string_view text) { return text.size() == 4; }>> {
  };
  static constexpr auto escape = lit<"\\"> + any;
  struct CHARLITERAL
      : terminal<lit<"'"> + (escape | cr<R"(^'\\\r\n)">)+lit<"'">, Unquoted> {};
  struct STRINGLITERAL
      : terminal<lit<"\""> + many(escape | cr<R"(^"\\\r\n)">) + lit<"\"">,
                 Unquoted> {};
  static constexpr auto exponent = cr<"eE"> + option(cr<"+-">) + some(d);
  static constexpr auto sign = option(cr<"+-">);
  struct FLOATLITERAL
      : terminal<sign + ((some(d) + lit<"."> + many(d) + option(exponent)) |
                         (lit<"."> + some(d) + option(exponent)) |
                         (some(d) + exponent))> {};
  struct INTLITERAL
      : terminal<sign +
                 (some(d) |
                  (lit<"'"> + cr<"hH"> + some(cr<"0-9A-Fa-f">) + lit<"'">) |
                  (lit<"'"> + cr<"bB"> + some(cr<"01">) + lit<"'">))> {};
  struct IntegerRule : rule<assign<&ast::Integer::text>(ref<INTLITERAL>)> {};
  struct FloatRule : rule<assign<&ast::FloatLiteral::text>(ref<FLOATLITERAL>)> {
  };
  struct ReferenceRule : rule<assign<&ast::Reference::name>(ref<IDENTIFIER>)> {
  };
  struct ArrayRule;
  static constexpr auto primitive = ref<BOOLLITERAL> | ref<CHARLITERAL> |
                                    ref<STRINGLITERAL> | ref<FloatRule> |
                                    ref<IntegerRule>;
  static constexpr auto expression =
      ref<ArrayRule> | primitive | ref<ReferenceRule>;
  struct ArrayRule
      : rule<lit<"["> + many(append<&ast::Array::items>(expression), lit<",">) +
             lit<"]">> {};
  struct Wildcard : rule<assign<&ast::Reference::name>(lit<"*">)> {};
  struct Dimensions
      : rule<create<ast::Array>(
            lit<"{"> +
            many(append<&ast::Array::items>(expression | ref<Wildcard>),
                 lit<",">) +
            lit<"}">)> {};
  struct ConstKind : terminal<ikw<"CONST">, value<ast::InitKind::Const>> {};
  struct PersKind : terminal<ikw<"PERS">, value<ast::InitKind::Pers>> {};
  struct VarKind : terminal<ikw<"VAR">, value<ast::InitKind::Var>> {};
  static constexpr auto init_kind =
      ref<ConstKind> | ref<PersKind> | ref<VarKind>;
  struct DeclarationRule
      : rule<option(flag<&ast::Declaration::local>(ikw<"LOCAL">) |
                    flag<&ast::Declaration::task>(ikw<"TASK">)) +
             assign<&ast::Declaration::kind>(init_kind) +
             assign<&ast::Declaration::type>(ref<IDENTIFIER>) +
             assign<&ast::Declaration::name>(ref<IDENTIFIER>) +
             option(assign<&ast::Declaration::dimensions>(ref<Dimensions>)) +
             option(lit<":="> + assign<&ast::Declaration::value>(expression)) +
             lit<";">> {};
  struct OptionalArgument
      : rule<flag<&ast::BindingArgument::isOptional>(lit<"\\">) +
             assign<&ast::BindingArgument::name>(ref<IDENTIFIER>) +
             option(lit<":="> +
                    assign<&ast::BindingArgument::value>(expression))> {};
  struct NamedArgument
      : rule<assign<&ast::BindingArgument::name>(ref<IDENTIFIER>) + lit<":="> +
             assign<&ast::BindingArgument::value>(expression)> {};
  static constexpr auto argument =
      ref<OptionalArgument> | ref<NamedArgument> | expression;
  struct CallRule
      : rule<assign<&ast::Call::name>(ref<CALL_NAME>) +
             option(append<&ast::Call::arguments>(argument)) +
             many((lit<","> + append<&ast::Call::arguments>(argument)) |
                  append<&ast::Call::arguments>(ref<OptionalArgument>)) +
             lit<";">> {};
  struct StatementRule;
  struct AssignmentRule
      : rule<assign<&ast::Assignment::target>(ref<ReferenceRule>) + lit<":="> +
             assign<&ast::Assignment::value>(expression) + lit<";">> {};
  struct ReturnRule
      : rule<ikw<"RETURN"> + option(assign<&ast::Return::value>(expression)) +
             lit<";">> {};
  struct BranchRule
      : rule<assign<&ast::Branch::condition>(expression) + ikw<"THEN"> +
             many(append<&ast::Branch::statements>(ref<StatementRule>))> {};
  struct IfRule
      : rule<ikw<"IF"> + append<&ast::If::branches>(ref<BranchRule>) +
             many(ikw<"ELSEIF"> + append<&ast::If::branches>(ref<BranchRule>)) +
             option(
                 flag<&ast::If::hasElse>(ikw<"ELSE">) +
                 many(append<&ast::If::elseStatements>(ref<StatementRule>))) +
             ikw<"ENDIF">> {};
  struct WhileRule
      : rule<ikw<"WHILE"> + assign<&ast::While::condition>(expression) +
             ikw<"DO"> +
             many(append<&ast::While::statements>(ref<StatementRule>)) +
             ikw<"ENDWHILE">> {};
  struct StatementRule : rule<ref<AssignmentRule> | ref<ReturnRule> |
                              ref<IfRule> | ref<WhileRule> | ref<CallRule>> {};
  struct ParameterRule
      : rule<option(flag<&ast::Parameter::isOptional>(lit<"\\">)) +
             option(assign<&ast::Parameter::mode>(ikw<"VAR"> | ikw<"PERS"> |
                                                  ikw<"INOUT">)) +
             ((assign<&ast::Parameter::type>(ref<IDENTIFIER>) +
               assign<&ast::Parameter::name>(ref<IDENTIFIER>)) |
              assign<&ast::Parameter::name>(ref<IDENTIFIER>)) +
             option(assign<&ast::Parameter::dimensions>(ref<Dimensions>))> {};
  static constexpr auto parameters = option(
      flag<&ast::Routine::parenthesized>(lit<"(">) +
      many(append<&ast::Routine::parameters>(ref<ParameterRule>), lit<",">) +
      lit<")">);
  static constexpr auto body =
      many(append<&ast::Routine::locals>(ref<DeclarationRule>)) +
      many(append<&ast::Routine::statements>(ref<StatementRule>));
  struct ProcedureRule
      : rule<create<ast::Procedure>(
            ikw<"PROC"> + assign<&ast::Routine::name>(ref<IDENTIFIER>) +
            parameters + body + ikw<"ENDPROC">)> {};
  struct FunctionRule
      : rule<create<ast::Function>(
            ikw<"FUNC"> + assign<&ast::Function::returnType>(ref<IDENTIFIER>) +
            assign<&ast::Routine::name>(ref<IDENTIFIER>) + parameters + body +
            ikw<"ENDFUNC">)> {};
  struct RecordFieldRule
      : rule<assign<&ast::RecordField::type>(ref<IDENTIFIER>) +
             assign<&ast::RecordField::name>(ref<IDENTIFIER>) + lit<";">> {};
  struct RecordRule
      : rule<ikw<"RECORD"> + assign<&ast::Record::name>(ref<IDENTIFIER>) +
             many(append<&ast::Record::fields>(ref<RecordFieldRule>)) +
             ikw<"ENDRECORD">> {};
  struct ModuleRule
      : rule<ikw<"MODULE"> + assign<&ast::Module::name>(ref<IDENTIFIER>) +
             option(flag<&ast::Module::parenthesized>(lit<"(">) +
                    many(append<&ast::Module::options>(ref<IDENTIFIER>),
                         lit<",">) +
                    lit<")">) +
             many(append<&ast::Module::records>(ref<RecordRule>) |
                  append<&ast::Module::declarations>(ref<DeclarationRule>) |
                  append<&ast::Module::procedures>(ref<ProcedureRule>) |
                  append<&ast::Module::functions>(ref<FunctionRule>)) +
             ikw<"ENDMODULE">> {};
  using entry = ModuleRule;
  using ignore = WS;
  using hidden = LINE_COMMENT;
};
} // namespace abb
