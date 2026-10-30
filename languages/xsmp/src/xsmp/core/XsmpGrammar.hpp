#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "xsmp/core/ast.hpp"
#include "xsmp/core/LiteralConversions.hpp"

namespace xsmp {

struct Xsmp : cursiv::GrammarSpec {
  // terminal ID: /[_a-zA-Z]\w*/;
  static constexpr auto ReservedName = kw<"false"> | kw<"float"> | kw<"nullptr"> | kw<"private"> |
      kw<"protected"> | kw<"public"> | kw<"true"> | kw<"using"> | kw<"void">;
  struct ID : terminal<!ReservedName + cr<"_a-zA-Z"> + many(w)> {};

  static constexpr auto HEXDIGIT = cr<"0-9a-fA-F">;

  static constexpr auto ESCAPE =
      lit<"\\"> + (cr<R"(abfnrtv\\"'?)">                 //
                   | (lit<"x"> + repeat<2>(HEXDIGIT))  //
                   | (lit<"u"> + repeat<4>(HEXDIGIT))  //
                   | (lit<"U"> + repeat<8>(HEXDIGIT)));

  // terminal STRING:
  // /"(?:\\(?:...)|[^"\n])*"/;
  struct STRING
      : terminal<(lit<"\""> + many(ESCAPE | cr<R"(^"\n)">) + lit<"\"">)> {};

  // terminal CHAR:
  // /'(?:\\(?:...)|[^'\n])*'/;
  struct CHAR : terminal<(lit<"'"> + many(ESCAPE | cr<R"(^'\n)">) + lit<"'">)> {};

  // terminal fragment DIGITSEQUENCE: /[0-9][0-9']*/;
  static constexpr auto DIGITSEQUENCE = d + many(cr<"0-9'">);

  // terminal fragment EXPONENTPART: /[eE][+-]?[0-9][0-9']*/;
  static constexpr auto EXPONENTPART =
      cr<"eE"> + option(cr<"+-">) + d + many(cr<"0-9'">);

  // terminal fragment FLOATINGSUFFIX: /[fF]/;   (no l/L suffix)
  static constexpr auto FLOATINGSUFFIX = cr<"fF">;

  // terminal fragment FRACTIONALCONSTANT:
  // DIGITSEQUENCE? '.' DIGITSEQUENCE | DIGITSEQUENCE '.';
  static constexpr auto FRACTIONALCONSTANT =
      (option(DIGITSEQUENCE) + lit<"."> + DIGITSEQUENCE)  //
      | (DIGITSEQUENCE + lit<".">);

  // terminal FLOATING_LITERAL:
  // FRACTIONALCONSTANT EXPONENTPART? FLOATINGSUFFIX?
  // | DIGITSEQUENCE EXPONENTPART FLOATINGSUFFIX?;
  struct FLOATING_LITERAL
      : terminal<((FRACTIONALCONSTANT + option(EXPONENTPART) +
                   option(FLOATINGSUFFIX))  //
                  | (DIGITSEQUENCE + EXPONENTPART + option(FLOATINGSUFFIX)))> {
  };

  static constexpr auto INTEGERSUFFIX = (cr<"uU"> + option(cr<"lL">))  //
                                        | (cr<"lL"> + option(cr<"uU">));

  // terminal INTEGER_LITERAL:
  // /(?:0[xX][0-9a-fA-F][0-9a-fA-F']*|0[bB][01]['01]*)(?:[uU][lL]?|[lL][uU]?)?
  // |(?:[1-9][0-9']*|0[0-7']*)(?:ns|us|ms|s|mn|h|d|[uU][lL]?|[lL][uU]?)?/;
  struct INTEGER_LITERAL
      : terminal<((((lit<"0"> + cr<"xX"> + cr<"0-9a-fA-F"> +
                     many(cr<"0-9a-fA-F'">))                                //
                    | (lit<"0"> + cr<"bB"> + cr<"01"> + many(cr<"'01">)))   //
                   + option(INTEGERSUFFIX))                                 //
                  | (((cr<"1-9"> + many(cr<"0-9'">))                        //
                      | (lit<"0"> + many(cr<"0-7'">)))                      //
                     + option(lit<"ns"> | lit<"us"> | lit<"ms"> | lit<"s">  //
                              | lit<"mn"> | lit<"h"> | lit<"d">             //
                              | INTEGERSUFFIX)))> {};

  // QualifiedName returns string:
  // ValidID ('.' ValidID)*;
  struct QualifiedName : rule<some(ref<ID>, kw<".">)> {};

