// The NASM language: instruction names are identifiers, while register
// names are recognized in operand positions. New opcodes need no grammar edit.
#pragma once

#include <charconv>
#include <cstdint>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "nasm/core/ast.hpp"

namespace nasm {

namespace detail {

inline constexpr auto name_text = [](std::string_view text) {
  using Result = cursiv::ConversionResult<std::string_view>;
  if (!text.empty() && text.front() == '$') text.remove_prefix(1);
  if (text.empty()) return Result::err("Incomplete NASM identifier.");
  return Result::ok(text);
};

// Quotes off; the token graph admits no escapes (`~'"'*` / `~'\''*`), so the
// body is exactly what is between the quotes.
inline constexpr auto unquote = [](std::string_view t,
                                   std::pmr::memory_resource*) {
  return t.substr(1, t.size() - 2);
};

}  // namespace detail

struct Nasm : cursiv::GrammarSpec {
  // `.`, `?`, `$`, `#`, `@`, `~` continue a name; a keyword must not run into
  // any of them either, or `mov.foo`/`byte?` would split wrong.
  static constexpr auto word_boundary = cr<"a-zA-Z0-9_$#@~.?">;

  struct WS : terminal<some(cr<R"( \t)">)> {};
  struct EOL : terminal<some(cr<R"(\r\n)">)> {};
  struct COMMENT : terminal<lit<";"> + many(cr<R"(^\r\n)">)> {};

  // The eighteen `%directive` names, each followed by anything up to the
  // newline; entirely discarded, like NASM's own preprocessor pass.
  struct PREPROC
      : terminal<lit<"%"> +
                 (lit<"xdefine"> | lit<"ixdefine"> | lit<"defstr"> |
                  lit<"deftok"> | lit<"defalias"> | lit<"undefalias"> |
                  lit<"undef"> | lit<"define"> | lit<"strcat"> |
                  lit<"strlen"> | lit<"substr"> | lit<"pathsearch"> |
                  lit<"include"> | lit<"depend"> | lit<"assign"> |
                  lit<"iassign"> | lit<"use"> | lit<"line"> | lit<"clear">) +
                 many(cr<R"(^\r\n)">)> {};

  // `%macro ... %endmacro`, verbatim, body included: NASM macro bodies (with
  // `%1`-style parameter references) are never parsed, only invocations are.
  struct MACROBLOCK : terminal<through(lit<"%macro">, lit<"%endmacro">)> {};

  static constexpr auto NameHead = cr<"a-zA-Z._?">;
  static constexpr auto NameTail = cr<"a-zA-Z0-9_$#@~.?">;
  struct NAME : terminal<as<detail::name_text>(
      option(lit<"$">) + NameHead + many(NameTail))> {};

  // `NAME (MINUS NAME)+` -- a hyphenated warning class, e.g. `no-deprecated`.
  struct WARNING_NAME
      : terminal<NameHead + many(NameTail) +
                 some(lit<"-"> + NameHead + many(NameTail))> {};

  struct DECIMAL_INTEGER
      : terminal<(cr<"0-9"> + many(cr<"0-9_">) + option(cr<"dDtT">)) |
                     (cr<"0"> + cr<"dDtT"> + cr<"0-9"> + many(cr<"0-9_">))> {};

  struct OCT_INTEGER
      : terminal<(cr<"0-7"> + many(cr<"0-7_">) + cr<"qQoO">) |
                     (cr<"0"> + cr<"qQoO"> + cr<"0-7"> + many(cr<"0-7_">))> {};

  struct HEX_INTEGER
      : terminal<(d + many(cr<"0-9A-Fa-f_">) + cr<"hHxX">) |
                     (lit<"$"> + d + many(cr<"0-9A-Fa-f_">)) |
                     (cr<"0"> + cr<"xXhH"> + cr<"0-9A-Fa-f"> +
                      many(cr<"0-9A-Fa-f_">))> {};

  struct BIN_INTEGER
      : terminal<(cr<"01"> + many(cr<"01_">) + cr<"bByY">) |
                     (cr<"0"> + cr<"bByY"> + cr<"01"> + many(cr<"01_">))> {};

