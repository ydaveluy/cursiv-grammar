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

  struct ID : terminal<Letter + many(Letter | d | IdSymbol)> {};

  struct NUMBER : terminal<(lit<"0"> + cr<"xX"> + some(cr<"0-9a-fA-F">)) |
                           (lit<"0"> + cr<"bB"> + some(cr<"01">)) | some(d)> {};
  struct FLOAT : terminal<some(d) + lit<"."> + some(d), double> {};
  struct STRING : terminal<lit<"\""> + many(cr<R"(^")">) + lit<"\"">> {};

  struct INTTYPE : terminal<kw<"int">, value<ast::SimpleTypeKind::Int>> {};
  struct FLOATTYPE : terminal<kw<"float">, value<ast::SimpleTypeKind::Float>> {
  };
  struct STRINGTYPE
      : terminal<kw<"string">, value<ast::SimpleTypeKind::String>> {};
  struct PROGRAMTYPE
      : terminal<kw<"program">, value<ast::SimpleTypeKind::Program>> {};
  struct OBJECTTYPE : terminal<kw<"object">> {};
  struct MAPPINGTYPE : terminal<kw<"mapping">> {};
  struct ARRAYTYPE : terminal<kw<"array">> {};
  struct MULTISETTYPE : terminal<kw<"multiset">> {};
  struct FUNCTIONTYPE : terminal<kw<"function">> {};

  // One terminal per modifier, each denoting its own constant -- `op` holds
  // what it means rather than what it spelled.
  struct STATIC : terminal<kw<"static">, value<ast::Modifier::Static>> {};
  struct PRIVATE : terminal<kw<"private">, value<ast::Modifier::Private>> {};
  struct NOMASK : terminal<kw<"nomask">, value<ast::Modifier::Nomask>> {};
  struct PUBLIC : terminal<kw<"public">, value<ast::Modifier::Public>> {};
  struct PROTECTED
      : terminal<kw<"protected">, value<ast::Modifier::Protected>> {};
  struct INLINE : terminal<kw<"inline">, value<ast::Modifier::Inline>> {};

  static constexpr auto ModifierWord = ref<STATIC> | ref<PRIVATE> |
                                       ref<NOMASK> | ref<PUBLIC> |
                                       ref<PROTECTED> | ref<INLINE>;

  // Assignment operators, longest spelling first per level so `<<=` never
  // lexes as `<<`.
  struct ASSIGN : terminal<lit<"=">, value<ast::AssignOp::Assign>> {};
  struct ADD_ASSIGN : terminal<lit<"+=">, value<ast::AssignOp::Add>> {};
  struct SUB_ASSIGN : terminal<lit<"-=">, value<ast::AssignOp::Subtract>> {};
  struct MUL_ASSIGN : terminal<lit<"*=">, value<ast::AssignOp::Multiply>> {};
  struct DIV_ASSIGN : terminal<lit<"/=">, value<ast::AssignOp::Divide>> {};
  struct AND_ASSIGN : terminal<lit<"&=">, value<ast::AssignOp::BitAnd>> {};
  struct OR_ASSIGN : terminal<lit<"|=">, value<ast::AssignOp::BitOr>> {};
  struct XOR_ASSIGN : terminal<lit<"^=">, value<ast::AssignOp::BitXor>> {};
  struct SHL_ASSIGN : terminal<lit<"<<=">, value<ast::AssignOp::ShiftLeft>> {};
  struct SHR_ASSIGN : terminal<lit<">>=">, value<ast::AssignOp::ShiftRight>> {};
  struct MOD_ASSIGN : terminal<lit<"%=">, value<ast::AssignOp::Modulo>> {};

  static constexpr auto AssignOp =
      ref<SHL_ASSIGN> | ref<SHR_ASSIGN> | ref<ADD_ASSIGN> | ref<MUL_ASSIGN> |
      ref<DIV_ASSIGN> | ref<AND_ASSIGN> | ref<OR_ASSIGN> | ref<XOR_ASSIGN> |
      ref<MOD_ASSIGN> | ref<SUB_ASSIGN> | ref<ASSIGN>;

  // Longest spelling first: `||`/`&&` before `|`/`&`, `==`/`!=` before
  // nothing shorter, `<=`/`<<` and `>=`/`>>` before `<`/`>`.
  struct OROR : terminal<lit<"||">, value<ast::BinOp::Or>> {};
  struct ANDAND : terminal<lit<"&&">, value<ast::BinOp::And>> {};
  struct EQ : terminal<lit<"==">, value<ast::BinOp::Equal>> {};
  struct NEQ : terminal<lit<"!=">, value<ast::BinOp::NotEqual>> {};
  struct SHL : terminal<lit<"<<">, value<ast::BinOp::ShiftLeft>> {};
  struct SHR : terminal<lit<">>">, value<ast::BinOp::ShiftRight>> {};
  struct LE : terminal<lit<"<=">, value<ast::BinOp::LessEqual>> {};
  struct GE : terminal<lit<">=">, value<ast::BinOp::GreaterEqual>> {};
  struct LT : terminal<lit<"<">, value<ast::BinOp::Less>> {};
  struct GT : terminal<lit<">">, value<ast::BinOp::Greater>> {};
  struct BOR : terminal<lit<"|">, value<ast::BinOp::BitOr>> {};
  struct BXOR : terminal<lit<"^">, value<ast::BinOp::BitXor>> {};
  struct BAND : terminal<lit<"&">, value<ast::BinOp::BitAnd>> {};
  struct PLUS : terminal<lit<"+">, value<ast::BinOp::Add>> {};
  struct MINUS : terminal<lit<"-">, value<ast::BinOp::Subtract>> {};
  struct STAR : terminal<lit<"*">, value<ast::BinOp::Multiply>> {};
  struct SLASH : terminal<lit<"/">, value<ast::BinOp::Divide>> {};
  struct PERCENT : terminal<lit<"%">, value<ast::BinOp::Modulo>> {};

  // Pike's own precedence table, tightest first.
  static constexpr auto BinaryOpLevels =
      (left(ref<STAR> | ref<SLASH> | ref<PERCENT>) >>
       left(ref<PLUS> | ref<MINUS>) >> left(ref<SHR> | ref<SHL>) >>
       left(ref<GT> | ref<GE> | ref<LT> | ref<LE>) >>
       left(ref<EQ> | ref<NEQ>) >> left(ref<BAND>) >> left(ref<BXOR>) >>
       left(ref<BOR>) >> left(ref<ANDAND>) >> left(ref<OROR>));

  struct ConstantIdentifierRule;
  struct TypeRule;
  struct Argument;
  struct Block;
  struct StatementRule;
  struct ExpressionRule;
  struct Expr2;
  struct Expr3;
  struct Expr4;
  struct Expr5;
  struct Expr6;
  struct Primary;
  struct ArgItemRule;
  struct LValueRule;
  struct ProgramRule;
  struct DefinitionRule;

  // A dotted name, shared by imports, inheritance targets and bare-name
  // expressions.
  struct ConstantIdentifierRule
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
      option(flag<&ast::FunctionType::variadic>(lit<"...">)) + lit<")">;

  // The trailing `*`s collapse to one flag, written by every branch itself
  // (`Type::pointer` sits on the base, but a choice inside a further
  // sequence may not create more than once).
  static constexpr auto Stars =
      option(flag<&ast::Type::pointer>(some(lit<"*">)));

  struct TypeRule
      : rule<create<ast::SimpleType>(assign<&ast::SimpleType::kind>(
                                         ref<INTTYPE> | ref<FLOATTYPE> |
                                         ref<STRINGTYPE> | ref<PROGRAMTYPE>) +
                                     Stars) //
             | create<ast::ObjectType>(ref<OBJECTTYPE> +
                                       option(lit<"("> +
                                              assign<&ast::ObjectType::program>(
                                                  ref<ConstantIdentifierRule>) +
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

  struct Argument
      : rule<assign<&ast::Argument::type>(ref<TypeRule>) +
             option(flag<&ast::Argument::variadic>(lit<"...">)) +
             assign<&ast::Argument::name>(ref<ID>)> {};

  struct ArgItemRule : rule<create<ast::ArgItem>(
                           option(flag<&ast::ArgItem::splice>(lit<"@">)) +
                           assign<&ast::ArgItem::value>(ref<Expr2>))> {};

  // Always a `CommaExpr`, even for one item, so every caller reads one
  // uniform class.
  struct ExpressionRule
      : rule<append<&ast::CommaExpr::items>(ref<Expr2>) +
             many(lit<","> + append<&ast::CommaExpr::items>(ref<Expr2>))> {};

  struct LValueRule
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
                          assign<&ast::Assignment::lvalue>(ref<LValueRule>) +
                          assign<&ast::Assignment::op>(AssignOp) +
                          assign<&ast::Assignment::right>(ref<Expr2>)) //
                      | ref<Expr3>> {};

  struct Expr3 : rule<create<ast::Ternary>(
                          assign<&ast::Ternary::cond>(ref<Expr4>) + lit<"?"> +
                          assign<&ast::Ternary::then>(ref<Expr3>) + lit<":"> +
                          assign<&ast::Ternary::else_>(ref<Expr3>)) //
                      | ref<Expr4>> {};

  struct Expr4 : infix<Expr5, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
                       &ast::BinaryExpr::right, BinaryOpLevels> {};

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
                      option(append<&ast::CallExpr::args>(ref<ArgItemRule>) +
                             many(lit<","> + append<&ast::CallExpr::args>(
                                                 ref<ArgItemRule>))) +
                      option(lit<",">) + lit<")">) //
                  | nest<ast::MemberExpr, &ast::MemberExpr::object>(
                        lit<"->"> + assign<&ast::MemberExpr::field>(ref<ID>)) //
                  | nest<ast::IndexExpr, &ast::IndexExpr::object>(
                        lit<"["> +
                        assign<&ast::IndexExpr::index>(ref<ExpressionRule>) +
                        option(lit<".."> + assign<&ast::IndexExpr::rangeEnd>(
                                               ref<ExpressionRule>)) +
                        lit<"]">))> {};

  struct CatchRule
      : rule<create<ast::CatchExpr>(
            kw<"catch"> + (assign<&ast::CatchExpr::body>(
                               lit<"("> + ref<ExpressionRule> + lit<")">) //
                           | assign<&ast::CatchExpr::body>(ref<Block>)))> {};

  struct GaugeRule
      : rule<create<ast::GaugeExpr>(
            kw<"gauge"> + (assign<&ast::GaugeExpr::body>(
                               lit<"("> + ref<ExpressionRule> + lit<")">) //
                           | assign<&ast::GaugeExpr::body>(ref<Block>)))> {};

  struct SscanfRule
      : rule<create<ast::SscanfExpr>(
            kw<"sscanf"> + lit<"("> +
            assign<&ast::SscanfExpr::input>(ref<Expr2>) + lit<","> +
            assign<&ast::SscanfExpr::format>(ref<Expr2>) +
            many(lit<","> +
                 append<&ast::SscanfExpr::targets>(ref<LValueRule>)) +
            lit<")">)> {};

  struct LambdaRule
      : rule<create<ast::LambdaExpr>(
            kw<"lambda"> + lit<"("> +
            option(
                append<&ast::LambdaExpr::args>(ref<Argument>) +
                many(lit<","> + append<&ast::LambdaExpr::args>(ref<Argument>)) +
                option(lit<",">)) +
            lit<")"> + assign<&ast::LambdaExpr::body>(ref<Block>))> {};

  // `class_implementation: 'class' identifier? '{' program '}'`, reachable
  // only as an expression (never from `definition`; see ast.hpp).
  struct ClassImplementation
      : rule<create<ast::ClassExpr>(
            kw<"class"> + option(assign<&ast::ClassExpr::name>(ref<ID>)) +
            lit<"{"> + assign<&ast::ClassExpr::body>(ref<ProgramRule>) +
            lit<"}">)> {};

  struct MapEntryRule
      : rule<assign<&ast::MapEntry::key>(ref<ExpressionRule>) + lit<":"> +
             assign<&ast::MapEntry::value>(ref<ExpressionRule>)> {};

  struct MappingRule
      : rule<create<ast::MappingLit>(
            lit<"(["> +
            option(append<&ast::MappingLit::entries>(ref<MapEntryRule>) +
                   many(lit<","> +
                        append<&ast::MappingLit::entries>(ref<MapEntryRule>))) +
            option(lit<",">) + lit<"])">)> {};

  struct MultisetRule
      : rule<create<ast::MultisetLit>(
            lit<"(<"> +
            option(append<&ast::MultisetLit::items>(ref<ArgItemRule>) +
                   many(lit<","> +
                        append<&ast::MultisetLit::items>(ref<ArgItemRule>))) +
            option(lit<",">) + lit<">)">)> {};

  struct ArrayRule
      : rule<create<ast::ArrayLit>(
            lit<"({"> +
            option(append<&ast::ArrayLit::items>(ref<ArgItemRule>) +
                   many(lit<","> +
                        append<&ast::ArrayLit::items>(ref<ArgItemRule>))) +
            option(lit<",">) + lit<"})">)> {};

  struct ParenRule
      : rule<create<ast::ParenExpr>(
            lit<"("> + assign<&ast::ParenExpr::inner>(ref<ExpressionRule>) +
            lit<")">)> {};

  struct Primary : rule<assign<&ast::StringLit::value>(ref<STRING>)   //
                        | assign<&ast::NumberLit::value>(ref<NUMBER>) //
                        | assign<&ast::FloatLit::value>(ref<FLOAT>)   //
                        | ref<CatchRule>                              //
                        | ref<GaugeRule>                              //
                        | ref<SscanfRule>                             //
                        | ref<LambdaRule>                             //
                        | ref<ClassImplementation>                    //
                        | ref<ConstantIdentifierRule>                 //
                        | ref<MappingRule>                            //
                        | ref<MultisetRule>                           //
                        | ref<ArrayRule>                              //
                        | ref<ParenRule>> {};

  struct Block : rule<create<ast::Block>(
                     lit<"{"> +
                     many(append<&ast::Block::statements>(ref<StatementRule>)) +
                     lit<"}">)> {};

  struct IfRule
      : rule<create<ast::If>(
            kw<"if"> + lit<"("> + assign<&ast::If::cond>(ref<ExpressionRule>) +
            lit<")"> + assign<&ast::If::then>(ref<StatementRule>) +
            option(kw<"else"> + assign<&ast::If::else_>(ref<StatementRule>)))> {
  };

  struct WhileRule
      : rule<create<ast::While>(
            kw<"while"> + lit<"("> +
            assign<&ast::While::cond>(ref<ExpressionRule>) + lit<")"> +
            assign<&ast::While::body>(ref<StatementRule>))> {};

  struct DoWhileRule
      : rule<create<ast::DoWhile>(
            kw<"do"> + assign<&ast::DoWhile::body>(ref<StatementRule>) +
            kw<"while"> + lit<"("> +
            assign<&ast::DoWhile::cond>(ref<ExpressionRule>) + lit<")"> +
            lit<";">)> {};

  struct ForRule
      : rule<create<ast::For>(
            kw<"for"> + lit<"("> +
            option(assign<&ast::For::init>(ref<ExpressionRule>)) + lit<";"> +
            option(assign<&ast::For::cond>(ref<ExpressionRule>)) + lit<";"> +
            option(assign<&ast::For::step>(ref<ExpressionRule>)) + lit<")"> +
            assign<&ast::For::body>(ref<StatementRule>))> {};

  struct SwitchRule : rule<create<ast::Switch>(
                          kw<"switch"> + lit<"("> +
                          assign<&ast::Switch::value>(ref<ExpressionRule>) +
                          lit<")"> + assign<&ast::Switch::body>(ref<Block>))> {
  };

  struct CaseRule
      : rule<create<ast::Case>(
            kw<"case"> + assign<&ast::Case::value>(ref<ExpressionRule>) +
            option(lit<".."> +
                   assign<&ast::Case::rangeEnd>(ref<ExpressionRule>)) +
            lit<":">)> {};

  struct DefaultRule : rule<create<ast::Default> + kw<"default"> + lit<":">> {};

  struct ForeachRule
      : rule<create<ast::Foreach>(
            kw<"foreach"> + lit<"("> +
            assign<&ast::Foreach::collection>(ref<ExpressionRule>) + lit<":"> +
            assign<&ast::Foreach::variable>(ref<Expr6>) + lit<")"> +
            assign<&ast::Foreach::body>(ref<StatementRule>))> {};

  struct BreakRule : rule<create<ast::Break> + kw<"break"> + lit<";">> {};
  struct ContinueRule
      : rule<create<ast::Continue> + kw<"continue"> + lit<";">> {};

  // No trailing `;`: the `';'` alternative of `statement` below consumes
  // the source's own terminator (`example1.txt`'s `return 0;`).
  struct ReturnRule
      : rule<create<ast::Return>(
            kw<"return"> + assign<&ast::Return::value>(ref<ExpressionRule>))> {
  };

  struct StatementRule
      : rule<create<ast::ExprStatement>(
                 assign<&ast::ExprStatement::expr>(ref<ExpressionRule>) +
                 lit<";">)       //
             | ref<IfRule>       //
             | ref<WhileRule>    //
             | ref<DoWhileRule>  //
             | ref<ForRule>      //
             | ref<SwitchRule>   //
             | ref<CaseRule>     //
             | ref<DefaultRule>  //
             | ref<Block>        //
             | ref<ReturnRule>   //
             | ref<ForeachRule>  //
             | ref<BreakRule>    //
             | ref<ContinueRule> //
             | (create<ast::EmptyStatement> + lit<";">)> {};

  struct ImportRule
      : rule<option(assign<&ast::Import::modifier>(ModifierWord)) +
             kw<"import"> +
             assign<&ast::Import::path>(ref<ConstantIdentifierRule>) +
             lit<";">> {};

  struct InheritanceRule
      : rule<option(assign<&ast::Inheritance::modifier>(ModifierWord)) +
             kw<"inherit"> +
             assign<&ast::Inheritance::program>(ref<ConstantIdentifierRule>) +
             option(lit<":"> + assign<&ast::Inheritance::alias>(ref<ID>)) +
             lit<";">> {};

  struct FunctionDeclarationRule
      : rule<option(assign<&ast::FunctionDeclaration::modifier>(ModifierWord)) +
             assign<&ast::FunctionDeclaration::returnType>(ref<TypeRule>) +
             assign<&ast::FunctionDeclaration::name>(ref<ID>) + lit<"("> +
             option(append<&ast::FunctionDeclaration::args>(ref<Argument>) +
                    many(lit<","> + append<&ast::FunctionDeclaration::args>(
                                        ref<Argument>)) +
                    option(lit<",">)) +
             lit<")"> + lit<";">> {};

  struct FunctionDefinitionRule
      : rule<option(assign<&ast::FunctionDefinition::modifier>(ModifierWord)) +
             assign<&ast::FunctionDefinition::returnType>(ref<TypeRule>) +
             assign<&ast::FunctionDefinition::name>(ref<ID>) + lit<"("> +
             option(append<&ast::FunctionDefinition::args>(ref<Argument>) +
                    many(lit<","> + append<&ast::FunctionDefinition::args>(
                                        ref<Argument>)) +
                    option(lit<",">)) +
             lit<")"> + assign<&ast::FunctionDefinition::body>(ref<Block>)> {};

  struct VariableDeclRule
      : rule<create<ast::VariableDecl>(
            option(flag<&ast::VariableDecl::pointer>(some(lit<"*">))) +
            assign<&ast::VariableDecl::name>(ref<ID>) +
            option(lit<"="> + assign<&ast::VariableDecl::init>(ref<Expr2>)))> {
  };

  struct VariablesRule
      : rule<option(assign<&ast::Variables::modifier>(ModifierWord)) +
             assign<&ast::Variables::type>(ref<TypeRule>) +
             append<&ast::Variables::names>(ref<VariableDeclRule>) +
             many(lit<","> +
                  append<&ast::Variables::names>(ref<VariableDeclRule>)) +
             lit<";">> {};

  struct ConstantDeclRule
      : rule<create<ast::ConstantDecl>(
            assign<&ast::ConstantDecl::name>(ref<ID>) + lit<"="> +
            assign<&ast::ConstantDecl::value>(ref<Expr2>))> {};

  struct ConstantRule
      : rule<option(assign<&ast::Constant::modifier>(ModifierWord)) +
             kw<"constant"> +
             append<&ast::Constant::names>(ref<ConstantDeclRule>) +
             many(lit<","> +
                  append<&ast::Constant::names>(ref<ConstantDeclRule>)) +
             lit<";">> {};

  // No name captured: a bare `class` or `class;`, nothing more; see ast.hpp.
  struct ClassDefRule
      : rule<create<ast::ClassDef>(
            option(assign<&ast::ClassDef::modifier>(ModifierWord)) +
            kw<"class"> + option(lit<";">))> {};

  struct DefinitionRule : rule<ref<ImportRule>                //
                               | ref<InheritanceRule>         //
                               | ref<FunctionDeclarationRule> //
                               | ref<FunctionDefinitionRule>  //
                               | ref<VariablesRule>           //
                               | ref<ConstantRule>            //
                               | ref<ClassDefRule>> {};

  struct ProgramRule
      : rule<create<ast::Program> +
             many(append<&ast::Program::definitions>(ref<DefinitionRule>))> {};

  using entry = ProgramRule;
  struct LINE_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct BLOCK_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  using ignore = WS;
  using hidden = list<LINE_COMMENT, BLOCK_COMMENT>;
};

} // namespace pike
