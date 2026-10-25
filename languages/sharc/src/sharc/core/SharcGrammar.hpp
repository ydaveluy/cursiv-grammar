// The SHARC language: its grammar. SHARC is SIMD/DSP assembly: register-file
// operands and instruction opcodes have no useful AST shape, so instruction
// bodies keep their matched text while value expressions, declarations and
// directives get real structure.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "sharc/core/ast.hpp"

namespace sharc {

struct Sharc : cursiv::GrammarSpec {
  static constexpr auto word_boundary = cr<"a-zA-Z0-9_.">;

  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">) + option(eol)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  // An identifier may itself open on a dot (`.foo`, section-local labels):
  // a bare dot-digit run is left to NUM, since the two would otherwise match
  // the same bytes at the same length with no way to prefer one.
  struct ID : terminal<(cr<"a-zA-Z_"> + many(cr<"a-zA-Z_.0-9">)) |
                       (lit<"."> + cr<"a-zA-Z_."> + many(cr<"a-zA-Z_.0-9">))> {
  };

  static constexpr auto HexDigit = cr<"0-9a-fA-F">;
  static constexpr auto HexPrefix = lit<"0x"> | lit<"0X">;
  static constexpr auto Exponent = cr<"eE"> + option(cr<"+-">) + some(d);

  struct NUM : terminal<(some(d) + lit<"."> + many(d) + option(Exponent)) |
                        (lit<"."> + some(d) + option(Exponent)) |
                        (some(d) + Exponent) | some(d) |
                        (HexPrefix + some(HexDigit))> {};

  struct STRING
      : terminal<lit<"\""> +
                 many((lit<"\\"> + any) | cr<R"(^"\\\r\n)">) + lit<"\"">> {};

  struct CHARLIT
      : terminal<lit<"'"> + ((lit<"\\"> + any) | cr<R"(^'\\\r\n)">) + lit<"'">> {
  };

  struct PLUS_OP : terminal<lit<"+">, value<ast::Operator::add>> {};
  struct MINUS_OP : terminal<lit<"-">, value<ast::Operator::subtract>> {};
  struct MULT_OP : terminal<lit<"*">, value<ast::Operator::multiply>> {};
  struct DIV_OP : terminal<lit<"/">, value<ast::Operator::divide>> {};
  struct MOD_OP : terminal<lit<"%">, value<ast::Operator::modulo>> {};
  struct IOR_OP : terminal<lit<"|">, value<ast::Operator::bit_or>> {};
  struct IXOR_OP : terminal<lit<"^">, value<ast::Operator::bit_xor>> {};

  // Forward declarations: value expressions and statement bodies recurse
  // into each other and into themselves.
  struct NumberLitRule;
  struct NameRefRule;
  struct AddrOfRule;
  struct LengthOfRule;
  struct VarAddrRule;
  struct AtomRule;
  struct FactorRule;
  struct TermRule;
  struct ValueExprRule;
  struct StringLitRule;
  struct CharLitRule;
  struct InitExprRule;

  struct StatementRule;
  struct StmtAtomRule;
  struct StmtRule;
  struct ComputeRule;
  struct FlowControlRule;
  struct ImmMovRule;
  struct MiscRule;
  struct DeclarationRule;
  struct IfComputeMovRule;
  struct ComputeMovExprRule;
  struct SecRule;
  struct SegRule;
  struct EndSegRule;
  struct DirectiveExpRule;
  struct ProgramRule;