  // hidden terminal WS:         /\s+/;
  // hidden terminal ML_COMMENT: /\/\*[\s\S]*?\*\//;
  // hidden terminal SL_COMMENT: /\/\/[^\n\r]*/;
  struct WS : terminal<some(cr<R"(\t-\r \u{00A0}\u{1680}\u{2000}-\u{200A}\u{2028}\u{2029}\u{202F}\u{205F}\u{3000}\u{FEFF})">)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  struct Expression;
  struct UnaryOperation;
  struct NamedElementReference;
  struct Literal;
  struct ParenthesizedExpression;
  struct DesignatedInitializer;
  struct CollectionExpression;
  struct CollectionLiteral;
  struct IntegerLiteral;
  struct FloatingLiteral;
  struct StringLiteral;
  struct CharacterLiteral;
  struct BuiltInConstant;
  struct BuiltInFunction;
  struct BooleanLiteral;
  struct KeywordExpression;

  // OpOr returns OpBinary:
  // '||';
  static constexpr auto OpOr = as<ast::OpBinary::logical_or>(lit<"||">);

  // OpAnd returns OpBinary:
  // '&&';
  static constexpr auto OpAnd = as<ast::OpBinary::logical_and>(lit<"&&">);

  // OpBitwiseOr returns OpBinary:
  // '|';
  static constexpr auto OpBitwiseOr = as<ast::OpBinary::bitwise_or>(lit<"|">);

  // OpBitwiseXor returns OpBinary:
  // '^';
  static constexpr auto OpBitwiseXor = as<ast::OpBinary::bitwise_xor>(lit<"^">);

  // OpBitwiseAnd returns OpBinary:
  // '&';
  static constexpr auto OpBitwiseAnd = as<ast::OpBinary::bitwise_and>(lit<"&">);

  // OpEquality returns OpBinary:
  // '==' | '!=';
  static constexpr auto OpEquality = as<ast::OpBinary::equal>(lit<"==">) | as<ast::OpBinary::not_equal>(lit<"!=">);

  // OpCompare returns OpBinary:
  // '>=' | '<=' | '>' | '<';
  static constexpr auto OpCompare =
      as<ast::OpBinary::greater_equal>(lit<">=">) | as<ast::OpBinary::less_equal>(lit<"<=">) | as<ast::OpBinary::greater>(lit<">">) | as<ast::OpBinary::less>(lit<"<">);

  // OpBitwise returns OpBinary:
  // '<<' | '>>';
  static constexpr auto OpBitwise = as<ast::OpBinary::shift_left>(lit<"<<">) | as<ast::OpBinary::shift_right>(lit<">>">);

  // OpAdd returns OpBinary:
  // '+' | '-';
  static constexpr auto OpAdd = as<ast::OpBinary::add>(lit<"+">) | as<ast::OpBinary::subtract>(lit<"-">);

  // OpMulti returns OpBinary:
  // '*' | '/' | '%';
  static constexpr auto OpMulti = as<ast::OpBinary::multiply>(lit<"*">) | as<ast::OpBinary::divide>(lit<"/">) | as<ast::OpBinary::modulo>(lit<"%">);

  // Expression returns Expression:
  // OrExpression;
  struct Expression
      : infix<
            UnaryOperation, ast::BinaryOperation,
            levels(left(OpMulti), left(OpAdd), left(OpBitwise), left(OpCompare),
                   left(OpEquality), left(OpBitwiseAnd), left(OpBitwiseXor),
                   left(OpBitwiseOr), left(OpAnd), left(OpOr)),
            &ast::BinaryOperation::leftOperand, &ast::BinaryOperation::feature,
            &ast::BinaryOperation::rightOperand> {};

  // OpUnary returns OpUnary:
  // "!" | "-" | "+" | "~";
  static constexpr auto OpUnary = lit<"!"> | lit<"-"> | lit<"+"> | lit<"~">;

  // UnaryOperation returns Expression:
  // {UnaryOperation} feature=OpUnary operand=UnaryOperation | Literal;
  struct UnaryOperation
      : rule<(create<ast::UnaryOperation>(
                  assign<&ast::UnaryOperation::feature>(OpUnary) +
                  assign<&ast::UnaryOperation::operand>(ref<UnaryOperation>)) |
              ref<Literal>)> {};

  // NamedElementReference returns Expression:
  // {NamedElementReference}
  // value=[ConstantOrEnumerationLiteral:QualifiedName];
  struct NamedElementReference
      : rule<assign<&ast::NamedElementReference::value>(ref<QualifiedName>)> {};

  // Literal returns Expression:
  // CollectionLiteral | FloatingLiteral | IntegerLiteral | StringLiteral |
  // CharacterLiteral | BooleanLiteral | ParenthesizedExpression |
  // NamedElementReference | BuiltInConstant | BuiltInFunction |
  // KeywordExpression;
  struct Literal
      : rule<(ref<CollectionLiteral>                                 //
              | ref<FloatingLiteral>                                 //
              | ref<IntegerLiteral>                                  //
              | ref<StringLiteral>                                   //
              | ref<CharacterLiteral>                                //
              | ref<BooleanLiteral>                                  //
              | ref<ParenthesizedExpression> | ref<BuiltInFunction>  //
              | ref<BuiltInConstant>                                 //
              | ref<KeywordExpression>                               //
              | ref<NamedElementReference>)> {};

