// Swift 5.4 core with selected later syntax. Binary operators stay flat;
// declarations of operators and precedence groups require semantic resolution.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "swift/core/ast.hpp"
#include "cursiv/core/text/scalar_at.hpp"

namespace swift {

// Swift 6.2 raw-identifier exclusions (SE-0451 and the native lexer).
inline constexpr bool raw_operator_start(char32_t c) {
  if (c < 0x80) return std::string_view("/=-+*%<>!&|^~.?").find(static_cast<char>(c)) != std::string_view::npos;
  return (c >= 0xA1 && c <= 0xA7) || c == 0xA9 || c == 0xAB || c == 0xAC || c == 0xAE ||
      c == 0xB0 || c == 0xB1 || c == 0xB6 || c == 0xBB || c == 0xBF || c == 0xD7 || c == 0xF7 ||
      c == 0x2016 || c == 0x2017 || (c >= 0x2020 && c <= 0x2027) ||
      (c >= 0x2030 && c <= 0x203E) || (c >= 0x2041 && c <= 0x2053) ||
      (c >= 0x2055 && c <= 0x205E) || (c >= 0x2190 && c <= 0x23FF) ||
      (c >= 0x2500 && c <= 0x2775) || (c >= 0x2794 && c <= 0x2BFF) ||
      (c >= 0x2E00 && c <= 0x2E7F) || (c >= 0x3001 && c <= 0x3003) ||
      (c >= 0x3008 && c <= 0x3030);
}
inline constexpr bool raw_operator_continue(char32_t c) {
  return raw_operator_start(c) || (c >= 0x300 && c <= 0x36F) ||
      (c >= 0x1DC0 && c <= 0x1DFF) || (c >= 0x20D0 && c <= 0x20FF) ||
      (c >= 0xFE00 && c <= 0xFE0F) || (c >= 0xFE20 && c <= 0xFE2F) ||
      (c >= 0xE0100 && c <= 0xE01EF);
}
inline constexpr auto identifier_text = [](std::string_view text)
    -> cursiv::ConversionResult<std::string_view> {
  using Result = cursiv::ConversionResult<std::string_view>;
  if (!text.starts_with('`')) return Result::ok(text);
  text = text.substr(1, text.size() - 2);
  bool all_space = true, all_operator = true;
  for (std::size_t i = 0; i < text.size();) {
    std::size_t width = 0;
    const auto c = cursiv::scalar_at(text.data() + i, text.size() - i, width);
    if (!width || c < 0x20 || c == 0x7F || c == '`' || c == '\\' ||
        c == 0x85 || c == 0xA0 || c == 0x1680 || (c >= 0x2000 && c <= 0x200A) ||
        c == 0x2028 || c == 0x2029 || c == 0x202F || c == 0x205F || c == 0x3000)
      return Result::err("Forbidden character in a raw identifier");
    all_space &= c == 0x20 || c == 0x200E || c == 0x200F;
    all_operator &= i == 0 ? raw_operator_start(c) : raw_operator_continue(c);
    i += width;
  }
  if (all_space || all_operator) return Result::err("A raw identifier cannot be entirely whitespace or an operator");
  return Result::ok(text);
};

struct Swift : cursiv::GrammarSpec {
  static constexpr auto IdentifierChar = cr<"a-zA-Z_0-9$\u00A8\u00AA\u00AD\u00AF\u00B2-\u00B5\u00B7-\u00BA\u00BC-\u00BE"
      "\u00C0-\u00D6\u00D8-\u00F6\u00F8-\u00FF\u0100-\u167F\u1681-\u180D\u180F-\u1FFF"
      "\u200B-\u200D\u202A-\u202E\u203F-\u2040\u2054\u2060-\u206F\u2070-\u218F"
      "\u2460-\u24FF\u2776-\u2793\u2C00-\u2DFF\u2E80-\u2FFF\u3004-\u3007\u3021-\u302F"
      "\u3031-\u303F\u3040-\uD7FF\uF900-\uFD3D\uFD40-\uFDCF\uFDF0-\uFE44\uFE47-\uFFF8"
      "\U00010000-\U0001FFFD\U00020000-\U0002FFFD\U00030000-\U0003FFFD"
      "\U00040000-\U0004FFFD\U00050000-\U0005FFFD\U00060000-\U0006FFFD"
      "\U00070000-\U0007FFFD\U00080000-\U0008FFFD\U00090000-\U0009FFFD"
      "\U000A0000-\U000AFFFD\U000B0000-\U000BFFFD\U000C0000-\U000CFFFD"
      "\U000D0000-\U000DFFFD\U000E0000-\U000EFFFD">;
  static constexpr auto IdentifierHead =
      !cr<"0-9$\u0300-\u036F\u1DC0-\u1DFF\u20D0-\u20FF\uFE20-\uFE2F\uFEFF"> + IdentifierChar;
  static constexpr auto word_boundary = IdentifierChar;
  static constexpr auto eos = implied(lit<";">, gap::line | &lit<"}"> | eof);

  // -- Trivia -------------------------------------------------------------

  struct WS : terminal<some(cr<R"( \n\r\t\v\f\x00)">)> {};
  struct HASHBANG : terminal<lit<"#!"> + many(cr<R"(^\r\n)">)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT
      : terminal<lit<"/*"> +
                 many(ref<ML_COMMENT> | (!lit<"/*"> + !lit<"*/"> + any)) +
                 lit<"*/">> {};

  // -- Identifiers and keywords -------------------------------------------

  // Identifier_head Identifier_characters?, or the '$'-prefixed implicit
  // closure parameter / property-wrapper projection forms (both admit the
  // same Identifier_characters* run once digits are inside IdentifierChar).
  static constexpr auto IdentifierWord =
      (IdentifierHead + many(IdentifierChar)) | (lit<"$"> + some(IdentifierChar));
  static constexpr auto EscapedWord = lit<"`"> + some(cr<R"(^`\\\r\n)">) + lit<"`">;
  struct ID : terminal<IdentifierWord> {};


  struct Keyword
      : rule<kw<"associatedtype"> | kw<"class"> | kw<"deinit"> | kw<"enum"> |
             kw<"extension"> | kw<"fileprivate"> | kw<"func"> | kw<"import"> |
             kw<"init"> | kw<"inout"> | kw<"internal"> | kw<"let"> |
             kw<"open"> | kw<"operator"> | kw<"private"> | kw<"protocol"> |
             kw<"public"> | kw<"rethrows"> | kw<"static"> | kw<"struct"> |
             kw<"subscript"> | kw<"typealias"> | kw<"var"> | kw<"break"> |
             kw<"case"> | kw<"continue"> | kw<"default"> | kw<"defer"> |
             kw<"do"> | kw<"else"> | kw<"fallthrough"> | kw<"for"> |
             kw<"guard"> | kw<"if"> | kw<"in"> | kw<"repeat"> | kw<"return"> |
             kw<"switch"> | kw<"where"> | kw<"while"> | kw<"as"> | kw<"Any"> |
             kw<"catch"> | kw<"false"> | kw<"is"> | kw<"nil"> | kw<"super"> |
             kw<"self"> | kw<"Self"> | kw<"throw"> | kw<"throws"> |
             kw<"true"> | kw<"try"> | lit<"_"> | kw<"#available"> |
             kw<"#colorLiteral"> | kw<"#column"> | kw<"#else"> |
             kw<"#elseif"> | kw<"#endif"> | kw<"#error"> | kw<"#file"> |
             kw<"#fileID"> | kw<"#fileLiteral"> | kw<"#filePath"> |
             kw<"#function"> | kw<"#if"> | kw<"#imageLiteral"> | kw<"#line"> |
             kw<"#selector"> | kw<"#sourceLocation"> | kw<"#warning">> {};

  // The contextual keyword set: each may still name a declaration
  // everywhere Swift allows it.
  static constexpr auto ReservedReferenceKeyword =
      kw<"class"> | kw<"struct"> | kw<"protocol"> | kw<"for"> | kw<"do"> |
      kw<"guard"> | kw<"while"> | kw<"public"> | kw<"private"> |
      kw<"internal"> | kw<"as"> | kw<"catch"> | kw<"in"> | kw<"where"> |
      kw<"default"> | kw<"operator"> | kw<"self"> | kw<"super">;
  struct ESCAPED_ID : terminal<EscapedWord> {};
  static constexpr auto EscapedIdentifier = ref<ESCAPED_ID>;
  static constexpr auto ContextualKeyword =
      kw<"Self"> | kw<"open"> | kw<"get"> | kw<"set"> |
      kw<"willSet"> | kw<"didSet"> | kw<"associativity"> | kw<"convenience"> |
      kw<"dynamic"> | kw<"final"> | kw<"indirect"> | kw<"infix"> | kw<"lazy"> |
      kw<"left"> | kw<"mutating"> | kw<"none"> | kw<"nonmutating"> |
      kw<"optional"> | kw<"override"> | kw<"postfix"> | kw<"precedence"> |
      kw<"prefix"> | kw<"required"> | kw<"right"> | kw<"type"> | kw<"unowned"> |
      kw<"weak"> | kw<"alpha"> | kw<"arch"> | kw<"arm"> | kw<"arm64"> |
      kw<"assignment"> | kw<"blue"> | kw<"canImport"> | kw<"compiler"> |
      kw<"file"> | kw<"green"> | kw<"higherThan"> | kw<"i386"> | kw<"iOS"> |
      kw<"OSX"> | kw<"iOSApplicationExtension"> | kw<"line"> | kw<"lowerThan"> |
      kw<"macCatalyst"> | kw<"macCatalystApplicationExtension"> | kw<"macOS"> |
      kw<"macOSApplicationExtension"> | kw<"os"> | kw<"precedencegroup"> |
      kw<"red"> | kw<"resourceName"> | kw<"safe"> | kw<"simulator"> |
      kw<"some"> | kw<"swift"> | kw<"targetEnvironment"> | kw<"tvOS"> |
      kw<"unsafe"> | kw<"watchOS"> | kw<"x86_64"> |
      kw<"async"> | kw<"reasync"> | kw<"await"> | kw<"actor"> | kw<"any"> |
      kw<"each"> | kw<"copy"> | kw<"consume"> | kw<"borrowing"> | kw<"consuming"> |
      kw<"isolated"> | kw<"nonisolated"> | kw<"distributed"> | kw<"sending">;
  struct Identifier : rule<as<identifier_text>(
                          ref<ID> | ReservedReferenceKeyword |
                          ContextualKeyword | EscapedIdentifier | lit<"_">)> {};

