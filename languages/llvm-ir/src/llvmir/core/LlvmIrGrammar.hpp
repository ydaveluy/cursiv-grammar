// The llvmir language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "llvmir/core/ast.hpp"

namespace llvmir {

struct LlvmIr : cursiv::GrammarSpec {
  // ---- Trivia ----
  struct WS : terminal<some(cr<R"(\x00 \t\n\r)">)> {};
  struct COMMENT : terminal<lit<";"> + many(cr<R"(^\r\n)">)> {};

  struct BLOCK_COMMENT
      : terminal<lit<"/*"> + many(!lit<"*/"> + any) + lit<"*/">> {};

  // ---- Lexer fragments and terminals ----
  static constexpr auto AsciiLetter = cr<"A-Za-z">;
  static constexpr auto LetterCls = cr<"A-Za-z$._-">;
  static constexpr auto EscapeLetterCls = LetterCls | cr<R"(\\)">;
  static constexpr auto HexDigitCls = cr<"A-Fa-f0-9">;
  static constexpr auto SignCls = cr<"+-">;
  static constexpr auto NameBody = LetterCls + many(LetterCls | d);
  static constexpr auto EscapeNameBody =
      EscapeLetterCls + many(EscapeLetterCls | d);
  static constexpr auto IdBody = some(d);
  static constexpr auto QuotedStringBody =
      lit<"\""> + many(cr<R"(^")">) + lit<"\"">;
  static constexpr auto DwarfWordTail = many(AsciiLetter | d | lit<"_">);

  struct INT_LIT : terminal<(option(lit<"-">) + some(d)) |
                            (cr<"us"> + lit<"0x"> + some(HexDigitCls))> {};
  struct FLOAT_LIT
      : terminal<(option(SignCls) + some(d) + lit<"."> + many(d) +
                  option(cr<"eE"> + option(SignCls) + some(d))) |
                 (option(SignCls) + lit<"0x"> + some(HexDigitCls) + lit<"."> +
                  many(HexDigitCls) + cr<"pP"> + option(SignCls) + some(d)) |
                 (lit<"0x"> + option(cr<"KLMHR">) + some(HexDigitCls)) |
                 (lit<"f0x"> + some(HexDigitCls)) |
                 (SignCls + (lit<"inf"> | lit<"qnan"> |
                             ((lit<"nan"> | lit<"snan">)+lit<"(0x"> +
                              some(HexDigitCls) + lit<")">)))> {};
  struct WORD
      : terminal<(AsciiLetter | lit<"_">)+many(AsciiLetter | d | lit<"_">)> {};
  struct STRING_LIT : terminal<QuotedStringBody> {};
  struct GLOBAL_IDENT
      : terminal<lit<"@"> + (NameBody | QuotedStringBody | IdBody)> {};
  struct LOCAL_IDENT
      : terminal<lit<"%"> + (NameBody | QuotedStringBody | IdBody)> {};
  struct LABEL_IDENT : terminal<(some(LetterCls | d) + lit<":">) |
                                (QuotedStringBody + lit<":">)> {};
  struct SUMMARY_ID : terminal<lit<"^"> + IdBody> {};
  struct FIELD_NAME : terminal<(NameBody | QuotedStringBody) + lit<":">,
                               text_conv<[](std::string_view t) {
                                 return t.substr(0, t.size() - 1);
                               }>> {};
  struct ATTR_GROUP_ID : terminal<lit<"#"> + IdBody> {};
  struct COMDAT_NAME : terminal<lit<"$"> + (NameBody | QuotedStringBody)> {};
  struct METADATA_NAME : terminal<lit<"!"> + EscapeNameBody> {};
  struct METADATA_ID : terminal<lit<"!"> + IdBody> {};
  struct INT_TYPE : terminal<lit<"i"> + some(d)> {};
  struct BYTE_TYPE : terminal<lit<"b"> + some(d)> {};
  struct DEBUG_RECORD : terminal<lit<"#dbg_"> + NameBody> {};
  struct DWARF_TAG : terminal<lit<"DW_TAG_"> + DwarfWordTail> {};
  struct DWARF_ATT_ENCODING : terminal<lit<"DW_ATE_"> + DwarfWordTail> {};
  struct DI_FLAG : terminal<lit<"DIFlag"> + DwarfWordTail> {};
  struct DISP_FLAG : terminal<lit<"DISPFlag"> + DwarfWordTail> {};
  struct DWARF_LANG : terminal<lit<"DW_LANG_"> + DwarfWordTail> {};
  struct DWARF_CC : terminal<lit<"DW_CC_"> + DwarfWordTail> {};
  struct CHECKSUM_KIND : terminal<lit<"CSK_"> + DwarfWordTail> {};
  struct DWARF_VIRTUALITY : terminal<lit<"DW_VIRTUALITY_"> + DwarfWordTail> {};
  struct DWARF_MACINFO : terminal<lit<"DW_MACINFO_"> + DwarfWordTail> {};
  struct DWARF_OP : terminal<lit<"DW_OP_"> + DwarfWordTail> {};

  // ---- Forward declarations: every rule referenced before its own
  // definition. `ref<R>` only needs R named, never complete. ----
  struct TypeRule;
  struct ParamsRule;
  struct TypedValueRule;
  struct BareValueRule;
  struct GepIndexRule;
  struct ConstantRule;
  struct ValueRule;
  struct InlineAsmRule;
  struct CallingConvRule;
  struct StructConstRule;
  struct ArrayConstRule;
  struct VectorConstRule;
  struct MdStringRule;
  struct MdRefRule;
  struct AttrEntryRule;
  struct MdTupleRule;
  struct DiArgListRule;
  struct MdListRule;
  struct DiFlagsRule;
  struct SpFlagsRule;
  struct NamedFieldRule;
  struct SpecializedMdNodeRule;
  struct MdAttachmentRule;
  struct LabelRule;
  struct CallArgRule;
  struct OperandBundleRule;
  struct CallingConvRule;
  struct BasicBlockRule;
  struct UseListOrderRule;
  struct FuncHeaderRule;
  struct FuncBodyRule;

  // ================= Attributes =================
  // Every param/return/func attribute and every section/partition/comdat/
  // align/gc/prefix/prologue/personality/sanitizer decoration is one of a
  // handful of shapes -- see ast::Attr.
  static constexpr auto BareAttrWord =
      kw<"allocalign"> | kw<"allocptr"> | kw<"alwaysinline"> |
      kw<"argmemonly"> | kw<"builtin"> | kw<"cold"> | kw<"convergent"> |
      kw<"coro_elide_safe"> | kw<"coro_only_destroy_when_complete"> |
      kw<"dead_on_unwind"> | kw<"disable_sanitizer_instrumentation"> |
      kw<"flatten"> | kw<"fn_ret_thunk_extern"> | kw<"hot"> |
      kw<"hybrid_patchable"> | kw<"immarg"> |
      kw<"inaccessiblemem_or_argmemonly"> | kw<"inaccessiblememonly"> |
      kw<"inlinehint"> | kw<"inreg"> | kw<"jumptable"> | kw<"minsize"> |
      kw<"mustprogress"> | kw<"naked"> | kw<"nest"> |
      kw<"no_sanitize_address"> | kw<"no_sanitize_hwaddress"> | kw<"noalias"> |
      kw<"nobuiltin"> | kw<"nocallback"> | kw<"nocapture"> | kw<"nocf_check"> |
      kw<"nocreateundeforpoison"> | kw<"nodivergencesource"> |
      kw<"noduplicate"> | kw<"noext"> | kw<"nofree"> | kw<"noimplicitfloat"> |
      kw<"noinline"> | kw<"noipa"> | kw<"nomerge"> | kw<"nonlazybind"> |
      kw<"nonnull"> | kw<"nooutline"> | kw<"noprofile"> | kw<"norecurse"> |
      kw<"noredzone"> | kw<"noreturn"> | kw<"nosanitize_bounds"> |
      kw<"nosanitize_coverage"> | kw<"nosync"> | kw<"noundef"> |
      kw<"nounwind"> | kw<"null_pointer_is_valid"> | kw<"optdebug"> |
      kw<"optforfuzzing"> | kw<"optnone"> | kw<"optsize"> |
      kw<"presplitcoroutine"> | kw<"readnone"> | kw<"readonly"> |
      kw<"returned"> | kw<"returns_twice"> | kw<"safestack"> |
      kw<"sanitize_address"> | kw<"sanitize_address_dyninit"> |
      kw<"sanitize_alloc_token"> | kw<"sanitize_hwaddress"> |
      kw<"sanitize_memory"> | kw<"sanitize_memtag"> |
      kw<"sanitize_numerical_stability"> | kw<"sanitize_realtime"> |
      kw<"sanitize_realtime_blocking"> | kw<"sanitize_thread"> |
      kw<"sanitize_type"> | kw<"shadowcallstack"> | kw<"signext"> |
      kw<"skipprofile"> | kw<"speculatable"> |
      kw<"speculative_load_hardening"> | kw<"ssp"> | kw<"sspreq"> |
      kw<"sspstrong"> | kw<"strictfp"> | kw<"swiftasync"> | kw<"swifterror"> |
      kw<"swiftself"> | kw<"willreturn"> | kw<"writable"> | kw<"writeonly"> |
      kw<"zeroext">;

  struct BareAttr : rule<assign<&ast::Attr::name>(BareAttrWord)> {};
  struct StringAttr : rule<assign<&ast::Attr::name>(ref<STRING_LIT>)> {};
  struct PairAttr : rule<assign<&ast::Attr::name>(ref<STRING_LIT>) + lit<"="> +
                         assign<&ast::Attr::stringArg>(ref<STRING_LIT>)> {};
  struct AttrGroupRefAttr : rule<assign<&ast::Attr::name>(ref<ATTR_GROUP_ID>)> {
  };