  // ParenthesizedExpression returns Expression:
  // {ParenthesizedExpression} '(' expr=Expression ')';
  struct ParenthesizedExpression
      : rule<lit<"("> +
             assign<&ast::ParenthesizedExpression::expr>(ref<Expression>) +
             lit<")">> {};

  // DesignatedInitializer returns Expression:
  // {DesignatedInitializer} '.' field=[Field:ValidID] ('='
  // expr=Expression)?;
  struct DesignatedInitializer
      : rule<lit<"."> + assign<&ast::DesignatedInitializer::field>(ref<ID>) +
             option(lit<"="> + assign<&ast::DesignatedInitializer::expr>(
                                   ref<Expression>))> {};

  // CollectionExpression returns Expression:
  // Expression | DesignatedInitializer;
  struct CollectionExpression
      : rule<(ref<Expression> | ref<DesignatedInitializer>)> {};

  // CollectionLiteral returns Expression:
  // {CollectionLiteral} '{' (elements+=CollectionExpression (','
  // elements+=CollectionExpression)* )? '}';
  struct CollectionLiteral
      : rule<lit<"{"> +
             many(append<&ast::CollectionLiteral::elements>(
                      ref<CollectionExpression>),
                  lit<",">) +
             lit<"}">> {};

  // IntegerLiteral returns IntegerLiteral:
  // {IntegerLiteral} text=INTEGER_LITERAL;
  struct IntegerLiteral
      : rule<assign<&ast::IntegerLiteral::text>(as<integer_value>(ref<INTEGER_LITERAL>))> {};

  // FloatingLiteral returns FloatingLiteral:
  // {FloatingLiteral} text=FLOATING_LITERAL;
  struct FloatingLiteral
      : rule<assign<&ast::FloatingLiteral::text>(ref<FLOATING_LITERAL>)> {};

  // StringLiteral returns Expression:
  // {StringLiteral} value+=STRING+;
  struct StringLiteral
      : rule<some(append<&ast::StringLiteral::value>(as<quoted_value>(ref<STRING>)))> {};

  // CharacterLiteral returns Expression:
  // {CharacterLiteral} value=CHAR;
  struct CharacterLiteral
      : rule<assign<&ast::CharacterLiteral::value>(as<quoted_value>(ref<CHAR>))> {};

  // BuiltInConstant returns Expression:
  // {BuiltInConstant} '$' name=ID;
  struct BuiltInConstant
      : rule<lit<"$"> + assign<&ast::BuiltInConstant::name>(as<builtin_name>(ref<ID>))> {};

  // BuiltInFunction returns Expression:
  // {BuiltInFunction} '$' name=ID '(' argument=Expression? ')';
  struct BuiltInFunction
      : rule<lit<"$"> + assign<&ast::BuiltInFunction::name>(as<builtin_name>(ref<ID>)) +
             lit<"("> +
             option(assign<&ast::BuiltInFunction::argument>(ref<Expression>)) +
             lit<")">> {};

  // BooleanLiteral returns Expression:
  // {BooleanLiteral} ('false' | isTrue?='true');
  struct BooleanLiteral
      : rule<create<ast::BooleanLiteral> +
             (kw<"false"> |
              flag<&ast::BooleanLiteral::isTrue>(kw<"true">))> {};

  // KeywordExpression returns Expression:
  // {KeywordExpression} name='nullptr';
  struct KeywordExpression
      : rule<assign<&ast::KeywordExpression::name>(kw<"nullptr">)> {};

  struct Attribute;
  struct Constant;
  struct Field;
  struct Property;
  struct Operation;
  struct ReturnParameter;
  struct Parameter;
  struct Association;
  struct Container;
  struct Reference;
  struct EntryPoint;
  struct EventSink;
  struct EventSource;

  // VisibilityModifiers returns string:
  // 'private' | 'protected' | 'public';
  static constexpr auto VisibilityModifiers =
      kw<"private"> | kw<"protected"> | kw<"public">;

  // FieldFlags returns string:
  // 'input' | 'output' | 'transient';
  static constexpr auto FieldFlags =
      kw<"input"> | kw<"output"> | kw<"transient">;

  // FieldModifiers returns string:
  // VisibilityModifiers | FieldFlags;
  static constexpr auto FieldModifiers = VisibilityModifiers | FieldFlags;

  // ClassModifiers returns string:
  // VisibilityModifiers | 'abstract';
  static constexpr auto ClassModifiers = VisibilityModifiers | kw<"abstract">;

  // AccessKind returns string:
  // 'readWrite' | 'readOnly' | 'writeOnly';
  static constexpr auto AccessKind =
      kw<"readWrite"> | kw<"readOnly"> | kw<"writeOnly">;

