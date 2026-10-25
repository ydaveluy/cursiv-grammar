

#pragma once

#include <memory_resource>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "wat/core/ast.hpp"

namespace wat {
inline constexpr auto hexDigit = [](char c) {
  return c >= '0' && c <= '9'   ? c - '0'
         : c >= 'a' && c <= 'f' ? c - 'a' + 10
         : c >= 'A' && c <= 'F' ? c - 'A' + 10
                                : -1;
};
inline constexpr auto stringValue =
    [](std::string_view text,
       std::pmr::memory_resource* memory) -> std::string_view {
  text = text.substr(1, text.size() - 2);
  if (text.find('\\') == text.npos) return text;
  auto* out = static_cast<char*>(memory->allocate(text.size(), alignof(char)));
  std::size_t size = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    char c = text[i];
    if (c == '\\' && i + 1 < text.size()) {
      c = text[++i];
      if (c == 'n')
        c = '\n';
      else if (c == 'r')
        c = '\r';
      else if (c == 't')
        c = '\t';
      else if (c == '"' || c == '\\' || c == '\'') {
      } else if (hexDigit(c) >= 0 && i + 1 < text.size() &&
                 hexDigit(text[i + 1]) >= 0) {
        c = char(hexDigit(c) * 16 + hexDigit(text[++i]));
      } else if (c == 'u' && i + 1 < text.size() && text[i + 1] == '{') {
        auto end = text.find('}', i + 2);
        std::uint32_t scalar = 0;
        bool valid = end != text.npos && end > i + 2;
        if (valid)
          for (auto j = i + 2; j < end; ++j) {
            int digit = hexDigit(text[j]);
            if (digit < 0 || scalar > 0x10ffff / 16) {
              valid = false;
              break;
            }
            scalar = scalar * 16 + digit;
          }
        valid = valid && scalar <= 0x10ffff &&
                !(scalar >= 0xd800 && scalar <= 0xdfff);
        if (valid) {
          i = end;
          if (scalar < 0x80)
            out[size++] = char(scalar);
          else if (scalar < 0x800) {
            out[size++] = char(0xc0 | (scalar >> 6));
            out[size++] = char(0x80 | (scalar & 63));
          } else if (scalar < 0x10000) {
            out[size++] = char(0xe0 | (scalar >> 12));
            out[size++] = char(0x80 | ((scalar >> 6) & 63));
            out[size++] = char(0x80 | (scalar & 63));
          } else {
            out[size++] = char(0xf0 | (scalar >> 18));
            out[size++] = char(0x80 | ((scalar >> 12) & 63));
            out[size++] = char(0x80 | ((scalar >> 6) & 63));
            out[size++] = char(0x80 | (scalar & 63));
          }
          continue;
        }
        out[size++] = '\\';
      } else
        out[size++] = '\\';
    }
    out[size++] = c;
  }
  return {out, size};
};