  struct AlignAttr
      : rule<assign<&ast::Attr::name>(kw<"align">) +
             (assign<&ast::Attr::intArg>(ref<INT_LIT>)  //
              | (lit<"("> + assign<&ast::Attr::intArg>(ref<INT_LIT>) +
                 lit<")">))> {};
  struct AlignPairAttr : rule<assign<&ast::Attr::name>(kw<"align">) + lit<"="> +
                              assign<&ast::Attr::intArg>(ref<INT_LIT>)> {};
  struct AlignStackAttr
      : rule<assign<&ast::Attr::name>(kw<"alignstack">) + lit<"("> +
             assign<&ast::Attr::intArg>(ref<INT_LIT>) + lit<")">> {};
  struct AlignStackPairAttr
      : rule<assign<&ast::Attr::name>(kw<"alignstack">) + lit<"="> +
             assign<&ast::Attr::intArg>(ref<INT_LIT>)> {};
  struct AllocKindAttr
      : rule<assign<&ast::Attr::name>(kw<"allockind">) + lit<"("> +
             assign<&ast::Attr::stringArg>(ref<STRING_LIT>) + lit<")">> {};
  struct AllocSizeAttr
      : rule<assign<&ast::Attr::name>(kw<"allocsize">) + lit<"("> +
             assign<&ast::Attr::intArg>(ref<INT_LIT>) +
             option(lit<","> + assign<&ast::Attr::intArg2>(ref<INT_LIT>)) +
             lit<")">> {};
  struct UnwindTableAttr
      : rule<assign<&ast::Attr::name>(kw<"uwtable">) +
             option(lit<"("> +
                    assign<&ast::Attr::stringArg>(kw<"async"> | kw<"sync">) +
                    lit<")">)> {};
  struct VectorScaleRangeAttr
      : rule<assign<&ast::Attr::name>(kw<"vscale_range">) +
             option(
                 lit<"("> + assign<&ast::Attr::intArg>(ref<INT_LIT>) +
                 option(lit<","> + assign<&ast::Attr::intArg2>(ref<INT_LIT>)) +
                 lit<")">)> {};
  struct ByRefAttr
      : rule<assign<&ast::Attr::name>(kw<"byref">) + lit<"("> +
             assign<&ast::Attr::typeArg>(ref<TypeRule>) + lit<")">> {};
  struct ByvalAttr
      : rule<assign<&ast::Attr::name>(kw<"byval">) +
             option(lit<"("> + assign<&ast::Attr::typeArg>(ref<TypeRule>) +
                    lit<")">)> {};
  struct DereferenceableAttr
      : rule<assign<&ast::Attr::name>(kw<"dereferenceable"> |
                                      kw<"dereferenceable_or_null">) +
             lit<"("> + assign<&ast::Attr::intArg>(ref<INT_LIT>) + lit<")">> {};
  struct ElementTypeAttr
      : rule<assign<&ast::Attr::name>(kw<"elementtype">) + lit<"("> +
             assign<&ast::Attr::typeArg>(ref<TypeRule>) + lit<")">> {};
  struct InAllocaAttr
      : rule<assign<&ast::Attr::name>(kw<"inalloca">) + lit<"("> +
             assign<&ast::Attr::typeArg>(ref<TypeRule>) + lit<")">> {};
  struct PreallocatedAttr
      : rule<assign<&ast::Attr::name>(kw<"preallocated">) + lit<"("> +
             assign<&ast::Attr::typeArg>(ref<TypeRule>) + lit<")">> {};
  struct StructRetAttr
      : rule<assign<&ast::Attr::name>(kw<"sret">) + lit<"("> +
             assign<&ast::Attr::typeArg>(ref<TypeRule>) + lit<")">> {};
  struct SectionAttr : rule<assign<&ast::Attr::name>(kw<"section">) +
                            assign<&ast::Attr::stringArg>(ref<STRING_LIT>)> {};
  struct PartitionAttr : rule<assign<&ast::Attr::name>(kw<"partition">) +
                              assign<&ast::Attr::stringArg>(ref<STRING_LIT>)> {
  };
  struct ComdatAttr
      : rule<assign<&ast::Attr::name>(kw<"comdat">) +
             option(lit<"("> + assign<&ast::Attr::stringArg>(ref<COMDAT_NAME>) +
                    lit<")">)> {};
  struct GcAttr : rule<assign<&ast::Attr::name>(kw<"gc">) +
                       assign<&ast::Attr::stringArg>(ref<STRING_LIT>)> {};
  struct PrefixAttr : rule<assign<&ast::Attr::name>(kw<"prefix">) +
                           assign<&ast::Attr::valueArg>(ref<TypedValueRule>)> {
  };
  struct PrologueAttr
      : rule<assign<&ast::Attr::name>(kw<"prologue">) +
             assign<&ast::Attr::valueArg>(ref<TypedValueRule>)> {};
  struct PersonalityAttr
      : rule<assign<&ast::Attr::name>(kw<"personality">) +
             assign<&ast::Attr::valueArg>(ref<TypedValueRule>)> {};
  // sanitizerKind's remaining word (globalField only, to keep BareAttrWord
  // free of a duplicate already-listed spelling)
  struct SanitizeMemtagAttr
      : rule<assign<&ast::Attr::name>(kw<"sanitize_memtag">)> {};

  struct AttrEntryRule
      : rule<(flag<&ast::AttrEntry::grouped>(lit<"(">) +
              many(append<&ast::AttrEntry::entries>(ref<AttrEntryRule>),
                   lit<",">) +
              lit<")">) |
             (option(assign<&ast::AttrEntry::name>(ref<WORD>) + lit<":">) +
              some(append<&ast::AttrEntry::values>(ref<WORD> | ref<INT_LIT> |
                                                   lit<"|">)))> {};
  struct CompoundAttr
      : rule<assign<&ast::Attr::name>(kw<"memory"> | kw<"captures"> |
                                      kw<"nofpclass"> | kw<"initializes"> |
                                      kw<"denormal_fpenv">) +
             lit<"("> +
             many(append<&ast::Attr::entries>(ref<AttrEntryRule>), lit<",">) +
             lit<")">> {};
  struct RangeAttr
      : rule<assign<&ast::Attr::name>(kw<"range">) + lit<"("> +
             assign<&ast::Attr::typeArg>(ref<TypeRule>) +
             assign<&ast::Attr::intArg>(ref<INT_LIT>) + lit<","> +
             assign<&ast::Attr::intArg2>(ref<INT_LIT>) + lit<")">> {};
  struct DeadOnReturnAttr
      : rule<assign<&ast::Attr::name>(kw<"dead_on_return">) +
             option(lit<"("> + assign<&ast::Attr::intArg>(ref<INT_LIT>) +
                    lit<")">)> {};
  struct CodeModelAttr : rule<assign<&ast::Attr::name>(kw<"code_model">) +
                              assign<&ast::Attr::stringArg>(ref<STRING_LIT>)> {
  };
  static constexpr auto Attribute =
      ref<StringAttr> | ref<PairAttr> | ref<AlignAttr> | ref<AlignStackAttr> |
      ref<ByRefAttr> | ref<ByvalAttr> | ref<DereferenceableAttr> |
      ref<ElementTypeAttr> | ref<InAllocaAttr> | ref<BareAttr> |
      ref<PreallocatedAttr> | ref<StructRetAttr> | ref<AttrGroupRefAttr> |
      ref<AlignPairAttr> | ref<AlignStackPairAttr> | ref<AllocKindAttr> |
      ref<AllocSizeAttr> | ref<UnwindTableAttr> | ref<VectorScaleRangeAttr> |
      ref<CompoundAttr> | ref<RangeAttr> | ref<DeadOnReturnAttr>;
  static constexpr auto ParamAttribute = Attribute;
  static constexpr auto ReturnAttribute = Attribute;
  static constexpr auto FuncAttribute = Attribute;
  static constexpr auto GlobalField =
      ref<SectionAttr> | ref<PartitionAttr> | ref<ComdatAttr> | ref<AlignAttr> |
      ref<BareAttr> | ref<SanitizeMemtagAttr> | ref<CodeModelAttr>;
  static constexpr auto FuncHdrField =
      FuncAttribute | ref<SectionAttr> | ref<PartitionAttr> | ref<ComdatAttr> |
      ref<AlignAttr> | ref<GcAttr> | ref<PrefixAttr> | ref<PrologueAttr> |
      ref<PersonalityAttr>;

  // ================= Types =================
  static constexpr auto FloatKindWord =
      kw<"half"> | kw<"bfloat"> | kw<"float"> | kw<"double"> | kw<"x86_fp80"> |
      kw<"fp128"> | kw<"ppc_fp128">;