  // ==================== value expressions ====================
  // No precedence step: `+ - * / % | ^` all stand on the one level a value
  // expression evaluates, left to right.
  static constexpr auto ValueOperator = ref<PLUS_OP> | ref<MINUS_OP> | ref<MULT_OP> |
                                        ref<DIV_OP> | ref<MOD_OP> | ref<IOR_OP> |
                                        ref<IXOR_OP>;
  template <bool Build>
  struct ValueSyntax;
  template <bool Build>
  struct AtomSyntax : rule<[] {
    if constexpr (Build)
      return ref<AtomRule>;
    else
      return ref<NUM> | (lit<"@"> + ref<ID>) |
             (kw<"length"> + lit<"("> + ref<ID> + lit<")">) | ref<ID>;
  }()> {};
  template <bool Build>
  struct FactorSyntax
      : rule<ref<AtomSyntax<Build>> | (lit<"("> + ref<ValueSyntax<Build>> + lit<")">)> {
  };
  template <bool Build>
  struct TermSyntax : rule<[] {
    if constexpr (Build)
      return create<ast::UnaryMinus>(lit<"-"> + assign<&ast::UnaryMinus::operand>(
                                                    ref<FactorSyntax<Build>>)) |
             ref<FactorSyntax<Build>>;
    else
      return option(lit<"-">) + ref<FactorSyntax<Build>>;
  }()> {};
  template <bool Build>
  struct ValueSyntax : rule<[] {
    if constexpr (Build)
      return ref<TermSyntax<Build>> +
             many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 assign<&ast::BinaryExpr::op>(ValueOperator) +
                 assign<&ast::BinaryExpr::right>(ref<TermSyntax<Build>>)));
    else
      return ref<TermSyntax<Build>> + many(ValueOperator + ref<TermSyntax<Build>>);
  }()> {};
  struct ValueExprRule : rule<ref<ValueSyntax<true>>> {};

  struct NumberLitRule
      : rule<create<ast::NumberLit>(assign<&ast::NumberLit::value>(ref<NUM>))> {
  };

  struct NameRefRule
      : rule<create<ast::NameRef>(assign<&ast::NameRef::name>(ref<ID>))> {};

  struct AddrOfRule
      : rule<create<ast::AddrOf>(lit<"@"> +
                                 assign<&ast::AddrOf::name>(ref<ID>))> {};

  struct LengthOfRule
      : rule<create<ast::LengthOf>(kw<"length"> + lit<"("> +
                                   assign<&ast::LengthOf::name>(ref<ID>) +
                                   lit<")">)> {};

  struct VarAddrRule : rule<ref<AddrOfRule> | ref<LengthOfRule>> {};

  struct AtomRule
      : rule<ref<NumberLitRule> | ref<VarAddrRule> | ref<NameRefRule>> {};

  struct StringLitRule
      : rule<create<ast::StringLit>(assign<&ast::StringLit::value>(
            ref<STRING>))> {};

  struct CharLitRule
      : rule<create<ast::CharLit>(assign<&ast::CharLit::value>(
            ref<CHARLIT>))> {};

  struct InitExprRule : rule<ref<ValueExprRule> | ref<CharLitRule>> {};

  // Instruction strings reuse the same expression syntax without allocating nodes.
  struct ValueExprTextRule : rule<ref<ValueSyntax<false>>> {};
  static constexpr auto ValueSkip = ref<ValueExprTextRule>;
  static constexpr auto AtomSkip = ref<AtomSyntax<false>>;

  // ==================== registers ====================
  static constexpr auto RReg =
      kw<"r0"> | kw<"r1"> | kw<"r2"> | kw<"r3"> | kw<"r4"> | kw<"r5"> |
      kw<"r6"> | kw<"r7"> | kw<"r8"> | kw<"r9"> | kw<"r10"> | kw<"r11"> |
      kw<"r12"> | kw<"r13"> | kw<"r14"> | kw<"r15">;
  static constexpr auto FReg =
      kw<"f0"> | kw<"f1"> | kw<"f2"> | kw<"f3"> | kw<"f4"> | kw<"f5"> |
      kw<"f6"> | kw<"f7"> | kw<"f8"> | kw<"f9"> | kw<"f10"> | kw<"f11"> |
      kw<"f12"> | kw<"f13"> | kw<"f14"> | kw<"f15">;
  static constexpr auto BReg =
      kw<"b0"> | kw<"b1"> | kw<"b2"> | kw<"b3"> | kw<"b4"> | kw<"b5"> |
      kw<"b6"> | kw<"b7"> | kw<"b8"> | kw<"b9"> | kw<"b10"> | kw<"b11"> |
      kw<"b12"> | kw<"b13"> | kw<"b14"> | kw<"b15">;
  static constexpr auto LReg =
      kw<"l0"> | kw<"l1"> | kw<"l2"> | kw<"l3"> | kw<"l4"> | kw<"l5"> |
      kw<"l6"> | kw<"l7"> | kw<"l8"> | kw<"l9"> | kw<"l10"> | kw<"l11"> |
      kw<"l12"> | kw<"l13"> | kw<"l14"> | kw<"l15">;
  static constexpr auto IA = kw<"i0"> | kw<"i1"> | kw<"i2"> | kw<"i3"> |
                             kw<"i4"> | kw<"i5"> | kw<"i6"> | kw<"i7">;
  static constexpr auto IC = kw<"i8"> | kw<"i9"> | kw<"i10"> | kw<"i11"> |
                             kw<"i12"> | kw<"i13"> | kw<"i14"> | kw<"i15">;
  static constexpr auto MB = kw<"m0"> | kw<"m1"> | kw<"m2"> | kw<"m3"> |
                             kw<"m4"> | kw<"m5"> | kw<"m6"> | kw<"m7">;
  static constexpr auto MD = kw<"m8"> | kw<"m9"> | kw<"m10"> | kw<"m11"> |
                             kw<"m12"> | kw<"m13"> | kw<"m14"> | kw<"m15">;
  static constexpr auto SReg = kw<"mode1"> | kw<"mode2"> | kw<"irptl"> |
                               kw<"imask"> | kw<"imaskp"> | kw<"astat"> |
                               kw<"stky"> | kw<"ustat1"> | kw<"ustat2">;
  static constexpr auto IReg = IA | IC;
  static constexpr auto MReg = MB | MD;
  static constexpr auto DagReg = IReg | MReg | BReg | LReg;
  static constexpr auto DReg = RReg | FReg;
  static constexpr auto UReg2 =
      DReg | kw<"pc"> | kw<"pcstk"> | kw<"pcstkp"> | kw<"faddr"> |
      kw<"daddr"> | kw<"laddr"> | kw<"curlcntr"> | DagReg | kw<"px1"> |
      kw<"px2"> | kw<"px"> | kw<"tperiod"> | kw<"tcount"> | SReg;
  static constexpr auto UReg = UReg2 | kw<"lcntr">;
  static constexpr auto MRF = kw<"mr0f"> | kw<"mr1f"> | kw<"mr2f">;
  static constexpr auto MRB = kw<"mr0b"> | kw<"mr1b"> | kw<"mr2b">;
  static constexpr auto MR = kw<"mrb"> | kw<"mrf">;
  static constexpr auto AddOrSub = lit<"+"> | lit<"-">;

  static constexpr auto Condition =
      kw<"eq"> | kw<"lt"> | kw<"le"> | kw<"ac"> | kw<"av"> | kw<"mv"> |
      kw<"ms"> | kw<"sv"> | kw<"sz"> | kw<"flag0_in"> | kw<"flag1_in"> |
      kw<"flag2_in"> | kw<"flag3_in"> | kw<"tf"> | kw<"bm"> | kw<"lce"> |
      (kw<"not"> + kw<"lce">) | kw<"ne"> | kw<"ge"> | kw<"gt"> |
      (kw<"not"> + kw<"ac">) | (kw<"not"> + kw<"av">) |
      (kw<"not"> + kw<"mv">) | (kw<"not"> + kw<"ms">) |
      (kw<"not"> + kw<"sv">) | (kw<"not"> + kw<"sz">) |
      (kw<"not"> + kw<"flag0_in">) | (kw<"not"> + kw<"flag1_in">) |
      (kw<"not"> + kw<"flag2_in">) | (kw<"not"> + kw<"flag3_in">) |
      (kw<"not"> + kw<"tf">) | kw<"nbm"> | kw<"forever"> | kw<"true">;

  // ==================== memory addressing ====================
  static constexpr auto MemIaMb = lit<"("> + IA + lit<","> + MB + lit<")">;
  static constexpr auto MemIcMd = lit<"("> + IC + lit<","> + MD + lit<")">;
  static constexpr auto MemMdIc = lit<"("> + MD + lit<","> + IC + lit<")">;
  static constexpr auto MemMbIa = lit<"("> + MB + lit<","> + IA + lit<")">;
  static constexpr auto MemIaInt =
      lit<"("> + IA + lit<","> + ValueSkip + lit<")">;
  static constexpr auto MemIcInt =
      lit<"("> + IC + lit<","> + ValueSkip + lit<")">;
  static constexpr auto MemIntIa =
      lit<"("> + ValueSkip + lit<","> + IA + lit<")">;
  static constexpr auto MemIntIc =
      lit<"("> + ValueSkip + lit<","> + IC + lit<")">;
  static constexpr auto MemInt = lit<"("> + ValueSkip + lit<")">;

  static constexpr auto MemDmIaMb = kw<"dm"> + MemIaMb;
  static constexpr auto MemPmIcMd = kw<"pm"> + MemIcMd;
  static constexpr auto MemDmMbIa = kw<"dm"> + MemMbIa;
  static constexpr auto MemPmMdIc = kw<"pm"> + MemMdIc;
  static constexpr auto MemDmIaInt = kw<"dm"> + MemIaInt;
  static constexpr auto MemPmIcInt = kw<"pm"> + MemIcInt;
  static constexpr auto MemDmIntIa = kw<"dm"> + MemIntIa;
  static constexpr auto MemPmIntIc = kw<"pm"> + MemIntIc;
  static constexpr auto MemDmInt = kw<"dm"> + MemInt;
  static constexpr auto MemPmInt = kw<"pm"> + MemInt;

  // ==================== compute ====================
  static constexpr auto RExp =
      (RReg + AddOrSub + RReg) |
      (RReg + lit<"+"> + RReg + lit<"+"> + kw<"ci">) |
      (RReg + lit<"+"> + RReg + lit<"+"> + kw<"ci"> + lit<"-"> + ref<NUM>) |
      (lit<"("> + RReg + lit<"+"> + RReg + lit<")"> + lit<"/"> + ref<NUM>) |
      (RReg + lit<"+"> + kw<"ci">) |
      (RReg + lit<"+"> + kw<"ci"> + lit<"-"> + ref<NUM>) |
      (RReg + lit<"+"> + ref<NUM>) | (RReg + lit<"-"> + ref<NUM>) |
      (lit<"-"> + RReg) | (kw<"abs"> + RReg) | (kw<"pass"> + RReg) |
      (RReg + kw<"and"> + RReg) | (RReg + kw<"or"> + RReg) |
      (RReg + kw<"xor"> + RReg) | (kw<"not"> + RReg) |
      (kw<"min"> + lit<"("> + RReg + lit<","> + RReg + lit<")">) |
      (kw<"max"> + lit<"("> + RReg + lit<","> + RReg + lit<")">) |
      (kw<"clip"> + RReg + kw<"by"> + RReg) | (kw<"mant"> + FReg) |
      (kw<"logb"> + FReg) | (kw<"fix"> + FReg + option(kw<"by"> + RReg)) |
      (kw<"trunc"> + FReg + option(kw<"by"> + RReg));

  static constexpr auto FixpointAluOp =
      (RReg + lit<"="> + RExp) |
      (kw<"comp"> + lit<"("> + RReg + lit<","> + RReg + lit<")">);

  static constexpr auto FExp =
      (FReg + lit<"+"> + FReg) | (FReg + lit<"-"> + FReg) |
      (kw<"abs"> + lit<"("> + FReg + lit<"+"> + FReg + lit<")">) |
      (kw<"abs"> + lit<"("> + FReg + lit<"-"> + FReg + lit<")">) |
      (lit<"("> + FReg + lit<"+"> + FReg + lit<")"> + lit<"/"> + ref<NUM>) |
      (lit<"-"> + FReg) | (kw<"abs"> + FReg) | (kw<"pass"> + FReg) |
      (kw<"rnd"> + FReg) | (kw<"scalb"> + FReg + kw<"by"> + RReg) |
      (kw<"float"> + RReg + option(kw<"by"> + RReg)) |
      (kw<"recips"> + FReg) | (kw<"rsqrts"> + FReg) |
      (FReg + kw<"copysign"> + FReg) |
      (kw<"min"> + lit<"("> + FReg + lit<","> + FReg + lit<")">) |
      (kw<"max"> + lit<"("> + FReg + lit<","> + FReg + lit<")">) |
      (kw<"clip"> + FReg + kw<"by"> + FReg) | (FReg + lit<"*"> + FReg);

  static constexpr auto FloatingAluOp =
      (FReg + lit<"="> + FExp) |
      (kw<"comp"> + lit<"("> + FReg + lit<","> + FReg + lit<")">);

  static constexpr auto MultiMod1_ =
      lit<"("> + (kw<"si"> | kw<"ui"> | kw<"sf"> | kw<"uf">) + lit<")">;
  static constexpr auto MultiMod2_ =
      lit<"("> +
      (kw<"ssi"> | kw<"sui"> | kw<"usi"> | kw<"uui"> | kw<"ssf"> |
       kw<"suf"> | kw<"usf"> | kw<"uuf"> | kw<"ssfr"> | kw<"sufr"> |
       kw<"usfr"> | kw<"uufr">) +
      lit<")">;
  static constexpr auto MultiR =
      RReg + lit<"*"> + RReg + option(MultiMod2_);
  static constexpr auto MultiExpR =
      MultiR | (MR + AddOrSub + MultiR) |
      (kw<"sat"> + MR + option(MultiMod1_)) |
      (kw<"rnd"> + MR + option(MultiMod1_)) | MR;
  static constexpr auto MultiExpMrf =
      MultiR | (kw<"mrf"> + AddOrSub + MultiR) |
      (kw<"sat"> + kw<"mrf"> + option(MultiMod1_)) |
      (kw<"rnd"> + kw<"mrf"> + option(MultiMod1_));
  static constexpr auto MultiExpMrb =
      MultiR | (kw<"mrb"> + AddOrSub + MultiR) |
      (kw<"sat"> + kw<"mrb"> + option(MultiMod1_)) |
      (kw<"rnd"> + kw<"mrb"> + option(MultiMod1_));

  static constexpr auto MultiOp =
      (RReg + lit<"="> + MultiExpR) | (kw<"mrf"> + lit<"="> + MultiExpMrf) |
      (kw<"mrb"> + lit<"="> + MultiExpMrb) | (MR + lit<"="> + ref<NUM>) |
      ((MRF | MRB) + lit<"="> + RReg) | (RReg + lit<"="> + (MRF | MRB));

  static constexpr auto SecOp = RReg | AtomSkip | (lit<"-"> + AtomSkip);
  static constexpr auto BitData = ref<NUM> + lit<":"> + ref<NUM>;
  static constexpr auto SecOp2 = RReg | BitData;

  static constexpr auto ShifterExp =
      (kw<"lshift"> + RReg + kw<"by"> + SecOp) |
      (RReg + kw<"or"> + kw<"lshift"> + RReg + kw<"by"> + SecOp) |
      (kw<"ashift"> + RReg + kw<"by"> + SecOp) |
      (RReg + kw<"or"> + kw<"ashift"> + RReg + kw<"by"> + SecOp) |
      (kw<"rot"> + RReg + kw<"by"> + SecOp) |
      (kw<"bclr"> + RReg + kw<"by"> + SecOp) |
      (kw<"bset"> + RReg + kw<"by"> + SecOp) |
      (kw<"btgl"> + RReg + kw<"by"> + SecOp) |
      (kw<"fdep"> + RReg + kw<"by"> + SecOp2 +
       option(lit<"("> + kw<"se"> + lit<")">)) |
      (kw<"fext"> + RReg + kw<"by"> + SecOp2 +
       option(lit<"("> + kw<"se"> + lit<")">)) |
      (RReg + kw<"or"> + kw<"fdep"> + RReg + kw<"by"> + SecOp2) |
      (kw<"exp"> + RReg + option(lit<"("> + kw<"ex"> + lit<")">)) |
      (kw<"leftz"> + RReg) | (kw<"lefto"> + RReg) | (kw<"fpack"> + FReg);

  static constexpr auto ShifterOp =
      (RReg + lit<"="> + ShifterExp) | (kw<"btst"> + RReg + kw<"by"> + SecOp) |
      (FReg + lit<"="> + kw<"funpack"> + RReg);

  static constexpr auto DualAddR =
      RReg + lit<"="> + RReg + lit<"+"> + RReg + lit<","> + RReg + lit<"="> +
      RReg + lit<"-"> + RReg;
  static constexpr auto ParallelMulti =
      (MultiOp + some(lit<","> + FixpointAluOp)) |
      (FloatingAluOp + some(lit<","> + FloatingAluOp));
  static constexpr auto DualOp = DualAddR | ParallelMulti;

  // No directive anywhere in this body: it joins as the matched text, per
  // `assign<>`'s requirement that its argument itself produce a value.
  struct ComputeBodyRule
      : rule<DualOp | FixpointAluOp | FloatingAluOp | MultiOp | ShifterOp> {};

  struct ComputeRule
      : rule<create<ast::Compute>(
            assign<&ast::Compute::raw>(ref<ComputeBodyRule>))> {};

  // ==================== moves ====================
  static constexpr auto MovExp1_1 =
      (MemDmIaMb + lit<"="> + DReg) | (DReg + lit<"="> + MemDmIaMb);
  static constexpr auto MovExp1_2 =
      (MemPmIcMd + lit<"="> + DReg) | (DReg + lit<"="> + MemPmIcMd);
  static constexpr auto MovExp1 = MovExp1_1 + lit<","> + MovExp1_2;
  static constexpr auto MovExp3a = (MemDmIaMb | MemPmIcMd) + lit<"="> + UReg;
  static constexpr auto MovExp3b = (MemDmMbIa | MemPmMdIc) + lit<"="> + UReg;
  static constexpr auto MovExp3c = UReg + lit<"="> + (MemDmIaMb | MemPmIcMd);
  static constexpr auto MovExp3d = UReg + lit<"="> + (MemDmMbIa | MemPmMdIc);
  static constexpr auto MovExp4a =
      (MemDmIaInt | MemPmIcInt) + lit<"="> + DReg;
  static constexpr auto ImmMov15a =
      (MemDmIntIa | MemPmIntIc) + lit<"="> + UReg;
  static constexpr auto MovExp4b = ImmMov15a;
  static constexpr auto MovExp4c =
      DReg + lit<"="> + (MemDmIaInt | MemPmIcInt);
  static constexpr auto ImmMov15b =
      UReg + lit<"="> + (MemDmIntIa | MemPmIntIc);
  static constexpr auto MovExp4d = ImmMov15b;
  static constexpr auto MovExp5 = UReg2 + lit<"="> + UReg;
  static constexpr auto MovExp7 =
      kw<"modify"> +
      ((lit<"("> + IA + lit<","> + MB + lit<")">) |
       (lit<"("> + IC + lit<","> + MD + lit<")">));

  struct ComputeMovExpBodyRule
      : rule<option(ref<ComputeBodyRule> + lit<",">) +
             (MovExp1 | MovExp3a | MovExp3b | MovExp3c | MovExp3d | MovExp4a |
              MovExp4b | MovExp4c | MovExp4d | MovExp5 | MovExp7)> {};

  struct ComputeMovExprRule
      : rule<create<ast::ComputeMovExpr>(
            assign<&ast::ComputeMovExpr::raw>(ref<ComputeMovExpBodyRule>))> {
  };

  struct IfComputeMovBodyRule
      : rule<kw<"if"> + Condition +
             (ref<ComputeMovExpBodyRule> | ref<ComputeBodyRule>)> {};

  struct IfComputeMovRule
      : rule<create<ast::IfComputeMov>(
            assign<&ast::IfComputeMov::raw>(ref<IfComputeMovBodyRule>))> {};

  // ==================== flow control ====================
  static constexpr auto JumpAddrPc =
      lit<"("> + kw<"pc"> + lit<","> + ValueSkip + lit<")">;
  static constexpr auto JumpAddrInt = ValueSkip;
  static constexpr auto JumpAddrIntOrPc = JumpAddrInt | JumpAddrPc;
  static constexpr auto FlowControl10Frag = MemMdIc | JumpAddrPc;

  static constexpr auto JumpModifier =
      lit<"("> +
      ((kw<"db"> + option(lit<","> + (kw<"la"> | kw<"ci">))) | kw<"la"> |
       kw<"ci">) +
      lit<")">;
  static constexpr auto JumpModifier2 = lit<"("> + kw<"db"> + lit<")">;
  static constexpr auto JumpModifier3 =
      lit<"("> + ((kw<"db"> + option(lit<","> + kw<"lr">)) | kw<"lr">) +
      lit<")">;

  static constexpr auto FlowControl8a =
      kw<"jump"> + JumpAddrInt + option(JumpModifier);
  static constexpr auto FlowControl8b =
      kw<"call"> + JumpAddrInt + option(JumpModifier2);
  static constexpr auto FlowControl9a =
      kw<"jump"> + FlowControl10Frag + option(JumpModifier) +
      option(lit<","> + ref<ComputeBodyRule>);
  static constexpr auto FlowControl9b =
      kw<"call"> + FlowControl10Frag + option(JumpModifier2) +
      option(lit<","> + ref<ComputeBodyRule>);
  static constexpr auto FlowControl11a =
      kw<"rts"> + option(JumpModifier3) + option(lit<","> + ref<ComputeBodyRule>);
  static constexpr auto FlowControl11b =
      kw<"rti"> + option(JumpModifier2) + option(lit<","> + ref<ComputeBodyRule>);
  static constexpr auto FlowControl8Exp = FlowControl8a | FlowControl8b;
  static constexpr auto FlowControl9and11Exp =
      FlowControl9a | FlowControl9b | FlowControl11a | FlowControl11b;
  static constexpr auto FlowContorl8 = kw<"if"> + Condition + FlowControl8Exp;
  static constexpr auto FlowControl9and11 = kw<"if"> + Condition +
                                            FlowControl9and11Exp + lit<","> +
                                            kw<"else"> + ref<ComputeBodyRule>;
  static constexpr auto FlowControl10 =
      kw<"if"> + Condition + kw<"jump"> + FlowControl10Frag + lit<","> +
      kw<"else"> + option(ref<ComputeBodyRule> + lit<",">) + MovExp1_1;
  static constexpr auto LcntrV = ValueSkip | UReg;
  static constexpr auto FlowControl12 =
      kw<"lcntr"> + lit<"="> + LcntrV + lit<","> + kw<"do"> +
      JumpAddrIntOrPc + kw<"until"> + kw<"lce">;
  static constexpr auto FlowControl13 =
      kw<"do"> + JumpAddrIntOrPc + kw<"until"> + Condition;

  struct FlowControlBodyRule
      : rule<FlowContorl8 | FlowControl9and11 | FlowControl10 |
             FlowControl8a | FlowControl8b | FlowControl9a | FlowControl9b |
             FlowControl11a | FlowControl11b | FlowControl12 |
             FlowControl13> {};

  struct FlowControlRule
      : rule<create<ast::FlowControl>(
            assign<&ast::FlowControl::raw>(ref<FlowControlBodyRule>))> {};

  // ==================== immediate moves ====================
  static constexpr auto ImmMov14a = (MemDmInt | MemPmInt) + lit<"="> + UReg;
  static constexpr auto ImmMov14b = UReg + lit<"="> + (MemDmInt | MemPmInt);
  static constexpr auto ImmMov16 =
      (MemDmIaMb | MemPmIcMd) + lit<"="> + ValueSkip;
  static constexpr auto ImmMov17 = UReg2 + lit<"="> + ValueSkip;
  struct ImmMovBodyRule
      : rule<ImmMov14a | ImmMov14b | ImmMov16 | ImmMov17> {};

  struct ImmMovRule
      : rule<create<ast::ImmMov>(
            assign<&ast::ImmMov::raw>(ref<ImmMovBodyRule>))> {};

  // ==================== misc ====================
  static constexpr auto Misc20 =
      (kw<"push"> | kw<"pop">) + (kw<"loop"> | kw<"sts"> | kw<"pcstk">);
  struct MiscBodyRule
      : rule<(kw<"bit"> +
              (kw<"set"> | kw<"clr"> | kw<"tgl"> | kw<"tst"> | kw<"xor">) +
              SReg + ValueSkip) |
             (kw<"bitrev"> + (MemIaInt | MemIcInt)) |
             (kw<"modify"> + lit<"("> + IA + lit<","> + ValueSkip + lit<")">) |
             (kw<"modify"> + lit<"("> + IC + lit<","> + ValueSkip + lit<")">) |
             (Misc20 + many(lit<","> + Misc20)) |
             (kw<"flush"> + kw<"cache">) | kw<"nop"> | kw<"idle"> |
             kw<"idle16"> |
             (kw<"cjump"> + JumpAddrIntOrPc + option(JumpModifier2)) |
             kw<"rframe">> {};

  struct MiscRule
      : rule<create<ast::Misc>(assign<&ast::Misc::raw>(ref<MiscBodyRule>))> {
  };

  // ==================== declaration ====================
  static constexpr auto DeclF1 =
      some(append<&ast::Declaration::values>(ref<InitExprRule>), lit<",">) |
      append<&ast::Declaration::values>(ref<StringLitRule>);
  static constexpr auto DeclF2 = (lit<"{"> + DeclF1 + lit<"}">) | DeclF1;

  static constexpr auto DeclExp1 =
      some(append<&ast::Declaration::names>(ref<ID>), lit<",">);
  static constexpr auto DeclExp2 =
      lit<"="> +
      some(append<&ast::Declaration::values>(ref<InitExprRule>), lit<",">);
  static constexpr auto DeclExp3 =
      append<&ast::Declaration::names>(ref<ID>) +
      flag<&ast::Declaration::is_array>(lit<"[">) + lit<"]"> +
      option(lit<"="> + DeclF2);
  static constexpr auto DeclExp4 =
      append<&ast::Declaration::names>(ref<ID>) +
      flag<&ast::Declaration::is_array>(lit<"[">) +
      assign<&ast::Declaration::size>(ref<ValueExprRule>) + lit<"]"> +
      option(lit<"="> + DeclF2);
  static constexpr auto DeclExp5 = append<&ast::Declaration::names>(
                                       ref<ID>) +
                                   lit<"="> +
                                   append<&ast::Declaration::values>(
                                       ref<ValueExprRule>);

  struct DeclarationRule
      : rule<create<ast::Declaration>(
            kw<".var"> +
            (DeclExp1 | DeclExp2 | DeclExp3 | DeclExp4 | DeclExp5))> {};

  // ==================== sections and segments ====================
  static constexpr auto SegQ1 =
      lit<"/"> +
      (kw<"pm"> | kw<"code"> | kw<"dm"> | kw<"data64"> | kw<"data">);
  static constexpr auto SegQ2 =
      lit<"/"> + (kw<"no_init"> | kw<"zero_init"> | kw<"runtime_init">);
  static constexpr auto SegQ3 = lit<"/"> + kw<"dmaonly">;
  struct SegQualifierRule
      : rule<(SegQ1 + option(SegQ2 | SegQ3)) | (SegQ2 + option(SegQ1 | SegQ3)) |
             (SegQ3 + option(SegQ1 | SegQ2))> {};

  struct SecRule
      : rule<create<ast::SectionDecl>(
            kw<".section"> +
            assign<&ast::SectionDecl::qualifier>(ref<SegQualifierRule>) +
            assign<&ast::SectionDecl::name>(ref<ID>))> {};

  struct SegRule
      : rule<create<ast::SegmentDecl>(
            kw<".segment"> +
            assign<&ast::SegmentDecl::qualifier>(ref<SegQualifierRule>) +
            assign<&ast::SegmentDecl::name>(ref<ID>))> {};

  struct EndSegRule : rule<create<ast::EndSeg> + kw<".endseg">> {};

  // ==================== directives ====================
  // .file_attr's argument: approximated as this language's value-carrying
  // token kinds.
  struct DirectiveExpRule
      : rule<
            create<ast::Directive>(
                assign<&ast::Directive::keyword>(kw<".align">) +
                append<&ast::Directive::values>(ref<NumberLitRule>))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(kw<".compress">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(kw<".extern">) +
                  some(append<&ast::Directive::names>(ref<ID>), lit<",">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(kw<".file">) +
                  append<&ast::Directive::strings>(ref<STRING>))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(kw<".file_attr">) +
                  option(append<&ast::Directive::values>(ref<ValueExprRule>) |
                         append<&ast::Directive::strings>(ref<STRING>) |
                         append<&ast::Directive::strings>(ref<CHARLIT>) |
                         append<&ast::Directive::names>(ref<ID>)))  //
            | create<ast::Directive>(assign<&ast::Directive::keyword>(
                  kw<".forcecompress">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(kw<".global">) +
                  some(append<&ast::Directive::names>(ref<ID>), lit<",">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(kw<".import">) +
                  some(append<&ast::Directive::strings>(ref<STRING>),
                       lit<",">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(kw<".leftmargin">) +
                  append<&ast::Directive::values>(ref<ValueExprRule>))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(kw<".list">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(kw<".list_data">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(kw<".list_datfile">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(kw<".list_deftab">) +
                  append<&ast::Directive::values>(ref<ValueExprRule>))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(kw<".list_loctab">) +
                  append<&ast::Directive::values>(ref<ValueExprRule>))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(kw<".list_wrapdata">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(kw<".newpage">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(kw<".nocompress">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(kw<".nolist_data">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(kw<".nolist_datfile">))  //
            | create<ast::Directive>(assign<&ast::Directive::keyword>(
                  kw<".nolist_wrapdata">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(kw<".pagelength">) +
                  append<&ast::Directive::values>(ref<ValueExprRule>))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(kw<".pagewidth">) +
                  append<&ast::Directive::values>(ref<ValueExprRule>))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(kw<".precision">) +
                  option(lit<"=">) +
                  append<&ast::Directive::values>(ref<NumberLitRule>))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(kw<".round_minus">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(kw<".round_nearest">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(kw<".round_plus">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(kw<".round_zero">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(kw<".previous">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(kw<".weak">) +
                  append<&ast::Directive::names>(ref<ID>))> {};

  // ==================== statement ====================
  struct StmtRule
      : rule<ref<ComputeRule> | ref<FlowControlRule> | ref<ImmMovRule> |
             ref<MiscRule> | ref<DeclarationRule> | ref<IfComputeMovRule> |
             ref<ComputeMovExprRule>> {};

  struct StmtAtomRule
      : rule<ref<StmtRule> | ref<SecRule> | ref<SegRule> | ref<EndSegRule> |
             ref<DirectiveExpRule>> {};

  struct StatementRule
      : rule<create<ast::Statement>(
            many(append<&ast::Statement::labels>(ref<ID>) + lit<":">) +
            assign<&ast::Statement::atom>(ref<StmtAtomRule>))> {};

  struct ProgramRule
      : rule<create<ast::Program>(
            some(append<&ast::Program::statements>(ref<StatementRule>) +
                 lit<";">))> {};

  using entry = ProgramRule;
  using ignore = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace sharc
