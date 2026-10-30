// The SHARC language: its grammar. SHARC is SIMD/DSP assembly: register-file
// instruction payloads retain token boundaries. Value expressions,
// declarations and directives have structured AST nodes.
#pragma once

#include <memory_resource>
#include <span>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "sharc/core/ast.hpp"

namespace sharc {

// Preserve token boundaries in instruction payloads. Data rules otherwise
// concatenate adjacent keywords into spellings such as IFEQJUMPtarget.
inline constexpr auto instruction_text = [](std::span<const std::string_view> parts,
                                            std::pmr::memory_resource* mem) {
  std::size_t size = parts.empty() ? 0 : parts.size() - 1;
  for (const auto part : parts) size += part.size();
  const auto room = cursiv::text_room(mem, size);
  std::size_t offset = 0;
  for (std::size_t i = 0; i < parts.size(); ++i) {
    if (i) room[offset++] = ' ';
    for (char c : parts[i]) room[offset++] = c;
  }
  return std::string_view(room.data(), offset);
};


struct Sharc : cursiv::GrammarSpec {
  static constexpr auto word_boundary = cr<"a-zA-Z0-9_.$">;

  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">) + option(eol)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  struct ID;

  static constexpr auto HexDigit = cr<"0-9a-fA-F">;
  static constexpr auto HexPrefix = lit<"0x"> | lit<"0X">;
  static constexpr auto Exponent = cr<"eE"> + option(cr<"+-">) + some(d);

  struct NUM : terminal<
      (cr<"bB"> + lit<"#"> + some(cr<"01">)) |
      (HexPrefix + some(HexDigit)) |
      (((some(d) + lit<"."> + many(d) + option(Exponent)) |
        (lit<"."> + some(d) + option(Exponent)) |
        (some(d) + Exponent) | some(d)) + option(cr<"rR">))> {};

  struct STRING
      : terminal<lit<"\""> +
                 many((lit<"\\"> + any) | cr<R"(^"\\\r\n)">) + lit<"\"">> {};

  struct CHARLIT
      : terminal<lit<"'"> + ((lit<"\\"> + any) | cr<R"(^'\\\r\n)">) + lit<"'">> {
  };


  // Forward declarations: value expressions and statement bodies recurse
  // into each other and into themselves.
  struct NumberLit;
  struct NameRef;
  struct AddrOf;
  struct LengthOf;
  struct VarAddr;
  struct Atom;
  struct Factor;
  struct Term;
  struct ValueExpr;
  struct StringLit;
  struct CharLit;
  struct InitExpr;

  struct Statement;
  struct StmtAtom;
  struct Stmt;
  struct Compute;
  struct FlowControl;
  struct ImmMov;
  struct Misc;
  struct Declaration;
  struct IfComputeMov;
  struct ComputeMovExpr;
  struct Sec;
  struct Seg;
  struct EndSeg;
  struct DirectiveExp;
  struct Program;

  // ==================== value expressions ====================
  static constexpr auto MultiplyOp = as<ast::Operator::multiply>(lit<"*">) |
      as<ast::Operator::divide>(lit<"/">) | as<ast::Operator::modulo>(lit<"%">);
  static constexpr auto AddOp = as<ast::Operator::add>(lit<"+">) | as<ast::Operator::subtract>(lit<"-">);
  static constexpr auto ShiftOp = as<ast::Operator::shift_left>(lit<"<<">) | as<ast::Operator::shift_right>(lit<">>">);
  static constexpr auto AndOp = as<ast::Operator::bit_and>(lit<"&">);
  static constexpr auto OrOp = as<ast::Operator::bit_or>(lit<"|">);
  static constexpr auto XorOp = as<ast::Operator::bit_xor>(lit<"^">);
  static constexpr auto ValueOperator = MultiplyOp | AddOp | ShiftOp | AndOp | OrOp | XorOp;

  struct ValueExprText;
  struct Factor : rule<ref<Atom> | (lit<"("> + ref<ValueExpr> + lit<")">)> {};
  struct Term
      : rule<create<ast::UnaryMinus>(lit<"-"> + assign<&ast::UnaryMinus::operand>(ref<Term>)) |
             create<ast::BitNot>(lit<"~"> + assign<&ast::BitNot::operand>(ref<Term>)) | ref<Factor>> {};
  struct ValueExpr : infix<Term, ast::BinaryExpr,
      levels(left(MultiplyOp), left(AddOp), left(ShiftOp), left(AndOp), left(OrOp), left(XorOp))> {};

  struct NumberLit
      : rule<create<ast::NumberLit>(assign<&ast::NumberLit::value>(ref<NUM>))> {
  };

  struct NameRef
      : rule<create<ast::NameRef>(assign<&ast::NameRef::name>(ref<ID>))> {};

  struct AddrOf
      : rule<create<ast::AddrOf>(
            (lit<"@"> + assign<&ast::AddrOf::name>(ref<ID>)) |
            (ikw<"ADDRESS"> + lit<"("> + assign<&ast::AddrOf::name>(ref<ID>) + lit<")">))> {};