  // Reference names exclude only reserved declaration/statement heads.
  // Argument labels, members and declaration names still use Identifier.
  struct ReferenceID : terminal<!ReservedReferenceKeyword +
                                ((IdentifierHead + many(IdentifierChar)) |
                                 (lit<"$"> + some(IdentifierChar)))> {};
  struct ReferenceIdentifier
      : rule<as<identifier_text>(ref<ReferenceID> | ContextualKeyword |
                                 EscapedIdentifier | lit<"_">)> {};

  // -- Literals -------------------------------------------------------

  struct DECIMAL_DIGITS : terminal<some(d)> {};
  struct INT_LIT
      : terminal<(lit<"0b"> + cr<"01"> + many(cr<"01_">)) |
                 (lit<"0o"> + cr<"0-7"> + many(cr<"0-7_">)) |
                 (lit<"0x"> + cr<"0-9a-fA-F"> + many(cr<"0-9a-fA-F_">)) |
                 (d + many(cr<"0-9_">))> {};
  static constexpr auto FloatExp =
      cr<"eE"> + option(cr<"+-">) + d + many(cr<"0-9_">);
  static constexpr auto HexFloatExp =
      cr<"pP"> + option(cr<"+-">) + d + many(cr<"0-9_">);
  struct FLOAT_LIT
      : terminal<(d + many(cr<"0-9_">) + lit<"."> + d + many(cr<"0-9_">) +
                  option(FloatExp)) |
                 (d + many(cr<"0-9_">) + FloatExp) |
                 (lit<"0x"> + cr<"0-9a-fA-F"> + many(cr<"0-9a-fA-F_">) +
                  option(lit<"."> + cr<"0-9a-fA-F"> + many(cr<"0-9a-fA-F_">)) +
                  HexFloatExp)> {};

  static constexpr auto EscapeSeq = lit<"\\"> + many(lit<"#">);
  static constexpr auto EscapedChar = EscapeSeq + cr<R"(^(\r\n)">;
  static constexpr auto EscapedNewline =
      EscapeSeq + many(cr<R"( \t)">) + (eol);

  struct StringOpen : terminal<lit<"\"">> {};
  struct StringClose : terminal<lit<"\"">> {};
  struct MultiStringOpen : terminal<lit<"\"\"\"">> {};
  struct MultiStringClose : terminal<lit<"\"\"\"">> {};
  struct InterpOpen : terminal<lit<"\\(">> {};
  struct QuotedText : terminal<some(EscapedChar | cr<R"(^"\\\r\n)">)> {};
  // One unit per token -- not one more `some()` around the whole
  // alternation, or a run of two closing quotes would fold into this
  // instead of leaving the third for `MultiStringClose` to win by length.
  struct MultiQuotedText
      : terminal<EscapedChar | EscapedNewline | some(cr<R"(^\\")">) |
                 (lit<"\""> + option(lit<"\"">))> {};

  struct RawSingleHashes {};
  struct RawMultiHashes {};
  struct RawMultiString
      : terminal<mark<RawMultiHashes>(some(lit<"#">)) + lit<"\"\"\""> +
          many((lit<"\\"> + same<RawMultiHashes> + any) |
               (!(lit<"\"\"\""> + same<RawMultiHashes>) + any)) +
          lit<"\"\"\""> + same<RawMultiHashes>> {};
  struct RawSingleString
      : terminal<mark<RawSingleHashes>(some(lit<"#">)) + lit<"\""> +
          many((lit<"\\"> + same<RawSingleHashes> + cr<R"(^\r\n)">) |
               (!(lit<"\""> + same<RawSingleHashes>) + cr<R"(^\r\n)">)) +
          lit<"\""> + same<RawSingleHashes>> {};
  struct RegexHashes {};
  struct RAW_REGEX
      : terminal<through(mark<RegexHashes>(some(lit<"#">)) + lit<"/">, lit<"/"> + same<RegexHashes>)> {};
  static constexpr auto RegexEscape = lit<"\\"> + any;
  static constexpr auto RegexClass = lit<"["> +
      many(RegexEscape | cr<R"(^\]\\\r\n)">) + lit<"]">;
  struct REGEX : terminal<lit<"/"> + !cr<R"(*/ \t\r\n)"> +
      some(RegexEscape | RegexClass | cr<R"(^/\\[\r\n)">) + lit<"/">> {};

  // -- Operators ----------------------------------------------------------

  // An operator's non-ASCII head, and the characters that may follow it.
  static constexpr auto OperatorHeadOther = cr<"¡-§©«¬®°-±¶»¿×÷‖-‗†-‧‰-‾⁁-⁓⁕-⁞←-⏿─-❵➔-⯿⸀-⹿、-〃〈-〠〰">;
  static constexpr auto OperatorFollowingChar = cr<"̀-ͯ᷀-᷿⃐-⃿︀-️︠-︯󠄀-󠇯">;
  static constexpr auto OpChar =
      cr<R"(/=+!*%&|<>^~?\x2d)"> | OperatorHeadOther | OperatorFollowingChar;
  static constexpr auto NonSlashOpStart =
      cr<R"(+!*%&|<>^~?\x2d)"> | OperatorHeadOther;
  // A leading '/' refuses behind `*` or `/`: an operator can never open on
  // what would read as a comment.
  struct OPERATOR : terminal<(lit<"/", !cr<"*/">> + many(OpChar)) |
                             (NonSlashOpStart + many(OpChar)) |
                             (lit<"="> + some(OpChar)) |
                             (lit<"."> + some(OpChar | lit<".">))> {};

  // -- Forward declarations ------------------------------------------

  struct Type;
  struct Pattern;
  struct BindingPattern;
  struct Expression;
  struct PrefixExpression;
  struct PostfixExpression;
  struct Statement;
  struct Declaration;
  struct IfStatement;
  struct DoStatement;
  struct SwitchStatement;
  struct Argument;
  struct TryOrPrefix;

  template <class Owner>
  static constexpr auto Effects =
      option(assign<&Owner::asyncKind>(kw<"async"> | kw<"reasync">)) +
      option(assign<&Owner::throwsKind>(kw<"throws"> | kw<"rethrows">) +
             option(lit<"("> + assign<&Owner::thrownType>(ref<Type>) + lit<")">));
  static constexpr auto ParameterSpecifier =
      kw<"borrowing"> | kw<"consuming"> | kw<"isolated"> | kw<"sending">;

  // -- Attributes -----------------------------------------------------

  struct BalancedTokens;
  struct StringLiteral;
  struct BalancedToken
      : rule<create<ast::Opaque>(
                 assign<&ast::Opaque::text>(lit<"(">) +
                 option(append<&ast::Opaque::children>(ref<BalancedTokens>)) + lit<")">) |
             create<ast::Opaque>(
                 assign<&ast::Opaque::text>(lit<"[">) +
                 option(append<&ast::Opaque::children>(ref<BalancedTokens>)) + lit<"]">) |
             create<ast::Opaque>(
                 assign<&ast::Opaque::text>(lit<"{">) +
                 option(append<&ast::Opaque::children>(ref<BalancedTokens>)) + lit<"}">) |
             create<ast::Opaque>(assign<&ast::Opaque::literal>(ref<StringLiteral>)) |
             create<ast::Opaque>(assign<&ast::Opaque::text>(
                 ref<Identifier> | ref<Keyword> | ref<INT_LIT> | ref<FLOAT_LIT> |
                 ref<OPERATOR> | lit<"="> | lit<"<"> | lit<">"> | lit<"."> | lit<"/"> | lit<"\\"> |
                 lit<","> | lit<":"> | lit<";"> | lit<"@"> | lit<"#">))> {};
  struct BalancedTokens
      : rule<create<ast::Opaque>(
            some(append<&ast::Opaque::children>(ref<BalancedToken>)))> {};

  struct Attribute_
      : rule<create<ast::Attribute>(
            lit<"@"> + some(append<&ast::Attribute::nameParts>(ref<Identifier>), lit<".">) +
            option(flag<&ast::Attribute::hasArguments>(lit<"(">) +
                   option(assign<&ast::Attribute::rawArgs>(ref<BalancedTokens>)) +
                   lit<")">))> {};

  // -- Generics -------------------------------------------------------

  struct GenericParam
      : rule<create<ast::GenericParam>(
            option(flag<&ast::GenericParam::isPack>(kw<"each">)) +
            assign<&ast::GenericParam::name>(ref<Identifier>) +
            option(lit<":"> +
                   assign<&ast::GenericParam::constraint>(ref<Type>)))> {};
  struct GenericParamClause
      : rule<create<ast::GenericParams>(
            lit<"<"> +
            some(append<&ast::GenericParams::params>(ref<GenericParam>),
                 lit<",">) + option(lit<",">) +
            lit<">">)> {};

  struct Requirement
      : rule<ref<Type> +
             (nest<ast::SameTypeReq, &ast::SameTypeReq::left>(
                  lit<"=="> + assign<&ast::SameTypeReq::right>(ref<Type>)) |
              nest<ast::ConformanceReq, &ast::ConformanceReq::type>(
                  lit<":"> +
                  assign<&ast::ConformanceReq::constraint>(ref<Type>)))> {};

  // -- Types ----------------------------------------------------------