  struct ByteTypeRule : rule<assign<&ast::ByteType::text>(ref<BYTE_TYPE>)> {};
  struct TargetTypeRule
      : rule<kw<"target"> + lit<"("> +
             assign<&ast::TargetType::name>(ref<STRING_LIT>) +
             many(lit<","> + append<&ast::TargetType::parameters>(
                                 ref<TypeRule> | ref<INT_LIT>)) +
             lit<")">> {};
  struct IntTypeRule : rule<assign<&ast::IntType::text>(ref<INT_TYPE>)> {};
  struct OpaquePointerTypeRule
      : rule<kw<"ptr"> +
             option(kw<"addrspace"> + lit<"("> +
                    assign<&ast::PointerType::addrSpace>(ref<INT_LIT>) +
                    lit<")">)> {};
  struct VectorTypeRule
      : rule<(lit<"<"> + assign<&ast::VectorType::length>(ref<INT_LIT>) +
              kw<"x"> + assign<&ast::VectorType::element>(ref<TypeRule>) +
              lit<">">)  //
             | (lit<"<"> + flag<&ast::VectorType::scalable>(kw<"vscale">) +
                kw<"x"> + assign<&ast::VectorType::length>(ref<INT_LIT>) +
                kw<"x"> + assign<&ast::VectorType::element>(ref<TypeRule>) +
                lit<">">)> {};
  struct ArrayTypeRule
      : rule<lit<"["> + assign<&ast::ArrayType::length>(ref<INT_LIT>) +
             kw<"x"> + assign<&ast::ArrayType::element>(ref<TypeRule>) +
             lit<"]">> {};
  struct StructTypeRule
      : rule<(lit<"{"> +
              many(append<&ast::StructType::members>(ref<TypeRule>), lit<",">) +
              lit<"}">)  //
             |
             (flag<&ast::StructType::packed>(lit<"<">) + lit<"{"> +
              many(append<&ast::StructType::members>(ref<TypeRule>), lit<",">) +
              lit<"}"> + lit<">">)> {};

  struct TypeBase
      : rule<create<ast::VoidType>(kw<"void">)                             //
             | create<ast::OpaqueType>(kw<"opaque">)                       //
             | ref<ByteTypeRule> | ref<TargetTypeRule> | ref<IntTypeRule>  //
             | create<ast::FloatType>(
                   assign<&ast::FloatType::keyword>(FloatKindWord))  //
             | ref<OpaquePointerTypeRule>                            //
             | ref<VectorTypeRule>                                   //
             | create<ast::LabelType>(kw<"label">)                   //
             | ref<ArrayTypeRule>                                    //
             | ref<StructTypeRule>                                   //
             | create<ast::NamedType>(
                   assign<&ast::NamedType::name>(ref<LOCAL_IDENT>))  //
             | create<ast::AmxType>(kw<"x86_amx">) |
             create<ast::MmxType>(kw<"x86_mmx">)    //
             | create<ast::TokenType>(kw<"token">)  //
             | create<ast::MetadataType>(kw<"metadata">)> {};

  struct TypeRule
      : rule<ref<TypeBase> +
             many(
                 nest<ast::PointerType, &ast::PointerType::pointee>(
                     option(kw<"addrspace"> + lit<"("> +
                            assign<&ast::PointerType::addrSpace>(ref<INT_LIT>) +
                            lit<")">) +
                     lit<"*">)  //
                 | nest<ast::FunctionType, &ast::FunctionType::returnType>(
                       lit<"("> +
                       assign<&ast::FunctionType::params>(ref<ParamsRule>) +
                       lit<")">))> {};

  struct ParamRule : rule<assign<&ast::Param::type>(ref<TypeRule>) +
                          many(append<&ast::Param::attrs>(ParamAttribute)) +
                          option(assign<&ast::Param::name>(ref<LOCAL_IDENT>))> {
  };
  struct ParamsRule
      : rule<flag<&ast::Params::variadic>(lit<"...">)  //
             | (some(append<&ast::Params::params>(ref<ParamRule>), lit<",">) +
                option(lit<","> +
                       flag<&ast::Params::variadic>(lit<"...">)))  //
             | create<ast::Params>> {};

  // ================= Values and constants =================
  struct TypedValueRule
      : rule<assign<&ast::TypedValue::type>(ref<TypeRule>) +
             assign<&ast::TypedValue::value>(ref<ValueRule>)> {};
  struct GepIndexRule
      : rule<option(flag<&ast::TypedValue::inRange>(kw<"inrange">)) +
             assign<&ast::TypedValue::type>(ref<TypeRule>) +
             assign<&ast::TypedValue::value>(ref<ConstantRule>)> {};
  struct BareValueRule : rule<assign<&ast::TypedValue::value>(ref<ValueRule>)> {
  };

  struct InlineAsmRule
      : rule<kw<"asm"> +
             option(flag<&ast::InlineAsm::sideEffect>(kw<"sideeffect">)) +
             option(flag<&ast::InlineAsm::alignStack>(kw<"alignstack">)) +
             option(
                 flag<&ast::InlineAsm::intelDialect>(kw<"inteldialect">)) +
             option(flag<&ast::InlineAsm::unwind>(kw<"unwind">)) +
             assign<&ast::InlineAsm::code>(ref<STRING_LIT>) + lit<","> +
             assign<&ast::InlineAsm::constraints>(ref<STRING_LIT>)> {};

  static constexpr auto GepFlagWord = kw<"inbounds"> | kw<"nuw"> | kw<"nusw">;
  static constexpr auto OverflowFlagWord = kw<"nsw"> | kw<"nuw">;
  static constexpr auto IPredWord =
      kw<"eq"> | kw<"ne"> | kw<"sge"> | kw<"sgt"> | kw<"sle"> | kw<"slt"> |
      kw<"uge"> | kw<"ugt"> | kw<"ule"> | kw<"ult">;
  static constexpr auto FPredWord =
      kw<"false"> | kw<"oeq"> | kw<"oge"> | kw<"ogt"> | kw<"ole"> | kw<"olt"> |
      kw<"one"> | kw<"ord"> | kw<"true"> | kw<"ueq"> | kw<"uge"> | kw<"ugt"> |
      kw<"ule"> | kw<"ult"> | kw<"une"> | kw<"uno">;

  template <cursiv::str Kw>
  struct ConstBin2
      : rule<assign<&ast::ConstantExpr::op>(kw<Kw>) + lit<"("> +
             append<&ast::ConstantExpr::operands>(ref<TypedValueRule>) +
             lit<","> +
             append<&ast::ConstantExpr::operands>(ref<TypedValueRule>) +
             lit<")">> {};
  template <cursiv::str Kw>
  struct ConstBin2Ovf
      : rule<assign<&ast::ConstantExpr::op>(kw<Kw>) +
             many(append<&ast::ConstantExpr::flags>(OverflowFlagWord)) +
             lit<"("> +
             append<&ast::ConstantExpr::operands>(ref<TypedValueRule>) +
             lit<","> +
             append<&ast::ConstantExpr::operands>(ref<TypedValueRule>) +
             lit<")">> {};
  template <cursiv::str Kw>
  struct ConstBin2Exact
      : rule<assign<&ast::ConstantExpr::op>(kw<Kw>) +
             option(append<&ast::ConstantExpr::flags>(kw<"exact">)) + lit<"("> +
             append<&ast::ConstantExpr::operands>(ref<TypedValueRule>) +
             lit<","> +
             append<&ast::ConstantExpr::operands>(ref<TypedValueRule>) +
             lit<")">> {};
  template <cursiv::str Kw>
  struct ConstBin3
      : rule<assign<&ast::ConstantExpr::op>(kw<Kw>) + lit<"("> +
             append<&ast::ConstantExpr::operands>(ref<TypedValueRule>) +
             lit<","> +
             append<&ast::ConstantExpr::operands>(ref<TypedValueRule>) +
             lit<","> +
             append<&ast::ConstantExpr::operands>(ref<TypedValueRule>) +
             lit<")">> {};
  template <cursiv::str Kw>
  struct ConstConv
      : rule<assign<&ast::ConstantExpr::op>(kw<Kw>) + lit<"("> +
             append<&ast::ConstantExpr::operands>(ref<TypedValueRule>) +
             kw<"to"> + assign<&ast::ConstantExpr::destType>(ref<TypeRule>) +
             lit<")">> {};

  struct ConstFNeg
      : rule<assign<&ast::ConstantExpr::op>(kw<"fneg">) + lit<"("> +
             append<&ast::ConstantExpr::operands>(ref<TypedValueRule>) +
             lit<")">> {};
  struct ConstGetElementPtr
      : rule<assign<&ast::ConstantExpr::op>(kw<"getelementptr">) +
             many(append<&ast::ConstantExpr::flags>(GepFlagWord)) +
             option(kw<"inrange"> + lit<"("> +
                    assign<&ast::ConstantExpr::inRangeStart>(ref<INT_LIT>) +
                    lit<","> +
                    assign<&ast::ConstantExpr::inRangeEnd>(ref<INT_LIT>) +
                    lit<")">) +
             lit<"("> + assign<&ast::ConstantExpr::gepType>(ref<TypeRule>) +
             lit<","> +
             append<&ast::ConstantExpr::operands>(ref<TypedValueRule>) +
             many(lit<","> +
                  append<&ast::ConstantExpr::operands>(ref<GepIndexRule>)) +
             lit<")">> {};
  struct ConstICmp
      : rule<assign<&ast::ConstantExpr::op>(kw<"icmp">) +
             assign<&ast::ConstantExpr::predicate>(IPredWord) + lit<"("> +
             append<&ast::ConstantExpr::operands>(ref<TypedValueRule>) +
             lit<","> +
             append<&ast::ConstantExpr::operands>(ref<TypedValueRule>) +
             lit<")">> {};
  struct ConstFCmp
      : rule<assign<&ast::ConstantExpr::op>(kw<"fcmp">) +
             assign<&ast::ConstantExpr::predicate>(FPredWord) + lit<"("> +
             append<&ast::ConstantExpr::operands>(ref<TypedValueRule>) +
             lit<","> +
             append<&ast::ConstantExpr::operands>(ref<TypedValueRule>) +
             lit<")">> {};