  struct LengthOf
      : rule<create<ast::LengthOf>(ikw<"length"> + lit<"("> +
                                   assign<&ast::LengthOf::name>(ref<ID>) +
                                   lit<")">)> {};

  struct VarAddr : rule<ref<AddrOf> | ref<LengthOf>> {};

  struct Atom
      : rule<ref<NumberLit> | ref<VarAddr> | ref<NameRef>> {};

  struct StringLit
      : rule<create<ast::StringLit>(assign<&ast::StringLit::value>(
            ref<STRING>))> {};

  struct CharLit
      : rule<create<ast::CharLit>(assign<&ast::CharLit::value>(
            ref<CHARLIT>))> {};

  struct InitExpr : rule<ref<ValueExpr> | ref<CharLit>> {};

  // Instruction expression syntax needs no expression AST; every operator
  // still consumes the same token sequence as its structured counterpart.
  struct AtomText : rule<as<instruction_text>(ref<NUM> | (lit<"@"> + ref<ID>) |
      ((ikw<"length"> | ikw<"address">) + lit<"("> + ref<ID> + lit<")">) | ref<ID>)> {};
  struct FactorText : rule<as<instruction_text>(ref<AtomText> | (lit<"("> + ref<ValueExprText> + lit<")">))> {};
  struct TermText : rule<as<instruction_text>((cr<"-~"> + ref<TermText>) | ref<FactorText>)> {};
  struct ValueExprText : rule<as<instruction_text>(ref<TermText> + many(ValueOperator + ref<TermText>))> {};
  static constexpr auto ValueSkip = ref<ValueExprText>;
  static constexpr auto AtomSkip = ref<AtomText>;