  struct TYPE_HEAD : terminal<as<identifier_text>(!(kw<"Any"> | kw<"Self">)+(
                         IdentifierWord | EscapedWord))> {};
  struct TYPE_MEMBER
      : terminal<as<identifier_text>(
            !(kw<"Type"> | kw<"Protocol">)+(IdentifierWord | EscapedWord))> {};
  template <class Name>
  static constexpr auto TypeNameComponent =
      assign<&ast::NamedType::name>(ref<Name>) +
      option(lit<"<"> + some(append<&ast::NamedType::genericArgs>(ref<Type>), lit<",">) +
             option(lit<",">) + lit<">">);
  struct TypeIdentifier
      : rule<create<ast::NamedType>(TypeNameComponent<TYPE_HEAD>) +
             many(lit<"."> + nest<ast::NamedType, &ast::NamedType::qualifier>(
                                   TypeNameComponent<TYPE_MEMBER>))> {};

  struct TupleTypeElement
      : rule<create<ast::TupleTypeElement>(
            option(assign<&ast::TupleTypeElement::label>(ref<Identifier>) +
                   lit<":">) +
            assign<&ast::TupleTypeElement::type>(ref<Type>) +
            option(lit<"="> +
                   assign<&ast::TupleTypeElement::defaultValue>(
                       ref<Expression>)))> {};
  // `(T)` is `T` parenthesized: a tuple has none or at least two elements.
  struct TupleType
      : rule<create<ast::TupleType>(
            lit<"("> +
            option(append<&ast::TupleType::elements>(ref<TupleTypeElement>) +
                   some(lit<","> + append<&ast::TupleType::elements>(
                                       ref<TupleTypeElement>))) +
            lit<")">)> {};
  struct EnumCaseParameterClause
      : rule<create<ast::TupleType>(
            lit<"("> +
            option(some(append<&ast::TupleType::elements>(ref<TupleTypeElement>),
                        lit<",">)) +
            lit<")">)> {};

  struct ArrayType
      : rule<create<ast::ArrayType>(lit<"["> +
                                    assign<&ast::ArrayType::element>(ref<Type>) +
                                    !lit<":"> + lit<"]">)> {};
  struct DictionaryType
      : rule<create<ast::DictionaryType>(
            lit<"["> + assign<&ast::DictionaryType::key>(ref<Type>) + lit<":"> +
            assign<&ast::DictionaryType::value>(ref<Type>) + lit<"]">)> {};

  struct FunctionTypeParam
      : rule<create<ast::FunctionTypeParam>(
            many(append<&ast::FunctionTypeParam::attributes>(ref<Attribute_>)) +
            option(assign<&ast::FunctionTypeParam::label>(ref<Identifier>) +
                   lit<":">) +
            option(flag<&ast::FunctionTypeParam::inoutFlag>(kw<"inout">)) +
            many(append<&ast::FunctionTypeParam::specifiers>(ParameterSpecifier)) +
            assign<&ast::FunctionTypeParam::type>(ref<Type>))> {};

  struct FunctionType
      : rule<create<ast::FunctionType>(
            many(append<&ast::FunctionType::attributes>(ref<Attribute_>)) +
            lit<"("> +
            option(
                some(append<&ast::FunctionType::params>(ref<FunctionTypeParam>),
                     lit<",">) +
                option(flag<&ast::FunctionType::variadic>(lit<"...">))) +
            lit<")"> +
            Effects<ast::FunctionType> +
            lit<"->"> + assign<&ast::FunctionType::result>(ref<Type>))> {};

  struct ProtocolCompositionType
      : rule<create<ast::ProtocolCompositionType>(
            append<&ast::ProtocolCompositionType::members>(ref<TypeIdentifier>) +
            some(lit<"&"> + append<&ast::ProtocolCompositionType::members>(
                                 ref<TypeIdentifier>)))> {};

  struct OpaqueType
      : rule<create<ast::OpaqueType>(
            kw<"some"> + assign<&ast::OpaqueType::constraint>(ref<Type>))> {};
  struct ExistentialType
      : rule<create<ast::ExistentialType>(
            kw<"any"> + assign<&ast::ExistentialType::constraint>(ref<Type>))> {};
  struct PackElementType
      : rule<create<ast::PackElementType>(
            kw<"each"> + assign<&ast::PackElementType::pack>(ref<Type>))> {};
  struct PackExpansionType
      : rule<create<ast::PackExpansionType>(
            kw<"repeat"> + assign<&ast::PackExpansionType::pattern>(ref<Type>))> {};
  struct AnyType : rule<create<ast::AnyType> + kw<"Any">> {};
  struct SelfBigType : rule<create<ast::SelfType> + kw<"Self">> {};

  static constexpr auto TypeAtom =
      ref<FunctionType> | ref<ArrayType> | ref<DictionaryType> |
      ref<ProtocolCompositionType> | ref<TypeIdentifier> | ref<TupleType> |
      ref<OpaqueType> | ref<ExistentialType> | ref<PackElementType> |
      ref<PackExpansionType> | ref<AnyType> | ref<SelfBigType> |
      (lit<"("> + ref<Type> + lit<")">);

  struct Type
      : rule<TypeAtom +
             many(nest<ast::OptionalType, &ast::OptionalType::wrapped>(lit<"?">) |
                  nest<ast::OptionalType, &ast::OptionalType::wrapped>(
                      flag<&ast::OptionalType::implicitlyUnwrapped>(
                          lit<"!">)) |
                  nest<ast::MetatypeType, &ast::MetatypeType::base>(
                      lit<"."> + kw<"Type">) |
                  nest<ast::MetatypeType, &ast::MetatypeType::base>(
                      lit<"."> + flag<&ast::MetatypeType::isProtocol>(
                                     kw<"Protocol">)))> {};
  static constexpr auto TypeAnnotationType = lit<":"> + ref<Type>;

  // -- Patterns -------------------------------------------------------

  struct TuplePatternElement
      : rule<create<ast::TuplePatternElement>(
            option(assign<&ast::TuplePatternElement::label>(ref<Identifier>) +
                   lit<":">) +
            assign<&ast::TuplePatternElement::pattern>(ref<Pattern>))> {};
  struct TuplePattern
      : rule<create<ast::TuplePattern>(
            lit<"("> +
            option(some(append<&ast::TuplePattern::elements>(
                            ref<TuplePatternElement>),
                        lit<",">)) +
            lit<")"> + option(assign<&ast::TuplePattern::type>(TypeAnnotationType)))> {
  };

  struct TupleBindingElement
      : rule<create<ast::TuplePatternElement>(
            option(assign<&ast::TuplePatternElement::label>(ref<Identifier>) + lit<":">) +
            assign<&ast::TuplePatternElement::pattern>(ref<BindingPattern>))> {};
  struct BindingPattern
      : rule<create<ast::BindingPattern>(
                 (flag<&ast::BindingPattern::isVar>(kw<"var">) | kw<"let">) +
                 assign<&ast::BindingPattern::sub>(ref<BindingPattern>)) |
             create<ast::TuplePattern>(
                 lit<"("> + option(some(append<&ast::TuplePattern::elements>(ref<TupleBindingElement>), lit<",">)) +
                 lit<")"> + option(assign<&ast::TuplePattern::type>(TypeAnnotationType))) |
             create<ast::WildcardPattern>(lit<"_"> + option(assign<&ast::WildcardPattern::type>(TypeAnnotationType))) |
             create<ast::IdentifierPattern>(assign<&ast::IdentifierPattern::name>(ref<Identifier>) +
                 option(assign<&ast::IdentifierPattern::type>(TypeAnnotationType)))> {};

  struct EnumCasePattern
      : rule<create<ast::EnumCasePattern>(
            option(assign<&ast::EnumCasePattern::typeIdentifier>(
                ref<TypeIdentifier>)) +
            lit<"."> + assign<&ast::EnumCasePattern::caseName>(ref<Identifier>) +
            option(assign<&ast::EnumCasePattern::args>(ref<TuplePattern>)))> {};

  struct Pattern
      : rule<create<ast::BindingPattern>(
                flag<&ast::BindingPattern::isVar>(kw<"var">) +
                assign<&ast::BindingPattern::sub>(ref<Pattern>)) |
             create<ast::BindingPattern>(
                 kw<"let"> + assign<&ast::BindingPattern::sub>(ref<Pattern>)) |
             ref<EnumCasePattern> |
             create<ast::OptionalPattern>(
                 assign<&ast::OptionalPattern::name>(ref<Identifier>) + lit<"?">) |
             create<ast::IsTypePattern>(
                 kw<"is"> + assign<&ast::IsTypePattern::type>(ref<Type>)) |
             ref<TuplePattern> |
             create<ast::WildcardPattern>(
                 lit<"_"> +
                 option(assign<&ast::WildcardPattern::type>(TypeAnnotationType))) |
             create<ast::IdentifierPattern>(
                 assign<&ast::IdentifierPattern::name>(ref<Identifier>) +
                 option(assign<&ast::IdentifierPattern::type>(TypeAnnotationType))) |
             create<ast::ExprPattern>(
                 assign<&ast::ExprPattern::expr>(ref<Expression>))> {};

  // -- Literals as expressions ------------------------------------------

  struct IntLiteral
      : rule<create<ast::IntLit>(
            assign<&ast::IntLit::text>(ref<INT_LIT> | ref<DECIMAL_DIGITS>))> {};
  struct FloatLiteral
      : rule<assign<&ast::FloatLit::text>(ref<FLOAT_LIT>)> {
  };
  struct BoolLiteral
      : rule<create<ast::BoolLit>(
            flag<&ast::BoolLit::value>(kw<"true">) | kw<"false">)> {};
  struct NilLiteral : rule<create<ast::NilLit> + kw<"nil">> {};