  // PropertyModifiers returns string:
  // AccessKind | VisibilityModifiers;
  static constexpr auto PropertyModifiers = AccessKind | VisibilityModifiers;

  // ParameterDirectionKind returns ParameterDirectionKind:
  // 'in' | 'out' | 'inout';
  static constexpr auto ParameterDirectionKind =
      kw<"in"> | kw<"out"> | kw<"inout">;

  // RangeKind returns RangeKind:
  // '...' | '..<' | '<..' | '<.<';
  static constexpr auto RangeKind =
      kw<"..."> | kw<"..<"> | kw<"<.."> | kw<"<.<">;

  // ShortMultiplicity returns Multiplicity:
  // {Multiplicity} aux?='+' | {Multiplicity} '*';
  struct ShortMultiplicity
      : rule<create<ast::Multiplicity> +
             (flag<&ast::Multiplicity::aux>(kw<"+">) | kw<"*">)> {};

  // Multiplicity returns Multiplicity:
  // {Multiplicity} '[' ((lower=Expression ('...' (aux?='*' |
  // upper=Expression))? )
  // | aux?='+' | '*' )? ']';
  struct Multiplicity
      : rule<(kw<"["> +
              option((assign<&ast::Multiplicity::lower>(ref<Expression>) +
                      option(kw<"..."> +
                             (flag<&ast::Multiplicity::aux>(kw<"*">) |
                              assign<&ast::Multiplicity::upper>(
                                  ref<Expression>)))) |
                     flag<&ast::Multiplicity::aux>(kw<"+">) | kw<"*">) +
              kw<"]">)> {};

  // fragment MultiplicityDeclaration:
  // (optional?='?' | multiplicity=Multiplicity |
  // multiplicity=ShortMultiplicity)?;
  static constexpr auto MultiplicityDeclaration =
      option(flag<&ast::NamedElementWithMultiplicity::optional_>(kw<"?">) |
             assign<&ast::NamedElementWithMultiplicity::multiplicity>(
                 ref<Multiplicity> | ref<ShortMultiplicity>));

  // Shared declaration prefixes write the current node; no extra AST rule.
  static constexpr auto Attributes = many(append<&ast::NamedElement::attributes>(ref<Attribute>));
  static constexpr auto VisibilityPrefix = Attributes + many(append<&ast::VisibilityElement::modifiers>(VisibilityModifiers));
  static constexpr auto ClassPrefix = Attributes + many(append<&ast::VisibilityElement::modifiers>(ClassModifiers));
  static constexpr auto PropertyPrefix = Attributes + many(append<&ast::VisibilityElement::modifiers>(PropertyModifiers));
  static constexpr auto FieldPrefix = Attributes + many(append<&ast::VisibilityElement::modifiers>(FieldModifiers));

  // Structure returns Structure:
  // attributes+=Attribute*
  // modifiers+=VisibilityModifiers*
  // 'struct' name=ValidID
  // '{' elements+=(Constant|Field)* '}';
  struct Structure
      : rule<create<ast::Structure> +
             (VisibilityPrefix +
              kw<"struct"> + assign<&ast::Structure::name>(ref<ID>) + kw<"{"> +
              many(append<&ast::Structure::elements>(ref<Constant> |
                                                     ref<Field>)) +
              kw<"}">)> {};

  // Class returns Class:
  // attributes+=Attribute*
  // modifiers+=ClassModifiers*
  // 'class' name=ValidID ('extends' base=[Class:QualifiedName])?
  // '{' elements+=(Constant|Field|Property|Operation|Association)* '}';
  struct Class : rule<create<ast::Class> +
                      (ClassPrefix +
                       kw<"class"> + assign<&ast::Class::name>(ref<ID>) +
                       option(kw<"extends"> +
                              assign<xref<&ast::Class::base, ast::Class>>(ref<QualifiedName>)) +
                       kw<"{"> +
                       many(append<&ast::Class::elements>(
                           ref<Constant> | ref<Field> | ref<Property> |
                           ref<Operation> | ref<Association>)) +
                       kw<"}">)> {};

  // Exception returns Exception:
  // attributes+=Attribute*
  // modifiers+=ClassModifiers*
  // 'exception' name=ValidID ('extends' base=[Class:QualifiedName])?
  // '{' elements+=(Constant|Field|Property|Operation|Association)* '}';
  struct Exception
      : rule<create<ast::Exception> +
             (ClassPrefix +
              kw<"exception"> + assign<&ast::Exception::name>(ref<ID>) +
              option(kw<"extends"> +
                     assign<xref<&ast::Exception::base, ast::Class>>(ref<QualifiedName>)) +
              kw<"{"> +
              many(append<&ast::Exception::elements>(
                  ref<Constant> | ref<Field> | ref<Property> | ref<Operation> |
                  ref<Association>)) +
              kw<"}">)> {};

