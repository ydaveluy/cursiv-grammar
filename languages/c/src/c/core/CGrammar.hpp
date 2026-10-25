// The c language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "c/core/ast.hpp"

namespace c {

struct C : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};

  static constexpr auto LineSplice =
      lit<"\\"> + option(lit<"\r">) + lit<"\n">;
  struct SL_COMMENT
      : terminal<lit<"//"> + many(LineSplice | cr<R"(^\r\n)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  // A preprocessor line, GCC-line-marker or `#define` included: swallowed
  // whole rather than run, so the macros it names stay unexpanded text. A
  // trailing `\` splices the next physical line into the same directive.
  struct Directive
      : terminal<lit<"#"> +
                 many(LineSplice | cr<R"(^\n)">)> {};

  static constexpr auto HEXDIGIT = cr<"0-9a-fA-F">;
  static constexpr auto UCN =
      (lit<"\\u"> + repeat<4>(HEXDIGIT)) | (lit<"\\U"> + repeat<8>(HEXDIGIT));
  static constexpr auto LegacyID = cr<"\u00A8\u00AA\u00AD\u00AF\u00B2-\u00B5\u00B7-\u00BA\u00BC-\u00BE"
      "\u00C0-\u00D6\u00D8-\u00F6\u00F8-\u00FF\u0100-\u167F\u1681-\u180D"
      "\u180F-\u1FFF\u200B-\u200D\u202A-\u202E\u203F-\u2040\u2054"
      "\u2060-\u206F\u2070-\u218F\u2460-\u24FF\u2776-\u2793\u2C00-\u2DFF"
      "\u2E80-\u2FFF\u3004-\u3007\u3021-\u302F\u3031-\u303F\u3040-\uD7FF"
      "\uF900-\uFD3D\uFD40-\uFDCF\uFDF0-\uFE44\uFE47-\uFFFD"
      "\U00010000-\U0001FFFD\U00020000-\U0002FFFD\U00030000-\U0003FFFD"
      "\U00040000-\U0004FFFD\U00050000-\U0005FFFD\U00060000-\U0006FFFD"
      "\U00070000-\U0007FFFD\U00080000-\U0008FFFD\U00090000-\U0009FFFD"
      "\U000A0000-\U000AFFFD\U000B0000-\U000BFFFD\U000C0000-\U000CFFFD"
      "\U000D0000-\U000DFFFD\U000E0000-\U000EFFFD">;
  struct ID
      : terminal<(uc::XID_Start | cr<"_"> | LegacyID | UCN) +
                 many(uc::XID_Continue | LegacyID | UCN)> {};

  static constexpr auto ESCAPE =
      lit<"\\"> + (cr<R"(abfnrtv\\"'?)">                 //
                   | (lit<"x"> + some(HEXDIGIT))       //
                   | (lit<"u"> + repeat<4>(HEXDIGIT))  //
                   | (lit<"U"> + repeat<8>(HEXDIGIT))  //
                   | (cr<"0-7"> + option(cr<"0-7">) + option(cr<"0-7">))
                   | (option(lit<"\r">) + lit<"\n">));

  static constexpr auto IntegerLength =
      lit<"ll"> | lit<"LL"> | cr<"lL"> | lit<"wb"> | lit<"WB">;
  static constexpr auto INTEGERSUFFIX =
      (cr<"uU"> + option(IntegerLength)) |
      (IntegerLength + option(cr<"uU">));
  static constexpr auto DecimalDigits = some(d) + many(lit<"'"> + some(d));
  static constexpr auto HexDigits = some(HEXDIGIT) + many(lit<"'"> + some(HEXDIGIT));
  static constexpr auto BinaryDigits = some(cr<"01">) + many(lit<"'"> + some(cr<"01">));
  static constexpr auto OctalDigits = many(cr<"0-7"> | (lit<"'"> + cr<"0-7">));
  struct INT_LIT
      : terminal<((lit<"0"> + cr<"xX"> + HexDigits) |
                  (lit<"0"> + cr<"bB"> + BinaryDigits) |
                  (cr<"1-9"> + many(d | (lit<"'"> + d))) | (lit<"0"> + OctalDigits)) +
                 option(INTEGERSUFFIX)> {};
  static constexpr auto EXPONENT = cr<"eE"> + option(cr<"+-">) + DecimalDigits;
  static constexpr auto HexExponent = cr<"pP"> + option(cr<"+-">) + DecimalDigits;
  struct FLOAT_LIT : terminal<((((DecimalDigits + lit<"."> + option(DecimalDigits)) |
                                 (lit<"."> + DecimalDigits)) +
                                option(EXPONENT)) |
                               (DecimalDigits + EXPONENT) |
                               (lit<"0"> + cr<"xX"> +
                                ((HexDigits + option(lit<"."> + option(HexDigits))) |
                                 (lit<"."> + HexDigits)) +
                                HexExponent)) +
                              option(lit<"df"> | lit<"DF"> | lit<"dd"> |
                                     lit<"DD"> | lit<"dl"> | lit<"DL"> |
                                     cr<"flFL">)> {};

  struct CHAR_LIT : terminal<(option(lit<"u8"> | cr<"LuU">) + lit<"'"> +
                              some(ESCAPE | (!cr<R"('\n)"> + any)) + lit<"'">)> {};
  struct STRING_LIT
      : terminal<(option(lit<"u8"> | cr<"LuU">) + lit<"\""> +
                  many(ESCAPE | (!cr<R"("\n)"> + any)) + lit<"\"">)> {};

  static constexpr auto BuiltinTypeWord =
      kw<"void"> | kw<"char"> | kw<"short"> | kw<"int"> | kw<"long"> |
      kw<"float"> | kw<"double"> | kw<"signed"> | kw<"unsigned"> |
      kw<"_Bool"> | kw<"bool"> | kw<"_Complex"> | kw<"_Imaginary"> |
      kw<"_Decimal32"> | kw<"_Decimal64"> | kw<"_Decimal128"> |
      kw<"__int128"> | kw<"__m128"> | kw<"__m128d"> | kw<"__m128i">;

  static constexpr auto StorageClass =
      kw<"typedef"> | kw<"extern"> | kw<"static"> | kw<"auto"> |
      kw<"register"> | kw<"constexpr"> | kw<"_Thread_local"> |
      kw<"thread_local">;
  static constexpr auto TypeQualifier =
      kw<"const"> | kw<"volatile"> | kw<"__volatile__"> | kw<"restrict"> |
      kw<"__restrict__"> | kw<"__restrict"> | kw<"_Atomic">;
  static constexpr auto FunctionSpecifier =
      kw<"inline"> | kw<"__inline__"> | kw<"__inline"> | kw<"_Noreturn">;
  static constexpr auto Qualifier =
      StorageClass | TypeQualifier | FunctionSpecifier;

  static constexpr auto AlignofWord =
      kw<"alignof"> | kw<"_Alignof"> | kw<"__alignof__"> | kw<"__alignof">;
  static constexpr auto TypeofWord =
      kw<"typeof"> | kw<"__typeof__"> | kw<"__typeof">;
  static constexpr auto AsmKw = kw<"asm"> | kw<"__asm"> | kw<"__asm__">;
  static constexpr auto CountofWord = kw<"_Countof">;
  // Visual C calling conventions, an optional prefix on a declarator's core.
  static constexpr auto VcCallConv =
      kw<"__cdecl"> | kw<"__clrcall"> | kw<"__stdcall"> | kw<"__fastcall"> |
      kw<"__thiscall"> | kw<"__vectorcall">;
  static constexpr auto AttributeWord = kw<"__attribute__"> | kw<"__attribute">;

  static constexpr auto ReservedWord =
      Qualifier | BuiltinTypeWord | kw<"break"> | kw<"case"> | kw<"continue"> |
      kw<"default"> | kw<"do"> | kw<"else"> | kw<"enum"> | kw<"for"> |
      kw<"goto"> | kw<"if"> | kw<"return"> | kw<"sizeof"> | kw<"struct"> |
      kw<"switch"> | kw<"union"> | kw<"while"> | kw<"true"> | kw<"false"> |
      kw<"nullptr"> | AlignofWord | kw<"_Generic"> | AsmKw |
      kw<"alignas"> | kw<"_Alignas"> |
      kw<"__extension__"> | kw<"__func__"> | kw<"__FUNCTION__"> |
      kw<"__PRETTY_FUNCTION__"> | kw<"_Static_assert"> |
      kw<"static_assert"> | TypeofWord | kw<"typeof_unqual"> |
      kw<"__typeof_unqual__"> | CountofWord | kw<"_Maxof"> | kw<"_Minof"> |
      kw<"__real__"> | kw<"__imag__"> | VcCallConv | kw<"__declspec"> |
      AttributeWord | kw<"__label__"> | kw<"_BitInt"> |
      kw<"__builtin_va_arg"> | kw<"__builtin_offsetof"> |
      kw<"__builtin_choose_expr"> | kw<"__builtin_types_compatible_p"> |
      kw<"__builtin_tgmath"> | kw<"__builtin_complex">;

  static constexpr auto OpMul = lit<"*"> | lit<"/"> | lit<"%">;
  static constexpr auto OpAdd = lit<"+"> | lit<"-">;
  static constexpr auto OpShift = lit<"<<"> | lit<">>">;
  static constexpr auto OpRel = lit<"<="> | lit<">="> | lit<"<"> | lit<">">;
  static constexpr auto OpEq = lit<"=="> | lit<"!=">;
  static constexpr auto OpBitAnd = lit<"&">;
  static constexpr auto OpBitXor = lit<"^">;
  static constexpr auto OpBitOr = lit<"|">;
  static constexpr auto OpLogAnd = lit<"&&">;
  static constexpr auto OpLogOr = lit<"||">;

  static constexpr auto OpAssign =
      lit<"<<="> | lit<">>="> | lit<"+="> | lit<"-="> | lit<"*="> | lit<"/="> |
      lit<"%="> | lit<"&="> | lit<"^="> | lit<"|="> | lit<"=", !cr<"=">>;

  static constexpr auto OpUnary = lit<"&"> | lit<"*"> | lit<"+"> | lit<"-"> |
                                  lit<"~"> | lit<"!"> | kw<"__extension__"> |
                                  kw<"__real__"> | kw<"__imag__">;

  struct Declaration;
  struct MemberDeclaration;
  struct Declarator;
  struct AbstractDeclarator;
  struct DeclarationSpecifiers;
  struct TypeSpecifier;
  struct StructOrUnionSpecifier;
  struct EnumSpecifier;
  struct Enumerator;
  struct TypeName;
  struct Expression;
  struct AssignmentExpression;
  struct ConditionalExpression;
  struct BinaryExpression;
  struct CastExpression;
  struct UnaryExpression;
  struct PostfixExpression;
  struct PrimaryExpression;
  struct Statement;
  struct CompoundStatement;
  struct BlockItem;
  struct AmbiguousStmt;
  struct Initializer;
  struct BracedInit;
  struct InitializerItem;
  struct Designator;
  struct DeclaratorSuffix;
  struct PointerPart;
  struct ParameterDeclaration;
  struct StringLiteral;
  struct BalancedParens;
  struct Label;

  // A standard `[[...]]` or GNU `__attribute__((...))`/`__attribute((...))`
  // attribute: matched and discarded rather than interpreted, since its
  // content is an open-ended, extension-specific grammar of its own.
  // `BalancedParens` balances the parens recursively so a nested one, e.g.
  // `((format(printf, 1, 2)))`, still matches as a whole.
  static constexpr auto AttrAtom =
      ref<ID> | ref<INT_LIT> | ref<FLOAT_LIT> | ref<STRING_LIT> |
      ref<CHAR_LIT> | lit<","> | lit<"::"> | lit<"..."> | lit<"*"> |
      lit<"&"> | lit<"+"> | lit<"-"> | lit<"="> | lit<"<"> | lit<">"> |
      lit<"!"> | lit<"~"> | lit<"?"> | lit<":"> | lit<".">;
  struct BalancedParens
      : rule<lit<"("> + many(ref<BalancedParens> | AttrAtom) + lit<")">> {};
  struct GnuAttribute : rule<AttributeWord + ref<BalancedParens>> {};
  static constexpr auto AttrName = ref<ID> + option(lit<"::"> + ref<ID>);
  struct StdAttribute : rule<AttrName + option(ref<BalancedParens>)> {};
  struct StdAttributeSpec
      : rule<lit<"["> + lit<"["> + option(ref<StdAttribute>) +
             many(lit<","> + option(ref<StdAttribute>)) + lit<"]"> + lit<"]">> {};
  struct AttributeSpec : rule<ref<GnuAttribute> | ref<StdAttributeSpec>> {};

  // `__declspec(...)`: like an attribute, matched and discarded.
  struct DeclSpec : rule<kw<"__declspec"> + ref<BalancedParens>> {};

  static constexpr auto BuiltinKeyword =
      append<&ast::BuiltinType::keywords>(BuiltinTypeWord);
  struct BuiltinTypeSpecifier
      : rule<(many(BuiltinKeyword) +
              append<&ast::BuiltinType::keywords>(kw<"_BitInt">) + lit<"("> +
              assign<&ast::BuiltinType::bitWidth>(ref<ConditionalExpression>) +
              lit<")"> + many(BuiltinKeyword)) |
             some(BuiltinKeyword)> {};

  // A bare identifier in type position: with no symbol table any name may be
  // one. `AmbiguousShape` keeps the two shapes it cannot settle out of here.
  struct TypedefName : rule<assign<&ast::NamedType::name>(ref<ID>)> {};

  // `_Atomic(T)`. Tried as part of `TypeSpecifier`; the bare qualifier form
  // is plain text in `Qualifier`, matched before a `TypeSpecifier` is due.
  struct AtomicSpecifier
      : rule<create<ast::AtomicType>(
                 kw<"_Atomic"> + lit<"("> +
                 assign<&ast::AtomicType::type>(ref<TypeName>) + lit<")">)> {
  };

  // `typeof(e)`/`typeof(T)`: ordered choice tries the type reading first,
  // same disambiguation `CastExpression` uses for `(a)`.
  struct TypeofSpecifier
      : rule<create<ast::TypeofType>(
                 (flag<&ast::TypeofType::unqual>(
                      kw<"typeof_unqual"> | kw<"__typeof_unqual__">) |
                  TypeofWord) +
                 lit<"("> +
                 (assign<&ast::TypeofType::type>(ref<TypeName>)  //
                  | assign<&ast::TypeofType::expr>(ref<Expression>)) +
                 lit<")">)> {};

  struct TypeSpecifier : rule<ref<BuiltinTypeSpecifier>      //
                              | ref<StructOrUnionSpecifier>  //
                              | ref<EnumSpecifier>           //
                              | ref<AtomicSpecifier>         //
                              | ref<TypeofSpecifier>         //
                              | ref<TypedefName>> {};

  // `alignas(T)`/`alignas(expr)`, `_Alignas(...)`: ordered choice tries the
  // type reading first, same as everywhere else this grammar carries the
  // cast-shaped ambiguity.
  struct AlignmentSpecifier
      : rule<create<ast::AlignSpec>(
                 (kw<"alignas"> | kw<"_Alignas">) + lit<"("> +
                 (assign<&ast::AlignSpec::type>(ref<TypeName>)  //
                  | assign<&ast::AlignSpec::expr>(
                        ref<ConditionalExpression>)) +
                 lit<")">)> {};

  static constexpr auto DeclSpecifierItem =
      append<&ast::Specifiers::qualifiers>(Qualifier) |
      append<&ast::Specifiers::aligns>(ref<AlignmentSpecifier>) |
      ref<AttributeSpec> | ref<DeclSpec>;

  struct DeclarationSpecifiers
      : rule<many(DeclSpecifierItem) +
             append<&ast::Specifiers::types>(ref<TypeSpecifier>) +
             many(DeclSpecifierItem +
                  option(append<&ast::Specifiers::types>(
                      ref<BuiltinTypeSpecifier>)))> {};

  struct StructOrUnionSpecifier
      : rule<assign<&ast::TagType::keyword>(kw<"struct"> | kw<"union">) +
             many(ref<AttributeSpec>) +
             ((option(assign<&ast::TagType::name>(ref<ID>)) +
               flag<&ast::TagType::defined>(lit<"{">) +
               many(append<&ast::TagType::members>(ref<MemberDeclaration>)) +
               lit<"}">)  //
              | assign<&ast::TagType::name>(ref<ID>))> {};

  struct EnumSpecifier
      : rule<kw<"enum"> + many(ref<AttributeSpec>) +
             ((option(assign<&ast::EnumType::name>(ref<ID>)) +
               option(option(flag<&ast::EnumType::hasUnderlyingColon>(lit<":">)) +
                      assign<&ast::EnumType::underlying>(ref<DeclarationSpecifiers>)) +
               flag<&ast::EnumType::defined>(lit<"{">) +
               some(append<&ast::EnumType::enumerators>(ref<Enumerator>),
                    lit<",">) +
               option(lit<",">) + lit<"}">)  //
              |
              (assign<&ast::EnumType::name>(ref<ID>) +
               option(
                   flag<&ast::EnumType::hasUnderlyingColon>(lit<":">) +
                   assign<&ast::EnumType::underlying>(ref<DeclarationSpecifiers>))))> {
  };

  struct Enumerator : rule<assign<&ast::Enumerator::name>(ref<ID>) +
                           many(ref<AttributeSpec>) +
                           option(lit<"="> + assign<&ast::Enumerator::value>(
                                                 ref<ConditionalExpression>))> {
  };

  struct PointerPart
      : rule<(lit<"*"> | flag<&ast::Pointer::isBlock>(lit<"^">)) +
             many(append<&ast::Pointer::qualifiers>(TypeQualifier))> {};

  struct ArraySuffix
      : rule<lit<"["> +
             many(append<&ast::ArraySuffix::qualifiers>(TypeQualifier) |
                  flag<&ast::ArraySuffix::isStatic>(kw<"static">)) +
             option(assign<&ast::ArraySuffix::size>(ref<AssignmentExpression>) |
                    lit<"*">) +
             lit<"]">> {};

  struct FunctionSuffix
      : rule<lit<"("> +
             option((some(append<&ast::FunctionSuffix::params>(
                              ref<ParameterDeclaration>),
                          lit<",">) +
                     option(lit<","> +
                            flag<&ast::FunctionSuffix::variadic>(
                                lit<"...">)))  //
                    | flag<&ast::FunctionSuffix::variadic>(lit<"...">)) +
             lit<")">> {};

  struct DeclaratorSuffix : rule<ref<ArraySuffix> | ref<FunctionSuffix>> {};

  struct Declarator
      : rule<many(append<&ast::Declarator::pointers>(ref<PointerPart>)) +
             option(assign<&ast::Declarator::callingConvention>(VcCallConv)) +
             (assign<&ast::Declarator::name>(ref<ID>)  //
              | (lit<"("> + assign<&ast::Declarator::nested>(ref<Declarator>) +
                 lit<")">)) +
             many(append<&ast::Declarator::suffixes>(ref<DeclaratorSuffix>)) +
             many(ref<AttributeSpec>)> {};

  static constexpr auto AbstractTail =
      (lit<"("> + assign<&ast::Declarator::nested>(ref<AbstractDeclarator>) +
       lit<")"> +
       many(append<&ast::Declarator::suffixes>(ref<DeclaratorSuffix>)))  //
      | some(append<&ast::Declarator::suffixes>(ref<DeclaratorSuffix>));

  struct AbstractDeclarator
      : rule<option(assign<&ast::Declarator::callingConvention>(VcCallConv)) +
             ((some(append<&ast::Declarator::pointers>(ref<PointerPart>)) +
               option(AbstractTail))  //
              | AbstractTail)> {};

  struct TypeName
      : rule<assign<&ast::TypeName::specifiers>(ref<DeclarationSpecifiers>) +
             option(
                 assign<&ast::TypeName::declarator>(ref<AbstractDeclarator>))> {
  };

  struct ParameterDeclaration
      : rule<assign<&ast::TypeName::specifiers>(ref<DeclarationSpecifiers>) +
             option(assign<&ast::TypeName::declarator>(
                 ref<Declarator> | ref<AbstractDeclarator>))> {};

  struct Identifier : rule<assign<&ast::Ident::name>(ref<ID>)> {};
  struct IntegerLiteral : rule<assign<&ast::IntLit::text>(ref<INT_LIT>)> {};
  struct FloatingLiteral : rule<assign<&ast::FloatLit::text>(ref<FLOAT_LIT>)> {
  };
  struct CharacterLiteral : rule<assign<&ast::CharLit::text>(ref<CHAR_LIT>)> {};
  struct StringLiteral
      : rule<some(append<&ast::StringLit::pieces>(ref<STRING_LIT>))> {};
  struct BoolLiteral
      : rule<create<ast::BoolLit>(
                 flag<&ast::BoolLit::value>(kw<"true">) | kw<"false">)> {
  };
  struct NullptrLiteral : rule<create<ast::NullptrLit>(kw<"nullptr">)> {};
  struct PredefinedLiteral
      : rule<create<ast::PredefinedExpr>(assign<&ast::PredefinedExpr::name>(
                 kw<"__func__"> | kw<"__FUNCTION__"> |
                 kw<"__PRETTY_FUNCTION__">))> {};

  struct GenericAssociation
      : rule<(assign<&ast::GenericAssoc::type>(ref<TypeName>) |
              kw<"default">) +
             lit<":"> +
             assign<&ast::GenericAssoc::value>(ref<AssignmentExpression>)> {};

  struct GenericSelection
      : rule<create<ast::GenericExpr>(
                 kw<"_Generic"> + lit<"("> +
                 assign<&ast::GenericExpr::control>(
                     ref<AssignmentExpression>) +
                 lit<","> +
                 some(append<&ast::GenericExpr::assocs>(
                          ref<GenericAssociation>),
                      lit<",">) +
                 lit<")">)> {};

  // The GNU `__builtin_*` pseudo-functions: kept generic (ast::BuiltinExpr)
  // since each mixes type and expression arguments differently.
  struct BuiltinCall
      : rule<create<ast::BuiltinExpr>(
                 assign<&ast::BuiltinExpr::name>(kw<"__builtin_va_arg">) +
                 lit<"("> + append<&ast::BuiltinExpr::args>(ref<UnaryExpression>) +
                 lit<","> + append<&ast::BuiltinExpr::types>(ref<TypeName>) +
                 lit<")">)  //
             | create<ast::BuiltinExpr>(
                   assign<&ast::BuiltinExpr::name>(kw<"__builtin_offsetof">) +
                   lit<"("> +
                   append<&ast::BuiltinExpr::types>(ref<TypeName>) +
                   lit<","> +
                   append<&ast::BuiltinExpr::args>(ref<UnaryExpression>) +
                   lit<")">)  //
             | create<ast::BuiltinExpr>(
                   assign<&ast::BuiltinExpr::name>(
                       kw<"__builtin_choose_expr">) +
                   lit<"("> +
                   append<&ast::BuiltinExpr::args>(ref<UnaryExpression>) +
                   lit<","> +
                   append<&ast::BuiltinExpr::args>(ref<UnaryExpression>) +
                   lit<","> +
                   append<&ast::BuiltinExpr::args>(ref<UnaryExpression>) +
                   lit<")">)  //
             | create<ast::BuiltinExpr>(
                   assign<&ast::BuiltinExpr::name>(
                       kw<"__builtin_types_compatible_p">) +
                   lit<"("> + append<&ast::BuiltinExpr::types>(ref<TypeName>) +
                   lit<","> + append<&ast::BuiltinExpr::types>(ref<TypeName>) +
                   lit<")">)  //
             | create<ast::BuiltinExpr>(
                   assign<&ast::BuiltinExpr::name>(kw<"__builtin_tgmath">) +
                   lit<"("> +
                   some(append<&ast::BuiltinExpr::args>(
                            ref<AssignmentExpression>),
                        lit<",">) +
                   lit<")">)  //
             | create<ast::BuiltinExpr>(
                   assign<&ast::BuiltinExpr::name>(kw<"__builtin_complex">) +
                   lit<"("> +
                   append<&ast::BuiltinExpr::args>(ref<AssignmentExpression>) +
                   lit<","> +
                   append<&ast::BuiltinExpr::args>(ref<AssignmentExpression>) +
                   lit<")">)> {};

  struct PrimaryExpression : rule<create<ast::StmtExpr>(
                                      option(kw<"__extension__">) + lit<"("> +
                                      assign<&ast::StmtExpr::body>(
                                          ref<CompoundStatement>) +
                                      lit<")">)                            //
                                  | (lit<"("> + ref<Expression> + lit<")">)  //
                                  | ref<GenericSelection>                  //
                                  | ref<BuiltinCall>                       //
                                  | ref<FloatingLiteral>                   //
                                  | ref<IntegerLiteral>                    //
                                  | ref<CharacterLiteral>                  //
                                  | ref<StringLiteral>                     //
                                  | ref<BoolLiteral>                       //
                                  | ref<NullptrLiteral>                    //
                                  | ref<PredefinedLiteral>                 //
                                  | ref<Identifier>> {};

  // `ref<BracedInit>`, not `ref<Initializer>`: the bare-expression form
  // would make `(I)1` a compound literal too, forking every cast on `+`.
  struct CompoundLiteral
      : rule<create<ast::CompoundLiteral>(
                 option(kw<"__extension__">) + lit<"("> +
                 assign<&ast::CompoundLiteral::type>(ref<TypeName>) +
                 lit<")"> +
                 assign<&ast::CompoundLiteral::init>(ref<BracedInit>))> {};

  struct PostfixExpression
      : rule<(ref<PrimaryExpression> | ref<CompoundLiteral>) +
             many(nest<ast::IndexExpr, &ast::IndexExpr::array>(
                      lit<"["> +
                      assign<&ast::IndexExpr::index>(ref<Expression>) +
                      lit<"]">)  //
                  | nest<ast::CallExpr, &ast::CallExpr::callee>(
                        lit<"("> +
                        many(append<&ast::CallExpr::args>(
                                 ref<AssignmentExpression>),
                             lit<",">) +
                        lit<")">)  //
                  | nest<ast::MemberExpr, &ast::MemberExpr::object>(
                        flag<&ast::MemberExpr::arrow>(lit<"->">) +
                        assign<&ast::MemberExpr::field>(ref<ID>))  //
                  | nest<ast::MemberExpr, &ast::MemberExpr::object>(
                        lit<"."> + assign<&ast::MemberExpr::field>(ref<ID>))  //
                  |
                  nest<ast::PostfixExpr, &ast::PostfixExpr::operand>(
                      assign<&ast::PostfixExpr::op>(lit<"++"> | lit<"--">)))> {
  };

  struct UnaryExpression
      : rule<create<ast::PrefixExpr>(
                 assign<&ast::PrefixExpr::op>(lit<"++"> | lit<"--">) +
                 assign<&ast::PrefixExpr::operand>(ref<UnaryExpression>))  //
             | create<ast::PrefixExpr>(
                   assign<&ast::PrefixExpr::op>(OpUnary) +
                   assign<&ast::PrefixExpr::operand>(ref<CastExpression>)) |
             create<ast::SizeofTypeExpr>(
                 kw<"sizeof"> + lit<"("> +
                 assign<&ast::SizeofTypeExpr::type>(ref<TypeName>) + lit<")">) |
             create<ast::SizeofExpr>(
                 kw<"sizeof"> +
                 assign<&ast::SizeofExpr::operand>(ref<UnaryExpression>)) |
             create<ast::AlignofTypeExpr>(
                 AlignofWord + lit<"("> +
                 assign<&ast::AlignofTypeExpr::type>(ref<TypeName>) +
                 lit<")">) |
             create<ast::AlignofExpr>(
                 AlignofWord +
                 assign<&ast::AlignofExpr::operand>(ref<UnaryExpression>)) |
             create<ast::CountofTypeExpr>(
                 CountofWord + lit<"("> +
                 assign<&ast::CountofTypeExpr::type>(ref<TypeName>) +
                 lit<")">) |
             create<ast::CountofExpr>(
                 CountofWord +
                 assign<&ast::CountofExpr::operand>(ref<UnaryExpression>)) |
             create<ast::MinMaxOfExpr>(
                 assign<&ast::MinMaxOfExpr::op>(kw<"_Maxof"> | kw<"_Minof">) +
                 lit<"("> + assign<&ast::MinMaxOfExpr::type>(ref<TypeName>) +
                 lit<")">) |
             create<ast::AddrOfLabelExpr>(
                 lit<"&&"> +
                 assign<&ast::AddrOfLabelExpr::label>(ref<ID>)) |
             ref<PostfixExpression>> {};

  struct CastExpression
      : rule<create<ast::CastExpr>(
                 lit<"("> + assign<&ast::CastExpr::type>(ref<TypeName>) +
                 lit<")"> +
                 assign<&ast::CastExpr::operand>(ref<CastExpression>))  //
             | ref<UnaryExpression>> {};

  struct BinaryExpression : infix<CastExpression, &ast::BinaryExpr::left,
                                  &ast::BinaryExpr::op, &ast::BinaryExpr::right,
                                  (left(OpMul) >>     //
                                   left(OpAdd) >>     //
                                   left(OpShift) >>   //
                                   left(OpRel) >>     //
                                   left(OpEq) >>      //
                                   left(OpBitAnd) >>  //
                                   left(OpBitXor) >>  //
                                   left(OpBitOr) >>   //
                                   left(OpLogAnd) >>  //
                                   left(OpLogOr))> {};

  struct ConditionalExpression
      : rule<create<ast::CondExpr>(
                 assign<&ast::CondExpr::cond>(ref<BinaryExpression>) +
                 lit<"?"> + assign<&ast::CondExpr::thenValue>(ref<Expression>) +
                 lit<":"> +
                 assign<&ast::CondExpr::elseValue>(
                     ref<ConditionalExpression>))  //
             | ref<BinaryExpression>> {};

  struct AssignmentExpression
      : rule<create<ast::AssignExpr>(
                 assign<&ast::AssignExpr::target>(ref<UnaryExpression>) +
                 assign<&ast::AssignExpr::op>(OpAssign) +
                 assign<&ast::AssignExpr::value>(ref<AssignmentExpression>))  //
             | ref<ConditionalExpression>> {};

  struct Expression
      : infix<AssignmentExpression, &ast::BinaryExpr::left,
              &ast::BinaryExpr::op, &ast::BinaryExpr::right, left(lit<",">)> {};

  struct Designator
      : rule<create<ast::FieldDesignator>(
                 lit<"."> + assign<&ast::FieldDesignator::name>(ref<ID>))  //
             |
             create<ast::IndexDesignator>(
                 lit<"["> +
                 assign<&ast::IndexDesignator::index>(
                     ref<ConditionalExpression>) +
                 option(lit<"..."> + assign<&ast::IndexDesignator::rangeEnd>(
                                          ref<ConditionalExpression>)) +
                 lit<"]">)> {};

  struct BracedInit
      : rule<create<ast::ListInit>(
                 lit<"{"> +
                 many(append<&ast::ListInit::items>(ref<InitializerItem>),
                      lit<",">) +
                 option(lit<",">) + lit<"}">)> {};

  struct Initializer
      : rule<ref<BracedInit>  //
             | create<ast::ExprInit>(
                   assign<&ast::ExprInit::value>(ref<AssignmentExpression>))> {
  };

  struct InitializerItem
      : rule<create<ast::DesignatedInit>(
                 some(append<&ast::DesignatedInit::designators>(
                     ref<Designator>)) +
                 lit<"="> +
                 assign<&ast::DesignatedInit::value>(ref<Initializer>))  //
             | ref<Initializer>> {};

  struct InitDeclarator
      : rule<assign<&ast::InitDeclarator::declarator>(ref<Declarator>) +
             option(lit<"="> +
                    assign<&ast::InitDeclarator::init>(ref<Initializer>))> {};

  struct MemberDeclarator
      : rule<(assign<&ast::InitDeclarator::declarator>(ref<Declarator>) +
              option(lit<":"> + assign<&ast::InitDeclarator::bitWidth>(
                                    ref<ConditionalExpression>)))  //
             | (lit<":"> + assign<&ast::InitDeclarator::bitWidth>(
                               ref<ConditionalExpression>))> {};

  struct StaticAssertDecl
      : rule<create<ast::StaticAssertDecl>(
                 (kw<"_Static_assert"> | kw<"static_assert">) + lit<"("> +
                 assign<&ast::StaticAssertDecl::cond>(
                     ref<ConditionalExpression>) +
                 option(lit<","> + assign<&ast::StaticAssertDecl::message>(
                                       ref<STRING_LIT>)) +
                 lit<")"> + lit<";">)> {};

  struct MemberDeclaration
      : rule<ref<StaticAssertDecl>  //
             | (create<ast::Declaration>(
                   many(ref<AttributeSpec>) +
                   assign<&ast::Declaration::specifiers>(
                       ref<DeclarationSpecifiers>) +
                   many(append<&ast::Declaration::declarators>(
                            ref<MemberDeclarator>),
                        lit<",">) +
                   lit<";">))> {};

  struct Declaration
      : rule<assign<&ast::Declaration::specifiers>(ref<DeclarationSpecifiers>) +
             many(append<&ast::Declaration::declarators>(ref<InitDeclarator>),
                  lit<",">) +
             lit<";">> {};

  // A K&R declaration declares a parameter: `T;` is none, so a run of
  // declarations is never read as the list of an implicit-int `T`.
  struct ParamTypeDeclaration
      : rule<assign<&ast::Declaration::specifiers>(ref<DeclarationSpecifiers>) +
             some(append<&ast::Declaration::declarators>(ref<InitDeclarator>),
                  lit<",">) +
             lit<";">> {};

  struct ExpressionStatement
      : rule<option(assign<&ast::ExprStmt::value>(ref<Expression>)) +
             lit<";">> {};

  struct CompoundStatement
      : rule<lit<"{"> +
             many(append<&ast::CompoundStmt::items>(ref<BlockItem>)) +
             lit<"}">> {};

  struct IfStatement
      : rule<kw<"if"> + lit<"("> + assign<&ast::IfStmt::cond>(ref<Expression>) +
             lit<")"> + assign<&ast::IfStmt::thenBranch>(ref<Statement>) +
             option(commit(kw<"else">, assign<&ast::IfStmt::elseBranch>(ref<Statement>)))> {};

  struct SwitchStatement
      : rule<kw<"switch"> + lit<"("> +
             assign<&ast::SwitchStmt::value>(ref<Expression>) + lit<")"> +
             assign<&ast::SwitchStmt::body>(ref<Statement>)> {};

  struct WhileStatement
      : rule<kw<"while"> + lit<"("> +
             assign<&ast::WhileStmt::cond>(ref<Expression>) + lit<")"> +
             assign<&ast::WhileStmt::body>(ref<Statement>)> {};

  struct DoStatement
      : rule<kw<"do"> + assign<&ast::DoStmt::body>(ref<Statement>) +
             kw<"while"> + lit<"("> +
             assign<&ast::DoStmt::cond>(ref<Expression>) + lit<")"> +
             lit<";">> {};

  struct ForStatement
      : rule<kw<"for"> + lit<"("> +
             assign<&ast::ForStmt::init>(ref<AmbiguousStmt> |
                                         ref<Declaration> |
                                         ref<ExpressionStatement>) +
             option(assign<&ast::ForStmt::cond>(ref<Expression>)) + lit<";"> +
             option(assign<&ast::ForStmt::step>(ref<Expression>)) + lit<")"> +
             assign<&ast::ForStmt::body>(ref<Statement>)> {};

  struct CaseStatement
      : rule<kw<"case"> +
             assign<&ast::CaseLabel::value>(ref<ConditionalExpression>) +
             option(lit<"..."> + assign<&ast::CaseLabel::rangeEnd>(
                                      ref<ConditionalExpression>)) +
             lit<":"> + assign<&ast::CaseLabel::body>(ref<Statement>)> {};

  struct DefaultStatement
      : rule<kw<"default"> + lit<":"> +
             assign<&ast::DefaultLabel::body>(ref<Statement>)> {};

  // A label labels the statement after it: it stands alone only where none
  // can follow, a block's end or a declaration a keyword opens (C23). Optional
  // anywhere, a run of labels reads every way they can nest. `case` and
  // `default` always take one.
  struct LabeledStatement
      : rule<assign<&ast::Label::name>(ref<ID>) + lit<":"> +
             (assign<&ast::Label::body>(ref<Statement>) |
              &(lit<"}"> | BuiltinTypeWord | Qualifier | kw<"struct"> |
                kw<"union"> | kw<"enum"> | TypeofWord |
                kw<"_Static_assert"> | kw<"static_assert">))> {};

  // GNU `__label__ name;`: declares a block-scoped label name.
  struct LocalLabelDeclaration
      : rule<create<ast::LocalLabelDecl>(
                 kw<"__label__"> +
                 assign<&ast::LocalLabelDecl::name>(ref<ID>) + lit<";">)> {};

  struct JumpStatement
      : rule<create<ast::GotoStmt>(kw<"goto"> +
                                   assign<&ast::GotoStmt::target>(ref<ID>) +
                                   lit<";">)  //
             | create<ast::ComputedGotoStmt>(
                   kw<"goto"> +
                   assign<&ast::ComputedGotoStmt::target>(
                       ref<UnaryExpression>) +
                   lit<";">)                                         //
             | create<ast::ContinueStmt>(kw<"continue"> + lit<";">)  //
             | create<ast::BreakStmt>(kw<"break"> + lit<";">)        //
             | create<ast::ReturnStmt>(
                   kw<"return"> +
                   option(assign<&ast::ReturnStmt::value>(ref<Expression>)) +
                   lit<";">)> {};

  struct AsmOperandSpec
      : rule<option(lit<"["> +
                    assign<&ast::AsmOperand::symbolicName>(ref<ID>) +
                    lit<"]">) +
             assign<&ast::AsmOperand::constraint>(ref<StringLiteral>) +
             lit<"("> + assign<&ast::AsmOperand::expr>(ref<Expression>) +
             lit<")">> {};

  static constexpr auto AsmQualifier =
      kw<"volatile"> | kw<"__volatile__"> | kw<"inline"> | kw<"goto">;
  static constexpr auto AsmClobberItem = ref<STRING_LIT> | ref<ID>;

  // `asm volatile ("code" : outputs : inputs : clobbers);`. `clobbers` also
  // absorbs any further `:`-separated group -- GCC's "asm goto" jump-label
  // list shares its comma-of-string-or-identifier shape.
  struct AsmStatement
      : rule<create<ast::AsmStmt>(
                 AsmKw + many(append<&ast::AsmStmt::qualifiers>(
                             AsmQualifier)) +
                 lit<"("> +
                 assign<&ast::AsmStmt::code>(ref<StringLiteral>) +
                 option(
                     lit<":"> +
                     option(some(append<&ast::AsmStmt::outputs>(
                                     ref<AsmOperandSpec>),
                                 lit<",">)) +
                     option(
                         lit<":"> +
                         option(some(append<&ast::AsmStmt::inputs>(
                                         ref<AsmOperandSpec>),
                                     lit<",">)) +
                         many(lit<":"> +
                              option(some(append<&ast::AsmStmt::clobbers>(
                                              AsmClobberItem),
                                          lit<",">))))) +
                 lit<")"> + lit<";">)> {};

  struct Statement : rule<ref<CompoundStatement>   //
                          | ref<IfStatement>       //
                          | ref<SwitchStatement>   //
                          | ref<WhileStatement>    //
                          | ref<DoStatement>       //
                          | ref<ForStatement>      //
                          | ref<CaseStatement>     //
                          | ref<DefaultStatement>  //
                          | ref<JumpStatement>     //
                          | ref<AsmStatement>      //
                          | ref<LocalLabelDeclaration>  //
                          | ref<LabeledStatement>  //
                          | ref<ExpressionStatement>> {};

  // A standalone `[[...]];` or `__attribute__((...));`.
  struct AttributeDeclaration
      : rule<create<ast::AttributeDecl>(some(ref<AttributeSpec>) +
                                        lit<";">)> {};

  // Resolution of T(x) / T*x requires the surrounding typedef scope.
  struct AmbiguousShape
      : rule<assign<&ast::AmbiguousStmt::leftName>(ref<ID>) +
             ((assign<&ast::AmbiguousStmt::form>(lit<"(">) +
               assign<&ast::AmbiguousStmt::rightName>(ref<ID>) + lit<")">) |
              (assign<&ast::AmbiguousStmt::form>(lit<"*">) +
               assign<&ast::AmbiguousStmt::rightName>(ref<ID>)))> {};
  struct AmbiguousStmt : rule<ref<AmbiguousShape> + lit<";">> {};

  struct BlockItem
      : rule<ref<AmbiguousStmt> | ref<Declaration> | ref<Statement> |
             ref<StaticAssertDecl> | ref<AttributeDeclaration>> {};

  // Old-style: `main() { ... }` has no return type and means `int`; a
  // trailing `declaration*` between the declarator and the body is the
  // separate K&R parameter-type list (`f(a, b) int a; char *b; { ... }`).
  // A K&R declaration names a type first: an attribute there is the
  // declarator's, never the list's opening.
  struct FunctionDefinition
      : rule<option(assign<&ast::FunctionDef::specifiers>(
                 ref<DeclarationSpecifiers>)) +
             assign<&ast::FunctionDef::declarator>(ref<Declarator>) +
             many(!AttributeWord + append<&ast::FunctionDef::paramDeclarations>(
                                       ref<ParamTypeDeclaration>)) +
             assign<&ast::FunctionDef::body>(ref<CompoundStatement>)> {};

  // File-scope `asm("..." : outputs : inputs);`.
  struct AsmDeclaration
      : rule<create<ast::AsmDecl>(
                 AsmKw + lit<"("> +
                 assign<&ast::AsmDecl::code>(ref<StringLiteral>) +
                 option(lit<":"> +
                        option(some(append<&ast::AsmDecl::outputs>(
                                        ref<AsmOperandSpec>),
                                    lit<",">)) +
                        option(lit<":"> +
                               option(some(append<&ast::AsmDecl::inputs>(
                                               ref<AsmOperandSpec>),
                                           lit<",">)))) +
                 lit<")"> + lit<";">)> {};

  struct ExternalDeclaration
      : rule<option(kw<"__extension__">) +
             (ref<FunctionDefinition> | ref<Declaration> |
              ref<StaticAssertDecl> | ref<AsmDeclaration> |
              ref<AttributeDeclaration>)> {};

  struct TranslationUnit : rule<many(append<&ast::TranslationUnit::items>(
                               ref<ExternalDeclaration>))> {};

  using entry = TranslationUnit;
  static constexpr auto word_boundary = uc::XID_Continue | LegacyID;
  using ignore = list<WS, Directive>;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace c