  static constexpr auto Sign = cr<"+-">;
  static constexpr auto DecimalDigits = d + many(cr<"0-9_">);
  // The pinned native reader treats an empty/underscore-only exponent as zero.
  static constexpr auto DecimalExponent = cr<"eE"> + option(Sign) + many(cr<"0-9_">);
  // A bare final p is a packed-BCD suffix in NASM, not a binary exponent.
  static constexpr auto BinaryExponent = cr<"pP"> +
      ((Sign + many(cr<"0-9_">)) | some(cr<"0-9_">));
  // Preserve the spelling: NASM has binary/octal/hex reals and 80/128-bit
  // formats whose values cannot be represented by a host double.
  static constexpr auto HexMantissa = cr<"0-9A-Fa-f"> + many(cr<"0-9A-Fa-f_">);
  static constexpr auto BinMantissa = cr<"01"> + many(cr<"01_">);
  static constexpr auto OctMantissa = cr<"0-7"> + many(cr<"0-7_">);
  template <auto Digits, auto Head = Digits>
  static constexpr auto BasedFloat =
      Head + ((lit<"."> + many(Digits | lit<"_">) + option(BinaryExponent)) |
                BinaryExponent);
  struct FLOAT_NUMBER
      : terminal<(DecimalDigits +
                  ((lit<"."> + many(cr<"0-9_">) + option(DecimalExponent)) |
                   DecimalExponent)) |
                 (lit<"0"> + cr<"xXhH"> + BasedFloat<HexMantissa>) |
                 (lit<"$"> + BasedFloat<HexMantissa, d + many(cr<"0-9A-Fa-f_">)>) |
                 (lit<"0"> + cr<"bByY"> + BasedFloat<BinMantissa>) |
                 (lit<"0"> + cr<"oOqQ"> + BasedFloat<OctMantissa>)> {};

  struct STRING : terminal<as<detail::unquote>(
                      (lit<"\""> + many(cr<R"(^")">) + lit<"\"">) |
                      (lit<"'"> + many(cr<"^'">) + lit<"'">))> {};

  // Instruction names are open; ISA and macro resolution belong to the runtime.
  static constexpr auto Mnemonic = ref<NAME>;

