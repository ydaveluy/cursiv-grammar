#pragma once

#include <memory_resource>
#include <string>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "glsl/core/ast.hpp"

namespace glsl {

inline std::string_view unsplice(std::string_view text,
                                 std::pmr::memory_resource *memory) {
  if (text.find('\\') == text.npos)
    return text;
  auto *result =
      static_cast<char *>(memory->allocate(text.size(), alignof(char)));
  std::size_t length = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (text[i] == '\\' && i + 1 < text.size()) {
      if (text[i + 1] == '\n') {
        ++i;
        continue;
      }
      if (text[i + 1] == '\r') {
        ++i;
        if (i + 1 < text.size() && text[i + 1] == '\n')
          ++i;
        continue;
      }
    }
    result[length++] = text[i];
  }
  return {result, length};
}

namespace detail {
template <cursiv::str S, std::size_t I = 0> consteval auto spelling() {
  using namespace cursiv;
  constexpr auto character = [] {
    auto c = cursiv::str{"x"};
    c.data[0] = S[I];
    return c;
  }();
  if constexpr (I + 1 == S.size())
    return lit<character>;
  else
    return lit<character> + many(lit<"\\"> + (lit<"\r\n"> | cr<R"(\r\n)">)) +
           spelling<S, I + 1>();
}
} // namespace detail