struct Wat : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  struct LINE_COMMENT
      : terminal<lit<";;"> + many(cr<R"(^\r\n)">) + option(eol)> {};
  struct BLOCK_COMMENT
      : terminal<lit<"(;"> +
                 many(ref<BLOCK_COMMENT> | (!lit<"(;"> + !lit<";)"> + any)) +
                 lit<";)">> {};

  static constexpr auto Sign = lit<"+"> | lit<"-">;
  static constexpr auto HexDigit = cr<"0-9a-fA-F">;
  static constexpr auto Num = d + many(option(lit<"_">) + d);
  static constexpr auto HexNum = HexDigit + many(option(lit<"_">) + HexDigit);
  static constexpr auto NatBody = Num | (lit<"0x"> + HexNum);
  struct NAT : terminal<NatBody> {};
  struct INT : terminal<Sign + NatBody> {};
  static constexpr auto FloatBody =
      (option(Sign) + Num + lit<"."> + option(Num)) |
      (option(Sign) + Num + option(lit<"."> + option(Num)) +
       (lit<"e"> | lit<"E">)+option(Sign) + Num) |
      (option(Sign) + lit<"0x"> + HexNum + lit<"."> + option(HexNum)) |
      (option(Sign) + lit<"0x"> + HexNum + option(lit<"."> + option(HexNum)) +
       (lit<"p"> | lit<"P">)+option(Sign) + Num) |
      (option(Sign) + lit<"inf">) | (option(Sign) + lit<"nan">) |
      (option(Sign) + lit<"nan:0x"> + HexNum);
  struct FLOAT : terminal<FloatBody> {};

  struct OFFSET_EQ_NAT : terminal<lit<"offset="> + NatBody> {};
  struct ALIGN_EQ_NAT : terminal<lit<"align="> + NatBody> {};

  struct STRING_
      : terminal<lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\)">) +
                     lit<"\"">,
                 text_conv<stringValue>> {};

  static constexpr auto Letter = cr<"A-Za-z">;
  static constexpr auto SymbolChar = cr<R"(.+*/\\^~=<>!?@#$%&|:'`-)">;
  struct VAR : terminal<lit<"$"> + some(Letter | d | lit<"_"> | SymbolChar)> {};

  struct VALUE_TYPE
      : terminal<kw<"i32"> | kw<"i64"> | kw<"f32"> | kw<"f64"> | kw<"v128"> |
                 kw<"funcref"> | kw<"externref"> | kw<"exnref"> | kw<"anyref"> |
                 kw<"eqref"> | kw<"i31ref"> | kw<"structref"> | kw<"arrayref"> |
                 kw<"nullref"> | kw<"nullfuncref"> | kw<"nullexternref"> |
                 kw<"nullexnref"> | kw<"contref"> | kw<"nullcontref">> {};
  struct OPCODE
      : terminal<option(lit<"@">) + cr<"a-z"> + many(cr<"a-z0-9_.">)> {};
  struct CONST : terminal<(lit<"i32"> | lit<"i64"> | lit<"f32"> | lit<"f64"> |
                           lit<"v128">)+lit<".const">> {};
  struct NAN_EXPECTED
      : terminal<lit<"nan:"> + (lit<"canonical"> | lit<"arithmetic">)> {};
  static constexpr auto AbstractHeapType =
      kw<"func"> | kw<"extern"> | kw<"exn"> | kw<"any"> | kw<"eq"> | kw<"i31"> |
      kw<"struct"> | kw<"array"> | kw<"none"> | kw<"nofunc"> | kw<"noextern"> |
      kw<"noexn"> | kw<"cont"> | kw<"nocont">;
  struct HeapType
      : rule<ref<VAR> | ref<NAT> | AbstractHeapType |
                 (lit<"("> + kw<"shared"> + AbstractHeapType + lit<")">),
             std::string_view> {};
  struct RefType : rule<lit<"("> + kw<"ref"> + option(kw<"null">) +
                            ref<HeapType> + lit<")">,
                        std::string_view> {};
  struct ValueType : rule<ref<VALUE_TYPE> | ref<RefType>, std::string_view> {};
  struct StorageType
      : rule<kw<"i8"> | kw<"i16"> | ref<ValueType>, std::string_view> {};

  static constexpr auto varExpr = ref<NAT> | ref<VAR>;
  static constexpr auto literalExpr =
      ref<NAT> | ref<INT> | ref<FLOAT> | ref<NAN_EXPECTED>;

  struct InstrRule;
  struct ExprRule;
  struct FlatInstrRule;

  struct WconstRule;
  struct InlineImportRule;
  struct TypeDefRule;
  struct RecGroupRule;
  struct GlobalRule;
  struct TableRule;
  struct MemoryRule;
  struct FuncRule;
  struct ElemRule;
  struct DataRule;
  struct StartRule;
  struct ImportRule;
  struct ExportRule;
  struct ModuleRule;
  struct BinQuoteModuleRule;
  struct ActionRule;
  struct AssertionRule;
  struct RegisterRule;
  struct ScriptRule;
  struct TagRule;

  static constexpr auto addArg = append<&ast::Instr::args>;

#define WAT_INSTR_LIST(AddFn) many(AddFn(ref<InstrRule>))
#define WAT_OFFSET(AddFn)                                         \
  ((lit<"("> + kw<"offset"> + WAT_INSTR_LIST(AddFn) + lit<")">) | \
   AddFn(ref<ExprRule>))