  static constexpr auto StringInterpolation = create<ast::StringPart>(
      flag<&ast::StringPart::interpolation>(ref<InterpOpen>) +
      skip(ignore<WS>, hide<SL_COMMENT, ML_COMMENT>,
           option(some(append<&ast::StringPart::arguments>(ref<Argument>), lit<",">)) + lit<")">));
  struct StringLiteral
      : rule<create<ast::StringLit>(
                 (flag<&ast::StringLit::isMultiline>(ref<MultiStringOpen>) +
                  tight(many(append<&ast::StringLit::parts>(
                      create<ast::StringPart>(assign<&ast::StringPart::text>(ref<MultiQuotedText>)) |
                      StringInterpolation)) + ref<MultiStringClose>)) |
                 (ref<StringOpen> +
                  tight(many(append<&ast::StringLit::parts>(
                      create<ast::StringPart>(assign<&ast::StringPart::text>(ref<QuotedText>)) |
                      StringInterpolation)) + ref<StringClose>))) |
             create<ast::RawStringLit>(assign<&ast::RawStringLit::text>(
                 ref<RawMultiString> | ref<RawSingleString>))> {};

  struct ArrayLit
      : rule<create<ast::ArrayLit>(
            lit<"["> +
            option(some(append<&ast::ArrayLit::elements>(ref<Expression>),
                        lit<",">) +
                   option(lit<",">)) +
            lit<"]">)> {};
  struct DictLit
      : rule<create<ast::DictLit>(
            lit<"["> +
            (lit<":">  //
             | (some(append<&ast::DictLit::entries>(
                         create<ast::DictEntry>(
                             assign<&ast::DictEntry::key>(ref<Expression>) +
                             lit<":"> +
                             assign<&ast::DictEntry::value>(ref<Expression>))),
                     lit<",">) +
                option(lit<",">))) +
            lit<"]">)> {};

  struct PlaygroundColorLit
      : rule<create<ast::PlaygroundColorLit>(
            kw<"#colorLiteral"> + lit<"("> + kw<"red"> + lit<":"> +
            assign<&ast::PlaygroundColorLit::red>(ref<Expression>) + lit<","> +
            kw<"green"> + lit<":"> +
            assign<&ast::PlaygroundColorLit::green>(ref<Expression>) + lit<","> +
            kw<"blue"> + lit<":"> +
            assign<&ast::PlaygroundColorLit::blue>(ref<Expression>) + lit<","> +
            kw<"alpha"> + lit<":"> +
            assign<&ast::PlaygroundColorLit::alpha>(ref<Expression>) + lit<")">)> {
  };
  struct PlaygroundResourceLit
      : rule<create<ast::PlaygroundResourceLit>(
            (flag<&ast::PlaygroundResourceLit::isImage>(kw<"#imageLiteral">) +
                lit<"("> + kw<"resourceName"> + lit<":"> +
                assign<&ast::PlaygroundResourceLit::resourceName>(ref<Expression>) +
                lit<")">) |
            (kw<"#fileLiteral"> + lit<"("> + kw<"resourceName"> + lit<":"> +
                assign<&ast::PlaygroundResourceLit::resourceName>(ref<Expression>) +
                lit<")">))> {};

  struct LiteralExpr
      : rule<ref<FloatLiteral> | ref<IntLiteral> | ref<StringLiteral> |
             create<ast::RegexLit>(assign<&ast::RegexLit::text>(ref<RAW_REGEX> | ref<REGEX>)) |
             ref<BoolLiteral> | ref<NilLiteral> | ref<ArrayLit> | ref<DictLit> |
             ref<PlaygroundColorLit> | ref<PlaygroundResourceLit> |
             create<ast::MacroLiteralExpr>(assign<&ast::MacroLiteralExpr::kind>(
                 kw<"#file"> | kw<"#fileID"> | kw<"#filePath"> | kw<"#line"> |
                 kw<"#column"> | kw<"#function"> | kw<"#dsohandle">))> {};

  struct SelfExpr
      : rule<create<ast::SelfExpr> + kw<"self">> {};
  struct SuperExpr
      : rule<create<ast::SuperExpr> + kw<"super">> {};

  struct CaptureSpecifier
      : rule<kw<"weak"> | (kw<"unowned"> +
                           option(lit<"("> + (kw<"safe"> | kw<"unsafe">)+lit<")">))> {};
  struct CaptureItem
      : rule<create<ast::CaptureItem>(
            option(assign<&ast::CaptureItem::specifier>(ref<CaptureSpecifier>)) +
            ((assign<&ast::CaptureItem::name>(ref<Identifier>) +
              option(lit<"="> + assign<&ast::CaptureItem::value>(ref<Expression>))) |
             assign<&ast::CaptureItem::value>(ref<SelfExpr>)))> {};
  struct ClosureParam
      : rule<create<ast::ClosureParam>(
            option(lit<"_">) + assign<&ast::ClosureParam::name>(ref<Identifier>) +
            option(assign<&ast::ClosureParam::type>(TypeAnnotationType) +
                   option(flag<&ast::ClosureParam::variadic>(lit<"...">))))> {
  };
  static constexpr auto CaptureList =
      lit<"["> + some(append<&ast::ClosureExpr::captures>(ref<CaptureItem>), lit<",">) +
      lit<"]">;
  struct ClosureExpr
      : rule<create<ast::ClosureExpr>(
            lit<"{"> +
            option(flag<&ast::ClosureExpr::hasSignature>(
                option(CaptureList) +
                option(
                    (lit<"("> +
                     option(some(append<&ast::ClosureExpr::params>(ref<ClosureParam>),
                                 lit<",">)) +
                     lit<")">) |
                    some(append<&ast::ClosureExpr::params>(ref<ClosureParam>),
                         lit<",">)) +
                Effects<ast::ClosureExpr> +
                option(lit<"->"> + assign<&ast::ClosureExpr::resultType>(ref<Type>)) +
                kw<"in">)) +
            many(append<&ast::ClosureExpr::body>(ref<Statement>)) + lit<"}">)> {};

  struct OperatorRefExpr
      : rule<create<ast::OperatorRefExpr>(
            assign<&ast::OperatorRefExpr::op>(ref<OPERATOR> | lit<"=">))> {};
  struct TupleElement
      : rule<create<ast::TupleElement>(
            option(assign<&ast::TupleElement::label>(ref<Identifier>) + lit<":">) +
            assign<&ast::TupleElement::value>(ref<Expression>))> {};
  struct TupleOrParenExpr
      : rule<create<ast::TupleExpr>(lit<"("> + lit<")">) |
             create<ast::TupleExpr>(
                 lit<"("> + append<&ast::TupleExpr::elements>(ref<TupleElement>) +
                 lit<","> +
                 some(append<&ast::TupleExpr::elements>(ref<TupleElement>), lit<",">) +
                 lit<")">) |
             (lit<"("> + ref<Expression> + lit<")">) |
             (lit<"("> + ref<OperatorRefExpr> + lit<")">)> {};

  struct WildcardExpr : rule<create<ast::WildcardExpr> + lit<"_">> {};

  struct KeyPathComponent
      : rule<create<ast::KeyPathComponent>(
            assign<&ast::KeyPathComponent::name>(ref<Identifier>) +
            many(append<&ast::KeyPathComponent::postfixes>(lit<"?"> | lit<"!"> |
                                                            kw<"self">) |
                 (lit<"["> +
                  option(append<&ast::KeyPathComponent::subscriptArgs>(
                      ref<BalancedTokens>)) +
                  lit<"]">))) |
            create<ast::KeyPathComponent>(
                some(append<&ast::KeyPathComponent::postfixes>(
                         lit<"?"> | lit<"!"> | kw<"self">) |
                     (lit<"["> +
                      option(append<&ast::KeyPathComponent::subscriptArgs>(
                          ref<BalancedTokens>)) +
                      lit<"]">)))> {};
  struct KeyPathExpr
      : rule<create<ast::KeyPathExpr>(
            lit<"\\"> + option(assign<&ast::KeyPathExpr::root>(TypeAtom)) + lit<"."> +
            some(append<&ast::KeyPathExpr::components>(ref<KeyPathComponent>), lit<".">))> {};

  struct SelectorExpr
      : rule<create<ast::SelectorExpr>(
            kw<"#selector"> + lit<"("> +
            option(assign<&ast::SelectorExpr::kind>(kw<"getter"> | kw<"setter">) +
                   lit<":">) +
            assign<&ast::SelectorExpr::value>(ref<Expression>) + lit<")">)> {};
  struct KeyPathStringExpr
      : rule<create<ast::KeyPathStringExpr>(
            kw<"#keyPath"> + lit<"("> +
            assign<&ast::KeyPathStringExpr::value>(ref<Expression>) + lit<")">)> {};

  struct PrimaryExpression
      : rule<create<ast::Ident>(
                assign<&ast::Ident::name>(ref<ReferenceIdentifier>) +
                option(lit<"<"> +
                       some(append<&ast::Ident::genericArgs>(ref<Type>), lit<",">) +
                       lit<">">) +
                option(lit<"("> +
                       some(append<&ast::Ident::argumentLabels>(ref<Identifier>) +
                            lit<":">) +
                       lit<")">)) |
             ref<LiteralExpr> |
             ref<SelfExpr> | ref<SuperExpr> | ref<ClosureExpr> |
             ref<TupleOrParenExpr> |
             create<ast::ImplicitMemberExpr>(
                 lit<"."> +
                 assign<&ast::ImplicitMemberExpr::name>(ref<Identifier> |
                                                         ref<Keyword>)) |
             ref<WildcardExpr> | ref<KeyPathExpr> | ref<SelectorExpr> |
             ref<KeyPathStringExpr>> {};

  // -- Postfix chain ----------------------------------------------------

  struct Argument
      : rule<create<ast::Argument>(
            option(assign<&ast::Argument::label>(ref<Identifier>) + lit<":">) +
            assign<&ast::Argument::value>(ref<Expression> | ref<OperatorRefExpr>))> {
  };
  // Another identifier may start a statement after the trailing closure.
  // Its colon and opening brace distinguish it from a labeled statement.
  struct LabeledClosure
      : rule<&(ref<Identifier> + lit<":"> + lit<"{">) + create<ast::LabeledClosure>(
            assign<&ast::LabeledClosure::label>(ref<Identifier>) + lit<":"> +
            assign<&ast::LabeledClosure::closure>(ref<ClosureExpr>))> {};

