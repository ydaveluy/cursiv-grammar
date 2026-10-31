// The odin language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "odin/core/ast.hpp"

namespace odin {

struct Odin : cursiv::GrammarSpec {

  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct LINE_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct BUILD_TAG : terminal<lit<"#+"> + many(cr<R"(^\r\n)">)> {};
  struct BLOCK_COMMENT
      : terminal<lit<"/*"> +
                 many(ref<BLOCK_COMMENT> | (!lit<"/*"> + !lit<"*/"> + any)) +
                 lit<"*/">> {};

  static constexpr auto IdentStart = uc::L | cr<"_">;
  static constexpr auto IdentCont = uc::L | uc::Nd | cr<"_">;
  static constexpr auto word_boundary = IdentCont;
  static constexpr auto Reserved =
      kw<"asm"> | kw<"auto_cast"> | kw<"bit_field"> | kw<"bit_set"> |
      kw<"break"> | kw<"case"> | kw<"cast"> | kw<"context"> |
      kw<"continue"> | kw<"defer"> | kw<"distinct"> | kw<"do"> |
      kw<"dynamic"> | kw<"else"> | kw<"enum"> | kw<"fallthrough"> |
      kw<"for"> | kw<"foreign"> | kw<"if"> | kw<"import"> | kw<"in"> |
      kw<"map"> | kw<"matrix"> | kw<"not_in"> | kw<"or_break"> |
      kw<"or_continue"> | kw<"or_else"> | kw<"or_return"> |
      kw<"package"> | kw<"proc"> | kw<"return"> | kw<"struct"> |
      kw<"switch"> | kw<"transmute"> | kw<"typeid"> | kw<"union"> |
      kw<"using"> | kw<"when"> | kw<"where">;
  struct ID : terminal<!Reserved + IdentStart + many(IdentCont)> {};

  static constexpr auto DEC_DIGITS = cr<"0-9"> + many(cr<"0-9_">);
  static constexpr auto HEX_DIGITS = cr<"0-9a-fA-F"> + many(cr<"0-9a-fA-F_">);
  static constexpr auto OCT_DIGITS = cr<"0-7"> + many(cr<"0-7_">);
  static constexpr auto BIN_DIGITS = cr<"01"> + many(cr<"01_">);
  static constexpr auto DEC_EXP = cr<"eE"> + option(cr<"+-">) + some(d);
  static constexpr auto HEX_EXP = cr<"pP"> + option(cr<"+-">) + some(d);

  struct INTEGER
      : terminal<(lit<"0b"> + BIN_DIGITS) | (lit<"0o"> + OCT_DIGITS) |
                 (lit<"0x"> + HEX_DIGITS) | (lit<"0d"> + DEC_DIGITS) |
                 (lit<"0z"> + cr<"0-9aAbB"> + many(cr<"0-9aAbB_">)) |
                 DEC_DIGITS> {};

  static constexpr auto DEC_FLOAT =
      (DEC_DIGITS + lit<".", !lit<".">> + option(DEC_DIGITS) + option(DEC_EXP)) |
      (lit<"."> + DEC_DIGITS + option(DEC_EXP)) | (DEC_DIGITS + DEC_EXP);
  struct FLOAT
      : terminal<DEC_FLOAT |
                 (lit<"0h"> + (repeat<16>(cr<"0-9a-fA-F">) |
                                repeat<8>(cr<"0-9a-fA-F">) |
                                repeat<4>(cr<"0-9a-fA-F">)) +
                  !cr<"0-9a-fA-F_">) |
                 // Retained imported C-style hexadecimal float extension.
                 (lit<"0x"> + HEX_DIGITS + lit<"."> + option(HEX_DIGITS) +
                  option(HEX_EXP))> {};
  struct IMAG : terminal<(DEC_FLOAT | DEC_DIGITS) + cr<"ijk">> {};

  static constexpr auto ESCAPE_SEQ =
      lit<"\\"> +
      (cr<R"(abefnrtv\\'")">  //
       | (cr<"0-7"> + option(cr<"0-7">) + option(cr<"0-7">))  //
       | (lit<"x"> + repeat<2>(cr<"0-9a-fA-F">))  //
       | (lit<"u"> + repeat<4>(cr<"0-9a-fA-F">))  //
       | (lit<"U"> + repeat<8>(cr<"0-9a-fA-F">)));

  struct STRING
      : terminal<lit<"\""> + many(ESCAPE_SEQ | ~cr<R"("\\\r\n)">) + lit<"\"">> {
  };
  struct RAW_STRING : terminal<lit<"`"> + many(~cr<"`">) + lit<"`">> {};
  struct CHAR
      : terminal<lit<"'"> + (ESCAPE_SEQ | ~cr<R"('\\\r\n)">) + lit<"'">> {};

  static constexpr auto BasicTypeKw =
      kw<"bool"> | kw<"b8"> | kw<"b16"> | kw<"b32"> | kw<"b64">  //
      | kw<"int"> | kw<"i8"> | kw<"i16"> | kw<"i32"> | kw<"i64"> | kw<"i128">  //
      | kw<"uint"> | kw<"u8"> | kw<"u16"> | kw<"u32"> | kw<"u64"> |
      kw<"u128"> | kw<"uintptr">  //
      | kw<"f16"> | kw<"f32"> | kw<"f64">  //
      | kw<"complex32"> | kw<"complex64"> | kw<"complex128">  //
      | kw<"quaternion64"> | kw<"quaternion128"> | kw<"quaternion256">  //
      | kw<"rune"> | kw<"string"> | kw<"cstring"> | kw<"rawptr"> |
      kw<"typeid"> | kw<"any">;