  struct ConstSplat
      : rule<assign<&ast::ConstantExpr::op>(kw<"splat">) + lit<"("> +
             append<&ast::ConstantExpr::operands>(ref<TypedValueRule>) +
             lit<")">> {};
  struct ConstPtrAuth
      : rule<assign<&ast::ConstantExpr::op>(kw<"ptrauth">) + lit<"("> +
             many(append<&ast::ConstantExpr::operands>(ref<TypedValueRule>),
                  lit<",">) +
             lit<")">> {};
  static constexpr auto ConstantExprAlt =
      ref<ConstSplat> | ref<ConstPtrAuth> | ref<ConstFNeg> |
      ref<ConstBin2Ovf<"add">> | ref<ConstBin2Ovf<"sub">> |
      ref<ConstBin2Ovf<"mul">> | ref<ConstBin2Ovf<"shl">> |
      ref<ConstBin2Exact<"lshr">> | ref<ConstBin2Exact<"ashr">> |
      ref<ConstBin2<"and">> | ref<ConstBin2<"or">> | ref<ConstBin2<"xor">> |
      ref<ConstBin2<"extractelement">> | ref<ConstBin3<"insertelement">> |
      ref<ConstBin3<"shufflevector">> | ref<ConstGetElementPtr> |
      ref<ConstConv<"trunc">> | ref<ConstConv<"zext">> |
      ref<ConstConv<"sext">> | ref<ConstConv<"fptrunc">> |
      ref<ConstConv<"fpext">> | ref<ConstConv<"fptoui">> |
      ref<ConstConv<"fptosi">> | ref<ConstConv<"uitofp">> |
      ref<ConstConv<"sitofp">> | ref<ConstConv<"ptrtoaddr">> |
      ref<ConstConv<"ptrtoint">> | ref<ConstConv<"inttoptr">> |
      ref<ConstConv<"bitcast">> | ref<ConstConv<"addrspacecast">> |
      ref<ConstICmp> | ref<ConstFCmp> | ref<ConstBin3<"select">>;

  struct StructConstRule
      : rule<(lit<"{"> +
              many(append<&ast::StructConst::members>(ref<TypedValueRule>),
                   lit<",">) +
              lit<"}">)  //
             | (flag<&ast::StructConst::packed>(lit<"<">) + lit<"{"> +
                many(append<&ast::StructConst::members>(ref<TypedValueRule>),
                     lit<",">) +
                lit<"}"> + lit<">">)> {};
  struct ArrayConstRule
      : rule<(kw<"c"> + assign<&ast::ArrayConst::cString>(ref<STRING_LIT>))  //
             | (lit<"["> +
                many(append<&ast::ArrayConst::elements>(ref<TypedValueRule>),
                     lit<",">) +
                lit<"]">)> {};
  struct VectorConstRule
      : rule<lit<"<"> +
             many(append<&ast::VectorConst::elements>(ref<TypedValueRule>),
                  lit<",">) +
             lit<">">> {};

  struct ConstantRule
      : rule<create<ast::BoolConst>(
                 assign<&ast::BoolConst::value>(kw<"true"> | kw<"false">))  //
             | create<ast::IntConst>(
                   assign<&ast::IntConst::text>(ref<INT_LIT>))  //
             | create<ast::FloatConst>(
                   assign<&ast::FloatConst::text>(ref<FLOAT_LIT>))  //
             | create<ast::KeywordConst>(
                   assign<&ast::KeywordConst::keyword>(kw<"null">))  //
             | create<ast::KeywordConst>(
                   assign<&ast::KeywordConst::keyword>(kw<"none">))  //
             | ref<StructConstRule>                                  //
             | ref<ArrayConstRule>                                   //
             | ref<VectorConstRule>                                  //
             | create<ast::KeywordConst>(assign<&ast::KeywordConst::keyword>(
                   kw<"zeroinitializer">))  //
             | create<ast::Ident>(
                   assign<&ast::Ident::name>(ref<GLOBAL_IDENT>))  //
             | create<ast::KeywordConst>(
                   assign<&ast::KeywordConst::keyword>(kw<"undef">))  //
             | create<ast::KeywordConst>(
                   assign<&ast::KeywordConst::keyword>(kw<"poison">))  //
             |
             create<ast::BlockAddressConst>(
                 kw<"blockaddress"> + lit<"("> +
                 assign<&ast::BlockAddressConst::function>(ref<GLOBAL_IDENT>) +
                 lit<","> +
                 assign<&ast::BlockAddressConst::block>(ref<LOCAL_IDENT>) +
                 lit<")">)  //
             | create<ast::DsoLocalEquivalentConst>(
                   kw<"dso_local_equivalent"> +
                   assign<&ast::DsoLocalEquivalentConst::target>(
                       ref<GLOBAL_IDENT>))  //
             | create<ast::NoCfiConst>(
                   kw<"no_cfi"> +
                   assign<&ast::NoCfiConst::target>(ref<GLOBAL_IDENT>))  //
             | ConstantExprAlt> {};

  struct ValueRule
      : rule<ref<ConstantRule>  //
             |
             create<ast::Ident>(assign<&ast::Ident::name>(ref<LOCAL_IDENT>))  //
             | ref<InlineAsmRule>> {};

  struct CallingConvRule
      : rule<(assign<&ast::CallingConv::keyword>(kw<"riscv_vls_cc">) +
              option(lit<"("> +
                     assign<&ast::CallingConv::intArg>(ref<INT_LIT>) +
                     lit<")">)) |
             assign<&ast::CallingConv::keyword>(
                 kw<"aarch64_sme_preservemost_from_x0"> |
                 kw<"aarch64_sme_preservemost_from_x1"> |
                 kw<"aarch64_sme_preservemost_from_x2"> |
                 kw<"aarch64_sve_vector_pcs"> | kw<"aarch64_vector_pcs"> |
                 kw<"amdgpu_cs"> | kw<"amdgpu_cs_chain"> |
                 kw<"amdgpu_cs_chain_preserve"> | kw<"amdgpu_es"> |
                 kw<"amdgpu_gfx"> | kw<"amdgpu_gfx_whole_wave"> |
                 kw<"amdgpu_gs"> | kw<"amdgpu_hs"> | kw<"amdgpu_kernel"> |
                 kw<"amdgpu_ls"> | kw<"amdgpu_ps"> | kw<"amdgpu_vs"> |
                 kw<"anyregcc"> | kw<"arm_aapcs_vfpcc"> | kw<"arm_aapcscc"> |
                 kw<"arm_apcscc"> | kw<"avr_intrcc"> | kw<"avr_signalcc"> |
                 kw<"ccc"> | kw<"cfguard_checkcc"> |
                 kw<"cheriot_compartmentcallcc"> |
                 kw<"cheriot_compartmentcalleecc"> |
                 kw<"cheriot_librarycallcc"> | kw<"coldcc"> |
                 kw<"cxx_fast_tlscc"> | kw<"fastcc"> | kw<"ghccc"> |
                 kw<"graalcc"> | kw<"hhvm_ccc"> | kw<"hhvmcc"> |
                 kw<"intel_ocl_bicc"> | kw<"m68k_rtdcc"> | kw<"msp430_intrcc"> |
                 kw<"preserve_allcc"> | kw<"preserve_mostcc"> |
                 kw<"preserve_nonecc"> | kw<"ptx_device"> | kw<"ptx_kernel"> |
                 kw<"riscv_vector_cc"> | kw<"spir_func"> | kw<"spir_kernel"> |
                 kw<"swiftcc"> | kw<"swifttailcc"> | kw<"tailcc"> |
                 kw<"webkit_jscc"> | kw<"win64cc"> | kw<"x86_64_sysvcc"> |
                 kw<"x86_fastcallcc"> | kw<"x86_intrcc"> | kw<"x86_regcallcc"> |
                 kw<"x86_stdcallcc"> | kw<"x86_thiscallcc"> |
                 kw<"x86_vectorcallcc">) |
             (lit<"cc"> + assign<&ast::CallingConv::intArg>(ref<INT_LIT>))> {};

  // ================= Metadata =================
  // Every `!DIXxx(...)` node is a kind plus `name: value` fields; every field
  // value is one of a handful of shapes. See ast::NamedField / ast.hpp.
  static constexpr auto NamedFieldValue =
      ref<TypedValueRule> | ref<MdStringRule> | ref<MdTupleRule> |
      ref<MdRefRule> | ref<DiArgListRule> | ref<SpecializedMdNodeRule> |
      ref<STRING_LIT> | ref<INT_LIT> | ref<WORD> | ref<DiFlagsRule> |
      ref<SpFlagsRule> | ref<MdListRule>;

  struct MdRefRule : rule<assign<&ast::MdRef::id>(ref<METADATA_ID>)> {};
  struct MdStringRule
      : rule<lit<"!"> + assign<&ast::MdString::text>(ref<STRING_LIT>)> {};
  struct DiArgListRule
      : rule<lit<"!DIArgList"> + lit<"("> +
             many(append<&ast::DiArgList::args>(ref<TypedValueRule>),
                  lit<",">) +
             lit<")">> {};
  struct MdTupleRule
      : rule<lit<"!"> + lit<"{"> +
             many(append<&ast::MdTuple::elements>(NamedFieldValue), lit<",">) +
             lit<"}">> {};
  struct MdListRule
      : rule<lit<"{"> +
             many(append<&ast::MdList::items>(NamedFieldValue), lit<",">) +
             lit<"}">> {};
  struct DiFlagsRule
      : rule<some(append<&ast::MdList::items>(ref<DI_FLAG>), lit<"|">)> {};
  struct SpFlagsRule
      : rule<some(append<&ast::MdList::items>(ref<INT_LIT> | ref<DISP_FLAG>),
                  lit<"|">)> {};
  struct NamedFieldRule
      : rule<assign<&ast::NamedField::name>(ref<FIELD_NAME>) +
             assign<&ast::NamedField::value>(NamedFieldValue)> {};