  struct PostfixExpression
      : rule<ref<PrimaryExpression> +
             many(nest<ast::CallExpr, &ast::CallExpr::callee>(
                      (lit<"("> +
                       option(some(append<&ast::CallExpr::args>(ref<Argument>),
                                   lit<",">) + option(lit<",">)) +
                       lit<")"> +
                       option(assign<&ast::CallExpr::trailingClosure>(
                                  ref<ClosureExpr>) +
                              many(append<&ast::CallExpr::labeledTrailingClosures>(
                                  ref<LabeledClosure>)))) |
                      (assign<&ast::CallExpr::trailingClosure>(ref<ClosureExpr>) +
                       many(append<&ast::CallExpr::labeledTrailingClosures>(
                           ref<LabeledClosure>))))  //
                  | nest<ast::InitExpr, &ast::InitExpr::base>(
                        lit<"."> + kw<"init"> +
                        option(lit<"("> +
                               some(append<&ast::InitExpr::argumentLabels>(
                                        ref<Identifier>) +
                                    lit<":">) +
                               lit<")">))  //
                  | nest<ast::MemberExpr, &ast::MemberExpr::base>(
                        lit<"."> +
                        (assign<&ast::MemberExpr::name>(ref<DECIMAL_DIGITS>) |
                         (!(kw<"self"> | kw<"init">) + assign<&ast::MemberExpr::name>(ref<Identifier>) +
                          option(lit<"<"> +
                                 some(append<&ast::MemberExpr::genericArgs>(
                                          ref<Type>),
                                      lit<",">) +
                                 lit<">">) +
                          option(lit<"("> +
                                 some(append<&ast::MemberExpr::argumentLabels>(
                                          ref<Identifier>) +
                                      lit<":">) +
                                 lit<")">))))  //
                  | nest<ast::SelfPostfixExpr, &ast::SelfPostfixExpr::base>(
                        lit<"."> + kw<"self">)  //
                  | nest<ast::SubscriptExpr, &ast::SubscriptExpr::base>(
                        lit<"["> +
                        some(append<&ast::SubscriptExpr::args>(ref<Argument>),
                             lit<",">) +
                        lit<"]">)  //
                  | nest<ast::ForceUnwrapExpr, &ast::ForceUnwrapExpr::base>(lit<"!">)  //
                  | nest<ast::OptionalChainExpr, &ast::OptionalChainExpr::base>(
                        lit<"?">)  //
                  // `postfix_operator: {isPostfixOp}? operator;` -- no gap
                  // before (glued to the operand), a gap after.
                  | nest<ast::PostfixOpExpr, &ast::PostfixOpExpr::operand>(
                        gap::none + assign<&ast::PostfixOpExpr::op>(ref<OPERATOR>) +
                        !gap::none))> {};

  struct PrefixExpression
      : rule<create<ast::ControlExpr>(assign<&ast::ControlExpr::statement>(
                 ref<IfStatement> | ref<SwitchStatement>)) |
             create<ast::PrefixExpr>(
                assign<&ast::PrefixExpr::op>(ref<OPERATOR>) +
                assign<&ast::PrefixExpr::operand>(ref<PrefixExpression>)) |
             ref<PostfixExpression>> {};

  struct TryKind : rule<kw<"try"> + option(lit<"?"> | lit<"!">)> {};
  struct TryOrPrefix : rule<create<ast::TryExpr>(
                                assign<&ast::TryExpr::kind>(ref<TryKind>) +
                                assign<&ast::TryExpr::operand>(ref<TryOrPrefix>)) |
                            create<ast::AwaitExpr>(kw<"await"> +
                                assign<&ast::AwaitExpr::operand>(ref<TryOrPrefix>)) |
                            create<ast::OwnershipExpr>(
                                assign<&ast::OwnershipExpr::kind>(kw<"consume"> | kw<"copy">) +
                                !gap::line + &(ref<ID> | kw<"self">) +
                                assign<&ast::OwnershipExpr::operand>(ref<TryOrPrefix>)) |
                            ref<PrefixExpression>> {};

  // -- Binary chain ------------------------------------------------------

  // An operator is binary when the gap reads the same way on both sides:
  // whitespace on both, or on neither.
  static constexpr auto BinaryOperator = ref<OPERATOR> | lit<"<"> | lit<">">;
  struct BinaryOp
      : rule<create<ast::BinaryOp>(
            ((gap::none + assign<&ast::BinaryOp::op>(BinaryOperator) + gap::none) |
             (!gap::none + assign<&ast::BinaryOp::op>(BinaryOperator) + !gap::none)) +
            assign<&ast::BinaryOp::operand>(ref<TryOrPrefix>))> {};
  struct AssignOp : rule<create<ast::AssignOp>(
                        ((gap::none + lit<"="> + gap::none) |
                         (!gap::none + lit<"="> + !gap::none)) +
                        assign<&ast::AssignOp::operand>(ref<TryOrPrefix>))> {};
  struct TernaryOp
      : rule<create<ast::TernaryOp>(
            lit<"?"> + assign<&ast::TernaryOp::thenExpr>(ref<Expression>) + lit<":"> +
            assign<&ast::TernaryOp::operand>(ref<TryOrPrefix>))> {};
  struct IsCastOp
      : rule<create<ast::IsCastOp>(kw<"is"> + assign<&ast::IsCastOp::type>(ref<Type>))> {
  };
  struct AsCastOp
      : rule<create<ast::AsCastOp>(
            kw<"as"> + option(assign<&ast::AsCastOp::marker>(lit<"?"> | lit<"!">)) +
            assign<&ast::AsCastOp::type>(ref<Type>))> {};

  struct Expression
      : rule<ref<TryOrPrefix> +
             option(nest<ast::BinaryChainExpr, &ast::BinaryChainExpr::first>(
                 some(append<&ast::BinaryChainExpr::items>(
                     ref<BinaryOp> | ref<AssignOp> | ref<TernaryOp> | ref<IsCastOp> |
                     ref<AsCastOp>))))> {};

  // -- Conditions -------------------------------------------------------

  struct VERSION : terminal<some(d | lit<".">)> {};
  struct AvailabilityArg
      : rule<create<ast::AvailabilityArg>(
                flag<&ast::AvailabilityArg::wildcard>(lit<"*">)) |
             create<ast::AvailabilityArg>(
                 assign<&ast::AvailabilityArg::platform>(ref<Identifier>) +
                 assign<&ast::AvailabilityArg::version>(ref<VERSION>))> {};
  struct AvailabilityCondition
      : rule<create<ast::Condition>(
            flag<&ast::Condition::isAvailability>(kw<"#available"> |
                flag<&ast::Condition::isUnavailable>(kw<"#unavailable">)) + lit<"("> +
            some(append<&ast::Condition::availability>(ref<AvailabilityArg>),
                 lit<",">) +
            lit<")">)> {};

  struct Condition
      : rule<create<ast::Condition>(
                assign<&ast::Condition::bindingKeyword>(kw<"let"> | kw<"var">) +
                assign<&ast::Condition::pattern>(ref<BindingPattern>) +
                option(lit<"="> + assign<&ast::Condition::expr>(ref<Expression>))) |
             create<ast::Condition>(
                 flag<&ast::Condition::isCase>(kw<"case">) +
                 assign<&ast::Condition::pattern>(ref<Pattern>) + lit<"="> +
                 assign<&ast::Condition::expr>(ref<Expression>)) |
             ref<AvailabilityCondition> |
             create<ast::Condition>(assign<&ast::Condition::expr>(ref<Expression>))> {
  };

  // -- Statements -------------------------------------------------------

  struct CodeBlock
      : rule<create<ast::Block>(
            lit<"{"> + many(append<&ast::Block::statements>(ref<Statement>)) +
            lit<"}">)> {};

  struct ForInStatement
      : rule<create<ast::ForInStmt>(
            kw<"for"> + option(flag<&ast::ForInStmt::tryFlag>(kw<"try">)) +
            option(flag<&ast::ForInStmt::awaitFlag>(kw<"await">)) +
            ((flag<&ast::ForInStmt::isCase>(kw<"case">) +
              assign<&ast::ForInStmt::pattern>(ref<Pattern>)) |
             assign<&ast::ForInStmt::pattern>(ref<BindingPattern>)) + kw<"in"> +
            assign<&ast::ForInStmt::collection>(ref<Expression>) +
            option(kw<"where"> +
                   assign<&ast::ForInStmt::whereClause>(ref<Expression>)) +
            assign<&ast::ForInStmt::body>(ref<CodeBlock>))> {};
  struct WhileStatement
      : rule<create<ast::WhileStmt>(
            kw<"while"> +
            some(append<&ast::WhileStmt::conditions>(ref<Condition>), lit<",">) +
            assign<&ast::WhileStmt::body>(ref<CodeBlock>))> {};
  struct RepeatWhileStatement
      : rule<create<ast::RepeatWhileStmt>(
            kw<"repeat"> + assign<&ast::RepeatWhileStmt::body>(ref<CodeBlock>) +
            kw<"while"> + assign<&ast::RepeatWhileStmt::cond>(ref<Expression>))> {};
  struct LoopStatement
      : rule<ref<ForInStatement> | ref<WhileStatement> | ref<RepeatWhileStatement>> {
  };

  struct IfStatement
      : rule<create<ast::IfStmt>(
            kw<"if"> +
            some(append<&ast::IfStmt::conditions>(ref<Condition>), lit<",">) +
            assign<&ast::IfStmt::thenBranch>(ref<CodeBlock>) +
            option(kw<"else"> +
                   assign<&ast::IfStmt::elseBranch>(ref<CodeBlock> | ref<IfStatement>)))> {
  };
  struct GuardStatement
      : rule<create<ast::GuardStmt>(
            kw<"guard"> +
            some(append<&ast::GuardStmt::conditions>(ref<Condition>), lit<",">) +
            kw<"else"> + assign<&ast::GuardStmt::elseBranch>(ref<CodeBlock>))> {};