  struct TypeSpec;
  struct BasicType;
  struct NamedType;
  struct PolyType;
  struct DistinctType;
  struct PointerType;
  struct MultiPointerType;
  struct ArrayType;
  struct SliceType;
  struct DynamicArrayType;
  struct MapType;
  struct ReturnParam;
  struct Param;
  struct ProcType;
  struct WhereCondition;
  struct PolyParamGroup;
  struct StructField;
  struct StructType;
  struct EnumField;
  struct EnumType;
  struct UnionType;
  struct RangeExpr;
  struct BitSetType;
  struct BitFieldEntry;
  struct BitFieldType;
  struct MatrixType;
  struct SoaArrayType;
  struct SparseArrayType;
  struct TypeArgs;

  struct Expression;
  struct Ident;
  struct Literal;
  struct DefinedExpr;
  struct ProcedureExpr;
  struct LoadExpr;
  struct AssertExpr;
  struct Operand;
  struct PostfixLevel;
  struct CastLevel;
  struct UnaryExpr;
  struct CastExpr;
  struct TransmuteExpr;
  struct AutoCastExpr;
  struct BinaryLevel;
  struct CallArg;
  struct LiteralElem;
  struct ProcExpr;
  struct ProcGroup;

  struct Statement;
  struct Block;
  struct LabelStmt;
  struct SimpleStmt;
  struct AssignStmt;
  struct IfStmt;
  struct ForStmt;
  struct SwitchCase;
  struct SwitchStmt;
  struct TypeSwitchCase;
  struct TypeSwitchStmt;
  struct WhenStmt;
  struct AssertStmt;

  struct NamedDecl;
  struct VarDecl;
  struct ForeignImportDecl;
  struct ForeignEntry;
  struct ForeignBlock;
  struct TopDecl;
  struct Attribute;

  struct BasicType : rule<assign<&ast::BasicType::keyword>(BasicTypeKw)> {};

  struct TypeArgs
      : rule<some(append<&ast::TypeArgs::args>(ref<TypeSpec> | ref<Expression>),
                  lit<",">)> {};

  struct NamedType
      : rule<((assign<&ast::NamedType::pkg>(ref<ID>) + lit<"."> +
               assign<&ast::NamedType::name>(ref<ID>))  //
              | assign<&ast::NamedType::name>(ref<ID>)) +
             option(lit<"("> + assign<&ast::NamedType::typeArgs>(ref<TypeArgs>) +
                    lit<")">)> {};

  struct PolyType
      : rule<lit<"$"> + assign<&ast::PolyType::name>(ref<ID>) +
             option(lit<"/"> +
                    assign<&ast::PolyType::specialization>(ref<TypeSpec>))> {};

  struct DistinctType
      : rule<kw<"distinct"> + assign<&ast::DistinctType::base>(ref<TypeSpec>)> {
  };

  struct PointerType
      : rule<lit<"^"> + assign<&ast::PointerType::base>(ref<TypeSpec>)> {};

  struct MultiPointerType
      : rule<lit<"["> + lit<"^"> + lit<"]"> +
             assign<&ast::MultiPointerType::base>(ref<TypeSpec>)> {};

  struct ArrayType
      : rule<(lit<"["> + assign<&ast::ArrayType::size>(ref<Expression>) +
              lit<"]"> + assign<&ast::ArrayType::elem>(ref<TypeSpec>))  //
             | (lit<"["> + lit<"$"> +
                assign<&ast::ArrayType::polyDim>(ref<ID>) + lit<"]"> +
                assign<&ast::ArrayType::elem>(ref<TypeSpec>))  //
             | (lit<"["> + flag<&ast::ArrayType::inferred>(lit<"?">) +
                lit<"]"> + assign<&ast::ArrayType::elem>(ref<TypeSpec>))> {};

  struct SliceType
      : rule<lit<"["> + lit<"]"> +
             assign<&ast::SliceType::elem>(ref<TypeSpec>)> {};

  struct DynamicArrayType
      : rule<lit<"["> + kw<"dynamic"> +
             option(lit<";"> +
                    assign<&ast::DynamicArrayType::capacity>(ref<Expression>)) +
             lit<"]"> +
             assign<&ast::DynamicArrayType::elem>(ref<TypeSpec>)> {};

  struct MapType
      : rule<kw<"map"> + lit<"["> + assign<&ast::MapType::key>(ref<TypeSpec>) +
             lit<"]"> + assign<&ast::MapType::value>(ref<TypeSpec>)> {};

  struct ReturnParam
      : rule<(some(append<&ast::ReturnParam::names>(ref<ID>), lit<",">) +
              lit<":"> + assign<&ast::ReturnParam::type>(ref<TypeSpec>))  //
             | assign<&ast::ReturnParam::type>(ref<TypeSpec>)> {};

  struct Param
      : rule<(some(append<&ast::Param::names>(ref<ID>), lit<",">) + lit<":"> +
              flag<&ast::Param::variadic>(lit<"..">) +
              assign<&ast::Param::type>(ref<TypeSpec>))  // variadic
             | (flag<&ast::Param::usingParam>(kw<"using">) +
                some(append<&ast::Param::names>(ref<ID>), lit<",">) +
                lit<":"> + assign<&ast::Param::type>(ref<TypeSpec>) +
                option(lit<"="> +
                       assign<&ast::Param::defaultValue>(ref<Expression>)))  //
             | (some(append<&ast::Param::names>(ref<ID>), lit<",">) +
                lit<":"> + assign<&ast::Param::type>(ref<TypeSpec>) +
                option(lit<"="> +
                       assign<&ast::Param::defaultValue>(ref<Expression>)))  //
             | (some(append<&ast::Param::names>(ref<ID>), lit<",">) +
                flag<&ast::Param::inferredDefault>(lit<":=">) +
                assign<&ast::Param::defaultValue>(ref<Expression>))  //
             | (flag<&ast::Param::isPoly>(lit<"$">) +
                append<&ast::Param::names>(ref<ID>) + lit<":"> +
                assign<&ast::Param::type>(ref<TypeSpec>) +
                option(lit<"/"> +
                       assign<&ast::Param::specialization>(ref<TypeSpec>)))  //
             | assign<&ast::Param::type>(ref<TypeSpec>)> {};

