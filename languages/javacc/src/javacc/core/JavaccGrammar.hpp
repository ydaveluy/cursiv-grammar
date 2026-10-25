// The javacc language: its grammar. A .jj file is options, a
// PARSER_BEGIN/PARSER_END compilation unit, then productions whose bodies
// embed a real declaration/statement/expression grammar for the generated
// parser's target language.
#pragma once

#include <cstdint>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "javacc/core/ast.hpp"

namespace javacc {

inline constexpr auto stripEnds1 = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unwrapped = cursiv::text_conv<stripEnds1>;

inline constexpr auto parseIntLiteral = [](std::string_view t) -> std::int64_t {
  if (!t.empty() && (t.back() == 'l' || t.back() == 'L')) t.remove_suffix(1);
  int base = 10;
  if (t.size() > 1 && t[0] == '0' && (t[1] == 'x' || t[1] == 'X')) {
    base = 16;
    t.remove_prefix(2);
  } else if (t.size() > 1 && t[0] == '0' && (t[1] == 'b' || t[1] == 'B')) {
    base = 2;
    t.remove_prefix(2);
  } else if (t.size() > 1 && t[0] == '0') {
    base = 8;
    t.remove_prefix(1);
  }
  std::int64_t v = 0;
  for (const char c : t) {
    if (c == '_') continue;
    int d = -1;
    if (c >= '0' && c <= '9') d = c - '0';
    else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
    else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
    if (d < 0 || d >= base) continue;
    v = v * base + d;
  }
  return v;
};
using IntLiteralValue = cursiv::text_conv<parseIntLiteral>;

inline constexpr auto countBrackets = [](std::string_view t) -> std::uint8_t {
  std::uint8_t n = 0;
  for (const char c : t)
    if (c == '[') ++n;
  return n;
};
using BracketCount = cursiv::text_conv<countBrackets>;

struct Javacc : cursiv::GrammarSpec {
  // A "java letter": ASCII letters, `$`, `_`, and every scalar past ASCII.
  static constexpr auto JavaLetter = cr<"a-zA-Z$_"> | cr<R"(-􏿿)">;
  static constexpr auto JavaLetterOrDigit = cr<"a-zA-Z0-9$_"> | cr<R"(-􏿿)">;
  static constexpr auto word_boundary = JavaLetterOrDigit;

  struct WS : terminal<some(cr<R"( \t\n\r\f)">)> {};
  struct EGEN : terminal<lit<"/*@egen*/">> {};
  struct COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  struct LINE_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};

  struct ID : terminal<JavaLetter + many(JavaLetterOrDigit)> {};

  static constexpr auto Digits = d + option(many(d | lit<"_">) + d);
  struct INTEGER_LITERAL
      : terminal<((cr<"1-9"> + option(Digits)) |                          // decimal
                  (lit<"0"> + cr<"xX"> + cr<"0-9a-fA-F"> +
                   option(many(cr<"0-9a-fA-F"> | lit<"_">) + cr<"0-9a-fA-F">)) |  // hex
                  (lit<"0"> + cr<"bB"> + cr<"01"> +
                   option(many(cr<"01"> | lit<"_">) + cr<"01">)) |         // binary
                  (lit<"0"> + option(many(lit<"_"> | cr<"0-7">) + cr<"0-7">))) +  // octal
                 option(cr<"lL">)> {};

  static constexpr auto DecExponent = cr<"eE"> + option(cr<"+-">) + d + option(many(d | lit<"_">) + d);
  static constexpr auto HexExponent = cr<"pP"> + option(cr<"+-">) + d + option(many(d | lit<"_">) + d);
  struct FLOATING_POINT_LITERAL
      : terminal<(Digits + lit<"."> + option(Digits) + option(DecExponent) + option(cr<"fFdD">)) |
                 (lit<"."> + Digits + option(DecExponent) + option(cr<"fFdD">)) |
                 (Digits + DecExponent + option(cr<"fFdD">)) |
                 (Digits + option(DecExponent) + cr<"fFdD">) |
                 (lit<"0"> + cr<"xX"> + cr<"0-9a-fA-F"> +
                  option(many(cr<"0-9a-fA-F"> | lit<"_">) + cr<"0-9a-fA-F">) +
                  option(lit<".">) + HexExponent + option(cr<"fFdD">)) |
                 (lit<"0"> + cr<"xX"> +
                  option(cr<"0-9a-fA-F"> + option(many(cr<"0-9a-fA-F"> | lit<"_">) + cr<"0-9a-fA-F">)) +
                  lit<"."> + cr<"0-9a-fA-F"> +
                  option(many(cr<"0-9a-fA-F"> | lit<"_">) + cr<"0-9a-fA-F">) + HexExponent +
                  option(cr<"fFdD">))> {};

  struct BOOL_LIT : terminal<kw<"true"> | kw<"false">, bool> {};