  struct CaseItem
      : rule<create<ast::CaseItem>(
            assign<&ast::CaseItem::pattern>(ref<Pattern>) +
            option(kw<"where"> +
                   assign<&ast::CaseItem::whereClause>(ref<Expression>)))> {};
  struct SwitchCase;
  struct IfConfigSwitchCase;
  struct SwitchCaseOrConfig : rule<ref<SwitchCase> | ref<IfConfigSwitchCase>> {};
  struct SwitchCase
      : rule<create<ast::SwitchCase>(
            many(append<&ast::SwitchCase::attributes>(ref<Attribute_>)) +
            ((kw<"case"> + some(append<&ast::SwitchCase::items>(ref<CaseItem>), lit<",">) +
              lit<":">)  //
             | (flag<&ast::SwitchCase::isDefault>(kw<"default">) + lit<":">)) +
            some(append<&ast::SwitchCase::body>(ref<Statement>)))> {};
  struct CompilationCondition;
  struct IfConfigSwitchCase
      : rule<create<ast::IfConfigStmt>(
            append<&ast::IfConfigStmt::branches>(create<ast::IfConfigBranch>(
                kw<"#if"> +
                assign<&ast::IfConfigBranch::condition>(ref<CompilationCondition>) +
                many(append<&ast::IfConfigBranch::body>(ref<SwitchCaseOrConfig>)))) +
            many(append<&ast::IfConfigStmt::branches>(create<ast::IfConfigBranch>(
                kw<"#elseif"> +
                assign<&ast::IfConfigBranch::condition>(ref<CompilationCondition>) +
                many(append<&ast::IfConfigBranch::body>(ref<SwitchCaseOrConfig>))))) +
            option(append<&ast::IfConfigStmt::branches>(create<ast::IfConfigBranch>(
                kw<"#else"> +
                many(append<&ast::IfConfigBranch::body>(ref<SwitchCaseOrConfig>))))) +
            kw<"#endif">)> {};

  struct SwitchStatement
      : rule<create<ast::SwitchStmt>(
            kw<"switch"> + assign<&ast::SwitchStmt::subject>(ref<Expression>) +
            lit<"{"> +
            many(append<&ast::SwitchStmt::cases>(ref<SwitchCaseOrConfig>)) +
            lit<"}">)> {};
  struct BranchStatement
      : rule<ref<IfStatement> | ref<GuardStatement> | ref<SwitchStatement>> {};

  struct LabeledStatement
      : rule<create<ast::LabeledStmt>(
            assign<&ast::LabeledStmt::label>(ref<Identifier>) + lit<":"> +
            assign<&ast::LabeledStmt::body>(
                ref<LoopStatement> | ref<IfStatement> | ref<SwitchStatement> |
                ref<DoStatement>))> {};

  struct BreakStatement
      : rule<create<ast::BreakStmt>(
            kw<"break"> +
            option(!gap::line + assign<&ast::BreakStmt::label>(ref<Identifier>)))> {};
  struct ContinueStatement
      : rule<create<ast::ContinueStmt>(
            kw<"continue"> +
            option(!gap::line + assign<&ast::ContinueStmt::label>(ref<Identifier>)))> {};
  struct FallthroughStatement
      : rule<create<ast::FallthroughStmt> + kw<"fallthrough">> {};
  struct ReturnStatement
      : rule<create<ast::ReturnStmt>(
            kw<"return"> +
            option(!gap::line + assign<&ast::ReturnStmt::value>(ref<Expression>)))> {};
  struct ThrowStatement
      : rule<create<ast::ThrowStmt>(
            kw<"throw"> + assign<&ast::ThrowStmt::value>(ref<Expression>))> {};
  struct ControlTransferStatement
      : rule<ref<BreakStatement> | ref<ContinueStatement> |
             ref<FallthroughStatement> | ref<ReturnStatement> | ref<ThrowStatement>> {
  };

  struct DeferStatement
      : rule<create<ast::DeferStmt>(
            kw<"defer"> + assign<&ast::DeferStmt::body>(ref<CodeBlock>))> {};

  // `where` opens the guard, never a pattern, though `Identifier` admits it.
  struct CatchPattern
      : rule<create<ast::CatchPattern>(
            (!kw<"where"> + assign<&ast::CatchPattern::pattern>(ref<Pattern>) +
             option(kw<"where"> + assign<&ast::CatchPattern::whereClause>(
                                      ref<Expression>))) |
            (kw<"where"> +
             assign<&ast::CatchPattern::whereClause>(ref<Expression>)))> {};
  struct CatchClause
      : rule<create<ast::CatchClause>(
            kw<"catch"> +
            option(some(append<&ast::CatchClause::patterns>(ref<CatchPattern>),
                        lit<",">)) +
            assign<&ast::CatchClause::body>(ref<CodeBlock>))> {};
  struct DoStatement
      : rule<create<ast::DoStmt>(
            kw<"do"> + assign<&ast::DoStmt::body>(ref<CodeBlock>) +
            many(append<&ast::DoStmt::catches>(ref<CatchClause>)))> {};

  // -- Compiler control ---------------------------------------------------

  static constexpr auto PlatformWord = kw<"os"> | kw<"arch"> | kw<"swift"> |
                                       kw<"compiler"> | kw<"canImport"> |
                                       kw<"targetEnvironment">;
  struct PlatformArgument : terminal<some(cr<"^()">)> {};
  struct PlatformCondition
      : rule<create<ast::PlatformCondition>(
            assign<&ast::PlatformCondition::kind>(PlatformWord) + lit<"("> +
            assign<&ast::PlatformCondition::argument>(ref<PlatformArgument>) +
            lit<")">)> {};

  struct CompilationAtom
      : rule<ref<PlatformCondition> |
             create<ast::PrefixExpr>(
                 assign<&ast::PrefixExpr::op>(lit<"!">) +
                 assign<&ast::PrefixExpr::operand>(ref<CompilationAtom>)) |
             (lit<"("> + ref<CompilationCondition> + lit<")">) |
             ref<BoolLiteral> |
             create<ast::Ident>(!(PlatformWord + &lit<"(">)+assign<
                                &ast::Ident::name>(ref<Identifier>))> {};
  struct CompilationCondition
      : rule<ref<CompilationAtom> +
             option(
                 nest<ast::BinaryChainExpr, &ast::BinaryChainExpr::first>(some(
                     append<&ast::BinaryChainExpr::items>(create<ast::BinaryOp>(
                         assign<&ast::BinaryOp::op>(lit<"&&"> | lit<"||">) +
                         assign<&ast::BinaryOp::operand>(
                             ref<CompilationAtom>))))))> {};

  struct IfConfigBlock
      : rule<create<ast::IfConfigStmt>(
            append<&ast::IfConfigStmt::branches>(create<ast::IfConfigBranch>(
                kw<"#if"> +
                assign<&ast::IfConfigBranch::condition>(
                    ref<CompilationCondition>) +
                many(append<&ast::IfConfigBranch::body>(ref<Statement>)))) +
            many(append<
                 &ast::IfConfigStmt::branches>(create<ast::IfConfigBranch>(
                kw<"#elseif"> +
                assign<&ast::IfConfigBranch::condition>(
                    ref<CompilationCondition>) +
                many(append<&ast::IfConfigBranch::body>(ref<Statement>))))) +
            option(append<&ast::IfConfigStmt::branches>(
                create<ast::IfConfigBranch>(
                    kw<"#else"> + many(append<&ast::IfConfigBranch::body>(
                                      ref<Statement>))))) +
            kw<"#endif">)> {};

  // Assign even the empty span, keeping file: "" distinct from a reset.
  struct LineFileText : rule<option(ref<QuotedText>)> {};

  struct LineControlStatement
      : rule<create<ast::LineControlStmt>(
            kw<"#sourceLocation"> + lit<"("> +
            option(kw<"file"> + lit<":"> + ref<StringOpen> +
                   assign<&ast::LineControlStmt::fileName>(ref<LineFileText>) +
                   ref<StringClose> + lit<","> + kw<"line"> + lit<":"> +
                   assign<&ast::LineControlStmt::line>(ref<DECIMAL_DIGITS>)) +
            lit<")">)> {};
  struct DiagnosticStatement
      : rule<create<ast::DiagnosticStmt>(
            (flag<&ast::DiagnosticStmt::isError>(kw<"#error">) + lit<"("> +
                assign<&ast::DiagnosticStmt::message>(ref<Expression>) + lit<")">) |
            (kw<"#warning"> + lit<"("> +
                assign<&ast::DiagnosticStmt::message>(ref<Expression>) + lit<")">))> {
  };
  struct CompilerControlStatement
      : rule<ref<IfConfigBlock> | ref<LineControlStatement> | ref<DiagnosticStatement>> {
  };

  // Bare reserved words introduce declarations or statements; an expression
  // may still use escaped names and keyword members/argument labels.
  static constexpr auto ReservedStatementHead =
      kw<"class"> | kw<"struct"> | kw<"protocol"> | kw<"for"> | kw<"do"> |
      kw<"guard"> | kw<"while"> | kw<"public"> | kw<"private"> | kw<"internal">;

  struct Statement
      : rule<ref<Declaration> | (ref<LoopStatement> + eos) |
             (ref<BranchStatement> + eos) | (ref<LabeledStatement> + eos) |
             (ref<ControlTransferStatement> + eos) | (ref<DeferStatement> + eos) |
             (ref<DoStatement> + eos) |
             (!ReservedStatementHead +
              create<ast::ExprStmt>(assign<&ast::ExprStmt::value>(ref<Expression>)) +
              eos) |
             ref<CompilerControlStatement>> {};