  static constexpr auto RegisterName =
    ikw<"al"> | ikw<"ah"> | ikw<"ax"> | ikw<"eax"> | ikw<"rax"> | ikw<"bl"> |
    ikw<"bh"> | ikw<"bx"> | ikw<"ebx"> | ikw<"rbx"> | ikw<"cl"> | ikw<"ch"> |
    ikw<"cx"> | ikw<"ecx"> | ikw<"rcx"> | ikw<"dl"> | ikw<"dh"> | ikw<"dx"> |
    ikw<"edx"> | ikw<"rdx"> | ikw<"spl"> | ikw<"sp"> | ikw<"esp"> | ikw<"rsp"> |
    ikw<"bpl"> | ikw<"bp"> | ikw<"ebp"> | ikw<"rbp"> | ikw<"sil"> | ikw<"si"> |
    ikw<"esi"> | ikw<"rsi"> | ikw<"dil"> | ikw<"di"> | ikw<"edi"> | ikw<"rdi"> |
    ikw<"r8b"> | ikw<"r9b"> | ikw<"r10b"> | ikw<"r11b"> | ikw<"r12b"> | ikw<"r13b"> |
    ikw<"r14b"> | ikw<"r15b"> | ikw<"r8w"> | ikw<"r9w"> | ikw<"r10w"> | ikw<"r11w"> |
    ikw<"r12w"> | ikw<"r13w"> | ikw<"r14w"> | ikw<"r15w"> | ikw<"r8d"> | ikw<"r9d"> |
    ikw<"r10d"> | ikw<"r11d"> | ikw<"r12d"> | ikw<"r13d"> | ikw<"r14d"> | ikw<"r15d"> |
    ikw<"r8"> | ikw<"r9"> | ikw<"r10"> | ikw<"r11"> | ikw<"r12"> | ikw<"r13"> |
    ikw<"r14"> | ikw<"r15"> | ikw<"ip"> | ikw<"eip"> | ikw<"rip"> | ikw<"es"> |
    ikw<"cs"> | ikw<"ss"> | ikw<"ds"> | ikw<"fs"> | ikw<"gs"> | ikw<"segr6"> |
    ikw<"segr7"> | ikw<"cr0"> | ikw<"cr1"> | ikw<"cr2"> | ikw<"cr3"> | ikw<"cr4"> |
    ikw<"cr5"> | ikw<"cr6"> | ikw<"cr7"> | ikw<"cr8"> | ikw<"cr9"> | ikw<"cr10"> |
    ikw<"cr11"> | ikw<"cr12"> | ikw<"cr13"> | ikw<"cr14"> | ikw<"cr15"> | ikw<"dr0"> |
    ikw<"dr1"> | ikw<"dr2"> | ikw<"dr3"> | ikw<"dr4"> | ikw<"dr5"> | ikw<"dr6"> |
    ikw<"dr7"> | ikw<"dr8"> | ikw<"dr9"> | ikw<"dr10"> | ikw<"dr11"> | ikw<"dr12"> |
    ikw<"dr13"> | ikw<"dr14"> | ikw<"dr15"> | ikw<"tr0"> | ikw<"tr1"> | ikw<"tr2"> |
    ikw<"tr3"> | ikw<"tr4"> | ikw<"tr5"> | ikw<"tr6"> | ikw<"tr7"> | ikw<"st0"> |
    ikw<"st1"> | ikw<"st2"> | ikw<"st3"> | ikw<"st4"> | ikw<"st5"> | ikw<"st6"> |
    ikw<"st7"> | ikw<"mm0"> | ikw<"mm1"> | ikw<"mm2"> | ikw<"mm3"> | ikw<"mm4"> |
    ikw<"mm5"> | ikw<"mm6"> | ikw<"mm7"> | ikw<"xmm0"> | ikw<"xmm1"> | ikw<"xmm2"> |
    ikw<"xmm3"> | ikw<"xmm4"> | ikw<"xmm5"> | ikw<"xmm6"> | ikw<"xmm7"> | ikw<"xmm8"> |
    ikw<"xmm9"> | ikw<"xmm10"> | ikw<"xmm11"> | ikw<"xmm12"> | ikw<"xmm13"> | ikw<"xmm14"> |
    ikw<"xmm15"> | ikw<"xmm16"> | ikw<"xmm17"> | ikw<"xmm18"> | ikw<"xmm19"> | ikw<"xmm20"> |
    ikw<"xmm21"> | ikw<"xmm22"> | ikw<"xmm23"> | ikw<"xmm24"> | ikw<"xmm25"> | ikw<"xmm26"> |
    ikw<"xmm27"> | ikw<"xmm28"> | ikw<"xmm29"> | ikw<"xmm30"> | ikw<"xmm31"> | ikw<"ymm0"> |
    ikw<"ymm1"> | ikw<"ymm2"> | ikw<"ymm3"> | ikw<"ymm4"> | ikw<"ymm5"> | ikw<"ymm6"> |
    ikw<"ymm7"> | ikw<"ymm8"> | ikw<"ymm9"> | ikw<"ymm10"> | ikw<"ymm11"> | ikw<"ymm12"> |
    ikw<"ymm13"> | ikw<"ymm14"> | ikw<"ymm15"> | ikw<"ymm16"> | ikw<"ymm17"> | ikw<"ymm18"> |
    ikw<"ymm19"> | ikw<"ymm20"> | ikw<"ymm21"> | ikw<"ymm22"> | ikw<"ymm23"> | ikw<"ymm24"> |
    ikw<"ymm25"> | ikw<"ymm26"> | ikw<"ymm27"> | ikw<"ymm28"> | ikw<"ymm29"> | ikw<"ymm30"> |
    ikw<"ymm31"> | ikw<"zmm0"> | ikw<"zmm1"> | ikw<"zmm2"> | ikw<"zmm3"> | ikw<"zmm4"> |
    ikw<"zmm5"> | ikw<"zmm6"> | ikw<"zmm7"> | ikw<"zmm8"> | ikw<"zmm9"> | ikw<"zmm10"> |
    ikw<"zmm11"> | ikw<"zmm12"> | ikw<"zmm13"> | ikw<"zmm14"> | ikw<"zmm15"> | ikw<"zmm16"> |
    ikw<"zmm17"> | ikw<"zmm18"> | ikw<"zmm19"> | ikw<"zmm20"> | ikw<"zmm21"> | ikw<"zmm22"> |
    ikw<"zmm23"> | ikw<"zmm24"> | ikw<"zmm25"> | ikw<"zmm26"> | ikw<"zmm27"> | ikw<"zmm28"> |
    ikw<"zmm29"> | ikw<"zmm30"> | ikw<"zmm31"> | ikw<"tmm0"> | ikw<"tmm1"> | ikw<"tmm2"> |
    ikw<"tmm3"> | ikw<"tmm4"> | ikw<"tmm5"> | ikw<"tmm6"> | ikw<"tmm7"> | ikw<"k0"> |
    ikw<"k1"> | ikw<"k2"> | ikw<"k3"> | ikw<"k4"> | ikw<"k5"> | ikw<"k6"> |
    ikw<"k7"> | ikw<"bnd0"> | ikw<"bnd1"> | ikw<"bnd2"> | ikw<"bnd3">;