  // ==================== registers ====================
  static constexpr auto RReg =
      ikw<"r0"> | ikw<"r1"> | ikw<"r2"> | ikw<"r3"> | ikw<"r4"> | ikw<"r5"> |
      ikw<"r6"> | ikw<"r7"> | ikw<"r8"> | ikw<"r9"> | ikw<"r10"> | ikw<"r11"> |
      ikw<"r12"> | ikw<"r13"> | ikw<"r14"> | ikw<"r15">;
  static constexpr auto FReg =
      ikw<"f0"> | ikw<"f1"> | ikw<"f2"> | ikw<"f3"> | ikw<"f4"> | ikw<"f5"> |
      ikw<"f6"> | ikw<"f7"> | ikw<"f8"> | ikw<"f9"> | ikw<"f10"> | ikw<"f11"> |
      ikw<"f12"> | ikw<"f13"> | ikw<"f14"> | ikw<"f15">;
  static constexpr auto BReg =
      ikw<"b0"> | ikw<"b1"> | ikw<"b2"> | ikw<"b3"> | ikw<"b4"> | ikw<"b5"> |
      ikw<"b6"> | ikw<"b7"> | ikw<"b8"> | ikw<"b9"> | ikw<"b10"> | ikw<"b11"> |
      ikw<"b12"> | ikw<"b13"> | ikw<"b14"> | ikw<"b15">;
  static constexpr auto LReg =
      ikw<"l0"> | ikw<"l1"> | ikw<"l2"> | ikw<"l3"> | ikw<"l4"> | ikw<"l5"> |
      ikw<"l6"> | ikw<"l7"> | ikw<"l8"> | ikw<"l9"> | ikw<"l10"> | ikw<"l11"> |
      ikw<"l12"> | ikw<"l13"> | ikw<"l14"> | ikw<"l15">;
  static constexpr auto IA = ikw<"i0"> | ikw<"i1"> | ikw<"i2"> | ikw<"i3"> |
                             ikw<"i4"> | ikw<"i5"> | ikw<"i6"> | ikw<"i7">;
  static constexpr auto IC = ikw<"i8"> | ikw<"i9"> | ikw<"i10"> | ikw<"i11"> |
                             ikw<"i12"> | ikw<"i13"> | ikw<"i14"> | ikw<"i15">;
  static constexpr auto MB = ikw<"m0"> | ikw<"m1"> | ikw<"m2"> | ikw<"m3"> |
                             ikw<"m4"> | ikw<"m5"> | ikw<"m6"> | ikw<"m7">;
  static constexpr auto MD = ikw<"m8"> | ikw<"m9"> | ikw<"m10"> | ikw<"m11"> |
                             ikw<"m12"> | ikw<"m13"> | ikw<"m14"> | ikw<"m15">;
  static constexpr auto SReg = ikw<"mode1"> | ikw<"mode2"> | ikw<"irptl"> |
                               ikw<"imask"> | ikw<"imaskp"> | ikw<"astat"> |
                               ikw<"stky"> | ikw<"ustat1"> | ikw<"ustat2">;
  static constexpr auto IReg = IA | IC;
  static constexpr auto MReg = MB | MD;
  static constexpr auto DagReg = IReg | MReg | BReg | LReg;
  static constexpr auto DReg = RReg | FReg;
  static constexpr auto UReg2 =
      DReg | ikw<"pc"> | ikw<"pcstk"> | ikw<"pcstkp"> | ikw<"faddr"> |
      ikw<"daddr"> | ikw<"laddr"> | ikw<"curlcntr"> | DagReg | ikw<"px1"> |
      ikw<"px2"> | ikw<"px"> | ikw<"tperiod"> | ikw<"tcount"> | SReg;
  static constexpr auto UReg = UReg2 | ikw<"lcntr">;
  static constexpr auto MRF = ikw<"mr0f"> | ikw<"mr1f"> | ikw<"mr2f">;
  static constexpr auto MRB = ikw<"mr0b"> | ikw<"mr1b"> | ikw<"mr2b">;
  static constexpr auto MR = ikw<"mrb"> | ikw<"mrf">;
  // VisualDSP++ 5.0 assembler Table 1-11; suspect printed cells are
  // kept out of this set. C-struct field names need a separate identifier.
  static constexpr auto ReservedWord = UReg | MRF | MRB | MR |
      ikw<".align"> | ikw<".compress"> | ikw<".elif"> | ikw<".else"> | ikw<".endif"> |
      ikw<".extern"> | ikw<".file"> | ikw<".file_attr"> | ikw<".forcecompress"> |
      ikw<".global"> | ikw<".if"> | ikw<".import"> | ikw<".leftmargin"> | ikw<".list"> |
      ikw<".list_data"> | ikw<".list_datfile"> | ikw<".list_deftab"> | ikw<".list_loctab"> |
      ikw<".list_wrapdata"> | ikw<".newpage"> | ikw<".nocompress"> | ikw<".nolist_data"> |
      ikw<".nolist_datfile"> | ikw<".nolist_wrapdata"> | ikw<".pagelength"> |
      ikw<".pagewidth"> | ikw<".precision"> | ikw<".previous"> | ikw<".round_minus"> |
      ikw<".round_nearest"> | ikw<".round_plus"> | ikw<".round_zero"> | ikw<".section"> |
      ikw<".struct"> | ikw<".var"> | ikw<".weak"> | ikw<"abs"> | ikw<"acs"> | ikw<"act"> |
      ikw<"address"> | ikw<"and"> | ikw<"ashift"> | ikw<"av"> | ikw<"bb"> | ikw<"bclr"> |
      ikw<"bf"> | ikw<"bit"> | ikw<"bitrev"> | ikw<"bm"> | ikw<"bset"> | ikw<"btgl"> |
      ikw<"btsts"> | ikw<"by"> | ikw<"ca"> | ikw<"cache"> | ikw<"call"> | ikw<"ch"> |
      ikw<"ci"> | ikw<"cjump"> | ikw<"cl"> | ikw<"clip"> | ikw<"comp"> | ikw<"copysign"> |
      ikw<"cos"> | ikw<"db"> | ikw<"dec"> | ikw<"def"> | ikw<"dim"> | ikw<"dm"> |
      ikw<"dma1e"> | ikw<"dma1s"> | ikw<"dma2e"> | ikw<"dma2s"> | ikw<"dmabank1"> |
      ikw<"dmabank2"> | ikw<"dmabank3"> | ikw<"dmadr"> | ikw<"dmawait"> | ikw<"do"> |
      ikw<"dovl"> | ikw<"eb"> | ikw<"ece"> | ikw<"ef"> | ikw<"else"> | ikw<"emuclk"> |
      ikw<"emuclk2"> | ikw<"emuidle"> | ikw<"emun"> | ikw<"endef"> | ikw<"eos"> | ikw<"eq"> |
      ikw<"ex"> | ikw<"exp"> | ikw<"exp2"> | ikw<"fdep"> | ikw<"fext"> | ikw<"file"> |
      ikw<"fix"> | ikw<"flag0_in"> | ikw<"flag1_in"> | ikw<"flag2_in"> | ikw<"flag3_in"> |
      ikw<"float"> | ikw<"flush"> | ikw<"fmerg"> | ikw<"forever"> | ikw<"fpack"> |
      ikw<"fractional"> | ikw<"fta"> | ikw<"ftb"> | ikw<"ftc"> | ikw<"funpack"> |
      ikw<"gcc_compiled"> | ikw<"ge"> | ikw<"gt"> | ikw<"idle16"> | ikw<"if"> | ikw<"inc"> |
      ikw<"jump"> | ikw<"la"> | ikw<"lce"> | ikw<"le"> | ikw<"lefto"> | ikw<"leftz"> |
      ikw<"length"> | ikw<"line"> | ikw<"ln"> | ikw<"load"> | ikw<"log2"> | ikw<"logb"> |
      ikw<"loop"> | ikw<"lr"> | ikw<"lshift"> | ikw<"lt"> | ikw<"mant"> | ikw<"max"> |
      ikw<"mbm"> | ikw<"min"> | ikw<"mod"> | ikw<"modify"> | ikw<"ms"> | ikw<"mv"> |
      ikw<"ne"> | ikw<"nofz"> | ikw<"nop"> | ikw<"nopspecial"> | ikw<"not"> | ikw<"nu"> |
      ikw<"nw"> | ikw<"offsetof"> | ikw<"or"> | ikw<"p20"> | ikw<"p32"> | ikw<"p40"> |
      ikw<"pack"> | ikw<"page"> | ikw<"pm"> | ikw<"pmadr"> | ikw<"pmbank1"> | ikw<"pmdae"> |
      ikw<"pmdas"> | ikw<"pop"> | ikw<"povl1"> | ikw<"psa1e"> | ikw<"psa1s"> | ikw<"psa2e"> |
      ikw<"psa3e"> | ikw<"psa3s"> | ikw<"psa4e"> | ikw<"psa4s"> | ikw<"push"> | ikw<"read"> |
      ikw<"recips"> | ikw<"retain_name"> | ikw<"rframe"> | ikw<"rnd"> | ikw<"rot"> |
      ikw<"rs"> | ikw<"rsqrts"> | ikw<"rti"> | ikw<"rts"> | ikw<"scalb"> | ikw<"scl"> |
      ikw<"se"> | ikw<"set"> | ikw<"sf"> | ikw<"sin"> | ikw<"size"> | ikw<"sizeof"> |
      ikw<"sqr"> | ikw<"sr"> | ikw<"ssf"> | ikw<"ssfr"> | ikw<"ssi"> | ikw<"ssir"> |
      ikw<"st"> | ikw<"step"> | ikw<"struct"> | ikw<"sts"> | ikw<"suf"> | ikw<"sufr"> |
      ikw<"sv"> | ikw<"sw"> | ikw<"sz"> | ikw<"tag"> | ikw<"tf"> | ikw<"tgl"> | ikw<"trap"> |
      ikw<"true"> | ikw<"trunc"> | ikw<"tst"> | ikw<"type"> | ikw<"uf"> | ikw<"ui"> |
      ikw<"unpack"> | ikw<"until"> | ikw<"ur"> | ikw<"usf"> | ikw<"usfr"> | ikw<"usi"> |
      ikw<"usir"> | ikw<"uuf"> | ikw<"uufr"> | ikw<"uuir"> | ikw<"val"> | ikw<"with"> |
      ikw<"xor"> | ikw<"__adi__"> | ikw<"__date__"> | ikw<"__file__"> |
      ikw<"__lastsuffix__"> | ikw<"__line__"> | ikw<"__normal_word_code__"> |
      ikw<"__short_word_code__"> | ikw<"__time__">;