  // Interface returns Interface:
  // attributes+=Attribute*
  // modifiers+=VisibilityModifiers*
  // 'interface' name=ValidID
  // ('extends' base+=[Interface:QualifiedName] (','
  // base+=[Interface:QualifiedName])*)?
  // '{' elements+=(Constant|Property|Operation)* '}';
  struct Interface
      : rule<(VisibilityPrefix +
              kw<"interface"> + assign<&ast::Interface::name>(ref<ID>) +
              option(kw<"extends"> +
                     some(append<xref<&ast::Interface::base, ast::Interface>>(ref<QualifiedName>),
                          kw<",">)) +
              kw<"{"> +
              many(append<&ast::Interface::elements>(
                  ref<Constant> | ref<Property> | ref<Operation>)) +
              kw<"}">)> {};

  // Model returns Model:
  // attributes+=Attribute*
  // modifiers+=ClassModifiers*
  // 'model' name=ValidID ('extends' base=[Component:QualifiedName])?
  // ('implements' interface+=[Interface:QualifiedName] (','
  // interface+=[Interface:QualifiedName])*)?
  // '{' elements+=(Constant|Field|Property|Operation|Association|Container|
  // Reference|EntryPoint|EventSink|EventSource)* '}';
  struct Model
      : rule<create<ast::Model> +
             (ClassPrefix +
              kw<"model"> + assign<&ast::Model::name>(ref<ID>) +
              option(kw<"extends"> +
                     assign<xref<&ast::Model::base, ast::Component>>(ref<QualifiedName>)) +
              option(kw<"implements"> +
                     some(append<xref<&ast::Model::interface_, ast::Interface>>(ref<QualifiedName>),
                          kw<",">)) +
              kw<"{"> +
              many(append<&ast::Model::elements>(
                  ref<Constant> | ref<Field> | ref<Property> | ref<Operation> |
                  ref<Association> | ref<Container> | ref<Reference> |
                  ref<EntryPoint> | ref<EventSink> | ref<EventSource>)) +
              kw<"}">)> {};

  // Service returns Service:
  // ... identical to Model except the 'service' keyword ...
  struct Service
      : rule<create<ast::Service> +
             (ClassPrefix +
              kw<"service"> + assign<&ast::Service::name>(ref<ID>) +
              option(kw<"extends"> +
                     assign<xref<&ast::Service::base, ast::Component>>(ref<QualifiedName>)) +
              option(kw<"implements"> +
                     some(append<xref<&ast::Service::interface_, ast::Interface>>(ref<QualifiedName>),
                          kw<",">)) +
              kw<"{"> +
              many(append<&ast::Service::elements>(
                  ref<Constant> | ref<Field> | ref<Property> | ref<Operation> |
                  ref<Association> | ref<Container> | ref<Reference> |
                  ref<EntryPoint> | ref<EventSink> | ref<EventSource>)) +
              kw<"}">)> {};

  // ArrayType returns ArrayType:
  // attributes+=Attribute*
  // modifiers+=VisibilityModifiers*
  // 'array' name=ValidID '=' itemType=[ValueType:QualifiedName] '['
  // size=Expression? ']';
  struct ArrayType
      : rule<(VisibilityPrefix +
              kw<"array"> + assign<&ast::ArrayType::name>(ref<ID>) + kw<"="> +
              assign<xref<&ast::ArrayType::itemType, ast::ValueType>>(ref<QualifiedName>) + kw<"["> +
              option(assign<&ast::ArrayType::size>(ref<Expression>)) +
              kw<"]">)> {};

  // ValueReference returns ValueReference:
  // attributes+=Attribute*
  // modifiers+=VisibilityModifiers*
  // 'using' name=ValidID '=' type=[ValueType:QualifiedName] '*';
  struct ValueReference
      : rule<(
            VisibilityPrefix +
            kw<"using"> + assign<&ast::ValueReference::name>(ref<ID>) +
            kw<"="> + assign<xref<&ast::ValueReference::type, ast::ValueType>>(ref<QualifiedName>) +
            kw<"*">)> {};

  // Integer returns Integer:
  // attributes+=Attribute*
  // modifiers+=VisibilityModifiers*
  // 'integer' name=ValidID ('extends'
  // primitiveType=[PrimitiveType:QualifiedName])?
  // ('in' (minimum=Expression | '*') '...' (maximum=Expression | '*'))?;
  struct Integer
      : rule<(VisibilityPrefix +
              kw<"integer"> + assign<&ast::Integer::name>(ref<ID>) +
              option(kw<"extends"> +
                     assign<xref<&ast::Integer::primitiveType, ast::PrimitiveType>>(ref<QualifiedName>)) +
              option(kw<"in"> +
                     (assign<&ast::Integer::minimum>(ref<Expression>) |
                      kw<"*">)+kw<"..."> +
                     (assign<&ast::Integer::maximum>(ref<Expression>) |
                      kw<"*">)))> {};