  static constexpr auto SizeKw =
      ikw<"byte"> | ikw<"word"> | ikw<"dword"> | ikw<"qword"> |
      ikw<"tword"> | ikw<"oword"> | ikw<"yword"> | ikw<"zword">;

  static constexpr auto DxKw = ikw<"db"> | ikw<"dw"> | ikw<"dd"> |
                               ikw<"dq"> | ikw<"dt"> | ikw<"do"> |
                               ikw<"dy"> | ikw<"dz">;

  static constexpr auto ResxKw = ikw<"resb"> | ikw<"resw"> | ikw<"resd"> |
                                 ikw<"resq"> | ikw<"rest"> | ikw<"reso"> |
                                 ikw<"resy"> | ikw<"resz">;

  struct Expression;
  struct Primary;
  struct BinaryLadder;
  struct Value;

  struct IntegerLiteral
      : rule<assign<&ast::IntegerLit::text>(
            ref<DECIMAL_INTEGER> | ref<OCT_INTEGER> | ref<HEX_INTEGER> |
            ref<BIN_INTEGER>)> {};

  struct FloatLiteral
      : rule<assign<&ast::FloatLit::text>(ref<FLOAT_NUMBER>)> {};

  struct StringLiteral
      : rule<assign<&ast::StringLit::value>(ref<STRING>)> {};

  struct NameRef : rule<assign<&ast::NameRef::name>(ref<NAME>)> {};

  struct RegisterRef
      : rule<assign<&ast::RegisterRef::name>(RegisterName)> {};

  // A plain expression, or `?`, the reserve-with-no-value marker: a lone `?`
  // is the marker here, though it spells a name elsewhere.
  struct Atom : rule<create<ast::QuestionLit>(lit<"?">) | ref<Expression>> {
  };