  // -- Declaration modifiers ----------------------------------------------

  static constexpr auto AccessLevelWord =
      kw<"private"> | kw<"fileprivate"> | kw<"internal"> | kw<"public"> | kw<"open">;
  static constexpr auto DeclModifierWord =
      kw<"class"> | kw<"convenience"> | kw<"dynamic"> | kw<"final"> | kw<"infix"> |
      kw<"lazy"> | kw<"optional"> | kw<"override"> | kw<"postfix"> | kw<"prefix"> |
      kw<"required"> | kw<"static"> | kw<"unowned"> | kw<"weak"> | AccessLevelWord |
      kw<"mutating"> | kw<"nonmutating"> | kw<"nonisolated"> |
      kw<"isolated"> | kw<"distributed"> | kw<"consuming"> | kw<"borrowing"> | kw<"indirect">;
  // The parenthesized qualifier changes what the modifier means
  // (`private` vs `private(set)`, `unowned` vs `unowned(safe)`), so it is
  // kept on the node rather than only being matched and dropped.
  struct Modifier_
      : rule<create<ast::Modifier>(
            assign<&ast::Modifier::keyword>(DeclModifierWord) +
            option(lit<"("> +
                   assign<&ast::Modifier::qualifier>(
                       kw<"safe"> | kw<"unsafe"> | kw<"set"> | kw<"nonsending">) +
                   lit<")">))> {};


  // -- Declarations -------------------------------------------------------

  struct ImportDecl
      : rule<create<ast::ImportDecl>(
            many(append<&ast::Decl::attributes>(ref<Attribute_>)) +
            kw<"import"> +
            option(assign<&ast::ImportDecl::kind>(
                kw<"typealias"> | kw<"struct"> | kw<"class"> | kw<"enum"> |
                kw<"protocol"> | kw<"let"> | kw<"var"> | kw<"func">)) +
            append<&ast::ImportDecl::path>(ref<Identifier>) +
            many(lit<"."> +
                 append<&ast::ImportDecl::path>(ref<Identifier> | ref<OPERATOR>)) +
            eos)> {};

  struct PatternInitializer
      : rule<create<ast::PatternBinding>(
            assign<&ast::PatternBinding::pattern>(ref<BindingPattern>) +
            option(lit<"="> +
                   assign<&ast::PatternBinding::initializer>(ref<Expression>)))> {};

  struct AccessorClauseRule
      : rule<create<ast::Accessor>(
            many(append<&ast::Accessor::attributes>(ref<Attribute_>)) +
            ((option(assign<&ast::Accessor::mutability>(kw<"mutating"> | kw<"nonmutating">)) +
              (assign<&ast::Accessor::kind>(kw<"get">) |
               (assign<&ast::Accessor::kind>(kw<"set">) +
                option(lit<"("> + assign<&ast::Accessor::parameterName>(ref<Identifier>) + lit<")">)))) |
             (assign<&ast::Accessor::kind>(kw<"willSet"> | kw<"didSet">) +
              option(lit<"("> + assign<&ast::Accessor::parameterName>(ref<Identifier>) + lit<")">))) +
            Effects<ast::Accessor> +
            option(flag<&ast::Accessor::hasBody>(lit<"{">) +
                   many(append<&ast::Accessor::body>(ref<Statement>)) + lit<"}">))> {};
  static constexpr auto AccessorClause = ref<AccessorClauseRule>;
  // `var x: T { statements }`, the body alone: an implicit getter, same as a
  // computed property spelling `get` explicitly. Kept apart from `AccessorClause`
  // (empty `kind`) so a reader can tell the two spellings apart.
  static constexpr auto VarAccessorBody =
      some(append<&ast::VariableDecl::accessors>(AccessorClause)) |
      append<&ast::VariableDecl::accessors>(
          create<ast::Accessor>(many(append<&ast::Accessor::body>(ref<Statement>))));
  static constexpr auto SubscriptAccessorBody =
      some(append<&ast::SubscriptDecl::accessors>(AccessorClause)) |
      append<&ast::SubscriptDecl::accessors>(
          create<ast::Accessor>(many(append<&ast::Accessor::body>(ref<Statement>))));

  struct VariableDecl
      : rule<create<ast::VariableDecl>(
            many(append<&ast::Decl::attributes>(ref<Attribute_>)) +
            many(append<&ast::Decl::modifiers>(ref<Modifier_>)) +
            ((kw<"var"> +
              append<&ast::VariableDecl::bindings>(create<ast::PatternBinding>(
                  assign<&ast::PatternBinding::pattern>(create<
                                                        ast::IdentifierPattern>(
                      assign<&ast::IdentifierPattern::name>(ref<Identifier>) +
                      option(assign<&ast::IdentifierPattern::type>(
                          TypeAnnotationType)))) +
                  option(lit<"="> + assign<&ast::PatternBinding::initializer>(
                                        ref<Expression>)))) +
              lit<"{"> + VarAccessorBody + lit<"}">) |
             ((flag<&ast::VariableDecl::isConstant>(kw<"let">) |
               kw<"var">)+some(append<&ast::VariableDecl::
                                          bindings>(ref<PatternInitializer>),
                               lit<",">))))> {};

  struct TypealiasDecl
      : rule<create<ast::TypealiasDecl>(
            many(append<&ast::Decl::attributes>(ref<Attribute_>)) +
            many(append<&ast::Decl::modifiers>(ref<Modifier_>)) +
            kw<"typealias"> +
            assign<&ast::TypealiasDecl::name>(ref<Identifier>) +
            option(assign<&ast::TypealiasDecl::generics>(
                ref<GenericParamClause>)) +
            lit<"="> + assign<&ast::TypealiasDecl::target>(ref<Type>))> {};

  struct Param
      : rule<create<ast::Param>(
            many(append<&ast::Param::attributes>(ref<Attribute_>)) +
            ((assign<&ast::Param::externalName>(ref<Identifier>) +
              assign<&ast::Param::localName>(ref<Identifier>)) |
             assign<&ast::Param::localName>(ref<Identifier>)) +
            lit<":"> + many(append<&ast::Param::attributes>(ref<Attribute_>)) +
            option(flag<&ast::Param::inoutFlag>(kw<"inout">)) +
            many(append<&ast::Param::specifiers>(ParameterSpecifier)) +
            assign<&ast::Param::type>(ref<Type>) +
            option((lit<"="> +
                       assign<&ast::Param::defaultValue>(ref<Expression>)) |
                   flag<&ast::Param::variadic>(lit<"...">)))> {};

  struct FunctionDecl
      : rule<create<ast::FunctionDecl>(
            many(append<&ast::Decl::attributes>(ref<Attribute_>)) +
            many(append<&ast::Decl::modifiers>(ref<Modifier_>)) +
            kw<"func"> +
            assign<&ast::FunctionDecl::name>(ref<Identifier> | ref<OPERATOR> |
                                             lit<"="> | lit<"<"> | lit<">">) +
            option(assign<&ast::FunctionDecl::generics>(ref<GenericParamClause>)) +
            lit<"("> +
            option(some(append<&ast::FunctionDecl::params>(ref<Param>), lit<",">) + option(lit<",">)) +
            lit<")"> +
            Effects<ast::FunctionDecl> +
            option(lit<"->"> +
                   many(append<&ast::Decl::attributes>(ref<Attribute_>)) +
                   assign<&ast::FunctionDecl::resultType>(ref<Type>)) +
            option(kw<"where"> +
                   some(append<&ast::FunctionDecl::whereClause>(ref<Requirement>),
                        lit<",">)) +
            option(flag<&ast::FunctionDecl::hasBody>(lit<"{">) +
                   many(append<&ast::FunctionDecl::body>(ref<Statement>)) + lit<"}">))> {
  };

  struct EnumCaseElement
      : rule<create<ast::EnumCaseElement>(
                 assign<&ast::EnumCaseElement::name>(ref<Identifier>) +
                 option(assign<&ast::EnumCaseElement::associatedValue>(
                            ref<EnumCaseParameterClause>) |
                        (lit<"="> +
                         assign<&ast::EnumCaseElement::rawValue>(ref<Expression>))))> {};
  struct EnumCaseDecl
      : rule<create<ast::EnumCaseDecl>(
            many(append<&ast::Decl::attributes>(ref<Attribute_>)) +
            many(append<&ast::Decl::modifiers>(ref<Modifier_>)) +
            kw<"case"> +
            some(append<&ast::EnumCaseDecl::cases>(ref<EnumCaseElement>), lit<",">))> {
  };
  struct EnumDecl
      : rule<create<ast::EnumDecl>(
            many(append<&ast::Decl::attributes>(ref<Attribute_>)) +
            many(append<&ast::Decl::modifiers>(ref<Modifier_>)) +
            kw<"enum"> + assign<&ast::EnumDecl::name>(ref<Identifier>) +
            option(assign<&ast::EnumDecl::generics>(ref<GenericParamClause>)) +
            option(lit<":"> +
                   some(append<&ast::EnumDecl::inheritance>(ref<TypeIdentifier>), lit<",">)) +
            option(kw<"where"> +
                   some(append<&ast::EnumDecl::whereClause>(ref<Requirement>),
                        lit<",">)) +
            lit<"{"> +
            many(append<&ast::EnumDecl::members>(
                ref<EnumCaseDecl> | ref<Declaration> | ref<CompilerControlStatement>)) +
            lit<"}">)> {};

  struct StructDecl
      : rule<create<ast::StructDecl>(
            many(append<&ast::Decl::attributes>(ref<Attribute_>)) +
            many(append<&ast::Decl::modifiers>(ref<Modifier_>)) +
            kw<"struct"> + assign<&ast::StructDecl::name>(ref<Identifier>) +
            option(assign<&ast::StructDecl::generics>(ref<GenericParamClause>)) +
            option(lit<":"> +
                   some(append<&ast::StructDecl::inheritance>(ref<TypeIdentifier>), lit<",">)) +
            option(kw<"where"> +
                   some(append<&ast::StructDecl::whereClause>(ref<Requirement>),
                        lit<",">)) +
            lit<"{"> +
            many(append<&ast::StructDecl::members>(
                ref<Declaration> | ref<CompilerControlStatement>)) +
            lit<"}">)> {};

