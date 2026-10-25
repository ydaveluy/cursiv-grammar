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

  struct ID : terminal<cr<"a-zA-Z_"> + many(w)> {};

  static constexpr auto DEC_DIGITS = cr<"0-9"> + many(cr<"0-9_">);
  static constexpr auto HEX_DIGITS = cr<"0-9a-fA-F"> + many(cr<"0-9a-fA-F_">);
  static constexpr auto OCT_DIGITS = cr<"0-7"> + many(cr<"0-7_">);
  static constexpr auto BIN_DIGITS = cr<"01"> + many(cr<"01_">);
  static constexpr auto DEC_EXP = cr<"eE"> + option(cr<"+-">) + some(d);
  static constexpr auto HEX_EXP = cr<"pP"> + option(cr<"+-">) + some(d);

  struct INTEGER
      : terminal<(lit<"0b"> + BIN_DIGITS) | (lit<"0o"> + OCT_DIGITS) |
                 (lit<"0x"> + HEX_DIGITS) | DEC_DIGITS> {};

  // A mandatory digit on both sides of the dot: `1..5` never loses its first
  // dot to this rule, so a range needs no guard against it.
  struct FLOAT
      : terminal<(DEC_DIGITS + lit<"."> + DEC_DIGITS + option(DEC_EXP))  //
                 | (DEC_DIGITS + DEC_EXP)  //
                 | (lit<"0x"> + HEX_DIGITS + lit<"."> + option(HEX_DIGITS) +
                    option(HEX_EXP))> {};

  struct IMAG
      : terminal<(DEC_DIGITS + lit<"."> + DEC_DIGITS + option(DEC_EXP) +
                  cr<"ijk">)  //
                 | (DEC_DIGITS + DEC_EXP + cr<"ijk">)  //
                 | (DEC_DIGITS + cr<"ijk">)> {};

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
  struct RangeExprRule;
  struct BitSetType;
  struct BitFieldEntry;
  struct BitFieldType;
  struct MatrixType;
  struct SoaArrayType;
  struct SparseArrayType;
  struct TypeArgs;

  struct Expression;
  struct IdentRule;
  struct Literal;
  struct DefinedExprRule;
  struct ProcedureExprRule;
  struct LoadExprRule;
  struct AssertExprRule;
  struct Operand;
  struct PostfixLevel;
  struct CastLevel;
  struct UnaryExprRule;
  struct CastExprRule;
  struct TransmuteExprRule;
  struct AutoCastExprRule;
  struct BinaryLevel;
  struct CondTail;
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
             option(some(append<&ast::ProcType::params>(ref<Param>), lit<",">)) +
             lit<")"> +
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
      append<&ast::StructType::tags>(lit<"#align">) + lit<"("> +
      append<&ast::StructType::align>(ref<Expression>) + lit<")">;
  static constexpr auto StructMinFieldAlign =
      append<&ast::StructType::tags>(lit<"#min_field_align">) + lit<"("> +
      append<&ast::StructType::minFieldAlign>(ref<Expression>) + lit<")">;
  static constexpr auto StructTagItem =
      flag<&ast::StructType::packed>(
          append<&ast::StructType::tags>(lit<"#packed">))  //
      | flag<&ast::StructType::rawUnion>(
            append<&ast::StructType::tags>(lit<"#raw_union">))  //
      | flag<&ast::StructType::simple>(
            append<&ast::StructType::tags>(lit<"#simple">))  //
      | flag<&ast::StructType::allOrNone>(
            append<&ast::StructType::tags>(lit<"#all_or_none">))  //
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
             option(assign<&ast::EnumType::underlying>(ref<BasicType>)) +
             lit<"{"> +
             option(some(append<&ast::EnumType::fields>(ref<EnumField>),
                         lit<",">) +
                    option(lit<",">)) +
             lit<"}">> {};

  static constexpr auto UnionTagItem =
      flag<&ast::UnionType::noNil>(lit<"#no_nil">)  //
      | flag<&ast::UnionType::sharedNil>(lit<"#shared_nil">)  //
      | flag<&ast::UnionType::maybe>(lit<"#maybe">);

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
  struct RangeExprRule
      : rule<assign<&ast::RangeExpr::low>(ref<Expression>) + lit<".."> +
             (flag<&ast::RangeExpr::inclusive>(lit<"=">) | lit<"<">) +
             assign<&ast::RangeExpr::high>(ref<Expression>)> {};

  static constexpr auto RangeOrExpr = ref<RangeExprRule> | ref<Expression>;

  struct BitSetType
      : rule<kw<"bit_set"> + lit<"["> +
             assign<&ast::BitSetType::elem>(ref<RangeExprRule> |
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
      : rule<option(flag<&ast::MatrixType::rowMajor>(lit<"#row_major">)) +
             kw<"matrix"> + lit<"["> +
             assign<&ast::MatrixType::rows>(ref<Expression>) + lit<","> +
             assign<&ast::MatrixType::cols>(ref<Expression>) + lit<"]"> +
             assign<&ast::MatrixType::elem>(ref<TypeSpec>)> {};

  struct SoaArrayType
      : rule<lit<"#soa"> +
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
      : rule<lit<"#sparse"> + lit<"["> +
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
             option(some(append<&ast::ProcExpr::params>(ref<Param>), lit<",">)) +
             lit<")"> +
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

  struct IdentRule
      : rule<((assign<&ast::Ident::pkg>(ref<ID>) + lit<"."> +
               assign<&ast::Ident::name>(ref<ID>))  //
              | assign<&ast::Ident::name>(ref<ID>)) +
             option(lit<"("> + assign<&ast::Ident::typeArgs>(ref<TypeArgs>) +
                    lit<")">)> {};

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

  struct DefinedExprRule
      : rule<lit<"#defined"> + lit<"("> +
             assign<&ast::DefinedExpr::name>(ref<ID>) + lit<")">> {};
  struct ProcedureExprRule : rule<create<ast::ProcedureExpr>(lit<"#procedure">)> {
  };
  struct LoadExprRule
      : rule<lit<"#load"> + lit<"("> +
             assign<&ast::LoadExpr::path>(ref<Expression>) + lit<")">> {};
  struct AssertExprRule
      : rule<lit<"#assert"> + lit<"("> +
             assign<&ast::AssertExpr::cond>(ref<WhereCondition>) + lit<")">> {};

  static constexpr auto CompositeBody =
      option(some(append<&ast::CompositeLit::elems>(ref<LiteralElem>), lit<",">) +
             option(lit<",">)) +
      lit<"}">;

  struct Operand
      : rule<ref<Literal>  //
             | ref<IdentRule>  //
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
                   lit<"{"> + CompositeBody)> {};

  static constexpr auto PrimaryBase = ref<Operand> | ref<DefinedExprRule> |
                                      ref<ProcedureExprRule> | ref<LoadExprRule> |
                                      ref<AssertExprRule>;

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
      | nest<ast::CompositeLit, &ast::CompositeLit::typeExpr>(lit<"{"> +
                                                               CompositeBody);

  struct PostfixLevel : rule<PrimaryBase + many(PostfixOp)> {};

  static constexpr auto UnaryOp =
      lit<"+"> | lit<"-"> | lit<"~"> | lit<"!"> | lit<"^"> | lit<"&">;

  struct UnaryExprRule
      : rule<assign<&ast::UnaryExpr::op>(UnaryOp) +
             assign<&ast::UnaryExpr::operand>(ref<CastLevel>)> {};

  struct CastExprRule
      : rule<assign<&ast::CastExpr::kind>(kw<"cast">) + lit<"("> +
             assign<&ast::CastExpr::type>(ref<TypeSpec>) + lit<")"> +
             assign<&ast::CastExpr::operand>(ref<CastLevel>)> {};

  struct TransmuteExprRule
      : rule<assign<&ast::CastExpr::kind>(kw<"transmute">) + lit<"("> +
             assign<&ast::CastExpr::type>(ref<TypeSpec>) + lit<")"> +
             assign<&ast::CastExpr::operand>(ref<CastLevel>)> {};

  struct AutoCastExprRule
      : rule<kw<"auto_cast"> +
             assign<&ast::AutoCastExpr::operand>(ref<CastLevel>)> {};

  struct CastLevel
      : rule<ref<UnaryExprRule> | ref<CastExprRule> | ref<TransmuteExprRule> |
             ref<AutoCastExprRule> | ref<PostfixLevel>> {};

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
      : infix<CastLevel, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(MulLevelOp) >> left(AddLevelOp) >> left(RelLevelOp) >>
               left(AndLevelOp) >> left(OrLevelOp))> {};

  // Odin's documented table puts the ternary forms and `or_else` one
  // level below every binary operator. `if`/`when`/`?` take a
  // `BinaryLevel` condition and recurse rightward into a full
  // `Expression` on each branch, so one of these nests inside another
  // without parentheses -- `a if b else c if d else e`, `(a if b else c)
  // or_else d` without the parens.
  struct CondTail
      : rule<create<ast::CondExpr>(
                 assign<&ast::CondExpr::thenValue>(ref<BinaryLevel>) +
                 kw<"if"> + assign<&ast::CondExpr::cond>(ref<Expression>) +
                 kw<"else"> +
                 assign<&ast::CondExpr::elseValue>(ref<Expression>))  //
             | create<ast::CondExpr>(
                   assign<&ast::CondExpr::thenValue>(ref<BinaryLevel>) +
                   flag<&ast::CondExpr::compileTime>(kw<"when">) +
                   assign<&ast::CondExpr::cond>(ref<Expression>) +
                   kw<"else"> +
                   assign<&ast::CondExpr::elseValue>(ref<Expression>))  //
             | create<ast::TernaryExpr>(
                   assign<&ast::TernaryExpr::cond>(ref<BinaryLevel>) +
                   lit<"?"> +
                   assign<&ast::TernaryExpr::thenValue>(ref<Expression>) +
                   lit<":"> +
                   assign<&ast::TernaryExpr::elseValue>(ref<Expression>))  //
             | ref<BinaryLevel>> {};

  // The error-propagation tier: `or_else`'s left operand reaches down
  // through `CondTail`, so it can take a ternary/`if`/`when` form
  // un-parenthesized.
  struct Expression
      : rule<create<ast::OrElseExpr>(
                 assign<&ast::OrElseExpr::value>(ref<CondTail>) +
                 kw<"or_else"> +
                 assign<&ast::OrElseExpr::fallback>(ref<Expression>))  //
             | create<ast::OrJumpExpr>(
                   assign<&ast::OrJumpExpr::value>(ref<CondTail>) +
                   assign<&ast::OrJumpExpr::kind>(kw<"or_return">))  //
             | create<ast::OrJumpExpr>(
                   assign<&ast::OrJumpExpr::value>(ref<CondTail>) +
                   assign<&ast::OrJumpExpr::kind>(kw<"or_continue">) +
                   option(assign<&ast::OrJumpExpr::label>(ref<ID>)))  //
             | create<ast::OrJumpExpr>(
                   assign<&ast::OrJumpExpr::value>(ref<CondTail>) +
                   assign<&ast::OrJumpExpr::kind>(kw<"or_break">) +
                   option(assign<&ast::OrJumpExpr::label>(ref<ID>)))  //
             | ref<CondTail>> {};

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
      : rule<option(assign<&ast::ForStmt::directive>(lit<"#reverse"> |
                                                      lit<"#unroll">)) +
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
             option(some(
                 append<&ast::SwitchCase::values>(ref<RangeExprRule> |
                                                   ref<TypeSpec> |
                                                   ref<Expression>),
                 lit<",">)) +
             lit<":"> + many(append<&ast::SwitchCase::body>(ref<Statement>))> {};

  struct TypeSwitchCase
      : rule<kw<"case"> +
             option(some(append<&ast::SwitchCase::values>(ref<TypeSpec> |
                                                           ref<Expression>),
                         lit<",">)) +
             lit<":"> + many(append<&ast::SwitchCase::body>(ref<Statement>))> {};

  struct SwitchStmt
      : rule<option(flag<&ast::SwitchStmt::partial>(lit<"#partial">)) +
             kw<"switch"> +
             option(assign<&ast::SwitchStmt::init>(ref<SimpleStmt>) + lit<";">) +
             option(assign<&ast::SwitchStmt::tag>(ref<Expression>)) + lit<"{"> +
             many(append<&ast::SwitchStmt::cases>(ref<SwitchCase>)) +
             lit<"}">> {};

  // Before SwitchStmt in every choice: `name in value` also reads as a plain
  // subject, and the first arm wins.
  struct TypeSwitchStmt
      : rule<option(flag<&ast::TypeSwitchStmt::partial>(lit<"#partial">)) +
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
             option(some(append<&ast::ReturnStmt::values>(ref<Expression>),
                         lit<",">))> {};

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
      : rule<lit<"#assert"> + lit<"("> +
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
             option(some(append<&ast::ForeignEntry::params>(ref<Param>),
                         lit<",">)) +
             lit<")"> +
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

  struct Unit
      : rule<create<ast::Unit>(
                 option(kw<"package"> +
                        assign<&ast::Unit::packageName>(ref<ID>)) +
                 many(append<&ast::Unit::imports>(ref<ImportItem>)) +
                 many(append<&ast::Unit::items>(ref<TopDecl>)))> {};

  using entry = Unit;
  using ignore = list<WS, BUILD_TAG>;
  using hidden = list<LINE_COMMENT, BLOCK_COMMENT>;
};

}  // namespace odin
