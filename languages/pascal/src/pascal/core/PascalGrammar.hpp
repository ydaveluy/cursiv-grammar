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
  // Core reserved words in the retained FPC profile; directives remain identifiers.
  static constexpr auto Reserved =
      ikw<"and"> | ikw<"array"> | ikw<"asm"> | ikw<"begin"> | ikw<"case"> |
      ikw<"const"> | ikw<"div"> | ikw<"do"> | ikw<"downto"> | ikw<"else"> |
      ikw<"end"> | ikw<"file"> | ikw<"for"> | ikw<"function"> | ikw<"goto"> |
      ikw<"if"> | ikw<"in"> | ikw<"label"> | ikw<"mod"> | ikw<"nil"> |
      ikw<"not"> | ikw<"of"> | ikw<"or"> | ikw<"packed"> | ikw<"procedure"> |
      ikw<"program"> | ikw<"record"> | ikw<"repeat"> | ikw<"set"> | ikw<"shl"> |
      ikw<"shr"> | ikw<"then"> | ikw<"to"> | ikw<"type"> | ikw<"until"> |
      ikw<"var"> | ikw<"while"> | ikw<"with"> | ikw<"xor">;
  struct IDENT : terminal<as<identifier_text>(
      !Reserved + option(lit<"&">) + cr<"A-Za-z_"> + many(w))> {};
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
  struct PATH_STRING : terminal<as<unquote>(QUOTED)> {};

  struct Expression;
  struct Factor;
  struct Power;
  struct Postfix;
  struct ActualArg;
  struct SetElement;
  struct Statement;
  struct UnlabelledStatement;
  struct CompoundStmt;
  struct Type;
  struct SimpleType;
  struct NamedType;
  struct Param;
  struct ProcType;
  struct VariantPart;
  struct RecordField;
  struct ProcDecl;
  struct FuncDecl;
  struct InterfaceProc;
  struct InterfaceFunc;
  struct Block;

  struct NumberLit
      : rule<assign<&ast::NumberLit::spelling>(ref<NUM_REAL> | ref<NUM_INT>)> {
  };
  struct StringLit
      : rule<assign<&ast::StringLit::spelling>(ref<STRING_LITERAL>)> {};
  struct NameExpr : rule<assign<&ast::NameExpr::name>(ref<IDENT>)> {};
  struct NilLit : rule<create<ast::NilLit> + ikw<"nil">> {};
  struct ActualArg
      : rule<assign<&ast::ActualArg::value>(ref<Expression>) +
             many(lit<":"> +
                  append<&ast::ActualArg::widths>(ref<Expression>))> {};
  struct SetElement
      : rule<assign<&ast::SetElement::low>(ref<Expression>) +
             option(lit<".."> +
                    assign<&ast::SetElement::high>(ref<Expression>))> {};
  struct SetLit
      : rule<(lit<"["> | lit<"(.">)+many(append<&ast::SetLit::elements>(ref<SetElement>),
                      lit<",">) +
             (lit<"]"> | lit<".)">)> {};
  struct Postfix
      : rule<(ref<NameExpr> | ref<NumberLit> | ref<StringLit> |
              ref<NilLit> | ref<SetLit> |
              (lit<"("> + ref<Expression> + lit<")">)) +
             many(nest<ast::CallExpr, &ast::CallExpr::callee>(
                      lit<"("> +
                      many(append<&ast::CallExpr::args>(ref<ActualArg>),
                               lit<",">) +
                      lit<")">) |
                  nest<ast::IndexExpr, &ast::IndexExpr::array>(
                      (lit<"["> | lit<"(.">)+some(
                          append<&ast::IndexExpr::indices>(ref<Expression>),
                          lit<",">) +
                      (lit<"]"> | lit<".)">)) |
                  nest<ast::FieldExpr, &ast::FieldExpr::object>(
                      lit<"."> + assign<&ast::FieldExpr::field>(ref<IDENT>)) |
                  nest<ast::DerefExpr, &ast::DerefExpr::operand>(lit<"^">))> {};
  struct Factor
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"-">) +
                 assign<&ast::UnaryExpr::operand>(ref<Power>)) |
             create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"+"> | ikw<"not"> | lit<"@">) +
                 assign<&ast::UnaryExpr::operand>(ref<Factor>)) |
             ref<Postfix>> {};
  struct Power
      : infix<Factor, ast::BinaryExpr, left(lit<"**">)> {};
  static constexpr auto MulOp = lit<"*"> | lit<"/"> | ikw<"div"> | ikw<"mod"> |
                                ikw<"and"> | ikw<"shl"> | ikw<"shr"> |
                                ikw<"as"> | ikw<"is"> | lit<"><">;
  static constexpr auto AddOp = lit<"+"> | lit<"-"> | ikw<"or"> | ikw<"xor">;
  static constexpr auto RelOp = lit<"<>"> | lit<"<="> | lit<">="> | lit<"="> |
                                lit<"<"> | lit<">"> | ikw<"in">;
  struct Expression : infix<Power, ast::BinaryExpr,
                                levels(left(MulOp), left(AddOp), left(RelOp))> {
  };

  struct CompoundStmt
      : rule<ikw<"begin"> +
             some(append<&ast::CompoundStmt::statements>(ref<Statement>),
                  lit<";">) +
             ikw<"end">> {};
  struct SimpleStatement
      : rule<create<ast::AssignStmt>(
                 assign<&ast::AssignStmt::target>(ref<Expression>) +
                 lit<":="> +
                 assign<&ast::AssignStmt::value>(ref<Expression>)) |
             create<ast::ExprStmt>(
                 assign<&ast::ExprStmt::value>(ref<Expression>))> {};
  struct IfStmt
      : rule<ikw<"if"> + assign<&ast::IfStmt::cond>(ref<Expression>) +
             ikw<"then"> +
             assign<&ast::IfStmt::thenBranch>(ref<Statement>) +
             option(commit(ikw<"else">, assign<&ast::IfStmt::elseBranch>(ref<Statement>)))> {};
  struct CaseArm
      : rule<some(append<&ast::CaseArm::labels>(ref<SetElement>),
                  lit<",">) +
             lit<":"> + assign<&ast::CaseArm::body>(ref<Statement>)> {};
  struct CaseStmt
      : rule<ikw<"case"> + assign<&ast::CaseStmt::value>(ref<Expression>) +
             ikw<"of"> +
             many(append<&ast::CaseStmt::arms>(ref<CaseArm>),
                         lit<";">) +
             option(lit<";">) +
             option(ikw<"else"> +
                    some(append<&ast::CaseStmt::elseBody>(ref<Statement>),
                         lit<";">)) +
             ikw<"end">> {};
  struct WhileStmt
      : rule<ikw<"while"> + assign<&ast::WhileStmt::cond>(ref<Expression>) +
             ikw<"do"> + assign<&ast::WhileStmt::body>(ref<Statement>)> {};
  struct RepeatStmt
      : rule<ikw<"repeat"> +
             some(append<&ast::RepeatStmt::body>(ref<Statement>),
                  lit<";">) +
             ikw<"until"> +
             assign<&ast::RepeatStmt::cond>(ref<Expression>)> {};
  struct ForStmt
      : rule<ikw<"for"> + assign<&ast::ForStmt::variable>(ref<IDENT>) +
             ((lit<":="> + assign<&ast::ForStmt::from>(ref<Expression>) +
               (flag<&ast::ForStmt::downto>(ikw<"downto">) |
                ikw<"to">)+assign<&ast::ForStmt::to>(ref<Expression>)) |
              (ikw<"in"> +
               assign<&ast::ForStmt::iterable>(ref<Expression>))) +
             ikw<"do"> + assign<&ast::ForStmt::body>(ref<Statement>)> {};
  struct WithStmt
      : rule<ikw<"with"> +
             some(append<&ast::WithStmt::records>(ref<Expression>),
                  lit<",">) +
             ikw<"do"> + assign<&ast::WithStmt::body>(ref<Statement>)> {};
  struct GotoStmt : rule<ikw<"goto"> + assign<&ast::GotoStmt::target>(
                                               ref<NUM_INT> | ref<IDENT>)> {};
  struct UnlabelledStatement
      : rule<ref<CompoundStmt> | ref<IfStmt> | ref<CaseStmt> |
             ref<WhileStmt> | ref<RepeatStmt> | ref<ForStmt> |
             ref<WithStmt> | ref<GotoStmt> | ref<SimpleStatement> |
             create<ast::EmptyStmt>> {};
  struct Statement
      : rule<create<ast::LabeledStmt>(
                 assign<&ast::LabeledStmt::label>(ref<NUM_INT> | ref<IDENT>) +
                 lit<":"> +
                 assign<&ast::LabeledStmt::body>(
                     ref<UnlabelledStatement>)) |
             ref<UnlabelledStatement>> {};

  struct NamedType
      : rule<some(append<&ast::NamedType::names>(ref<IDENT>), lit<".">)> {};
  struct EnumType
      : rule<lit<"("> +
             some(append<&ast::EnumType::values>(ref<IDENT>), lit<",">) +
             lit<")">> {};
  struct SubrangeType
      : rule<assign<&ast::SubrangeType::low>(ref<Expression>) + lit<".."> +
             assign<&ast::SubrangeType::high>(ref<Expression>)> {};
  struct StringType
      : rule<ikw<"string"> +
             option(lit<"["> +
                    assign<&ast::StringType::size>(ref<Expression>) +
                    lit<"]">)> {};
  struct SimpleType : rule<ref<SubrangeType> | ref<EnumType> |
                               ref<StringType> | ref<NamedType>> {};
  struct ArrayType
      : rule<option(flag<&ast::ArrayType::packed>(ikw<"packed">)) +
             ikw<"array"> +
             option((lit<"["> | lit<"(.">)+some(
                        append<&ast::ArrayType::indices>(ref<SimpleType>),
                        lit<",">) +
                    (lit<"]"> | lit<".)">)) +
             ikw<"of"> + assign<&ast::ArrayType::component>(ref<Type>)> {};
  struct RecordField
      : rule<some(append<&ast::RecordField::names>(ref<IDENT>), lit<",">) +
             lit<":"> + assign<&ast::RecordField::type>(ref<Type>)> {};
  template <auto Fields, auto Variant>
  static constexpr auto FieldsAndVariant =
      option(some(append<Fields>(ref<RecordField>), lit<";">) +
             option(lit<";">)) +
      option(assign<Variant>(ref<VariantPart>) + option(lit<";">));
  struct VariantCase
      : rule<some(append<&ast::VariantCase::labels>(ref<SetElement>),
                  lit<",">) +
             lit<":"> + lit<"("> +
             FieldsAndVariant<&ast::VariantCase::fields,
                              &ast::VariantCase::variant> +
             lit<")">> {};
  struct VariantPart
      : rule<ikw<"case"> +
             option(assign<&ast::VariantPart::tagName>(ref<IDENT>) + lit<":">) +
             assign<&ast::VariantPart::tagType>(ref<NamedType>) +
             ikw<"of"> +
             some(append<&ast::VariantPart::cases>(ref<VariantCase>),
                  lit<";">)> {};
  struct RecordType
      : rule<option(flag<&ast::RecordType::packed>(ikw<"packed">)) +
             ikw<"record"> +
             FieldsAndVariant<&ast::RecordType::fields,
                              &ast::RecordType::variant> +
             ikw<"end">> {};
  struct SetType
      : rule<option(flag<&ast::SetType::packed>(ikw<"packed">)) +
             ikw<"set"> + ikw<"of"> +
             assign<&ast::SetType::base>(ref<SimpleType>)> {};
  struct FileType
      : rule<option(flag<&ast::FileType::packed>(ikw<"packed">)) +
             ikw<"file"> +
             option(ikw<"of"> +
                    assign<&ast::FileType::component>(ref<Type>))> {};
  struct PointerType
      : rule<lit<"^"> + assign<&ast::PointerType::target>(ref<Type>)> {};
  template <auto Member>
  static constexpr auto Params =
      option(lit<"("> + many(append<Member>(ref<Param>), lit<";">) +
             lit<")">);
  template <class T>
  static constexpr auto ProcHead =
      ikw<"procedure"> + assign<&T::name>(ref<IDENT>) + Params<&T::params>;
  template <class T>
  static constexpr auto FuncHead =
      ikw<"function"> + assign<&T::name>(ref<IDENT>) + Params<&T::params> +
      lit<":"> + assign<&T::resultType>(ref<Type>);
  struct ProcType
      : rule<(ikw<"procedure"> + Params<&ast::ProcType::params>) |
             (ikw<"function"> + Params<&ast::ProcType::params> + lit<":"> +
              assign<&ast::ProcType::result>(ref<Type>))> {};
  struct Param
      : rule<create<ast::Param>(ikw<"procedure"> +
                                append<&ast::Param::names>(ref<IDENT>) +
                                assign<&ast::Param::type>(create<ast::ProcType>(
                                    Params<&ast::ProcType::params>))) |
             create<ast::Param>(
                 ikw<"function"> + append<&ast::Param::names>(ref<IDENT>) +
                 assign<&ast::Param::type>(create<ast::ProcType>(
                     Params<&ast::ProcType::params> + lit<":"> +
                     assign<&ast::ProcType::result>(ref<Type>)))) |
             create<ast::Param>(
                 option(assign<&ast::Param::kind>(ikw<"var"> | ikw<"const"> |
                                                  ikw<"out"> |
                                                  ikw<"constref">)) +
                 some(append<&ast::Param::names>(ref<IDENT>), lit<",">) +
                 option(lit<":"> + assign<&ast::Param::type>(ref<Type>)) +
                 option(lit<"="> + assign<&ast::Param::defaultValue>(
                                       ref<Expression>)))> {};
  struct Type
      : rule<ref<SimpleType> | ref<RecordType> | ref<ArrayType> |
             ref<SetType> | ref<FileType> | ref<PointerType> |
             ref<ProcType>> {};

  struct ConstDecl
      : rule<assign<&ast::ConstDecl::name>(ref<IDENT>) +
             option(lit<":"> + assign<&ast::ConstDecl::type>(ref<Type>)) +
             lit<"="> + assign<&ast::ConstDecl::value>(ref<Expression>)> {};
  struct TypeDecl
      : rule<assign<&ast::TypeDecl::name>(ref<IDENT>) + lit<"="> +
             assign<&ast::TypeDecl::type>(ref<Type>)> {};
  struct VarDecl
      : rule<some(append<&ast::VarDecl::names>(ref<IDENT>), lit<",">) +
             lit<":"> + assign<&ast::VarDecl::type>(ref<Type>) +
             option(lit<"="> +
                    assign<&ast::VarDecl::value>(ref<Expression>))> {};
  struct LabelDecl
      : rule<ikw<"label"> +
             some(append<&ast::LabelDecl::labels>(ref<NUM_INT> | ref<IDENT>),
                  lit<",">) +
             lit<";">> {};
  struct UsesItem
      : rule<some(append<&ast::UsesItem::name>(ref<IDENT>), lit<".">) +
             option(ikw<"in"> +
                    assign<&ast::UsesItem::sourcePath>(ref<PATH_STRING>))> {};
  struct UsesDecl
      : rule<ikw<"uses"> +
             some(append<&ast::UsesDecl::units>(ref<UsesItem>), lit<",">) +
             lit<";">> {};
  static constexpr auto RoutineDirective =
      ikw<"cdecl"> | ikw<"stdcall"> | ikw<"pascal"> | ikw<"register"> |
      ikw<"safecall"> | ikw<"inline"> | ikw<"overload">;
  template <class T>
  static constexpr auto Directives =
      many(append<&T::directives>(RoutineDirective) + lit<";">);
  struct ProcDecl
      : rule<ProcHead<ast::ProcDecl> + lit<";"> + Directives<ast::ProcDecl> +
             ((assign<&ast::ProcDecl::body>(ref<Block>) + lit<";">) |
              (append<&ast::ProcDecl::directives>(ikw<"forward"> |
                                                  ikw<"external">) +
               lit<";">))> {};
  struct FuncDecl
      : rule<FuncHead<ast::FuncDecl> + lit<";"> + Directives<ast::FuncDecl> +
             ((assign<&ast::FuncDecl::body>(ref<Block>) + lit<";">) |
              (append<&ast::FuncDecl::directives>(ikw<"forward"> |
                                                  ikw<"external">) +
               lit<";">))> {};
  struct InterfaceProc
      : rule<ProcHead<ast::ProcDecl> + lit<";"> + Directives<ast::ProcDecl>> {};
  struct InterfaceFunc
      : rule<FuncHead<ast::FuncDecl> + lit<";"> + Directives<ast::FuncDecl>> {};
  template <auto Member>
  static constexpr auto Declarations = many(
      append<Member>(ref<LabelDecl> | ref<UsesDecl>) |
      (ikw<"const"> + some(append<Member>(ref<ConstDecl>) + lit<";">)) |
      (ikw<"type"> + some(append<Member>(ref<TypeDecl>) + lit<";">)) |
      (ikw<"var"> + some(append<Member>(ref<VarDecl>) + lit<";">)));
  template <auto Member>
  static constexpr auto BodyDeclarations =
      many(Declarations<Member> +
           append<Member>(ref<ProcDecl> | ref<FuncDecl>)) +
      Declarations<Member>;
  struct Block : rule<BodyDeclarations<&ast::Block::decls> +
                          assign<&ast::Block::body>(ref<CompoundStmt>)> {};
  struct Program
      : rule<option((ikw<"program"> |
                     flag<&ast::Program::library>(ikw<"library">)) +
                    assign<&ast::Program::name>(ref<IDENT>) +
                    option(lit<"("> +
                           some(append<&ast::Program::params>(ref<IDENT>),
                                lit<",">) +
                           lit<")">) +
                    lit<";">) +
             assign<&ast::Program::block>(ref<Block>) + lit<".">> {};
  struct Unit
      : rule<ikw<"unit"> +
             some(append<&ast::Unit::name>(ref<IDENT>), lit<".">) + lit<";"> +
             ikw<"interface"> +
             many(Declarations<&ast::Unit::interfaceDecls> +
                  append<&ast::Unit::interfaceDecls>(ref<InterfaceProc> |
                                                     ref<InterfaceFunc>)) +
             Declarations<&ast::Unit::interfaceDecls> + ikw<"implementation"> +
             BodyDeclarations<&ast::Unit::implementationDecls> +
             ((ikw<"begin"> +
               some(append<&ast::Unit::initialization>(ref<Statement>),
                    lit<";">)) |
              (option(
                   ikw<"initialization"> +
                   some(append<&ast::Unit::initialization>(ref<Statement>),
                        lit<";">)) +
               option(ikw<"finalization"> +
                      some(append<&ast::Unit::finalization>(ref<Statement>),
                           lit<";">)))) +
             ikw<"end"> + lit<".">> {};
  struct SourceFile : rule<ref<Unit> | ref<Program>> {};
  using entry = SourceFile;
  using ignored = WS;
  using hidden = list<COMMENT_STAR, COMMENT_CURLY, COMMENT_LINE>;
};

}  // namespace pascal
