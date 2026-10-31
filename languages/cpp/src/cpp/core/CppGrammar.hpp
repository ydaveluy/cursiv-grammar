// The cpp language: its grammar.
#pragma once

#include "cpp/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace cpp {

struct Cpp : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  static constexpr auto LineSplice =
      lit<"\\"> + option(lit<"\r">) + lit<"\n">;
  struct SL_COMMENT
      : terminal<lit<"//"> + many(LineSplice | cr<R"(^\r\n)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  // A preprocessor line, `#define`/`#include`/... included: swallowed whole
  // rather than run, so the macros it names stay unexpanded text. A trailing
  // `\` splices the next physical line into the same directive.
  struct Directive
      : terminal<(lit<"#"> | lit<"%:">) +
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
  static constexpr auto LegacyStart =
      ~(~LegacyID | cr<"\u0300-\u036F\u1DC0-\u1DFF\u20D0-\u20FF\uFE20-\uFE2F">);
  static constexpr auto IdChars =
      (uc::XID_Start | cr<"_"> | LegacyStart | UCN) +
      many(uc::XID_Continue | LegacyID | UCN);
  struct ID : terminal<IdChars> {};

  // A backslash-newline splice: valid inside a literal's escape sequence,
  // not just between preprocessor-directive lines. `\u`/`\U` here are a
  // universal-character-name too -- same spelling as in `ID` above.
  static constexpr auto ESCAPE =
      lit<"\\"> + (cr<R"(abfnrtv\\"'?)">                //
                   | (lit<"x"> + some(HEXDIGIT))       //
                   | (lit<"u"> + repeat<4>(HEXDIGIT))  //
                   | (lit<"U"> + repeat<8>(HEXDIGIT))  //
                   | (cr<"0-7"> + option(cr<"0-7">) + option(cr<"0-7">))  //
                   | (option(lit<"\r">) + lit<"\n">));

  static constexpr auto INTEGERSUFFIX =
      (cr<"uU"> + option(lit<"ll"> | lit<"LL"> | cr<"lL">))  //
      | ((lit<"ll"> | lit<"LL"> | cr<"lL">) + option(cr<"uU">));
  static constexpr auto INT_BODY =
      ((lit<"0"> + cr<"xX"> + HEXDIGIT + many(option(lit<"'">) + HEXDIGIT))  //
       | (lit<"0"> + cr<"bB"> + cr<"01"> + many(option(lit<"'">) + cr<"01">))  //
       | (cr<"1-9"> + many(option(lit<"'">) + d))                           //
       | (lit<"0"> + many(option(lit<"'">) + cr<"0-7">)));
  struct INT_LIT : terminal<INT_BODY + option(INTEGERSUFFIX)> {};

  static constexpr auto DIGITSEQ = d + many(option(lit<"'">) + d);
  static constexpr auto HEXDIGITSEQ = HEXDIGIT + many(option(lit<"'">) + HEXDIGIT);
  static constexpr auto EXPONENT = cr<"eE"> + option(cr<"+-">) + DIGITSEQ;
  static constexpr auto HEXEXPONENT = cr<"pP"> + option(cr<"+-">) + DIGITSEQ;
  static constexpr auto FLOAT_BODY =
      (((option(DIGITSEQ) + lit<"."> + DIGITSEQ) | (DIGITSEQ + lit<".">)) +
       option(EXPONENT))  //
      | (DIGITSEQ + EXPONENT)
      | (lit<"0"> + cr<"xX"> +
         ((HEXDIGITSEQ + option(lit<"."> + option(HEXDIGITSEQ))) |
          (lit<"."> + HEXDIGITSEQ)) + HEXEXPONENT);
  struct FLOAT_LIT : terminal<FLOAT_BODY + option(cr<"flFL">)> {};

  static constexpr auto ENCODING_PREFIX = lit<"u8"> | cr<"uUL">;
  static constexpr auto CHAR_BODY =
      option(ENCODING_PREFIX) + lit<"'"> +
      some(ESCAPE | (!cr<R"('\r\n)"> + any)) + lit<"'">;
  struct CHAR_LIT : terminal<CHAR_BODY> {};

  static constexpr auto STRING_BODY = option(ENCODING_PREFIX) + lit<"\""> +
                                       many(ESCAPE | (!cr<R"("\r\n)"> + any)) +
                                       lit<"\"">;
  struct STRING_LIT : terminal<STRING_BODY> {};

  // A raw string's delimiter is a run `mark<>` remembers and `same<>` reads
  // back on the closer -- checked, not merely assumed to match.
  struct RawDelim {};
  static constexpr auto RAW_STRING_BODY =
      through(option(ENCODING_PREFIX) + lit<"R\""> +
       mark<RawDelim>(many(cr<R"chars(^ ()\\\t\v\f\r\n)chars">)) + lit<"(">, lit<")"> + same<RawDelim> + lit<"\"">);
  struct RAW_STRING_LIT : terminal<RAW_STRING_BODY> {};

  static constexpr auto UD_SUFFIX = IdChars;
  struct UD_LIT : terminal<(INT_BODY + UD_SUFFIX) | (FLOAT_BODY + UD_SUFFIX) |
                           (CHAR_BODY + UD_SUFFIX) |
                           (STRING_BODY + UD_SUFFIX) |
                           (RAW_STRING_BODY + UD_SUFFIX)> {};

  // C++14 digraphs, including the <:: tokenization exception.
  static constexpr auto LBracket = lit<"["> | lit<"<:", !(lit<":"> + !cr<":>">)>;
  static constexpr auto RBracket = lit<"]"> | lit<":>">;
  static constexpr auto LBrace = lit<"{"> | lit<"<%">;
  static constexpr auto RBrace = lit<"}"> | lit<"%>">;

  static constexpr auto Lt = lit<"<", !cr<"=">>;
  static constexpr auto Gt = lit<">", !cr<"=">>;

  static constexpr auto StorageClass = kw<"register"> | kw<"static"> |
                                       kw<"thread_local"> | kw<"extern"> |
                                       kw<"mutable">;
  static constexpr auto FunctionSpecifierKw =
      kw<"inline"> | kw<"virtual"> | kw<"explicit">;
  static constexpr auto CvQualifierKw = kw<"const"> | kw<"volatile">;
  static constexpr auto Qualifier = StorageClass | FunctionSpecifierKw |
                                    CvQualifierKw | kw<"friend"> |
                                    kw<"typedef"> | kw<"constexpr">;
  static constexpr auto BuiltinTypeWord =
      kw<"char"> | kw<"char16_t"> | kw<"char32_t"> | kw<"wchar_t"> |
      kw<"bool"> | kw<"short"> | kw<"int"> | kw<"long"> | kw<"float"> |
      kw<"signed"> | kw<"unsigned"> | kw<"double"> | kw<"void"> | kw<"auto">;
  static constexpr auto ClassKeyKw = kw<"class"> | kw<"struct"> | kw<"union">;
  static constexpr auto ReservedWord =
      Qualifier | BuiltinTypeWord | ClassKeyKw | kw<"alignas"> | kw<"alignof"> |
      kw<"and"> | kw<"and_eq"> | kw<"asm"> | kw<"bitand"> | kw<"bitor"> |
      kw<"break"> | kw<"case"> | kw<"catch"> | kw<"compl"> | kw<"const_cast"> |
      kw<"continue"> | kw<"decltype"> | kw<"default"> | kw<"delete"> | kw<"do"> |
      kw<"dynamic_cast"> | kw<"else"> | kw<"enum"> | kw<"export"> | kw<"false"> |
      kw<"for"> | kw<"goto"> | kw<"if"> | kw<"namespace"> | kw<"new"> |
      kw<"noexcept"> | kw<"not"> | kw<"not_eq"> | kw<"nullptr"> | kw<"operator"> |
      kw<"or"> | kw<"or_eq"> | kw<"private"> | kw<"protected"> | kw<"public"> |
      kw<"reinterpret_cast"> | kw<"return"> | kw<"sizeof"> | kw<"static_assert"> |
      kw<"static_cast"> | kw<"switch"> | kw<"template"> | kw<"this"> |
      kw<"throw"> | kw<"true"> | kw<"try"> | kw<"typeid"> | kw<"typename"> |
      kw<"using"> | kw<"while"> | kw<"xor"> | kw<"xor_eq">;

  struct TypeId;
  struct NewTypeId;
  struct NewDeclarator;
  struct ExpandedTypeId;
  struct PackExpansion;
  struct Argument;
  struct UnnamedPackDeclarator;
  struct Declarator;
  struct AbstractDeclarator;
  struct Expression;
  struct AssignmentExpression;
  struct BinaryExpression;
  struct CastExpression;
  struct ConditionalExpression;
  struct ConstantExpression;
  struct CompoundStatement;
  struct Statement;
  struct InitializerClause;
  struct BracedInitList;
  struct NestedNameSpecifier;
  struct UnqualifiedId;
  struct TheTypeName;
  struct DecltypeSpecifier;
  struct SimpleTemplateId;
  struct ClassSpecifier;
  struct EnumSpecifier;
  struct ElaboratedTypeSpecifier;
  struct TypeNameSpecifier;
  struct NamedTypeSpec;
  struct DeclSpecifierSeq;
  struct Param;
  struct ExceptionSpec;
  struct ParametersAndQualifiers;
  struct MemberDeclarator;
  struct Declaration;
  struct MemberDeclaration;
  struct TemplateParameter;
  struct AccessLabel;
  struct TemplateDecl;
  struct FunctionDefinition;
  struct InitDeclarator;
  struct BlockDeclaration;
  struct CatchClause;

  // -- Names --------------------------------------------------------

  struct PlainIdent : rule<assign<&ast::Ident::name>(ref<ID>)> {};

  // A type name is any identifier, with no symbol table to say otherwise: a
  // class, enum or typedef name all collapse to this one rule.
  struct TypedefName : rule<ref<PlainIdent>> {};
  struct TheTypeName
      : rule<ref<SimpleTemplateId> | ref<TypedefName>> {};

  struct NamespaceName : rule<ref<PlainIdent>> {};

  struct DecltypeOrTypeName
      : rule<ref<DecltypeSpecifier> | ref<TheTypeName> | ref<NamespaceName>> {
  };

  struct NestedNameSpecifier
      : rule<(option(assign<&ast::QualifiedName::qualifier>(
                  ref<DecltypeOrTypeName>)) +
              lit<"::">)  //
             | (assign<&ast::QualifiedName::qualifier>(
                    ref<NestedNameSpecifier>) +
                assign<&ast::QualifiedName::id>(
                    ref<PlainIdent> |
                    (option(kw<"template">) + ref<SimpleTemplateId>)) +
                lit<"::">)> {};

  struct QualifiedId
      : rule<assign<&ast::QualifiedName::qualifier>(ref<NestedNameSpecifier>) +
             option(flag<&ast::QualifiedName::templateKw>(
                 kw<"template">)) +
             assign<&ast::QualifiedName::id>(ref<UnqualifiedId>)> {};

  // `N::X` is one QualifiedName holding the prefix and the name; a bare `X`
  // is X's own node.
  template <class X>
  static constexpr auto Qualified =
      create<ast::QualifiedName>(
          assign<&ast::QualifiedName::qualifier>(ref<NestedNameSpecifier>) +
          assign<&ast::QualifiedName::id>(ref<X>)) |
      ref<X>;
  static constexpr auto QualifiedTemplateId = create<ast::QualifiedName>(
      assign<&ast::QualifiedName::qualifier>(ref<NestedNameSpecifier>) +
      flag<&ast::QualifiedName::templateKw>(kw<"template">) +
      assign<&ast::QualifiedName::id>(ref<SimpleTemplateId>));

  struct OpCallParens : rule<lit<"("> + lit<")">> {};
  struct OpSubscriptBrackets : rule<LBracket + RBracket> {};
  struct OpNew : rule<kw<"new"> + option(LBracket + RBracket)> {};
  struct OpDelete : rule<kw<"delete"> + option(LBracket + RBracket)> {};
  struct OpShl : rule<Lt + gap::none + Lt> {};
  struct OpShr : rule<Gt + gap::none + Gt> {};
  static constexpr auto TheOperatorText =
      ref<OpNew> | ref<OpDelete>  //
      | lit<"+"> | lit<"-"> | lit<"*"> | lit<"/"> | lit<"%"> |
      (lit<"^"> | kw<"xor">) | (lit<"&"> | kw<"bitand">) |
      (lit<"|"> | kw<"bitor">) | (lit<"~"> | kw<"compl">) |
      (lit<"!"> | kw<"not">) |
      lit<"=", !cr<"=">> | ref<OpShl> | ref<OpShr> | lit<">>="> |
      lit<"<<="> | lit<"+="> | lit<"-="> | lit<"*="> | lit<"/="> |
      lit<"%="> | (lit<"^="> | kw<"xor_eq">) |
      (lit<"&="> | kw<"and_eq">) | (lit<"|="> | kw<"or_eq">) | lit<"=="> |
      (lit<"!="> | kw<"not_eq">) | lit<"<="> | lit<">="> |
      (lit<"&&"> | kw<"and">) | (lit<"||"> | kw<"or">) | lit<"++"> |
      lit<"--"> | lit<","> | lit<"->*"> | lit<"->"> |
      ref<OpCallParens> | ref<OpSubscriptBrackets> | Gt | Lt;

  struct OperatorFunctionId
      : rule<kw<"operator"> +
             assign<&ast::OperatorFunctionName::op>(TheOperatorText)> {};

  struct ConversionFunctionId
      : rule<kw<"operator"> +
             assign<&ast::ConversionFunctionName::type>(ref<TypeId>)> {};

  // The no-space spelling (`operator""_km`) reads the same way here: two
  // adjacent tokens with nothing between them still parse, so only the
  // general, two-token form (`operator` `""` name) is needed.
  struct LiteralOperatorId
      : rule<kw<"operator"> + ref<STRING_LIT> +
             assign<&ast::LiteralOperatorName::suffix>(ref<ID>)> {};

  struct DestructorNameSimple
      : rule<lit<"~"> +
             assign<&ast::DestructorName::target>(ref<TheTypeName> |
                                                   ref<DecltypeSpecifier>)> {
  };

  struct TemplateId
      : rule<assign<&ast::TemplateId::templateName>(ref<PlainIdent> |
                 ref<OperatorFunctionId> | ref<LiteralOperatorId>) + Lt +
             option(some(append<&ast::TemplateId::args>(
                             ref<ExpandedTypeId> | ref<TypeId> |
                             ref<PackExpansion> | ref<ConstantExpression>),
                    lit<",">)) +
             Gt> {};

  struct SimpleTemplateId : rule<ref<TemplateId>> {};

  struct UnqualifiedId
      : rule<ref<PlainIdent> | ref<OperatorFunctionId> |
             ref<ConversionFunctionId> | ref<LiteralOperatorId> |
             ref<DestructorNameSimple> | ref<TemplateId>> {};

  struct IdExpression : rule<ref<QualifiedId> | ref<UnqualifiedId>> {};

  // -- Attributes -----------------------------------------------------

  struct BalancedParens;
  struct BalancedBrackets;
  struct BalancedBraces;
  static constexpr auto AttrAtom =
      ref<ID> | ref<INT_LIT> | ref<FLOAT_LIT> | ref<CHAR_LIT> |
      ref<STRING_LIT> | ref<RAW_STRING_LIT> | ref<UD_LIT> | lit<","> |
      lit<"::"> | lit<"..."> | lit<"*"> | lit<"&"> | lit<"+"> | lit<"-"> |
      lit<"="> | Lt | Gt | lit<"!"> | lit<"~"> | lit<"?"> | lit<":"> |
      lit<"."> | lit<"/", !cr<"/*">> | lit<"%"> | lit<"|"> | lit<"^"> |
      lit<"&&"> | lit<"||"> | lit<"=="> | lit<"!="> | lit<"<="> | lit<">="> |
      lit<"->"> | lit<"->*"> | lit<".*"> | lit<"++"> | lit<"--"> | lit<";"> |
      lit<"<<"> | lit<">>"> | lit<"<<="> | lit<">>="> | lit<"+="> | lit<"-="> |
      lit<"*="> | lit<"/="> | lit<"%="> | lit<"&="> | lit<"^="> | lit<"|=">;
  static constexpr auto BalancedTokens =
      many(append<&ast::AttributeGroup::tokens>(
          ref<BalancedParens> | ref<BalancedBrackets> | ref<BalancedBraces> |
          create<ast::AttributeAtom>(
              assign<&ast::AttributeAtom::text>(AttrAtom))));
  struct BalancedParens
      : rule<assign<&ast::AttributeGroup::opening>(lit<"(">) + BalancedTokens +
             assign<&ast::AttributeGroup::closing>(lit<")">)> {};
  struct BalancedBrackets
      : rule<assign<&ast::AttributeGroup::opening>(LBracket) + BalancedTokens +
             assign<&ast::AttributeGroup::closing>(RBracket)> {};
  struct BalancedBraces
      : rule<assign<&ast::AttributeGroup::opening>(LBrace) + BalancedTokens +
             assign<&ast::AttributeGroup::closing>(RBrace)> {};
  static constexpr auto AttrName = ref<ID> + option(lit<"::"> + ref<ID>);
  struct StdAttribute
      : rule<assign<&ast::Attribute::name>(AttrName) +
             option(assign<&ast::Attribute::arguments>(ref<BalancedParens>)) +
             option(flag<&ast::Attribute::packExpansion>(lit<"...">))> {};
  struct AlignAttr
      : rule<assign<&ast::AttributeSpecifier::kind>(
                 as<ast::AttributeKind::Alignment>(kw<"alignas">)) +
             lit<"("> +
             (assign<&ast::AttributeSpecifier::alignmentType>(ref<TypeId>) |
              assign<&ast::AttributeSpecifier::alignmentExpression>(
                  ref<ConstantExpression>)) +
             option(flag<&ast::AttributeSpecifier::packExpansion>(lit<"...">)) +
             lit<")">> {};
  struct StdAttributeSpec
      : rule<create<ast::AttributeSpecifier> + LBracket + LBracket +
             option(append<&ast::AttributeSpecifier::attributes>(
                 ref<StdAttribute>)) +
             many(lit<","> +
                  option(append<&ast::AttributeSpecifier::attributes>(
                      ref<StdAttribute>))) +
             RBracket + RBracket> {};
  static constexpr auto AttributeSpec = ref<StdAttributeSpec> | ref<AlignAttr>;
  template <auto Member>
  static constexpr auto Attrs = many(append<Member>(AttributeSpec));

  // -- Expressions ----------------------------------------------------

  struct ThisExpr : rule<create<ast::ThisExpr>(kw<"this">)> {};
  struct IntegerLiteral : rule<assign<&ast::IntLit::text>(ref<INT_LIT>)> {};
  struct FloatingLiteral : rule<assign<&ast::FloatLit::text>(ref<FLOAT_LIT>)> {
  };
  struct CharacterLiteral : rule<assign<&ast::CharLit::text>(ref<CHAR_LIT>)> {
  };
  struct StringLiteral
      : rule<some(append<&ast::StringLit::pieces>(ref<STRING_LIT> |
                                                   ref<RAW_STRING_LIT>))> {};
  struct BoolLiteral
      : rule<create<ast::BoolLit>(
                 flag<&ast::BoolLit::value>(kw<"true">) |
                 kw<"false">)> {};
  struct PointerLiteral : rule<create<ast::NullptrLit>(kw<"nullptr">)> {};
  struct UserDefinedLiteral
      : rule<assign<&ast::UserDefinedLit::text>(ref<UD_LIT>)> {};

  // `literal+`: adjacent string literals concatenate (real C++); every
  // other kind stands alone, since a mixed run has no meaning in the
  // language this grammar is approximating.
  struct Literal
      : rule<ref<StringLiteral> | ref<FloatingLiteral> | ref<IntegerLiteral> |
             ref<CharacterLiteral> | ref<BoolLiteral> | ref<PointerLiteral> |
             ref<UserDefinedLiteral>> {};

  struct SimpleCapture
      : rule<create<ast::Capture>(
                 option(flag<&ast::Capture::byRef>(lit<"&"> | kw<"bitand">)) +
                 assign<&ast::Capture::name>(ref<ID>) +
                 option(flag<&ast::Capture::pack>(lit<"...">)))  //
             | create<ast::Capture>(
                   flag<&ast::Capture::isThis>(kw<"this">))> {};
  struct InitCapture
      : rule<create<ast::Capture>(
                 option(flag<&ast::Capture::byRef>(lit<"&"> | kw<"bitand">)) +
                 assign<&ast::Capture::name>(ref<ID>) +
                 ((lit<"="> + assign<&ast::Capture::init>(ref<InitializerClause>)) |
                  assign<&ast::Capture::init>(ref<BracedInitList>)))> {};
  struct Capture : rule<ref<InitCapture> | ref<SimpleCapture>> {};
  // Plain combinators, not rules of their own: each writes straight into
  // `ast::LambdaExpr`, so naming either as a `ref<>`-called rule would make
  // it a second, competing creator of the same class (the generator refuses
  // that -- "this alternative can create twice"). Inlined into
  // LambdaExpression's one `create<>` below, there is only the one head.
  static constexpr auto CaptureDefault =
      assign<&ast::LambdaExpr::captureDefault>(lit<"&"> | kw<"bitand"> | lit<"=">);
  static constexpr auto LambdaIntroducer =
      LBracket +
      option((some(append<&ast::LambdaExpr::captures>(ref<Capture>), lit<",">)) |
             (CaptureDefault +
              option(lit<","> +
                     some(append<&ast::LambdaExpr::captures>(ref<Capture>), lit<",">)))) +
      RBracket;
  static constexpr auto LambdaDeclarator =
      flag<&ast::LambdaExpr::hasDeclarator>(lit<"(">) +
      option((some(append<&ast::LambdaExpr::params>(ref<Param>), lit<",">) +
              option(lit<","> + flag<&ast::LambdaExpr::variadic>(lit<"...">))) |
             flag<&ast::LambdaExpr::variadic>(lit<"...">)) +
      lit<")"> + option(flag<&ast::LambdaExpr::mutableKw>(kw<"mutable">)) +
      option(assign<&ast::LambdaExpr::exceptionSpec>(ref<ExceptionSpec>)) +
      Attrs<&ast::LambdaExpr::attributes> +
      option(lit<"->"> +
             assign<&ast::LambdaExpr::trailingReturnType>(ref<TypeId>));
  struct LambdaExpression
      : rule<create<ast::LambdaExpr>(LambdaIntroducer +
                                     option(LambdaDeclarator) +
                                     assign<&ast::LambdaExpr::body>(
                                         ref<CompoundStatement>))> {};

  struct PrimaryExpression
      : rule<ref<Literal> | ref<ThisExpr> |
             (lit<"("> + ref<Expression> + lit<")">) | ref<IdExpression> |
             ref<LambdaExpression>> {};

  struct NamedCast
      : rule<create<ast::NamedCastExpr>(
                 assign<&ast::NamedCastExpr::kind>(
                     kw<"dynamic_cast"> | kw<"static_cast"> |
                     kw<"reinterpret_cast"> | kw<"const_cast">) +
                 Lt + assign<&ast::NamedCastExpr::type>(ref<TypeId>) + Gt +
                 lit<"("> +
                 assign<&ast::NamedCastExpr::operand>(ref<Expression>) +
                 lit<")">)> {};

  struct TypeidExpr
      : rule<create<ast::TypeidExpr>(
                 kw<"typeid"> + lit<"("> +
                 (assign<&ast::TypeidExpr::expr>(ref<Expression>)  //
                  | assign<&ast::TypeidExpr::type>(ref<TypeId>)) +
                 lit<")">)> {};

  struct FunctionalCast
      : rule<create<ast::FunctionalCastExpr>(
                 assign<&ast::FunctionalCastExpr::type>(ref<TypeId>) +
                 ((lit<"("> +
                   option(some(append<&ast::FunctionalCastExpr::args>(
                                   ref<Argument>),
                               lit<",">)) +
                   lit<")">)  //
                  | (flag<&ast::FunctionalCastExpr::braced>(LBrace) +
                     option(some(append<&ast::FunctionalCastExpr::args>(
                                     ref<Argument>),
                                 lit<",">) +
                            option(lit<",">)) +
                     RBrace)))> {};

  struct PseudoDestructorName
      : rule<option(assign<&ast::DestructorName::qualifier>(
                 ref<NestedNameSpecifier>)) +
             option(assign<&ast::DestructorName::extraType>(
                        ref<TheTypeName>) +
                    lit<"::">) +
             lit<"~"> +
             assign<&ast::DestructorName::target>(
                 ref<TheTypeName> | ref<DecltypeSpecifier>)> {};

  struct MemberName
      : rule<(option(kw<"template">) + ref<IdExpression>)  //
             | ref<PseudoDestructorName>> {};

  struct PostfixExpression
      : rule<(ref<PrimaryExpression> | ref<FunctionalCast> | ref<NamedCast> |
              ref<TypeidExpr>) +
             many(nest<ast::IndexExpr, &ast::IndexExpr::array>(
                      LBracket +
                      assign<&ast::IndexExpr::index>(
                          ref<Expression> | ref<BracedInitList>) +
                      RBracket)  //
                  | nest<ast::CallExpr, &ast::CallExpr::callee>(
                        lit<"("> +
                        option(some(append<&ast::CallExpr::args>(
                                        ref<Argument>),
                                    lit<",">)) +
                        lit<")">)  //
                  | nest<ast::MemberExpr, &ast::MemberExpr::object>(
                        flag<&ast::MemberExpr::arrow>(lit<"->">) +
                        assign<&ast::MemberExpr::member>(ref<MemberName>))  //
                  | nest<ast::MemberExpr, &ast::MemberExpr::object>(
                        lit<"."> +
                        assign<&ast::MemberExpr::member>(ref<MemberName>))  //
                  | nest<ast::PostfixExpr, &ast::PostfixExpr::operand>(
                        assign<&ast::PostfixExpr::op>(lit<"++"> |
                                                      lit<"--">)))> {};

  static constexpr auto UnaryOp =
      lit<"*"> | (lit<"&"> | kw<"bitand">) | lit<"+"> |
      (lit<"~"> | kw<"compl">) | lit<"-"> | (lit<"!"> | kw<"not">);

  struct NewExpression
      : rule<create<ast::NewExpr>(
                 option(flag<&ast::NewExpr::globalScope>(lit<"::">)) +
                 kw<"new"> +
                 option(lit<"("> +
                        some(append<&ast::NewExpr::placement>(
                                 ref<Argument>),
                             lit<",">) +
                        lit<")">) +
                 (assign<&ast::NewExpr::type>(ref<NewTypeId>)  //
                  | (lit<"("> + assign<&ast::NewExpr::type>(ref<TypeId>) +
                     lit<")">)) +
                 option(flag<&ast::NewExpr::hasInit>((lit<"("> +
                         option(some(append<&ast::NewExpr::initArgs>(
                                         ref<Argument>),
                                     lit<",">)) +
                         lit<")">)  //
                        | (flag<&ast::NewExpr::braced>(LBrace) +
                           option(some(append<&ast::NewExpr::initArgs>(
                                           ref<Argument>),
                                       lit<",">) +
                                  option(lit<",">)) +
                           RBrace))))> {};

  struct DeleteExpression
      : rule<create<ast::DeleteExpr>(
                 option(flag<&ast::DeleteExpr::globalScope>(
                     lit<"::">)) +
                 kw<"delete"> +
                 option(flag<&ast::DeleteExpr::isArray>(LBracket) +
                        RBracket) +
                 assign<&ast::DeleteExpr::operand>(ref<CastExpression>))> {};

  struct NoExceptExpr
      : rule<create<ast::NoexceptExpr>(
                 kw<"noexcept"> + lit<"("> +
                 assign<&ast::NoexceptExpr::expr>(ref<Expression>) +
                 lit<")">)> {};

  struct UnaryExpression
      : rule<create<ast::PrefixExpr>(
                 assign<&ast::PrefixExpr::op>(lit<"++"> | lit<"--">) +
                 assign<&ast::PrefixExpr::operand>(ref<CastExpression>))  //
             | create<ast::PrefixExpr>(
                   assign<&ast::PrefixExpr::op>(UnaryOp) +
                   assign<&ast::PrefixExpr::operand>(ref<CastExpression>))  //
             | create<ast::SizeofTypeExpr>(
                   kw<"sizeof"> + lit<"("> +
                   assign<&ast::SizeofTypeExpr::type>(ref<TypeId>) +
                   lit<")">)  //
             | create<ast::SizeofPackExpr>(
                   kw<"sizeof"> + lit<"..."> + lit<"("> +
                   assign<&ast::SizeofPackExpr::name>(ref<ID>) + lit<")">)  //
             | create<ast::SizeofExpr>(
                   kw<"sizeof"> +
                   assign<&ast::SizeofExpr::operand>(ref<UnaryExpression>))  //
             | create<ast::AlignofExpr>(
                   kw<"alignof"> + lit<"("> +
                   assign<&ast::AlignofExpr::type>(ref<TypeId>) + lit<")">)  //
             | ref<NoExceptExpr> | ref<NewExpression> |
             ref<DeleteExpression> | ref<PostfixExpression>> {};

  struct CastExpression
      : rule<create<ast::CastExpr>(
                 lit<"("> + assign<&ast::CastExpr::type>(ref<TypeId>) +
                 lit<")"> +
                 assign<&ast::CastExpr::operand>(ref<CastExpression>))  //
             | ref<UnaryExpression>> {};

  static constexpr auto OpPtrMem = lit<"->*"> | lit<".*">;
  static constexpr auto OpMul = lit<"*"> | lit<"/"> | lit<"%">;
  static constexpr auto OpAdd = lit<"+"> | lit<"-">;

  struct AdditiveLevel
      : infix<CastExpression, ast::BinaryExpr,
              levels(left(OpPtrMem), left(OpMul), left(OpAdd))> {};

  // Reuse adjacent operator rules while keeping individual '>' tokens
  // available to close nested template arguments.
  struct ShiftOp : rule<ref<OpShl> | ref<OpShr>> {};
  struct ShiftExpression
      : infix<AdditiveLevel, ast::BinaryExpr, left(ref<ShiftOp>)> {};

  static constexpr auto OpRel = lit<"<="> | lit<">="> | Lt | Gt;
  static constexpr auto OpEq = lit<"=="> | lit<"!="> | kw<"not_eq">;
  static constexpr auto OpBitAnd = lit<"&"> | kw<"bitand">;
  static constexpr auto OpBitXor = lit<"^"> | kw<"xor">;
  static constexpr auto OpBitOr = lit<"|"> | kw<"bitor">;
  static constexpr auto OpLogAnd = lit<"&&"> | kw<"and">;
  static constexpr auto OpLogOr = lit<"||"> | kw<"or">;

  struct BinaryExpression
      : infix<ShiftExpression, ast::BinaryExpr,
              levels(left(OpRel),     //
                     left(OpEq),      //
                     left(OpBitAnd),  //
                     left(OpBitXor),  //
                     left(OpBitOr),   //
                     left(OpLogAnd),  //
                     left(OpLogOr))> {};

  struct ConditionalExpression
      : rule<create<ast::CondExpr>(
                 assign<&ast::CondExpr::cond>(ref<BinaryExpression>) +
                 lit<"?"> +
                 assign<&ast::CondExpr::thenValue>(ref<Expression>) +
                 lit<":"> +
                 assign<&ast::CondExpr::elseValue>(
                     ref<AssignmentExpression>))  //
             | ref<BinaryExpression>> {};

  static constexpr auto AssignOp =
      lit<"=", !cr<"=">> | lit<"*="> | lit<"/="> | lit<"%="> | lit<"+="> |
      lit<"-="> | lit<">>="> | lit<"<<="> | lit<"&="> | kw<"and_eq"> |
      lit<"^="> | kw<"xor_eq"> | lit<"|="> | kw<"or_eq">;

  struct AssignmentExpression
      : rule<create<ast::AssignExpr>(
                 assign<&ast::AssignExpr::target>(ref<BinaryExpression>) +
                 assign<&ast::AssignExpr::op>(AssignOp) +
                 assign<&ast::AssignExpr::value>(ref<InitializerClause>))  //
             | create<ast::ThrowExpr>(
                   kw<"throw"> +
                   option(assign<&ast::ThrowExpr::value>(
                       ref<AssignmentExpression>)))  //
             | ref<ConditionalExpression>> {};

  struct Expression
      : infix<AssignmentExpression, ast::BinaryExpr, left(lit<",">)> {
  };

  struct ConstantExpression : rule<ref<ConditionalExpression>> {};

  struct PackExpansion
      : rule<create<ast::PackExpansionExpr>(
            assign<&ast::PackExpansionExpr::pattern>(ref<InitializerClause>) + lit<"...">)> {};
  struct Argument : rule<ref<PackExpansion> | ref<InitializerClause>> {};

  struct BracedInitList
      : rule<create<ast::BracedInitList>(
                 LBrace +
                 option(some(append<&ast::BracedInitList::items>(
                                 ref<Argument>),
                             lit<",">) +
                        option(lit<",">)) +
                 RBrace)> {};

  struct InitializerClause
      : rule<ref<AssignmentExpression> | ref<BracedInitList>> {};

  // -- Declarators ----------------------------------------------------

  // An optional trailing `const` follows ANY pointer operator here, not
  // only `*` -- accepted (if nonsensical on a reference) rather than
  // narrowed to the `*` form alone.
  struct PointerOperator
      : rule<create<ast::Pointer>(
                 assign<&ast::Pointer::kind>(lit<"&&"> | kw<"and"> | lit<"&"> |
                                             kw<"bitand">) +
                 Attrs<&ast::Pointer::attributes> +
                 option(append<&ast::Pointer::qualifiers>(kw<"const">)))  //
             | create<ast::Pointer>(
                   option(assign<&ast::Pointer::memberOf>(
                       ref<NestedNameSpecifier>)) +
                   assign<&ast::Pointer::kind>(lit<"*">) +
                   Attrs<&ast::Pointer::attributes> +
                   many(append<&ast::Pointer::qualifiers>(CvQualifierKw)))> {};

  struct ArraySuffix
      : rule<create<ast::ArraySuffix>(
            LBracket +
            option(assign<&ast::ArraySuffix::size>(ref<Expression>)) +
            RBracket + Attrs<&ast::ArraySuffix::attributes>)> {};

  struct ExceptionSpec
      : rule<create<ast::ExceptionSpec>(
                 flag<&ast::ExceptionSpec::dynamic>(kw<"throw">) +
                 lit<"("> +
                 option(some(append<&ast::ExceptionSpec::types>(ref<ExpandedTypeId> | ref<TypeId>),
                             lit<",">)) +
                 lit<")">)  //
             | create<ast::ExceptionSpec>(
                   flag<&ast::ExceptionSpec::isNoexcept>(
                       kw<"noexcept">) +
                   option(lit<"("> +
                          assign<&ast::ExceptionSpec::noexceptExpr>(
                              ref<ConstantExpression>) +
                          lit<")">))> {};

  struct ParametersAndQualifiers
      : rule<create<ast::FunctionSuffix>(
            lit<"("> +
            option((some(append<&ast::FunctionSuffix::params>(ref<Param>),
                         lit<",">) +
                    option(lit<","> + flag<&ast::FunctionSuffix::variadic>(
                                          lit<"...">)))  //
                   | flag<&ast::FunctionSuffix::variadic>(lit<"...">)) +
            lit<")"> +
            many(append<&ast::FunctionSuffix::cvQualifiers>(CvQualifierKw)) +
            option(assign<&ast::FunctionSuffix::refQualifier>(
                lit<"&&"> | kw<"and"> | lit<"&"> | kw<"bitand">)) +
            option(assign<&ast::FunctionSuffix::exceptionSpec>(
                ref<ExceptionSpec>)) +
            Attrs<&ast::FunctionSuffix::attributes>)> {};

  struct DeclaratorSuffix
      : rule<ref<ArraySuffix> | ref<ParametersAndQualifiers>> {};

  struct Param : rule<create<ast::Param>(
                     Attrs<&ast::Param::attributes> +
                     assign<&ast::Param::specifiers>(ref<DeclSpecifierSeq>) +
                     option(assign<&ast::Param::declarator>(
                         ref<Declarator> | ref<UnnamedPackDeclarator> |
                         ref<AbstractDeclarator>)) +
                     option(lit<"="> + assign<&ast::Param::defaultValue>(
                                           ref<InitializerClause>)))> {};

  static constexpr auto TypeIdBody =
      assign<&ast::TypeId::specifiers>(ref<DeclSpecifierSeq>) +
      option(assign<&ast::TypeId::declarator>(ref<AbstractDeclarator>));
  struct TypeId : rule<create<ast::TypeId>(TypeIdBody)> {};
  struct ExpandedTypeId : rule<create<ast::TypeId>(
      TypeIdBody + flag<&ast::TypeId::packExpansion>(lit<"...">))> {};

  // A new-type-id cannot consume constructor parentheses as a function
  // declarator: those parentheses are the new-initializer.
  struct NewDeclarator
      : rule<create<ast::Declarator>(
          (some(append<&ast::Declarator::pointers>(ref<PointerOperator>)) +
           many(append<&ast::Declarator::suffixes>(ref<ArraySuffix>))) |
          some(append<&ast::Declarator::suffixes>(ref<ArraySuffix>)))> {};
  struct NewTypeId : rule<create<ast::TypeId>(
      assign<&ast::TypeId::specifiers>(ref<DeclSpecifierSeq>) +
      option(assign<&ast::TypeId::declarator>(ref<NewDeclarator>)))> {};

  static constexpr auto AbstractTail =
      (lit<"("> + assign<&ast::Declarator::nested>(ref<AbstractDeclarator>) +
       lit<")"> +
       many(append<&ast::Declarator::suffixes>(ref<DeclaratorSuffix>)))  //
      | some(append<&ast::Declarator::suffixes>(ref<DeclaratorSuffix>));

  struct AbstractDeclarator
      : rule<option(flag<&ast::Declarator::pack>(lit<"...">)) +
             ((some(append<&ast::Declarator::pointers>(
                        ref<PointerOperator>)) +
               option(AbstractTail))  //
              | AbstractTail) +
             option(lit<"->"> + assign<&ast::Declarator::trailingReturnType>(
                                     ref<TypeId>))> {};

  struct UnnamedPackDeclarator
      : rule<create<ast::Declarator>(
          many(append<&ast::Declarator::pointers>(ref<PointerOperator>)) +
          flag<&ast::Declarator::pack>(lit<"...">))> {};

  // A member function's own declarator: whether the pure-specifier `= 0` is
  // allowed is checked structurally here instead of by walking the tree
  // afterward -- this rule cannot match a data declarator (`int x`) at
  // all, so `MemberDeclaratorRule`'s
  // pure-specifier branch is only reachable after a function declarator,
  // and forks against the general-declarator branch's initializer reading
  // only where both are possible (`f() = 0`); defined before `Declarator`
  // so that fork resolves here, not there. Deliberately narrower than
  // `Declarator` itself -- no pointer-to-member-function's parenthesised
  // core, no array-then-function chain, no pack -- every one is a shape no
  // real member-function declarator uses, and each still parses via the
  // general branch, just without the pure flag.
  struct FunctionShapedDeclarator
      : rule<many(append<&ast::Declarator::pointers>(ref<PointerOperator>)) +
             assign<&ast::Declarator::name>(ref<IdExpression>) +
             Attrs<&ast::Declarator::attributes> +
             append<&ast::Declarator::suffixes>(ref<ParametersAndQualifiers>) +
             option(lit<"->"> + assign<&ast::Declarator::trailingReturnType>(
                                    ref<TypeId>))> {};

  // The trailing-return-type declarator form folds into an optional
  // trailing `-> TypeId` here rather than a second top-level alternative --
  // over-accepting a leading pointer alongside it, a harmless corner no
  // real declarator uses.
  // An explicit creator fixes the recursive node type before attributes.
  struct Declarator
      : rule<create<ast::Declarator>(
            many(append<&ast::Declarator::pointers>(ref<PointerOperator>)) +
            ((option(flag<&ast::Declarator::pack>(lit<"...">)) +
              assign<&ast::Declarator::name>(ref<IdExpression>) +
              Attrs<&ast::Declarator::attributes>)  //
             | (lit<"("> + assign<&ast::Declarator::nested>(ref<Declarator>) +
                lit<")">)) +
            many(append<&ast::Declarator::suffixes>(ref<DeclaratorSuffix>)) +
            option(lit<"->"> + assign<&ast::Declarator::trailingReturnType>(
                                   ref<TypeId>)))> {};

  // -- Type specifiers --------------------------------------------------

  // A name followed by `::` always opens a nested-name-specifier.
  struct NamedTypeSpec
      : rule<create<ast::NamedType>(
                 assign<&ast::NamedType::name>(Qualified<TheTypeName> |
                                               QualifiedTemplateId) +
                 !lit<"::">)> {};

  struct ElaboratedTypeSpecifier
      : rule<create<ast::ElaboratedType>(
                 assign<&ast::ElaboratedType::keyword>(ClassKeyKw) +
                 Attrs<&ast::ElaboratedType::attributes> +
                 assign<&ast::ElaboratedType::name>(
                     Qualified<PlainIdent> | Qualified<SimpleTemplateId> |
                     QualifiedTemplateId))  //
             | create<ast::ElaboratedType>(
                   assign<&ast::ElaboratedType::keyword>(kw<"enum">) +
                   assign<&ast::ElaboratedType::name>(Qualified<PlainIdent>))> {
  };

  struct TypeNameSpecifier
      : rule<create<ast::TypenameType>(
                 kw<"typename"> +
                 assign<&ast::TypenameType::name>(create<ast::QualifiedName>(
                     assign<&ast::QualifiedName::qualifier>(
                         ref<NestedNameSpecifier>) +
                     assign<&ast::QualifiedName::id>(
                         ref<PlainIdent> |
                         (option(kw<"template">) +
                          ref<SimpleTemplateId>)))))> {};

  struct DecltypeSpecifier
      : rule<create<ast::DecltypeType>(
                 kw<"decltype"> + lit<"("> +
                 (flag<&ast::DecltypeType::isAuto>(kw<"auto">)  //
                  | assign<&ast::DecltypeType::expr>(ref<Expression>)) +
                 lit<")">)> {};

  struct BuiltinTypeSpecifier
      : rule<some(append<&ast::BuiltinType::keywords>(BuiltinTypeWord))> {};

  static constexpr auto DeclQualifier =
      append<&ast::Specifiers::qualifiers>(Qualifier);
  static constexpr auto TypeSpecifier =
      ref<BuiltinTypeSpecifier> | ref<ClassSpecifier> |
      ref<EnumSpecifier> | ref<ElaboratedTypeSpecifier> |
      ref<TypeNameSpecifier> | ref<DecltypeSpecifier> | ref<NamedTypeSpec>;

  // Pin the node type explicitly; attributes follow the complete sequence.
  struct DeclSpecifierSeq
      : rule<create<ast::Specifiers>(
                 (many(DeclQualifier) +
                  append<&ast::Specifiers::types>(TypeSpecifier) +
                  many(DeclQualifier + option(append<&ast::Specifiers::types>(
                                           ref<BuiltinTypeSpecifier>)))) |
                 some(DeclQualifier)) +
             Attrs<&ast::Specifiers::attributes>> {};

  // -- Classes ----------------------------------------------------------

  static constexpr auto AccessSpecKw =
      kw<"public"> | kw<"protected"> | kw<"private">;
  static constexpr auto BaseTypeExpr =
      Qualified<TheTypeName> | ref<DecltypeSpecifier>;

  struct BaseSpecifier
      : rule<create<ast::BaseSpecifier>(
            Attrs<&ast::BaseSpecifier::attributes> +
            ((option(assign<&ast::BaseSpecifier::accessSpec>(AccessSpecKw)) +
              option(flag<&ast::BaseSpecifier::isVirtual>(kw<"virtual">)))  //
             |
             (flag<&ast::BaseSpecifier::isVirtual>(kw<"virtual">) +
              option(assign<&ast::BaseSpecifier::accessSpec>(AccessSpecKw)))) +
            assign<&ast::BaseSpecifier::type>(BaseTypeExpr) +
            option(flag<&ast::BaseSpecifier::pack>(lit<"...">)))> {};

  struct ClassSpecifier
      : rule<create<ast::ClassType>(
            assign<&ast::ClassType::keyword>(ClassKeyKw) +
            Attrs<&ast::ClassType::attributes> +
            option(assign<&ast::ClassType::name>(Qualified<TheTypeName>) +
                   option(flag<&ast::ClassType::isFinal>(kw<"final">))) +
            option(lit<":"> +
                   some(append<&ast::ClassType::bases>(ref<BaseSpecifier>),
                        lit<",">)) +
            flag<&ast::ClassType::defined>(LBrace) +
            many(append<&ast::ClassType::members>(ref<AccessLabel> |
                                                  ref<MemberDeclaration>)) +
            RBrace)> {};

  struct AccessLabel
      : rule<create<ast::AccessLabel>(
                 assign<&ast::AccessLabel::keyword>(AccessSpecKw) +
                 lit<":">)> {};

  // Tried first: a function-shaped declarator followed by `= 0` reads as
  // pure, ordered choice's answer for the fork against the general branch's
  // `= expr` on the same input (see FunctionShapedDeclarator).
  struct MemberDeclaratorRule
      : rule<(assign<&ast::MemberDeclarator::declarator>(
                  ref<FunctionShapedDeclarator>) +
              many(append<&ast::MemberDeclarator::virtualSpecifiers>(
                  kw<"override"> | kw<"final">)) +
              flag<&ast::MemberDeclarator::isPure>(lit<"=">) +
              assign<&ast::MemberDeclarator::init>(create<ast::IntLit>(
                  assign<&ast::IntLit::text>(ref<INT_LIT>))))  //
             | (assign<&ast::MemberDeclarator::declarator>(ref<Declarator>) +
                option(some(append<&ast::MemberDeclarator::virtualSpecifiers>(
                           kw<"override"> | kw<"final">))  //
                       | (lit<"="> + assign<&ast::MemberDeclarator::init>(
                                         ref<InitializerClause>))  //
                       | assign<&ast::MemberDeclarator::init>(
                             ref<BracedInitList>)))  //
             | (option(assign<&ast::MemberDeclarator::declarator>(
                    create<ast::Declarator>(
                        assign<&ast::Declarator::name>(ref<PlainIdent>)))) +
                Attrs<&ast::MemberDeclarator::attributes> + lit<":"> +
                assign<&ast::MemberDeclarator::bitWidth>(
                    ref<ConstantExpression>))> {};

  struct StaticAssertDecl
      : rule<create<ast::StaticAssertDecl>(
                 kw<"static_assert"> + lit<"("> +
                 assign<&ast::StaticAssertDecl::cond>(
                     ref<ConstantExpression>) +
                 option(lit<","> + assign<&ast::StaticAssertDecl::message>(
                                       ref<STRING_LIT>)) +
                 lit<")"> + lit<";">)> {};

  struct AliasDeclaration
      : rule<create<ast::AliasDecl>(
            kw<"using"> + assign<&ast::AliasDecl::name>(ref<ID>) +
            Attrs<&ast::AliasDecl::attributes> + lit<"="> +
            assign<&ast::AliasDecl::type>(ref<TypeId>) + lit<";">)> {};

  struct UsingDecl
      : rule<create<ast::UsingDecl>(
                 kw<"using"> +
                 ((option(flag<&ast::UsingDecl::typenameKw>(
                       kw<"typename">)) +
                   assign<&ast::UsingDecl::name>(
                       Qualified<UnqualifiedId>))  //
                  | (lit<"::"> +
                     assign<&ast::UsingDecl::name>(ref<UnqualifiedId>))) +
                 lit<";">)> {};

  struct EmptyDecl : rule<create<ast::EmptyDecl>(lit<";">)> {};

  struct PlainMemberDecl
      : rule<create<ast::MemberDecl>(
            Attrs<&ast::MemberDecl::attributes> +
            option(
                assign<&ast::MemberDecl::specifiers>(ref<DeclSpecifierSeq>)) +
            option(some(append<&ast::MemberDecl::declarators>(
                            ref<MemberDeclaratorRule>),
                        lit<",">)) +
            lit<";">)> {};

  struct MemberDeclaration
      : rule<ref<StaticAssertDecl> | ref<TemplateDecl> |
             ref<AliasDeclaration> | ref<UsingDecl> |
             ref<EmptyDecl> | ref<FunctionDefinition> |
             ref<PlainMemberDecl>> {};

  // -- Enums --------------------------------------------------------

  static constexpr auto EnumKey =
      kw<"enum"> +
      option(flag<&ast::EnumType::isClass>(kw<"class"> | kw<"struct">));

  // C++14 enumerators are identifiers; attributes belong to the enum head.
  struct Enumerator : rule<assign<&ast::Enumerator::name>(ref<ID>) +

                           option(lit<"="> + assign<&ast::Enumerator::value>(
                                                 ref<ConstantExpression>))> {};

  struct EnumSpecifier
      : rule<create<ast::EnumType>(
            EnumKey + Attrs<&ast::EnumType::attributes> +
            option(assign<&ast::EnumType::name>(Qualified<PlainIdent>)) +
            option(lit<":"> +
                   assign<&ast::EnumType::underlying>(ref<DeclSpecifierSeq>)) +
            flag<&ast::EnumType::defined>(LBrace) +
            option(some(append<&ast::EnumType::enumerators>(ref<Enumerator>),
                        lit<",">) +
                   option(lit<",">)) +
            RBrace)> {};

  struct OpaqueEnumDecl
      : rule<create<ast::Declaration>(
            assign<&ast::Declaration::specifiers>(create<ast::Specifiers>(
                append<&ast::Specifiers::types>(create<ast::EnumType>(
                    EnumKey + Attrs<&ast::EnumType::attributes> +
                    assign<&ast::EnumType::name>(ref<PlainIdent>) +
                    option(lit<":"> + assign<&ast::EnumType::underlying>(
                                          ref<DeclSpecifierSeq>)))))) +
            lit<";">)> {};

  // -- Statements -------------------------------------------------------

  static constexpr auto ExpressionStatementBody = create<ast::ExprStmt>(
      option(assign<&ast::ExprStmt::value>(ref<Expression>)) + lit<";">);
  struct ExpressionStatement : rule<ExpressionStatementBody> {};
  struct AttributedExpressionStatement
      : rule<Attrs<&ast::Item::attributes> + ExpressionStatementBody> {};

  static constexpr auto CompoundStatementBody =
      LBrace + many(append<&ast::CompoundStmt::items>(ref<Statement>)) + RBrace;
  struct CompoundStatement : rule<CompoundStatementBody> {};
  struct AttributedCompoundStatement
      : rule<Attrs<&ast::Item::attributes> + CompoundStatementBody> {};

  struct ConditionDecl
      : rule<create<ast::Condition>(
            Attrs<&ast::Condition::attributes> +
            assign<&ast::Condition::specifiers>(ref<DeclSpecifierSeq>) +
            assign<&ast::Condition::declarator>(ref<Declarator>) +
            ((lit<"="> +
              assign<&ast::Condition::init>(ref<InitializerClause>))  //
             | assign<&ast::Condition::init>(ref<BracedInitList>)))> {};
  struct Condition : rule<ref<ConditionDecl> | ref<Expression>> {};

  struct IfStatement
      : rule<Attrs<&ast::Item::attributes> +
             (create<ast::IfStmt>(
                 kw<"if"> + lit<"("> +
                 assign<&ast::IfStmt::cond>(ref<Condition>) + lit<")"> +
                 assign<&ast::IfStmt::thenBranch>(ref<Statement>) +
                 option(commit(kw<"else">, assign<&ast::IfStmt::elseBranch>(
                                               ref<Statement>)))))> {};

  struct SwitchStatement
      : rule<Attrs<&ast::Item::attributes> +
             (create<ast::SwitchStmt>(
                 kw<"switch"> + lit<"("> +
                 assign<&ast::SwitchStmt::cond>(ref<Condition>) + lit<")"> +
                 assign<&ast::SwitchStmt::body>(ref<Statement>)))> {};

  struct SelectionStatement
      : rule<ref<IfStatement> | ref<SwitchStatement>> {};

  struct WhileStatement
      : rule<Attrs<&ast::Item::attributes> +
             (create<ast::WhileStmt>(
                 kw<"while"> + lit<"("> +
                 assign<&ast::WhileStmt::cond>(ref<Condition>) + lit<")"> +
                 assign<&ast::WhileStmt::body>(ref<Statement>)))> {};

  struct DoStatement
      : rule<Attrs<&ast::Item::attributes> +
             (create<ast::DoStmt>(kw<"do"> +
                                  assign<&ast::DoStmt::body>(ref<Statement>) +
                                  kw<"while"> + lit<"("> +
                                  assign<&ast::DoStmt::cond>(ref<Expression>) +
                                  lit<")"> + lit<";">))> {};

  struct ForInitDecl
      : rule<create<ast::Declaration>(
            Attrs<&ast::Item::attributes> +
            assign<&ast::Declaration::specifiers>(ref<DeclSpecifierSeq>) +
            some(append<&ast::Declaration::declarators>(ref<InitDeclarator>),
                 lit<",">) +
            lit<";">)> {};
  struct ForInitStatement
      : rule<ref<ForInitDecl> | ref<ExpressionStatement>> {};

  struct ForStatement
      : rule<Attrs<&ast::Item::attributes> +
             (create<ast::ForStmt>(
                 kw<"for"> + lit<"("> +
                 assign<&ast::ForStmt::init>(ref<ForInitStatement>) +
                 option(assign<&ast::ForStmt::cond>(ref<Expression>)) +
                 lit<";"> +
                 option(assign<&ast::ForStmt::step>(ref<Expression>)) +
                 lit<")"> + assign<&ast::ForStmt::body>(ref<Statement>)))> {};

  struct ForRangeStatement
      : rule<Attrs<&ast::Item::attributes> +
             (create<ast::ForRangeStmt>(
                 kw<"for"> + lit<"("> +
                 Attrs<&ast::ForRangeStmt::rangeAttributes> +
                 assign<&ast::ForRangeStmt::specifiers>(ref<DeclSpecifierSeq>) +
                 assign<&ast::ForRangeStmt::declarator>(ref<Declarator>) +
                 lit<":"> +
                 assign<&ast::ForRangeStmt::range>(ref<Expression> |
                                                   ref<BracedInitList>) +
                 lit<")"> +
                 assign<&ast::ForRangeStmt::body>(ref<Statement>)))> {};

  struct IterationStatement
      : rule<ref<WhileStatement> | ref<DoStatement> | ref<ForStatement> |
             ref<ForRangeStatement>> {};

  struct JumpStatement
      : rule<Attrs<&ast::Item::attributes> +
             (create<ast::BreakStmt>(kw<"break"> + lit<";">)          //
              | create<ast::ContinueStmt>(kw<"continue"> + lit<";">)  //
              | create<ast::ReturnStmt>(
                    kw<"return"> +
                    option(assign<&ast::ReturnStmt::value>(
                        ref<Expression> | ref<BracedInitList>)) +
                    lit<";">)  //
              | create<ast::GotoStmt>(kw<"goto"> +
                                      assign<&ast::GotoStmt::target>(ref<ID>) +
                                      lit<";">))> {};

  struct LabeledStatement
      : rule<create<ast::CaseLabel>(
                 Attrs<&ast::Item::attributes> + kw<"case"> +
                 assign<&ast::CaseLabel::value>(ref<ConstantExpression>) +
                 lit<":"> + assign<&ast::CaseLabel::body>(ref<Statement>))  //
             | create<ast::DefaultLabel>(
                   Attrs<&ast::Item::attributes> + kw<"default"> + lit<":"> +
                   assign<&ast::DefaultLabel::body>(ref<Statement>))  //
             |
             create<ast::Label>(Attrs<&ast::Item::attributes> +
                                assign<&ast::Label::name>(ref<ID>) + lit<":"> +
                                assign<&ast::Label::body>(ref<Statement>))> {};

  struct MemInit
      : rule<assign<&ast::MemInit::target>(BaseTypeExpr) +
             ((lit<"("> +
               option(some(append<&ast::MemInit::args>(
                               ref<Argument>),
                           lit<",">)) +
               lit<")">)  //
              | (flag<&ast::MemInit::braced>(LBrace) +
                 option(some(append<&ast::MemInit::args>(
                                 ref<Argument>),
                             lit<",">) +
                        option(lit<",">)) +
                 RBrace)) +
             option(flag<&ast::MemInit::pack>(lit<"...">))> {};

  struct CatchClause
      : rule<create<ast::CatchClause>(
            kw<"catch"> + lit<"("> +
            (flag<&ast::CatchClause::catchAll>(lit<"...">)  //
             | (Attrs<&ast::CatchClause::attributes> +
                assign<&ast::CatchClause::specifiers>(ref<DeclSpecifierSeq>) +
                option(assign<&ast::CatchClause::declarator>(
                    ref<Declarator> | ref<AbstractDeclarator>)))) +
            lit<")"> +
            assign<&ast::CatchClause::body>(ref<CompoundStatement>))> {};

  struct TryBlock
      : rule<Attrs<&ast::Item::attributes> +
             (create<ast::TryStmt>(
                 kw<"try"> +
                 assign<&ast::TryStmt::body>(ref<CompoundStatement>) +
                 some(append<&ast::TryStmt::handlers>(ref<CatchClause>))))> {};

  struct StatementCore
      : rule<ref<LabeledStatement> | ref<BlockDeclaration> |
             ref<AttributedCompoundStatement> | ref<SelectionStatement> |
             ref<IterationStatement> | ref<JumpStatement> | ref<TryBlock> |
             ref<AttributedExpressionStatement>> {};
  struct Statement : rule<ref<StatementCore>> {};
  ;

  // -- Declarations -------------------------------------------------

  struct InitDeclarator
      : rule<assign<&ast::InitDeclarator::declarator>(ref<Declarator>) +
             option((lit<"("> +
                     some(append<&ast::InitDeclarator::directInit>(
                              ref<Argument>),
                          lit<",">) +
                     lit<")">)  //
                    | (lit<"="> + assign<&ast::InitDeclarator::init>(
                                       ref<InitializerClause>))  //
                    | assign<&ast::InitDeclarator::init>(
                          ref<BracedInitList>))> {};

  // `int;` alone with no declarator IS accepted (an empty declarator list
  // is legal); a declarator with no specifiers at all is not -- that
  // reading falls through to an expression statement instead, since it has
  // no meaning in real C++ and no example needs the finer disambiguation.
  struct SimpleDeclaration
      : rule<create<ast::Declaration>(
            Attrs<&ast::Declaration::attributes> +
            assign<&ast::Declaration::specifiers>(ref<DeclSpecifierSeq>) +
            option(some(
                append<&ast::Declaration::declarators>(ref<InitDeclarator>),
                lit<",">)) +
            lit<";">)> {};

  struct AsmDecl
      : rule<create<ast::AsmDecl>(
                 kw<"asm"> + lit<"("> +
                 assign<&ast::AsmDecl::code>(ref<STRING_LIT>) + lit<")"> +
                 lit<";">)> {};

  struct NamespaceAliasDecl
      : rule<create<ast::NamespaceAliasDecl>(
                 kw<"namespace"> +
                 assign<&ast::NamespaceAliasDecl::name>(ref<ID>) + lit<"="> +
                 assign<&ast::NamespaceAliasDecl::target>(
                     Qualified<NamespaceName>) +
                 lit<";">)> {};

  struct UsingDirective
      : rule<create<ast::UsingDirective>(
            Attrs<&ast::UsingDirective::attributes> + kw<"using"> +
            kw<"namespace"> +
            assign<&ast::UsingDirective::name>(Qualified<NamespaceName>) +
            lit<";">)> {};

  struct BlockDeclaration
      : rule<ref<SimpleDeclaration> | ref<AsmDecl> |
             ref<NamespaceAliasDecl> | ref<UsingDecl> |
             ref<UsingDirective> | ref<StaticAssertDecl> |
             ref<AliasDeclaration> | ref<OpaqueEnumDecl>> {};

  struct AttributeDecl
      : rule<create<ast::AttributeDecl>(
            some(append<&ast::Item::attributes>(AttributeSpec)) + lit<";">)> {};

  struct LinkageSpec
      : rule<create<ast::LinkageSpec>(
                 kw<"extern"> +
                 assign<&ast::LinkageSpec::lang>(ref<STRING_LIT>) +
                 ((LBrace +
                   many(append<&ast::LinkageSpec::items>(ref<Declaration>)) +
                   RBrace)  //
                  | append<&ast::LinkageSpec::items>(
                        ref<Declaration>)))> {};

  struct NamespaceDecl
      : rule<create<ast::NamespaceDecl>(
                 option(flag<&ast::NamespaceDecl::isInline>(
                     kw<"inline">)) +
                 kw<"namespace"> +
                 option(assign<&ast::NamespaceDecl::name>(ref<ID>)) +
                 LBrace +
                 many(append<&ast::NamespaceDecl::items>(ref<Declaration>)) +
                 RBrace)> {};

  struct FunctionDefinition
      : rule<Attrs<&ast::FunctionDef::attributes> +
             create<ast::FunctionDef>(

                 option(assign<&ast::FunctionDef::specifiers>(
                     ref<DeclSpecifierSeq>)) +
                 assign<&ast::FunctionDef::declarator>(ref<Declarator>) +
                 many(append<&ast::FunctionDef::virtualSpecifiers>(
                     kw<"override"> | kw<"final">)) +
                 ((option(lit<":"> + some(append<&ast::FunctionDef::ctorInit>(
                                              ref<MemInit>),
                                          lit<",">)) +
                   assign<&ast::FunctionDef::body>(ref<CompoundStatement>))  //
                  | (kw<"try"> +
                     option(lit<":"> + some(append<&ast::FunctionDef::ctorInit>(
                                                ref<MemInit>),
                                            lit<",">)) +
                     assign<&ast::FunctionDef::body>(ref<CompoundStatement>) +
                     some(append<&ast::FunctionDef::handlers>(
                         ref<CatchClause>)))  //
                  | (lit<"="> +
                     (flag<&ast::FunctionDef::isDefault>(kw<"default">)  //
                      | flag<&ast::FunctionDef::isDelete>(kw<"delete">)) +
                     lit<";">)))> {};

  // -- Templates ------------------------------------------------------

  struct ParamTemplateParam
      : rule<assign<&ast::ParamTemplateParam::param>(ref<Param>)> {};

  struct TypeTemplateParam
      : rule<create<ast::TypeTemplateParam>(
                 ((kw<"template"> + Lt +
                   some(append<&ast::TypeTemplateParam::templateParams>(
                            ref<TemplateParameter>),
                        lit<",">) +
                   Gt + (kw<"class"> | kw<"typename">))  //
                  | (kw<"class"> | kw<"typename">)) +
                 option(flag<&ast::TypeTemplateParam::pack>(
                     lit<"...">)) +
                 option(assign<&ast::TypeTemplateParam::name>(ref<ID>)) +
                 option(lit<"="> +
                        assign<&ast::TypeTemplateParam::defaultType>(
                            ref<TypeId>)))> {};

  struct TemplateParameter
      : rule<ref<TypeTemplateParam> | ref<ParamTemplateParam>> {};

  struct TemplateDecl
      : rule<create<ast::TemplateDecl>(
                 kw<"template"> + Lt +
                 some(append<&ast::TemplateDecl::params>(
                          ref<TemplateParameter>),
                      lit<",">) +
                 Gt + assign<&ast::TemplateDecl::body>(ref<Declaration>))> {};

  struct ExplicitInstantiation
      : rule<create<ast::ExplicitInstantiation>(
                 option(flag<&ast::ExplicitInstantiation::isExtern>(
                     kw<"extern">)) +
                 kw<"template"> +
                 assign<&ast::ExplicitInstantiation::decl>(
                     ref<Declaration>))> {};

  struct ExplicitSpecialization
      : rule<create<ast::ExplicitSpecialization>(
                 kw<"template"> + Lt + Gt +
                 assign<&ast::ExplicitSpecialization::decl>(
                     ref<Declaration>))> {};

  struct Declaration
      : rule<ref<BlockDeclaration> | ref<FunctionDefinition> |
             ref<TemplateDecl> | ref<ExplicitInstantiation> |
             ref<ExplicitSpecialization> | ref<LinkageSpec> |
             ref<NamespaceDecl> | ref<EmptyDecl> |
             ref<AttributeDecl>> {};

  struct TranslationUnit
      : rule<many(append<&ast::TranslationUnit::items>(ref<Declaration>))> {
  };

  using entry = TranslationUnit;
  static constexpr auto word_boundary = uc::XID_Continue | LegacyID;
  using ignored = list<WS, Directive>;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace cpp