  // Float returns Float:
  // attributes+=Attribute*
  // modifiers+=VisibilityModifiers*
  // 'float' name=ValidID ('extends'
  // primitiveType=[PrimitiveType:QualifiedName])?
  // ('in' (minimum=Expression | '*') range=RangeKind (maximum=Expression |
  // '*'))?;
  struct Float
      : rule<(VisibilityPrefix +
              kw<"float"> + assign<&ast::Float::name>(ref<ID>) +
              option(kw<"extends"> +
                     assign<xref<&ast::Float::primitiveType, ast::PrimitiveType>>(ref<QualifiedName>)) +
              option(kw<"in"> +
                     (assign<&ast::Float::minimum>(ref<Expression>) |
                      kw<"*">)+assign<&ast::Float::range>(RangeKind) +
                     (assign<&ast::Float::maximum>(ref<Expression>) |
                      kw<"*">)))> {};

  // EventType returns EventType:
  // attributes+=Attribute*
  // modifiers+=VisibilityModifiers*
  // 'event' name=ValidID ('extends' eventArgs=[SimpleType:QualifiedName])?;
  struct EventType
      : rule<(VisibilityPrefix +
              kw<"event"> + assign<&ast::EventType::name>(ref<ID>) +
              option(kw<"extends"> +
                     assign<xref<&ast::EventType::eventArgs, ast::SimpleType>>(ref<QualifiedName>)))> {
  };

  // StringType returns StringType:
  // attributes+=Attribute*
  // modifiers+=VisibilityModifiers*
  // 'string' name=ValidID '[' length=Expression? ']';
  struct StringType
      : rule<(VisibilityPrefix +
              kw<"string"> + assign<&ast::StringType::name>(ref<ID>) + kw<"["> +
              option(assign<&ast::StringType::length>(ref<Expression>)) +
              kw<"]">)> {};

  // PrimitiveType returns PrimitiveType:
  // attributes+=Attribute* modifiers+=VisibilityModifiers* 'primitive'
  // name=ValidID;
  struct PrimitiveType
      : rule<create<ast::PrimitiveType> +
             (VisibilityPrefix +
              kw<"primitive"> + assign<&ast::PrimitiveType::name>(ref<ID>))> {};

  // NativeType returns NativeType:
  // attributes+=Attribute* modifiers+=VisibilityModifiers* 'native'
  // name=ValidID;
  struct NativeType
      : rule<create<ast::NativeType> +
             (VisibilityPrefix +
              kw<"native"> + assign<&ast::NativeType::name>(ref<ID>))> {};

  // AttributeType returns AttributeType:
  // attributes+=Attribute*
  // modifiers+=VisibilityModifiers*
  // 'attribute' type=[ValueType:QualifiedName] name=ValidID ('='
  // default=Expression)?;
  struct AttributeType
      : rule<(
            VisibilityPrefix +
            kw<"attribute"> +
            assign<xref<&ast::AttributeType::type, ast::ValueType>>(ref<QualifiedName>) +
            assign<&ast::AttributeType::name>(ref<ID>) +
            option(kw<"="> +
                   assign<&ast::AttributeType::default_>(ref<Expression>)))> {};

  struct EnumerationLiteral;

  // Enumeration returns Enumeration:
  // attributes+=Attribute*
  // modifiers+=VisibilityModifiers*
  // 'enum' name=ValidID
  // '{' (literal+=EnumerationLiteral (',' literal+=EnumerationLiteral)*
  // ','?)? '}';
  struct Enumeration
      : rule<(VisibilityPrefix +
              kw<"enum"> + assign<&ast::Enumeration::name>(ref<ID>) + kw<"{"> +
              option(some(append<&ast::Enumeration::literal>(
                              ref<EnumerationLiteral>),
                          kw<",">) +
                     option(kw<",">)) +
              kw<"}">)> {};

  // EnumerationLiteral returns EnumerationLiteral:
  // attributes+=Attribute* name=ValidID '=' value=Expression;
  struct EnumerationLiteral
      : rule<(
            Attributes +
            assign<&ast::EnumerationLiteral::name>(ref<ID>) + kw<"="> +
            assign<&ast::EnumerationLiteral::value>(ref<Expression>))> {};

  // Type returns Type:
  // Structure | Class | Exception | Interface | Model | Service |
  // ArrayType | ValueReference | Integer | Float | EventType | StringType |
  // PrimitiveType | NativeType | AttributeType | Enumeration;
  struct Type
      : rule<(ref<Structure> | ref<Class> | ref<Exception> | ref<Interface> |
              ref<Model> | ref<Service> | ref<ArrayType> | ref<ValueReference> |
              ref<Integer> | ref<Float> | ref<EventType> | ref<StringType> |
              ref<PrimitiveType> | ref<NativeType> | ref<AttributeType> |
              ref<Enumeration>)> {};