#define WAT_TYPED_CLAUSE(Kw, AddFn)                           \
  (lit<"("> + Kw +                                            \
   (many(AddFn(create<ast::Binding>(                          \
        assign<&ast::Binding::valueType>(ref<ValueType>)))) | \
    AddFn(create<ast::Binding>(                               \
        assign<&ast::Binding::name>(ref<VAR>) +               \
        assign<&ast::Binding::valueType>(ref<ValueType>)))) + \
   lit<")">)
#define WAT_RESULT_CLAUSE(AddFn) \
  (lit<"("> + kw<"result"> + many(AddFn(ref<ValueType>)) + lit<")">)
#define WAT_MEMORY_TYPE(MinM, MaxM) \
  (assign<MinM>(ref<NAT>) + option(assign<MaxM>(ref<NAT>)))
#define WAT_GLOBAL_TYPE(ValM, MutM)                                        \
  (assign<ValM>(ref<ValueType>) | (lit<"("> + flag<MutM>(kw<"mut">) + \
                                   assign<ValM>(ref<ValueType>) + lit<")">))
#define WAT_MODULE_FIELD(AddFn)                                              \
  AddFn(ref<TypeDefRule> | ref<RecGroupRule> | ref<GlobalRule> |             \
        ref<TableRule> | ref<MemoryRule> | ref<FuncRule> | ref<ElemRule> |   \
        ref<DataRule> | ref<StartRule> | ref<ImportRule> | ref<ExportRule> | \
        ref<TagRule> | ref<AnnotationRule>)

  struct ClauseRule;
  struct FlatMarkerRule;
  struct DelegateRule;
  struct AnnotationRule;
  static constexpr auto ImmediateWord =
      kw<"i8x16"> | kw<"i16x8"> | kw<"i32x4"> | kw<"i64x2"> | kw<"f32x4"> |
      kw<"f64x2"> | AbstractHeapType;
  struct PlainOpcode
      : rule<!(kw<"block"> | kw<"loop"> | kw<"if"> | kw<"try"> |
               kw<"try_table"> | kw<"end"> | kw<"else"> | kw<"catch"> |
               kw<"catch_all"> | kw<"delegate">)+ref<OPCODE>,
             std::string_view> {};
  static constexpr auto Immediate =
      literalExpr | ref<VAR> | ref<OFFSET_EQ_NAT> | ref<ALIGN_EQ_NAT> |
      ref<STRING_> | ImmediateWord | ref<VALUE_TYPE> | ref<RefType>;
  struct ClauseRule
      : rule<flag<&ast::Instr::folded>(lit<"(">) +
             assign<&ast::Instr::op>(kw<"type"> | kw<"param"> | kw<"result"> |
                                     kw<"memory"> | kw<"table">) +
             many(addArg(Immediate | ref<ValueType>)) + lit<")">> {};
  static constexpr auto PlainInstr = assign<&ast::Instr::op>(ref<PlainOpcode>) +
                                     many(addArg(Immediate | ref<ClauseRule>));
  struct FlatMarkerRule
      : rule<(assign<&ast::Instr::op>(kw<"else">) +
              option(assign<&ast::Instr::label>(ref<VAR>)) +
              many(addArg(ref<InstrRule>))) |
             (assign<&ast::Instr::op>(kw<"catch"> | kw<"catch_all">) +
              many(addArg(varExpr)) + many(addArg(ref<InstrRule>)))> {};
  struct DelegateRule
      : rule<assign<&ast::Instr::op>(kw<"delegate">) + addArg(varExpr)> {};
  struct FlatInstrRule
      : rule<(assign<&ast::Instr::op>(kw<"block"> | kw<"loop"> | kw<"if"> |
                                      kw<"try"> | kw<"try_table">) +
              option(assign<&ast::Instr::label>(ref<VAR>)) +
              many(addArg(ref<InstrRule>)) + many(addArg(ref<FlatMarkerRule>)) +
              ((kw<"end"> + option(assign<&ast::Instr::endLabel>(ref<VAR>))) |
               addArg(ref<DelegateRule>))) |
             PlainInstr> {};
  struct ExprOpcode
      : rule<!(kw<"ref"> + !lit<".">)+ref<OPCODE>, std::string_view> {};
  struct ExprRule
      : rule<flag<&ast::Instr::folded>(lit<"(">) +
             ((assign<&ast::Instr::op>(kw<"block"> | kw<"loop"> | kw<"if"> |
                                       kw<"try"> | kw<"try_table">) +
               option(assign<&ast::Instr::label>(ref<VAR>))) |
              assign<&ast::Instr::op>(ref<ExprOpcode>)) +
             many(addArg(Immediate | ref<InstrRule>)) + lit<")">> {};
  struct ANNOTATION : terminal<lit<"@"> + cr<"a-z"> + many(cr<"a-z0-9_.">)> {};
  struct AnnotationPositionRule
      : rule<flag<&ast::Instr::folded>(lit<"(">) +
             assign<&ast::Instr::op>(kw<"before"> | kw<"after">) +
             addArg(ref<OPCODE>) + lit<")">> {};
  struct AnnotationRule
      : rule<flag<&ast::Instr::folded>(lit<"(">) +
             assign<&ast::Instr::op>(ref<ANNOTATION>) +
             many(addArg(Immediate | ref<AnnotationPositionRule> |
                         ref<ExprRule>)) +
             lit<")">> {};
  struct InstrRule : rule<ref<ExprRule> | ref<FlatInstrRule>> {};

  struct WconstRule
      : rule<lit<"("> + assign<&ast::Wconst::valueType>(ref<CONST>) +
             some(append<&ast::Wconst::literals>(literalExpr | ImmediateWord)) +
             lit<")">> {};

  struct InlineImportRule
      : rule<lit<"("> + kw<"import"> +
             assign<&ast::Import::module_>(ref<STRING_>) +
             assign<&ast::Import::name>(ref<STRING_>) + lit<")">> {};

  static constexpr auto FieldValue =
      (lit<"("> + flag<&ast::Binding::mutable_>(kw<"mut">) +
       assign<&ast::Binding::valueType>(ref<StorageType>) + lit<")">) |
      assign<&ast::Binding::valueType>(ref<StorageType>);
  struct FieldRule
      : rule<option(assign<&ast::Binding::name>(ref<VAR>)) + FieldValue> {};
  static constexpr auto CompositeType =
      (assign<&ast::TypeDef::kind>(kw<"func">) +
       many(WAT_TYPED_CLAUSE(kw<"param">, append<&ast::TypeDef::params>)) +
       many(WAT_RESULT_CLAUSE(append<&ast::TypeDef::results>))) |
      (assign<&ast::TypeDef::kind>(kw<"struct">) +
       many((lit<"("> + kw<"field"> +
             many(append<&ast::TypeDef::fields>(ref<FieldRule>)) + lit<")">) |
            append<&ast::TypeDef::fields>(ref<FieldRule>))) |
      (assign<&ast::TypeDef::kind>(kw<"array">) +
       many(append<&ast::TypeDef::fields>(ref<FieldRule>)));
  static constexpr auto SharedType =
      (flag<&ast::TypeDef::shared>(kw<"shared">) + lit<"("> +
       CompositeType + lit<")">) |
      CompositeType;
  struct SubHeader : rule<kw<"sub"> + option(kw<"final">),
                          text_conv<[](std::string_view text) {
                            return text.ends_with("final");
                          }>> {};
  struct TypeDefRule
      : rule<lit<"("> + kw<"type"> +
             option(assign<&ast::TypeDef::name>(ref<VAR>)) + lit<"("> +
             ((assign<&ast::TypeDef::final_>(ref<SubHeader>) +
               many(append<&ast::TypeDef::supertypes>(varExpr)) + lit<"("> +
               SharedType + lit<")">) |
              SharedType) +
             lit<")"> + lit<")">> {};
  struct RecGroupRule
      : rule<lit<"("> + kw<"rec"> +
             many(append<&ast::RecGroup::types>(ref<TypeDefRule>)) + lit<")">> {
  };

  struct GlobalRule
      : rule<lit<"("> + kw<"global"> +
             option(assign<&ast::Global::name>(ref<VAR>)) +
             many(lit<"("> + kw<"export"> +
                  append<&ast::Global::exports>(ref<STRING_>) + lit<")">) +
             ((assign<&ast::Global::import_>(ref<InlineImportRule>) +
               WAT_GLOBAL_TYPE(&ast::Global::valueType,
                               &ast::Global::mutable_)) |
              (WAT_GLOBAL_TYPE(&ast::Global::valueType,
                               &ast::Global::mutable_) +
               WAT_INSTR_LIST(append<&ast::Global::init>))) +
             lit<")">> {};

  static constexpr auto TableLimits =
      option(assign<&ast::Table::indexType>(kw<"i32"> | kw<"i64">)) +
      assign<&ast::Table::min>(ref<NAT>) +
      option(assign<&ast::Table::max>(ref<NAT>)) +
      option(flag<&ast::Table::shared>(kw<"shared">)) +
      assign<&ast::Table::elemType>(ref<ValueType>);
  static constexpr auto ImportTableLimits =
      option(assign<&ast::Import::indexType>(kw<"i32"> | kw<"i64">)) +
      assign<&ast::Import::min>(ref<NAT>) +
      option(assign<&ast::Import::max>(ref<NAT>)) +
      option(flag<&ast::Import::shared>(kw<"shared">)) +
      assign<&ast::Import::elemType>(ref<ValueType>);
  struct TableRule
      : rule<lit<"("> + kw<"table"> +
             option(assign<&ast::Table::name>(ref<VAR>)) +
             many(lit<"("> + kw<"export"> +
                  append<&ast::Table::exports>(ref<STRING_>) + lit<")">) +
             ((assign<&ast::Table::import_>(ref<InlineImportRule>) +
               TableLimits) |
              TableLimits |
              (assign<&ast::Table::elemType>(kw<"funcref">) + lit<"("> +
               kw<"elem"> +
               many(append<&ast::Table::inlineElems>(varExpr) |
                    append<&ast::Table::inlineExprs>(ref<ExprRule>)) +
               lit<")">)) +
             lit<")">> {};

  static constexpr auto MemoryLimits =
      option(assign<&ast::Memory::indexType>(kw<"i32"> | kw<"i64">)) +
      WAT_MEMORY_TYPE(&ast::Memory::min, &ast::Memory::max) +
      option(flag<&ast::Memory::shared>(kw<"shared">)) +
      option(lit<"("> + kw<"pagesize"> +
             assign<&ast::Memory::pageSize>(ref<NAT>) + lit<")">);
  static constexpr auto ImportMemoryLimits =
      option(assign<&ast::Import::indexType>(kw<"i32"> | kw<"i64">)) +
      WAT_MEMORY_TYPE(&ast::Import::min, &ast::Import::max) +
      option(flag<&ast::Import::shared>(kw<"shared">)) +
      option(lit<"("> + kw<"pagesize"> +
             assign<&ast::Import::pageSize>(ref<NAT>) + lit<")">);
  struct MemoryRule
      : rule<lit<"("> + kw<"memory"> +
             option(assign<&ast::Memory::name>(ref<VAR>)) +
             many(lit<"("> + kw<"export"> +
                  append<&ast::Memory::exports>(ref<STRING_>) + lit<")">) +
             ((assign<&ast::Memory::import_>(ref<InlineImportRule>) +
               MemoryLimits) |
              MemoryLimits |
              (option(assign<&ast::Memory::indexType>(kw<"i32"> | kw<"i64">)) +
               lit<"("> + kw<"data"> +
               many(append<&ast::Memory::inlineData>(ref<STRING_>)) +
               lit<")">)) +
             lit<")">> {};

  struct FuncRule
      : rule<create<ast::Func> + lit<"("> + kw<"func"> +
             option(assign<&ast::Func::name>(ref<VAR>)) +
             many(lit<"("> + kw<"export"> +
                  append<&ast::Func::exports>(ref<STRING_>) + lit<")">) +
             ((assign<&ast::Func::import_>(ref<InlineImportRule>) +
               option(lit<"("> + kw<"type"> +
                      assign<&ast::Func::typeUse>(varExpr) + lit<")">) +
               many(WAT_TYPED_CLAUSE(kw<"param">, append<&ast::Func::params>)) +
               many(WAT_RESULT_CLAUSE(append<&ast::Func::results>))) |
              (option(lit<"("> + kw<"type"> +
                      assign<&ast::Func::typeUse>(varExpr) + lit<")">) +
               many(WAT_TYPED_CLAUSE(kw<"param">, append<&ast::Func::params>)) +
               many(WAT_RESULT_CLAUSE(append<&ast::Func::results>)) +
               many(WAT_TYPED_CLAUSE(kw<"local">, append<&ast::Func::locals>)) +
               WAT_INSTR_LIST(append<&ast::Func::body>))) +
             lit<")">> {};

  struct ElemRule
      : rule<lit<"("> + kw<"elem"> +
             option(assign<&ast::Elem::name>(ref<VAR>)) +
             option(flag<&ast::Elem::declared>(kw<"declare">)) +
             option((lit<"("> + kw<"table"> +
                     assign<&ast::Elem::tableVar>(varExpr) + lit<")">) |
                    assign<&ast::Elem::tableVar>(ref<NAT>)) +
             option(WAT_OFFSET(append<&ast::Elem::offset>)) +
             option(assign<&ast::Elem::elemType>(ref<ValueType>) | kw<"func">) +
             many(append<&ast::Elem::funcVars>(varExpr) |
                  append<&ast::Elem::expressions>(ref<ExprRule>)) +
             lit<")">> {};
  struct DataRule
      : rule<lit<"("> + kw<"data"> +
             option(assign<&ast::Data::name>(ref<VAR>)) +
             option((lit<"("> + kw<"memory"> +
                     assign<&ast::Data::memVar>(varExpr) + lit<")">) |
                    assign<&ast::Data::memVar>(ref<NAT>)) +
             option(WAT_OFFSET(append<&ast::Data::offset>)) +
             many(append<&ast::Data::strings>(ref<STRING_>)) + lit<")">> {};
  struct TagRule
      : rule<create<ast::Tag> + lit<"("> + kw<"tag"> +
             option(assign<&ast::Func::name>(ref<VAR>)) +
             many(lit<"("> + kw<"export"> +
                  append<&ast::Func::exports>(ref<STRING_>) + lit<")">) +
             option(assign<&ast::Func::import_>(ref<InlineImportRule>)) +
             option(lit<"("> + kw<"type"> +
                    assign<&ast::Func::typeUse>(varExpr) + lit<")">) +
             many(WAT_TYPED_CLAUSE(kw<"param">, append<&ast::Func::params>)) +
             many(WAT_RESULT_CLAUSE(append<&ast::Func::results>)) + lit<")">> {
  };

  struct StartRule : rule<lit<"("> + kw<"start"> +
                          assign<&ast::Start::func>(varExpr) + lit<")">> {};

  struct ImportRule
      : rule<lit<"("> + kw<"import"> +
             assign<&ast::Import::module_>(ref<STRING_>) +
             assign<&ast::Import::name>(ref<STRING_>) +
             ((lit<"("> + assign<&ast::Import::kind>(kw<"func"> | kw<"tag">) +
               option(assign<&ast::Import::bindVar>(ref<VAR>)) +
               (option(lit<"("> + kw<"type"> +
                       assign<&ast::Import::typeUse>(varExpr) + lit<")">) +
                (many(WAT_TYPED_CLAUSE(kw<"param">,
                                       append<&ast::Import::params>)) +
                 many(WAT_RESULT_CLAUSE(append<&ast::Import::results>)))) +
               lit<")">) |
              (lit<"("> + assign<&ast::Import::kind>(kw<"table">) +
               option(assign<&ast::Import::bindVar>(ref<VAR>)) +
               ImportTableLimits + lit<")">) |
              (lit<"("> + assign<&ast::Import::kind>(kw<"memory">) +
               option(assign<&ast::Import::bindVar>(ref<VAR>)) +
               ImportMemoryLimits + lit<")">) |
              (lit<"("> + assign<&ast::Import::kind>(kw<"global">) +
               option(assign<&ast::Import::bindVar>(ref<VAR>)) +
               WAT_GLOBAL_TYPE(&ast::Import::valueType,
                               &ast::Import::mutable_) +
               lit<")">)) +
             lit<")">> {};

  struct ExportRule
      : rule<lit<"("> + kw<"export"> +
             assign<&ast::Export::name>(ref<STRING_>) + lit<"("> +
             (assign<&ast::Export::kind>(kw<"func">) |
              assign<&ast::Export::kind>(kw<"table">) |
              assign<&ast::Export::kind>(kw<"memory">) |
              assign<&ast::Export::kind>(kw<"global"> | kw<"tag">)) +
             assign<&ast::Export::var_>(varExpr) + lit<")"> + lit<")">> {};

  struct ModuleRule
      : rule<lit<"("> + kw<"module"> +
             option(assign<&ast::Module::name>(ref<VAR>)) +
             many(WAT_MODULE_FIELD(append<&ast::Module::fields>)) + lit<")">> {
  };

  struct BinQuoteModuleRule
      : rule<lit<"("> + kw<"module"> +
             option(assign<&ast::BinQuoteModule::name>(ref<VAR>)) +
             (assign<&ast::BinQuoteModule::kind>(kw<"binary">) |
              assign<&ast::BinQuoteModule::kind>(kw<"quote">)) +
             many(append<&ast::BinQuoteModule::strings>(ref<STRING_>)) +
             lit<")">> {};

  struct ActionRule
      : rule<(lit<"("> + assign<&ast::Action::kind>(kw<"invoke">) +
              option(assign<&ast::Action::moduleVar>(ref<VAR>)) +
              assign<&ast::Action::name>(ref<STRING_>) +
              many(append<&ast::Action::args>(ref<WconstRule>)) + lit<")">) |
             (lit<"("> + assign<&ast::Action::kind>(kw<"get">) +
              option(assign<&ast::Action::moduleVar>(ref<VAR>)) +
              assign<&ast::Action::name>(ref<STRING_>) + lit<")">)> {};

  static constexpr auto scriptModuleExpr =
      ref<ModuleRule> | ref<BinQuoteModuleRule>;

  struct AssertionRule
      : rule<lit<"("> +
             ((assign<&ast::Assertion::kind>(kw<"assert_malformed"> |
                                             kw<"assert_invalid"> |
                                             kw<"assert_unlinkable">) +
               assign<&ast::Assertion::module_>(scriptModuleExpr) +
               assign<&ast::Assertion::message>(ref<STRING_>)) |
              (assign<&ast::Assertion::kind>(kw<"assert_trap">) +
               (assign<&ast::Assertion::module_>(scriptModuleExpr) |
                assign<&ast::Assertion::action>(ref<ActionRule>)) +
               assign<&ast::Assertion::message>(ref<STRING_>)) |
              (assign<&ast::Assertion::kind>(kw<"assert_exhaustion">) +
               assign<&ast::Assertion::action>(ref<ActionRule>) +
               assign<&ast::Assertion::message>(ref<STRING_>)) |
              (assign<&ast::Assertion::kind>(kw<"assert_return">) +
               assign<&ast::Assertion::action>(ref<ActionRule>) +
               many(append<&ast::Assertion::results>(ref<WconstRule>))) |
              (assign<&ast::Assertion::kind>(kw<"assert_exception">) +
               assign<&ast::Assertion::action>(ref<ActionRule>))) +
             lit<")">> {};

  struct RegisterRule
      : rule<lit<"("> + kw<"register"> +
             assign<&ast::Register::name>(ref<STRING_>) +
             option(assign<&ast::Register::var_>(ref<VAR>)) + lit<")">> {};

  static constexpr auto cmdExpr = ref<ActionRule> | ref<AssertionRule> |
                                  ref<ModuleRule> | ref<BinQuoteModuleRule> |
                                  ref<RegisterRule>;

  struct ScriptRule
      : rule<many(append<&ast::Script::cmds>(cmdExpr)) |
             some(WAT_MODULE_FIELD(append<&ast::Script::fields>))> {};

  using entry = ScriptRule;
  using ignore = WS;
  using hidden = list<LINE_COMMENT, BLOCK_COMMENT>;

#undef WAT_INSTR_LIST
#undef WAT_OFFSET
#undef WAT_TYPED_CLAUSE
#undef WAT_RESULT_CLAUSE
#undef WAT_MEMORY_TYPE
#undef WAT_GLOBAL_TYPE
#undef WAT_MODULE_FIELD
};

}  // namespace wat