  // Exact words are reserved in every symbol position; keyword boundaries
  // keep r1_name, r1.local and r1$foo as identifiers. Dot-digit runs stay NUM.
  struct ID : terminal<!ReservedWord +
      ((cr<"a-zA-Z_"> + many(cr<"a-zA-Z_.$0-9">)) |
       (lit<"."> + cr<"a-zA-Z_.$"> + many(cr<"a-zA-Z_.$0-9">)))> {};

  static constexpr auto AddOrSub = lit<"+"> | lit<"-">;

  static constexpr auto Condition =
      ikw<"eq"> | ikw<"lt"> | ikw<"le"> | ikw<"ac"> | ikw<"av"> | ikw<"mv"> |
      ikw<"ms"> | ikw<"sv"> | ikw<"sz"> | ikw<"flag0_in"> | ikw<"flag1_in"> |
      ikw<"flag2_in"> | ikw<"flag3_in"> | ikw<"tf"> | ikw<"bm"> | ikw<"lce"> |
      (ikw<"not"> + ikw<"lce">) | ikw<"ne"> | ikw<"ge"> | ikw<"gt"> |
      (ikw<"not"> + ikw<"ac">) | (ikw<"not"> + ikw<"av">) |
      (ikw<"not"> + ikw<"mv">) | (ikw<"not"> + ikw<"ms">) |
      (ikw<"not"> + ikw<"sv">) | (ikw<"not"> + ikw<"sz">) |
      (ikw<"not"> + ikw<"flag0_in">) | (ikw<"not"> + ikw<"flag1_in">) |
      (ikw<"not"> + ikw<"flag2_in">) | (ikw<"not"> + ikw<"flag3_in">) |
      (ikw<"not"> + ikw<"tf">) | ikw<"nbm"> | ikw<"forever"> | ikw<"true">;

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

  static constexpr auto MemDmIaMb = ikw<"dm"> + MemIaMb;
  static constexpr auto MemPmIcMd = ikw<"pm"> + MemIcMd;
  static constexpr auto MemDmMbIa = ikw<"dm"> + MemMbIa;
  static constexpr auto MemPmMdIc = ikw<"pm"> + MemMdIc;
  static constexpr auto MemDmIaInt = ikw<"dm"> + MemIaInt;
  static constexpr auto MemPmIcInt = ikw<"pm"> + MemIcInt;
  static constexpr auto MemDmIntIa = ikw<"dm"> + MemIntIa;
  static constexpr auto MemPmIntIc = ikw<"pm"> + MemIntIc;
  static constexpr auto MemDmInt = ikw<"dm"> + MemInt;
  static constexpr auto MemPmInt = ikw<"pm"> + MemInt;

