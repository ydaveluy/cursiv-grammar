// The trapc language: its grammar. TrapC is C without `union` or `goto`,
// plus a simplified inline-asm statement and `trap { ... }`.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "trapc/core/ast.hpp"

namespace trapc {

struct Trapc : cursiv::GrammarSpec {
  static constexpr auto word_boundary = cr<"_a-zA-Z0-9">;
  struct WS : terminal<some(s)> {};

  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  // A preprocessor directive, kept as trivia: there is no preprocessing
  // pass, so `#include`/`#define` lines are skipped verbatim, not expanded.
  struct PREPROC : terminal<lit<"#"> + many(cr<R"(^\n)">)> {};

  struct IdentifierToken : terminal<cr<"_a-zA-Z"> + many(cr<"_a-zA-Z0-9">)> {};
  static constexpr auto Reserved =
      kw<"alignas"> | kw<"alignof"> | kw<"auto"> | kw<"bool"> |
      kw<"break"> | kw<"case"> | kw<"char"> | kw<"const"> |
      kw<"constexpr"> | kw<"continue"> | kw<"default"> | kw<"do"> |
      kw<"double"> | kw<"else"> | kw<"enum"> | kw<"extern"> |
      kw<"false"> | kw<"float"> | kw<"for"> | kw<"goto"> |
      kw<"if"> | kw<"inline"> | kw<"int"> | kw<"long"> |
      kw<"nullptr"> | kw<"register"> | kw<"restrict"> | kw<"return"> |
      kw<"short"> | kw<"signed"> | kw<"sizeof"> | kw<"static"> |
      kw<"static_assert"> | kw<"struct"> | kw<"switch"> | kw<"thread_local"> |
      kw<"true"> | kw<"typedef"> | kw<"typeof"> | kw<"typeof_unqual"> |
      kw<"union"> | kw<"unsigned"> | kw<"void"> | kw<"volatile"> |
      kw<"while"> | kw<"_Alignas"> | kw<"_Alignof"> | kw<"_Atomic"> |
      kw<"_BitInt"> | kw<"_Bool"> | kw<"_Complex"> | kw<"_Decimal32"> |
      kw<"_Decimal64"> | kw<"_Decimal128"> | kw<"_Generic"> | kw<"_Imaginary"> |
      kw<"_Noreturn"> | kw<"_Static_assert"> | kw<"_Thread_local"> | kw<"trap"> |
      kw<"alias">;
  struct ID : rule<as<std::string_view>(!Reserved + ref<IdentifierToken>)> {};

  static constexpr auto HEXDIGIT = cr<"0-9a-fA-F">;

  static constexpr auto ESCAPE =
      lit<"\\"> + (cr<R"(abfnrtv\\"'?)">                 //
                   | (lit<"x"> + some(HEXDIGIT))       //
                   | (lit<"u"> + repeat<4>(HEXDIGIT))  //
                   | (lit<"U"> + repeat<8>(HEXDIGIT))  //
                   | (cr<"0-7"> + option(cr<"0-7">) + option(cr<"0-7">)));

  static constexpr auto INTEGERSUFFIX =
      (cr<"uU"> + option(lit<"ll"> | lit<"LL"> | cr<"lL">))  //
      | ((lit<"ll"> | lit<"LL"> | cr<"lL">) + option(cr<"uU">));
  struct INT_LIT : terminal<(((lit<"0"> + cr<"xX"> + some(HEXDIGIT))    //
                              | (lit<"0"> + cr<"bB"> + some(cr<"01">))  //
                              | (cr<"1-9"> + many(d))                   //
                              | (lit<"0"> + many(cr<"0-7">))) +         //
                             option(INTEGERSUFFIX))> {};

  static constexpr auto EXPONENT = cr<"eE"> + option(cr<"+-">) + some(d);
  static constexpr auto DecimalFloat =
      (((many(d) + lit<"."> + some(d)) | (some(d) + lit<".">)) + option(EXPONENT)) |
      (some(d) + EXPONENT);
  static constexpr auto HexSignificand =
      (some(HEXDIGIT) + option(lit<"."> + many(HEXDIGIT))) |
      (lit<"."> + some(HEXDIGIT));
  static constexpr auto HexFloat = lit<"0"> + cr<"xX"> + HexSignificand +
      cr<"pP"> + option(cr<"+-">) + some(d);
  struct FLOAT_LIT : terminal<(DecimalFloat | HexFloat) + option(cr<"flFL">)> {};


