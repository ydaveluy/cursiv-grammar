#pragma once

#include <memory_resource>
#include <span>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "pascal/core/ast.hpp"

namespace pascal {

struct Pascal : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct COMMENT_STAR
      : terminal<lit<"(*"> +
                 many(ref<COMMENT_STAR> | (!lit<"(*"> + !lit<"*)"> + any)) +
                 lit<"*)">> {};
  struct COMMENT_CURLY
      : terminal<lit<"{"> +
                 many(ref<COMMENT_CURLY> | (!lit<"{"> + !lit<"}"> + any)) +
                 lit<"}">> {};
  struct COMMENT_LINE : terminal<lit<"//"> + many(cr<"^\r\n">)> {};

  static constexpr auto identifier_text = [](std::string_view text) {
    return text.starts_with('&') ? text.substr(1) : text;
  };
  struct IDENT : terminal<option(lit<"&">) + cr<"A-Za-z_"> + many(w),
                          text_conv<identifier_text>> {};
  struct NUM_INT : terminal<(lit<"$"> + some(cr<"0-9a-fA-F">)) |
                            (lit<"%"> + some(cr<"01">)) |
                            (lit<"&"> + some(cr<"0-7">)) | some(d)> {};
  static constexpr auto EXPONENT = cr<"eE"> + option(cr<"+-">) + some(d);
  struct NUM_REAL
      : terminal<some(d) +
                 ((lit<"."> + some(d) + option(EXPONENT)) | EXPONENT)> {};
  static constexpr auto QUOTED =
      lit<"'"> + many(lit<"''"> | cr<"^'">) + lit<"'">;
  static constexpr auto CHAR_CODE =
      lit<"#"> + ((lit<"$"> + some(cr<"0-9a-fA-F">)) |
                  (lit<"&"> + some(cr<"0-7">)) | (lit<"%"> + some(cr<"01">)) |
                  some(d));
  struct STRING_LITERAL : terminal<some(QUOTED | CHAR_CODE)> {};
  static constexpr auto unquote = [](std::string_view t,
                                     std::pmr::memory_resource* mem) {
    const auto body = t.substr(1, t.size() - 2);
    const auto room = cursiv::text_room(mem, body.size());
    std::size_t n = 0;
    for (std::size_t i = 0; i < body.size(); ++i) {
      room[n++] = body[i];
      if (body[i] == '\'') ++i;
    }
    return std::string_view(room.data(), n);
  };
  struct PATH_STRING : terminal<QUOTED, text_conv<unquote>> {};

  struct ExpressionRule;
  struct FactorRule;
  struct PowerRule;
  struct PostfixRule;
  struct ActualArgRule;
  struct SetElementRule;
  struct StatementRule;
  struct UnlabelledStatementRule;
  struct CompoundStmtRule;
  struct TypeRule;
  struct SimpleTypeRule;
  struct NamedTypeRule;
  struct ParamRule;
  struct ProcTypeRule;
  struct VariantPartRule;
  struct RecordFieldRule;
  struct ProcDeclRule;
  struct FuncDeclRule;
  struct InterfaceProcRule;
  struct InterfaceFuncRule;
  struct BlockRule;

