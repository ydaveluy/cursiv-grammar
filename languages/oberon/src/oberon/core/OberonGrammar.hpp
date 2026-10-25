// The oberon language: its grammar. Case-sensitive uppercase keywords, one
// LR precedence table folding relation/add/mul, nested `(* *)` comments.
#pragma once

#include <charconv>
#include <cstdint>
#include <memory_resource>
#include <span>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "oberon/core/ast.hpp"

namespace oberon {

struct Oberon : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct COMMENT
      : terminal<lit<"(*"> +
                 many(ref<COMMENT> | (!lit<"(*"> + !lit<"*)"> + any)) +
                 lit<"*)">> {};

  // A keyword and a name end at the same byte: `_` ends neither.
  static constexpr auto word_boundary = cr<"a-zA-Z0-9">;

  struct IDENT : terminal<cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9">)> {};

  static constexpr auto parseInt = [](std::string_view t) -> std::int64_t {
    std::int64_t v = 0;
    if (!t.empty() && t.back() == 'H') {
      std::from_chars(t.data(), t.data() + t.size() - 1, v, 16);
    } else {
      std::from_chars(t.data(), t.data() + t.size(), v, 10);
    }
    return v;
  };
  using IntConv = cursiv::text_conv<parseInt>;
  struct INTEGER
      : terminal<some(d) | (d + many(cr<"0-9A-F">) + lit<"H">), IntConv> {};

  struct REAL
      : terminal<some(d) + lit<".", !lit<".">> + many(d) +
                     option(lit<"E"> + option(cr<"+-">) + some(d)),
                 double> {};

  static constexpr auto decodeString =
      [](std::string_view t, std::pmr::memory_resource* mem) {
        if (t.back() == 'X') {
          std::uint8_t code = 0;
          std::from_chars(t.data(), t.data() + t.size() - 1, code, 16);
          const std::span<char> room = cursiv::text_room(mem, 1);
          room[0] = static_cast<char>(code);
          return std::string_view(room.data(), 1);
        }
        return t.substr(1, t.size() - 2);
      };
  using StringConv = cursiv::text_conv<decodeString>;
  struct STRING
      : terminal<through(lit<"\"">, lit<"\"">) |
                     (d + many(cr<"0-9A-F">) + lit<"X">),
                 StringConv> {};

  // identdef: an identifier with its optional export marker, kept in the
  // text -- this port does not resolve names, so there is nothing else to
  // do with the marker.
  struct IdentDefRule : rule<ref<IDENT> + option(lit<"*">)> {};

  // qualident: at most one module qualifier, kept as one span of text.
  struct QualidentRule : rule<option(ref<IDENT> + lit<".">) + ref<IDENT>> {};

  static constexpr auto MulOp =
      lit<"*"> | lit<"/"> | kw<"DIV"> | kw<"MOD"> | lit<"&">;
  static constexpr auto AddOp = lit<"+"> | lit<"-"> | kw<"OR">;
  static constexpr auto RelOp = lit<"<="> | lit<">="> | lit<"="> | lit<"#"> |
                                lit<"<"> | lit<">"> | kw<"IN"> | kw<"IS">;

  struct ExpressionRule;
  struct FactorRule;
  struct DesignatorRule;
  struct DesignatorCallRule;
  struct TypeRule;
  struct StatementRule;
  struct FormalParametersRule;

  // -- designators and factors ----

  // A postfix chain over a qualident: indexing, field access, dereference,
  // type guard -- each `nest<>` wraps the node built so far, so `a[i].b^`
  // reads left to right into DerefExpr(FieldExpr(IndexExpr(...))).
  struct DesignatorRule
      : rule<assign<&ast::NameExpr::name>(ref<QualidentRule>) +
             many(
                 nest<ast::IndexExpr, &ast::IndexExpr::array>(
                     lit<"["> +
                     some(append<&ast::IndexExpr::indices>(ref<ExpressionRule>),
                          lit<",">) +
                     lit<"]">)  //
                 |
                 nest<ast::FieldExpr, &ast::FieldExpr::object>(
                     lit<"."> + assign<&ast::FieldExpr::field>(ref<IDENT>))  //
                 | nest<ast::DerefExpr, &ast::DerefExpr::operand>(lit<"^">)  //
                 | nest<ast::ApplyExpr, &ast::ApplyExpr::operand>(
                       lit<"("> +
                       many(append<&ast::ApplyExpr::args>(ref<ExpressionRule>),
                            lit<",">) +
                       lit<")">))> {};
  struct DesignatorCallRule : rule<ref<DesignatorRule>> {};