  struct CHAR_LIT : terminal<(option(lit<"u8"> | cr<"LuU">) + lit<"'"> +
                              some(ESCAPE | cr<R"(^'\\\r\n)">) + lit<"'">)> {
  };
  struct STRING_LIT
      : terminal<(option(lit<"u8"> | cr<"LuU">) + lit<"\""> +
                  many(ESCAPE | cr<R"(^"\\\r\n)">) + lit<"\"">)> {};

  static constexpr auto BuiltinTypeWord =
      kw<"void"> | kw<"char"> | kw<"short"> | kw<"int"> | kw<"long"> |
      kw<"float"> | kw<"double"> | kw<"signed"> | kw<"unsigned"> |
      kw<"_Bool"> | kw<"bool"> | kw<"_Complex"> | kw<"__m128"> |
      kw<"__m128d"> | kw<"__m128i">;

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
  static constexpr auto TypeofWord = kw<"typeof"> | kw<"__typeof__"> |
                                     kw<"__typeof">;
  static constexpr auto AsmWord = kw<"__asm"> | kw<"__asm__">;
  static constexpr auto CountofWord = kw<"_Countof">;
  // Visual C calling conventions, an optional prefix on a declarator's core.
  static constexpr auto VcCallConv =
      kw<"__cdecl"> | kw<"__clrcall"> | kw<"__stdcall"> | kw<"__fastcall"> |
      kw<"__thiscall"> | kw<"__vectorcall">;
  static constexpr auto AttributeWord = kw<"__attribute__"> | kw<"__attribute">;

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
  struct MemberDeclarator;
  struct Declarator;
  struct AbstractDeclarator;
  struct DeclarationSpecifiers;
  struct TypeSpecifier;
  struct StructSpecifier;
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
  struct AttributeDeclaration;
  struct Initializer;
  struct BracedInit;
  struct InitializerItem;
  struct Designator;
  struct DeclaratorSuffix;
  struct PointerPart;
  struct ParameterDeclaration;
  struct StringLiteral;
  struct BalancedParens;

  // A standard `[[...]]` or GNU `__attribute__((...))`/`__attribute((...))`
  // attribute: retained as text, since its
  // content is an open-ended, extension-specific grammar of its own.
  // `BalancedParens` balances the parens recursively so a nested one, e.g.
  // `((format(printf, 1, 2)))`, still matches as a whole.
  static constexpr auto AttrAtom =
      ref<IdentifierToken> | ref<INT_LIT> | ref<FLOAT_LIT> | ref<STRING_LIT> |
      ref<CHAR_LIT> | lit<","> | lit<"::"> | lit<"..."> | lit<"*"> |
      lit<"&"> | lit<"+"> | lit<"-"> | lit<"="> | lit<"<"> | lit<">"> |
      lit<"!"> | lit<"~"> | lit<"?"> | lit<":"> | lit<".">;
  struct BalancedParens
      : rule<lit<"("> + many(ref<BalancedParens> | AttrAtom) + lit<")">> {};
  struct GnuAttribute : rule<AttributeWord + ref<BalancedParens>> {};
  static constexpr auto AttrName = ref<IdentifierToken> + option(lit<"::"> + ref<IdentifierToken>);
  struct StdAttribute
      : rule<AttrName + option(ref<BalancedParens>)> {};
  struct StdAttributeSpec
      : rule<lit<"["> + lit<"["> + ref<StdAttribute> +
             many(lit<","> + ref<StdAttribute>) + lit<"]"> + lit<"]">> {};
  struct AttributeSpec
      : rule<ref<GnuAttribute> | ref<StdAttributeSpec>> {};

  // `__declspec(...)` is retained with declaration attributes.
  struct DeclSpec : rule<kw<"__declspec"> + ref<BalancedParens>> {};

  struct BuiltinTypeSpecifier
      : rule<some(append<&ast::BuiltinType::keywords>(BuiltinTypeWord))> {};

  struct TypedefName : rule<assign<&ast::NamedType::name>(ref<ID>)> {};

  // `_Atomic(T)`. Tried as part of `TypeSpecifier`; the bare qualifier form
  // is plain text in `Qualifier`, matched before a `TypeSpecifier` is due.
  struct AtomicSpecifier
      : rule<create<ast::AtomicType>(
            kw<"_Atomic"> + lit<"("> +
            assign<&ast::AtomicType::type>(ref<TypeName>) + lit<")">)> {};

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