  struct ProcType
      : rule<kw<"proc"> +
             option(assign<&ast::ProcType::callingConvention>(ref<STRING>)) +
             lit<"("> +
             many(append<&ast::ProcType::params>(ref<Param>), lit<",">) +
             option(lit<",">) + lit<")"> +
             option(lit<"->"> +
                    ((lit<"("> +
                      some(append<&ast::ProcType::results>(ref<ReturnParam>),
                           lit<",">) +
                      lit<")">)  //
                     | append<&ast::ProcType::results>(ref<ReturnParam>)))> {};

  struct WhereCondition
      : rule<(assign<&ast::WhereCondition::expr>(ref<Expression>) +
              assign<&ast::WhereCondition::op>(lit<"=="> | lit<"!=">) +
              assign<&ast::WhereCondition::type>(ref<TypeSpec>))  //
             | assign<&ast::WhereCondition::expr>(ref<Expression>)> {};

  struct PolyParamGroup
      : rule<some(lit<"$"> + append<&ast::PolyParamGroup::names>(ref<ID>),
                  lit<",">) +
             lit<":"> + assign<&ast::PolyParamGroup::type>(ref<TypeSpec>) +
             option(lit<"/"> + assign<&ast::PolyParamGroup::specialization>(
                                    ref<TypeSpec>))> {};

  struct StructField
      : rule<option(flag<&ast::StructField::usingField>(kw<"using">)) +
             some(append<&ast::StructField::names>(ref<ID>), lit<",">) +
             lit<":"> + assign<&ast::StructField::type>(ref<TypeSpec>) +
             option(assign<&ast::StructField::tag>(ref<STRING> |
                                                    ref<RAW_STRING>))> {};

  static constexpr auto StructAlign =
      append<&ast::StructType::tags>((lit<"#"> + kw<"align">)) + lit<"("> +
      append<&ast::StructType::align>(ref<Expression>) + lit<")">;
  static constexpr auto StructMinFieldAlign =
      append<&ast::StructType::tags>((lit<"#"> + kw<"min_field_align">)) + lit<"("> +
      append<&ast::StructType::minFieldAlign>(ref<Expression>) + lit<")">;
  static constexpr auto StructTagItem =
      flag<&ast::StructType::packed>(
          append<&ast::StructType::tags>((lit<"#"> + kw<"packed">)))  //
      | flag<&ast::StructType::rawUnion>(
            append<&ast::StructType::tags>((lit<"#"> + kw<"raw_union">)))  //
      | flag<&ast::StructType::simple>(
            append<&ast::StructType::tags>((lit<"#"> + kw<"simple">)))  //
      | flag<&ast::StructType::allOrNone>(
            append<&ast::StructType::tags>((lit<"#"> + kw<"all_or_none">)))  //
      | (kw<"where"> +
         some(append<&ast::StructType::whereConditions>(ref<WhereCondition>),
              lit<",">));
  static constexpr auto StructTags =
      many(StructAlign | StructMinFieldAlign | StructTagItem);

  struct StructType
      : rule<kw<"struct"> +
             option(lit<"("> +
                    some(append<&ast::StructType::polyParams>(
                             ref<PolyParamGroup>),
                         lit<",">) +
                    lit<")">) +
             StructTags + lit<"{"> +
             option(some(append<&ast::StructType::fields>(ref<StructField>),
                         lit<",">) +
                    option(lit<",">)) +
             lit<"}">> {};

  struct EnumField
      : rule<assign<&ast::EnumField::name>(ref<ID>) +
             option(lit<"="> +
                    assign<&ast::EnumField::value>(ref<Expression>))> {};

  struct EnumType
      : rule<kw<"enum"> +
             option(assign<&ast::EnumType::underlying>(ref<TypeSpec>)) +
             lit<"{"> +
             option(some(append<&ast::EnumType::fields>(ref<EnumField>),
                         lit<",">) +
                    option(lit<",">)) +
             lit<"}">> {};

  static constexpr auto UnionTagItem =
      flag<&ast::UnionType::noNil>((lit<"#"> + kw<"no_nil">))  //
      | flag<&ast::UnionType::sharedNil>((lit<"#"> + kw<"shared_nil">))  //
      | flag<&ast::UnionType::maybe>((lit<"#"> + kw<"maybe">));

  struct UnionType
      : rule<kw<"union"> +
             option(lit<"("> +
                    some(append<&ast::UnionType::polyParams>(
                             ref<PolyParamGroup>),
                         lit<",">) +
                    lit<")">) +
             many(UnionTagItem) + lit<"{"> +
             option(some(append<&ast::UnionType::variants>(ref<TypeSpec>),
                         lit<",">) +
                    option(lit<",">)) +
             lit<"}">> {};

