// The pike language: its grammar.
#pragma once

#include <charconv>
#include <cstdint>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "pike/core/ast.hpp"

namespace pike {

struct Pike : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};

  static constexpr auto Letter = cr<"a-zA-Z_">;
  // `` `+ ``, `` `<< ``, ... -- fragment IDSYMBOL, longest spellings first so
  // `` `<< `` and `` `<= `` never lex as `` `< ``.
  static constexpr auto IdSymbol =
      lit<"`->="> | lit<"`->"> | lit<"`<<"> | lit<"`<="> | lit<"`<"> |
      lit<"`>>"> | lit<"`>="> | lit<"`>"> | lit<"`=="> | lit<"`!="> |
      lit<"`!"> | lit<"`()"> | lit<"`[]="> | lit<"`[]"> | lit<"`+"> |
      lit<"`/"> | lit<"`%"> | lit<"`*"> | lit<"`&"> | lit<"`|"> | lit<"`^"> |
      lit<"`~"> | lit<"`-">;

  static constexpr auto reserved = kw<"if"> | kw<"else"> | kw<"for"> | kw<"foreach"> |
      kw<"while"> | kw<"do"> | kw<"switch"> | kw<"case"> | kw<"default"> | kw<"break"> |
      kw<"continue"> | kw<"return"> | kw<"catch"> | kw<"gauge"> | kw<"sscanf"> |
      kw<"class"> | kw<"constant"> | kw<"inherit"> | kw<"import"> | kw<"lambda"> |
      kw<"private"> | kw<"public"> | kw<"protected"> | kw<"static"> | kw<"nomask"> |
      kw<"inline"> | kw<"final"> | kw<"extern"> | kw<"optional"> | kw<"local"> |
      kw<"variant"> | kw<"int"> | kw<"float"> | kw<"string"> | kw<"void"> |
      kw<"mixed"> | kw<"program"> | kw<"object"> | kw<"array"> | kw<"mapping"> |
      kw<"multiset"> | kw<"function"> | kw<"enum"> | kw<"typedef"> | kw<"typeof">;
  static constexpr auto IdentifierBody = IdSymbol | (Letter + many(Letter | d));
  struct ID : terminal<!reserved + IdentifierBody> {};
  struct MEMBER_ID : terminal<IdentifierBody> {};

  struct NUMBER : terminal<(lit<"0"> + cr<"xX"> + some(cr<"0-9a-fA-F">)) |
                           (lit<"0"> + cr<"bB"> + some(cr<"01">)) | some(d)> {};
  static constexpr auto Exponent = cr<"eE"> + option(cr<"+-">) + some(d);
  struct FLOAT : terminal<as<double>(
      ((option(some(d)) + lit<"."> + some(d)) + option(Exponent)) |
      (some(d) + Exponent))> {};
  struct STRING : terminal<lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\\r\n)">) + lit<"\"">> {};
  struct CHAR : terminal<lit<"'"> + some((lit<"\\"> + any) | cr<R"(^'\\\x00)">) + lit<"'">> {};

  struct OBJECTTYPE : terminal<kw<"object">> {};
  struct MAPPINGTYPE : terminal<kw<"mapping">> {};
  struct ARRAYTYPE : terminal<kw<"array">> {};
  struct MULTISETTYPE : terminal<kw<"multiset">> {};
  struct FUNCTIONTYPE : terminal<kw<"function">> {};

  // One terminal per modifier, each denoting its own constant -- `op` holds
  // what it means rather than what it spelled.
  struct PRIVATE : terminal<as<ast::Modifier::Private>(kw<"private">)> {};
  struct PUBLIC : terminal<as<ast::Modifier::Public>(kw<"public">)> {};

  static constexpr auto ModifierWord = as<ast::Modifier::Static>(kw<"static">) | ref<PRIVATE> |
                                       as<ast::Modifier::Nomask>(kw<"nomask">) | ref<PUBLIC> |
                                       as<ast::Modifier::Protected>(kw<"protected">) | as<ast::Modifier::Inline>(kw<"inline">) |
                                       as<ast::Modifier::Final>(kw<"final">) | as<ast::Modifier::Extern>(kw<"extern">) |
                                       as<ast::Modifier::Optional>(kw<"optional">) | as<ast::Modifier::Local>(kw<"local">) |
                                       as<ast::Modifier::Variant>(kw<"variant">);

  // Assignment operators, longest spelling first per level so `<<=` never
  // lexes as `<<`.

  static constexpr auto AssignOp =
      as<ast::AssignOp::ShiftLeft>(lit<"<<=">) | as<ast::AssignOp::ShiftRight>(lit<">>=">) | as<ast::AssignOp::Add>(lit<"+=">) | as<ast::AssignOp::Multiply>(lit<"*=">) |
      as<ast::AssignOp::Divide>(lit<"/=">) | as<ast::AssignOp::BitAnd>(lit<"&=">) | as<ast::AssignOp::BitOr>(lit<"|=">) | as<ast::AssignOp::BitXor>(lit<"^=">) |
      as<ast::AssignOp::Modulo>(lit<"%=">) | as<ast::AssignOp::Subtract>(lit<"-=">) | as<ast::AssignOp::Assign>(lit<"=">);

  // Longest spelling first: `||`/`&&` before `|`/`&`, `==`/`!=` before
  // nothing shorter, `<=`/`<<` and `>=`/`>>` before `<`/`>`.
  struct PLUS : terminal<as<ast::BinOp::Add>(lit<"+">)> {};

  // Pike's own precedence table, tightest first.
  static constexpr auto BinaryOpLevels = levels(
      left(as<ast::BinOp::Multiply>(lit<"*">) | as<ast::BinOp::Divide>(lit<"/">) | as<ast::BinOp::Modulo>(lit<"%">)), left(ref<PLUS> | as<ast::BinOp::Subtract>(lit<"-">)),
      left(as<ast::BinOp::ShiftRight>(lit<">>">) | as<ast::BinOp::ShiftLeft>(lit<"<<">)), left(as<ast::BinOp::Greater>(lit<">">) | as<ast::BinOp::GreaterEqual>(lit<">=">) | as<ast::BinOp::Less>(lit<"<">) | as<ast::BinOp::LessEqual>(lit<"<=">)),
      left(as<ast::BinOp::Equal>(lit<"==">) | as<ast::BinOp::NotEqual>(lit<"!=">)), left(as<ast::BinOp::BitAnd>(lit<"&">)), left(as<ast::BinOp::BitXor>(lit<"^">)),
      left(as<ast::BinOp::BitOr>(lit<"|">)), left(as<ast::BinOp::And>(lit<"&&">)), left(as<ast::BinOp::Or>(lit<"||">)));

  struct ConstantIdentifier;
  struct TypeRule;
  struct Argument;
  struct Block;
  struct Statement;
  struct Expression;
  struct Expr2;
  struct Expr3;
  struct Expr4;
  struct Expr5;
  struct Expr6;
  struct Primary;
  struct ArgItem;
  struct LValue;
  struct Program;
  struct Definition;
  struct Variables;
  struct Import;
  struct Constant;

  // A dotted name, shared by imports, inheritance targets and bare-name
  // expressions.
  struct ConstantIdentifier
      : rule<append<&ast::ConstantIdentifier::parts>(ref<ID>) +
             many(lit<"."> +
                  append<&ast::ConstantIdentifier::parts>(ref<ID>))> {};

  // Inlined into `TypeRule`'s `FUNCTIONTYPE` branch rather than its own
  // rule: its `append`/`flag` target `FunctionType`, so a separate
  // rule would deduce that class itself and create a second node.
  static constexpr auto FunctionTypeTail =
      lit<"("> +
      option(
          append<&ast::FunctionType::params>(ref<TypeRule>) +
          many(lit<","> + append<&ast::FunctionType::params>(ref<TypeRule>))) +
      option(flag<&ast::FunctionType::variadic>(lit<"...">)) +
      option(lit<":"> + assign<&ast::FunctionType::returnType>(ref<TypeRule>)) + lit<")">;

  // The trailing `*`s collapse to one flag, written by every branch itself
  // (`Type::pointer` sits on the base, but a choice inside a further
  // sequence may not create more than once).
  static constexpr auto Stars =
      option(flag<&ast::Type::pointer>(some(lit<"*">)));

  struct TypeBound : rule<option(lit<"-">) + ref<NUMBER>> {};
  static constexpr auto IntRange = lit<"("> +
      option(assign<&ast::SimpleType::lower>(ref<TypeBound>)) +
      flag<&ast::SimpleType::hasRange>(lit<"..">) +
      option(assign<&ast::SimpleType::upper>(ref<TypeBound>)) + lit<")">;

  struct TypeAtom
      : rule<create<ast::SimpleType>(assign<&ast::SimpleType::kind>(as<ast::SimpleTypeKind::Int>(kw<"int">)) +
                                     option(IntRange) + Stars) |
             create<ast::SimpleType>(assign<&ast::SimpleType::kind>(
                                         as<ast::SimpleTypeKind::Float>(kw<"float">) |
                                         as<ast::SimpleTypeKind::String>(kw<"string">) | as<ast::SimpleTypeKind::Program>(kw<"program">) |
                                         as<ast::SimpleTypeKind::Void>(kw<"void">) | as<ast::SimpleTypeKind::Mixed>(kw<"mixed">)) +
                                     Stars) //
             | create<ast::ObjectType>(ref<OBJECTTYPE> +
                                       option(lit<"("> +
                                              assign<&ast::ObjectType::program>(
                                                  ref<ConstantIdentifier>) +
                                              lit<")">) +
                                       Stars) //
             | create<ast::MappingType>(
                   ref<MAPPINGTYPE> +
                   option(lit<"("> +
                          assign<&ast::MappingType::key>(ref<TypeRule>) +
                          lit<":"> +
                          assign<&ast::MappingType::value>(ref<TypeRule>) +
                          lit<")">) +
                   Stars) //
             | create<ast::ArrayType>(
                   ref<ARRAYTYPE> +
                   option(lit<"("> +
                          assign<&ast::ArrayType::element>(ref<TypeRule>) +
                          lit<")">) +
                   Stars) //
             | create<ast::MultisetType>(
                   ref<MULTISETTYPE> +
                   option(lit<"("> +
                          assign<&ast::MultisetType::element>(ref<TypeRule>) +
                          lit<")">) +
                   Stars) //
             | create<ast::FunctionType>(ref<FUNCTIONTYPE> +
                                         option(FunctionTypeTail) + Stars)> {};

  struct TypeUnion : terminal<lit<"|">> {};
  struct TypeRule : rule<ref<TypeAtom> +
      many(nest<ast::UnionType, &ast::UnionType::left>(ref<TypeUnion> +
          assign<&ast::UnionType::right>(ref<TypeAtom>)))> {};

  struct Argument
      : rule<assign<&ast::Argument::type>(ref<TypeRule>) +
             option(flag<&ast::Argument::variadic>(lit<"...">)) +
             assign<&ast::Argument::name>(ref<ID>)> {};

  struct ArgItem : rule<create<ast::ArgItem>(
                           option(flag<&ast::ArgItem::splice>(lit<"@">)) +
                           assign<&ast::ArgItem::value>(ref<Expr2>))> {};

  // Always a `CommaExpr`, even for one item, so every caller reads one
  // uniform class.
  struct Expression
      : rule<append<&ast::CommaExpr::items>(ref<Expr2>) +
             many(lit<","> + append<&ast::CommaExpr::items>(ref<Expr2>))> {};

  struct LValue
      : rule<create<ast::LambdaLValue>(
                 kw<"lambda"> +
                 assign<&ast::LambdaLValue::target>(ref<Expr6>)) //
             | create<ast::DeclLValue>(
                   assign<&ast::DeclLValue::type>(ref<TypeRule>) +
                   assign<&ast::DeclLValue::name>(ref<ID>)) //
             | create<ast::TargetLValue>(
                   assign<&ast::TargetLValue::target>(ref<Expr6>))> {};

  // Right recursion: zero or more `lvalue op` pairs before a final Expr3.
  struct Expr2 : rule<create<ast::Assignment>(
                          assign<&ast::Assignment::lvalue>(ref<LValue>) +
                          assign<&ast::Assignment::op>(AssignOp) +
                          assign<&ast::Assignment::right>(ref<Expr2>)) //
                      | ref<Expr3>> {};

  struct Expr3 : rule<create<ast::Ternary>(
                          assign<&ast::Ternary::cond>(ref<Expr4>) + lit<"?"> +
                          assign<&ast::Ternary::then>(ref<Expr3>) + lit<":"> +
                          assign<&ast::Ternary::else_>(ref<Expr3>)) //
                      | ref<Expr4>> {};

  struct Expr4 : infix<Expr5, ast::BinaryExpr, BinaryOpLevels> {};

  struct Expr5
      : rule<create<ast::CastExpr>(
                 lit<"("> + assign<&ast::CastExpr::type>(ref<TypeRule>) +
                 lit<")"> + assign<&ast::CastExpr::operand>(ref<Expr5>)) //
             | create<ast::UnaryExpr>(
                   assign<&ast::UnaryExpr::op>(lit<"--"> | lit<"++">) +
                   assign<&ast::UnaryExpr::operand>(ref<Expr6>)) //
             | create<ast::UnaryExpr>(
                   assign<&ast::UnaryExpr::op>(lit<"~"> | lit<"-"> | lit<"!">) +
                   assign<&ast::UnaryExpr::operand>(ref<Expr5>)) //
             | (ref<Expr6> +
                option(nest<ast::UnaryExpr, &ast::UnaryExpr::operand>(
                    assign<&ast::UnaryExpr::op>(lit<"--"> | lit<"++">))))> {};

  struct Expr6
      : rule<ref<Primary> +
             many(nest<ast::CallExpr, &ast::CallExpr::callee>(
                      lit<"("> +
                      option(append<&ast::CallExpr::args>(ref<ArgItem>) +
                             many(lit<","> + append<&ast::CallExpr::args>(
                                                 ref<ArgItem>))) +
                      option(lit<",">) + lit<")">) //
                  | nest<ast::MemberExpr, &ast::MemberExpr::object>(
                        lit<"->"> + assign<&ast::MemberExpr::field>(ref<MEMBER_ID>)) //
                  | nest<ast::IndexExpr, &ast::IndexExpr::object>(
                        lit<"["> +
                        ((assign<&ast::IndexExpr::index>(ref<Expression>) +
                          option(flag<&ast::IndexExpr::range>(lit<"..">) +
                                 option(assign<&ast::IndexExpr::rangeEnd>(ref<Expression>)))) |
                         (flag<&ast::IndexExpr::range>(lit<"..">) +
                          option(assign<&ast::IndexExpr::rangeEnd>(ref<Expression>)))) +
                        lit<"]">))> {};


  struct Catch
      : rule<create<ast::CatchExpr>(
            kw<"catch"> + (assign<&ast::CatchExpr::body>(
                               lit<"("> + ref<Expression> + lit<")">) //
                           | assign<&ast::CatchExpr::body>(ref<Block>)))> {};

  struct Gauge
      : rule<create<ast::GaugeExpr>(
            kw<"gauge"> + (assign<&ast::GaugeExpr::body>(
                               lit<"("> + ref<Expression> + lit<")">) //
                           | assign<&ast::GaugeExpr::body>(ref<Block>)))> {};

  struct Sscanf
      : rule<create<ast::SscanfExpr>(
            kw<"sscanf"> + lit<"("> +
            assign<&ast::SscanfExpr::input>(ref<Expr2>) + lit<","> +
            assign<&ast::SscanfExpr::format>(ref<Expr2>) +
            many(lit<","> +
                 append<&ast::SscanfExpr::targets>(ref<LValue>)) +
            lit<")">)> {};

  struct Lambda
      : rule<create<ast::LambdaExpr>(
            kw<"lambda"> + lit<"("> +
            option(
                append<&ast::LambdaExpr::args>(ref<Argument>) +
                many(lit<","> + append<&ast::LambdaExpr::args>(ref<Argument>)) +
                option(lit<",">)) +
            lit<")"> + assign<&ast::LambdaExpr::body>(ref<Block>))> {};

  template<auto Params, auto Present>
  static constexpr auto ConstructorParams = option(flag<Present>(lit<"("> +
      option(some(append<Params>(ref<Argument>), lit<",">) + option(lit<",">)) + lit<")">));

  // `class_implementation: 'class' identifier? '{' program '}'`, reachable
  // only as an expression (never from `definition`; see ast.hpp).
  struct ClassImplementation
      : rule<create<ast::ClassExpr>(
            kw<"class"> + option(assign<&ast::ClassExpr::name>(ref<ID>)) +
            ConstructorParams<&ast::ClassExpr::constructorParams, &ast::ClassExpr::hasConstructorParams> +
            lit<"{"> + assign<&ast::ClassExpr::body>(ref<Program>) +
            lit<"}">)> {};

  struct MapEntry
      : rule<assign<&ast::MapEntry::key>(ref<Expression>) + lit<":"> +
             assign<&ast::MapEntry::value>(ref<Expression>)> {};

  struct Mapping
      : rule<create<ast::MappingLit>(
            lit<"(["> +
            option(append<&ast::MappingLit::entries>(ref<MapEntry>) +
                   many(lit<","> +
                        append<&ast::MappingLit::entries>(ref<MapEntry>))) +
            option(lit<",">) + lit<"])">)> {};

  struct Multiset
      : rule<create<ast::MultisetLit>(
            lit<"(<"> +
            option(append<&ast::MultisetLit::items>(ref<ArgItem>) +
                   many(lit<","> +
                        append<&ast::MultisetLit::items>(ref<ArgItem>))) +
            option(lit<",">) + lit<">)">)> {};

  struct Array
      : rule<create<ast::ArrayLit>(
            lit<"({"> +
            option(append<&ast::ArrayLit::items>(ref<ArgItem>) +
                   many(lit<","> +
                        append<&ast::ArrayLit::items>(ref<ArgItem>))) +
            option(lit<",">) + lit<"})">)> {};

  struct Paren
      : rule<create<ast::ParenExpr>(
            lit<"("> + assign<&ast::ParenExpr::inner>(ref<Expression>) +
            lit<")">)> {};

  struct StringLiteral : rule<create<ast::StringLit>(
      some(append<&ast::StringLit::pieces>(ref<STRING>)))> {};

  struct Primary : rule<ref<StringLiteral> | create<ast::CharLit>(assign<&ast::CharLit::text>(ref<CHAR>))   //
                        | assign<&ast::NumberLit::value>(ref<NUMBER>) //
                        | assign<&ast::FloatLit::value>(ref<FLOAT>)   //
                        | ref<Catch>                              //
                        | ref<Gauge>                              //
                        | ref<Sscanf>                             //
                        | ref<Lambda>                             //
                        | ref<ClassImplementation>                    //
                        | ref<ConstantIdentifier>                 //
                        | ref<Mapping>                            //
                        | ref<Multiset>                           //
                        | ref<Array>                              //
                        | ref<Paren>> {};

  struct Block : rule<create<ast::Block>(
                     lit<"{"> +
                     many(append<&ast::Block::statements>(ref<Statement>)) +
                     lit<"}">)> {};

  struct If
      : rule<create<ast::If>(
            kw<"if"> + lit<"("> + assign<&ast::If::cond>(ref<Expression>) +
            lit<")"> + assign<&ast::If::then>(ref<Statement>) +
            option(commit(kw<"else">, assign<&ast::If::else_>(ref<Statement>))))> {
  };

  struct While
      : rule<create<ast::While>(
            kw<"while"> + lit<"("> +
            assign<&ast::While::cond>(ref<Expression>) + lit<")"> +
            assign<&ast::While::body>(ref<Statement>))> {};

  struct DoWhile
      : rule<create<ast::DoWhile>(
            kw<"do"> + assign<&ast::DoWhile::body>(ref<Statement>) +
            kw<"while"> + lit<"("> +
            assign<&ast::DoWhile::cond>(ref<Expression>) + lit<")"> +
            lit<";">)> {};

  struct For
      : rule<create<ast::For>(
            kw<"for"> + lit<"("> +
            option(assign<&ast::For::init>(ref<Expression>)) + lit<";"> +
            option(assign<&ast::For::cond>(ref<Expression>)) + lit<";"> +
            option(assign<&ast::For::step>(ref<Expression>)) + lit<")"> +
            assign<&ast::For::body>(ref<Statement>))> {};

  struct Switch : rule<create<ast::Switch>(
                          kw<"switch"> + lit<"("> +
                          assign<&ast::Switch::value>(ref<Expression>) +
                          lit<")"> + assign<&ast::Switch::body>(ref<Block>))> {
  };

  struct Case
      : rule<create<ast::Case>(
            kw<"case"> + assign<&ast::Case::value>(ref<Expression>) +
            option(lit<".."> +
                   assign<&ast::Case::rangeEnd>(ref<Expression>)) +
            lit<":">)> {};

  struct Default : rule<create<ast::Default> + kw<"default"> + lit<":">> {};

  struct Foreach
      : rule<create<ast::Foreach>(
            kw<"foreach"> + lit<"("> +
            assign<&ast::Foreach::collection>(ref<Expr2>) +
            ((lit<","> + assign<&ast::Foreach::variable>(ref<LValue>)) |
             (lit<";"> + option(assign<&ast::Foreach::key>(ref<LValue>)) + lit<";"> +
              option(assign<&ast::Foreach::variable>(ref<LValue>)))) + lit<")"> +
            assign<&ast::Foreach::body>(ref<Statement>))> {};

  struct Break : rule<create<ast::Break> + kw<"break"> + lit<";">> {};
  struct Continue
      : rule<create<ast::Continue> + kw<"continue"> + lit<";">> {};

  struct Return : rule<create<ast::Return>(
      kw<"return"> + option(assign<&ast::Return::value>(ref<Expression>)) + lit<";">)> {};

  struct LocalDeclaration : rule<create<ast::LocalDeclaration>(
      assign<&ast::LocalDeclaration::declaration>(ref<Variables> | ref<Import> | ref<Constant>))> {};

  struct Statement
      : rule<ref<LocalDeclaration> | create<ast::ExprStatement>(
                 assign<&ast::ExprStatement::expr>(ref<Expression>) +
                 lit<";">)       //
             | ref<If>       //
             | ref<While>    //
             | ref<DoWhile>  //
             | ref<For>      //
             | ref<Switch>   //
             | ref<Case>     //
             | ref<Default>  //
             | ref<Block>        //
             | ref<Return>   //
             | ref<Foreach>  //
             | ref<Break>    //
             | ref<Continue> //
             | (create<ast::EmptyStatement> + lit<";">)> {};

  struct Import
      : rule<many(append<&ast::Definition::modifiers>(ModifierWord)) +
             kw<"import"> +
             assign<&ast::Import::path>(ref<ConstantIdentifier>) +
             lit<";">> {};

  struct Inheritance
      : rule<many(append<&ast::Definition::modifiers>(ModifierWord)) +
             kw<"inherit"> +
             assign<&ast::Inheritance::program>(ref<ConstantIdentifier> | ref<StringLiteral>) +
             option(lit<":"> + assign<&ast::Inheritance::alias>(ref<ID>)) +
             lit<";">> {};

  struct FunctionDeclaration
      : rule<many(append<&ast::Definition::modifiers>(ModifierWord)) +
             assign<&ast::FunctionDeclaration::returnType>(ref<TypeRule>) +
             assign<&ast::FunctionDeclaration::name>(ref<ID>) + lit<"("> +
             option(append<&ast::FunctionDeclaration::args>(ref<Argument>) +
                    many(lit<","> + append<&ast::FunctionDeclaration::args>(
                                        ref<Argument>)) +
                    option(lit<",">)) +
             lit<")"> + lit<";">> {};

  struct FunctionDefinition
      : rule<many(append<&ast::Definition::modifiers>(ModifierWord)) +
             assign<&ast::FunctionDefinition::returnType>(ref<TypeRule>) +
             assign<&ast::FunctionDefinition::name>(ref<ID>) + lit<"("> +
             option(append<&ast::FunctionDefinition::args>(ref<Argument>) +
                    many(lit<","> + append<&ast::FunctionDefinition::args>(
                                        ref<Argument>)) +
                    option(lit<",">)) +
             lit<")"> + assign<&ast::FunctionDefinition::body>(ref<Block>)> {};

  struct VariableDecl
      : rule<create<ast::VariableDecl>(
            option(flag<&ast::VariableDecl::pointer>(some(lit<"*">))) +
            assign<&ast::VariableDecl::name>(ref<ID>) +
            option(lit<"="> + assign<&ast::VariableDecl::init>(ref<Expr2>)))> {
  };

  struct Variables
      : rule<many(append<&ast::Definition::modifiers>(ModifierWord)) +
             assign<&ast::Variables::type>(ref<TypeRule>) +
             append<&ast::Variables::names>(ref<VariableDecl>) +
             many(lit<","> +
                  append<&ast::Variables::names>(ref<VariableDecl>)) +
             lit<";">> {};

  struct ConstantDecl
      : rule<create<ast::ConstantDecl>(
            assign<&ast::ConstantDecl::name>(ref<ID>) + lit<"="> +
            assign<&ast::ConstantDecl::value>(ref<Expr2>))> {};

  struct Constant
      : rule<many(append<&ast::Definition::modifiers>(ModifierWord)) +
             kw<"constant"> +
             append<&ast::Constant::names>(ref<ConstantDecl>) +
             many(lit<","> +
                  append<&ast::Constant::names>(ref<ConstantDecl>)) +
             lit<";">> {};

  struct ClassDef : rule<create<ast::ClassDef>(
      many(append<&ast::Definition::modifiers>(ModifierWord)) +
      kw<"class"> + option(assign<&ast::ClassDef::name>(ref<ID>)) +
      ConstructorParams<&ast::ClassDef::constructorParams, &ast::ClassDef::hasConstructorParams> +
      lit<"{"> + assign<&ast::ClassDef::body>(ref<Program>) + lit<"}"> + option(lit<";">))> {};

  struct Definition : rule<ref<Import>                //
                               | ref<Inheritance>         //
                               | ref<FunctionDeclaration> //
                               | ref<FunctionDefinition>  //
                               | ref<Variables>           //
                               | ref<Constant>            //
                               | ref<ClassDef>> {};

  struct Program
      : rule<create<ast::Program> +
             many(append<&ast::Program::definitions>(ref<Definition>))> {};

  using entry = Program;
  struct LINE_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct BLOCK_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  using ignored = WS;
  using hidden = list<LINE_COMMENT, BLOCK_COMMENT>;
};

} // namespace pike