  struct TypeSpecifier : rule<ref<BuiltinTypeSpecifier>   //
                              | ref<StructSpecifier>      //
                              | ref<EnumSpecifier>        //
                              | ref<AtomicSpecifier>      //
                              | ref<TypeofSpecifier>      //
                              | ref<TypedefName>> {};

  // `alignas(T)`/`alignas(expr)`, `_Alignas(...)`: ordered choice tries the
  // type reading first, same as everywhere else this grammar carries the
  // cast-shaped ambiguity.
  struct AlignmentSpecifier
      : rule<create<ast::AlignSpec>(
            (kw<"alignas"> | kw<"_Alignas">) + lit<"("> +
            (assign<&ast::AlignSpec::type>(ref<TypeName>)  //
             | assign<&ast::AlignSpec::expr>(ref<ConditionalExpression>)) +
            lit<")">)> {};

  static constexpr auto DeclSpecifierItem =
      append<&ast::Specifiers::qualifiers>(Qualifier) |
      append<&ast::Specifiers::aligns>(ref<AlignmentSpecifier>) |
      append<&ast::Specifiers::attributes>(ref<AttributeSpec>) |
      append<&ast::Specifiers::attributes>(ref<DeclSpec>);

  struct DeclarationSpecifiers
      : rule<many(DeclSpecifierItem) +
             assign<&ast::Specifiers::type>(ref<TypeSpecifier>) +
             many(DeclSpecifierItem)> {};

  struct StaticAssertDecl
      : rule<create<ast::StaticAssertItem>(
            (kw<"static_assert"> | kw<"_Static_assert">) + lit<"("> +
            assign<&ast::StaticAssertItem::cond>(ref<ConditionalExpression>) +
            option(lit<","> + assign<&ast::StaticAssertItem::message>(
                                   ref<StringLiteral>)) +
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

  struct MemberDeclarator
      : rule<(assign<&ast::InitDeclarator::declarator>(ref<Declarator>) +
              option(lit<":"> + assign<&ast::InitDeclarator::bitWidth>(
                                    ref<ConditionalExpression>)))  //
             | (lit<":"> + assign<&ast::InitDeclarator::bitWidth>(
                               ref<ConditionalExpression>))> {};

  // The only tag kind: no rule accepts `union`, so `ast::TagType` carries no
  // keyword.
  struct StructSpecifier
      : rule<kw<"struct"> + many(ref<AttributeSpec>) +
             ((option(assign<&ast::TagType::name>(ref<ID>)) +
               flag<&ast::TagType::defined>(lit<"{">) +
               many(append<&ast::TagType::members>(ref<MemberDeclaration>)) +
               lit<"}">)  //
              | assign<&ast::TagType::name>(ref<ID>))> {};

  struct EnumSpecifier
      : rule<kw<"enum"> + many(ref<AttributeSpec>) +
             ((option(assign<&ast::EnumType::name>(ref<ID>)) +
               option(lit<":"> + assign<&ast::EnumType::underlying>(
                   ref<DeclarationSpecifiers>)) +
               flag<&ast::EnumType::defined>(lit<"{">) +
               some(append<&ast::EnumType::enumerators>(ref<Enumerator>),
                    lit<",">) +
               option(lit<",">) + lit<"}">)  //
              | (assign<&ast::EnumType::name>(ref<ID>) +
                 option(lit<":"> + assign<&ast::EnumType::underlying>(
                     ref<DeclarationSpecifiers>))))> {};

  struct Enumerator : rule<assign<&ast::Enumerator::name>(ref<ID>) +
                           many(ref<AttributeSpec>) +
                           option(lit<"="> + assign<&ast::Enumerator::value>(
                                                 ref<ConditionalExpression>))> {
  };

  struct PointerPart
      : rule<(lit<"*"> | flag<&ast::Pointer::isBlock>(lit<"^">)) +
             many(append<&ast::Pointer::qualifiers>(TypeQualifier))> {};

  struct ArraySuffix : rule<lit<"["> +
                            option(assign<&ast::ArraySuffix::size>(
                                ref<AssignmentExpression>)) +
                            lit<"]">> {};

  struct FunctionSuffix
      : rule<lit<"("> +
             option(some(append<&ast::FunctionSuffix::params>(
                             ref<ParameterDeclaration>),
                         lit<",">) +
                    option(lit<","> + flag<&ast::FunctionSuffix::variadic>(
                                          lit<"...">))) +
             lit<")">> {};

  struct DeclaratorSuffix : rule<ref<ArraySuffix> | ref<FunctionSuffix>> {};

  struct Declarator
      : rule<many(append<&ast::Declarator::pointers>(ref<PointerPart>)) +
             option(assign<&ast::Declarator::callingConvention>(VcCallConv)) +
             (assign<&ast::Declarator::name>(ref<ID>)  //
              | (lit<"("> + assign<&ast::Declarator::nested>(ref<Declarator>) +
                 lit<")">)) +
             many(append<&ast::Declarator::suffixes>(ref<DeclaratorSuffix>)) +
             many(append<&ast::Declarator::attributes>(ref<AttributeSpec>))> {};

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
            flag<&ast::BoolLit::value>(kw<"true">) | kw<"false">)> {};
  struct NullptrLiteral : rule<create<ast::NullptrLit>(kw<"nullptr">)> {};
  struct PredefinedLiteral
      : rule<create<ast::PredefinedExpr>(assign<&ast::PredefinedExpr::name>(
            kw<"__func__"> | kw<"__FUNCTION__"> |
            kw<"__PRETTY_FUNCTION__">))> {};

