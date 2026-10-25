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
  struct BlockComment : terminal<through(lit<"/*">, lit<"*/">)> {};
  struct LineComment : terminal<lit<"//"> + many(Splice | cr<R"(^\r\n)">)> {};
  struct ID : terminal<cr<"a-zA-Z_"> + many(w | Splice), text_conv<unsplice>> {
  };
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
  struct Op : terminal<detail::spelling<S>(), text_conv<unsplice>> {};

  static constexpr auto QualifierWord =
      kw<"const"> | kw<"in"> | kw<"out"> | kw<"inout"> | kw<"centroid"> |
      kw<"patch"> | kw<"sample"> | kw<"uniform"> | kw<"buffer"> | kw<"shared"> |
      kw<"coherent"> | kw<"volatile"> | kw<"restrict"> | kw<"readonly"> |
      kw<"writeonly"> | kw<"attribute"> | kw<"varying"> | kw<"highp"> |
      kw<"mediump"> | kw<"lowp"> | kw<"smooth"> | kw<"flat"> |
      kw<"noperspective"> | kw<"invariant"> | kw<"precise"> |
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
      kw<"highp"> | kw<"mediump"> | kw<"lowp">;

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
  struct TypeRefRule;
  struct PlainTypeRefRule;
  struct TypeArgumentRule;
  struct ParameterizedTypeRefRule;
  struct StructSpecifierRule;
  struct TypeQualifierRule;
  struct InitializerRule;
  struct DeclarationRule;
  struct LocalDeclarationRule;
  struct FunctionPrototypeRule;
  struct LocalFunctionPrototypeRule;
  struct StatementRule;
  struct CompoundStmtRule;
  struct ConditionRule;
  struct DirectiveRule;

  struct ArrayDimRule
      : rule<lit<"["> +
             option(assign<&ast::ArrayDim::size>(ref<ConstantExpression>)) +
             lit<"]">> {};

  struct IdentifierRule : rule<assign<&ast::Ident::name>(ref<ID>)> {};
  struct BoolLitRule
      : rule<create<ast::BoolLit> +
             (flag<&ast::BoolLit::value>(kw<"true">) | kw<"false">)> {};
  struct NumberLitRule : rule<assign<&ast::NumberLit::text>(ref<Number>)> {};
  struct StringLitRule : rule<assign<&ast::StringLit::text>(ref<String>)> {};
  static constexpr auto BareVoid = kw<"void"> + &lit<")">;
  struct PrimaryExpression
      : rule<ref<IdentifierRule> | ref<BoolLitRule> | ref<NumberLitRule> |
             ref<StringLitRule> | (lit<"("> + ref<Expression> + lit<")">)> {};

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
      : infix<UnaryExpression, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(OpMul) >> left(OpAdd) >> left(OpShift) >> left(OpRel) >>
               left(OpEq) >> left(OpBitAnd) >> left(OpBitXor) >>
               left(OpBitOr) >> left(OpLogAnd) >> left(OpLogXor) >>
               left(OpLogOr))> {};

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
      : infix<AssignmentExpression, &ast::BinaryExpr::left,
              &ast::BinaryExpr::op, &ast::BinaryExpr::right, left(lit<",">)> {};

  struct LayoutQualifierIdRule
      : rule<assign<&ast::LayoutQualifierId::name>(ref<ID> | kw<"shared">) +
             option(lit<"="> + assign<&ast::LayoutQualifierId::value>(
                                   ref<AssignmentExpression>))> {};
  struct QualifierClauseRule
      : rule<assign<&ast::QualifierClause::name>(QualifierWord) |
             (assign<&ast::QualifierClause::name>(kw<"subroutine">) +
              option(
                  lit<"("> +
                  some(append<&ast::QualifierClause::subroutineTypes>(ref<ID>),
                       lit<",">) +
                  lit<")">)) |
             (assign<&ast::QualifierClause::name>(kw<"layout">) + lit<"("> +
              some(append<&ast::QualifierClause::layout>(
                       ref<LayoutQualifierIdRule>),
                   lit<",">) +
              lit<")">)> {};
  struct TypeQualifierRule : rule<some(append<&ast::TypeQualifier::clauses>(
                                 ref<QualifierClauseRule>))> {};

  struct DeclaratorRule
      : rule<assign<&ast::Declarator::name>(ref<ID>) +
             many(append<&ast::Declarator::dims>(ref<ArrayDimRule>))> {};

  struct StructMemberRule
      : rule<option(assign<&ast::StructMember::qualifier>(
                 ref<TypeQualifierRule>)) +
             assign<&ast::StructMember::type>(ref<TypeRefRule>) +
             some(append<&ast::StructMember::declarators>(ref<DeclaratorRule>),
                  lit<",">) +
             lit<";">> {};

  struct StructSpecifierRule
      : rule<kw<"struct"> +
             option(assign<&ast::StructSpecifier::name>(ref<ID>)) + lit<"{"> +
             many(append<&ast::StructSpecifier::members>(ref<StructMemberRule> |
                                                         ref<DirectiveRule>)) +
             lit<"}">> {};

  static constexpr auto TypeHead =
      assign<&ast::TypeRef::name>(ref<ID>) |
      assign<&ast::TypeRef::structure>(ref<StructSpecifierRule>);
  static constexpr auto TypeParameters =
      lit<"<"> +
      some(append<&ast::TypeRef::parameters>(ref<TypeArgumentRule>), lit<",">) +
      lit<">">;
  static constexpr auto TypeDimensions =
      many(append<&ast::TypeRef::dims>(ref<ArrayDimRule>));
  struct TypeRefRule
      : rule<TypeHead + option(TypeParameters) + TypeDimensions> {};
  struct PlainTypeRefRule : rule<TypeHead + TypeDimensions> {};
  struct ParameterizedTypeRefRule
      : rule<TypeHead + TypeParameters + TypeDimensions> {};
  struct TypeArgumentRule
      : rule<ref<ParameterizedTypeRefRule> |
             (&kw<"struct"> + ref<TypeRefRule>) | ref<UnaryExpression>> {};
  struct InitializerRule
      : rule<create<ast::ListInit>(
                 lit<"{"> +
                 many(append<&ast::ListInit::items>(ref<InitializerRule>),
                      lit<",">) +
                 option(lit<",">) + lit<"}">) |
             create<ast::ExprInit>(
                 assign<&ast::ExprInit::value>(ref<AssignmentExpression>))> {};

  struct InitDeclaratorRule
      : rule<assign<&ast::InitDeclarator::name>(ref<ID>) +
             many(append<&ast::InitDeclarator::dims>(ref<ArrayDimRule>)) +
             option(lit<"="> +
                    assign<&ast::InitDeclarator::init>(ref<InitializerRule>))> {
  };

  struct FunctionDeclStmtRule : rule<assign<&ast::FunctionDeclStmt::prototype>(
                                         ref<FunctionPrototypeRule>) +
                                     lit<";">> {};

  template <class Type>
  static constexpr auto VariableDeclaration =
      option(assign<&ast::VarDeclStmt::qualifier>(ref<TypeQualifierRule>)) +
      assign<&ast::VarDeclStmt::type>(ref<Type>) +
      some(append<&ast::VarDeclStmt::declarators>(ref<InitDeclaratorRule>),
           lit<",">) +
      lit<";">;
  struct VarDeclStmtRule : rule<VariableDeclaration<TypeRefRule>> {};
  struct LocalVarDeclStmtRule : rule<VariableDeclaration<PlainTypeRefRule>> {};
  struct LocalFunctionDeclStmtRule
      : rule<assign<&ast::FunctionDeclStmt::prototype>(
                 ref<LocalFunctionPrototypeRule>) +
             lit<";">> {};
  struct LocalTypeDeclStmtRule
      : rule<option(
                 assign<&ast::VarDeclStmt::qualifier>(ref<TypeQualifierRule>)) +
             assign<&ast::VarDeclStmt::type>(&kw<"struct"> +
                                             ref<PlainTypeRefRule>) +
             lit<";">> {};
  struct TypeDeclStmtRule : rule<option(assign<&ast::VarDeclStmt::qualifier>(
                                     ref<TypeQualifierRule>)) +
                                 assign<&ast::VarDeclStmt::type>(
                                     (&kw<"struct"> + ref<TypeRefRule>) |
                                     ref<ParameterizedTypeRefRule>) +
                                 lit<";">> {};

  struct PrecisionDeclStmtRule
      : rule<kw<"precision"> +
             assign<&ast::PrecisionDeclStmt::precision>(PrecisionWord) +
             assign<&ast::PrecisionDeclStmt::type>(ref<TypeRefRule>) +
             lit<";">> {};

  struct InterfaceBlockDeclStmtRule
      : rule<assign<&ast::InterfaceBlockDeclStmt::qualifier>(
                 ref<TypeQualifierRule>) +
             assign<&ast::InterfaceBlockDeclStmt::blockName>(ref<ID>) +
             lit<"{"> +
             many(append<&ast::InterfaceBlockDeclStmt::members>(
                 ref<StructMemberRule> | ref<DirectiveRule>)) +
             lit<"}"> +
             option(
                 assign<&ast::InterfaceBlockDeclStmt::instanceName>(ref<ID>) +
                 option(some(append<&ast::InterfaceBlockDeclStmt::instanceDims>(
                     ref<ArrayDimRule>)))) +
             lit<";">> {};

  struct QualifierDeclStmtRule
      : rule<assign<&ast::QualifierDeclStmt::qualifier>(
                 ref<TypeQualifierRule>) +
             option(some(append<&ast::QualifierDeclStmt::names>(ref<ID>),
                         lit<",">)) +
             lit<";">> {};

  struct DeclarationRule
      : rule<ref<FunctionDeclStmtRule> | ref<VarDeclStmtRule> |
             ref<TypeDeclStmtRule> | ref<PrecisionDeclStmtRule> |
             ref<InterfaceBlockDeclStmtRule> | ref<QualifierDeclStmtRule>> {};

  struct LocalDeclarationRule
      : rule<ref<LocalFunctionDeclStmtRule> | ref<LocalVarDeclStmtRule> |
             ref<LocalTypeDeclStmtRule> | ref<PrecisionDeclStmtRule> |
             ref<InterfaceBlockDeclStmtRule> | ref<QualifierDeclStmtRule>> {};

  struct ParamRule
      : rule<option(assign<&ast::Param::qualifier>(ref<TypeQualifierRule>)) +
             assign<&ast::Param::type>(ref<TypeRefRule>) +
             option(assign<&ast::Param::name>(ref<ID>) +
                    many(append<&ast::Param::dims>(ref<ArrayDimRule>)))> {};

  template <class Type>
  static constexpr auto FunctionPrototype =
      option(
          assign<&ast::FunctionPrototype::qualifier>(ref<TypeQualifierRule>)) +
      assign<&ast::FunctionPrototype::returnType>(ref<Type>) +
      assign<&ast::FunctionPrototype::name>(ref<ID>) + lit<"("> +
      option(BareVoid |
             (!BareVoid +
              some(append<&ast::FunctionPrototype::params>(ref<ParamRule>),
                   lit<",">))) +
      lit<")">;
  struct FunctionPrototypeRule : rule<FunctionPrototype<TypeRefRule>> {};
  struct LocalFunctionPrototypeRule
      : rule<FunctionPrototype<PlainTypeRefRule>> {};

  struct FunctionDefRule
      : rule<assign<&ast::FunctionDef::prototype>(ref<FunctionPrototypeRule>) +
             assign<&ast::FunctionDef::body>(ref<CompoundStmtRule>)> {};

  struct NameStmtRule : rule<assign<&ast::NameStmt::name>(ref<ID>) + lit<";">> {
  };
  struct ExprStmtRule
      : rule<!(ref<ID> + lit<";">)+option(
                 assign<&ast::ExprStmt::value>(ref<Expression>)) +
             lit<";">> {};

  struct ConditionRule
      : rule<assign<&ast::Condition::expr>(ref<Expression>) |
             (option(
                  assign<&ast::Condition::qualifier>(ref<TypeQualifierRule>)) +
              assign<&ast::Condition::type>(ref<PlainTypeRefRule>) +
              assign<&ast::Condition::name>(ref<ID>) + lit<"="> +
              assign<&ast::Condition::init>(ref<InitializerRule>))> {};

  struct IfStmtRule
      : rule<kw<"if"> + lit<"("> + assign<&ast::IfStmt::cond>(ref<Expression>) +
             lit<")"> + assign<&ast::IfStmt::thenBranch>(ref<StatementRule>) +
             option(commit(kw<"else">, assign<&ast::IfStmt::elseBranch>(ref<StatementRule>)))> {};

  struct SwitchStmtRule
      : rule<kw<"switch"> + lit<"("> +
             assign<&ast::SwitchStmt::value>(ref<Expression>) + lit<")"> +
             lit<"{"> +
             many(append<&ast::SwitchStmt::items>(ref<StatementRule>)) +
             lit<"}">> {};

  struct CaseLabelStmtRule
      : rule<kw<"case"> + assign<&ast::CaseLabelStmt::value>(ref<Expression>) +
             lit<":">> {};

  struct DefaultLabelStmtRule
      : rule<create<ast::DefaultLabelStmt> + kw<"default"> + lit<":">> {};

  struct WhileStmtRule
      : rule<kw<"while"> + lit<"("> +
             assign<&ast::WhileStmt::cond>(ref<ConditionRule>) + lit<")"> +
             assign<&ast::WhileStmt::body>(ref<StatementRule>)> {};

  struct DoWhileStmtRule
      : rule<kw<"do"> + assign<&ast::DoWhileStmt::body>(ref<StatementRule>) +
             kw<"while"> + lit<"("> +
             assign<&ast::DoWhileStmt::cond>(ref<Expression>) + lit<")"> +
             lit<";">> {};

  struct ForStmtRule
      : rule<kw<"for"> + lit<"("> +
             assign<&ast::ForStmt::init>(ref<NameStmtRule> | ref<ExprStmtRule> |
                                         ref<LocalDeclarationRule>) +
             option(assign<&ast::ForStmt::cond>(ref<ConditionRule>)) +
             lit<";"> + option(assign<&ast::ForStmt::step>(ref<Expression>)) +
             lit<")"> + assign<&ast::ForStmt::body>(ref<StatementRule>)> {};

  struct ContinueStmtRule
      : rule<create<ast::ContinueStmt> + kw<"continue"> + lit<";">> {};
  struct BreakStmtRule : rule<create<ast::BreakStmt> + kw<"break"> + lit<";">> {
  };
  struct ReturnStmtRule
      : rule<kw<"return"> +
             option(assign<&ast::ReturnStmt::value>(ref<Expression>)) +
             lit<";">> {};
  struct DiscardStmtRule
      : rule<create<ast::DiscardStmt> + kw<"discard"> + lit<";">> {};

  struct CompoundStmtRule
      : rule<lit<"{"> +
             many(append<&ast::CompoundStmt::items>(ref<StatementRule>)) +
             lit<"}">> {};

  struct DirectiveRule
      : rule<assign<&ast::RawDirective::text>(ref<DirectiveText>)> {};
  struct AttributeRule
      : rule<assign<&ast::Attribute::name>(ref<ID>) +
             option(
                 lit<"("> +
                 some(append<&ast::Attribute::args>(ref<AssignmentExpression>),
                      lit<",">) +
                 lit<")">)> {};
  struct AttributedStmtRule
      : rule<lit<"[["> +
             some(append<&ast::AttributedStmt::attributes>(ref<AttributeRule>),
                  lit<",">) +
             lit<"]]"> +
             assign<&ast::AttributedStmt::body>(ref<StatementRule>)> {};

  struct StatementRule
      : rule<ref<AttributedStmtRule> | ref<CompoundStmtRule> |
             ref<LocalDeclarationRule> | ref<NameStmtRule> | ref<ExprStmtRule> |
             ref<IfStmtRule> | ref<SwitchStmtRule> | ref<CaseLabelStmtRule> |
             ref<DefaultLabelStmtRule> | ref<WhileStmtRule> |
             ref<DoWhileStmtRule> | ref<ForStmtRule> | ref<ContinueStmtRule> |
             ref<BreakStmtRule> | ref<ReturnStmtRule> | ref<DiscardStmtRule> |
             ref<DirectiveRule>> {};

  struct TranslationUnitRule
      : rule<create<ast::TranslationUnit> +
             many(append<&ast::TranslationUnit::items>(ref<FunctionDefRule>) |
                  append<&ast::TranslationUnit::items>(ref<DeclarationRule>) |
                  append<&ast::TranslationUnit::items>(ref<DirectiveRule>) |
                  append<&ast::TranslationUnit::items>(ref<NameStmtRule>) |
                  append<&ast::TranslationUnit::items>(
                      create<ast::ExprStmt>(lit<";">)))> {};

  using entry = TranslationUnitRule;
  using ignore = list<WS, LineCont>;
  using hidden = list<BlockComment, LineComment>;
};

} // namespace glsl