  static constexpr auto CharEscape =
      lit<"\\"> + (cr<R"(ntbrf\\'")">                              //
                   | (cr<"0-3"> + option(cr<"0-7"> + option(cr<"0-7">)))  //
                   | (cr<"4-7"> + option(cr<"0-7">)));
  struct CHARACTER_LITERAL
      : terminal<lit<"'"> + (CharEscape | (!cr<R"('\\\n\r)"> + any)) + lit<"'">> {};

  static constexpr auto StringEscape =
      lit<"\\"> + (cr<R"(ntbrf\\'")">
                   | (lit<"u"> + cr<"0-9a-fA-F"> + option(cr<"0-9a-fA-F"> + option(cr<"0-9a-fA-F"> + option(cr<"0-9a-fA-F">))))
                   | (cr<"0-3"> + cr<"0-7"> + cr<"0-7">)
                   | cr<"0-7">);
  struct STRING_LITERAL
      : terminal<lit<"\""> + many(StringEscape | (!cr<R"("\\\n\r)"> + any)) + lit<"\"">> {};

  // The node descriptor's optional `(...)` customization: opaque, balanced
  // text (real corpus uses a bare digit count, e.g. `#Add(2)`).
  struct NODE_DESC_ARGS
      : terminal<lit<"("> +
                 many(ref<NODE_DESC_ARGS> | (!lit<"("> + !lit<")"> + any)) +
                 lit<")">, Unwrapped> {};

  static constexpr auto JavaSoftKw =
      kw<"LOOKAHEAD"> | kw<"IGNORE_CASE"> | kw<"PARSER_BEGIN"> | kw<"PARSER_END"> |
      kw<"JAVACODE"> | kw<"CPPCODE"> | kw<"TOKEN"> | kw<"SPECIAL_TOKEN"> | kw<"MORE"> |
      kw<"SKIP"> | kw<"TOKEN_MGR_DECLS"> | kw<"EOF"> | kw<"template"> |
      kw<"DCL_PARSER_BEGIN"> | kw<"DCL_PARSER_END"> | kw<"INC_PARSER_BEGIN"> |
      kw<"INC_PARSER_END"> | kw<"DEF_PARSER_BEGIN"> | kw<"DEF_PARSER_END">;
  // A javaIdentifier: any of the JavaCC-reserved spellings above is also an
  // ordinary Java identifier wherever the state reads a name, spelled once.
  struct JavaIdentifierRule : rule<ref<ID> | JavaSoftKw> {};
  struct NameRule
      : rule<ref<JavaIdentifierRule> + many(lit<"."> + ref<JavaIdentifierRule>)> {};

  struct IntLitValue : rule<ref<INTEGER_LITERAL>, IntLiteralValue> {};
  struct ExtraDims : rule<some(lit<"["> + lit<"]">), BracketCount> {};

  static constexpr auto OpMul = lit<"*"> | lit<"/"> | lit<"%">;
  static constexpr auto OpAdd = lit<"+", !cr<"+=">> | lit<"-", !cr<"-=">>;
  static constexpr auto OpEq = lit<"=="> | lit<"!=">;
  static constexpr auto AssignOp =
      lit<"="> | lit<"*="> | lit<"/="> | lit<"%="> | lit<"+="> | lit<"-="> |
      lit<"<<="> | lit<">>="> | lit<">>>="> | lit<"&="> | lit<"^="> | lit<"|=">;

  static constexpr auto PrimitiveTypeKw =
      kw<"boolean"> | kw<"char"> | kw<"byte"> | kw<"short"> | kw<"int"> | kw<"long"> |
      kw<"float"> | kw<"double">;

  static constexpr auto ModifierKw =
      kw<"public"> | kw<"static"> | kw<"protected"> | kw<"private"> | kw<"final"> |
      kw<"abstract"> | kw<"synchronized"> | kw<"native"> | kw<"transient"> |
      kw<"volatile"> | kw<"strictfp">;

  // -- forward declarations --------------------------------------------

  struct ExpressionRule;
  struct ConditionalExpressionRule;
  struct LogicalExpressionRule;
  struct RelationalExpressionRule;
  struct UnaryExpressionRule;
  struct PrimaryExpressionRule;
  struct AllocationExpressionRule;
  struct BlockRule;
  struct TypeRule;
  struct TemplateRule;
  struct ClassTypePartRule;
  struct TypeArgumentRule;
  struct ResultTypeRule;
  struct ModifiersRule;
  struct AnnotationRule;
  struct AnnotationArrayInitRule;
  struct StatementRule;
  struct FormalParametersRule;
  struct TypeParameterRule;
  struct ClassBodyRule;
  struct ClassOrInterfaceDeclarationRule;
  struct ClassOrInterfaceBodyDeclarationRule;
  struct EnumDeclRule;
  struct AnnotationTypeDeclRule;
  struct RegularExpressionRule;
  struct RegexUnitRule;
  struct ComplexRegexAltRule;
  struct ExpansionChoicesRule;

  // -- types ------------------------------------------------------------

  struct PrimitiveTypeRule
      : rule<create<ast::PrimitiveType>(assign<&ast::PrimitiveType::keyword>(PrimitiveTypeKw))> {
  };

  struct TemplatePackRule : rule<kw<"class"> | kw<"typename">> {};

  struct TemplateBaseRule
      : rule<create<ast::TemplateBase>(
                 assign<&ast::TemplateBase::pack>(TemplatePackRule::body) +
                 option(flag<&ast::TemplateBase::variadic>(lit<"...">)) +
                 assign<&ast::TemplateBase::name>(ref<ID>))> {};

  struct TemplateRule
      : rule<create<ast::Template>(
                 kw<"template"> + lit<"<"> +
                 some(append<&ast::Template::bases>(ref<TemplateBaseRule>), lit<",">) +
                 lit<">">)> {};

  struct ClassTypePartRule
      : rule<create<ast::ClassTypePart>(
                 assign<&ast::ClassTypePart::name>(ref<ID>) +
                 option(lit<"<"> +
                        option(some(append<&ast::ClassTypePart::typeArgs>(
                                        ref<TypeArgumentRule>),
                                    lit<",">)) +
                        lit<">">))> {};

  static constexpr auto ClassOrInterfaceTypeBody =
      option(flag<&ast::ClassType::globalScope>(lit<"::">)) +
      append<&ast::ClassType::parts>(ref<ClassTypePartRule>) +
      many((lit<"."> | lit<"::">) + append<&ast::ClassType::parts>(ref<ClassTypePartRule>));

  struct ClassOrInterfaceTypeRule
      : rule<create<ast::ClassType>(ClassOrInterfaceTypeBody)> {};

  struct ReferenceTypeRule
      : rule<create<ast::ArrayType>(
                 assign<&ast::ArrayType::element>(ref<PrimitiveTypeRule>) +
                 assign<&ast::ArrayType::dims>(ref<ExtraDims>))  //
             | (create<ast::ClassType>(
                    option(assign<&ast::ClassType::tmpl>(ref<TemplateRule>)) +
                    ClassOrInterfaceTypeBody) +
                option(nest<ast::ArrayType, &ast::ArrayType::element>(
                    assign<&ast::ArrayType::dims>(ref<ExtraDims>))))> {};

  struct TypeRule : rule<ref<ReferenceTypeRule> | ref<PrimitiveTypeRule>> {};

  // Spliced directly into TypeArgumentRule's own create<>: a helper that
  // writes into the caller's node cannot be its own rule<> (a second head).
  static constexpr auto WildcardBoundsBody =
      assign<&ast::WildcardTypeArgument::boundKind>(kw<"extends"> | kw<"super">) +
      assign<&ast::WildcardTypeArgument::bound>(ref<ReferenceTypeRule>);

  struct TypeArgumentRule
      : rule<create<ast::ConcreteTypeArgument>(
                 assign<&ast::ConcreteTypeArgument::type>(ref<ReferenceTypeRule>) +
                 option(flag<&ast::ConcreteTypeArgument::variadic>(lit<"...">)))  //
             | create<ast::WildcardTypeArgument>(lit<"?"> + option(WildcardBoundsBody))> {
  };

  static constexpr auto TypeBoundBody =
      kw<"extends"> + some(append<&ast::TypeParameter::bounds>(ref<ClassOrInterfaceTypeRule>),
                           lit<"&">);

  struct TypeParameterRule
      : rule<create<ast::TypeParameter>(assign<&ast::TypeParameter::name>(ref<JavaIdentifierRule>) +
                                        option(TypeBoundBody))> {};

  struct ResultTypeRule
      : rule<create<ast::ResultType>(
                 (flag<&ast::ResultType::isVoid>(kw<"void">) +
                  option(flag<&ast::ResultType::pointer>(lit<"*">)))  //
                 | (option(flag<&ast::ResultType::isConst>(kw<"const">)) +
                    assign<&ast::ResultType::type>(ref<TypeRule>) +
                    option(flag<&ast::ResultType::pointer>(lit<"*">) |
                           flag<&ast::ResultType::reference>(lit<"&">))))> {
  };

  // -- names, parameters --------------------------------------------------

  struct VariableDeclaratorIdRule
      : rule<create<ast::VariableDeclaratorId>(
                 assign<&ast::VariableDeclaratorId::name>(ref<JavaIdentifierRule>) +
                 option(assign<&ast::VariableDeclaratorId::extraDims>(ref<ExtraDims>)))> {
  };

  struct FormalParameterRule
      : rule<create<ast::FormalParameter>(
                 assign<&ast::FormalParameter::mods>(ref<ModifiersRule>) +
                 assign<&ast::FormalParameter::type>(ref<TypeRule>) +
                 option(assign<&ast::FormalParameter::marker>(lit<"&"> | lit<"*"> | lit<"...">)) +
                 assign<&ast::FormalParameter::id>(ref<VariableDeclaratorIdRule>))> {};

  struct FormalParametersRule
      : rule<create<ast::FormalParameters>(
                 lit<"("> +
                 option(some(append<&ast::FormalParameters::params>(ref<FormalParameterRule>),
                             lit<",">)) +
                 lit<")">)> {};

  // -- annotations, modifiers ----------------------------------------------

  struct AnnotationArrayInitRule
      : rule<create<ast::AnnotationArrayInit>(
                 lit<"{"> +
                 option(some(append<&ast::AnnotationArrayInit::items>(
                                 ref<AnnotationRule> | ref<AnnotationArrayInitRule> |
                                 ref<ConditionalExpressionRule>),
                             lit<",">)) +
                 option(lit<",">) + lit<"}">)> {};

  struct MemberValuePairRule
      : rule<create<ast::MemberValuePair>(
                 assign<&ast::MemberValuePair::name>(ref<JavaIdentifierRule>) + lit<"="> +
                 assign<&ast::MemberValuePair::value>(
                     ref<AnnotationRule> | ref<AnnotationArrayInitRule> |
                     ref<ConditionalExpressionRule>))> {};

  struct AnnotationRule
      : rule<create<ast::NormalAnnotation>(
                 lit<"@"> + assign<&ast::NormalAnnotation::name>(ref<NameRule>) + lit<"("> +
                 option(some(append<&ast::NormalAnnotation::pairs>(ref<MemberValuePairRule>),
                             lit<",">)) +
                 lit<")">)  //
             | create<ast::SingleMemberAnnotation>(
                   lit<"@"> + assign<&ast::SingleMemberAnnotation::name>(ref<NameRule>) + lit<"("> +
                   assign<&ast::SingleMemberAnnotation::value>(
                       ref<AnnotationRule> | ref<AnnotationArrayInitRule> |
                       ref<ConditionalExpressionRule>) +
                   lit<")">)  //
             | create<ast::MarkerAnnotation>(lit<"@"> +
                                             assign<&ast::MarkerAnnotation::name>(ref<NameRule>))> {};

  struct ModifiersRule
      : rule<create<ast::Modifiers>(
                 many(append<&ast::Modifiers::keywords>(ModifierKw) |
                      append<&ast::Modifiers::annotations>(ref<AnnotationRule>)))> {};

  // -- expressions ----------------------------------------------------

  struct ArrayInitRule
      : rule<create<ast::ArrayInit>(
                 lit<"{"> +
                 option(some(append<&ast::ArrayInit::items>(ref<ArrayInitRule> |
                                                             ref<ExpressionRule>),
                             lit<",">)) +
                 option(lit<",">) + lit<"}">)> {};

  static constexpr auto ArrayDimsAndInitsBody =
      (some(lit<"["> + append<&ast::NewArrayExpr::dims>(ref<ExpressionRule>) + lit<"]">) +
       option(assign<&ast::NewArrayExpr::extraDims>(ref<ExtraDims>)))  //
      | (assign<&ast::NewArrayExpr::extraDims>(ref<ExtraDims>) +
         assign<&ast::NewArrayExpr::init>(ref<ArrayInitRule>));

  struct AllocationExpressionRule
      : rule<create<ast::NewArrayExpr>(
                 kw<"new"> + assign<&ast::NewArrayExpr::elementType>(ref<PrimitiveTypeRule>) +
                 ArrayDimsAndInitsBody)  //
             | create<ast::NewObjectExpr>(
                   kw<"new"> +
                   assign<&ast::NewObjectExpr::type>(ref<ClassOrInterfaceTypeRule>) +
                   option(lit<"<"> +
                          option(some(append<&ast::NewObjectExpr::typeArgs>(
                                          ref<TypeArgumentRule>),
                                      lit<",">)) +
                          lit<">">) +
                   lit<"("> +
                   option(some(append<&ast::NewObjectExpr::args>(ref<ExpressionRule>),
                               lit<",">)) +
                   lit<")"> + option(assign<&ast::NewObjectExpr::body>(ref<ClassBodyRule>)))  //
             | create<ast::NewArrayExpr>(
                   kw<"new"> +
                   assign<&ast::NewArrayExpr::elementType>(ref<ClassOrInterfaceTypeRule>) +
                   ArrayDimsAndInitsBody)> {};

  struct PrimaryPrefixRule
      : rule<create<ast::IntLit>(assign<&ast::IntLit::text>(ref<INTEGER_LITERAL>))  //
             | create<ast::FloatLit>(assign<&ast::FloatLit::text>(ref<FLOATING_POINT_LITERAL>))  //
             | create<ast::CharLit>(assign<&ast::CharLit::text>(ref<CHARACTER_LITERAL>))  //
             | create<ast::StringLit>(assign<&ast::StringLit::text>(ref<STRING_LITERAL>))  //
             | create<ast::BoolLit>(assign<&ast::BoolLit::value>(ref<BOOL_LIT>))  //
             | create<ast::NullLit>(kw<"null">)  //
             | create<ast::ThisExpr>(kw<"this">)  //
             | create<ast::SuperFieldExpr>(kw<"super"> + lit<"."> +
                                           assign<&ast::SuperFieldExpr::field>(ref<JavaIdentifierRule>))  //
             | create<ast::ParenExpr>(lit<"("> + assign<&ast::ParenExpr::inner>(ref<ExpressionRule>) +
                                      lit<")">)  //
             | ref<AllocationExpressionRule>  //
             | create<ast::ClassLiteralExpr>(assign<&ast::ClassLiteralExpr::type>(
                                                  ref<ResultTypeRule>) +
                                             lit<"."> + kw<"class">)  //
             | create<ast::NameExpr>(assign<&ast::NameExpr::name>(ref<NameRule>))> {};

  struct PrimaryExpressionRule
      : rule<ref<PrimaryPrefixRule> +
             many(nest<ast::FieldAccessExpr, &ast::FieldAccessExpr::object>(
                      lit<"."> +
                      assign<&ast::FieldAccessExpr::field>(ref<JavaIdentifierRule>))  //
                  | nest<ast::ThisAccessExpr, &ast::ThisAccessExpr::object>(lit<"."> +
                                                                            kw<"this">)  //
                  | nest<ast::AllocAccessExpr, &ast::AllocAccessExpr::object>(
                        lit<"."> + assign<&ast::AllocAccessExpr::alloc>(
                                       ref<AllocationExpressionRule>))  //
                  | nest<ast::IndexExpr, &ast::IndexExpr::array>(
                        lit<"["> + assign<&ast::IndexExpr::index>(ref<ExpressionRule>) +
                        lit<"]">)  //
                  | nest<ast::CallExpr, &ast::CallExpr::callee>(
                        lit<"("> +
                        option(some(append<&ast::CallExpr::args>(ref<ExpressionRule>),
                                    lit<",">)) +
                        lit<")">))> {};

  struct PostfixExpressionRule
      : rule<ref<PrimaryExpressionRule> +
             option(nest<ast::PostfixExpr, &ast::PostfixExpr::operand>(
                 assign<&ast::PostfixExpr::op>(lit<"++"> | lit<"--">)))> {};

  struct CastExpressionRule
      : rule<create<ast::CastExpr>(lit<"("> + assign<&ast::CastExpr::type>(ref<TypeRule>) +
                                   lit<")"> +
                                   assign<&ast::CastExpr::operand>(ref<UnaryExpressionRule>))> {
  };

  struct UnaryExpressionNotPlusMinusRule
      : rule<create<ast::PrefixExpr>(assign<&ast::PrefixExpr::op>(lit<"~"> | lit<"!">) +
                                     assign<&ast::PrefixExpr::operand>(ref<UnaryExpressionRule>))  //
             | ref<CastExpressionRule>  //
             | ref<PostfixExpressionRule>> {};

  struct UnaryExpressionRule
      : rule<create<ast::PrefixExpr>(assign<&ast::PrefixExpr::op>(lit<"+"> | lit<"-">) +
                                     assign<&ast::PrefixExpr::operand>(ref<UnaryExpressionRule>))  //
             | create<ast::PrefixExpr>(assign<&ast::PrefixExpr::op>(lit<"++">) +
                                       assign<&ast::PrefixExpr::operand>(
                                           ref<PrimaryExpressionRule>))  //
             | create<ast::PrefixExpr>(assign<&ast::PrefixExpr::op>(lit<"--">) +
                                       assign<&ast::PrefixExpr::operand>(
                                           ref<PrimaryExpressionRule>))  //
             | ref<UnaryExpressionNotPlusMinusRule>> {};

  struct RSignedShift
      : rule<lit<">"> + lit<">">, text_conv<[] { return std::string_view(">>"); }>> {};
  struct RUnsignedShift
      : rule<lit<">"> + lit<">"> + lit<">">,
             text_conv<[] { return std::string_view(">>>"); }>> {};

  // GT opens both a relational `>` and the RSignedShift/RUnsignedShift
  // sequences below: one infix<> table cannot give it two precedence steps,
  // so the relational level is a hand-written wrapper instead of one shared
  // ladder.
  struct ShiftExpressionRule
      : infix<UnaryExpressionRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(OpMul) >>                                               //
               left(OpAdd) >>                                               //
               left(lit<"<<"> | ref<RUnsignedShift> | ref<RSignedShift>))> {
  };

  struct RelationalExpressionRule
      : rule<ref<ShiftExpressionRule> +
             many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 assign<&ast::BinaryExpr::op>(lit<"<="> | lit<">="> | lit<"<"> |
                                              lit<">">) +
                 assign<&ast::BinaryExpr::right>(ref<ShiftExpressionRule>)))> {};

  struct InstanceOfExpressionRule
      : rule<create<ast::InstanceOfExpr>(
                 assign<&ast::InstanceOfExpr::operand>(ref<RelationalExpressionRule>) +
                 kw<"instanceof"> + assign<&ast::InstanceOfExpr::type>(ref<TypeRule>))  //
             | ref<RelationalExpressionRule>> {};

  struct LogicalExpressionRule
      : infix<InstanceOfExpressionRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(OpEq) >>            //
               left(lit<"&", !lit<"&">> ) >>  //
               left(lit<"^">) >>        //
               left(lit<"|", !lit<"|">> ) >>  //
               left(lit<"&&">) >>       //
               left(lit<"||">))> {};

  struct ConditionalExpressionRule
      : rule<create<ast::CondExpr>(
                 assign<&ast::CondExpr::cond>(ref<LogicalExpressionRule>) + lit<"?"> +
                 assign<&ast::CondExpr::thenValue>(ref<ExpressionRule>) + lit<":"> +
                 assign<&ast::CondExpr::elseValue>(ref<ExpressionRule>))  //
             | ref<LogicalExpressionRule>> {};

  struct ExpressionRule
      : rule<create<ast::AssignExpr>(
                 assign<&ast::AssignExpr::target>(ref<ConditionalExpressionRule>) +
                 assign<&ast::AssignExpr::op>(AssignOp) +
                 assign<&ast::AssignExpr::value>(ref<ExpressionRule>))  //
             | ref<ConditionalExpressionRule>> {};

  // -- statements, declarations ----------------------------------------

  struct VariableDeclaratorRule
      : rule<create<ast::VariableDeclarator>(
                 assign<&ast::VariableDeclarator::id>(ref<VariableDeclaratorIdRule>) +
                 option(lit<"="> + assign<&ast::VariableDeclarator::init>(
                                       ref<ArrayInitRule> | ref<ExpressionRule>)))> {};

  struct LocalVarDeclRule
      : rule<create<ast::VarDecl>(
                 assign<&ast::VarDecl::mods>(ref<ModifiersRule>) +
                 assign<&ast::VarDecl::type>(ref<TypeRule>) +
                 some(append<&ast::VarDecl::decls>(ref<VariableDeclaratorRule>), lit<",">))> {
  };

  struct ExprStmtRule
      : rule<create<ast::ExprStmt>(option(assign<&ast::ExprStmt::value>(ref<ExpressionRule>)) +
                                   lit<";">)> {};

  struct AssertStmtRule
      : rule<create<ast::AssertStmt>(
                 kw<"assert"> + assign<&ast::AssertStmt::cond>(ref<ExpressionRule>) +
                 option(lit<":"> + assign<&ast::AssertStmt::message>(ref<ExpressionRule>)) +
                 lit<";">)> {};

  struct IfStmtRule
      : rule<create<ast::IfStmt>(
                 kw<"if"> + lit<"("> + assign<&ast::IfStmt::cond>(ref<ExpressionRule>) +
                 lit<")"> + assign<&ast::IfStmt::thenBranch>(ref<StatementRule>) +
                 option(kw<"else"> +
                        assign<&ast::IfStmt::elseBranch>(ref<StatementRule>)))> {};

  struct WhileStmtRule
      : rule<create<ast::WhileStmt>(
                 kw<"while"> + lit<"("> + assign<&ast::WhileStmt::cond>(ref<ExpressionRule>) +
                 lit<")"> + assign<&ast::WhileStmt::body>(ref<StatementRule>))> {};

  struct DoStmtRule
      : rule<create<ast::DoStmt>(
                 kw<"do"> + assign<&ast::DoStmt::body>(ref<StatementRule>) + kw<"while"> +
                 lit<"("> + assign<&ast::DoStmt::cond>(ref<ExpressionRule>) + lit<")"> +
                 lit<";">)> {};

  struct ExprListRule
      : rule<create<ast::ExprList>(
                 some(append<&ast::ExprList::exprs>(ref<ExpressionRule>), lit<",">))> {};

  struct ForStmtRule
      : rule<create<ast::ForEachStmt>(
                 kw<"for"> + lit<"("> + assign<&ast::ForEachStmt::mods>(ref<ModifiersRule>) +
                 assign<&ast::ForEachStmt::type>(ref<TypeRule>) +
                 assign<&ast::ForEachStmt::name>(ref<JavaIdentifierRule>) + lit<":"> +
                 assign<&ast::ForEachStmt::iterable>(ref<ExpressionRule>) + lit<")"> +
                 assign<&ast::ForEachStmt::body>(ref<StatementRule>))  //
             | create<ast::ForStmt>(
                   kw<"for"> + lit<"("> +
                   option(assign<&ast::ForStmt::init>(ref<LocalVarDeclRule> |
                                                       ref<ExprListRule>)) +
                   lit<";">                                                   //
                   + option(assign<&ast::ForStmt::cond>(ref<ExpressionRule>)) +
                   lit<";">                                                   //
                   + option(assign<&ast::ForStmt::update>(ref<ExprListRule>)) +
                   lit<")"> + assign<&ast::ForStmt::body>(ref<StatementRule>))> {};

  struct BreakStmtRule
      : rule<create<ast::BreakStmt>(kw<"break"> +
                                    option(assign<&ast::BreakStmt::label>(ref<JavaIdentifierRule>)) +
                                    lit<";">)> {};
  struct ContinueStmtRule
      : rule<create<ast::ContinueStmt>(
                 kw<"continue"> + option(assign<&ast::ContinueStmt::label>(ref<JavaIdentifierRule>)) +
                 lit<";">)> {};
  struct ReturnStmtRule
      : rule<create<ast::ReturnStmt>(
                 kw<"return"> + option(assign<&ast::ReturnStmt::value>(ref<ExpressionRule>)) +
                 lit<";">)> {};
  struct ThrowStmtRule
      : rule<create<ast::ThrowStmt>(kw<"throw"> +
                                    assign<&ast::ThrowStmt::value>(ref<ExpressionRule>) +
                                    lit<";">)> {};

  struct SwitchLabelRule
      : rule<create<ast::SwitchLabelStmt>(
                 ((kw<"case"> + assign<&ast::SwitchLabelStmt::value>(ref<ExpressionRule>))  //
                 | flag<&ast::SwitchLabelStmt::isDefault>(kw<"default">)) + lit<":">)> {};

  struct BlockItemRule;

  struct SwitchStmtRule
      : rule<create<ast::SwitchStmt>(
                 kw<"switch"> + lit<"("> + assign<&ast::SwitchStmt::value>(ref<ExpressionRule>) +
                 lit<")"> + lit<"{"> +
                 many(append<&ast::SwitchStmt::items>(ref<SwitchLabelRule>) +
                      many(append<&ast::SwitchStmt::items>(ref<BlockItemRule>))) +
                 lit<"}">)> {};

  struct SynchronizedStmtRule
      : rule<create<ast::SynchronizedStmt>(
                 kw<"synchronized"> + lit<"("> + assign<&ast::SynchronizedStmt::lock>(
                                                      ref<ExpressionRule>) +
                 lit<")"> + assign<&ast::SynchronizedStmt::body>(ref<BlockRule>))> {};

  struct LabeledStmtRule
      : rule<create<ast::LabeledStmt>(assign<&ast::LabeledStmt::label>(ref<JavaIdentifierRule>) +
                                      lit<":"> +
                                      assign<&ast::LabeledStmt::body>(ref<StatementRule>))> {
  };

  struct ResourceDeclRule
      : rule<create<ast::ResourceDecl>(
                 assign<&ast::ResourceDecl::type>(ref<TypeRule>) +
                 assign<&ast::ResourceDecl::id>(ref<VariableDeclaratorIdRule>) + lit<"="> +
                 assign<&ast::ResourceDecl::init>(ref<ExpressionRule>))> {};

  struct CatchClauseRule
      : rule<create<ast::CatchClause>(
                 kw<"catch"> + lit<"("> + assign<&ast::CatchClause::mods>(ref<ModifiersRule>) +
                 append<&ast::CatchClause::types>(ref<TypeRule>) +
                 option(assign<&ast::CatchClause::marker>(lit<"&"> | lit<"*"> | lit<"...">)) +
                 many(lit<"|"> + append<&ast::CatchClause::types>(ref<TypeRule>)) +
                 assign<&ast::CatchClause::id>(ref<VariableDeclaratorIdRule>) + lit<")"> +
                 assign<&ast::CatchClause::body>(ref<BlockRule>))> {};

  struct TryStmtRule
      : rule<create<ast::TryStmt>(
                 kw<"try"> +
                 option(lit<"("> +
                        some(append<&ast::TryStmt::resources>(ref<ResourceDeclRule>),
                             lit<";">) +
                        option(lit<";">) + lit<")">) +
                 assign<&ast::TryStmt::body>(ref<BlockRule>) +
                 many(append<&ast::TryStmt::catches>(ref<CatchClauseRule>)) +
                 option(kw<"finally"> +
                        assign<&ast::TryStmt::finallyBlock>(ref<BlockRule>)))> {};

  struct StatementRule
      : rule<ref<LabeledStmtRule>       //
             | ref<AssertStmtRule>      //
             | ref<BlockRule>           //
             | ref<SwitchStmtRule>      //
             | ref<IfStmtRule>          //
             | ref<WhileStmtRule>       //
             | ref<DoStmtRule>          //
             | ref<ForStmtRule>         //
             | ref<BreakStmtRule>       //
             | ref<ContinueStmtRule>    //
             | ref<ReturnStmtRule>      //
             | ref<ThrowStmtRule>       //
             | ref<SynchronizedStmtRule>  //
             | ref<TryStmtRule>         //
             | ref<ExprStmtRule>> {};

  struct BlockItemRule
      : rule<(ref<LocalVarDeclRule> + lit<";">)  //
             | ref<StatementRule>                //
             | ref<ClassOrInterfaceDeclarationRule>> {};

  struct BlockRule
      : rule<create<ast::Block>(lit<"{"> +
                                many(append<&ast::Block::items>(ref<BlockItemRule>)) +
                                lit<"}">)> {};

  struct ExplicitCtorInvocationRule
      : rule<create<ast::ExplicitCtorInvocation>(
                 kw<"this"> + lit<"("> +
                 option(some(append<&ast::ExplicitCtorInvocation::args>(ref<ExpressionRule>),
                             lit<",">)) +
                 lit<")"> + lit<";">)  //
             | create<ast::ExplicitCtorInvocation>(
                   option(assign<&ast::ExplicitCtorInvocation::qualifier>(
                              ref<PrimaryExpressionRule>) +
                          lit<".">) +
                   flag<&ast::ExplicitCtorInvocation::isSuper>(kw<"super">) + lit<"("> +
                   option(some(append<&ast::ExplicitCtorInvocation::args>(ref<ExpressionRule>),
                               lit<",">)) +
                   lit<")"> + lit<";">)> {};

  struct ConstructorDeclRule
      : rule<create<ast::ConstructorDecl>(
                 assign<&ast::ConstructorDecl::mods>(ref<ModifiersRule>) +
                 option(lit<"<"> +
                        some(append<&ast::ConstructorDecl::typeParams>(ref<TypeParameterRule>),
                             lit<",">) +
                        lit<">">) +
                 assign<&ast::ConstructorDecl::name>(ref<JavaIdentifierRule>) +
                 assign<&ast::ConstructorDecl::params>(ref<FormalParametersRule>) +
                 option(kw<"throws"> +
                        some(append<&ast::ConstructorDecl::throwsList>(ref<NameRule>), lit<",">)) +
                 lit<"{"> +
                 option(assign<&ast::ConstructorDecl::eci>(ref<ExplicitCtorInvocationRule>)) +
                 many(append<&ast::ConstructorDecl::body>(ref<BlockItemRule>)) + lit<"}">)> {
  };

  struct MethodDeclRule
      : rule<create<ast::MethodDecl>(
                 assign<&ast::MethodDecl::mods>(ref<ModifiersRule>) +
                 option(lit<"<"> +
                        some(append<&ast::MethodDecl::typeParams>(ref<TypeParameterRule>),
                             lit<",">) +
                        lit<">">) +
                 assign<&ast::MethodDecl::resultType>(ref<ResultTypeRule>) +
                 assign<&ast::MethodDecl::name>(ref<JavaIdentifierRule>) +
                 assign<&ast::MethodDecl::params>(ref<FormalParametersRule>) +
                 option(assign<&ast::MethodDecl::extraDims>(ref<ExtraDims>)) +
                 option(kw<"throws"> +
                        some(append<&ast::MethodDecl::throwsList>(ref<NameRule>), lit<",">)) +
                 (assign<&ast::MethodDecl::body>(ref<BlockRule>) | lit<";">))> {};

  struct ClassOrInterfaceDeclarationRule
      : rule<create<ast::ClassOrInterfaceDecl>(
                 assign<&ast::ClassOrInterfaceDecl::mods>(ref<ModifiersRule>) +
                 (flag<&ast::ClassOrInterfaceDecl::isInterface>(kw<"interface">) |
                  kw<"class">) +
                 assign<&ast::ClassOrInterfaceDecl::name>(ref<JavaIdentifierRule>) +
                 option(lit<"<"> +
                        some(append<&ast::ClassOrInterfaceDecl::typeParams>(
                                 ref<TypeParameterRule>),
                             lit<",">) +
                        lit<">">) +
                 option(kw<"extends"> +
                        some(append<&ast::ClassOrInterfaceDecl::extendsList>(
                                 ref<ClassOrInterfaceTypeRule>),
                             lit<",">)) +
                 option(kw<"implements"> +
                        some(append<&ast::ClassOrInterfaceDecl::implementsList>(
                                 ref<ClassOrInterfaceTypeRule>),
                             lit<",">)) +
                 assign<&ast::ClassOrInterfaceDecl::body>(ref<ClassBodyRule>))> {};

  struct EnumConstantRule
      : rule<create<ast::EnumConstant>(
                 assign<&ast::EnumConstant::mods>(ref<ModifiersRule>) +
                 assign<&ast::EnumConstant::name>(ref<JavaIdentifierRule>) +
                 option(lit<"("> +
                        option(some(append<&ast::EnumConstant::args>(ref<ExpressionRule>),
                                    lit<",">)) +
                        lit<")">) +
                 option(assign<&ast::EnumConstant::body>(ref<ClassBodyRule>)))> {};

  struct EnumDeclRule
      : rule<create<ast::EnumDecl>(
                 assign<&ast::EnumDecl::mods>(ref<ModifiersRule>) + kw<"enum"> +
                 assign<&ast::EnumDecl::name>(ref<JavaIdentifierRule>) +
                 option(kw<"implements"> +
                        some(append<&ast::EnumDecl::implementsList>(
                                 ref<ClassOrInterfaceTypeRule>),
                             lit<",">)) +
                 lit<"{"> +
                 option(some(append<&ast::EnumDecl::constants>(ref<EnumConstantRule>),
                             lit<",">)) +
                 option(lit<",">) +
                 option(lit<";"> + many(append<&ast::EnumDecl::bodyDecls>(
                                            ref<ClassOrInterfaceBodyDeclarationRule>))) +
                 lit<"}">)> {};

  struct AnnotationElementRule
      : rule<create<ast::AnnotationElement>(
                 assign<&ast::AnnotationElement::mods>(ref<ModifiersRule>) +
                 assign<&ast::AnnotationElement::type>(ref<TypeRule>) +
                 assign<&ast::AnnotationElement::name>(ref<JavaIdentifierRule>) + lit<"("> + lit<")"> +
                 option(kw<"default"> +
                        assign<&ast::AnnotationElement::defaultVal>(
                            ref<AnnotationRule> | ref<AnnotationArrayInitRule> |
                            ref<ConditionalExpressionRule>)) +
                 lit<";">)> {};

  struct AnnotationTypeMemberDeclarationRule
      : rule<ref<AnnotationElementRule>             //
             | ref<ClassOrInterfaceDeclarationRule>  //
             | ref<EnumDeclRule>                     //
             | ref<AnnotationTypeDeclRule>           //
             | (ref<LocalVarDeclRule> + lit<";">)     //
             | create<ast::EmptyDecl>(lit<";">)> {};

  struct AnnotationTypeDeclRule
      : rule<create<ast::AnnotationTypeDecl>(
                 assign<&ast::AnnotationTypeDecl::mods>(ref<ModifiersRule>) + lit<"@"> +
                 kw<"interface"> + assign<&ast::AnnotationTypeDecl::name>(ref<JavaIdentifierRule>) +
                 lit<"{"> +
                 many(append<&ast::AnnotationTypeDecl::members>(
                          ref<AnnotationTypeMemberDeclarationRule>)) +
                 lit<"}">)> {};

  struct ClassOrInterfaceBodyDeclarationRule
      : rule<create<ast::InitializerBlock>(
                 option(flag<&ast::InitializerBlock::isStatic>(kw<"static">)) +
                 assign<&ast::InitializerBlock::body>(ref<BlockRule>))  //
             | ref<ClassOrInterfaceDeclarationRule>                    //
             | ref<EnumDeclRule>                                       //
             | ref<ConstructorDeclRule>                                //
             | (ref<LocalVarDeclRule> + lit<";">)                       //
             | ref<MethodDeclRule>                                     //
             | create<ast::EmptyDecl>(lit<";">)> {};

  struct ClassBodyRule
      : rule<create<ast::ClassBody>(
                 lit<"{"> +
                 many(append<&ast::ClassBody::members>(ref<ClassOrInterfaceBodyDeclarationRule>)) +
                 lit<"}">)> {};

  struct TypeDeclarationRule
      : rule<ref<ClassOrInterfaceDeclarationRule>  //
             | ref<EnumDeclRule>                    //
             | ref<AnnotationTypeDeclRule>          //
             | create<ast::EmptyDecl>(lit<";">)> {};

  struct PackageDeclRule
      : rule<create<ast::PackageDecl>(
                 assign<&ast::PackageDecl::mods>(ref<ModifiersRule>) + kw<"package"> +
                 assign<&ast::PackageDecl::name>(ref<NameRule>) + lit<";">)> {};

  struct ImportDeclRule
      : rule<create<ast::ImportDecl>(
                 kw<"import"> + option(flag<&ast::ImportDecl::isStatic>(kw<"static">)) +
                 assign<&ast::ImportDecl::name>(ref<NameRule>) +
                 option(lit<"."> + flag<&ast::ImportDecl::onDemand>(lit<"*">)) +
                 lit<";">)> {};

  struct CompilationUnitRule
      : rule<create<ast::CompilationUnit>(
                 option(assign<&ast::CompilationUnit::pkg>(ref<PackageDeclRule>)) +
                 many(append<&ast::CompilationUnit::imports>(ref<ImportDeclRule>)) +
                 many(append<&ast::CompilationUnit::types>(ref<TypeDeclarationRule>)))> {};

  // -- javacc: node descriptors, regular expressions -----------------------

  struct NodeDescriptorRule
      : rule<create<ast::NodeDescriptor>(
                 lit<"#"> + assign<&ast::NodeDescriptor::name>(ref<ID> | kw<"void">) +
                 option(assign<&ast::NodeDescriptor::args>(ref<NODE_DESC_ARGS>)))> {};

  struct CharDescriptorRule
      : rule<create<ast::CharDescriptor>(
                 assign<&ast::CharDescriptor::from>(ref<STRING_LITERAL>) +
                 option(lit<"-"> + assign<&ast::CharDescriptor::to>(ref<STRING_LITERAL>)))> {
  };

  struct ComplexRegexAltRule
      : rule<create<ast::ComplexRegexAlt>(
                 some(append<&ast::ComplexRegexAlt::units>(ref<RegexUnitRule>)))> {};

  struct RegexUnitRule
      : rule<create<ast::RegexStringUnit>(
                 assign<&ast::RegexStringUnit::text>(ref<STRING_LITERAL>))  //
             | create<ast::RegexRefUnit>(lit<"<"> +
                                         assign<&ast::RegexRefUnit::name>(ref<ID>) + lit<">">)  //
             | create<ast::CharListUnit>(
                   option(flag<&ast::CharListUnit::negated>(lit<"~">)) + lit<"["> +
                   option(some(append<&ast::CharListUnit::items>(ref<CharDescriptorRule>),
                               lit<",">)) +
                   lit<"]">)  //
             | create<ast::RegexGroupUnit>(
                   lit<"("> +
                   some(append<&ast::RegexGroupUnit::choices>(ref<ComplexRegexAltRule>),
                        lit<"|">) +
                   lit<")"> +
                   option(assign<&ast::RegexGroupUnit::simpleSuffix>(lit<"+"> | lit<"*"> |
                                                                     lit<"?">)  //
                          | (flag<&ast::RegexGroupUnit::hasCountSuffix>(lit<"{">) +
                             assign<&ast::RegexGroupUnit::repeatMin>(ref<IntLitValue>) +
                             option(flag<&ast::RegexGroupUnit::repeatHasComma>(lit<",">) +
                                    option(assign<&ast::RegexGroupUnit::repeatMax>(
                                        ref<IntLitValue>))) +
                             lit<"}">)))> {};

  struct RegularExpressionRule
      : rule<create<ast::StringLiteralRegex>(
                 assign<&ast::StringLiteralRegex::text>(ref<STRING_LITERAL>))  //
             | create<ast::ComplexRegex>(
                   lit<"<"> +
                   option(option(flag<&ast::ComplexRegex::isPrivate>(lit<"#">)) +
                          assign<&ast::ComplexRegex::label>(ref<ID>) + lit<":">) +
                   some(append<&ast::ComplexRegex::choices>(ref<ComplexRegexAltRule>),
                        lit<"|">) +
                   lit<">">)  //
             | create<ast::TokenRefRegex>(lit<"<"> +
                                          assign<&ast::TokenRefRegex::name>(ref<ID>) +
                                          lit<">">)  //
             | create<ast::EofRegex>(lit<"<"> + kw<"EOF"> + lit<">">)> {};

  struct RegexprSpecRule
      : rule<create<ast::RegexprSpec>(
                 assign<&ast::RegexprSpec::regex>(ref<RegularExpressionRule>) +
                 option(assign<&ast::RegexprSpec::action>(ref<BlockRule>)) +
                 option(lit<":"> +
                        assign<&ast::RegexprSpec::lexicalStateAfter>(ref<ID>)))> {};

  struct RegularExprProductionRule
      : rule<create<ast::RegularExprProduction>(
                 option((lit<"<"> +
                         flag<&ast::RegularExprProduction::allStates>(lit<"*">) +
                         lit<">">)  //
                        | (lit<"<"> +
                           some(append<&ast::RegularExprProduction::states>(ref<ID>),
                                lit<",">) +
                           lit<">">)) +
                 assign<&ast::RegularExprProduction::kind>(kw<"TOKEN"> | kw<"SPECIAL_TOKEN"> |
                                                           kw<"SKIP"> | kw<"MORE">) +
                 option(lit<"["> +
                        flag<&ast::RegularExprProduction::ignoreCase>(kw<"IGNORE_CASE">) +
                        lit<"]">) +
                 lit<":"> + lit<"{"> +
                 some(append<&ast::RegularExprProduction::specs>(ref<RegexprSpecRule>),
                      lit<"|">) +
                 lit<"}">)> {};

  struct TokenManagerDeclsRule
      : rule<create<ast::TokenManagerDecls>(
                 kw<"TOKEN_MGR_DECLS"> + lit<":"> +
                 option(assign<&ast::TokenManagerDecls::body>(ref<ClassBodyRule>)))> {};

  struct JavaCodeProductionRule
      : rule<create<ast::JavaCodeProduction>(
                 kw<"JAVACODE"> +
                 assign<&ast::JavaCodeProduction::resultType>(ref<ResultTypeRule>) +
                 assign<&ast::JavaCodeProduction::name>(ref<ID>) +
                 assign<&ast::JavaCodeProduction::params>(ref<FormalParametersRule>) +
                 option(kw<"throws"> +
                        some(append<&ast::JavaCodeProduction::throwsList>(ref<NameRule>), lit<",">)) +
                 option(assign<&ast::JavaCodeProduction::nodeDesc>(ref<NodeDescriptorRule>)) +
                 assign<&ast::JavaCodeProduction::body>(ref<BlockRule>))> {};

  struct CppCodeProductionRule
      : rule<create<ast::CppCodeProduction>(
                 kw<"CPPCODE"> +
                 option(assign<&ast::CppCodeProduction::accessModifier>(
                     kw<"public"> | kw<"protected"> | kw<"private">)) +
                 assign<&ast::CppCodeProduction::resultType>(ref<ResultTypeRule>) +
                 assign<&ast::CppCodeProduction::name>(ref<ID>) +
                 assign<&ast::CppCodeProduction::params>(ref<FormalParametersRule>) +
                 option(kw<"throws"> +
                        some(append<&ast::CppCodeProduction::throwsList>(ref<NameRule>), lit<",">)) +
                 assign<&ast::CppCodeProduction::body>(ref<BlockRule>))> {};

  struct BnfProductionRule
      : rule<create<ast::BnfProduction>(
                 assign<&ast::BnfProduction::resultType>(ref<ResultTypeRule>) +
                 assign<&ast::BnfProduction::name>(ref<ID>) +
                 assign<&ast::BnfProduction::params>(ref<FormalParametersRule>) +
                 option(kw<"throws"> +
                        some(append<&ast::BnfProduction::throwsList>(ref<NameRule>), lit<",">)) +
                 option(assign<&ast::BnfProduction::nodeDesc>(ref<NodeDescriptorRule>)) +
                 lit<":"> + assign<&ast::BnfProduction::localDecls>(ref<BlockRule>) + lit<"{"> +
                 assign<&ast::BnfProduction::body>(ref<ExpansionChoicesRule>) + lit<"}">)> {
  };

  struct ProductionRule
      : rule<ref<JavaCodeProductionRule>       //
             | ref<CppCodeProductionRule>      //
             | ref<RegularExprProductionRule>  //
             | ref<TokenManagerDeclsRule>      //
             | ref<BnfProductionRule>> {};

  // -- javacc: expansions -----------------------------------------------

  struct LocalLookaheadRule
      : rule<create<ast::LocalLookahead>(
                 option(assign<&ast::LocalLookahead::amount>(ref<IntLitValue>)) +
                 option(lit<",">) +
                 option(assign<&ast::LocalLookahead::choices>(ref<ExpansionChoicesRule>)) +
                 option(lit<",">) +
                 option(lit<"{"> +
                        option(assign<&ast::LocalLookahead::semanticPredicate>(
                            ref<ExpressionRule>)) +
                        lit<"}">))> {};

  struct ExpansionCatchClauseRule
      : rule<create<ast::ExpansionCatchClause>(
                 kw<"catch"> + lit<"("> +
                 option(assign<&ast::ExpansionCatchClause::typeName>(ref<NameRule>) +
                        assign<&ast::ExpansionCatchClause::varName>(ref<ID>)) +
                 lit<")"> + assign<&ast::ExpansionCatchClause::body>(ref<BlockRule>))> {};

  struct ExpansionUnitRule
      : rule<create<ast::LookaheadUnit>(
                 kw<"LOOKAHEAD"> + lit<"("> +
                 assign<&ast::LookaheadUnit::lookahead>(ref<LocalLookaheadRule>) + lit<")">)  //
             | create<ast::ActionUnit>(assign<&ast::ActionUnit::action>(ref<BlockRule>))  //
             | create<ast::OptionalUnit>(lit<"["> +
                                         assign<&ast::OptionalUnit::choices>(
                                             ref<ExpansionChoicesRule>) +
                                         lit<"]">)  //
             | create<ast::TryExpansionUnit>(
                   kw<"try"> + lit<"{"> +
                   assign<&ast::TryExpansionUnit::body>(ref<ExpansionChoicesRule>) + lit<"}"> +
                   many(append<&ast::TryExpansionUnit::catches>(ref<ExpansionCatchClauseRule>)) +
                   option(kw<"finally"> +
                          assign<&ast::TryExpansionUnit::finallyBlock>(ref<BlockRule>)))  //
             | create<ast::ExpansionCallUnit>(
                   option(assign<&ast::ExpansionCallUnit::assignTarget>(
                              ref<PrimaryExpressionRule>) +
                          lit<"=">) +
                   assign<&ast::ExpansionCallUnit::name>(ref<ID>) +
                   option(lit<"<"> +
                          option(some(append<&ast::ExpansionCallUnit::typeArgs>(
                                          ref<TypeArgumentRule>),
                                      lit<",">)) +
                          lit<">">) +
                   lit<"("> +
                   option(some(append<&ast::ExpansionCallUnit::args>(ref<ExpressionRule>),
                               lit<",">)) +
                   lit<")">)  //
             | create<ast::ExpansionRegexUnit>(
                   option(assign<&ast::ExpansionRegexUnit::assignTarget>(
                              ref<PrimaryExpressionRule>) +
                          lit<"=">) +
                   assign<&ast::ExpansionRegexUnit::regex>(ref<RegularExpressionRule>) +
                   option(lit<"."> +
                          assign<&ast::ExpansionRegexUnit::lexicalStateAfter>(ref<ID>)))  //
             | create<ast::GroupUnit>(
                   lit<"("> + assign<&ast::GroupUnit::choices>(ref<ExpansionChoicesRule>) +
                   lit<")"> +
                   option(assign<&ast::GroupUnit::repeat>(lit<"+"> | lit<"*"> | lit<"?">)))> {
      };

  struct ExpansionItemRule
      : rule<create<ast::ExpansionItem>(
                 assign<&ast::ExpansionItem::unit>(ref<ExpansionUnitRule>) +
                 option(assign<&ast::ExpansionItem::nodeDesc>(ref<NodeDescriptorRule>)))> {};

  struct ExpansionRule
      : rule<create<ast::Expansion>(
                 option(kw<"LOOKAHEAD"> + lit<"("> +
                        assign<&ast::Expansion::lookahead>(ref<LocalLookaheadRule>) +
                        lit<")">) +
                 many(append<&ast::Expansion::items>(ref<ExpansionItemRule>)))> {};

  struct ExpansionChoicesRule
      : rule<create<ast::ExpansionChoices>(
                 some(append<&ast::ExpansionChoices::alts>(ref<ExpansionRule>), lit<"|">))> {};

  struct StringListRule
      : rule<create<ast::StringList>(
                 lit<"("> + some(append<&ast::StringList::items>(ref<STRING_LITERAL>), lit<",">) +
                 lit<")">)> {};

  struct OptionBindingRule
      : rule<create<ast::OptionBinding>(
                 assign<&ast::OptionBinding::name>(ref<ID> | kw<"LOOKAHEAD"> |
                                                   kw<"IGNORE_CASE"> | kw<"static"> |
                                                   kw<"PARSER_BEGIN">) +
                 lit<"="> +
                 assign<&ast::OptionBinding::value>(ref<IntLitValue> | ref<BOOL_LIT> |
                                                    ref<STRING_LITERAL> | ref<StringListRule>) +
                 lit<";">)> {};

  struct JavaccInputRule
      : rule<option(ref<ID> + lit<"{"> +
                    many(append<&ast::JavaccInput::options>(ref<OptionBindingRule>)) +
                    lit<"}">) +
             kw<"PARSER_BEGIN"> + lit<"("> +
             assign<&ast::JavaccInput::parserBeginName>(ref<ID>) + lit<")"> +
             assign<&ast::JavaccInput::unit>(ref<CompilationUnitRule>) + kw<"PARSER_END"> +
             lit<"("> + assign<&ast::JavaccInput::parserEndName>(ref<ID>) + lit<")"> +
             some(append<&ast::JavaccInput::productions>(ref<ProductionRule>))> {};

  using entry = JavaccInputRule;
  using ignore = list<WS, EGEN>;
  using hidden = list<COMMENT, LINE_COMMENT>;
};

}  // namespace javacc