  struct SpecializedMdNodeRule
      : rule<assign<&ast::SpecializedMdNode::kind>(ref<METADATA_NAME>) +
             lit<"("> +
             many(append<&ast::SpecializedMdNode::fields>(ref<NamedFieldRule>) |
                      append<&ast::SpecializedMdNode::positional>(ref<INT_LIT> |
                                                                  ref<WORD>),
                  lit<",">) +
             lit<")">> {};

  static constexpr auto MdNode =
      ref<MdTupleRule> | ref<MdRefRule> | ref<SpecializedMdNodeRule>;
  struct MdAttachmentRule
      : rule<assign<&ast::MdAttachment::name>(ref<METADATA_NAME>) +
             assign<&ast::MdAttachment::node>(MdNode)> {};

  template <auto Member>
  static constexpr auto MdAttachments =
      many(lit<","> + append<Member>(ref<MdAttachmentRule>));

  // ================= Instructions and terminators =================
  struct LabelRule
      : rule<kw<"label"> + assign<&ast::Label::name>(ref<LOCAL_IDENT>)> {};

  static constexpr auto FastMathFlagWord =
      kw<"afn"> | kw<"arcp"> | kw<"contract"> | kw<"fast"> | kw<"ninf"> |
      kw<"nnan"> | kw<"nsz"> | kw<"reassoc">;
  static constexpr auto AtomicOrderingWord = kw<"acq_rel"> | kw<"acquire"> |
                                             kw<"monotonic"> | kw<"release"> |
                                             kw<"seq_cst"> | kw<"unordered">;
  static constexpr auto AtomicOpWord =
      kw<"add"> | kw<"and"> | kw<"fadd"> | kw<"fmax"> | kw<"fmin"> |
      kw<"fsub"> | kw<"max"> | kw<"min"> | kw<"nand"> | kw<"or"> | kw<"sub"> |
      kw<"umax"> | kw<"umin"> | kw<"xchg"> | kw<"xor"> | kw<"uinc_wrap"> |
      kw<"udec_wrap"> | kw<"usub_cond"> | kw<"usub_sat"> | kw<"fmaximum"> |
      kw<"fminimum"> | kw<"fmaximumnum"> | kw<"fminimumnum">;

  template <auto Member>
  static constexpr auto SyncScopeOpt = option(
      kw<"syncscope"> + lit<"("> + assign<Member>(ref<STRING_LIT>) + lit<")">);
  template <auto Member>
  static constexpr auto Alignments =
      many(lit<","> + kw<"align"> + append<Member>(ref<INT_LIT>));
  template <auto Member>
  static constexpr auto AddressSpaces =
      many(lit<","> + kw<"addrspace"> + lit<"("> +
           append<Member>(ref<INT_LIT>) + lit<")">);

  struct CallArgRule
      : rule<(!kw<"metadata"> + assign<&ast::CallArg::type>(ref<TypeRule>) +
              many(append<&ast::CallArg::attrs>(ParamAttribute)) +
              assign<&ast::CallArg::value>(ref<ValueRule>))  //
             | (flag<&ast::CallArg::isMetadata>(kw<"metadata">) +
                assign<&ast::CallArg::metadata>(NamedFieldValue))> {};

  template <cursiv::str Kw>
  static constexpr auto InstBin2Ovf =
      assign<&ast::GenericInst::op>(kw<Kw>) +
      many(append<&ast::GenericInst::flags>(OverflowFlagWord)) +
      append<&ast::GenericInst::operands>(ref<TypedValueRule>) + lit<","> +
      append<&ast::GenericInst::operands>(ref<BareValueRule>);
  template <cursiv::str Kw>
  static constexpr auto InstBin2Fmf =
      assign<&ast::GenericInst::op>(kw<Kw>) +
      many(append<&ast::GenericInst::flags>(FastMathFlagWord)) +
      append<&ast::GenericInst::operands>(ref<TypedValueRule>) + lit<","> +
      append<&ast::GenericInst::operands>(ref<BareValueRule>);
  template <cursiv::str Kw>
  static constexpr auto InstBin2Exact =
      assign<&ast::GenericInst::op>(kw<Kw>) +
      option(append<&ast::GenericInst::flags>(kw<"exact">)) +
      append<&ast::GenericInst::operands>(ref<TypedValueRule>) + lit<","> +
      append<&ast::GenericInst::operands>(ref<BareValueRule>);
  template <cursiv::str Kw>
  static constexpr auto InstBin2Plain =
      assign<&ast::GenericInst::op>(kw<Kw>) +
      option(append<&ast::GenericInst::flags>(kw<"disjoint">)) +
      append<&ast::GenericInst::operands>(ref<TypedValueRule>) + lit<","> +
      append<&ast::GenericInst::operands>(ref<BareValueRule>);
  template <cursiv::str Kw>
  static constexpr auto InstBin2Typed =
      assign<&ast::GenericInst::op>(kw<Kw>) +
      append<&ast::GenericInst::operands>(ref<TypedValueRule>) + lit<","> +
      append<&ast::GenericInst::operands>(ref<TypedValueRule>);
  template <cursiv::str Kw>
  static constexpr auto InstBin3Typed =
      assign<&ast::GenericInst::op>(kw<Kw>) +
      append<&ast::GenericInst::operands>(ref<TypedValueRule>) + lit<","> +
      append<&ast::GenericInst::operands>(ref<TypedValueRule>) + lit<","> +
      append<&ast::GenericInst::operands>(ref<TypedValueRule>);
  template <cursiv::str Kw>
  static constexpr auto InstConv =
      assign<&ast::GenericInst::op>(kw<Kw>) +
      many(append<&ast::GenericInst::flags>(FastMathFlagWord |
                                            OverflowFlagWord | kw<"nneg">)) +
      append<&ast::GenericInst::operands>(ref<TypedValueRule>) + kw<"to"> +
      assign<&ast::GenericInst::type>(ref<TypeRule>);