  struct IntLitRule : rule<assign<&ast::IntLit::value>(ref<INTEGER>)> {};
  struct RealLitRule : rule<assign<&ast::RealLit::value>(ref<REAL>)> {};
  struct StringLitRule
      : rule<assign<&ast::StringLit::value>(ref<STRING>)> {};
  struct NilLitRule : rule<create<ast::NilLit> + kw<"NIL">> {};
  struct BoolLitRule
      : rule<create<ast::BoolLit> +
             (flag<&ast::BoolLit::value>(kw<"TRUE">) |
              kw<"FALSE">)> {};

  struct SetElementRule
      : rule<create<ast::SetElement>(
            assign<&ast::SetElement::low>(ref<ExpressionRule>) +
            option(lit<".."> +
                   assign<&ast::SetElement::high>(ref<ExpressionRule>)))> {
  };
  struct SetLitRule
      : rule<create<ast::SetLit>(
            lit<"{"> +
            option(some(append<&ast::SetLit::elements>(ref<SetElementRule>),
                        lit<",">)) +
            lit<"}">)> {};

  struct FactorRule
      : rule<ref<IntLitRule>                                          //
             | ref<RealLitRule>                                       //
             | ref<StringLitRule>                                     //
             | ref<NilLitRule>                                        //
             | ref<BoolLitRule>                                       //
             | ref<SetLitRule>                                        //
             | ref<DesignatorCallRule>                                //
             | (lit<"("> + ref<ExpressionRule> + lit<")">)             //
             | create<ast::UnaryExpr>(
                   assign<&ast::UnaryExpr::op>(lit<"~">) +
                   assign<&ast::UnaryExpr::operand>(ref<FactorRule>))> {};