  // ==================== compute ====================
  static constexpr auto RExp =
      (RReg + AddOrSub + RReg) |
      (RReg + lit<"+"> + RReg + lit<"+"> + ikw<"ci">) |
      (RReg + lit<"-"> + RReg + lit<"+"> + ikw<"ci"> + lit<"-"> + ref<NUM>) |
      (lit<"("> + RReg + lit<"+"> + RReg + lit<")"> + lit<"/"> + ref<NUM>) |
      (RReg + lit<"+"> + ikw<"ci">) |
      (RReg + lit<"+"> + ikw<"ci"> + lit<"-"> + ref<NUM>) |
      (RReg + lit<"+"> + ref<NUM>) | (RReg + lit<"-"> + ref<NUM>) |
      (lit<"-"> + RReg) | (ikw<"abs"> + RReg) | (ikw<"pass"> + RReg) |
      (RReg + ikw<"and"> + RReg) | (RReg + ikw<"or"> + RReg) |
      (RReg + ikw<"xor"> + RReg) | (ikw<"not"> + RReg) |
      (ikw<"min"> + lit<"("> + RReg + lit<","> + RReg + lit<")">) |
      (ikw<"max"> + lit<"("> + RReg + lit<","> + RReg + lit<")">) |
      (ikw<"clip"> + RReg + ikw<"by"> + RReg) | (ikw<"mant"> + FReg) |
      (ikw<"logb"> + FReg) | (ikw<"fix"> + FReg + option(ikw<"by"> + RReg)) |
      (ikw<"trunc"> + FReg + option(ikw<"by"> + RReg));

  static constexpr auto FixpointAluOp =
      (RReg + lit<"="> + RExp) |
      (ikw<"comp"> + lit<"("> + RReg + lit<","> + RReg + lit<")">);

  static constexpr auto FExp =
      (FReg + lit<"+"> + FReg) | (FReg + lit<"-"> + FReg) |
      (ikw<"abs"> + lit<"("> + FReg + lit<"+"> + FReg + lit<")">) |
      (ikw<"abs"> + lit<"("> + FReg + lit<"-"> + FReg + lit<")">) |
      (lit<"("> + FReg + lit<"+"> + FReg + lit<")"> + lit<"/"> + ref<NUM>) |
      (lit<"-"> + FReg) | (ikw<"abs"> + FReg) | (ikw<"pass"> + FReg) |
      (ikw<"rnd"> + FReg) | (ikw<"scalb"> + FReg + ikw<"by"> + RReg) |
      (ikw<"float"> + RReg + option(ikw<"by"> + RReg)) |
      (ikw<"recips"> + FReg) | (ikw<"rsqrts"> + FReg) |
      (FReg + ikw<"copysign"> + FReg) |
      (ikw<"min"> + lit<"("> + FReg + lit<","> + FReg + lit<")">) |
      (ikw<"max"> + lit<"("> + FReg + lit<","> + FReg + lit<")">) |
      (ikw<"clip"> + FReg + ikw<"by"> + FReg) | (FReg + lit<"*"> + FReg);

  static constexpr auto FloatingAluOp =
      (FReg + lit<"="> + FExp) |
      (ikw<"comp"> + lit<"("> + FReg + lit<","> + FReg + lit<")">);

  static constexpr auto MultiMod1_ =
      lit<"("> + (ikw<"si"> | ikw<"ui"> | ikw<"sf"> | ikw<"uf">) + lit<")">;
  static constexpr auto MultiMod2_ =
      lit<"("> +
      (ikw<"ssi"> | ikw<"sui"> | ikw<"usi"> | ikw<"uui"> | ikw<"ssf"> |
       ikw<"suf"> | ikw<"usf"> | ikw<"uuf"> | ikw<"ssfr"> | ikw<"sufr"> |
       ikw<"usfr"> | ikw<"uufr">) +
      lit<")">;
  static constexpr auto MultiR =
      RReg + lit<"*"> + RReg + option(MultiMod2_);
  static constexpr auto MultiExpR =
      MultiR | (MR + AddOrSub + MultiR) |
      (ikw<"sat"> + MR + option(MultiMod1_)) |
      (ikw<"rnd"> + MR + option(MultiMod1_)) | MR;
  static constexpr auto MultiExpMrf =
      MultiR | (ikw<"mrf"> + AddOrSub + MultiR) |
      (ikw<"sat"> + ikw<"mrf"> + option(MultiMod1_)) |
      (ikw<"rnd"> + ikw<"mrf"> + option(MultiMod1_));
  static constexpr auto MultiExpMrb =
      MultiR | (ikw<"mrb"> + AddOrSub + MultiR) |
      (ikw<"sat"> + ikw<"mrb"> + option(MultiMod1_)) |
      (ikw<"rnd"> + ikw<"mrb"> + option(MultiMod1_));

  static constexpr auto MultiOp =
      (RReg + lit<"="> + MultiExpR) | (ikw<"mrf"> + lit<"="> + MultiExpMrf) |
      (ikw<"mrb"> + lit<"="> + MultiExpMrb) | (MR + lit<"="> + ref<NUM>) |
      ((MRF | MRB) + lit<"="> + RReg) | (RReg + lit<"="> + (MRF | MRB));

  static constexpr auto SecOp = RReg | AtomSkip | (lit<"-"> + AtomSkip);
  static constexpr auto BitData = ref<NUM> + lit<":"> + ref<NUM>;
  static constexpr auto SecOp2 = RReg | BitData;