  // A range, or a plain value -- one rule for a for-loop's range clause, a
  // bit-set element, and a switch case value alike.
  struct RangeExpr
      : rule<assign<&ast::RangeExpr::low>(ref<Expression>) + lit<".."> +
             (flag<&ast::RangeExpr::inclusive>(lit<"=">) | lit<"<">) +
             assign<&ast::RangeExpr::high>(ref<Expression>)> {};

  static constexpr auto RangeOrExpr = ref<RangeExpr> | ref<Expression>;

  struct BitSetType
      : rule<kw<"bit_set"> + lit<"["> +
             assign<&ast::BitSetType::elem>(ref<RangeExpr> |
                                            ref<TypeSpec>) +
             option(lit<";"> + assign<&ast::BitSetType::word>(ref<TypeSpec>)) +
             lit<"]">> {};

  struct BitFieldEntry
      : rule<assign<&ast::BitFieldEntry::name>(ref<ID>) + lit<":"> +
             assign<&ast::BitFieldEntry::type>(ref<TypeSpec>) + lit<"|"> +
             assign<&ast::BitFieldEntry::width>(ref<Expression>)> {};

  struct BitFieldType
      : rule<kw<"bit_field"> +
             assign<&ast::BitFieldType::backing>(ref<TypeSpec>) + lit<"{"> +
             option(some(append<&ast::BitFieldType::entries>(
                             ref<BitFieldEntry>),
                         lit<",">) +
                    option(lit<",">)) +
             lit<"}">> {};

  struct MatrixType
      : rule<option(flag<&ast::MatrixType::rowMajor>((lit<"#"> + kw<"row_major">))) +
             kw<"matrix"> + lit<"["> +
             assign<&ast::MatrixType::rows>(ref<Expression>) + lit<","> +
             assign<&ast::MatrixType::cols>(ref<Expression>) + lit<"]"> +
             assign<&ast::MatrixType::elem>(ref<TypeSpec>)> {};

  struct SoaArrayType
      : rule<(lit<"#"> + kw<"soa">) +
             ((lit<"["> + assign<&ast::SoaArrayType::size>(ref<Expression>) +
               lit<"]">)  //
              | (lit<"["> +
                 flag<&ast::SoaArrayType::inferred>(lit<"?">) +
                 lit<"]">)  //
              | (lit<"["> + lit<"]">)  //
              | (lit<"["> +
                 flag<&ast::SoaArrayType::dynamic>(kw<"dynamic">) +
                 lit<"]">)) +
             assign<&ast::SoaArrayType::elem>(ref<TypeSpec>)> {};

  struct SparseArrayType
      : rule<(lit<"#"> + kw<"sparse">) + lit<"["> +
             assign<&ast::SparseArrayType::key>(ref<TypeSpec>) + lit<"]"> +
             assign<&ast::SparseArrayType::elem>(ref<TypeSpec>)> {};

  struct TypeSpec
      : rule<ref<BasicType>          //
             | ref<PointerType>      //
             | ref<MultiPointerType>  //
             | ref<ArrayType>        //
             | ref<SliceType>        //
             | ref<DynamicArrayType>  //
             | ref<MapType>          //
             | ref<ProcType>         //
             | ref<StructType>       //
             | ref<EnumType>         //
             | ref<UnionType>        //
             | ref<BitSetType>       //
             | ref<BitFieldType>     //
             | ref<MatrixType>       //
             | ref<SoaArrayType>     //
             | ref<SparseArrayType>  //
             | ref<DistinctType>     //
             | (lit<"("> + ref<TypeSpec> + lit<")">)  //
             | ref<PolyType>         //
             | ref<NamedType>> {};

  struct ProcExpr
      : rule<kw<"proc"> +
             option(assign<&ast::ProcExpr::callingConvention>(ref<STRING>)) +
             lit<"("> +
             many(append<&ast::ProcExpr::params>(ref<Param>), lit<",">) +
             option(lit<",">) + lit<")"> +
             option(lit<"->"> +
                    ((lit<"("> +
                      some(append<&ast::ProcExpr::results>(ref<ReturnParam>),
                           lit<",">) +
                      lit<")">)  //
                     | append<&ast::ProcExpr::results>(ref<ReturnParam>))) +
             option(kw<"where"> +
                    some(append<&ast::ProcExpr::whereConditions>(
                             ref<WhereCondition>),
                         lit<",">)) +
             assign<&ast::ProcExpr::body>(ref<Block>)> {};

  struct ProcGroup
      : rule<kw<"proc"> + lit<"{"> +
             some(append<&ast::ProcGroup::members>(ref<ID>), lit<",">) +
             lit<"}">> {};

  struct Ident
      : rule<(assign<&ast::Ident::pkg>(ref<ID>) + lit<"."> +
              assign<&ast::Ident::name>(ref<ID>)) |
             assign<&ast::Ident::name>(ref<ID>)> {};

  struct Literal
      : rule<assign<&ast::FloatLit::text>(ref<FLOAT>)  //
             | assign<&ast::ImagLit::text>(ref<IMAG>)  //
             | assign<&ast::IntLit::text>(ref<INTEGER>)  //
             | assign<&ast::StringLit::text>(ref<STRING>)  //
             | flag<&ast::StringLit::raw>(
                   assign<&ast::StringLit::text>(ref<RAW_STRING>))  //
             | assign<&ast::CharLit::text>(ref<CHAR>)  //
             | create<ast::NilLit>(kw<"nil">)  //
             | create<ast::BoolLit>(
                   flag<&ast::BoolLit::value>(kw<"true">) | kw<"false">)  //
             | create<ast::UninitLit>(lit<"---">)> {};