  struct NumberLitRule
      : rule<assign<&ast::NumberLit::spelling>(ref<NUM_REAL> | ref<NUM_INT>)> {
  };
  struct StringLitRule
      : rule<assign<&ast::StringLit::spelling>(ref<STRING_LITERAL>)> {};
  struct NameExprRule : rule<assign<&ast::NameExpr::name>(ref<IDENT>)> {};
  struct NilLitRule : rule<create<ast::NilLit> + ikw<"nil">> {};
  struct ActualArgRule
      : rule<assign<&ast::ActualArg::value>(ref<ExpressionRule>) +
             many(lit<":"> +
                  append<&ast::ActualArg::widths>(ref<ExpressionRule>))> {};
  struct SetElementRule
      : rule<assign<&ast::SetElement::low>(ref<ExpressionRule>) +
             option(lit<".."> +
                    assign<&ast::SetElement::high>(ref<ExpressionRule>))> {};
  struct SetLitRule
      : rule<(lit<"["> | lit<"(.">)+option(
                 some(append<&ast::SetLit::elements>(ref<SetElementRule>),
                      lit<",">)) +
             (lit<"]"> | lit<".)">)> {};
  struct PostfixRule
      : rule<(ref<NameExprRule> | ref<NumberLitRule> | ref<StringLitRule> |
              ref<NilLitRule> | ref<SetLitRule> |
              (lit<"("> + ref<ExpressionRule> + lit<")">)) +
             many(nest<ast::CallExpr, &ast::CallExpr::callee>(
                      lit<"("> +
                      option(
                          some(append<&ast::CallExpr::args>(ref<ActualArgRule>),
                               lit<",">)) +
                      lit<")">) |
                  nest<ast::IndexExpr, &ast::IndexExpr::array>(
                      (lit<"["> | lit<"(.">)+some(
                          append<&ast::IndexExpr::indices>(ref<ExpressionRule>),
                          lit<",">) +
                      (lit<"]"> | lit<".)">)) |
                  nest<ast::FieldExpr, &ast::FieldExpr::object>(
                      lit<"."> + assign<&ast::FieldExpr::field>(ref<IDENT>)) |
                  nest<ast::DerefExpr, &ast::DerefExpr::operand>(lit<"^">))> {};
  struct FactorRule
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-">) +
                 assign<&ast::UnaryExpr::operand>(ref<PowerRule>)) |
             create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ikw<"not"> | lit<"@">) +
                 assign<&ast::UnaryExpr::operand>(ref<FactorRule>)) |
             ref<PostfixRule>> {};
  struct PowerRule
      : infix<FactorRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(lit<"**">)> {};
  static constexpr auto MulOp = lit<"*"> | lit<"/"> | ikw<"div"> | ikw<"mod"> |
                                ikw<"and"> | ikw<"shl"> | ikw<"shr"> |
                                ikw<"as"> | ikw<"is"> | lit<"><">;
  static constexpr auto AddOp = lit<"+"> | lit<"-"> | ikw<"or"> | ikw<"xor">;
  static constexpr auto RelOp = lit<"<>"> | lit<"<="> | lit<">="> | lit<"="> |
                                lit<"<"> | lit<">"> | ikw<"in">;
  struct ExpressionRule : infix<PowerRule, &ast::BinaryExpr::left,
                                &ast::BinaryExpr::op, &ast::BinaryExpr::right,
                                (left(MulOp) >> left(AddOp) >> left(RelOp))> {};

  struct CompoundStmtRule
      : rule<ikw<"begin"> +
             some(append<&ast::CompoundStmt::statements>(ref<StatementRule>),
                  lit<";">) +
             ikw<"end">> {};
  struct SimpleStatementRule
      : rule<create<ast::AssignStmt>(
                 assign<&ast::AssignStmt::target>(ref<ExpressionRule>) +
                 lit<":="> +
                 assign<&ast::AssignStmt::value>(ref<ExpressionRule>)) |
             create<ast::ExprStmt>(
                 assign<&ast::ExprStmt::value>(ref<ExpressionRule>))> {};
  struct IfStmtRule
      : rule<ikw<"if"> + assign<&ast::IfStmt::cond>(ref<ExpressionRule>) +
             ikw<"then"> +
             assign<&ast::IfStmt::thenBranch>(ref<StatementRule>) +
             option(commit(ikw<"else">, assign<&ast::IfStmt::elseBranch>(ref<StatementRule>)))> {};
  struct CaseArmRule
      : rule<some(append<&ast::CaseArm::labels>(ref<SetElementRule>),
                  lit<",">) +
             lit<":"> + assign<&ast::CaseArm::body>(ref<StatementRule>)> {};
  struct CaseStmtRule
      : rule<ikw<"case"> + assign<&ast::CaseStmt::value>(ref<ExpressionRule>) +
             ikw<"of"> +
             option(some(append<&ast::CaseStmt::arms>(ref<CaseArmRule>),
                         lit<";">)) +
             option(lit<";">) +
             option(ikw<"else"> +
                    some(append<&ast::CaseStmt::elseBody>(ref<StatementRule>),
                         lit<";">)) +
             ikw<"end">> {};
  struct WhileStmtRule
      : rule<ikw<"while"> + assign<&ast::WhileStmt::cond>(ref<ExpressionRule>) +
             ikw<"do"> + assign<&ast::WhileStmt::body>(ref<StatementRule>)> {};
  struct RepeatStmtRule
      : rule<ikw<"repeat"> +
             some(append<&ast::RepeatStmt::body>(ref<StatementRule>),
                  lit<";">) +
             ikw<"until"> +
             assign<&ast::RepeatStmt::cond>(ref<ExpressionRule>)> {};
  struct ForStmtRule
      : rule<ikw<"for"> + assign<&ast::ForStmt::variable>(ref<IDENT>) +
             ((lit<":="> + assign<&ast::ForStmt::from>(ref<ExpressionRule>) +
               (flag<&ast::ForStmt::downto>(ikw<"downto">) |
                ikw<"to">)+assign<&ast::ForStmt::to>(ref<ExpressionRule>)) |
              (ikw<"in"> +
               assign<&ast::ForStmt::iterable>(ref<ExpressionRule>))) +
             ikw<"do"> + assign<&ast::ForStmt::body>(ref<StatementRule>)> {};
  struct WithStmtRule
      : rule<ikw<"with"> +
             some(append<&ast::WithStmt::records>(ref<ExpressionRule>),
                  lit<",">) +
             ikw<"do"> + assign<&ast::WithStmt::body>(ref<StatementRule>)> {};
  struct GotoStmtRule : rule<ikw<"goto"> + assign<&ast::GotoStmt::target>(
                                               ref<NUM_INT> | ref<IDENT>)> {};
  struct UnlabelledStatementRule
      : rule<ref<CompoundStmtRule> | ref<IfStmtRule> | ref<CaseStmtRule> |
             ref<WhileStmtRule> | ref<RepeatStmtRule> | ref<ForStmtRule> |
             ref<WithStmtRule> | ref<GotoStmtRule> | ref<SimpleStatementRule> |
             create<ast::EmptyStmt>> {};
  struct StatementRule
      : rule<create<ast::LabeledStmt>(
                 assign<&ast::LabeledStmt::label>(ref<NUM_INT> | ref<IDENT>) +
                 lit<":"> +
                 assign<&ast::LabeledStmt::body>(
                     ref<UnlabelledStatementRule>)) |
             ref<UnlabelledStatementRule>> {};

  struct NamedTypeRule
      : rule<some(append<&ast::NamedType::names>(ref<IDENT>), lit<".">)> {};
  struct EnumTypeRule
      : rule<lit<"("> +
             some(append<&ast::EnumType::values>(ref<IDENT>), lit<",">) +
             lit<")">> {};
  struct SubrangeTypeRule
      : rule<assign<&ast::SubrangeType::low>(ref<ExpressionRule>) + lit<".."> +
             assign<&ast::SubrangeType::high>(ref<ExpressionRule>)> {};
  struct StringTypeRule
      : rule<ikw<"string"> +
             option(lit<"["> +
                    assign<&ast::StringType::size>(ref<ExpressionRule>) +
                    lit<"]">)> {};
  struct SimpleTypeRule : rule<ref<SubrangeTypeRule> | ref<EnumTypeRule> |
                               ref<StringTypeRule> | ref<NamedTypeRule>> {};
  struct ArrayTypeRule
      : rule<option(flag<&ast::ArrayType::packed>(ikw<"packed">)) +
             ikw<"array"> +
             option((lit<"["> | lit<"(.">)+some(
                        append<&ast::ArrayType::indices>(ref<SimpleTypeRule>),
                        lit<",">) +
                    (lit<"]"> | lit<".)">)) +
             ikw<"of"> + assign<&ast::ArrayType::component>(ref<TypeRule>)> {};
  struct RecordFieldRule
      : rule<some(append<&ast::RecordField::names>(ref<IDENT>), lit<",">) +
             lit<":"> + assign<&ast::RecordField::type>(ref<TypeRule>)> {};
  template <auto Fields, auto Variant>
  static constexpr auto FieldsAndVariant =
      option(some(append<Fields>(ref<RecordFieldRule>), lit<";">) +
             option(lit<";">)) +
      option(assign<Variant>(ref<VariantPartRule>) + option(lit<";">));
  struct VariantCaseRule
      : rule<some(append<&ast::VariantCase::labels>(ref<SetElementRule>),
                  lit<",">) +
             lit<":"> + lit<"("> +
             FieldsAndVariant<&ast::VariantCase::fields,
                              &ast::VariantCase::variant> +
             lit<")">> {};
  struct VariantPartRule
      : rule<ikw<"case"> +
             option(assign<&ast::VariantPart::tagName>(ref<IDENT>) + lit<":">) +
             assign<&ast::VariantPart::tagType>(ref<NamedTypeRule>) +
             ikw<"of"> +
             some(append<&ast::VariantPart::cases>(ref<VariantCaseRule>),
                  lit<";">)> {};
  struct RecordTypeRule
      : rule<option(flag<&ast::RecordType::packed>(ikw<"packed">)) +
             ikw<"record"> +
             FieldsAndVariant<&ast::RecordType::fields,
                              &ast::RecordType::variant> +
             ikw<"end">> {};
  struct SetTypeRule
      : rule<option(flag<&ast::SetType::packed>(ikw<"packed">)) +
             ikw<"set"> + ikw<"of"> +
             assign<&ast::SetType::base>(ref<SimpleTypeRule>)> {};
  struct FileTypeRule
      : rule<option(flag<&ast::FileType::packed>(ikw<"packed">)) +
             ikw<"file"> +
             option(ikw<"of"> +
                    assign<&ast::FileType::component>(ref<TypeRule>))> {};
  struct PointerTypeRule
      : rule<lit<"^"> + assign<&ast::PointerType::target>(ref<TypeRule>)> {};
  template <auto Member>
  static constexpr auto Params =
      option(lit<"("> + option(some(append<Member>(ref<ParamRule>), lit<";">)) +
             lit<")">);
  template <class T>
  static constexpr auto ProcHead =
      ikw<"procedure"> + assign<&T::name>(ref<IDENT>) + Params<&T::params>;
  template <class T>
  static constexpr auto FuncHead =
      ikw<"function"> + assign<&T::name>(ref<IDENT>) + Params<&T::params> +
      lit<":"> + assign<&T::resultType>(ref<TypeRule>);
  struct ProcTypeRule
      : rule<(ikw<"procedure"> + Params<&ast::ProcType::params>) |
             (ikw<"function"> + Params<&ast::ProcType::params> + lit<":"> +
              assign<&ast::ProcType::result>(ref<TypeRule>))> {};
  struct ParamRule
      : rule<create<ast::Param>(ikw<"procedure"> +
                                append<&ast::Param::names>(ref<IDENT>) +
                                assign<&ast::Param::type>(create<ast::ProcType>(
                                    Params<&ast::ProcType::params>))) |
             create<ast::Param>(
                 ikw<"function"> + append<&ast::Param::names>(ref<IDENT>) +
                 assign<&ast::Param::type>(create<ast::ProcType>(
                     Params<&ast::ProcType::params> + lit<":"> +
                     assign<&ast::ProcType::result>(ref<TypeRule>)))) |
             create<ast::Param>(
                 option(assign<&ast::Param::kind>(ikw<"var"> | ikw<"const"> |
                                                  ikw<"out"> |
                                                  ikw<"constref">)) +
                 some(append<&ast::Param::names>(ref<IDENT>), lit<",">) +
                 option(lit<":"> + assign<&ast::Param::type>(ref<TypeRule>)) +
                 option(lit<"="> + assign<&ast::Param::defaultValue>(
                                       ref<ExpressionRule>)))> {};
  struct TypeRule
      : rule<ref<SimpleTypeRule> | ref<RecordTypeRule> | ref<ArrayTypeRule> |
             ref<SetTypeRule> | ref<FileTypeRule> | ref<PointerTypeRule> |
             ref<ProcTypeRule>> {};

  struct ConstDeclRule
      : rule<assign<&ast::ConstDecl::name>(ref<IDENT>) +
             option(lit<":"> + assign<&ast::ConstDecl::type>(ref<TypeRule>)) +
             lit<"="> + assign<&ast::ConstDecl::value>(ref<ExpressionRule>)> {};
  struct TypeDeclRule
      : rule<assign<&ast::TypeDecl::name>(ref<IDENT>) + lit<"="> +
             assign<&ast::TypeDecl::type>(ref<TypeRule>)> {};
  struct VarDeclRule
      : rule<some(append<&ast::VarDecl::names>(ref<IDENT>), lit<",">) +
             lit<":"> + assign<&ast::VarDecl::type>(ref<TypeRule>) +
             option(lit<"="> +
                    assign<&ast::VarDecl::value>(ref<ExpressionRule>))> {};
  struct LabelDeclRule
      : rule<ikw<"label"> +
             some(append<&ast::LabelDecl::labels>(ref<NUM_INT> | ref<IDENT>),
                  lit<",">) +
             lit<";">> {};
  struct UsesItemRule
      : rule<some(append<&ast::UsesItem::name>(ref<IDENT>), lit<".">) +
             option(ikw<"in"> +
                    assign<&ast::UsesItem::sourcePath>(ref<PATH_STRING>))> {};
  struct UsesDeclRule
      : rule<ikw<"uses"> +
             some(append<&ast::UsesDecl::units>(ref<UsesItemRule>), lit<",">) +
             lit<";">> {};
  static constexpr auto RoutineDirective =
      ikw<"cdecl"> | ikw<"stdcall"> | ikw<"pascal"> | ikw<"register"> |
      ikw<"safecall"> | ikw<"inline"> | ikw<"overload">;
  template <class T>
  static constexpr auto Directives =
      many(append<&T::directives>(RoutineDirective) + lit<";">);
  struct ProcDeclRule
      : rule<ProcHead<ast::ProcDecl> + lit<";"> + Directives<ast::ProcDecl> +
             ((assign<&ast::ProcDecl::body>(ref<BlockRule>) + lit<";">) |
              (append<&ast::ProcDecl::directives>(ikw<"forward"> |
                                                  ikw<"external">) +
               lit<";">))> {};
  struct FuncDeclRule
      : rule<FuncHead<ast::FuncDecl> + lit<";"> + Directives<ast::FuncDecl> +
             ((assign<&ast::FuncDecl::body>(ref<BlockRule>) + lit<";">) |
              (append<&ast::FuncDecl::directives>(ikw<"forward"> |
                                                  ikw<"external">) +
               lit<";">))> {};
  struct InterfaceProcRule
      : rule<ProcHead<ast::ProcDecl> + lit<";"> + Directives<ast::ProcDecl>> {};
  struct InterfaceFuncRule
      : rule<FuncHead<ast::FuncDecl> + lit<";"> + Directives<ast::FuncDecl>> {};
  template <auto Member>
  static constexpr auto Declarations = many(
      append<Member>(ref<LabelDeclRule> | ref<UsesDeclRule>) |
      (ikw<"const"> + some(append<Member>(ref<ConstDeclRule>) + lit<";">)) |
      (ikw<"type"> + some(append<Member>(ref<TypeDeclRule>) + lit<";">)) |
      (ikw<"var"> + some(append<Member>(ref<VarDeclRule>) + lit<";">)));
  template <auto Member>
  static constexpr auto BodyDeclarations =
      many(Declarations<Member> +
           append<Member>(ref<ProcDeclRule> | ref<FuncDeclRule>)) +
      Declarations<Member>;
  struct BlockRule : rule<BodyDeclarations<&ast::Block::decls> +
                          assign<&ast::Block::body>(ref<CompoundStmtRule>)> {};
  struct ProgramRule
      : rule<option((ikw<"program"> |
                     flag<&ast::Program::library>(ikw<"library">)) +
                    assign<&ast::Program::name>(ref<IDENT>) +
                    option(lit<"("> +
                           some(append<&ast::Program::params>(ref<IDENT>),
                                lit<",">) +
                           lit<")">) +
                    lit<";">) +
             assign<&ast::Program::block>(ref<BlockRule>) + lit<".">> {};
  struct UnitRule
      : rule<ikw<"unit"> +
             some(append<&ast::Unit::name>(ref<IDENT>), lit<".">) + lit<";"> +
             ikw<"interface"> +
             many(Declarations<&ast::Unit::interfaceDecls> +
                  append<&ast::Unit::interfaceDecls>(ref<InterfaceProcRule> |
                                                     ref<InterfaceFuncRule>)) +
             Declarations<&ast::Unit::interfaceDecls> + ikw<"implementation"> +
             BodyDeclarations<&ast::Unit::implementationDecls> +
             ((ikw<"begin"> +
               some(append<&ast::Unit::initialization>(ref<StatementRule>),
                    lit<";">)) |
              (option(
                   ikw<"initialization"> +
                   some(append<&ast::Unit::initialization>(ref<StatementRule>),
                        lit<";">)) +
               option(ikw<"finalization"> +
                      some(append<&ast::Unit::finalization>(ref<StatementRule>),
                           lit<";">)))) +
             ikw<"end"> + lit<".">> {};
  struct SourceFileRule : rule<ref<UnitRule> | ref<ProgramRule>> {};
  using entry = SourceFileRule;
  using ignore = WS;
  using hidden = list<COMMENT_STAR, COMMENT_CURLY, COMMENT_LINE>;
};

}  // namespace pascal