  static constexpr auto InstFNeg =
      assign<&ast::GenericInst::op>(kw<"fneg">) +
      many(append<&ast::GenericInst::flags>(FastMathFlagWord)) +
      append<&ast::GenericInst::operands>(ref<TypedValueRule>);
  static constexpr auto InstExtractValue =
      assign<&ast::GenericInst::op>(kw<"extractvalue">) +
      append<&ast::GenericInst::operands>(ref<TypedValueRule>) +
      some(lit<","> + append<&ast::GenericInst::indices>(ref<INT_LIT>));
  static constexpr auto InstInsertValue =
      assign<&ast::GenericInst::op>(kw<"insertvalue">) +
      append<&ast::GenericInst::operands>(ref<TypedValueRule>) + lit<","> +
      append<&ast::GenericInst::operands>(ref<TypedValueRule>) +
      some(lit<","> + append<&ast::GenericInst::indices>(ref<INT_LIT>));
  static constexpr auto InstAlloca =
      assign<&ast::GenericInst::op>(kw<"alloca">) +
      option(append<&ast::GenericInst::flags>(kw<"inalloca">)) +
      option(append<&ast::GenericInst::flags>(kw<"swifterror">)) +
      assign<&ast::GenericInst::type>(ref<TypeRule>) +
      option(lit<","> +
             append<&ast::GenericInst::operands>(ref<TypedValueRule>)) +
      Alignments<&ast::GenericInst::align> +
      AddressSpaces<&ast::GenericInst::addrSpace>;
  static constexpr auto InstLoad =
      (assign<&ast::GenericInst::op>(kw<"load">) +
       option(append<&ast::GenericInst::flags>(kw<"volatile">)) +
       assign<&ast::GenericInst::type>(ref<TypeRule>) + lit<","> +
       append<&ast::GenericInst::operands>(ref<TypedValueRule>) +
       Alignments<&ast::GenericInst::align>)  //
      | (assign<&ast::GenericInst::op>(kw<"load">) +
         append<&ast::GenericInst::flags>(kw<"atomic">) +
         option(append<&ast::GenericInst::flags>(kw<"volatile">)) +
         assign<&ast::GenericInst::type>(ref<TypeRule>) + lit<","> +
         append<&ast::GenericInst::operands>(ref<TypedValueRule>) +
         SyncScopeOpt<&ast::GenericInst::syncScope> +
         append<&ast::GenericInst::orderings>(AtomicOrderingWord) +
         Alignments<&ast::GenericInst::align>);
  static constexpr auto InstStore =
      (assign<&ast::GenericInst::op>(kw<"store">) +
       option(append<&ast::GenericInst::flags>(kw<"volatile">)) +
       append<&ast::GenericInst::operands>(ref<TypedValueRule>) + lit<","> +
       append<&ast::GenericInst::operands>(ref<TypedValueRule>) +
       Alignments<&ast::GenericInst::align>)  //
      | (assign<&ast::GenericInst::op>(kw<"store">) +
         append<&ast::GenericInst::flags>(kw<"atomic">) +
         option(append<&ast::GenericInst::flags>(kw<"volatile">)) +
         append<&ast::GenericInst::operands>(ref<TypedValueRule>) + lit<","> +
         append<&ast::GenericInst::operands>(ref<TypedValueRule>) +
         SyncScopeOpt<&ast::GenericInst::syncScope> +
         append<&ast::GenericInst::orderings>(AtomicOrderingWord) +
         Alignments<&ast::GenericInst::align>);
  static constexpr auto InstFence =
      assign<&ast::GenericInst::op>(kw<"fence">) +
      SyncScopeOpt<&ast::GenericInst::syncScope> +
      append<&ast::GenericInst::orderings>(AtomicOrderingWord);
  static constexpr auto InstCmpXchg =
      assign<&ast::GenericInst::op>(kw<"cmpxchg">) +
      option(append<&ast::GenericInst::flags>(kw<"weak">)) +
      option(append<&ast::GenericInst::flags>(kw<"volatile">)) +
      append<&ast::GenericInst::operands>(ref<TypedValueRule>) + lit<","> +
      append<&ast::GenericInst::operands>(ref<TypedValueRule>) + lit<","> +
      append<&ast::GenericInst::operands>(ref<TypedValueRule>) +
      SyncScopeOpt<&ast::GenericInst::syncScope> +
      append<&ast::GenericInst::orderings>(AtomicOrderingWord) +
      append<&ast::GenericInst::orderings>(AtomicOrderingWord) +
      Alignments<&ast::GenericInst::align>;
  static constexpr auto InstAtomicRmw =
      assign<&ast::GenericInst::op>(kw<"atomicrmw">) +
      option(append<&ast::GenericInst::flags>(kw<"volatile">)) +
      option(append<&ast::GenericInst::flags>(kw<"elementwise">)) +
      assign<&ast::GenericInst::predicate>(AtomicOpWord) +
      append<&ast::GenericInst::operands>(ref<TypedValueRule>) + lit<","> +
      append<&ast::GenericInst::operands>(ref<TypedValueRule>) +
      SyncScopeOpt<&ast::GenericInst::syncScope> +
      append<&ast::GenericInst::orderings>(AtomicOrderingWord) +
      Alignments<&ast::GenericInst::align>;
  static constexpr auto InstGetElementPtr =
      assign<&ast::GenericInst::op>(kw<"getelementptr">) +
      many(append<&ast::GenericInst::flags>(GepFlagWord)) +
      assign<&ast::GenericInst::type>(ref<TypeRule>) + lit<","> +
      append<&ast::GenericInst::operands>(ref<TypedValueRule>) +
      many(lit<","> + append<&ast::GenericInst::operands>(ref<TypedValueRule>));
  static constexpr auto InstICmp =
      assign<&ast::GenericInst::op>(kw<"icmp">) +
      option(append<&ast::GenericInst::flags>(kw<"samesign">)) +
      assign<&ast::GenericInst::predicate>(IPredWord) +
      append<&ast::GenericInst::operands>(ref<TypedValueRule>) + lit<","> +
      append<&ast::GenericInst::operands>(ref<BareValueRule>);
  static constexpr auto InstFCmp =
      assign<&ast::GenericInst::op>(kw<"fcmp">) +
      many(append<&ast::GenericInst::flags>(FastMathFlagWord)) +
      assign<&ast::GenericInst::predicate>(FPredWord) +
      append<&ast::GenericInst::operands>(ref<TypedValueRule>) + lit<","> +
      append<&ast::GenericInst::operands>(ref<BareValueRule>);
  static constexpr auto InstSelect =
      assign<&ast::GenericInst::op>(kw<"select">) +
      many(append<&ast::GenericInst::flags>(FastMathFlagWord)) +
      append<&ast::GenericInst::operands>(ref<TypedValueRule>) + lit<","> +
      append<&ast::GenericInst::operands>(ref<TypedValueRule>) + lit<","> +
      append<&ast::GenericInst::operands>(ref<TypedValueRule>);
  static constexpr auto InstFreeze =
      assign<&ast::GenericInst::op>(kw<"freeze">) +
      append<&ast::GenericInst::operands>(ref<TypedValueRule>);
  static constexpr auto InstVaArg =
      assign<&ast::GenericInst::op>(kw<"va_arg">) +
      append<&ast::GenericInst::operands>(ref<TypedValueRule>) + lit<","> +
      assign<&ast::GenericInst::type>(ref<TypeRule>);
  static constexpr auto InstLandingPad =
      assign<&ast::GenericInst::op>(kw<"landingpad">) +
      assign<&ast::GenericInst::type>(ref<TypeRule>) +
      option(append<&ast::GenericInst::flags>(kw<"cleanup">)) +
      many(append<&ast::GenericInst::clauseTypes>(kw<"catch"> | kw<"filter">) +
           append<&ast::GenericInst::operands>(ref<TypedValueRule>));
  static constexpr auto InstCatchPad =
      assign<&ast::GenericInst::op>(kw<"catchpad">) + kw<"within"> +
      assign<&ast::GenericInst::withinToken>(ref<LOCAL_IDENT>) + lit<"["> +
      many(append<&ast::GenericInst::exceptionArgs>(ref<CallArgRule>),
           lit<",">) +
      lit<"]">;
  static constexpr auto InstCleanupPad =
      assign<&ast::GenericInst::op>(kw<"cleanuppad">) + kw<"within"> +
      assign<&ast::GenericInst::withinToken>(kw<"none"> | ref<LOCAL_IDENT>) +
      lit<"["> +
      many(append<&ast::GenericInst::exceptionArgs>(ref<CallArgRule>),
           lit<",">) +
      lit<"]">;

  static constexpr auto ValueInstructionAlt =
      InstFNeg | InstBin2Ovf<"add"> | InstBin2Fmf<"fadd"> | InstBin2Ovf<"sub"> |
      InstBin2Fmf<"fsub"> | InstBin2Ovf<"mul"> | InstBin2Fmf<"fmul"> |
      InstBin2Exact<"udiv"> | InstBin2Exact<"sdiv"> | InstBin2Fmf<"fdiv"> |
      InstBin2Plain<"urem"> | InstBin2Plain<"srem"> | InstBin2Fmf<"frem"> |
      InstBin2Ovf<"shl"> | InstBin2Exact<"lshr"> | InstBin2Exact<"ashr"> |
      InstBin2Plain<"and"> | InstBin2Plain<"or"> | InstBin2Plain<"xor"> |
      InstBin2Typed<"extractelement"> | InstBin3Typed<"insertelement"> |
      InstBin3Typed<"shufflevector"> | InstExtractValue | InstInsertValue |
      InstAlloca | InstLoad | InstCmpXchg | InstAtomicRmw | InstGetElementPtr |
      InstConv<"trunc"> | InstConv<"zext"> | InstConv<"sext"> |
      InstConv<"fptrunc"> | InstConv<"fpext"> | InstConv<"fptoui"> |
      InstConv<"fptosi"> | InstConv<"uitofp"> | InstConv<"sitofp"> |
      InstConv<"ptrtoaddr"> | InstConv<"ptrtoint"> | InstConv<"inttoptr"> |
      InstConv<"bitcast"> | InstConv<"addrspacecast"> | InstICmp | InstFCmp |
      InstSelect | InstFreeze | InstVaArg | InstLandingPad | InstCatchPad |
      InstCleanupPad;

  struct PhiIncomingRule
      : rule<lit<"["> + assign<&ast::PhiIncoming::value>(ref<ValueRule>) +
             lit<","> + assign<&ast::PhiIncoming::block>(ref<LOCAL_IDENT>) +
             lit<"]">> {};
  static constexpr auto PhiInstBody =
      kw<"phi"> + many(append<&ast::PhiInst::fastMathFlags>(FastMathFlagWord)) +
      assign<&ast::PhiInst::type>(ref<TypeRule>) +
      many(append<&ast::PhiInst::incomings>(ref<PhiIncomingRule>), lit<",">);

  struct OperandBundleRule
      : rule<assign<&ast::OperandBundle::tag>(ref<STRING_LIT>) + lit<"("> +
             many(append<&ast::OperandBundle::args>(ref<TypedValueRule>),
                  lit<",">) +
             lit<")">> {};

  template <auto Member>
  static constexpr auto ArgList =
      option(lit<"..."> | (some(append<Member>(ref<CallArgRule>), lit<",">) +
                           option(lit<","> + lit<"...">)));
  template <auto Member>
  static constexpr auto Bundles =
      option(lit<"["> + some(append<Member>(ref<OperandBundleRule>), lit<",">) +
             lit<"]">);

  static constexpr auto CallInstBody =
      option(assign<&ast::CallInst::tailKind>(kw<"musttail"> | kw<"notail"> |
                                              kw<"tail">)) +
      assign<&ast::CallInst::kind>(kw<"call">) +
      many(append<&ast::CallInst::fastMathFlags>(FastMathFlagWord)) +
      option(assign<&ast::CallInst::callingConv>(ref<CallingConvRule>)) +
      many(append<&ast::CallInst::returnAttrs>(ReturnAttribute)) +
      option(kw<"addrspace"> + lit<"("> +
             assign<&ast::CallInst::addrSpace>(ref<INT_LIT>) + lit<")">) +
      assign<&ast::CallInst::type>(ref<TypeRule>) +
      assign<&ast::CallInst::callee>(ref<ValueRule>) + lit<"("> +
      ArgList<&ast::CallInst::args> + lit<")"> +
      many(append<&ast::CallInst::funcAttrs>(FuncAttribute)) +
      Bundles<&ast::CallInst::bundles>;

  // ---- Terminators ----
  static constexpr auto RetInstBody =
      kw<"ret"> +
      (kw<"void"> | assign<&ast::RetInst::value>(ref<TypedValueRule>));
  static constexpr auto BrInstBody =
      kw<"br"> + assign<&ast::BrInst::target>(ref<LabelRule>);
  static constexpr auto CondBrInstBody =
      kw<"br"> + assign<&ast::CondBrInst::cond>(ref<TypedValueRule>) +
      lit<","> + assign<&ast::CondBrInst::thenLabel>(ref<LabelRule>) +
      lit<","> + assign<&ast::CondBrInst::elseLabel>(ref<LabelRule>);
  struct SwitchCaseRule
      : rule<assign<&ast::SwitchCase::value>(ref<TypedValueRule>) + lit<","> +
             assign<&ast::SwitchCase::target>(ref<LabelRule>)> {};
  static constexpr auto SwitchInstBody =
      kw<"switch"> + assign<&ast::SwitchInst::value>(ref<TypedValueRule>) +
      lit<","> + assign<&ast::SwitchInst::defaultLabel>(ref<LabelRule>) +
      lit<"["> + many(append<&ast::SwitchInst::cases>(ref<SwitchCaseRule>)) +
      lit<"]">;
  static constexpr auto IndirectBrInstBody =
      kw<"indirectbr"> +
      assign<&ast::IndirectBrInst::address>(ref<TypedValueRule>) + lit<","> +
      lit<"["> +
      many(append<&ast::IndirectBrInst::targets>(ref<LabelRule>), lit<",">) +
      lit<"]">;
  static constexpr auto ResumeInstBody =
      assign<&ast::GenericInst::op>(kw<"resume">) +
      append<&ast::GenericInst::operands>(ref<TypedValueRule>);