  struct ClassDecl
      : rule<create<ast::ClassDecl>(
            many(append<&ast::Decl::attributes>(ref<Attribute_>)) +
            many(append<&ast::Decl::modifiers>(ref<Modifier_>)) +
            kw<"class"> + assign<&ast::ClassDecl::name>(ref<Identifier>) +
            option(assign<&ast::ClassDecl::generics>(ref<GenericParamClause>)) +
            option(lit<":"> +
                   some(append<&ast::ClassDecl::inheritance>(ref<TypeIdentifier>), lit<",">)) +
            option(kw<"where"> +
                   some(append<&ast::ClassDecl::whereClause>(ref<Requirement>),
                        lit<",">)) +
            lit<"{"> +
            many(append<&ast::ClassDecl::members>(
                ref<Declaration> | ref<CompilerControlStatement>)) +
            lit<"}">)> {};

  struct ActorDecl
      : rule<create<ast::ActorDecl>(
            many(append<&ast::Decl::attributes>(ref<Attribute_>)) +
            many(append<&ast::Decl::modifiers>(ref<Modifier_>)) +
            kw<"actor"> + assign<&ast::ActorDecl::name>(ref<Identifier>) +
            option(assign<&ast::ActorDecl::generics>(ref<GenericParamClause>)) +
            option(lit<":"> + some(append<&ast::ActorDecl::inheritance>(ref<TypeIdentifier>), lit<",">)) +
            option(kw<"where"> + some(append<&ast::ActorDecl::whereClause>(ref<Requirement>), lit<",">)) +
            lit<"{"> + many(append<&ast::ActorDecl::members>(ref<Declaration> | ref<CompilerControlStatement>)) + lit<"}">)> {};

  struct AssociatedTypeDecl
      : rule<create<ast::AssociatedTypeDecl>(
            many(append<&ast::Decl::attributes>(ref<Attribute_>)) +
            many(append<&ast::Decl::modifiers>(ref<Modifier_>)) +
            kw<"associatedtype"> +
            assign<&ast::AssociatedTypeDecl::name>(ref<Identifier>) +
            option(lit<":"> +
                   append<&ast::AssociatedTypeDecl::inheritance>(ref<TypeIdentifier>) +
                   many(lit<"&"> + append<&ast::AssociatedTypeDecl::inheritance>(
                                       ref<TypeIdentifier>))) +
            option(lit<"="> +
                   assign<&ast::AssociatedTypeDecl::defaultType>(ref<Type>)) +
            option(kw<"where"> +
                   some(append<&ast::AssociatedTypeDecl::whereClause>(
                            ref<Requirement>),
                        lit<",">)))> {};
  struct ProtocolMember
      : rule<ref<Declaration> | ref<AssociatedTypeDecl> | ref<CompilerControlStatement>> {};

  struct ProtocolDecl
      : rule<create<ast::ProtocolDecl>(
            many(append<&ast::Decl::attributes>(ref<Attribute_>)) +
            many(append<&ast::Decl::modifiers>(ref<Modifier_>)) +
            kw<"protocol"> + assign<&ast::ProtocolDecl::name>(ref<Identifier>) +
            option(lit<":"> + some(append<&ast::ProtocolDecl::inheritance>(ref<TypeIdentifier>), lit<",">)) +
            option(kw<"where"> +
                   some(append<&ast::ProtocolDecl::whereClause>(ref<Requirement>),
                        lit<",">)) +
            lit<"{"> +
            many(append<&ast::ProtocolDecl::members>(ref<ProtocolMember>)) +
            lit<"}">)> {};

  struct ExtensionDecl
      : rule<create<ast::ExtensionDecl>(
            many(append<&ast::Decl::attributes>(ref<Attribute_>)) +
            many(append<&ast::Decl::modifiers>(ref<Modifier_>)) +
            kw<"extension"> + assign<&ast::ExtensionDecl::type>(ref<TypeIdentifier>) +
            option(lit<":"> +
                   append<&ast::ExtensionDecl::inheritance>(ref<TypeIdentifier>) +
                   many(lit<","> + append<&ast::ExtensionDecl::inheritance>(
                                       ref<TypeIdentifier>))) +
            option(kw<"where"> +
                   some(append<&ast::ExtensionDecl::whereClause>(ref<Requirement>),
                        lit<",">)) +
            lit<"{"> +
            many(append<&ast::ExtensionDecl::members>(
                ref<Declaration> | ref<CompilerControlStatement>)) +
            lit<"}">)> {};

  struct InitializerDecl
      : rule<create<ast::InitializerDecl>(
            many(append<&ast::Decl::attributes>(ref<Attribute_>)) +
            many(append<&ast::Decl::modifiers>(ref<Modifier_>)) +
            kw<"init"> +
            option(assign<&ast::InitializerDecl::failable>(lit<"?"> | lit<"!">)) +
            option(assign<&ast::InitializerDecl::generics>(ref<GenericParamClause>)) +
            lit<"("> +
            option(some(append<&ast::InitializerDecl::params>(ref<Param>), lit<",">) + option(lit<",">)) +
            lit<")"> +
            Effects<ast::InitializerDecl> +
            option(kw<"where"> +
                   some(append<&ast::InitializerDecl::whereClause>(ref<Requirement>),
                        lit<",">)) +
            option(flag<&ast::InitializerDecl::hasBody>(lit<"{">) +
                   many(append<&ast::InitializerDecl::body>(ref<Statement>)) + lit<"}">))> {};

  struct DeinitializerDecl
      : rule<create<ast::DeinitializerDecl>(
            many(append<&ast::Decl::attributes>(ref<Attribute_>)) +
            kw<"deinit"> + lit<"{"> +
            many(append<&ast::DeinitializerDecl::body>(ref<Statement>)) + lit<"}">)> {
  };

  struct SubscriptDecl
      : rule<create<ast::SubscriptDecl>(
            many(append<&ast::Decl::attributes>(ref<Attribute_>)) +
            many(append<&ast::Decl::modifiers>(ref<Modifier_>)) +
            kw<"subscript"> +
            option(assign<&ast::SubscriptDecl::generics>(ref<GenericParamClause>)) +
            lit<"("> +
            option(some(append<&ast::SubscriptDecl::params>(ref<Param>), lit<",">) + option(lit<",">)) +
            lit<")"> + lit<"->"> +
            many(append<&ast::Decl::attributes>(ref<Attribute_>)) +
            assign<&ast::SubscriptDecl::resultType>(ref<Type>) +
            option(kw<"where"> +
                   some(append<&ast::SubscriptDecl::whereClause>(ref<Requirement>),
                        lit<",">)) +
            lit<"{"> + SubscriptAccessorBody +
            lit<"}">)> {};

  struct OperatorDecl
      : rule<create<ast::OperatorDecl>(
            assign<&ast::OperatorDecl::kind>(kw<"prefix"> | kw<"postfix">) +
            kw<"operator"> + assign<&ast::OperatorDecl::op>(ref<OPERATOR>)) |
            create<ast::OperatorDecl>(
                assign<&ast::OperatorDecl::kind>(kw<"infix">) + kw<"operator"> +
                assign<&ast::OperatorDecl::op>(ref<OPERATOR>) +
                option(lit<":"> + assign<&ast::OperatorDecl::precedenceGroup>(
                                      ref<Identifier>)))> {};
  static constexpr auto PrecedenceGroupAttribute =
      (kw<"higherThan"> + lit<":"> +
       append<&ast::PrecedenceGroupDecl::higherThan>(ref<Identifier>) +
       many(lit<","> + append<&ast::PrecedenceGroupDecl::higherThan>(
                           ref<Identifier>))) |
      (kw<"lowerThan"> + lit<":"> +
       append<&ast::PrecedenceGroupDecl::lowerThan>(ref<Identifier>) +
       many(lit<","> + append<&ast::PrecedenceGroupDecl::lowerThan>(
                           ref<Identifier>))) |
      (kw<"assignment"> + lit<":"> +
       (flag<&ast::PrecedenceGroupDecl::isAssignment>(kw<"true">) |
        kw<"false">));
  struct PrecedenceGroupDecl
      : rule<create<ast::PrecedenceGroupDecl>(
            kw<"precedencegroup"> +
            assign<&ast::PrecedenceGroupDecl::name>(ref<Identifier>) + lit<"{"> +
            many(PrecedenceGroupAttribute) +
            option(kw<"associativity"> + lit<":"> +
                   assign<&ast::PrecedenceGroupDecl::associativity>(
                       kw<"left"> | kw<"right"> | kw<"none">) +
                   many(PrecedenceGroupAttribute)) +
            lit<"}">)> {};

  struct Declaration
      : rule<(ref<ImportDecl>) |
             (ref<VariableDecl> + eos) | (ref<TypealiasDecl> + eos) |
             (ref<FunctionDecl> + eos) | (ref<EnumDecl> + eos) |
             (ref<StructDecl> + eos) | (ref<ClassDecl> + eos) | (ref<ActorDecl> + eos) |
             (ref<ProtocolDecl> + eos) | (ref<InitializerDecl> + eos) |
             (ref<DeinitializerDecl> + eos) | (ref<ExtensionDecl> + eos) |
             (ref<SubscriptDecl> + eos) | (ref<OperatorDecl> + eos) |
             (ref<PrecedenceGroupDecl> + eos)> {};

  struct File
      : rule<create<ast::File> + option(lit<"\xEF\xBB\xBF">) +
             option(assign<&ast::File::hashbang>(ref<HASHBANG>)) +
             many(append<&ast::File::statements>(ref<Statement>))> {
  };

  using entry = File;
  using ignored = list<WS>;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace swift