  // Constant returns Constant:
  // attributes+=Attribute*
  // modifiers+=VisibilityModifiers*
  // 'constant' type=[SimpleType:QualifiedName] name=ValidID ('='
  // value=Expression )?;
  struct Constant
      : rule<(
            VisibilityPrefix +
            kw<"constant"> + assign<xref<&ast::Constant::type, ast::SimpleType>>(ref<QualifiedName>) +
            assign<&ast::Constant::name>(ref<ID>) +
            option(kw<"="> + assign<&ast::Constant::value>(ref<Expression>)))> {
  };

  // Field returns Field:
  // attributes+=Attribute*
  // modifiers+=FieldModifiers*
  // 'field' type=[LanguageType:QualifiedName] name=ValidID ('='
  // default=Expression)?;
  struct Field
      : rule<(FieldPrefix +
              kw<"field"> + assign<xref<&ast::Field::type, ast::LanguageType>>(ref<QualifiedName>) +
              assign<&ast::Field::name>(ref<ID>) +
              option(kw<"="> +
                     assign<&ast::Field::default_>(ref<Expression>)))> {};

  // Property returns Property:
  // attributes+=Attribute*
  // modifiers+=PropertyModifiers*
  // 'property' type=[ValueType:QualifiedName] name=ValidID
  // ('get' 'throws' getRaises+=[Exception:QualifiedName] (','
  // getRaises+=[Exception:QualifiedName] )*)?
  // ('set' 'throws' setRaises+=[Exception:QualifiedName] (','
  // setRaises+=[Exception:QualifiedName] )*)?
  // ('->' attachedField=[Field:QualifiedName])?;
  struct Property
      : rule<(PropertyPrefix +
              kw<"property"> +
              assign<xref<&ast::Property::type, ast::ValueType>>(ref<QualifiedName>) +
              assign<&ast::Property::name>(ref<ID>) +
              option(kw<"get"> + kw<"throws"> +
                     some(append<xref<&ast::Property::getRaises, ast::Exception>>(ref<QualifiedName>),
                          kw<",">)) +
              option(kw<"set"> + kw<"throws"> +
                     some(append<xref<&ast::Property::setRaises, ast::Exception>>(ref<QualifiedName>),
                          kw<",">)) +
              option(kw<"->"> + assign<&ast::Property::attachedField>(
                                    ref<QualifiedName>)))> {};

  // Operation returns Operation:
  // attributes+=Attribute*
  // modifiers+=VisibilityModifiers*
  // 'def' (returnParameter=(ReturnParameter|ReturnParameterWithName) |
  // 'void') name=ValidID
  // '(' (parameter+=Parameter (',' parameter+=Parameter)*)? ')'
  // ('throws' raisedException+=[Exception:QualifiedName] (','
  // raisedException+=[Exception:QualifiedName])*)?;
  struct Operation
      : rule<(
            VisibilityPrefix +
            kw<"def"> +
            (kw<"void"> |
             assign<&ast::Operation::returnParameter>(ref<ReturnParameter>)) +
            assign<&ast::Operation::name>(ref<ID>) + kw<"("> +
            many(append<&ast::Operation::parameter>(ref<Parameter>), kw<",">) +
            kw<")"> +
            option(kw<"throws"> + some(append<xref<&ast::Operation::raisedException, ast::Exception>>(
                                           ref<QualifiedName>),
                                       kw<",">)))> {};

  // ReturnParameter returns ReturnParameter:
  // attributes+=Attribute*
  // type=[LanguageType:QualifiedName];
  struct ReturnParameter
      : rule<(many(append<&ast::ReturnParameter::attributes>(ref<Attribute>)) +
              assign<xref<&ast::ReturnParameter::type, ast::LanguageType>>(ref<QualifiedName>) +
              option(assign<&ast::ReturnParameter::name>(ref<ID>)))> {};

  // Parameter returns Parameter:
  // attributes+=Attribute*
  // direction=ParameterDirectionKind? type=[LanguageType:QualifiedName]
  // name=ValidID ('=' default=Expression)?;
  struct Parameter
      : rule<(
            Attributes +
            option(assign<&ast::Parameter::direction>(ParameterDirectionKind)) +
            assign<xref<&ast::Parameter::type, ast::LanguageType>>(ref<QualifiedName>) +
            assign<&ast::Parameter::name>(ref<ID>) +
            option(kw<"="> +
                   assign<&ast::Parameter::default_>(ref<Expression>)))> {};

  // Association returns Association:
  // attributes+=Attribute*
  // modifiers+=VisibilityModifiers*
  // 'association' type=[LanguageType:QualifiedName] name=ValidID;
  struct Association
      : rule<(VisibilityPrefix +
              kw<"association"> +
              assign<xref<&ast::Association::type, ast::LanguageType>>(ref<QualifiedName>) +
              assign<&ast::Association::name>(ref<ID>))> {};