  struct DefinedExpr
      : rule<(lit<"#"> + kw<"defined">) + lit<"("> +
             assign<&ast::DefinedExpr::name>(ref<ID>) + lit<")">> {};
  struct ProcedureExpr : rule<create<ast::ProcedureExpr>((lit<"#"> + kw<"procedure">))> {
  };
  struct LoadExpr
      : rule<(lit<"#"> + kw<"load">) + lit<"("> +
             assign<&ast::LoadExpr::path>(ref<Expression>) + lit<")">> {};
  struct AssertExpr
      : rule<(lit<"#"> + kw<"assert">) + lit<"("> +
             assign<&ast::AssertExpr::cond>(ref<WhereCondition>) + lit<")">> {};

  static constexpr auto CompositeBody =
      option(some(append<&ast::CompositeLit::elems>(ref<LiteralElem>), lit<",">) +
             option(lit<",">)) +
      lit<"}">;

  struct Operand
      : rule<ref<Literal>  //
             | create<ast::ContextExpr>(kw<"context">) |
             ref<Ident>  //
             | create<ast::BasicTypeExpr>(
                   assign<&ast::BasicTypeExpr::keyword>(BasicTypeKw))  //
             | create<ast::ImplicitSelectorExpr>(
                   lit<"."> +
                   assign<&ast::ImplicitSelectorExpr::name>(ref<ID>))  //
             | (lit<"("> + ref<Expression> + lit<")">)  //
             | create<ast::CompositeLit>(lit<"{"> + CompositeBody)  //
             | ref<ProcExpr>  //
             | create<ast::CompositeLit>(
                   assign<&ast::CompositeLit::typeSpec>(ref<ArrayType>) +
                   lit<"{"> + CompositeBody)  //
             | create<ast::CompositeLit>(
                   assign<&ast::CompositeLit::typeSpec>(ref<SliceType>) +
                   lit<"{"> + CompositeBody)  //
             | create<ast::CompositeLit>(
                   assign<&ast::CompositeLit::typeSpec>(
                       ref<DynamicArrayType>) +
                   lit<"{"> + CompositeBody)  //
             | create<ast::CompositeLit>(
                   assign<&ast::CompositeLit::typeSpec>(ref<MapType>) +
                   lit<"{"> + CompositeBody)  //
             | create<ast::CompositeLit>(
                   assign<&ast::CompositeLit::typeSpec>(ref<StructType>) +
                   lit<"{"> + CompositeBody)  //
             | create<ast::CompositeLit>(
                   assign<&ast::CompositeLit::typeSpec>(ref<EnumType>) +
                   lit<"{"> + CompositeBody)  //
             | create<ast::CompositeLit>(
                   assign<&ast::CompositeLit::typeSpec>(ref<SoaArrayType>) +
                   lit<"{"> + CompositeBody)  //
             | create<ast::CompositeLit>(
                   assign<&ast::CompositeLit::typeSpec>(
                       ref<SparseArrayType>) +
                   lit<"{"> + CompositeBody)  //
             | create<ast::CompositeLit>(
                   assign<&ast::CompositeLit::typeSpec>(ref<MatrixType>) +
                   lit<"{"> + CompositeBody)  //
             | create<ast::CompositeLit>(
                   assign<&ast::CompositeLit::typeSpec>(ref<BitSetType>) +
                   lit<"{"> + CompositeBody) |
             create<ast::TypeExpr>(assign<&ast::TypeExpr::type>(
                 ref<ArrayType> | ref<SliceType> | ref<DynamicArrayType> |
                 ref<MapType> | ref<StructType> | ref<EnumType> |
                 ref<SoaArrayType> | ref<SparseArrayType> |
                 ref<MatrixType> | ref<BitSetType>))> {};

  static constexpr auto PrimaryBase = ref<Operand> | ref<DefinedExpr> |
                                      ref<ProcedureExpr> | ref<LoadExpr> |
                                      ref<AssertExpr>;

  static constexpr auto PostfixOp =
      nest<ast::IndexExpr, &ast::IndexExpr::array>(
          lit<"["> + assign<&ast::IndexExpr::index>(ref<Expression>) +
          option(lit<","> + assign<&ast::IndexExpr::index2>(ref<Expression>)) +
          lit<"]">)  //
      | nest<ast::SliceExpr, &ast::SliceExpr::array>(
            lit<"["> + option(assign<&ast::SliceExpr::low>(ref<Expression>)) +
            lit<":"> +
            option(assign<&ast::SliceExpr::high>(ref<Expression>)) +
            option(lit<":"> + assign<&ast::SliceExpr::cap>(ref<Expression>)) +
            lit<"]">)  //
      | nest<ast::CallExpr, &ast::CallExpr::callee>(
            lit<"("> +
            option(some(append<&ast::CallExpr::args>(ref<CallArg>), lit<",">) +
                   option(lit<",">)) +
            lit<")">)  //
      | nest<ast::UnwrapExpr, &ast::UnwrapExpr::object>(
            lit<"."> + lit<"("> + assign<&ast::UnwrapExpr::type>(ref<TypeSpec>) +
            lit<")">)  //
      | nest<ast::UnwrapExpr, &ast::UnwrapExpr::object>(lit<"."> + lit<"?">)  //
      | nest<ast::MemberExpr, &ast::MemberExpr::object>(
            lit<"."> + assign<&ast::MemberExpr::field>(ref<ID>))  //
      | nest<ast::DerefExpr, &ast::DerefExpr::operand>(lit<"^">)  //
      | nest<ast::OrJumpExpr, &ast::OrJumpExpr::value>(
            assign<&ast::OrJumpExpr::kind>(kw<"or_return">)) |
        nest<ast::OrJumpExpr, &ast::OrJumpExpr::value>(
            assign<&ast::OrJumpExpr::kind>(kw<"or_break"> | kw<"or_continue">) +
            option(assign<&ast::OrJumpExpr::label>(ref<ID>)))
      | nest<ast::CompositeLit, &ast::CompositeLit::typeExpr>(lit<"{"> +
                                                               CompositeBody);