struct Glsl : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  static constexpr auto NL = (lit<"\r\n"> | cr<R"(\r\n)">);
  static constexpr auto Splice = lit<"\\"> + NL;
  struct LineCont : terminal<Splice> {};
  // Splicing precedes token recognition in GLSL, including keywords.
  template <cursiv::str S>
  struct Keyword : terminal<as<unsplice>(detail::spelling<S>() +
                                        !(many(Splice) + w))> {};
  struct BlockComment : terminal<through(lit<"/*">, lit<"*/">)> {};
  struct LineComment : terminal<lit<"//"> + many(Splice | cr<R"(^\r\n)">)> {};
  static constexpr auto HexStart = lit<"0"> + many(Splice) + cr<"xX">;
  struct Number : terminal<(HexStart + many(Splice | cr<"a-zA-Z_0-9.">)) |
                           (!HexStart + (d | (lit<"."> + many(Splice) + d)) +
                            many(Splice | (cr<"eE"> + many(Splice) + cr<"+-">) |
                                 cr<"a-zA-Z_0-9.">))> {};
  struct String : terminal<lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\)">) +
                           lit<"\"">> {};
  static constexpr auto DirectiveQuoted =
      lit<"\""> +
      many(Splice | (lit<"\\"> + cr<R"(^\r\n)">) | cr<R"(^"\\\r\n)">) +
      lit<"\"">;
  struct DirectiveText
      : terminal<lit<"#"> +
                 many(
                     Splice | DirectiveQuoted | through(lit<"/*">, lit<"*/">) |
                     (lit<"//"> + many(Splice | cr<R"(^\r\n)">) + &(NL | eof)) |
                     (!(lit<"/*"> | lit<"//"> | lit<"\"">)+cr<R"(^\r\n)">)) +
                 option(NL)> {};

  template <cursiv::str S>
  struct Op : terminal<as<unsplice>(detail::spelling<S>())> {};

  static constexpr auto CoreQualifierWord =
      ref<Keyword<"const">> | ref<Keyword<"in">> | ref<Keyword<"out">> | ref<Keyword<"inout">> | ref<Keyword<"centroid">> |
      ref<Keyword<"patch">> | ref<Keyword<"sample">> | ref<Keyword<"uniform">> | ref<Keyword<"buffer">> | ref<Keyword<"shared">> |
      ref<Keyword<"coherent">> | ref<Keyword<"volatile">> | ref<Keyword<"restrict">> | ref<Keyword<"readonly">> |
      ref<Keyword<"writeonly">> | ref<Keyword<"attribute">> | ref<Keyword<"varying">> | ref<Keyword<"highp">> |
      ref<Keyword<"mediump">> | ref<Keyword<"lowp">> | ref<Keyword<"smooth">> | ref<Keyword<"flat">> |
      ref<Keyword<"noperspective">> | ref<Keyword<"invariant">> | ref<Keyword<"precise">>;

  static constexpr auto QualifierWord = CoreQualifierWord |
      kw<"pervertexEXT"> | kw<"pervertexNV"> | kw<"perprimitiveEXT"> |
      kw<"perprimitiveNV"> | kw<"perviewNV"> | kw<"taskNV"> |
      kw<"nonuniformEXT"> | kw<"rayPayloadEXT"> | kw<"rayPayloadInEXT"> |
      kw<"hitAttributeEXT"> | kw<"callableDataEXT"> | kw<"callableDataInEXT"> |
      kw<"rayPayloadNV"> | kw<"rayPayloadInNV"> | kw<"hitAttributeNV"> |
      kw<"callableDataNV"> | kw<"callableDataInNV"> | kw<"devicecoherent"> |
      kw<"queuefamilycoherent"> | kw<"workgroupcoherent"> |
      kw<"subgroupcoherent"> | kw<"nonprivate"> | kw<"shadercallcoherent"> |
      kw<"nontemporal"> | kw<"taskPayloadSharedEXT">;

  static constexpr auto PrecisionWord =
      ref<Keyword<"highp">> | ref<Keyword<"mediump">> | ref<Keyword<"lowp">>;

  static constexpr auto ReservedWords = CoreQualifierWord |
      ref<Keyword<"true">> | ref<Keyword<"false">> | ref<Keyword<"void">> |
      ref<Keyword<"subroutine">> | ref<Keyword<"layout">> | ref<Keyword<"struct">> |
      ref<Keyword<"precision">> | ref<Keyword<"if">> | ref<Keyword<"else">> |
      ref<Keyword<"switch">> | ref<Keyword<"case">> | ref<Keyword<"default">> |
      ref<Keyword<"while">> | ref<Keyword<"do">> | ref<Keyword<"for">> |
      ref<Keyword<"continue">> | ref<Keyword<"break">> | ref<Keyword<"return">> |
      ref<Keyword<"discard">>;
  struct ID : terminal<as<unsplice>(!ReservedWords + cr<"a-zA-Z_"> +
                                    many(w | Splice))> {};

  static constexpr auto OpAssign =
      ref<Op<"<<=">> | ref<Op<">>=">> | ref<Op<"+=">> | ref<Op<"-=">> |
      ref<Op<"*=">> | ref<Op<"/=">> | ref<Op<"%=">> | ref<Op<"&=">> |
      ref<Op<"^=">> | ref<Op<"|=">> | lit<"=">;
  static constexpr auto UnaryOp = lit<"+"> | lit<"-"> | lit<"!"> | lit<"~">;
  static constexpr auto OpMul = lit<"*"> | lit<"/"> | lit<"%">;
  static constexpr auto OpAdd = lit<"+"> | lit<"-">;
  static constexpr auto OpShift = ref<Op<"<<">> | ref<Op<">>">>;
  static constexpr auto OpRel =
      ref<Op<"<=">> | ref<Op<">=">> | lit<"<"> | lit<">">;
  static constexpr auto OpEq = ref<Op<"==">> | ref<Op<"!=">>;
  static constexpr auto OpBitAnd = lit<"&">;
  static constexpr auto OpBitXor = lit<"^">;
  static constexpr auto OpBitOr = lit<"|">;
  static constexpr auto OpLogAnd = ref<Op<"&&">>;
  static constexpr auto OpLogXor = ref<Op<"^^">>;
  static constexpr auto OpLogOr = ref<Op<"||">>;

  struct Expression;
  struct AssignmentExpression;
  struct ConstantExpression;
  struct BinaryExpression;
  struct UnaryExpression;
  struct PostfixExpression;
  struct PrimaryExpression;
  struct TypeRef;
  struct PlainTypeRef;
  struct TypeArgument;
  struct ParameterizedTypeRef;
  struct StructSpecifier;
  struct TypeQualifier;
  struct Initializer;
  struct Declaration;
  struct LocalDeclaration;
  struct FunctionPrototypeRule;
  struct LocalFunctionPrototype;
  struct Statement;
  struct CompoundStmt;
  struct Condition;
  struct Directive;

  struct ArrayDim
      : rule<lit<"["> +
             option(assign<&ast::ArrayDim::size>(ref<ConstantExpression>)) +
             lit<"]">> {};

  struct Identifier : rule<assign<&ast::Ident::name>(ref<ID>)> {};
  struct BoolLit
      : rule<create<ast::BoolLit> +
             (flag<&ast::BoolLit::value>(ref<Keyword<"true">>) | ref<Keyword<"false">>)> {};
  struct NumberLit : rule<assign<&ast::NumberLit::text>(ref<Number>)> {};
  struct StringLit : rule<assign<&ast::StringLit::text>(ref<String>)> {};
  static constexpr auto BareVoid = ref<Keyword<"void">> + &lit<")">;
  struct PrimaryExpression
      : rule<ref<Identifier> | ref<BoolLit> | ref<NumberLit> |
             ref<StringLit> | (lit<"("> + ref<Expression> + lit<")">)> {};

  struct PostfixExpression
      : rule<ref<PrimaryExpression> +
             many(nest<ast::IndexExpr, &ast::IndexExpr::array>(
                      lit<"["> +
                      option(assign<&ast::IndexExpr::index>(ref<Expression>)) +
                      lit<"]">) |
                  nest<ast::CallExpr, &ast::CallExpr::callee>(
                      lit<"("> +
                      option(BareVoid |
                             (!BareVoid + some(append<&ast::CallExpr::args>(
                                                   ref<AssignmentExpression>),
                                               lit<",">))) +
                      lit<")">) |
                  nest<ast::MemberExpr, &ast::MemberExpr::object>(
                      lit<"."> + assign<&ast::MemberExpr::field>(ref<ID>)) |
                  nest<ast::PostfixOpExpr, &ast::PostfixOpExpr::operand>(
                      assign<&ast::PostfixOpExpr::op>(ref<Op<"++">> |
                                                      ref<Op<"--">>)))> {};

  struct UnaryExpression
      : rule<create<ast::PrefixOpExpr>(
                 assign<&ast::PrefixOpExpr::op>(ref<Op<"++">> | ref<Op<"--">>) +
                 assign<&ast::PrefixOpExpr::operand>(ref<UnaryExpression>)) |
             create<ast::PrefixOpExpr>(
                 assign<&ast::PrefixOpExpr::op>(UnaryOp) +
                 assign<&ast::PrefixOpExpr::operand>(ref<UnaryExpression>)) |
             ref<PostfixExpression>> {};

  struct BinaryExpression
      : infix<UnaryExpression, ast::BinaryExpr,
              levels(left(OpMul), left(OpAdd), left(OpShift), left(OpRel),
                     left(OpEq), left(OpBitAnd), left(OpBitXor), left(OpBitOr),
                     left(OpLogAnd), left(OpLogXor), left(OpLogOr))> {};

  struct ConstantExpression
      : rule<create<ast::CondExpr>(
                 assign<&ast::CondExpr::cond>(ref<BinaryExpression>) +
                 lit<"?"> + assign<&ast::CondExpr::thenValue>(ref<Expression>) +
                 lit<":"> +
                 assign<&ast::CondExpr::elseValue>(ref<AssignmentExpression>)) |
             ref<BinaryExpression>> {};

  struct AssignmentExpression
      : rule<create<ast::AssignExpr>(
                 assign<&ast::AssignExpr::target>(ref<UnaryExpression>) +
                 assign<&ast::AssignExpr::op>(OpAssign) +
                 assign<&ast::AssignExpr::value>(ref<AssignmentExpression>)) |
             ref<ConstantExpression>> {};

  struct Expression
      : infix<AssignmentExpression, ast::BinaryExpr, left(lit<",">)> {};

  struct LayoutQualifierId
      : rule<assign<&ast::LayoutQualifierId::name>(ref<ID> | ref<Keyword<"shared">>) +
             option(lit<"="> + assign<&ast::LayoutQualifierId::value>(
                                   ref<AssignmentExpression>))> {};
  struct QualifierClause
      : rule<assign<&ast::QualifierClause::name>(QualifierWord) |
             (assign<&ast::QualifierClause::name>(ref<Keyword<"subroutine">>) +
              option(
                  lit<"("> +
                  some(append<&ast::QualifierClause::subroutineTypes>(ref<ID>),
                       lit<",">) +
                  lit<")">)) |
             (assign<&ast::QualifierClause::name>(ref<Keyword<"layout">>) + lit<"("> +
              some(append<&ast::QualifierClause::layout>(
                       ref<LayoutQualifierId>),
                   lit<",">) +
              lit<")">)> {};
  struct TypeQualifier : rule<some(append<&ast::TypeQualifier::clauses>(
                                 ref<QualifierClause>))> {};

  struct Declarator
      : rule<assign<&ast::Declarator::name>(ref<ID>) +
             many(append<&ast::Declarator::dims>(ref<ArrayDim>))> {};

  struct StructMember
      : rule<option(assign<&ast::StructMember::qualifier>(
                 ref<TypeQualifier>)) +
             assign<&ast::StructMember::type>(ref<TypeRef>) +
             some(append<&ast::StructMember::declarators>(ref<Declarator>),
                  lit<",">) +
             lit<";">> {};

  struct StructSpecifier
      : rule<ref<Keyword<"struct">> +
             option(assign<&ast::StructSpecifier::name>(ref<ID>)) + lit<"{"> +
             many(append<&ast::StructSpecifier::members>(ref<StructMember> |
                                                         ref<Directive>)) +
             lit<"}">> {};

  static constexpr auto TypeHead =
      assign<&ast::TypeRef::name>(ref<ID> | ref<Keyword<"void">>) |
      assign<&ast::TypeRef::structure>(ref<StructSpecifier>);
  static constexpr auto TypeParameters =
      lit<"<"> +
      some(append<&ast::TypeRef::parameters>(ref<TypeArgument>), lit<",">) +
      lit<">">;
  static constexpr auto TypeDimensions =
      many(append<&ast::TypeRef::dims>(ref<ArrayDim>));
  struct TypeRef
      : rule<TypeHead + option(TypeParameters) + TypeDimensions> {};
  struct PlainTypeRef : rule<TypeHead + TypeDimensions> {};
  struct ParameterizedTypeRef
      : rule<TypeHead + TypeParameters + TypeDimensions> {};
  struct TypeArgument
      : rule<ref<ParameterizedTypeRef> |
             (&ref<Keyword<"struct">> + ref<TypeRef>) | ref<UnaryExpression>> {};
  struct Initializer
      : rule<create<ast::ListInit>(
                 lit<"{"> +
                 many(append<&ast::ListInit::items>(ref<Initializer>),
                      lit<",">) +
                 option(lit<",">) + lit<"}">) |
             create<ast::ExprInit>(
                 assign<&ast::ExprInit::value>(ref<AssignmentExpression>))> {};

  struct InitDeclarator
      : rule<assign<&ast::InitDeclarator::name>(ref<ID>) +
             many(append<&ast::InitDeclarator::dims>(ref<ArrayDim>)) +
             option(lit<"="> +
                    assign<&ast::InitDeclarator::init>(ref<Initializer>))> {
  };

  struct FunctionDeclStmt : rule<assign<&ast::FunctionDeclStmt::prototype>(
                                         ref<FunctionPrototypeRule>) +
                                     lit<";">> {};

  template <class Type>
  static constexpr auto VariableDeclaration =
      option(assign<&ast::VarDeclStmt::qualifier>(ref<TypeQualifier>)) +
      assign<&ast::VarDeclStmt::type>(ref<Type>) +
      some(append<&ast::VarDeclStmt::declarators>(ref<InitDeclarator>),
           lit<",">) +
      lit<";">;
  struct VarDeclStmt : rule<VariableDeclaration<TypeRef>> {};
  struct LocalVarDeclStmt : rule<VariableDeclaration<PlainTypeRef>> {};
  struct LocalFunctionDeclStmt
      : rule<assign<&ast::FunctionDeclStmt::prototype>(
                 ref<LocalFunctionPrototype>) +
             lit<";">> {};
  struct LocalTypeDeclStmt
      : rule<option(
                 assign<&ast::VarDeclStmt::qualifier>(ref<TypeQualifier>)) +
             assign<&ast::VarDeclStmt::type>(&ref<Keyword<"struct">> +
                                             ref<PlainTypeRef>) +
             lit<";">> {};
  struct TypeDeclStmt : rule<option(assign<&ast::VarDeclStmt::qualifier>(
                                     ref<TypeQualifier>)) +
                                 assign<&ast::VarDeclStmt::type>(
                                     (&ref<Keyword<"struct">> + ref<TypeRef>) |
                                     ref<ParameterizedTypeRef>) +
                                 lit<";">> {};

  struct PrecisionDeclStmt
      : rule<ref<Keyword<"precision">> +
             assign<&ast::PrecisionDeclStmt::precision>(PrecisionWord) +
             assign<&ast::PrecisionDeclStmt::type>(ref<TypeRef>) +
             lit<";">> {};

  struct InterfaceBlockDeclStmt
      : rule<assign<&ast::InterfaceBlockDeclStmt::qualifier>(
                 ref<TypeQualifier>) +
             assign<&ast::InterfaceBlockDeclStmt::blockName>(ref<ID>) +
             lit<"{"> +
             many(append<&ast::InterfaceBlockDeclStmt::members>(
                 ref<StructMember> | ref<Directive>)) +
             lit<"}"> +
             option(
                 assign<&ast::InterfaceBlockDeclStmt::instanceName>(ref<ID>) +
                 option(some(append<&ast::InterfaceBlockDeclStmt::instanceDims>(
                     ref<ArrayDim>)))) +
             lit<";">> {};

  struct QualifierDeclStmt
      : rule<assign<&ast::QualifierDeclStmt::qualifier>(
                 ref<TypeQualifier>) +
             option(some(append<&ast::QualifierDeclStmt::names>(ref<ID>),
                         lit<",">)) +
             lit<";">> {};

  struct Declaration
      : rule<ref<FunctionDeclStmt> | ref<VarDeclStmt> |
             ref<TypeDeclStmt> | ref<PrecisionDeclStmt> |
             ref<InterfaceBlockDeclStmt> | ref<QualifierDeclStmt>> {};

  struct LocalDeclaration
      : rule<ref<LocalFunctionDeclStmt> | ref<LocalVarDeclStmt> |
             ref<LocalTypeDeclStmt> | ref<PrecisionDeclStmt> |
             ref<InterfaceBlockDeclStmt> | ref<QualifierDeclStmt>> {};

  struct Param
      : rule<option(assign<&ast::Param::qualifier>(ref<TypeQualifier>)) +
             assign<&ast::Param::type>(ref<TypeRef>) +
             option(assign<&ast::Param::name>(ref<ID>) +
                    many(append<&ast::Param::dims>(ref<ArrayDim>)))> {};

  template <class Type>
  static constexpr auto FunctionPrototype =
      option(
          assign<&ast::FunctionPrototype::qualifier>(ref<TypeQualifier>)) +
      assign<&ast::FunctionPrototype::returnType>(ref<Type>) +
      assign<&ast::FunctionPrototype::name>(ref<ID>) + lit<"("> +
      option(BareVoid |
             (!BareVoid +
              some(append<&ast::FunctionPrototype::params>(ref<Param>),
                   lit<",">))) +
      lit<")">;
  struct FunctionPrototypeRule : rule<FunctionPrototype<TypeRef>> {};
  struct LocalFunctionPrototype
      : rule<FunctionPrototype<PlainTypeRef>> {};

  struct FunctionDef
      : rule<assign<&ast::FunctionDef::prototype>(ref<FunctionPrototypeRule>) +
             assign<&ast::FunctionDef::body>(ref<CompoundStmt>)> {};

  struct NameStmt : rule<assign<&ast::NameStmt::name>(ref<ID>) + lit<";">> {
  };
  struct ExprStmt
      : rule<!(ref<ID> + lit<";">)+option(
                 assign<&ast::ExprStmt::value>(ref<Expression>)) +
             lit<";">> {};

  struct Condition
      : rule<assign<&ast::Condition::expr>(ref<Expression>) |
             (option(
                  assign<&ast::Condition::qualifier>(ref<TypeQualifier>)) +
              assign<&ast::Condition::type>(ref<PlainTypeRef>) +
              assign<&ast::Condition::name>(ref<ID>) + lit<"="> +
              assign<&ast::Condition::init>(ref<Initializer>))> {};

  struct IfStmt
      : rule<ref<Keyword<"if">> + lit<"("> + assign<&ast::IfStmt::cond>(ref<Expression>) +
             lit<")"> + assign<&ast::IfStmt::thenBranch>(ref<Statement>) +
             option(commit(ref<Keyword<"else">>, assign<&ast::IfStmt::elseBranch>(ref<Statement>)))> {};

  struct SwitchStmt
      : rule<ref<Keyword<"switch">> + lit<"("> +
             assign<&ast::SwitchStmt::value>(ref<Expression>) + lit<")"> +
             lit<"{"> +
             many(append<&ast::SwitchStmt::items>(ref<Statement>)) +
             lit<"}">> {};

  struct CaseLabelStmt
      : rule<ref<Keyword<"case">> + assign<&ast::CaseLabelStmt::value>(ref<Expression>) +
             lit<":">> {};

  struct DefaultLabelStmt
      : rule<create<ast::DefaultLabelStmt> + ref<Keyword<"default">> + lit<":">> {};

  struct WhileStmt
      : rule<ref<Keyword<"while">> + lit<"("> +
             assign<&ast::WhileStmt::cond>(ref<Condition>) + lit<")"> +
             assign<&ast::WhileStmt::body>(ref<Statement>)> {};

  struct DoWhileStmt
      : rule<ref<Keyword<"do">> + assign<&ast::DoWhileStmt::body>(ref<Statement>) +
             ref<Keyword<"while">> + lit<"("> +
             assign<&ast::DoWhileStmt::cond>(ref<Expression>) + lit<")"> +
             lit<";">> {};

  struct ForStmt
      : rule<ref<Keyword<"for">> + lit<"("> +
             assign<&ast::ForStmt::init>(ref<NameStmt> | ref<ExprStmt> |
                                         ref<LocalDeclaration>) +
             option(assign<&ast::ForStmt::cond>(ref<Condition>)) +
             lit<";"> + option(assign<&ast::ForStmt::step>(ref<Expression>)) +
             lit<")"> + assign<&ast::ForStmt::body>(ref<Statement>)> {};

  struct ContinueStmt
      : rule<create<ast::ContinueStmt> + ref<Keyword<"continue">> + lit<";">> {};
  struct BreakStmt : rule<create<ast::BreakStmt> + ref<Keyword<"break">> + lit<";">> {
  };
  struct ReturnStmt
      : rule<ref<Keyword<"return">> +
             option(assign<&ast::ReturnStmt::value>(ref<Expression>)) +
             lit<";">> {};
  struct DiscardStmt
      : rule<create<ast::DiscardStmt> + ref<Keyword<"discard">> + lit<";">> {};

  struct CompoundStmt
      : rule<lit<"{"> +
             many(append<&ast::CompoundStmt::items>(ref<Statement>)) +
             lit<"}">> {};

  struct Directive
      : rule<assign<&ast::RawDirective::text>(ref<DirectiveText>)> {};
  struct Attribute
      : rule<assign<&ast::Attribute::name>(ref<ID>) +
             option(
                 lit<"("> +
                 some(append<&ast::Attribute::args>(ref<AssignmentExpression>),
                      lit<",">) +
                 lit<")">)> {};
  struct AttributedStmt
      : rule<lit<"[["> +
             some(append<&ast::AttributedStmt::attributes>(ref<Attribute>),
                  lit<",">) +
             lit<"]]"> +
             assign<&ast::AttributedStmt::body>(ref<Statement>)> {};

  struct Statement
      : rule<ref<AttributedStmt> | ref<CompoundStmt> |
             ref<LocalDeclaration> | ref<NameStmt> | ref<ExprStmt> |
             ref<IfStmt> | ref<SwitchStmt> | ref<CaseLabelStmt> |
             ref<DefaultLabelStmt> | ref<WhileStmt> |
             ref<DoWhileStmt> | ref<ForStmt> | ref<ContinueStmt> |
             ref<BreakStmt> | ref<ReturnStmt> | ref<DiscardStmt> |
             ref<Directive>> {};

  struct TranslationUnit
      : rule<create<ast::TranslationUnit> +
             many(append<&ast::TranslationUnit::items>(ref<FunctionDef>) |
                  append<&ast::TranslationUnit::items>(ref<Declaration>) |
                  append<&ast::TranslationUnit::items>(ref<Directive>) |
                  append<&ast::TranslationUnit::items>(ref<NameStmt>) |
                  append<&ast::TranslationUnit::items>(
                      create<ast::ExprStmt>(lit<";">)))> {};

  using entry = TranslationUnit;
  using ignored = list<WS, LineCont>;
  using hidden = list<BlockComment, LineComment>;
};

} // namespace glsl