  static constexpr auto UnwindTarget =
      flag<&ast::GenericInst::toCaller>(kw<"to"> + kw<"caller">)  //
      | append<&ast::GenericInst::labels>(ref<LabelRule>);

  static constexpr auto CatchRetInstBody =
      assign<&ast::GenericInst::op>(kw<"catchret">) + kw<"from"> +
      append<&ast::GenericInst::operands>(ref<BareValueRule>) + kw<"to"> +
      append<&ast::GenericInst::labels>(ref<LabelRule>);
  static constexpr auto CleanupRetInstBody =
      assign<&ast::GenericInst::op>(kw<"cleanupret">) + kw<"from"> +
      append<&ast::GenericInst::operands>(ref<BareValueRule>) + kw<"unwind"> +
      UnwindTarget;
  static constexpr auto UnreachableInstBody =
      assign<&ast::GenericInst::op>(kw<"unreachable">);
  static constexpr auto Handlers =
      some(append<&ast::GenericInst::labels>(ref<LabelRule>), lit<",">);
  static constexpr auto CatchSwitchInstBody =
      assign<&ast::GenericInst::op>(kw<"catchswitch">) + kw<"within"> +
      assign<&ast::GenericInst::withinToken>(kw<"none"> | ref<LOCAL_IDENT>) +
      lit<"["> + Handlers + lit<"]"> + kw<"unwind"> + UnwindTarget;

  static constexpr auto InvokeInstBody =
      assign<&ast::CallInst::kind>(kw<"invoke">) +
      option(assign<&ast::CallInst::callingConv>(ref<CallingConvRule>)) +
      many(append<&ast::CallInst::returnAttrs>(ReturnAttribute)) +
      option(kw<"addrspace"> + lit<"("> +
             assign<&ast::CallInst::addrSpace>(ref<INT_LIT>) + lit<")">) +
      assign<&ast::CallInst::type>(ref<TypeRule>) +
      assign<&ast::CallInst::callee>(ref<ValueRule>) + lit<"("> +
      ArgList<&ast::CallInst::args> + lit<")"> +
      many(append<&ast::CallInst::funcAttrs>(FuncAttribute)) +
      Bundles<&ast::CallInst::bundles> + kw<"to"> +
      assign<&ast::CallInst::toLabel>(ref<LabelRule>) + kw<"unwind"> +
      assign<&ast::CallInst::unwindLabel>(ref<LabelRule>);
  static constexpr auto CallBrInstBody =
      assign<&ast::CallInst::kind>(kw<"callbr">) +
      option(assign<&ast::CallInst::callingConv>(ref<CallingConvRule>)) +
      many(append<&ast::CallInst::returnAttrs>(ReturnAttribute)) +
      option(kw<"addrspace"> + lit<"("> +
             assign<&ast::CallInst::addrSpace>(ref<INT_LIT>) + lit<")">) +
      assign<&ast::CallInst::type>(ref<TypeRule>) +
      assign<&ast::CallInst::callee>(ref<ValueRule>) + lit<"("> +
      ArgList<&ast::CallInst::args> + lit<")"> +
      many(append<&ast::CallInst::funcAttrs>(FuncAttribute)) +
      Bundles<&ast::CallInst::bundles> + kw<"to"> +
      assign<&ast::CallInst::toLabel>(ref<LabelRule>) + lit<"["> +
      option(append<&ast::CallInst::indirectLabels>(ref<LabelRule>) +
             many(lit<","> +
                  append<&ast::CallInst::indirectLabels>(ref<LabelRule>))) +
      lit<"]">;

  struct DebugRecordRule
      : rule<assign<&ast::DebugRecord::kind>(ref<DEBUG_RECORD>) + lit<"("> +
             many(append<&ast::DebugRecord::args>(NamedFieldValue), lit<",">) +
             lit<")">> {};
  struct InstructionRule
      : rule<many(append<&ast::Instruction::debugRecords>(
                 ref<DebugRecordRule>)) +
             option(assign<&ast::Instruction::result>(ref<LOCAL_IDENT>) +
                    lit<"=">) +
             (create<ast::GenericInst>(ValueInstructionAlt) |
              create<ast::PhiInst>(PhiInstBody) |
              create<ast::CallInst>(CallInstBody) |
              create<ast::GenericInst>(InstStore | InstFence)) +
             MdAttachments<&ast::Instruction::metadata>> {};
  struct TerminatorRule
      : rule<many(append<&ast::Instruction::debugRecords>(
                 ref<DebugRecordRule>)) +
             option(assign<&ast::Instruction::result>(ref<LOCAL_IDENT>) +
                    lit<"=">) +
             (create<ast::GenericInst>(CatchSwitchInstBody) |
              create<ast::CallInst>(InvokeInstBody) |
              create<ast::CallInst>(CallBrInstBody) |
              create<ast::RetInst>(RetInstBody) |
              create<ast::CondBrInst>(CondBrInstBody) |
              create<ast::BrInst>(BrInstBody) |
              create<ast::SwitchInst>(SwitchInstBody) |
              create<ast::IndirectBrInst>(IndirectBrInstBody) |
              create<ast::GenericInst>(ResumeInstBody) |
              create<ast::GenericInst>(CatchRetInstBody) |
              create<ast::GenericInst>(CleanupRetInstBody) |
              create<ast::GenericInst>(UnreachableInstBody)) +
             MdAttachments<&ast::Instruction::metadata>> {};

  struct BasicBlockRule
      : rule<option(assign<&ast::BasicBlock::label>(ref<LABEL_IDENT>)) +
             many(
                 append<&ast::BasicBlock::instructions>(ref<InstructionRule>)) +
             assign<&ast::BasicBlock::terminator>(ref<TerminatorRule>)> {};

  struct UseListOrderRule
      : rule<kw<"uselistorder"> +
             assign<&ast::UseListOrder::value>(ref<TypedValueRule>) + lit<","> +
             lit<"{"> +
             some(append<&ast::UseListOrder::order>(ref<INT_LIT>), lit<",">) +
             lit<"}">> {};
  struct UseListOrderBBRule
      : rule<kw<"uselistorder_bb"> +
             assign<&ast::UseListOrderBB::func>(ref<GLOBAL_IDENT>) + lit<","> +
             assign<&ast::UseListOrderBB::block>(ref<LOCAL_IDENT>) + lit<","> +
             lit<"{"> +
             some(append<&ast::UseListOrderBB::order>(ref<INT_LIT>), lit<",">) +
             lit<"}">> {};

  struct FuncBodyRule
      : rule<lit<"{"> +
             some(append<&ast::FuncBody::blocks>(ref<BasicBlockRule>)) +
             many(
                 append<&ast::FuncBody::useListOrders>(ref<UseListOrderRule>)) +
             lit<"}">> {};

  // ================= Top level =================
  static constexpr auto ExternalLinkageWord =
      kw<"extern_weak"> | kw<"external">;
  static constexpr auto InternalLinkageWord =
      kw<"appending"> | kw<"available_externally"> | kw<"common"> |
      kw<"internal"> | kw<"linkonce"> | kw<"linkonce_odr"> | kw<"private"> |
      kw<"weak"> | kw<"weak_odr">;
  static constexpr auto LinkageWord = InternalLinkageWord | ExternalLinkageWord;
  static constexpr auto PreemptionWord =
      kw<"dso_local"> | kw<"dso_preemptable">;
  static constexpr auto VisibilityWord =
      kw<"default"> | kw<"hidden"> | kw<"protected">;
  static constexpr auto DllStorageClassWord = kw<"dllexport"> | kw<"dllimport">;
  static constexpr auto TlsModelWord =
      kw<"initialexec"> | kw<"localdynamic"> | kw<"localexec">;
  static constexpr auto UnnamedAddrWord =
      kw<"local_unnamed_addr"> | kw<"unnamed_addr">;

  template <auto FlagMember, auto ModelMember>
  static constexpr auto ThreadLocalOpt =
      option(flag<FlagMember>(kw<"thread_local">) +
             option(lit<"("> + assign<ModelMember>(TlsModelWord) + lit<")">));

  struct SourceFilenameRule
      : rule<kw<"source_filename"> + lit<"="> +
             assign<&ast::SourceFilename::text>(ref<STRING_LIT>)> {};
  struct TargetDefRule
      : rule<(kw<"target"> + assign<&ast::TargetDef::kind>(kw<"datalayout">) +
              lit<"="> + assign<&ast::TargetDef::text>(ref<STRING_LIT>))  //
             | (kw<"target"> + assign<&ast::TargetDef::kind>(kw<"triple">) +
                lit<"="> + assign<&ast::TargetDef::text>(ref<STRING_LIT>))> {};
  struct AsmPropertyRule
      : rule<assign<&ast::AsmProperty::name>(ref<FIELD_NAME>) +
             assign<&ast::AsmProperty::value>(ref<STRING_LIT>)> {};
  struct ModuleAsmRule
      : rule<kw<"module"> + kw<"asm"> +
             option(
                 lit<"("> +
                 many(append<&ast::ModuleAsm::properties>(ref<AsmPropertyRule>),
                      lit<",">) +
                 lit<")">) +
             some(append<&ast::ModuleAsm::lines>(ref<STRING_LIT>))> {};
  struct TypeDefRule
      : rule<assign<&ast::TypeDef::name>(ref<LOCAL_IDENT>) + lit<"="> +
             kw<"type"> + assign<&ast::TypeDef::type>(ref<TypeRule>)> {};
  struct ComdatDefRule : rule<assign<&ast::ComdatDef::name>(ref<COMDAT_NAME>) +
                              lit<"="> + kw<"comdat"> +
                              assign<&ast::ComdatDef::kind>(
                                  kw<"any"> | kw<"exactmatch"> | kw<"largest"> |
                                  kw<"nodeduplicate"> | kw<"samesize">)> {};