  struct PostfixLevel : rule<PrimaryBase + many(PostfixOp)> {};

  static constexpr auto UnaryOp =
      lit<"+"> | lit<"-"> | lit<"~"> | lit<"!"> | lit<"^"> | lit<"&">;

  struct UnaryExpr
      : rule<assign<&ast::UnaryExpr::op>(UnaryOp) +
             assign<&ast::UnaryExpr::operand>(ref<CastLevel>)> {};

  struct CastExpr
      : rule<assign<&ast::CastExpr::kind>(kw<"cast">) + lit<"("> +
             assign<&ast::CastExpr::type>(ref<TypeSpec>) + lit<")"> +
             assign<&ast::CastExpr::operand>(ref<CastLevel>)> {};

  struct TransmuteExpr
      : rule<assign<&ast::CastExpr::kind>(kw<"transmute">) + lit<"("> +
             assign<&ast::CastExpr::type>(ref<TypeSpec>) + lit<")"> +
             assign<&ast::CastExpr::operand>(ref<CastLevel>)> {};

  struct AutoCastExpr
      : rule<kw<"auto_cast"> +
             assign<&ast::AutoCastExpr::operand>(ref<CastLevel>)> {};

  struct CastLevel
      : rule<ref<UnaryExpr> | ref<CastExpr> | ref<TransmuteExpr> |
             ref<AutoCastExpr> | ref<PostfixLevel>> {};

  // Odin's precedence table (odin-lang.org/docs/overview/, "Operator
  // Precedence"), tightest first; same-level operators associate left to
  // right.
  static constexpr auto MulLevelOp =
      lit<"*"> | lit<"/"> | lit<"%%"> | lit<"%"> | lit<"&~"> | lit<"&">  //
      | lit<"<<"> | lit<">>">;
  static constexpr auto AddLevelOp =
      lit<"+"> | lit<"-"> | lit<"|"> | lit<"~"> | kw<"in"> | kw<"not_in">;
  static constexpr auto RelLevelOp =
      lit<"=="> | lit<"!="> | lit<"<="> | lit<">="> | lit<"<"> | lit<">">;
  static constexpr auto AndLevelOp = lit<"&&">;
  static constexpr auto OrLevelOp = lit<"||">;

  struct BinaryLevel
      : infix<CastLevel, ast::BinaryExpr,
              levels(left(MulLevelOp), left(AddLevelOp), left(RelLevelOp),
                     left(AndLevelOp), left(OrLevelOp))> {};

  // Native parser.cpp gives ternaries and or_else precedence1. Normal
  // or_else folds left; ternary branches recursively take complete expressions.
  struct Expression
      : rule<ref<BinaryLevel> +
             many(nest<ast::OrElseExpr, &ast::OrElseExpr::value>(
                      kw<"or_else"> +
                      assign<&ast::OrElseExpr::fallback>(ref<BinaryLevel>)) |
                  nest<ast::CondExpr, &ast::CondExpr::thenValue>(
                      (kw<"if"> | flag<&ast::CondExpr::compileTime>(kw<"when">)) +
                      assign<&ast::CondExpr::cond>(ref<Expression>) +
                      kw<"else"> +
                      assign<&ast::CondExpr::elseValue>(ref<Expression>)) |
                  nest<ast::TernaryExpr, &ast::TernaryExpr::cond>(
                      lit<"?"> +
                      assign<&ast::TernaryExpr::thenValue>(ref<Expression>) +
                      lit<":"> +
                      assign<&ast::TernaryExpr::elseValue>(ref<Expression>)))> {};

  struct CallArg
      : rule<(option(assign<&ast::CallArg::name>(ref<ID>) + lit<"=">) +
              assign<&ast::CallArg::value>(ref<Expression>))  //
             | (flag<&ast::CallArg::spread>(lit<"..">) +
                assign<&ast::CallArg::value>(ref<Expression>))> {};

  struct LiteralElem
      : rule<(assign<&ast::LiteralElem::key>(RangeOrExpr) + lit<"="> +
              assign<&ast::LiteralElem::value>(ref<Expression>))  //
             | assign<&ast::LiteralElem::value>(ref<Expression>)> {};

  struct Block : rule<lit<"{"> + many(append<&ast::Block::items>(ref<Statement>)) +
                      lit<"}">> {};

  struct LabelStmt
      : rule<assign<&ast::LabelStmt::name>(ref<ID>) + lit<":"> +
             assign<&ast::LabelStmt::body>(ref<ForStmt> | ref<TypeSwitchStmt> |
                                           ref<SwitchStmt> | ref<Block>)> {};

  struct SimpleStmt
      : rule<ref<VarDecl> | ref<AssignStmt> |
             create<ast::ExprStmt>(assign<&ast::ExprStmt::value>(ref<Expression>))> {
  };

  static constexpr auto AssignOp =
      lit<"+="> | lit<"-="> | lit<"*="> | lit<"/="> | lit<"%%="> | lit<"%=">  //
      | lit<"&&="> | lit<"||="> | lit<"&~="> | lit<"&="> | lit<"|="> |
      lit<"~=">  //
      | lit<"<<="> | lit<">>="> | lit<"=">;