  // GNU statement expression: `({ stmts; })`.
  struct StmtExpr
      : rule<create<ast::StmtExpr>(
            lit<"("> +
            assign<&ast::StmtExpr::body>(ref<CompoundStatement>) +
            lit<")">)> {};

  struct GenericAssociation
      : rule<(assign<&ast::GenericAssoc::type>(ref<TypeName>) |
              kw<"default">) +
             lit<":"> +
             assign<&ast::GenericAssoc::value>(ref<AssignmentExpression>)> {};

  struct GenericSelection
      : rule<create<ast::GenericExpr>(
            kw<"_Generic"> + lit<"("> +
            assign<&ast::GenericExpr::control>(ref<AssignmentExpression>) +
            lit<","> +
            some(append<&ast::GenericExpr::assocs>(ref<GenericAssociation>),
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

  struct PrimaryExpression : rule<(lit<"("> + ref<Expression> + lit<")">)  //
                                  | ref<StmtExpr>                         //
                                  | ref<GenericSelection>                 //
                                  | ref<BuiltinCall>                      //
                                  | ref<FloatingLiteral>                   //
                                  | ref<IntegerLiteral>                    //
                                  | ref<CharacterLiteral>                  //
                                  | ref<StringLiteral>                     //
                                  | ref<BoolLiteral>                       //
                                  | ref<NullptrLiteral>                    //
                                  | ref<PredefinedLiteral>                 //
                                  | ref<Identifier>> {};

  // `(T){ ... }`: shares its `(` prefix with a cast, disambiguated by the
  // `{` that follows the type name.
  struct CompoundLiteral
      : rule<create<ast::CompoundLiteral>(
            lit<"("> + assign<&ast::CompoundLiteral::type>(ref<TypeName>) +
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

  struct BinaryExpression : infix<CastExpression, ast::BinaryExpr,
                                  levels(left(OpMul),     //
                                         left(OpAdd),     //
                                         left(OpShift),   //
                                         left(OpRel),     //
                                         left(OpEq),      //
                                         left(OpBitAnd),  //
                                         left(OpBitXor),  //
                                         left(OpBitOr),   //
                                         left(OpLogAnd),  //
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
      : infix<AssignmentExpression, ast::BinaryExpr, left(lit<",">)> {};

  struct Designator
      : rule<create<ast::FieldDesignator>(
                 lit<"."> + assign<&ast::FieldDesignator::name>(ref<ID>))  //
             |
             create<ast::IndexDesignator>(lit<"["> +
                                          assign<&ast::IndexDesignator::index>(
                                              ref<ConditionalExpression>) +
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

  struct Declaration
      : rule<assign<&ast::Declaration::specifiers>(ref<DeclarationSpecifiers>) +
             many(append<&ast::Declaration::declarators>(ref<InitDeclarator>),
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
             assign<&ast::ForStmt::init>(ref<Declaration> |
                                         ref<ExpressionStatement>) +
             option(assign<&ast::ForStmt::cond>(ref<Expression>)) + lit<";"> +
             option(assign<&ast::ForStmt::step>(ref<Expression>)) + lit<")"> +
             assign<&ast::ForStmt::body>(ref<Statement>)> {};

  struct CaseStatement
      : rule<kw<"case"> +
             assign<&ast::CaseLabel::value>(ref<ConditionalExpression>) +
             lit<":"> + assign<&ast::CaseLabel::body>(ref<Statement>)> {};

  struct DefaultStatement
      : rule<kw<"default"> + lit<":"> +
             assign<&ast::DefaultLabel::body>(ref<Statement>)> {};

  struct LabeledStatement : rule<assign<&ast::Label::name>(ref<ID>) + lit<":"> +
                                 assign<&ast::Label::body>(ref<Statement>)> {};

  // GNU `__label__ name;`: declares a block-scoped label name.
  struct LocalLabelDeclaration
      : rule<create<ast::LocalLabelDecl>(
            kw<"__label__"> +
            assign<&ast::LocalLabelDecl::name>(ref<ID>) + lit<";">)> {};

  // No `goto`: only `continue`/`break`/`return`.
  struct JumpStatement
      : rule<create<ast::ContinueStmt>(kw<"continue"> + lit<";">)  //
             | create<ast::BreakStmt>(kw<"break"> + lit<";">)      //
             | create<ast::ReturnStmt>(
                   kw<"return"> +
                   option(assign<&ast::ReturnStmt::value>(ref<Expression>)) +
                   lit<";">)> {};

  struct AsmOperandList
      : rule<many(append<&ast::AsmOperandGroup::exprs>(ref<BinaryExpression>),
                  lit<",">)> {};

  // Inline asm, simpler than GNU's: no constraint strings, no clobber list,
  // just an unlabeled expression group and any number of `:`-separated ones.
  struct AsmStatement
      : rule<create<ast::AsmStmt>(
            AsmWord + (kw<"volatile"> | kw<"__volatile__">) + lit<"("> +
            append<&ast::AsmStmt::groups>(
                create<ast::AsmOperandGroup>(ref<AsmOperandList>)) +
            many(lit<":"> +
                 append<&ast::AsmStmt::groups>(
                     create<ast::AsmOperandGroup>(ref<AsmOperandList>))) +
            lit<")"> + lit<";">)> {};

  struct TrapStatement
      : rule<create<ast::TrapStmt>(
            kw<"trap"> +
            assign<&ast::TrapStmt::body>(ref<CompoundStatement>))> {};

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
                          | ref<TrapStatement>     //
                          | ref<LocalLabelDeclaration>  //
                          | ref<LabeledStatement>  //
                          | ref<ExpressionStatement>> {};

  struct BlockItem
      : rule<ref<StaticAssertDecl> | ref<Declaration> | ref<Statement> | ref<AttributeDeclaration>> {
  };

  struct FunctionDefinition
      : rule<assign<&ast::FunctionDef::specifiers>(ref<DeclarationSpecifiers>) +
             assign<&ast::FunctionDef::declarator>(ref<Declarator>) +
             assign<&ast::FunctionDef::body>(ref<CompoundStatement>)> {};

  // One operand of a file-scope asm: `["name"] "constraint"(expr)`.
  struct AsmOperandSpec
      : rule<option(lit<"["> +
                    assign<&ast::AsmOperand::symbolicName>(ref<ID>) +
                    lit<"]">) +
             assign<&ast::AsmOperand::constraint>(ref<StringLiteral>) +
             lit<"("> + assign<&ast::AsmOperand::expr>(ref<Expression>) +
             lit<")">> {};

  // File-scope `asm("..." : outputs : inputs)`.
  struct AsmDeclaration
      : rule<create<ast::AsmDecl>(
            (kw<"asm"> | AsmWord) + lit<"("> +
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

  // A standalone `[[...]];` or `__attribute__((...));`.
  struct AttributeDeclaration
      : rule<create<ast::AttributeDecl>(
            some(append<&ast::AttributeDecl::attributes>(ref<AttributeSpec>)) +
            lit<";">)> {};

  struct ExternalDeclaration
      : rule<option(kw<"__extension__">) +
             (ref<FunctionDefinition> | ref<Declaration> | ref<StaticAssertDecl> |
              ref<AsmDeclaration> | ref<AttributeDeclaration>)> {};

  struct TranslationUnit : rule<many(append<&ast::TranslationUnit::items>(
                               ref<ExternalDeclaration>))> {};

  using entry = TranslationUnit;
  using ignored = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT, PREPROC>;
};

}  // namespace trapc
