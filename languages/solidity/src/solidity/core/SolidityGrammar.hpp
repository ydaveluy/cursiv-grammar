// The solidity language: its grammar, including inline assembly (Yul).
// Lexing is context-aware: assembly and pragma bodies need no mode of
// their own, since a state only ever reads the terminals it accepts.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "solidity/core/ast.hpp"

namespace solidity {

struct Solidity : cursiv::GrammarSpec {
  static constexpr auto word_boundary = cr<"a-zA-Z0-9_$">;

  struct WS : terminal<some(s)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  struct ID : terminal<cr<"a-zA-Z_$"> + many(cr<"a-zA-Z0-9_$">)> {};

  static constexpr auto HexChar = cr<"0-9A-Fa-f">;
  static constexpr auto DecimalDigits = cr<"0-9"> + many(cr<"0-9_">);
  static constexpr auto HexDigitsRun = HexChar + many(HexChar | lit<"_">);
  static constexpr auto EvenHexDigits =
      HexChar + HexChar + many(option(lit<"_">) + HexChar + HexChar);

  // Refuses a further identifier byte right after the number: `1x` is one
  // invalid token, not `1` then an identifier `x`.
  struct DECIMAL_NUMBER
      : terminal<(DecimalDigits |
                  (option(DecimalDigits) + lit<"."> + DecimalDigits)) +
                 option(cr<"eE"> + option(cr<"+-">) + DecimalDigits) +
                 !cr<"a-zA-Z0-9_$">> {};
  struct HEX_NUMBER : terminal<lit<"0x"> + HexDigitsRun + !cr<"a-zA-Z0-9_$">> {
  };

  struct HEX_STRING
      : terminal<lit<"hex"> + ((lit<"\""> + option(EvenHexDigits) + lit<"\"">) |
                               (lit<"'"> + option(EvenHexDigits) + lit<"'">))> {
  };

  static constexpr auto Escape =
      lit<"\\"> + (lit<"\r\n"> | cr<R"('"\x5cnrt\x0a\x0d)"> |
                   (lit<"u"> + repeat<4>(HexChar)) |
                   (lit<"x"> + repeat<2>(HexChar)));
  static constexpr auto DQPrintable = cr<R"(\x20-\x21\x23-\x5b\x5d-\x7e)">;
  static constexpr auto SQPrintable = cr<R"(\x20-\x26\x28-\x5b\x5d-\x7e)">;

  // Empty and non-empty quoted strings are one token: `many` already
  // covers the empty case.
  struct STRING_LIT
      : terminal<(lit<"\""> + many(DQPrintable | Escape) + lit<"\"">) |
                 (lit<"'"> + many(SQPrintable | Escape) + lit<"'">)> {};