  struct AssignStmt
      : rule<some(append<&ast::AssignStmt::targets>(ref<Expression>), lit<",">) +
             assign<&ast::AssignStmt::op>(AssignOp) +
             some(append<&ast::AssignStmt::values>(ref<Expression>), lit<",">)> {
  };

  struct IfStmt
      : rule<kw<"if"> +
             option(assign<&ast::IfStmt::init>(ref<SimpleStmt>) + lit<";">) +
             assign<&ast::IfStmt::cond>(ref<Expression>) +
             assign<&ast::IfStmt::thenBlock>(ref<Block>) +
             option(kw<"else"> +
                    assign<&ast::IfStmt::elseBranch>(ref<IfStmt> |
                                                      ref<Block>))> {};

  // One or more names, an optional leading `&`, then `in` and a
  // range-or-expr -- the two range forms merged. This also accepts `&`
  // with three or more names or a literal range.
  struct ForStmt
      : rule<option(assign<&ast::ForStmt::directive>((lit<"#"> + kw<"reverse">) |
                                                      (lit<"#"> + kw<"unroll">))) +
             kw<"for"> +
             (assign<&ast::ForStmt::body>(ref<Block>)  // infinite
              | (option(assign<&ast::ForStmt::init>(ref<SimpleStmt>)) +
                 lit<";"> +
                 option(assign<&ast::ForStmt::cond>(ref<Expression>)) +
                 lit<";"> + option(assign<&ast::ForStmt::post>(ref<SimpleStmt>)) +
                 assign<&ast::ForStmt::body>(ref<Block>))  // C-style
              | (option(flag<&ast::ForStmt::byRef>(lit<"&">)) +
                 some(append<&ast::ForStmt::names>(ref<ID>), lit<",">) +
                 kw<"in"> +
                 assign<&ast::ForStmt::rangeValue>(RangeOrExpr) +
                 assign<&ast::ForStmt::body>(ref<Block>))  // range form
              | (assign<&ast::ForStmt::cond>(ref<Expression>) +
                 assign<&ast::ForStmt::body>(ref<Block>)))  // while-condition
            > {};

  struct SwitchCase
      : rule<kw<"case"> +
             many(
                 append<&ast::SwitchCase::values>(ref<RangeExpr> |
                                                   ref<TypeSpec> |
                                                   ref<Expression>),
                 lit<",">) +
             lit<":"> + many(append<&ast::SwitchCase::body>(ref<Statement>))> {};

  struct TypeSwitchCase
      : rule<kw<"case"> +
             many(append<&ast::SwitchCase::values>(ref<TypeSpec> |
                                                           ref<Expression>),
                         lit<",">) +
             lit<":"> + many(append<&ast::SwitchCase::body>(ref<Statement>))> {};

  struct SwitchStmt
      : rule<option(flag<&ast::SwitchStmt::partial>((lit<"#"> + kw<"partial">))) +
             kw<"switch"> +
             option(assign<&ast::SwitchStmt::init>(ref<SimpleStmt>) + lit<";">) +
             option(assign<&ast::SwitchStmt::tag>(ref<Expression>)) + lit<"{"> +
             many(append<&ast::SwitchStmt::cases>(ref<SwitchCase>)) +
             lit<"}">> {};

  // Before SwitchStmt in every choice: `name in value` also reads as a plain
  // subject, and the first arm wins.
  struct TypeSwitchStmt
      : rule<option(flag<&ast::TypeSwitchStmt::partial>((lit<"#"> + kw<"partial">))) +
             kw<"switch"> + option(assign<&ast::TypeSwitchStmt::name>(ref<ID>)) +
             kw<"in"> + assign<&ast::TypeSwitchStmt::value>(ref<Expression>) +
             lit<"{"> +
             many(append<&ast::TypeSwitchStmt::cases>(ref<TypeSwitchCase>)) +
             lit<"}">> {};

  static constexpr auto WhenBodyItem = ref<Statement> | ref<ForeignImportDecl>;

  struct WhenStmt
      : rule<kw<"when"> + assign<&ast::WhenStmt::cond>(ref<Expression>) +
             lit<"{"> + many(append<&ast::WhenStmt::body>(WhenBodyItem)) +
             lit<"}"> +
             option(kw<"else"> +
                    (assign<&ast::WhenStmt::elseWhen>(ref<WhenStmt>)  //
                     | (lit<"{"> +
                        many(append<&ast::WhenStmt::elseBody>(WhenBodyItem)) +
                        lit<"}">)))> {};

  struct ReturnStmt
      : rule<kw<"return"> +
             many(append<&ast::ReturnStmt::values>(ref<Expression>),
                         lit<",">)> {};

  struct DeferStmt
      : rule<kw<"defer"> + assign<&ast::DeferStmt::body>(ref<Statement>)> {};

  struct UsingStmt
      : rule<kw<"using"> + assign<&ast::UsingStmt::target>(ref<Expression>)> {};

  struct BreakStmt
      : rule<kw<"break"> + option(assign<&ast::BreakStmt::label>(ref<ID>))> {};

  struct ContinueStmt
      : rule<kw<"continue"> +
             option(assign<&ast::ContinueStmt::label>(ref<ID>))> {};

  struct FallthroughStmt
      : rule<create<ast::FallthroughStmt>(kw<"fallthrough">)> {};

  struct AssertStmt
      : rule<(lit<"#"> + kw<"assert">) + lit<"("> +
             assign<&ast::AssertStmt::cond>(ref<WhereCondition>) + lit<")">> {};

