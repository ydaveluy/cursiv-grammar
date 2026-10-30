

#pragma once

#include <cstdint>
#include <memory_resource>
#include <optional>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "wat/core/ast.hpp"

namespace wat {
inline constexpr auto hexDigit = [](char c) {
  return c >= '0' && c <= '9'   ? c - '0'
         : c >= 'a' && c <= 'f' ? c - 'a' + 10
         : c >= 'A' && c <= 'F' ? c - 'A' + 10
                                : -1;
};
// wast's Unicode string escape permits internal underscores, including
// repeated ones, but requires hexadecimal digits at both ends.
inline constexpr std::optional<std::uint32_t> unicodeScalar(std::string_view text) {
  if (text.empty() || hexDigit(text.front()) < 0 || hexDigit(text.back()) < 0)
    return {};
  std::uint32_t value = 0;
  for (const char c : text) {
    if (c == '_') continue;
    const int digit = hexDigit(c);
    if (digit < 0 || value > 0x10ffff / 16) return {};
    value = value * 16 + digit;
  }
  if (value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff)) return {};
  return value;
}

inline constexpr auto stringValue =
    [](std::string_view text,
       std::pmr::memory_resource* memory) -> std::string_view {
  if (text.size() < 2) return {};
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
        const auto decoded = end == text.npos ? std::nullopt :
            unicodeScalar(text.substr(i + 2, end - i - 2));
        if (decoded) {
          const auto scalar = *decoded;
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
  static constexpr auto Scalar = cr<R"(\x00-\u{10FFFF})">;
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  struct LINE_COMMENT
      : terminal<lit<";;"> + many(~(cr<R"(\r\n)"> | ~Scalar)) + option(eol)> {};
  struct BLOCK_COMMENT
      : terminal<lit<"(;"> +
                 many(ref<BLOCK_COMMENT> | (!lit<"(;"> + !lit<";)"> + Scalar)) +
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
      : terminal<as<stringValue>(
            lit<"\""> + many((lit<"\\"> + Scalar) | ~(cr<R"("\\)"> | ~Scalar)) + lit<"\"">)> {
  };

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
      : rule<as<std::string_view>(
            ref<VAR> | ref<NAT> | AbstractHeapType |
            (lit<"("> + kw<"shared"> + AbstractHeapType + lit<")">))> {};
  struct RefType
      : rule<as<std::string_view>(lit<"("> + kw<"ref"> + option(kw<"null">) +
                                  ref<HeapType> + lit<")">)> {};
  struct ValueType
      : rule<as<std::string_view>(ref<VALUE_TYPE> | ref<RefType>)> {};
  struct StorageType
      : rule<as<std::string_view>(kw<"i8"> | kw<"i16"> | ref<ValueType>)> {};

  static constexpr auto varExpr = ref<NAT> | ref<VAR>;
  static constexpr auto literalExpr =
      ref<NAT> | ref<INT> | ref<FLOAT> | ref<NAN_EXPECTED>;

  struct Instr;
  struct Expr;
  struct FlatInstr;

  struct Wconst;
  struct InlineImport;
  struct TypeDef;
  struct RecGroup;
  struct Global;
  struct Table;
  struct Memory;
  struct Func;
  struct Elem;
  struct Data;
  struct Start;
  struct Import;
  struct Export;
  struct Module;
  struct BinQuoteModule;
  struct Action;
  struct Assertion;
  struct Register;
  struct Script;
  struct Tag;

  static constexpr auto addArg = append<&ast::Instr::args>;

#define WAT_INSTR_LIST(AddFn) many(AddFn(ref<Instr>))
#define WAT_OFFSET(AddFn)                                         \
  ((lit<"("> + kw<"offset"> + WAT_INSTR_LIST(AddFn) + lit<")">) | \
   AddFn(ref<Expr>))
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
  AddFn(ref<TypeDef> | ref<RecGroup> | ref<Global> |             \
        ref<Table> | ref<Memory> | ref<Func> | ref<Elem> |   \
        ref<Data> | ref<Start> | ref<Import> | ref<Export> | \
        ref<Tag> | ref<Annotation>)

  struct Clause;
  struct FlatMarker;
  struct Delegate;
  struct Annotation;
  static constexpr auto ImmediateWord =
      kw<"i8x16"> | kw<"i16x8"> | kw<"i32x4"> | kw<"i64x2"> | kw<"f32x4"> |
      kw<"f64x2"> | AbstractHeapType;
  struct PlainOpcode
      : rule<as<std::string_view>(!(kw<"block"> | kw<"loop"> | kw<"if"> |
                                    kw<"try"> | kw<"try_table"> | kw<"end"> |
                                    kw<"else"> | kw<"catch"> | kw<"catch_all"> |
                                    kw<"delegate">)+ref<OPCODE>)> {};
  static constexpr auto Immediate =
      literalExpr | ref<VAR> | ref<OFFSET_EQ_NAT> | ref<ALIGN_EQ_NAT> |
      ref<STRING_> | ImmediateWord | ref<VALUE_TYPE> | ref<RefType>;
  struct Clause
      : rule<flag<&ast::Instr::folded>(lit<"(">) +
             assign<&ast::Instr::op>(kw<"type"> | kw<"param"> | kw<"result"> |
                                     kw<"memory"> | kw<"table">) +
             many(addArg(Immediate | ref<ValueType>)) + lit<")">> {};
  static constexpr auto PlainInstr = assign<&ast::Instr::op>(ref<PlainOpcode>) +
                                     many(addArg(Immediate | ref<Clause>));
  struct FlatMarker
      : rule<(assign<&ast::Instr::op>(kw<"else">) +
              option(assign<&ast::Instr::label>(ref<VAR>)) +
              many(addArg(ref<Instr>))) |
             (assign<&ast::Instr::op>(kw<"catch"> | kw<"catch_all">) +
              many(addArg(varExpr)) + many(addArg(ref<Instr>)))> {};
  struct Delegate
      : rule<assign<&ast::Instr::op>(kw<"delegate">) + addArg(varExpr)> {};
  struct FlatInstr
      : rule<(assign<&ast::Instr::op>(kw<"block"> | kw<"loop"> | kw<"if"> |
                                      kw<"try"> | kw<"try_table">) +
              option(assign<&ast::Instr::label>(ref<VAR>)) +
              many(addArg(ref<Instr>)) + many(addArg(ref<FlatMarker>)) +
              ((kw<"end"> + option(assign<&ast::Instr::endLabel>(ref<VAR>))) |
               addArg(ref<Delegate>))) |
             PlainInstr> {};
  struct ExprOpcode
      : rule<as<std::string_view>(!(kw<"ref"> + !lit<".">)+ref<OPCODE>)> {};
  struct Expr
      : rule<flag<&ast::Instr::folded>(lit<"(">) +
             ((assign<&ast::Instr::op>(kw<"block"> | kw<"loop"> | kw<"if"> |
                                       kw<"try"> | kw<"try_table">) +
               option(assign<&ast::Instr::label>(ref<VAR>))) |
              assign<&ast::Instr::op>(ref<ExprOpcode>)) +
             many(addArg(Immediate | ref<Instr>)) + lit<")">> {};
  struct ANNOTATION : terminal<lit<"@"> + cr<"a-z"> + many(cr<"a-z0-9_.">)> {};
  struct AnnotationPosition
      : rule<flag<&ast::Instr::folded>(lit<"(">) +
             assign<&ast::Instr::op>(kw<"before"> | kw<"after">) +
             addArg(ref<OPCODE>) + lit<")">> {};
  struct Annotation
      : rule<flag<&ast::Instr::folded>(lit<"(">) +
             assign<&ast::Instr::op>(ref<ANNOTATION>) +
             many(addArg(Immediate | ref<AnnotationPosition> |
                         ref<Expr>)) +
             lit<")">> {};
  struct Instr : rule<ref<Expr> | ref<FlatInstr>> {};

  struct Wconst
      : rule<lit<"("> + assign<&ast::Wconst::valueType>(ref<CONST>) +
             some(append<&ast::Wconst::literals>(literalExpr | ImmediateWord)) +
             lit<")">> {};

  struct InlineImport
      : rule<lit<"("> + kw<"import"> +
             assign<&ast::Import::module_>(ref<STRING_>) +
             assign<&ast::Import::name>(ref<STRING_>) + lit<")">> {};

  static constexpr auto FieldValue =
      (lit<"("> + flag<&ast::Binding::mutable_>(kw<"mut">) +
       assign<&ast::Binding::valueType>(ref<StorageType>) + lit<")">) |
      assign<&ast::Binding::valueType>(ref<StorageType>);
  struct Field
      : rule<option(assign<&ast::Binding::name>(ref<VAR>)) + FieldValue> {};
  static constexpr auto CompositeType =
      (assign<&ast::TypeDef::kind>(kw<"func">) +
       many(WAT_TYPED_CLAUSE(kw<"param">, append<&ast::TypeDef::params>)) +
       many(WAT_RESULT_CLAUSE(append<&ast::TypeDef::results>))) |
      (assign<&ast::TypeDef::kind>(kw<"struct">) +
       many((lit<"("> + kw<"field"> +
             many(append<&ast::TypeDef::fields>(ref<Field>)) + lit<")">) |
            append<&ast::TypeDef::fields>(ref<Field>))) |
      (assign<&ast::TypeDef::kind>(kw<"array">) +
       many(append<&ast::TypeDef::fields>(ref<Field>)));
  static constexpr auto SharedType =
      (flag<&ast::TypeDef::shared>(kw<"shared">) + lit<"("> +
       CompositeType + lit<")">) |
      CompositeType;
  struct SubHeader
      : rule<as<[](std::string_view text) { return text.ends_with("final"); }>(
            kw<"sub"> + option(kw<"final">))> {};
  struct TypeDef
      : rule<lit<"("> + kw<"type"> +
             option(assign<&ast::TypeDef::name>(ref<VAR>)) + lit<"("> +
             ((assign<&ast::TypeDef::final_>(ref<SubHeader>) +
               many(append<&ast::TypeDef::supertypes>(varExpr)) + lit<"("> +
               SharedType + lit<")">) |
              SharedType) +
             lit<")"> + lit<")">> {};
  struct RecGroup
      : rule<lit<"("> + kw<"rec"> +
             many(append<&ast::RecGroup::types>(ref<TypeDef>)) + lit<")">> {
  };

  struct Global
      : rule<lit<"("> + kw<"global"> +
             option(assign<&ast::Global::name>(ref<VAR>)) +
             many(lit<"("> + kw<"export"> +
                  append<&ast::Global::exports>(ref<STRING_>) + lit<")">) +
             ((assign<&ast::Global::import_>(ref<InlineImport>) +
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
  struct Table
      : rule<lit<"("> + kw<"table"> +
             option(assign<&ast::Table::name>(ref<VAR>)) +
             many(lit<"("> + kw<"export"> +
                  append<&ast::Table::exports>(ref<STRING_>) + lit<")">) +
             ((assign<&ast::Table::import_>(ref<InlineImport>) +
               TableLimits) |
              TableLimits |
              (assign<&ast::Table::elemType>(kw<"funcref">) + lit<"("> +
               kw<"elem"> +
               many(append<&ast::Table::inlineElems>(varExpr) |
                    append<&ast::Table::inlineExprs>(ref<Expr>)) +
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
  struct Memory
      : rule<lit<"("> + kw<"memory"> +
             option(assign<&ast::Memory::name>(ref<VAR>)) +
             many(lit<"("> + kw<"export"> +
                  append<&ast::Memory::exports>(ref<STRING_>) + lit<")">) +
             ((assign<&ast::Memory::import_>(ref<InlineImport>) +
               MemoryLimits) |
              MemoryLimits |
              (option(assign<&ast::Memory::indexType>(kw<"i32"> | kw<"i64">)) +
               lit<"("> + kw<"data"> +
               many(append<&ast::Memory::inlineData>(ref<STRING_>)) +
               lit<")">)) +
             lit<")">> {};

  struct Func
      : rule<create<ast::Func> + lit<"("> + kw<"func"> +
             option(assign<&ast::Func::name>(ref<VAR>)) +
             many(lit<"("> + kw<"export"> +
                  append<&ast::Func::exports>(ref<STRING_>) + lit<")">) +
             ((assign<&ast::Func::import_>(ref<InlineImport>) +
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

  struct Elem
      : rule<lit<"("> + kw<"elem"> +
             option(assign<&ast::Elem::name>(ref<VAR>)) +
             option(flag<&ast::Elem::declared>(kw<"declare">)) +
             option((lit<"("> + kw<"table"> +
                     assign<&ast::Elem::tableVar>(varExpr) + lit<")">) |
                    assign<&ast::Elem::tableVar>(ref<NAT>)) +
             option(WAT_OFFSET(append<&ast::Elem::offset>)) +
             option(assign<&ast::Elem::elemType>(ref<ValueType>) | kw<"func">) +
             many(append<&ast::Elem::funcVars>(varExpr) |
                  append<&ast::Elem::expressions>(ref<Expr>)) +
             lit<")">> {};
  struct Data
      : rule<lit<"("> + kw<"data"> +
             option(assign<&ast::Data::name>(ref<VAR>)) +
             option((lit<"("> + kw<"memory"> +
                     assign<&ast::Data::memVar>(varExpr) + lit<")">) |
                    assign<&ast::Data::memVar>(ref<NAT>)) +
             option(WAT_OFFSET(append<&ast::Data::offset>)) +
             many(append<&ast::Data::strings>(ref<STRING_>)) + lit<")">> {};
  struct Tag
      : rule<create<ast::Tag> + lit<"("> + kw<"tag"> +
             option(assign<&ast::Func::name>(ref<VAR>)) +
             many(lit<"("> + kw<"export"> +
                  append<&ast::Func::exports>(ref<STRING_>) + lit<")">) +
             option(assign<&ast::Func::import_>(ref<InlineImport>)) +
             option(lit<"("> + kw<"type"> +
                    assign<&ast::Func::typeUse>(varExpr) + lit<")">) +
             many(WAT_TYPED_CLAUSE(kw<"param">, append<&ast::Func::params>)) +
             many(WAT_RESULT_CLAUSE(append<&ast::Func::results>)) + lit<")">> {
  };

  struct Start : rule<lit<"("> + kw<"start"> +
                          assign<&ast::Start::func>(varExpr) + lit<")">> {};

  struct Import
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

  struct Export
      : rule<lit<"("> + kw<"export"> +
             assign<&ast::Export::name>(ref<STRING_>) + lit<"("> +
             (assign<&ast::Export::kind>(kw<"func">) |
              assign<&ast::Export::kind>(kw<"table">) |
              assign<&ast::Export::kind>(kw<"memory">) |
              assign<&ast::Export::kind>(kw<"global"> | kw<"tag">)) +
             assign<&ast::Export::var_>(varExpr) + lit<")"> + lit<")">> {};

  struct Module
      : rule<lit<"("> + kw<"module"> +
             option(assign<&ast::Module::name>(ref<VAR>)) +
             many(WAT_MODULE_FIELD(append<&ast::Module::fields>)) + lit<")">> {
  };

  struct BinQuoteModule
      : rule<lit<"("> + kw<"module"> +
             option(assign<&ast::BinQuoteModule::name>(ref<VAR>)) +
             (assign<&ast::BinQuoteModule::kind>(kw<"binary">) |
              assign<&ast::BinQuoteModule::kind>(kw<"quote">)) +
             many(append<&ast::BinQuoteModule::strings>(ref<STRING_>)) +
             lit<")">> {};

  struct Action
      : rule<(lit<"("> + assign<&ast::Action::kind>(kw<"invoke">) +
              option(assign<&ast::Action::moduleVar>(ref<VAR>)) +
              assign<&ast::Action::name>(ref<STRING_>) +
              many(append<&ast::Action::args>(ref<Wconst>)) + lit<")">) |
             (lit<"("> + assign<&ast::Action::kind>(kw<"get">) +
              option(assign<&ast::Action::moduleVar>(ref<VAR>)) +
              assign<&ast::Action::name>(ref<STRING_>) + lit<")">)> {};

  static constexpr auto scriptModuleExpr =
      ref<Module> | ref<BinQuoteModule>;

  struct Assertion
      : rule<lit<"("> +
             ((assign<&ast::Assertion::kind>(kw<"assert_malformed"> |
                                             kw<"assert_invalid"> |
                                             kw<"assert_unlinkable">) +
               assign<&ast::Assertion::module_>(scriptModuleExpr) +
               assign<&ast::Assertion::message>(ref<STRING_>)) |
              (assign<&ast::Assertion::kind>(kw<"assert_trap">) +
               (assign<&ast::Assertion::module_>(scriptModuleExpr) |
                assign<&ast::Assertion::action>(ref<Action>)) +
               assign<&ast::Assertion::message>(ref<STRING_>)) |
              (assign<&ast::Assertion::kind>(kw<"assert_exhaustion">) +
               assign<&ast::Assertion::action>(ref<Action>) +
               assign<&ast::Assertion::message>(ref<STRING_>)) |
              (assign<&ast::Assertion::kind>(kw<"assert_return">) +
               assign<&ast::Assertion::action>(ref<Action>) +
               many(append<&ast::Assertion::results>(ref<Wconst>))) |
              (assign<&ast::Assertion::kind>(kw<"assert_exception">) +
               assign<&ast::Assertion::action>(ref<Action>))) +
             lit<")">> {};

  struct Register
      : rule<lit<"("> + kw<"register"> +
             assign<&ast::Register::name>(ref<STRING_>) +
             option(assign<&ast::Register::var_>(ref<VAR>)) + lit<")">> {};

  static constexpr auto cmdExpr = ref<Action> | ref<Assertion> |
                                  ref<Module> | ref<BinQuoteModule> |
                                  ref<Register>;

  struct Script
      : rule<many(append<&ast::Script::cmds>(cmdExpr)) |
             some(WAT_MODULE_FIELD(append<&ast::Script::fields>))> {};

  using entry = Script;
  using ignored = WS;
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