  // Container returns Container:
  // attributes+=Attribute*
  // 'container' type=[ReferenceType:QualifiedName] MultiplicityDeclaration
  // name=ValidID
  // ('=' defaultComponent=[Component:QualifiedName])?;
  struct Container
      : rule<(Attributes +
              kw<"container"> +
              assign<&ast::Container::type>(ref<QualifiedName>) +
              MultiplicityDeclaration + assign<&ast::Container::name>(ref<ID>) +
              option(kw<"="> + assign<xref<&ast::Container::defaultComponent, ast::Component>>(
                                   ref<QualifiedName>)))> {};

  // Reference returns Reference:
  // attributes+=Attribute*
  // 'reference' interface=[ReferenceType:QualifiedName]
  // MultiplicityDeclaration name=ValidID;
  struct Reference
      : rule<(Attributes +
              kw<"reference"> +
              assign<&ast::Reference::interface_>(ref<QualifiedName>) +
              MultiplicityDeclaration +
              assign<&ast::Reference::name>(ref<ID>))> {};

  // EntryPoint returns EntryPoint:
  // attributes+=Attribute*
  // 'entrypoint' name=ValidID ('{' ('in' input+=[Field:QualifiedName] |
  // 'out' output+=[Field:QualifiedName])*'}')?;
  struct EntryPoint
      : rule<(Attributes +
              kw<"entrypoint"> + assign<&ast::EntryPoint::name>(ref<ID>) +
              option(kw<"{"> +
                     many((kw<"in"> + append<&ast::EntryPoint::input>(
                                          ref<QualifiedName>)) |
                          (kw<"out"> + append<&ast::EntryPoint::output>(
                                           ref<QualifiedName>))) +
                     kw<"}">))> {};

  // EventSink returns EventSink:
  // attributes+=Attribute*
  // 'eventsink' type=[EventType:QualifiedName] name=ValidID;
  struct EventSink
      : rule<(Attributes +
              kw<"eventsink"> +
              assign<xref<&ast::EventSink::type, ast::EventType>>(ref<QualifiedName>) +
              assign<&ast::EventSink::name>(ref<ID>))> {};

  // EventSource returns EventSource:
  // attributes+=Attribute*
  // 'eventsource' type=[EventType:QualifiedName] name=ValidID;
  struct EventSource
      : rule<(Attributes +
              kw<"eventsource"> +
              assign<xref<&ast::EventSource::type, ast::EventType>>(ref<QualifiedName>) +
              assign<&ast::EventSource::name>(ref<ID>))> {};

  // Attribute returns Attribute:
  // '@'type=[AttributeType:QualifiedName] ('(' value=Expression? ')')?;
  // Bare @ remains an editor form; argument parentheses belong to a typed
  // attribute, as in the native catalogue grammar.
  struct Attribute
      : rule<(kw<"@"> +
              option(assign<xref<&ast::Attribute::type, ast::AttributeType>>(ref<QualifiedName>) +
                     option(kw<"("> + option(assign<&ast::Attribute::value>(ref<Expression>)) + kw<")">)))> {};

  // entry Catalogue returns Catalogue:
  // attributes+=Attribute* 'catalogue' name=ValidID elements+=Namespace*;
  // Namespace returns Namespace:
  // attributes+=Attribute* 'namespace' name=ValidID
  // ( '::' elements+=NestedNamespace
  // | '{' elements+=(Namespace|Type)* '}' );
  // NestedNamespace returns Namespace:   // same, minus the 'namespace'
  // keyword
  // attributes+=Attribute* name=ValidID ( '::' ... | '{' ... );

  struct NestedNamespace;

  struct Namespace
      : rule<(Attributes +
              kw<"namespace"> + assign<&ast::Namespace::name>(ref<ID>) +
              ((kw<"::"> +
                append<&ast::Namespace::elements>(ref<NestedNamespace>)) |
               (kw<"{"> +
                many(append<&ast::Namespace::elements>(ref<Namespace> |
                                                       ref<Type>)) +
                kw<"}">)))> {};

  struct NestedNamespace
      : rule<(Attributes +
              assign<&ast::Namespace::name>(ref<ID>) +
              ((kw<"::"> +
                append<&ast::Namespace::elements>(ref<NestedNamespace>)) |
               (kw<"{"> +
                many(append<&ast::Namespace::elements>(ref<Namespace> |
                                                       ref<Type>)) +
                kw<"}">)))> {};

  struct Catalogue
      : rule<(Attributes +
              kw<"catalogue"> + assign<&ast::Catalogue::name>(ref<ID>) +
              many(append<&ast::Catalogue::elements>(ref<Namespace>)))> {};

  using entry = Catalogue;
  using ignored = WS;
  using hidden = list<ML_COMMENT, SL_COMMENT>;
};

}  // namespace xsmp