  struct Statement
      : rule<ref<LabelStmt>  //
             | (ref<NamedDecl> + option(lit<";">))  //
             | (ref<VarDecl> + option(lit<";">))  //
             | (ref<AssignStmt> + option(lit<";">))  //
             | ref<IfStmt>  //
             | ref<ForStmt>  //
             | ref<TypeSwitchStmt>  //
             | ref<SwitchStmt>  //
             | ref<WhenStmt>  //
             | (ref<ReturnStmt> + option(lit<";">))  //
             | (ref<DeferStmt> + option(lit<";">))  //
             | (ref<UsingStmt> + option(lit<";">))  //
             | (ref<BreakStmt> + option(lit<";">))  //
             | (ref<ContinueStmt> + option(lit<";">))  //
             | (ref<FallthroughStmt> + option(lit<";">))  //
             | (ref<AssertStmt> + option(lit<";">))  //
             | ref<ForeignBlock>  //
             | ref<Block>  //
             | (create<ast::ExprStmt>(assign<&ast::ExprStmt::value>(
                    ref<Expression>)) +
                option(lit<";">))> {};

  struct Attribute
      : rule<lit<"@"> + lit<"("> +
             some(append<&ast::Attribute::elems>(
                      create<ast::AttributeElem>(
                          assign<&ast::AttributeElem::name>(ref<ID>) +
                          option(lit<"="> + assign<&ast::AttributeElem::value>(
                                                 ref<Expression>)))),
                  lit<",">) +
             lit<")">> {};

  // A constant/proc/type binding (`id :: value`), or a typed variable
  // with an optional initialiser (`id : type : expr`). The separate
  // `id : type : proc_expr` alternative is redundant: a proc is an expr.
  struct NamedDecl
      : rule<many(append<&ast::NamedDecl::attributes>(ref<Attribute>)) +
             ((assign<&ast::NamedDecl::name>(ref<ID>) + lit<"::"> +
               (assign<&ast::NamedDecl::value>(ref<ProcGroup>)  //
                | assign<&ast::NamedDecl::typeDecl>(
                      ref<DistinctType> | ref<StructType> | ref<EnumType> |
                      ref<UnionType> | ref<BitSetType> | ref<BitFieldType> |
                      ref<MatrixType>)  //
                | assign<&ast::NamedDecl::value>(ref<Expression>)))  //
              | (assign<&ast::NamedDecl::name>(ref<ID>) + lit<":"> +
                 assign<&ast::NamedDecl::type>(ref<TypeSpec>) + lit<":"> +
                 assign<&ast::NamedDecl::value>(ref<Expression>)))> {};

  struct VarDecl
      : rule<many(append<&ast::VarDecl::attributes>(ref<Attribute>)) +
             ((some(append<&ast::VarDecl::names>(ref<ID>), lit<",">) +
               lit<":"> + assign<&ast::VarDecl::type>(ref<TypeSpec>) +
               option(lit<"="> +
                      some(append<&ast::VarDecl::values>(ref<Expression>),
                           lit<",">)))  //
              | (some(append<&ast::VarDecl::names>(ref<ID>), lit<",">) +
                 lit<":="> +
                 some(append<&ast::VarDecl::values>(ref<Expression>),
                      lit<",">)))> {};

  struct ForeignImportDecl
      : rule<kw<"foreign"> + kw<"import"> +
             option(assign<&ast::ForeignImportDecl::name>(ref<ID>)) +
             assign<&ast::ForeignImportDecl::path>(ref<STRING> |
                                                    ref<RAW_STRING>)> {};

  struct ForeignEntry
      : rule<many(append<&ast::ForeignEntry::attributes>(ref<Attribute>)) +
             assign<&ast::ForeignEntry::name>(ref<ID>) + lit<"::"> +
             kw<"proc"> +
             option(assign<&ast::ForeignEntry::callingConvention>(
                 ref<STRING>)) +
             lit<"("> +
             many(append<&ast::ForeignEntry::params>(ref<Param>),
                         lit<",">) +
             option(lit<",">) + lit<")"> +
             option(
                 lit<"->"> +
                 ((lit<"("> +
                   some(append<&ast::ForeignEntry::results>(ref<ReturnParam>),
                        lit<",">) +
                   lit<")">)  //
                  | append<&ast::ForeignEntry::results>(ref<ReturnParam>))) +
             lit<"---"> + option(lit<";">)> {};

  struct ForeignBlock
      : rule<many(append<&ast::ForeignBlock::attributes>(ref<Attribute>)) +
             kw<"foreign"> + assign<&ast::ForeignBlock::name>(ref<ID>) +
             lit<"{"> +
             many(append<&ast::ForeignBlock::entries>(ref<ForeignEntry>)) +
             lit<"}">> {};

  struct TopDecl
      : rule<ref<NamedDecl> | ref<VarDecl> | ref<ForeignImportDecl> |
             ref<ForeignBlock> | ref<WhenStmt>> {};

  struct ImportItem
      : rule<kw<"import"> + option(assign<&ast::ImportItem::alias>(ref<ID>)) +
             assign<&ast::ImportItem::path>(ref<STRING>)> {};

  // A token-free file is allowed; comments or build tags still need a package.
  struct Unit
      : rule<create<ast::Unit>(
                 (!gap::has(~cr<R"( \t\r\n)">) + eof) |
                 (kw<"package"> + assign<&ast::Unit::packageName>(ref<ID>) +
                  many(append<&ast::Unit::imports>(ref<ImportItem>)) +
                  many(append<&ast::Unit::items>(ref<TopDecl>))))> {};

  using entry = Unit;
  using ignored = list<WS, BUILD_TAG>;
  using hidden = list<LINE_COMMENT, BLOCK_COMMENT>;
};

}  // namespace odin