  struct GlobalVarDeclRule
      : rule<assign<&ast::GlobalVar::name>(ref<GLOBAL_IDENT>) + lit<"="> +
             assign<&ast::GlobalVar::linkage>(ExternalLinkageWord) +
             option(assign<&ast::GlobalVar::preemption>(PreemptionWord)) +
             option(assign<&ast::GlobalVar::visibility>(VisibilityWord)) +
             option(assign<&ast::GlobalVar::dllStorageClass>(
                 DllStorageClassWord)) +
             ThreadLocalOpt<&ast::GlobalVar::threadLocal,
                            &ast::GlobalVar::tlsModel> +
             option(assign<&ast::GlobalVar::unnamedAddr>(UnnamedAddrWord)) +
             option(kw<"addrspace"> + lit<"("> +
                    assign<&ast::GlobalVar::addrSpace>(ref<INT_LIT>) +
                    lit<")">) +
             option(flag<&ast::GlobalVar::externallyInitialized>(
                 kw<"externally_initialized">)) +
             assign<&ast::GlobalVar::immutable>(kw<"constant"> | kw<"global">) +
             assign<&ast::GlobalVar::type>(ref<TypeRule>) +
             many(lit<","> + append<&ast::GlobalVar::fields>(GlobalField)) +
             MdAttachments<&ast::GlobalVar::metadata> +
             many(append<&ast::GlobalVar::funcAttrs>(FuncAttribute))> {};
  struct GlobalVarDefRule
      : rule<assign<&ast::GlobalVar::name>(ref<GLOBAL_IDENT>) + lit<"="> +
             option(assign<&ast::GlobalVar::linkage>(InternalLinkageWord)) +
             option(assign<&ast::GlobalVar::preemption>(PreemptionWord)) +
             option(assign<&ast::GlobalVar::visibility>(VisibilityWord)) +
             option(assign<&ast::GlobalVar::dllStorageClass>(
                 DllStorageClassWord)) +
             ThreadLocalOpt<&ast::GlobalVar::threadLocal,
                            &ast::GlobalVar::tlsModel> +
             option(assign<&ast::GlobalVar::unnamedAddr>(UnnamedAddrWord)) +
             option(kw<"addrspace"> + lit<"("> +
                    assign<&ast::GlobalVar::addrSpace>(ref<INT_LIT>) +
                    lit<")">) +
             option(flag<&ast::GlobalVar::externallyInitialized>(
                 kw<"externally_initialized">)) +
             assign<&ast::GlobalVar::immutable>(kw<"constant"> | kw<"global">) +
             assign<&ast::GlobalVar::type>(ref<TypeRule>) +
             assign<&ast::GlobalVar::initializer>(ref<ConstantRule>) +
             many(lit<","> + append<&ast::GlobalVar::fields>(GlobalField)) +
             MdAttachments<&ast::GlobalVar::metadata> +
             many(append<&ast::GlobalVar::funcAttrs>(FuncAttribute))> {};

  struct IndirectSymbolRule
      : rule<assign<&ast::IndirectSymbol::name>(ref<GLOBAL_IDENT>) + lit<"="> +
             option(assign<&ast::IndirectSymbol::linkage>(LinkageWord)) +
             option(assign<&ast::IndirectSymbol::preemption>(PreemptionWord)) +
             option(assign<&ast::IndirectSymbol::visibility>(VisibilityWord)) +
             option(assign<&ast::IndirectSymbol::dllStorageClass>(
                 DllStorageClassWord)) +
             ThreadLocalOpt<&ast::IndirectSymbol::threadLocal,
                            &ast::IndirectSymbol::tlsModel> +
             option(
                 assign<&ast::IndirectSymbol::unnamedAddr>(UnnamedAddrWord)) +
             assign<&ast::IndirectSymbol::kind>(kw<"alias"> | kw<"ifunc">) +
             assign<&ast::IndirectSymbol::type>(ref<TypeRule>) + lit<","> +
             assign<&ast::IndirectSymbol::target>(ref<TypedValueRule> |
                                                  ConstantExprAlt) +
             many(lit<","> + kw<"partition"> +
                  append<&ast::IndirectSymbol::partitions>(ref<STRING_LIT>)) +
             MdAttachments<&ast::IndirectSymbol::metadata>> {};

  struct FuncHeaderRule
      : rule<option(assign<&ast::FuncHeader::linkage>(LinkageWord)) +
             option(assign<&ast::FuncHeader::preemption>(PreemptionWord)) +
             option(assign<&ast::FuncHeader::visibility>(VisibilityWord)) +
             option(assign<&ast::FuncHeader::dllStorageClass>(
                 DllStorageClassWord)) +
             option(
                 assign<&ast::FuncHeader::callingConv>(ref<CallingConvRule>)) +
             many(append<&ast::FuncHeader::returnAttrs>(ReturnAttribute)) +
             assign<&ast::FuncHeader::returnType>(ref<TypeRule>) +
             assign<&ast::FuncHeader::name>(ref<GLOBAL_IDENT>) + lit<"("> +
             assign<&ast::FuncHeader::params>(ref<ParamsRule>) + lit<")"> +
             option(assign<&ast::FuncHeader::unnamedAddr>(UnnamedAddrWord)) +
             option(kw<"addrspace"> + lit<"("> +
                    assign<&ast::FuncHeader::addrSpace>(ref<INT_LIT>) +
                    lit<")">) +
             many(append<&ast::FuncHeader::fields>(FuncHdrField))> {};

  struct FuncDeclRule
      : rule<kw<"declare"> +
             many(append<&ast::FuncDecl::metadata>(ref<MdAttachmentRule>)) +
             assign<&ast::FuncDecl::header>(ref<FuncHeaderRule>)> {};
  struct FuncDefRule
      : rule<kw<"define"> + assign<&ast::FuncDef::header>(ref<FuncHeaderRule>) +
             many(append<&ast::FuncDef::metadata>(ref<MdAttachmentRule>)) +
             assign<&ast::FuncDef::body>(ref<FuncBodyRule>)> {};

  struct AttrGroupDefRule
      : rule<kw<"attributes"> +
             assign<&ast::AttrGroupDef::id>(ref<ATTR_GROUP_ID>) + lit<"="> +
             lit<"{"> + many(append<&ast::AttrGroupDef::attrs>(FuncAttribute)) +
             lit<"}">> {};

  struct NamedMetadataDefRule
      : rule<assign<&ast::NamedMetadataDef::name>(ref<METADATA_NAME>) +
             lit<"="> + lit<"!"> + lit<"{"> +
             many(append<&ast::NamedMetadataDef::nodes>(MdNode), lit<",">) +
             lit<"}">> {};
  struct MetadataDefRule
      : rule<assign<&ast::MetadataDef::id>(ref<METADATA_ID>) + lit<"="> +
             option(flag<&ast::MetadataDef::distinct>(kw<"distinct">)) +
             assign<&ast::MetadataDef::value>(ref<MdTupleRule> |
                                              ref<SpecializedMdNodeRule>)> {};

  struct SummaryValueRule;
  struct SummaryFieldRule
      : rule<assign<&ast::SummaryField::name>(ref<WORD>) + lit<":"> +
             assign<&ast::SummaryField::value>(ref<SummaryValueRule>)> {};
  struct SummaryValueRule
      : rule<create<ast::SummaryValue> +
             ((flag<&ast::SummaryValue::grouped>(lit<"(">) +
               many(append<&ast::SummaryValue::elements>(ref<SummaryFieldRule> |
                                                         ref<SummaryValueRule>),
                    lit<",">) +
               lit<")">) |
              some(append<&ast::SummaryValue::atoms>(
                  ref<WORD> | ref<INT_LIT> | ref<STRING_LIT> | ref<SUMMARY_ID> |
                  ref<GLOBAL_IDENT> | ref<LOCAL_IDENT>)))> {};
  struct SummaryEntryRule
      : rule<assign<&ast::SummaryEntry::id>(ref<SUMMARY_ID>) + lit<"="> +
             assign<&ast::SummaryEntry::definition>(ref<SummaryFieldRule>)> {};
  static constexpr auto TopLevelEntityAlt =
      ref<SummaryEntryRule> | ref<SourceFilenameRule> | ref<TargetDefRule> |
      ref<ModuleAsmRule> | ref<TypeDefRule> | ref<ComdatDefRule> |
      ref<GlobalVarDeclRule> | ref<GlobalVarDefRule> | ref<IndirectSymbolRule> |
      ref<FuncDeclRule> | ref<FuncDefRule> | ref<AttrGroupDefRule> |
      ref<NamedMetadataDefRule> | ref<MetadataDefRule> | ref<UseListOrderRule> |
      ref<UseListOrderBBRule>;

  struct ModuleRule
      : rule<create<ast::Module> +
             many(append<&ast::Module::entities>(TopLevelEntityAlt))> {};

  using entry = ModuleRule;
  using ignore = WS;
  using hidden = list<COMMENT, BLOCK_COMMENT>;
};

}  // namespace llvmir