  // A primary expression: a unary op recursing on itself, a
  // parenthesised expression, or one of the five leaf shapes. A qualified
  // name (`reg:name` or, broadened here, `reg:reg`) is its own alternative
  // rather than an optional prefix on the bare cases, so a plain register or
  // name is not wrapped for nothing.
  struct Primary
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-"> |
                                             lit<"~"> | lit<"!">) +
                 assign<&ast::UnaryExpr::operand>(ref<Primary>))  //
             | (lit<"("> + ref<Expression> + lit<")">)            //
             | ref<IntegerLiteral>                                //
             | create<ast::QualifiedName>(
                   assign<&ast::QualifiedName::segment>(RegisterName) +
                   lit<":"> +
                   assign<&ast::QualifiedName::target>(
                       ref<RegisterRef> | ref<NameRef>))  //
             | ref<RegisterRef>                               //
             | ref<NameRef>                                   //
             | ref<StringLiteral>                                 //
             | ref<FloatLiteral>                                  //
             | create<ast::DollarExpr>(
                   flag<&ast::DollarExpr::section>(lit<"$$">) |
                   lit<"$">)> {};

  // The ten binary levels of the expression ladder, tightest to loosest.
  // `>=` is added to the comparison level, and boolean XOR (`^^`) is wired
  // into the ladder between boolean AND and boolean OR: both operators are
  // in the language but not in its parser rules.
  static constexpr auto OpMul =
      lit<"*"> | lit<"//"> | lit<"/"> | lit<"%%"> | lit<"%">;
  static constexpr auto OpAdd = lit<"+"> | lit<"-">;
  static constexpr auto OpShift =
      lit<"<<<"> | lit<"<<"> | lit<">>>"> | lit<">>">;
  static constexpr auto OpAnd = lit<"&">;
  static constexpr auto OpXor = lit<"^">;
  static constexpr auto OpOr = lit<"|">;
  static constexpr auto OpCompare = lit<"<=>"> | lit<"<="> | lit<">="> |
                                    lit<"<"> | lit<">"> | lit<"=="> |
                                    lit<"="> | lit<"!="> | lit<"<>">;
  static constexpr auto OpBoolAnd = lit<"&&">;
  static constexpr auto OpBoolXor = lit<"^^">;
  static constexpr auto OpBoolOr = lit<"||">;

  struct BinaryLadder
      : infix<Primary, ast::BinaryExpr,
              levels(left(OpMul),      //
                     left(OpAdd),      //
                     left(OpShift),    //
                     left(OpAnd),      //
                     left(OpXor),      //
                     left(OpOr),       //
                     left(OpCompare),  //
                     left(OpBoolAnd),  //
                     left(OpBoolXor),  //
                     left(OpBoolOr))> {};

  // Both ternary branches are expressions and preserve nested conditionals.
  struct Expression
      : rule<create<ast::ConditionalExpr>(
                 assign<&ast::ConditionalExpr::cond>(ref<BinaryLadder>) +
                 lit<"?"> +
                 assign<&ast::ConditionalExpr::then_>(ref<Expression>) +
                 lit<":"> +
                 assign<&ast::ConditionalExpr::else_>(ref<Expression>))  //
             | ref<BinaryLadder>> {};

  // The size-wrap and list-of-values shapes on top of Atom/Expression:
  // `size value`, `dup(...)`, and `%(...)`.
  static constexpr auto ParList =
      lit<"("> + append<&ast::ValueList::items>(ref<Value>) +
      many(lit<","> + append<&ast::ValueList::items>(ref<Value>)) +
      lit<")">;

  struct ValueList
      : rule<create<ast::ValueList>(
                 assign<&ast::ValueList::dupCount>(ref<Expression>) +
                 ikw<"dup"> + option(assign<&ast::ValueList::size>(SizeKw)) +
                 option(flag<&ast::ValueList::percent>(lit<"%">)) +
                 ParList)  //
             | create<ast::ValueList>(
                   flag<&ast::ValueList::percent>(lit<"%">) + ParList)  //
             | create<ast::ValueList>(
                   assign<&ast::ValueList::size>(SizeKw) +
                   option(flag<&ast::ValueList::percent>(lit<"%">)) +
                   ParList)> {};

  struct Value
      : rule<create<ast::Value>(assign<&ast::Value::size>(SizeKw) +
                                assign<&ast::Value::inner>(ref<Value>))  //
             | create<ast::Value>(
                   assign<&ast::Value::list>(ref<ValueList>))  //
             | create<ast::Value>(assign<&ast::Value::atom>(ref<Atom>))> {};

  // An operand's non-bracket forms are all subsets of Expression, so they
  // collapse to one shape here.
  struct Operand
      : rule<create<ast::Operand>(
            option(flag<&ast::Operand::strict>(ikw<"strict">)) +
            option(assign<&ast::Operand::size>(SizeKw)) +
            ((lit<"["> +
              append<&ast::Operand::memory>(ref<Expression>) +
              many(lit<","> + append<&ast::Operand::memory>(ref<Expression>)) +
              lit<"]">)  //
             | assign<&ast::Operand::value>(ref<Expression>)))> {};

  static constexpr auto TimesPrefix =
      option(ikw<"times"> + assign<&ast::Repeatable::times>(ref<Expression>));

  struct InstructionRule
      : rule<create<ast::Instruction>(
            TimesPrefix + assign<&ast::Instruction::mnemonic>(Mnemonic) +
            option(append<&ast::Instruction::operands>(ref<Operand>) +
                   many(lit<","> +
                        append<&ast::Instruction::operands>(
                            ref<Operand>))))> {};

  struct MacroParam
      : rule<ref<StringLiteral> | ref<NameRef> | ref<IntegerLiteral> |
             ref<FloatLiteral>> {};

  static constexpr auto MacroArgs =
      option(append<&ast::MacroCall::args>(ref<MacroParam>) +
             many(lit<","> + append<&ast::MacroCall::args>(ref<MacroParam>)));

  struct MacroCall
      : rule<create<ast::MacroCall>(
            TimesPrefix + assign<&ast::MacroCall::name>(ref<NAME>) +
            ((lit<"("> + MacroArgs + lit<")">) | MacroArgs))> {};

  struct DataDecl
      : rule<create<ast::DataDecl>(
            TimesPrefix +
            option(assign<&ast::DataDecl::label>(ref<NAME>)) +
            assign<&ast::DataDecl::kind>(DxKw) +
            append<&ast::DataDecl::values>(ref<Value>) +
            many(lit<","> + append<&ast::DataDecl::values>(ref<Value>)))> {
  };

  struct ReserveDecl
      : rule<create<ast::ReserveDecl>(
            TimesPrefix +
            option(assign<&ast::ReserveDecl::label>(ref<NAME>)) +
            assign<&ast::ReserveDecl::kind>(ResxKw) +
            assign<&ast::ReserveDecl::count>(ref<Expression>))> {};

  struct IncbinDecl
      : rule<create<ast::IncbinDecl>(
            TimesPrefix +
            option(assign<&ast::IncbinDecl::label>(ref<NAME>)) +
            ikw<"incbin"> + append<&ast::IncbinDecl::args>(ref<Atom>) +
            many(lit<","> + append<&ast::IncbinDecl::args>(ref<Atom>)))> {};

  struct EquDecl
      : rule<create<ast::EquDecl>(
            TimesPrefix +
            option(assign<&ast::EquDecl::label>(ref<NAME>)) + ikw<"equ"> +
            assign<&ast::EquDecl::value>(ref<Expression>))> {};

  // Every directive shape feeds the same ast::Directive; `kind` says which,
  // `names`/`values` collect the sub-clauses positionally rather than one
  // field per clause.
  static constexpr auto SectionAttribute =
      ikw<"private"> | ikw<"public"> | ikw<"common"> | ikw<"stack">;
  static constexpr auto SectionType =
      ikw<"code"> | ikw<"text"> | ikw<"data"> | ikw<"bss"> | ikw<"rdata"> |
      ikw<"info"> | ikw<"mixed"> | ikw<"zerofill"> | ikw<"no_dead_strip"> |
      ikw<"live_support"> | ikw<"strip_static_syms"> | ikw<"debug">;
  static constexpr auto SectionDesignation =
      ikw<"progbits"> | ikw<"nobits"> | ikw<"note"> | ikw<"preinit_array"> |
      ikw<"init_array"> | ikw<"fini_array">;
  static constexpr auto SectionAllocation = ikw<"alloc"> | ikw<"noalloc">;
  static constexpr auto SectionExecution = ikw<"exec"> | ikw<"noexec">;
  static constexpr auto SectionWriting = ikw<"write"> | ikw<"nowrite">;
  static constexpr auto GlobalTypeKw =
      ikw<"function"> | ikw<"data"> | ikw<"object">;
  static constexpr auto VisibilityKw =
      ikw<"default"> | ikw<"internal"> | ikw<"hidden"> | ikw<"protected">;
  static constexpr auto BindingKw = ikw<"weak"> | ikw<"strong">;
  static constexpr auto FloatParamsKw =
      ikw<"daz"> | ikw<"nodaz"> | ikw<"near"> | ikw<"up"> | ikw<"down"> |
      ikw<"zero"> | ikw<"default">;
  static constexpr auto WarningStateKw = lit<"+"> | lit<"-"> | lit<"*">;
  static constexpr auto MapTypeKw = ikw<"all"> | ikw<"brief"> |
                                    ikw<"sections"> | ikw<"segments"> |
                                    ikw<"symbols">;

  static constexpr auto SectionParams =
      append<&ast::Directive::names>(ref<NAME>) +
      option(append<&ast::Directive::names>(SectionAttribute)) +
      option(append<&ast::Directive::names>(SectionType)) +
      option(ikw<"class"> + lit<"="> +
             append<&ast::Directive::names>(ref<NAME>)) +
      option(ikw<"overlay"> + lit<"="> +
             append<&ast::Directive::names>(ref<NAME>)) +
      option(append<&ast::Directive::names>(SectionDesignation)) +
      option(append<&ast::Directive::names>(SectionAllocation)) +
      option(append<&ast::Directive::names>(SectionExecution)) +
      option(append<&ast::Directive::names>(SectionWriting)) +
      option(ikw<"vstart"> + lit<"="> +
             append<&ast::Directive::values>(ref<IntegerLiteral>)) +
      option((ikw<"follows"> | ikw<"vfollows">) + lit<"="> +
             append<&ast::Directive::names>(ref<NAME>)) +
      option(append<&ast::Directive::names>(ikw<"use16"> | ikw<"use32">)) +
      option(append<&ast::Directive::names>(ikw<"flat">)) +
      option((ikw<"absolute"> + lit<"="> +
              append<&ast::Directive::values>(ref<IntegerLiteral>)) |
             ((ikw<"align"> | ikw<"start">) + lit<"="> +
              append<&ast::Directive::values>(ref<IntegerLiteral>)) |
             append<&ast::Directive::names>(ikw<"pointer">)) +
      option(ikw<"comdat"> + lit<"="> +
             append<&ast::Directive::values>(ref<IntegerLiteral>) + lit<":"> +
             append<&ast::Directive::names>(ref<NAME>)) +
      option(append<&ast::Directive::names>(ikw<"tls">));

  static constexpr auto WrtClause =
      option(append<&ast::Directive::names>(ref<NAME>)) + ikw<"wrt"> +
      append<&ast::Directive::names>(ref<NAME>) +
      option(lit<":"> + append<&ast::Directive::values>(ref<IntegerLiteral>));

  static constexpr auto ExternParams = some(
      append<&ast::Directive::names>(ref<NAME>) +
          option(lit<":"> + (WrtClause |
                             append<&ast::Directive::names>(ikw<"weak">))),
      lit<",">);

  static constexpr auto GlobalParams = some(
      append<&ast::Directive::names>(ref<NAME>) +
          option(lit<":"> + append<&ast::Directive::names>(GlobalTypeKw)) +
          option(append<&ast::Directive::names>(VisibilityKw)) +
          option(append<&ast::Directive::names>(BindingKw)) +
          option(append<&ast::Directive::values>(ref<Expression>)),
      lit<",">);

  static constexpr auto CommonParams =
      append<&ast::Directive::names>(ref<NAME>) +
      some(append<&ast::Directive::values>(ref<IntegerLiteral>) +
           option(lit<":"> +
                  (append<&ast::Directive::names>(ikw<"near"> |
                                                  ikw<"far">) |
                   WrtClause |
                   append<&ast::Directive::values>(ref<IntegerLiteral>)))) +
      option(append<&ast::Directive::values>(ref<IntegerLiteral>));

  static constexpr auto ExportParams =
      append<&ast::Directive::names>(ref<NAME>) +
      option(append<&ast::Directive::names>(ref<NAME>)) +
      many(append<&ast::Directive::names>(ikw<"resident"> | ikw<"nodata">) |
           (ikw<"parm"> + lit<"="> +
            append<&ast::Directive::values>(ref<IntegerLiteral>)) |
           append<&ast::Directive::values>(ref<IntegerLiteral>));

  struct Directive
      : rule<create<ast::Directive>(
            (assign<&ast::Directive::kind>(ikw<"bits">) +
             append<&ast::Directive::values>(ref<IntegerLiteral>))  //
            | assign<&ast::Directive::kind>(ikw<"use16">)           //
            | assign<&ast::Directive::kind>(ikw<"use32">)           //
            | (assign<&ast::Directive::kind>(ikw<"default">) +
               append<&ast::Directive::names>(ikw<"rel"> | ikw<"abs"> |
                                              ikw<"bnd"> | ikw<"nobnd">))  //
            | (assign<&ast::Directive::kind>(ikw<"section"> |
                                             ikw<"segment">) +
               SectionParams)  //
            | (assign<&ast::Directive::kind>(ikw<"absolute">) +
               (append<&ast::Directive::values>(ref<IntegerLiteral>) |
                append<&ast::Directive::names>(ref<NAME>)))  //
            | (assign<&ast::Directive::kind>(ikw<"extern"> |
                                             ikw<"required">) +
               ExternParams)                                              //
            | (assign<&ast::Directive::kind>(ikw<"global">) +
               GlobalParams)                                              //
            | (assign<&ast::Directive::kind>(ikw<"common">) +
               CommonParams)                                              //
            | (assign<&ast::Directive::kind>(ikw<"static">) +
               append<&ast::Directive::names>(ref<NAME>))  //
            | (assign<&ast::Directive::kind>(ikw<"cpu">) +
               (append<&ast::Directive::values>(ref<IntegerLiteral>) |
                append<&ast::Directive::names>(ref<NAME>)))  //
            | (assign<&ast::Directive::kind>(ikw<"float">) +
               append<&ast::Directive::names>(FloatParamsKw))  //
            | (lit<"["> +
               ((assign<&ast::Directive::kind>(ikw<"warning">) +
                 option(append<&ast::Directive::names>(WarningStateKw)) +
                 (append<&ast::Directive::names>(ref<WARNING_NAME>) |
                  append<&ast::Directive::names>(ref<NAME>) |
                  append<&ast::Directive::names>(ikw<"push"> |
                                                 ikw<"pop">)))  //
                | (assign<&ast::Directive::kind>(ikw<"map">) +
                   append<&ast::Directive::names>(MapTypeKw) +
                   append<&ast::Directive::names>(ref<NAME>))) +
               lit<"]">)  //
            | (assign<&ast::Directive::kind>(ikw<"org">) +
               append<&ast::Directive::values>(ref<IntegerLiteral>))  //
            | (((assign<&ast::Directive::kind>(ikw<"group">) +
                 append<&ast::Directive::names>(ref<NAME>) +
                 append<&ast::Directive::names>(ref<NAME>)) |
                assign<&ast::Directive::kind>(ikw<"import">)) +
               some(append<&ast::Directive::names>(ref<NAME>)))  //
            | assign<&ast::Directive::kind>(ikw<"uppercase">)    //
            | (assign<&ast::Directive::kind>(ikw<"export">) +
               ExportParams)  //
            | (assign<&ast::Directive::kind>(ikw<"safeseh">) +
               append<&ast::Directive::names>(ref<NAME>))  //
            | (assign<&ast::Directive::kind>(ikw<"osabi">) +
               append<&ast::Directive::values>(ref<IntegerLiteral>)))> {};

  static constexpr auto LineStatement = assign<&ast::Line::stmt>(
      ref<Directive> | (lit<"["> + ref<Directive> + lit<"]">) |
      ref<DataDecl> | ref<ReserveDecl> | ref<IncbinDecl> | ref<EquDecl> |
      ref<InstructionRule> | ref<MacroCall>);

  struct Line
      : rule<create<ast::Line>(
            (((assign<&ast::Line::label>(ref<NAME>) + lit<":"> +
               option(LineStatement)) | LineStatement) + (ref<EOL> | eof)) |
            ref<EOL>)> {};

  struct Program
      : rule<create<ast::Program> +
             many(append<&ast::Program::lines>(ref<Line>))> {};

  using entry = Program;
  using ignored = list<WS, COMMENT, PREPROC, MACROBLOCK>;
};

}  // namespace nasm