  static constexpr auto DQUniPrintable = ~cr<R"("\x5c\x0a\x0d)">;
  static constexpr auto SQUniPrintable = ~cr<R"('\x5c\x0a\x0d)">;
  struct UNICODE_STRING_LIT
      : terminal<lit<"unicode"> +
                 ((lit<"\""> + many(DQUniPrintable | Escape) + lit<"\"">) |
                  (lit<"'"> + many(SQUniPrintable | Escape) + lit<"'">))> {};

  static constexpr auto IntegerWidth =
      lit<"8"> | lit<"16"> | lit<"24"> | lit<"32"> | lit<"40"> | lit<"48"> |
      lit<"56"> | lit<"64"> | lit<"72"> | lit<"80"> | lit<"88"> | lit<"96"> |
      lit<"104"> | lit<"112"> | lit<"120"> | lit<"128"> | lit<"136"> |
      lit<"144"> | lit<"152"> | lit<"160"> | lit<"168"> | lit<"176"> |
      lit<"184"> | lit<"192"> | lit<"200"> | lit<"208"> | lit<"216"> |
      lit<"224"> | lit<"232"> | lit<"240"> | lit<"248"> | lit<"256">;
  static constexpr auto FixedDecimals = d | (cr<"1-7"> + d) | lit<"80">;
  static constexpr auto BytesSize =
      cr<"1-9"> | (cr<"12"> + d) | (lit<"3"> + cr<"0-2">);
  struct INTEGER_TYPE
      : terminal<(lit<"int"> | lit<"uint">)+option(IntegerWidth) +
                 !word_boundary> {};
  struct FIXED_BYTES_TYPE
      : terminal<lit<"bytes"> + BytesSize + !word_boundary> {};
  struct FIXED_TYPE : terminal<(lit<"fixed"> | lit<"ufixed">)+option(
                                   IntegerWidth + lit<"x"> + FixedDecimals) +
                               !word_boundary> {};

  static constexpr auto ElementaryTypeWord =
      kw<"bool"> | kw<"string"> | kw<"address"> | kw<"bytes"> |
      ref<INTEGER_TYPE> | ref<FIXED_BYTES_TYPE> | ref<FIXED_TYPE>;

  static constexpr auto DataLocationWord =
      kw<"memory"> | kw<"storage"> | kw<"calldata">;
  static constexpr auto VisibilityWord =
      kw<"internal"> | kw<"external"> | kw<"private"> | kw<"public">;
  static constexpr auto StateMutabilityWord =
      kw<"pure"> | kw<"view"> | kw<"payable">;
  static constexpr auto SubDenominationWord =
      kw<"wei"> | kw<"gwei"> | kw<"ether"> | kw<"seconds"> | kw<"minutes"> |
      kw<"hours"> | kw<"days"> | kw<"weeks"> | kw<"years">;
  static constexpr auto UserDefinableOperatorWord =
      lit<"&"> | lit<"~"> | lit<"|"> | lit<"^"> | lit<"+"> | lit<"/"> |
      lit<"%"> | lit<"*"> | lit<"-"> | lit<"=="> | lit<">="> | lit<">"> |
      lit<"<="> | lit<"<"> | lit<"!=">;

  // `from`, `error`, `revert`, `global` and `transient` are keywords where
  // the grammar spells them and plain names everywhere else -- one terminal
  // for the spelling, admitted here too.
  struct Identifier
      : rule<ref<ID> | kw<"from"> | kw<"error"> | kw<"revert"> | kw<"global"> |
             kw<"transient"> | kw<"layout"> | kw<"at">> {};

  struct IdentifierPath
      : rule<ref<Identifier> + many(lit<"."> + ref<Identifier>)> {};

  struct Expression;
  struct TypeNameRule;
  struct Statement;
  struct Block;
  struct YulStatement;
  struct YulBlock;
  struct YulExpression;
  struct ElementaryTypeRule;
  struct TupleExprRule;
  struct InlineArrayExprRule;
  struct NumberLitRule;
  struct BoolLitRule;
  struct StringLitRule;
  struct HexStringLitRule;
  struct UnicodeStringLitRule;
  struct UncheckedBlockRule;

  // A pragma's payload is any run of bytes that is not `;`, scanned raw:
  // solc parses only a subset of it, so this grammar does not either.
  struct PRAGMA_BODY : terminal<some(cr<"^;">)> {};

  struct PragmaDirectiveRule
      : rule<create<ast::PragmaDirective>(
            kw<"pragma"> +
            assign<&ast::PragmaDirective::text>(ref<PRAGMA_BODY>) + lit<";">)> {
  };

  struct PathRule : rule<ref<STRING_LIT>> {};

  struct ImportAliasRule
      : rule<create<ast::ImportAlias>(
            assign<&ast::ImportAlias::symbol>(ref<Identifier>) +
            option(kw<"as"> +
                   assign<&ast::ImportAlias::alias>(ref<Identifier>)))> {};

  // Spliced, not a `ref<>`'d rule: its `append<>` targets the enclosing
  // ImportDirective, so it must share that rule's own node instead of
  // resolving one of its own.
  static constexpr auto SymbolAliasesExpr =
      lit<"{"> +
      some(append<&ast::ImportDirective::symbols>(ref<ImportAliasRule>),
           lit<",">) +
      lit<"}">;

  struct ImportDirectiveRule
      : rule<create<ast::ImportDirective>(
            kw<"import"> +
            ((assign<&ast::ImportDirective::path>(ref<PathRule>) +
              option(kw<"as"> + assign<&ast::ImportDirective::unitAlias>(
                                    ref<Identifier>))) |
             (SymbolAliasesExpr + kw<"from"> +
              assign<&ast::ImportDirective::path>(ref<PathRule>)) |
             (flag<&ast::ImportDirective::isStar>(lit<"*">) + kw<"as"> +
              assign<&ast::ImportDirective::unitAlias>(ref<Identifier>) +
              kw<"from"> +
              assign<&ast::ImportDirective::path>(ref<PathRule>))) +
            lit<";">)> {};

  struct NamedArgumentRule
      : rule<create<ast::NamedArgument>(
            assign<&ast::NamedArgument::name>(ref<Identifier>) + lit<":"> +
            assign<&ast::NamedArgument::value>(ref<Expression>))> {};

  struct CallArgumentsRule
      : rule<create<ast::CallArguments>(
            lit<"("> +
            (option(
                 some(append<&ast::CallArguments::positional>(ref<Expression>),
                      lit<",">)) |
             (lit<"{"> +
              option(some(
                  append<&ast::CallArguments::named>(ref<NamedArgumentRule>),
                  lit<",">)) +
              lit<"}">)) +
            lit<")">)> {};

  struct InheritanceSpecifierRule
      : rule<create<ast::InheritanceSpecifier>(
            assign<&ast::InheritanceSpecifier::name>(ref<IdentifierPath>) +
            option(assign<&ast::InheritanceSpecifier::arguments>(
                ref<CallArgumentsRule>)))> {};

  struct ContractBodyElement;

  struct StorageLayoutSpecifierRule
      : rule<create<ast::StorageLayoutSpecifier>(
            kw<"layout"> + kw<"at"> +
            assign<&ast::StorageLayoutSpecifier::baseSlot>(ref<Expression>))> {
  };

  struct ContractDefinitionRule
      : rule<create<ast::ContractDefinition>(
            option(flag<&ast::ContractDefinition::isAbstract>(
                kw<"abstract">)) +
            kw<"contract"> +
            assign<&ast::ContractDefinition::name>(ref<Identifier>) +
            many((kw<"is"> +
                  some(append<&ast::ContractDefinition::baseContracts>(
                           ref<InheritanceSpecifierRule>),
                       lit<",">)) |
                 append<&ast::ContractDefinition::storageLayouts>(
                     ref<StorageLayoutSpecifierRule>)) +
            lit<"{"> +
            many(append<&ast::ContractDefinition::body>(
                ref<ContractBodyElement>)) +
            lit<"}">)> {};

  struct InterfaceDefinitionRule
      : rule<create<ast::InterfaceDefinition>(
            kw<"interface"> +
            assign<&ast::InterfaceDefinition::name>(ref<Identifier>) +
            option(kw<"is"> +
                   some(append<&ast::InterfaceDefinition::baseContracts>(
                            ref<InheritanceSpecifierRule>),
                        lit<",">)) +
            lit<"{"> +
            many(append<&ast::InterfaceDefinition::body>(
                ref<ContractBodyElement>)) +
            lit<"}">)> {};

  struct LibraryDefinitionRule
      : rule<create<ast::LibraryDefinition>(
            kw<"library"> +
            assign<&ast::LibraryDefinition::name>(ref<Identifier>) + lit<"{"> +
            many(append<&ast::LibraryDefinition::body>(
                ref<ContractBodyElement>)) +
            lit<"}">)> {};

  struct ParameterDecl
      : rule<create<ast::Parameter>(
            assign<&ast::Parameter::type>(ref<TypeNameRule>) +
            option(assign<&ast::Parameter::location>(DataLocationWord)) +
            option(assign<&ast::Parameter::name>(ref<Identifier>)))> {};

  struct ModifierInvocationRule
      : rule<create<ast::ModifierInvocation>(
            assign<&ast::ModifierInvocation::name>(ref<IdentifierPath>) +
            option(assign<&ast::ModifierInvocation::arguments>(
                ref<CallArgumentsRule>)))> {};

  struct OverrideSpecifierRule
      : rule<create<ast::OverrideSpecifier>(
            kw<"override"> + option(lit<"("> +
                                    some(append<&ast::OverrideSpecifier::bases>(
                                             ref<IdentifierPath>),
                                         lit<",">) +
                                    lit<")">))> {};

  struct ConstructorDefinitionRule
      : rule<create<ast::ConstructorDefinition>(
            kw<"constructor"> + lit<"("> +
            option(some(append<&ast::ConstructorDefinition::parameters>(
                            ref<ParameterDecl>),
                        lit<",">)) +
            lit<")"> +
            many(append<&ast::ConstructorDefinition::modifiers>(
                     ref<ModifierInvocationRule>) |
                 append<&ast::ConstructorDefinition::modifiers>(
                     kw<"payable"> | kw<"internal"> | kw<"public">)) +
            assign<&ast::ConstructorDefinition::body>(ref<Block>))> {};

  struct FunctionDefinitionRule
      : rule<create<ast::FunctionDefinition>(
            kw<"function"> +
            assign<&ast::FunctionDefinition::name>(
                ref<Identifier> | kw<"fallback"> | kw<"receive">) +
            lit<"("> +
            option(some(append<&ast::FunctionDefinition::parameters>(
                            ref<ParameterDecl>),
                        lit<",">)) +
            lit<")"> +
            many(append<&ast::FunctionDefinition::modifiers>(VisibilityWord) |
                 append<&ast::FunctionDefinition::modifiers>(
                     StateMutabilityWord) |
                 append<&ast::FunctionDefinition::modifiers>(
                     ref<ModifierInvocationRule>) |
                 append<&ast::FunctionDefinition::modifiers>(kw<"virtual">) |
                 append<&ast::FunctionDefinition::modifiers>(
                     ref<OverrideSpecifierRule>)) +
            option(kw<"returns"> + lit<"("> +
                   some(append<&ast::FunctionDefinition::returnParameters>(
                            ref<ParameterDecl>),
                        lit<",">) +
                   lit<")">) +
            (lit<";"> | assign<&ast::FunctionDefinition::body>(ref<Block>)))> {
  };

  struct ModifierDefinitionRule
      : rule<create<ast::ModifierDefinition>(
            kw<"modifier"> +
            assign<&ast::ModifierDefinition::name>(ref<Identifier>) +
            option(flag<&ast::ModifierDefinition::hasParameters>(
                lit<"("> +
                option(some(append<&ast::ModifierDefinition::parameters>(
                                ref<ParameterDecl>),
                            lit<",">)) +
                lit<")">)) +
            many(append<&ast::ModifierDefinition::modifiers>(kw<"virtual">) |
                 append<&ast::ModifierDefinition::modifiers>(
                     ref<OverrideSpecifierRule>)) +
            (lit<";"> | assign<&ast::ModifierDefinition::body>(ref<Block>)))> {
  };

  struct FallbackFunctionDefinitionRule
      : rule<create<ast::FallbackFunctionDefinition>(
            kw<"fallback"> + lit<"("> +
            option(some(append<&ast::FallbackFunctionDefinition::parameters>(
                            ref<ParameterDecl>),
                        lit<",">)) +
            lit<")"> +
            many(append<&ast::FallbackFunctionDefinition::modifiers>(
                     kw<"external">) |
                 append<&ast::FallbackFunctionDefinition::modifiers>(
                     StateMutabilityWord) |
                 append<&ast::FallbackFunctionDefinition::modifiers>(
                     ref<ModifierInvocationRule>) |
                 append<&ast::FallbackFunctionDefinition::modifiers>(
                     kw<"virtual">) |
                 append<&ast::FallbackFunctionDefinition::modifiers>(
                     ref<OverrideSpecifierRule>)) +
            option(
                kw<"returns"> + lit<"("> +
                some(append<&ast::FallbackFunctionDefinition::returnParameters>(
                         ref<ParameterDecl>),
                     lit<",">) +
                lit<")">) +
            (lit<";"> |
             assign<&ast::FallbackFunctionDefinition::body>(ref<Block>)))> {};

  struct ReceiveFunctionDefinitionRule
      : rule<create<ast::ReceiveFunctionDefinition>(
            kw<"receive"> + lit<"("> + lit<")"> +
            many(append<&ast::ReceiveFunctionDefinition::modifiers>(
                     kw<"external">) |
                 append<&ast::ReceiveFunctionDefinition::modifiers>(
                     kw<"payable">) |
                 append<&ast::ReceiveFunctionDefinition::modifiers>(
                     ref<ModifierInvocationRule>) |
                 append<&ast::ReceiveFunctionDefinition::modifiers>(
                     kw<"virtual">) |
                 append<&ast::ReceiveFunctionDefinition::modifiers>(
                     ref<OverrideSpecifierRule>)) +
            (lit<";"> |
             assign<&ast::ReceiveFunctionDefinition::body>(ref<Block>)))> {};

  struct StructMemberRule
      : rule<create<ast::StructMember>(
            assign<&ast::StructMember::type>(ref<TypeNameRule>) +
            assign<&ast::StructMember::name>(ref<Identifier>) + lit<";">)> {};

  struct StructDefinitionRule
      : rule<create<ast::StructDefinition>(
            kw<"struct"> +
            assign<&ast::StructDefinition::name>(ref<Identifier>) + lit<"{"> +
            some(append<&ast::StructDefinition::members>(
                ref<StructMemberRule>)) +
            lit<"}">)> {};

  struct EnumDefinitionRule
      : rule<create<ast::EnumDefinition>(
            kw<"enum"> + assign<&ast::EnumDefinition::name>(ref<Identifier>) +
            lit<"{"> +
            some(append<&ast::EnumDefinition::values>(ref<Identifier>),
                 lit<",">) +
            lit<"}">)> {};

  struct UserDefinedValueTypeDefinitionRule
      : rule<create<ast::UserDefinedValueTypeDefinition>(
            kw<"type"> +
            assign<&ast::UserDefinedValueTypeDefinition::name>(
                ref<Identifier>) +
            kw<"is"> +
            assign<&ast::UserDefinedValueTypeDefinition::underlying>(
                ref<TypeNameRule>) +
            lit<";">)> {};

  struct StateVariableDeclarationRule
      : rule<create<ast::StateVariableDeclaration>(
            assign<&ast::StateVariableDeclaration::type>(ref<TypeNameRule>) +
            many(append<&ast::StateVariableDeclaration::modifiers>(
                     kw<"public"> | kw<"private"> | kw<"internal"> |
                     kw<"constant"> | kw<"immutable"> | kw<"transient">) |
                 append<&ast::StateVariableDeclaration::modifiers>(
                     ref<OverrideSpecifierRule>)) +
            assign<&ast::StateVariableDeclaration::name>(ref<Identifier>) +
            option(lit<"="> +
                   assign<&ast::StateVariableDeclaration::initialValue>(
                       ref<Expression>)) +
            lit<";">)> {};

  struct ConstantVariableDeclarationRule
      : rule<create<ast::ConstantVariableDeclaration>(
            assign<&ast::ConstantVariableDeclaration::type>(ref<TypeNameRule>) +
            kw<"constant"> +
            assign<&ast::ConstantVariableDeclaration::name>(ref<Identifier>) +
            option(lit<"="> +
                   assign<&ast::ConstantVariableDeclaration::initialValue>(
                       ref<Expression>)) +
            lit<";">)> {};

  struct EventParameterRule
      : rule<create<ast::EventParameter>(
            assign<&ast::EventParameter::type>(ref<TypeNameRule>) +
            option(flag<&ast::EventParameter::indexed>(kw<"indexed">)) +
            option(assign<&ast::EventParameter::name>(ref<Identifier>)))> {};

  struct EventDefinitionRule
      : rule<create<ast::EventDefinition>(
            kw<"event"> + assign<&ast::EventDefinition::name>(ref<Identifier>) +
            lit<"("> +
            option(some(append<&ast::EventDefinition::parameters>(
                            ref<EventParameterRule>),
                        lit<",">)) +
            lit<")"> +
            option(flag<&ast::EventDefinition::isAnonymous>(
                kw<"anonymous">)) +
            lit<";">)> {};

  struct ErrorParameterRule
      : rule<create<ast::ErrorParameter>(
            assign<&ast::ErrorParameter::type>(ref<TypeNameRule>) +
            option(assign<&ast::ErrorParameter::name>(ref<Identifier>)))> {};

  struct ErrorDefinitionRule
      : rule<create<ast::ErrorDefinition>(
            kw<"error"> + assign<&ast::ErrorDefinition::name>(ref<Identifier>) +
            lit<"("> +
            option(some(append<&ast::ErrorDefinition::parameters>(
                            ref<ErrorParameterRule>),
                        lit<",">)) +
            lit<")"> + lit<";">)> {};

  struct UsingAliasRule
      : rule<create<ast::UsingAlias>(
            assign<&ast::UsingAlias::path>(ref<IdentifierPath>) +
            option(kw<"as"> +
                   assign<&ast::UsingAlias::op>(UserDefinableOperatorWord)))> {
  };

  struct UsingDirectiveRule
      : rule<create<ast::UsingDirective>(
            kw<"using"> +
            (assign<&ast::UsingDirective::libraryPath>(ref<IdentifierPath>) |
             (lit<"{"> +
              some(append<&ast::UsingDirective::aliases>(ref<UsingAliasRule>),
                   lit<",">) +
              lit<"}">)) +
            kw<"for"> +
            (flag<&ast::UsingDirective::forAny>(lit<"*">) |
             assign<&ast::UsingDirective::forType>(ref<TypeNameRule>)) +
            option(flag<&ast::UsingDirective::isGlobal>(kw<"global">)) +
            lit<";">)> {};

  struct ElementaryTypeRule
      : rule<create<ast::ElementaryType>(
            assign<&ast::ElementaryType::text>(ElementaryTypeWord) +
            option(assign<&ast::ElementaryType::mutability>(
                StateMutabilityWord)))> {};

  struct MappingKeyTypeRule
      : rule<ref<ElementaryTypeRule> |
             create<ast::NamedType>(
                 assign<&ast::NamedType::path>(ref<IdentifierPath>))> {};

  struct MappingTypeRule
      : rule<create<ast::MappingType>(
            kw<"mapping"> + lit<"("> +
            assign<&ast::MappingType::keyType>(ref<MappingKeyTypeRule>) +
            option(assign<&ast::MappingType::keyName>(ref<Identifier>)) +
            lit<"=>"> +
            assign<&ast::MappingType::valueType>(ref<TypeNameRule>) +
            option(assign<&ast::MappingType::valueName>(ref<Identifier>)) +
            lit<")">)> {};

  struct FunctionTypeRule
      : rule<create<ast::FunctionType>(
            kw<"function"> + lit<"("> +
            option(
                some(append<&ast::FunctionType::parameters>(ref<ParameterDecl>),
                     lit<",">)) +
            lit<")"> +
            many(append<&ast::FunctionType::visibility>(VisibilityWord) |
                 append<&ast::FunctionType::stateMutability>(
                     StateMutabilityWord)) +
            option(kw<"returns"> + lit<"("> +
                   some(append<&ast::FunctionType::returnParameters>(
                            ref<ParameterDecl>),
                        lit<",">) +
                   lit<")">))> {};

  struct TypeNameRule
      : rule<(ref<FunctionTypeRule> | ref<MappingTypeRule> |
              ref<ElementaryTypeRule> |
              create<ast::NamedType>(
                  assign<&ast::NamedType::path>(ref<IdentifierPath>))) +
             many(nest<ast::ArrayType, &ast::ArrayType::base>(
                 lit<"["> +
                 option(assign<&ast::ArrayType::size>(ref<Expression>)) +
                 lit<"]">))> {};

  // Expressions, tightest to loosest: postfix, prefix, the binary ladder,
  // ternary, assignment. `Expression` is the full, loosest form -- every
  // other rule in the grammar that reads "expression" refers to it.

  struct PrimaryExpr
      : rule<create<ast::NewExpr>(
                 kw<"new"> + assign<&ast::NewExpr::type>(ref<TypeNameRule>)) |
             create<ast::PayableConversionExpr>(
                 kw<"payable"> + assign<&ast::PayableConversionExpr::args>(
                                     ref<CallArgumentsRule>)) |
             create<ast::MetaTypeExpr>(
                 kw<"type"> + lit<"("> +
                 assign<&ast::MetaTypeExpr::type>(ref<TypeNameRule>) +
                 lit<")">) |
             ref<TupleExprRule> | ref<InlineArrayExprRule> |
             ref<NumberLitRule> | ref<BoolLitRule> | ref<StringLitRule> |
             ref<HexStringLitRule> | ref<UnicodeStringLitRule> |
             create<ast::ElementaryTypeExpr>(
                 assign<&ast::ElementaryTypeExpr::text>(ElementaryTypeWord)) |
             create<ast::IdentifierExpr>(
                 assign<&ast::IdentifierExpr::name>(ref<Identifier>))> {};

  struct TupleExprRule
      : rule<create<ast::TupleExpr>(
            lit<"("> +
            append<&ast::TupleExpr::elements>(ref<Expression> |
                                              create<ast::EmptyExpr>) +
            many(lit<","> + append<&ast::TupleExpr::elements>(
                                ref<Expression> | create<ast::EmptyExpr>)) +
            lit<")">)> {};

  struct InlineArrayExprRule
      : rule<create<ast::InlineArrayExpr>(
            lit<"["> +
            many(append<&ast::InlineArrayExpr::elements>(ref<Expression>),
                 lit<",">) +
            lit<"]">)> {};

  struct NumberLitRule
      : rule<create<ast::NumberLit>(
            assign<&ast::NumberLit::text>(ref<DECIMAL_NUMBER> |
                                          ref<HEX_NUMBER>) +
            option(assign<&ast::NumberLit::unit>(SubDenominationWord)))> {};

  struct BoolLitRule
      : rule<create<ast::BoolLit>(flag<&ast::BoolLit::value>(kw<"true">) |
                                  kw<"false">)> {};

  struct StringLitRule
      : rule<create<ast::StringLit>(
            some(append<&ast::StringLit::pieces>(ref<STRING_LIT>)))> {};

  struct HexStringLitRule
      : rule<create<ast::HexStringLit>(
            some(append<&ast::HexStringLit::pieces>(ref<HEX_STRING>)))> {};

  struct UnicodeStringLitRule
      : rule<create<ast::UnicodeStringLit>(some(
            append<&ast::UnicodeStringLit::pieces>(ref<UNICODE_STRING_LIT>)))> {
  };

  struct PostfixExpr
      : rule<ref<PrimaryExpr> +
             many(
                 nest<ast::IndexExpr, &ast::IndexExpr::object>(
                     lit<"["> +
                     assign<&ast::IndexExpr::index>(ref<Expression> |
                                                    create<ast::EmptyExpr>) +
                     lit<"]">) |
                 nest<ast::IndexRangeExpr, &ast::IndexRangeExpr::object>(
                     lit<"["> +
                     assign<&ast::IndexRangeExpr::start>(
                         ref<Expression> | create<ast::EmptyExpr>) +
                     lit<":"> +
                     assign<&ast::IndexRangeExpr::end>(ref<Expression> |
                                                       create<ast::EmptyExpr>) +
                     lit<"]">) |
                 nest<ast::MemberExpr, &ast::MemberExpr::object>(
                     lit<"."> + assign<&ast::MemberExpr::field>(
                                    ref<Identifier> | kw<"address">)) |
                 nest<ast::FunctionCallOptionsExpr,
                      &ast::FunctionCallOptionsExpr::callee>(
                     lit<"{"> +
                     option(some(append<&ast::FunctionCallOptionsExpr::options>(
                                     ref<NamedArgumentRule>),
                                 lit<",">)) +
                     lit<"}">) |
                 nest<ast::CallExpr, &ast::CallExpr::callee>(
                     assign<&ast::CallExpr::args>(ref<CallArgumentsRule>)) |
                 nest<ast::PostfixExpr, &ast::PostfixExpr::operand>(
                     assign<&ast::PostfixExpr::op>(lit<"++"> | lit<"--">)))> {};

  struct UnaryExpr
      : rule<create<ast::PrefixExpr>(
                 assign<&ast::PrefixExpr::op>(lit<"++"> | lit<"--"> | lit<"!"> |
                                              lit<"~"> | kw<"delete"> |
                                              lit<"-">) +
                 assign<&ast::PrefixExpr::operand>(ref<UnaryExpr>)) |
             ref<PostfixExpr>> {};

  static constexpr auto OpExp = lit<"**">;
  static constexpr auto OpMulDivMod = lit<"*"> | lit<"/"> | lit<"%">;
  static constexpr auto OpAddSub = lit<"+"> | lit<"-">;
  static constexpr auto OpShift = lit<"<<"> | lit<">>>"> | lit<">>">;
  static constexpr auto OpBitAnd = lit<"&">;
  static constexpr auto OpBitXor = lit<"^">;
  static constexpr auto OpBitOr = lit<"|">;
  static constexpr auto OpCompare = lit<"<="> | lit<">="> | lit<"<"> | lit<">">;
  static constexpr auto OpEquality = lit<"=="> | lit<"!=">;
  static constexpr auto OpAnd = lit<"&&">;
  static constexpr auto OpOr = lit<"||">;

  struct BinaryExpr : infix<UnaryExpr, &ast::BinaryExpr::left,
                            &ast::BinaryExpr::op, &ast::BinaryExpr::right,
                            (right(OpExp) >>       //
                             left(OpMulDivMod) >>  //
                             left(OpAddSub) >>     //
                             left(OpShift) >>      //
                             left(OpBitAnd) >>     //
                             left(OpBitXor) >>     //
                             left(OpBitOr) >>      //
                             left(OpCompare) >>    //
                             left(OpEquality) >>   //
                             left(OpAnd) >>        //
                             left(OpOr))> {};

  struct ConditionalExpr
      : rule<ref<BinaryExpr> +
             option(nest<ast::CondExpr, &ast::CondExpr::cond>(
                 lit<"?"> + assign<&ast::CondExpr::thenValue>(ref<Expression>) +
                 lit<":"> +
                 assign<&ast::CondExpr::elseValue>(ref<Expression>)))> {};

  static constexpr auto AssignOpWord =
      lit<"="> | lit<"|="> | lit<"^="> | lit<"&="> | lit<"<<="> | lit<">>>="> |
      lit<">>="> | lit<"+="> | lit<"-="> | lit<"*="> | lit<"/="> | lit<"%=">;

  struct Expression
      : rule<ref<ConditionalExpr> +
             option(nest<ast::AssignExpr, &ast::AssignExpr::target>(
                 assign<&ast::AssignExpr::op>(AssignOpWord) +
                 assign<&ast::AssignExpr::value>(ref<Expression>)))> {};

  // Statements.

  struct VariableDeclarationRule
      : rule<create<ast::VariableDeclaration>(
            assign<&ast::VariableDeclaration::type>(ref<TypeNameRule>) +
            option(
                assign<&ast::VariableDeclaration::location>(DataLocationWord)) +
            assign<&ast::VariableDeclaration::name>(ref<Identifier>))> {};

  struct VariableDeclarationSlot
      : rule<ref<VariableDeclarationRule> | create<ast::VariableDeclaration>> {
  };
  static constexpr auto VariableDeclarationTupleExpr =
      lit<"("> +
      append<&ast::VariableDeclarationStatement::tuple>(
          ref<VariableDeclarationSlot>) +
      many(lit<","> + append<&ast::VariableDeclarationStatement::tuple>(
                          ref<VariableDeclarationSlot>)) +
      lit<")">;

  struct VariableDeclarationStatementRule
      : rule<create<ast::VariableDeclarationStatement>(
            ((assign<&ast::VariableDeclarationStatement::single>(
                  ref<VariableDeclarationRule>) +
              option(lit<"="> +
                     assign<&ast::VariableDeclarationStatement::initialValue>(
                         ref<Expression>))) |
             (flag<&ast::VariableDeclarationStatement::isTuple>(
                  VariableDeclarationTupleExpr) +
              lit<"="> +
              assign<&ast::VariableDeclarationStatement::initialValue>(
                  ref<Expression>))) +
            lit<";">)> {};

  struct ExpressionStatementRule
      : rule<create<ast::ExpressionStatement>(
            assign<&ast::ExpressionStatement::value>(ref<Expression>) +
            lit<";">)> {};

  static constexpr auto SimpleStatement =
      ref<VariableDeclarationStatementRule> | ref<ExpressionStatementRule>;

  struct Block
      : rule<create<ast::Block>(
            lit<"{"> +
            many(append<&ast::Block::statements>(ref<Statement>) |
                 append<&ast::Block::statements>(ref<UncheckedBlockRule>)) +
            lit<"}">)> {};

  struct UncheckedBlockRule
      : rule<create<ast::UncheckedBlock>(
            kw<"unchecked"> + assign<&ast::UncheckedBlock::body>(ref<Block>))> {
  };

  struct IfStatementRule
      : rule<create<ast::IfStmt>(
            kw<"if"> + lit<"("> + assign<&ast::IfStmt::cond>(ref<Expression>) +
            lit<")"> + assign<&ast::IfStmt::thenBranch>(ref<Statement>) +
            option(kw<"else"> +
                   assign<&ast::IfStmt::elseBranch>(ref<Statement>)))> {};

  struct ForStatementRule
      : rule<create<ast::ForStmt>(
            kw<"for"> + lit<"("> +
            (assign<&ast::ForStmt::init>(SimpleStatement) | lit<";">)+(
                (assign<&ast::ForStmt::cond>(ref<Expression>) + lit<";">) |
                lit<";">)+option(assign<&ast::ForStmt::post>(ref<Expression>)) +
            lit<")"> + assign<&ast::ForStmt::body>(ref<Statement>))> {};

  struct WhileStatementRule
      : rule<create<ast::WhileStmt>(
            kw<"while"> + lit<"("> +
            assign<&ast::WhileStmt::cond>(ref<Expression>) + lit<")"> +
            assign<&ast::WhileStmt::body>(ref<Statement>))> {};

  struct DoWhileStatementRule
      : rule<create<ast::DoWhileStmt>(
            kw<"do"> + assign<&ast::DoWhileStmt::body>(ref<Statement>) +
            kw<"while"> + lit<"("> +
            assign<&ast::DoWhileStmt::cond>(ref<Expression>) + lit<")"> +
            lit<";">)> {};

  struct ContinueStatementRule
      : rule<create<ast::ContinueStmt> + kw<"continue"> + lit<";">> {};

  struct BreakStatementRule
      : rule<create<ast::BreakStmt> + kw<"break"> + lit<";">> {};

  struct CatchClauseRule
      : rule<create<ast::CatchClause>(
            kw<"catch"> +
            option(flag<&ast::CatchClause::hasParameters>(
                option(assign<&ast::CatchClause::name>(ref<Identifier>)) +
                lit<"("> +
                option(some(
                    append<&ast::CatchClause::parameters>(ref<ParameterDecl>),
                    lit<",">)) +
                lit<")">)) +
            assign<&ast::CatchClause::body>(ref<Block>))> {};

  struct TryStatementRule
      : rule<create<ast::TryStmt>(
            kw<"try"> + assign<&ast::TryStmt::value>(ref<Expression>) +
            option(kw<"returns"> + lit<"("> +
                   some(append<&ast::TryStmt::returnParameters>(
                            ref<ParameterDecl>),
                        lit<",">) +
                   lit<")">) +
            assign<&ast::TryStmt::body>(ref<Block>) +
            some(append<&ast::TryStmt::catchClauses>(ref<CatchClauseRule>)))> {
  };

  struct ReturnStatementRule
      : rule<create<ast::ReturnStmt>(
            kw<"return"> +
            option(assign<&ast::ReturnStmt::value>(ref<Expression>)) +
            lit<";">)> {};

  struct EmitStatementRule
      : rule<create<ast::EmitStmt>(
            kw<"emit"> + assign<&ast::EmitStmt::event>(ref<Expression>) +
            assign<&ast::EmitStmt::args>(ref<CallArgumentsRule>) + lit<";">)> {
  };

  struct RevertStatementRule
      : rule<create<ast::RevertStmt>(
            kw<"revert"> + assign<&ast::RevertStmt::error>(ref<Expression>) +
            assign<&ast::RevertStmt::args>(ref<CallArgumentsRule>) +
            lit<";">)> {};

  static constexpr auto AssemblyFlagsExpr =
      lit<"("> +
      some(append<&ast::AssemblyStmt::flags>(ref<STRING_LIT>), lit<",">) +
      lit<")">;

  struct AssemblyStatementRule
      : rule<create<ast::AssemblyStmt>(
            kw<"assembly"> +
            option(assign<&ast::AssemblyStmt::dialect>(ref<STRING_LIT>)) +
            option(AssemblyFlagsExpr) + lit<"{"> +
            many(append<&ast::AssemblyStmt::body>(ref<YulStatement>)) +
            lit<"}">)> {};

  struct Statement : rule<ref<Block>                               //
                          | ref<VariableDeclarationStatementRule>  //
                          | ref<ExpressionStatementRule>           //
                          | ref<IfStatementRule>                   //
                          | ref<ForStatementRule>                  //
                          | ref<WhileStatementRule>                //
                          | ref<DoWhileStatementRule>              //
                          | ref<ContinueStatementRule>             //
                          | ref<BreakStatementRule>                //
                          | ref<TryStatementRule>                  //
                          | ref<ReturnStatementRule>               //
                          | ref<EmitStatementRule>                 //
                          | ref<RevertStatementRule>               //
                          | ref<AssemblyStatementRule>> {};

  struct ContractBodyElement
      : rule<ref<ConstructorDefinitionRule>             //
             | ref<FunctionDefinitionRule>              //
             | ref<ModifierDefinitionRule>              //
             | ref<FallbackFunctionDefinitionRule>      //
             | ref<ReceiveFunctionDefinitionRule>       //
             | ref<StructDefinitionRule>                //
             | ref<EnumDefinitionRule>                  //
             | ref<UserDefinedValueTypeDefinitionRule>  //
             | ref<StateVariableDeclarationRule>        //
             | ref<EventDefinitionRule>                 //
             | ref<ErrorDefinitionRule>                 //
             | ref<UsingDirectiveRule>> {};

  // Yul (inline assembly).

  struct YUL_DECIMAL : terminal<lit<"0"> | (cr<"1-9"> + many(cr<"0-9">))> {};
  struct YUL_HEX_NUMBER : terminal<lit<"0x"> + some(HexChar)> {};

  static constexpr auto YulIdentifierWord =
      kw<"break"> | kw<"case"> | kw<"continue"> | kw<"default"> | kw<"false"> |
      kw<"for"> | kw<"function"> | kw<"if"> | kw<"leave"> | kw<"let"> |
      kw<"switch"> | kw<"true"> | kw<"hex"> | ref<ID>;

  static constexpr auto YulBuiltinWord =
      kw<"stop"> | kw<"add"> | kw<"sub"> | kw<"mul"> | kw<"div"> | kw<"sdiv"> |
      kw<"mod"> | kw<"smod"> | kw<"exp"> | kw<"not"> | kw<"lt"> | kw<"gt"> |
      kw<"slt"> | kw<"sgt"> | kw<"eq"> | kw<"iszero"> | kw<"and"> | kw<"or"> |
      kw<"xor"> | kw<"byte"> | kw<"shl"> | kw<"shr"> | kw<"sar"> |
      kw<"addmod"> | kw<"mulmod"> | kw<"signextend"> | kw<"keccak256"> |
      kw<"pop"> | kw<"mload"> | kw<"mstore"> | kw<"mstore8"> | kw<"sload"> |
      kw<"sstore"> | kw<"tload"> | kw<"tstore"> | kw<"msize"> | kw<"gas"> |
      kw<"address"> | kw<"balance"> | kw<"selfbalance"> | kw<"caller"> |
      kw<"callvalue"> | kw<"calldataload"> | kw<"calldatasize"> |
      kw<"calldatacopy"> | kw<"extcodesize"> | kw<"extcodecopy"> |
      kw<"returndatasize"> | kw<"returndatacopy"> | kw<"mcopy"> |
      kw<"extcodehash"> | kw<"create"> | kw<"create2"> | kw<"call"> |
      kw<"callcode"> | kw<"delegatecall"> | kw<"staticcall"> | kw<"return"> |
      kw<"revert"> | kw<"selfdestruct"> | kw<"invalid"> | kw<"log0"> |
      kw<"log1"> | kw<"log2"> | kw<"log3"> | kw<"log4"> | kw<"chainid"> |
      kw<"origin"> | kw<"gasprice"> | kw<"blockhash"> | kw<"blobhash"> |
      kw<"coinbase"> | kw<"timestamp"> | kw<"number"> | kw<"difficulty"> |
      kw<"prevrandao"> | kw<"gaslimit"> | kw<"basefee"> | kw<"blobbasefee">;

  struct YulPathRule
      : rule<(YulIdentifierWord) +
             many(lit<"."> + (YulIdentifierWord | YulBuiltinWord))> {};

  struct YulLiteralRule
      : rule<create<ast::YulLiteral>(assign<&ast::YulLiteral::text>(
            ref<YUL_DECIMAL> | ref<STRING_LIT> | ref<YUL_HEX_NUMBER> |
            kw<"true"> | kw<"false"> | ref<HEX_STRING>))> {};

  struct YulFunctionCallRule
      : rule<create<ast::YulFunctionCall>(
            assign<&ast::YulFunctionCall::name>(YulIdentifierWord |
                                                YulBuiltinWord) +
            lit<"("> +
            option(some(append<&ast::YulFunctionCall::args>(ref<YulExpression>),
                        lit<",">)) +
            lit<")">)> {};

  struct YulExpression
      : rule<create<ast::YulPathExpr>(
                 assign<&ast::YulPathExpr::path>(ref<YulPathRule>)) |
             ref<YulFunctionCallRule> | ref<YulLiteralRule>> {};

  struct YulBlock
      : rule<create<ast::YulBlock>(
            lit<"{"> +
            many(append<&ast::YulBlock::statements>(ref<YulStatement>)) +
            lit<"}">)> {};

  struct YulVariableDeclarationRule
      : rule<create<ast::YulVariableDeclaration>(
            kw<"let"> +
            some(append<&ast::YulVariableDeclaration::variables>(ref<ID>),
                 lit<",">) +
            option(lit<":="> + assign<&ast::YulVariableDeclaration::value>(
                                   ref<YulExpression>)))> {};

  struct YulAssignmentRule
      : rule<create<ast::YulAssignment>(
            some(append<&ast::YulAssignment::targets>(ref<YulPathRule>),
                 lit<",">) +
            option(lit<":="> +
                   assign<&ast::YulAssignment::value>(ref<YulExpression>)))> {};

  struct YulIfStatementRule
      : rule<create<ast::YulIfStmt>(
            kw<"if"> + assign<&ast::YulIfStmt::cond>(ref<YulExpression>) +
            assign<&ast::YulIfStmt::body>(ref<YulBlock>))> {};

  struct YulForStatementRule
      : rule<create<ast::YulForStmt>(
            kw<"for"> + assign<&ast::YulForStmt::init>(ref<YulBlock>) +
            assign<&ast::YulForStmt::cond>(ref<YulExpression>) +
            assign<&ast::YulForStmt::post>(ref<YulBlock>) +
            assign<&ast::YulForStmt::body>(ref<YulBlock>))> {};

  struct YulSwitchCaseRule
      : rule<create<ast::YulSwitchCase>(
            kw<"case"> +
            assign<&ast::YulSwitchCase::value>(ref<YulLiteralRule>) +
            assign<&ast::YulSwitchCase::body>(ref<YulBlock>))> {};

  struct YulSwitchStatementRule
      : rule<create<ast::YulSwitchStmt>(
            kw<"switch"> +
            assign<&ast::YulSwitchStmt::value>(ref<YulExpression>) +
            ((some(append<&ast::YulSwitchStmt::cases>(ref<YulSwitchCaseRule>)) +
              option(kw<"default"> +
                     assign<&ast::YulSwitchStmt::defaultBody>(ref<YulBlock>))) |
             (kw<"default"> +
              assign<&ast::YulSwitchStmt::defaultBody>(ref<YulBlock>))))> {};

  struct YulFunctionDefinitionRule
      : rule<create<ast::YulFunctionDefinition>(
            kw<"function"> +
            assign<&ast::YulFunctionDefinition::name>(ref<ID>) + lit<"("> +
            option(
                append<&ast::YulFunctionDefinition::parameters>(ref<ID>) +
                many(lit<","> + append<&ast::YulFunctionDefinition::parameters>(
                                    ref<ID>))) +
            lit<")"> +
            option(
                lit<"->"> +
                append<&ast::YulFunctionDefinition::returnParameters>(ref<ID>) +
                many(lit<","> +
                     append<&ast::YulFunctionDefinition::returnParameters>(
                         ref<ID>))) +
            assign<&ast::YulFunctionDefinition::body>(ref<YulBlock>))> {};

  struct YulStatement
      : rule<ref<YulBlock>                      //
             | ref<YulVariableDeclarationRule>  //
             | ref<YulAssignmentRule>           //
             | ref<YulIfStatementRule>          //
             | ref<YulForStatementRule>         //
             | ref<YulSwitchStatementRule>      //
             | ref<YulFunctionDefinitionRule>   //
             |
             create<ast::YulKeywordStmt>(assign<&ast::YulKeywordStmt::keyword>(
                 kw<"leave"> | kw<"break"> | kw<"continue">))  //
             | ref<YulFunctionCallRule>> {};

  struct SourceUnitRule
      : rule<create<ast::SourceUnit>(
            many(append<&ast::SourceUnit::items>(ref<PragmaDirectiveRule>) |
                 append<&ast::SourceUnit::items>(ref<ImportDirectiveRule>) |
                 append<&ast::SourceUnit::items>(ref<UsingDirectiveRule>) |
                 append<&ast::SourceUnit::items>(ref<ContractDefinitionRule>) |
                 append<&ast::SourceUnit::items>(ref<InterfaceDefinitionRule>) |
                 append<&ast::SourceUnit::items>(ref<LibraryDefinitionRule>) |
                 append<&ast::SourceUnit::items>(ref<FunctionDefinitionRule>) |
                 append<&ast::SourceUnit::items>(
                     ref<ConstantVariableDeclarationRule>) |
                 append<&ast::SourceUnit::items>(ref<StructDefinitionRule>) |
                 append<&ast::SourceUnit::items>(ref<EnumDefinitionRule>) |
                 append<&ast::SourceUnit::items>(
                     ref<UserDefinedValueTypeDefinitionRule>) |
                 append<&ast::SourceUnit::items>(ref<ErrorDefinitionRule>) |
                 append<&ast::SourceUnit::items>(ref<EventDefinitionRule>)))> {
  };

  using entry = SourceUnitRule;
  using ignore = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace solidity