  static constexpr auto ShifterExp =
      (ikw<"lshift"> + RReg + ikw<"by"> + SecOp) |
      (RReg + ikw<"or"> + ikw<"lshift"> + RReg + ikw<"by"> + SecOp) |
      (ikw<"ashift"> + RReg + ikw<"by"> + SecOp) |
      (RReg + ikw<"or"> + ikw<"ashift"> + RReg + ikw<"by"> + SecOp) |
      (ikw<"rot"> + RReg + ikw<"by"> + SecOp) |
      (ikw<"bclr"> + RReg + ikw<"by"> + SecOp) |
      (ikw<"bset"> + RReg + ikw<"by"> + SecOp) |
      (ikw<"btgl"> + RReg + ikw<"by"> + SecOp) |
      (ikw<"fdep"> + RReg + ikw<"by"> + SecOp2 +
       option(lit<"("> + ikw<"se"> + lit<")">)) |
      (ikw<"fext"> + RReg + ikw<"by"> + SecOp2 +
       option(lit<"("> + ikw<"se"> + lit<")">)) |
      (RReg + ikw<"or"> + ikw<"fdep"> + RReg + ikw<"by"> + SecOp2) |
      (ikw<"exp"> + RReg + option(lit<"("> + ikw<"ex"> + lit<")">)) |
      (ikw<"leftz"> + RReg) | (ikw<"lefto"> + RReg) | (ikw<"fpack"> + FReg);

  static constexpr auto ShifterOp =
      (RReg + lit<"="> + ShifterExp) | (ikw<"btst"> + RReg + ikw<"by"> + SecOp) |
      (FReg + lit<"="> + ikw<"funpack"> + RReg);

  static constexpr auto DualAddR =
      RReg + lit<"="> + RReg + lit<"+"> + RReg + lit<","> + RReg + lit<"="> +
      RReg + lit<"-"> + RReg;
  static constexpr auto ParallelMulti =
      (MultiOp + some(lit<","> + FixpointAluOp)) |
      (FloatingAluOp + some(lit<","> + FloatingAluOp));
  static constexpr auto DualOp = DualAddR | ParallelMulti;

  // A converted data rule supplies each instruction payload.
  struct ComputeBody
      : rule<as<instruction_text>(DualOp | FixpointAluOp | FloatingAluOp | MultiOp | ShifterOp)> {};

  struct Compute
      : rule<create<ast::Compute>(
            assign<&ast::Compute::raw>(ref<ComputeBody>))> {};

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
      ikw<"modify"> +
      ((lit<"("> + IA + lit<","> + MB + lit<")">) |
       (lit<"("> + IC + lit<","> + MD + lit<")">));

  struct ComputeMovExpBody
      : rule<as<instruction_text>(option(ref<ComputeBody> + lit<",">) +
             (MovExp1 | MovExp3a | MovExp3b | MovExp3c | MovExp3d | MovExp4a |
              MovExp4b | MovExp4c | MovExp4d | MovExp5 | MovExp7))> {};

  struct ComputeMovExpr
      : rule<create<ast::ComputeMovExpr>(
            assign<&ast::ComputeMovExpr::raw>(ref<ComputeMovExpBody>))> {
  };

  struct IfComputeMovBody
      : rule<as<instruction_text>(ikw<"if"> + Condition +
             (ref<ComputeMovExpBody> | ref<ComputeBody>))> {};

  struct IfComputeMov
      : rule<create<ast::IfComputeMov>(
            assign<&ast::IfComputeMov::raw>(ref<IfComputeMovBody>))> {};

  // ==================== flow control ====================
  static constexpr auto JumpAddrPc =
      lit<"("> + ikw<"pc"> + lit<","> + ValueSkip + lit<")">;
  static constexpr auto JumpAddrInt = ValueSkip;
  static constexpr auto JumpAddrIntOrPc = JumpAddrInt | JumpAddrPc;
  static constexpr auto FlowControl10Frag = MemMdIc | JumpAddrPc;

  static constexpr auto JumpModifier =
      lit<"("> +
      ((ikw<"db"> + option(lit<","> + (ikw<"la"> | ikw<"ci">))) | ikw<"la"> |
       ikw<"ci">) +
      lit<")">;
  static constexpr auto JumpModifier2 = lit<"("> + ikw<"db"> + lit<")">;
  static constexpr auto JumpModifier3 =
      lit<"("> + ((ikw<"db"> + option(lit<","> + ikw<"lr">)) | ikw<"lr">) +
      lit<")">;