  // The sign of a `simpleExpression` binds the whole leading term, not any
  // factor: normalized here to one factor-level wrap, so a doubled sign
  // after an `addOperator` (`a + -b`) also parses -- an over-acceptance the
  // literal grammar refuses, priced nil.
  struct SignedFactorRule
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-">) +
                 assign<&ast::UnaryExpr::operand>(ref<FactorRule>))  //
             | ref<FactorRule>> {};

  // expression, simpleExpression and term collapse to one precedence table;
  // Oberon's own grammar allows at most one relation per expression, not a
  // chain; chaining it here is the same low-price normalization as the sign
  // above.
  struct ExpressionRule
      : infix<SignedFactorRule, &ast::BinaryExpr::left,
              &ast::BinaryExpr::op, &ast::BinaryExpr::right,
              (left(MulOp) >> left(AddOp) >> left(RelOp))> {};

  // -- statements ----

  struct AssignStmtRule
      : rule<create<ast::AssignStmt>(
            assign<&ast::AssignStmt::target>(ref<DesignatorRule>) +
            lit<":="> +
            assign<&ast::AssignStmt::value>(ref<ExpressionRule>))> {};

  struct ExprStmtRule
      : rule<create<ast::ExprStmt>(
            assign<&ast::ExprStmt::value>(ref<DesignatorCallRule>))> {};

  struct IfElsifRule
      : rule<create<ast::ElsifClause>(
            kw<"ELSIF"> + assign<&ast::ElsifClause::cond>(ref<ExpressionRule>) +
            kw<"THEN"> +
            some(append<&ast::ElsifClause::body>(ref<StatementRule>),
                 lit<";">))> {};

  struct IfStmtRule
      : rule<create<ast::IfStmt>(
            kw<"IF"> + assign<&ast::IfStmt::cond>(ref<ExpressionRule>) +
            kw<"THEN"> +
            some(append<&ast::IfStmt::thenBranch>(ref<StatementRule>),
                 lit<";">) +
            many(append<&ast::IfStmt::elsifs>(ref<IfElsifRule>)) +
            option(kw<"ELSE"> +
                   some(append<&ast::IfStmt::elseBranch>(ref<StatementRule>),
                        lit<";">)) +
            kw<"END">)> {};

  struct LabelRule
      : rule<ref<IntLitRule> | ref<StringLitRule> |
             assign<&ast::NameExpr::name>(ref<QualidentRule>)> {};
  struct LabelRangeRule
      : rule<create<ast::LabelRange>(
            assign<&ast::LabelRange::low>(ref<LabelRule>) +
            option(lit<".."> +
                   assign<&ast::LabelRange::high>(ref<LabelRule>)))> {};
  struct CaseArmRule
      : rule<create<ast::CaseArm>(option(
            some(append<&ast::CaseArm::labels>(ref<LabelRangeRule>),
                 lit<",">) +
            lit<":"> +
            some(append<&ast::CaseArm::body>(ref<StatementRule>),
                 lit<";">)))> {};
  struct CaseStmtRule
      : rule<create<ast::CaseStmt>(
            kw<"CASE"> + assign<&ast::CaseStmt::value>(ref<ExpressionRule>) +
            kw<"OF"> +
            some(append<&ast::CaseStmt::arms>(ref<CaseArmRule>), lit<"|">) +
            kw<"END">)> {};

  struct WhileElsifRule
      : rule<create<ast::ElsifClause>(
            kw<"ELSIF"> + assign<&ast::ElsifClause::cond>(ref<ExpressionRule>) +
            kw<"DO"> +
            some(append<&ast::ElsifClause::body>(ref<StatementRule>),
                 lit<";">))> {};
  struct WhileStmtRule
      : rule<create<ast::WhileStmt>(
            kw<"WHILE"> +
            assign<&ast::WhileStmt::cond>(ref<ExpressionRule>) + kw<"DO"> +
            some(append<&ast::WhileStmt::body>(ref<StatementRule>),
                 lit<";">) +
            many(append<&ast::WhileStmt::elsifs>(ref<WhileElsifRule>)) +
            kw<"END">)> {};

  struct RepeatStmtRule
      : rule<create<ast::RepeatStmt>(
            kw<"REPEAT"> +
            some(append<&ast::RepeatStmt::body>(ref<StatementRule>),
                 lit<";">) +
            kw<"UNTIL"> +
            assign<&ast::RepeatStmt::cond>(ref<ExpressionRule>))> {};

  struct ForStmtRule
      : rule<create<ast::ForStmt>(
            kw<"FOR"> + assign<&ast::ForStmt::variable>(ref<IDENT>) +
            lit<":="> + assign<&ast::ForStmt::from>(ref<ExpressionRule>) +
            kw<"TO"> + assign<&ast::ForStmt::to>(ref<ExpressionRule>) +
            option(kw<"BY"> +
                   assign<&ast::ForStmt::by>(ref<ExpressionRule>)) +
            kw<"DO"> +
            some(append<&ast::ForStmt::body>(ref<StatementRule>), lit<";">) +
            kw<"END">)> {};

  struct StatementRule
      : rule<ref<AssignStmtRule>  //
             | ref<ExprStmtRule>  //
             | ref<IfStmtRule>    //
             | ref<CaseStmtRule>  //
             | ref<WhileStmtRule>  //
             | ref<RepeatStmtRule>  //
             | ref<ForStmtRule>  //
             | create<ast::EmptyStmt>> {};

  // -- types ----

  struct NamedTypeRule
      : rule<assign<&ast::NamedType::name>(ref<QualidentRule>)> {};

  struct ArrayTypeRule
      : rule<create<ast::ArrayType>(
            kw<"ARRAY"> +
            some(append<&ast::ArrayType::lengths>(ref<ExpressionRule>),
                 lit<",">) +
            kw<"OF"> +
            assign<&ast::ArrayType::component>(ref<TypeRule>))> {};

  struct RecordFieldRule
      : rule<create<ast::RecordField>(
            some(append<&ast::RecordField::names>(ref<IdentDefRule>),
                 lit<",">) +
            lit<":"> + assign<&ast::RecordField::type>(ref<TypeRule>))> {};

  struct RecordTypeRule
      : rule<create<ast::RecordType>(
            kw<"RECORD"> +
            option(lit<"("> + assign<&ast::RecordType::base>(
                                   ref<QualidentRule>) +
                   lit<")">) +
            option(some(append<&ast::RecordType::fields>(
                            ref<RecordFieldRule>),
                        lit<";">)) +
            kw<"END">)> {};

  struct PointerTypeRule
      : rule<create<ast::PointerType>(
            kw<"POINTER"> + kw<"TO"> +
            assign<&ast::PointerType::target>(ref<TypeRule>))> {};

  // formalType: (ARRAY OF)* qualident -- a data rule, the whole spelling
  // kept as one span since this port does not resolve types.
  struct FormalTypeRule
      : rule<many(kw<"ARRAY"> + kw<"OF">) + ref<QualidentRule>> {};

  struct FPSectionRule
      : rule<create<ast::FormalParam>(
            option(flag<&ast::FormalParam::byRef>(kw<"VAR">)) +
            some(append<&ast::FormalParam::names>(ref<IDENT>),
                 lit<",">) +
            lit<":"> +
            assign<&ast::FormalParam::type>(ref<FormalTypeRule>))> {};

  struct FormalParametersRule
      : rule<create<ast::FormalParams>(
            lit<"("> +
            option(some(append<&ast::FormalParams::params>(
                            ref<FPSectionRule>),
                        lit<";">)) +
            lit<")"> +
            option(lit<":"> +
                   assign<&ast::FormalParams::resultType>(
                       ref<QualidentRule>)))> {};

  struct ProcedureTypeRule
      : rule<create<ast::ProcedureType>(
            kw<"PROCEDURE"> +
            option(assign<&ast::ProcedureType::params>(
                ref<FormalParametersRule>)))> {};

  struct TypeRule
      : rule<ref<NamedTypeRule> | ref<ArrayTypeRule> | ref<RecordTypeRule> |
             ref<PointerTypeRule> | ref<ProcedureTypeRule>> {};

  // -- declarations ----

  struct ConstDeclRule
      : rule<create<ast::ConstDecl>(
            assign<&ast::ConstDecl::name>(ref<IdentDefRule>) + lit<"="> +
            assign<&ast::ConstDecl::value>(ref<ExpressionRule>))> {};

  struct TypeDeclRule
      : rule<create<ast::TypeDecl>(
            assign<&ast::TypeDecl::name>(ref<IdentDefRule>) + lit<"="> +
            assign<&ast::TypeDecl::type>(ref<TypeRule>))> {};

  struct VarDeclRule
      : rule<create<ast::VarDecl>(
            some(append<&ast::VarDecl::names>(ref<IdentDefRule>), lit<",">) +
            lit<":"> + assign<&ast::VarDecl::type>(ref<TypeRule>))> {};

  struct ProcDeclRule
      : rule<create<ast::ProcDecl>(
            kw<"PROCEDURE"> +
            assign<&ast::ProcDecl::name>(ref<IdentDefRule>) +
            option(assign<&ast::ProcDecl::params>(
                ref<FormalParametersRule>)) +
            lit<";"> +
            option(kw<"CONST"> +
                   many(append<&ast::ProcDecl::decls>(ref<ConstDeclRule>) +
                        lit<";">)) +
            option(kw<"TYPE"> +
                   many(append<&ast::ProcDecl::decls>(ref<TypeDeclRule>) +
                        lit<";">)) +
            option(kw<"VAR"> +
                   many(append<&ast::ProcDecl::decls>(ref<VarDeclRule>) +
                        lit<";">)) +
            many(append<&ast::ProcDecl::decls>(ref<ProcDeclRule>) +
                 lit<";">) +
            option(kw<"BEGIN"> +
                   some(append<&ast::ProcDecl::body>(ref<StatementRule>),
                        lit<";">)) +
            option(kw<"RETURN"> +
                   assign<&ast::ProcDecl::returnExpr>(ref<ExpressionRule>)) +
            kw<"END"> +
            assign<&ast::ProcDecl::endName>(ref<IDENT>))> {};

  struct ImportRule
      : rule<create<ast::Import>(
            assign<&ast::Import::name>(ref<IDENT>) +
            option(lit<":="> +
                   assign<&ast::Import::actual>(ref<IDENT>)))> {};

  struct ModuleRule
      : rule<create<ast::Module>(
            kw<"MODULE"> + assign<&ast::Module::name>(ref<IDENT>) +
            lit<";"> +
            option(kw<"IMPORT"> +
                   some(append<&ast::Module::imports>(ref<ImportRule>),
                        lit<",">) +
                   lit<";">) +
            option(kw<"CONST"> +
                   many(append<&ast::Module::decls>(ref<ConstDeclRule>) +
                        lit<";">)) +
            option(kw<"TYPE"> +
                   many(append<&ast::Module::decls>(ref<TypeDeclRule>) +
                        lit<";">)) +
            option(kw<"VAR"> +
                   many(append<&ast::Module::decls>(ref<VarDeclRule>) +
                        lit<";">)) +
            many(append<&ast::Module::decls>(ref<ProcDeclRule>) + lit<";">) +
            option(kw<"BEGIN"> +
                   some(append<&ast::Module::body>(ref<StatementRule>),
                        lit<";">)) +
            kw<"END"> + assign<&ast::Module::endName>(ref<IDENT>) +
            lit<".">)> {};

  using entry = ModuleRule;
  using ignore = WS;
  using hidden = list<COMMENT>;
};

}  // namespace oberon