  static constexpr auto FlowControl8a =
      ikw<"jump"> + JumpAddrInt + option(JumpModifier);
  static constexpr auto FlowControl8b =
      ikw<"call"> + JumpAddrInt + option(JumpModifier2);
  static constexpr auto FlowControl9a =
      ikw<"jump"> + FlowControl10Frag + option(JumpModifier) +
      option(lit<","> + ref<ComputeBody>);
  static constexpr auto FlowControl9b =
      ikw<"call"> + FlowControl10Frag + option(JumpModifier2) +
      option(lit<","> + ref<ComputeBody>);
  static constexpr auto FlowControl11a =
      ikw<"rts"> + option(JumpModifier3) + option(lit<","> + ref<ComputeBody>);
  static constexpr auto FlowControl11b =
      ikw<"rti"> + option(JumpModifier2) + option(lit<","> + ref<ComputeBody>);
  static constexpr auto FlowControl8Exp = FlowControl8a | FlowControl8b;
  static constexpr auto FlowControl9and11Exp =
      FlowControl9a | FlowControl9b | FlowControl11a | FlowControl11b;
  static constexpr auto FlowContorl8 = ikw<"if"> + Condition + FlowControl8Exp;
  static constexpr auto FlowControl9and11 = ikw<"if"> + Condition +
                                            FlowControl9and11Exp + lit<","> +
                                            ikw<"else"> + ref<ComputeBody>;
  static constexpr auto FlowControl10 =
      ikw<"if"> + Condition + ikw<"jump"> + FlowControl10Frag + lit<","> +
      ikw<"else"> + option(ref<ComputeBody> + lit<",">) + MovExp1_1;
  static constexpr auto LcntrV = ValueSkip | UReg;
  static constexpr auto FlowControl12 =
      ikw<"lcntr"> + lit<"="> + LcntrV + lit<","> + ikw<"do"> +
      JumpAddrIntOrPc + ikw<"until"> + ikw<"lce">;
  static constexpr auto FlowControl13 =
      ikw<"do"> + JumpAddrIntOrPc + ikw<"until"> + Condition;

  struct FlowControlBody
      : rule<as<instruction_text>(FlowContorl8 | FlowControl9and11 | FlowControl10 |
             FlowControl8a | FlowControl8b | FlowControl9a | FlowControl9b |
             FlowControl11a | FlowControl11b | FlowControl12 |
             FlowControl13)> {};

  struct FlowControl
      : rule<create<ast::FlowControl>(
            assign<&ast::FlowControl::raw>(ref<FlowControlBody>))> {};

  // ==================== immediate moves ====================
  static constexpr auto ImmMov14a = (MemDmInt | MemPmInt) + lit<"="> + UReg;
  static constexpr auto ImmMov14b = UReg + lit<"="> + (MemDmInt | MemPmInt);
  static constexpr auto ImmMov16 =
      (MemDmIaMb | MemPmIcMd) + lit<"="> + ValueSkip;
  static constexpr auto ImmMov17 = UReg2 + lit<"="> + ValueSkip;
  struct ImmMovBody
      : rule<as<instruction_text>(ImmMov14a | ImmMov14b | ImmMov16 | ImmMov17)> {};

  struct ImmMov
      : rule<create<ast::ImmMov>(
            assign<&ast::ImmMov::raw>(ref<ImmMovBody>))> {};

  // ==================== misc ====================
  static constexpr auto Misc20 =
      (ikw<"push"> | ikw<"pop">) + (ikw<"loop"> | ikw<"sts"> | ikw<"pcstk">);
  struct MiscBody
      : rule<as<instruction_text>((ikw<"bit"> +
              (ikw<"set"> | ikw<"clr"> | ikw<"tgl"> | ikw<"tst"> | ikw<"xor">) +
              SReg + ValueSkip) |
             (ikw<"bitrev"> + (MemIaInt | MemIcInt)) |
             (ikw<"modify"> + lit<"("> + IA + lit<","> + ValueSkip + lit<")">) |
             (ikw<"modify"> + lit<"("> + IC + lit<","> + ValueSkip + lit<")">) |
             (Misc20 + many(lit<","> + Misc20)) |
             (ikw<"flush"> + ikw<"cache">) | ikw<"nop"> | ikw<"idle"> |
             ikw<"idle16"> |
             (ikw<"cjump"> + JumpAddrIntOrPc + option(JumpModifier2)) |
             ikw<"rframe">)> {};

  struct Misc
      : rule<create<ast::Misc>(assign<&ast::Misc::raw>(ref<MiscBody>))> {
  };

  // ==================== declaration ====================
  static constexpr auto DeclF1 =
      some(append<&ast::Declaration::values>(ref<InitExpr>), lit<",">) |
      append<&ast::Declaration::values>(ref<StringLit>);
  static constexpr auto DeclF2 = (lit<"{"> + DeclF1 + lit<"}">) | DeclF1;

  static constexpr auto DeclExp1 =
      some(append<&ast::Declaration::names>(ref<ID>), lit<",">);
  static constexpr auto DeclExp2 =
      lit<"="> +
      some(append<&ast::Declaration::values>(ref<InitExpr>), lit<",">);
  static constexpr auto DeclExp3 =
      append<&ast::Declaration::names>(ref<ID>) +
      flag<&ast::Declaration::is_array>(lit<"[">) + lit<"]"> +
      option(lit<"="> + DeclF2);
  static constexpr auto DeclExp4 =
      append<&ast::Declaration::names>(ref<ID>) +
      flag<&ast::Declaration::is_array>(lit<"[">) +
      assign<&ast::Declaration::size>(ref<ValueExpr>) + lit<"]"> +
      option(lit<"="> + DeclF2);
  static constexpr auto DeclExp5 = append<&ast::Declaration::names>(
                                       ref<ID>) +
                                   lit<"="> +
                                   append<&ast::Declaration::values>(
                                       ref<ValueExpr>);

  struct Declaration
      : rule<create<ast::Declaration>(
            ikw<".var"> +
            (DeclExp1 | DeclExp2 | DeclExp3 | DeclExp4 | DeclExp5))> {};

  // ==================== sections and segments ====================
  static constexpr auto SegQ1 =
      lit<"/"> +
      (ikw<"pm"> | ikw<"code"> | ikw<"dm"> | ikw<"data64"> | ikw<"data">);
  static constexpr auto SegQ2 =
      lit<"/"> + (ikw<"no_init"> | ikw<"zero_init"> | ikw<"runtime_init">);
  static constexpr auto SegQ3 = lit<"/"> + ikw<"dmaonly">;
  struct SegQualifier
      : rule<(SegQ1 + option(SegQ2 | SegQ3)) | (SegQ2 + option(SegQ1 | SegQ3)) |
             (SegQ3 + option(SegQ1 | SegQ2))> {};

  struct Sec
      : rule<create<ast::SectionDecl>(
            ikw<".section"> +
            option(assign<&ast::SectionDecl::qualifier>(ref<SegQualifier>)) +
            assign<&ast::SectionDecl::name>(ref<ID>))> {};

  struct Seg
      : rule<create<ast::SegmentDecl>(
            ikw<".segment"> +
            assign<&ast::SegmentDecl::qualifier>(ref<SegQualifier>) +
            assign<&ast::SegmentDecl::name>(ref<ID>))> {};

  struct EndSeg : rule<create<ast::EndSeg> + ikw<".endseg">> {};

  // ==================== directives ====================
  // .file_attr's argument: approximated as this language's value-carrying
  // token kinds.
  struct DirectiveExp
      : rule<
            create<ast::Directive>(
                assign<&ast::Directive::keyword>(ikw<".align">) +
                append<&ast::Directive::values>(ref<ValueExpr>))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(ikw<".compress">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(ikw<".extern">) +
                  some(append<&ast::Directive::names>(ref<ID>), lit<",">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(ikw<".file">) +
                  append<&ast::Directive::strings>(ref<STRING>))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(ikw<".file_attr">) +
                  option(append<&ast::Directive::values>(ref<ValueExpr>) |
                         append<&ast::Directive::strings>(ref<STRING>) |
                         append<&ast::Directive::strings>(ref<CHARLIT>) |
                         append<&ast::Directive::names>(ref<ID>)))  //
            | create<ast::Directive>(assign<&ast::Directive::keyword>(
                  ikw<".forcecompress">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(ikw<".global">) +
                  some(append<&ast::Directive::names>(ref<ID>), lit<",">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(ikw<".import">) +
                  some(append<&ast::Directive::strings>(ref<STRING>),
                       lit<",">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(ikw<".leftmargin">) +
                  append<&ast::Directive::values>(ref<ValueExpr>))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(ikw<".list">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(ikw<".list_data">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(ikw<".list_datfile">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(ikw<".list_deftab">) +
                  append<&ast::Directive::values>(ref<ValueExpr>))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(ikw<".list_loctab">) +
                  append<&ast::Directive::values>(ref<ValueExpr>))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(ikw<".list_wrapdata">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(ikw<".newpage">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(ikw<".nocompress">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(ikw<".nolist_data">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(ikw<".nolist_datfile">))  //
            | create<ast::Directive>(assign<&ast::Directive::keyword>(
                  ikw<".nolist_wrapdata">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(ikw<".pagelength">) +
                  append<&ast::Directive::values>(ref<ValueExpr>))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(ikw<".pagewidth">) +
                  append<&ast::Directive::values>(ref<ValueExpr>))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(ikw<".precision">) +
                  option(lit<"=">) +
                  append<&ast::Directive::values>(ref<NumberLit>))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(ikw<".round_minus">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(ikw<".round_nearest">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(ikw<".round_plus">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(ikw<".round_zero">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(ikw<".previous">))  //
            | create<ast::Directive>(
                  assign<&ast::Directive::keyword>(ikw<".weak">) +
                  append<&ast::Directive::names>(ref<ID>))> {};

  // ==================== statement ====================
  struct Stmt
      : rule<ref<Compute> | ref<FlowControl> | ref<ImmMov> |
             ref<Misc> | ref<Declaration> | ref<IfComputeMov> |
             ref<ComputeMovExpr>> {};

  struct StmtAtom
      : rule<ref<Stmt> | ref<Sec> | ref<Seg> | ref<EndSeg> |
             ref<DirectiveExp>> {};

  struct Statement
      : rule<create<ast::Statement>(
            many(append<&ast::Statement::labels>(ref<ID>) + lit<":">) +
            assign<&ast::Statement::atom>(ref<StmtAtom>))> {};

  struct Program
      : rule<create<ast::Program>(
            some(append<&ast::Program::statements>(ref<Statement>) +
                 lit<";">))> {};

  using entry = Program;
  using ignored = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace sharc
