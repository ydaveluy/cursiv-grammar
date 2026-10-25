// The modula2pim4 language: its grammar. PIM (4th edition) Modula-2:
// case-sensitive uppercase keywords, one LR precedence table folding
// relation/add/mul (shared by constant and runtime expressions), nested
// `(* *)` comments.
#pragma once

#include <charconv>
#include <cstdint>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "modula2pim4/core/ast.hpp"

namespace modula2pim4 {

struct Modula2pim4 : cursiv::GrammarSpec {
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
    if (t.back() == 'H') {
      std::from_chars(t.data(), t.data() + t.size() - 1, v, 16);
    } else if (t.back() == 'B' || t.back() == 'C') {
      std::from_chars(t.data(), t.data() + t.size() - 1, v, 8);
    } else {
      std::from_chars(t.data(), t.data() + t.size(), v, 10);
    }
    return v;
  };
  using IntConv = cursiv::text_conv<parseInt>;
  struct INTEGER
      : terminal<(some(cr<"0-7">) + (lit<"B"> | lit<"C">))  //
                     | (d + many(cr<"0-9A-F">) + lit<"H">)  //
                     | some(d),
                 IntConv> {};

  // The dot is guarded so a subrange's `..` (`[1..5]`) is not eaten as the
  // start of a real literal.
  struct REAL
      : terminal<some(d) + lit<".", !lit<".">> + many(d) +
                     option(lit<"E"> + option(cr<"+-">) + some(d)),
                 double> {};

  // Every printable byte but the two quotes: Modula-2 has no reason to
  // refuse a slash in a string.
  static constexpr auto StringByte = cr<" !#-&(-~">;
  static constexpr auto unquote = [](std::string_view t) {
    return t.substr(1, t.size() - 2);
  };
  using StringConv = cursiv::text_conv<unquote>;
  struct STRING
      : terminal<(lit<"'"> + many(StringByte | lit<"\"">) + lit<"'">)  //
                     | (lit<"\""> + many(StringByte | lit<"'">) +
                        lit<"\"">),
                 StringConv> {};

  // A dotted chain of names, kept as one span of text: a module qualifier
  // (`Module.Ident`) and, absent any preceding index/deref, a plain
  // record-field chain both read through here -- a designator's postfix
  // tail only opens on `[` or `^`.
  struct QualidentRule : rule<ref<IDENT> + many(lit<"."> + ref<IDENT>)> {};

  static constexpr auto MulOp =
      lit<"*"> | lit<"/"> | kw<"DIV"> | kw<"MOD"> | kw<"AND"> | lit<"&">;
  static constexpr auto AddOp = lit<"+"> | lit<"-"> | kw<"OR">;
  static constexpr auto RelOp = lit<"="> | lit<"#"> | lit<"<>"> | lit<"<="> |
                                lit<"<"> | lit<">="> | lit<">"> | kw<"IN">;

  struct ExpressionRule;
  struct ConstExpressionRule;
  struct FactorRule;
  struct ConstFactorRule;
  struct DesignatorRule;
  struct StatementRule;
  struct TypeRule;
  struct SimpleTypeRule;
  struct FieldItemRule;
  struct CaseFieldRule;
  struct VariantRule;
  struct BlockRule;
  struct ModuleDeclRule;
  struct ProcDeclRule;

  // -- literals and sets, shared by constant and runtime expressions ----

  struct IntLitRule : rule<assign<&ast::IntLit::value>(ref<INTEGER>)> {};
  struct RealLitRule : rule<assign<&ast::RealLit::value>(ref<REAL>)> {};
  struct StringLitRule
      : rule<assign<&ast::StringLit::value>(ref<STRING>)> {};

  // A set element's bounds are always a constant expression, even inside a
  // runtime `factor`'s set literal, so one rule serves both.
  struct SetElementRule
      : rule<create<ast::SetElement>(
            assign<&ast::SetElement::low>(ref<ConstExpressionRule>) +
            option(lit<".."> +
                   assign<&ast::SetElement::high>(ref<ConstExpressionRule>)))> {
  };
  static constexpr auto SetBody =
      lit<"{"> +
      option(some(append<&ast::SetLit::elements>(ref<SetElementRule>),
                  lit<",">)) +
      lit<"}">;
  struct SetLitRule : rule<create<ast::SetLit> + SetBody> {};

  // -- designators (postfix chains) ----

  static constexpr auto IndexPostfix =
      nest<ast::IndexExpr, &ast::IndexExpr::array>(
          lit<"["> +
          some(append<&ast::IndexExpr::indices>(ref<ExpressionRule>),
               lit<",">) +
          lit<"]">);
  static constexpr auto DerefPostfix =
      nest<ast::DerefExpr, &ast::DerefExpr::operand>(lit<"^">);
  static constexpr auto FieldPostfix =
      nest<ast::FieldExpr, &ast::FieldExpr::object>(
          lit<"."> + assign<&ast::FieldExpr::field>(ref<IDENT>));
  // designatorTail: one or more (index-or-deref, then any run of field
  // accesses) -- equivalently, a run that must OPEN on an index or a deref.
  static constexpr auto DesignatorTailBody =
      (IndexPostfix | DerefPostfix) +
      many(IndexPostfix | DerefPostfix | FieldPostfix);

  struct DesignatorRule
      : rule<assign<&ast::NameExpr::name>(ref<QualidentRule>) +
             option(DesignatorTailBody)> {};

  static constexpr auto CallBody =
      lit<"("> +
      option(some(append<&ast::CallExpr::args>(ref<ExpressionRule>),
                  lit<",">)) +
      lit<")">;

  // designator (actualParameters)? -- shared by an expression-context call
  // and a statement-context one.
  struct DesignatorCallRule
      : rule<ref<DesignatorRule> +
             option(nest<ast::CallExpr, &ast::CallExpr::callee>(
                 CallBody))> {};

  // A qualident followed by a mandatory set literal -- the typed-set half
  // of setOrQualident/setOrDesignatorOrProcCall. A bare qualident (no set)
  // is disjoint by lookahead (only `{` continues into this branch), so the
  // two halves never overlap despite each reading a qualident. `nest<>`
  // needs an optional context to adopt through, so the base name is its own
  // `create<>` here instead.
  struct QualSetRule
      : rule<create<ast::SetLit>(
            assign<&ast::SetLit::baseType>(create<ast::NameExpr>(
                assign<&ast::NameExpr::name>(ref<QualidentRule>))) +
            SetBody)> {};

  // constFactor's setOrQualident: a bare set, a qualident with a trailing
  // typed set, or a bare qualident naming a constant.
  struct ConstFactorRule
      : rule<ref<IntLitRule>                                            //
             | ref<RealLitRule>                                         //
             | ref<StringLitRule>                                       //
             | ref<SetLitRule>                                          //
             | ref<QualSetRule>                                         //
             | assign<&ast::NameExpr::name>(ref<QualidentRule>)          //
             | (lit<"("> + ref<ConstExpressionRule> + lit<")">)          //
             | create<ast::UnaryExpr>(
                   assign<&ast::UnaryExpr::op>(kw<"NOT"> | lit<"~">) +
                   assign<&ast::UnaryExpr::operand>(ref<ConstFactorRule>))> {
  };

  // factor's setOrDesignatorOrProcCall: a bare set, a qualident with a
  // trailing typed set, or a designator with an optional call -- disjoint
  // from the typed-set form by the same lookahead as ConstFactorRule.
  struct FactorRule
      : rule<ref<IntLitRule>                                            //
             | ref<RealLitRule>                                         //
             | ref<StringLitRule>                                       //
             | ref<SetLitRule>                                          //
             | ref<QualSetRule>                                         //
             | ref<DesignatorCallRule>                                  //
             | (lit<"("> + ref<ExpressionRule> + lit<")">)               //
             | create<ast::UnaryExpr>(
                   assign<&ast::UnaryExpr::op>(kw<"NOT"> | lit<"~">) +
                   assign<&ast::UnaryExpr::operand>(ref<FactorRule>))> {};

  // The leading sign of a simpleConstExpr/simpleExpression binds the whole
  // term; folded into one factor-level wrap, so `a + -b` also parses -- an
  // over-acceptance the literal grammar refuses, priced nil.
  struct SignedConstFactorRule
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-">) +
                 assign<&ast::UnaryExpr::operand>(ref<ConstFactorRule>))  //
             | ref<ConstFactorRule>> {};
  struct SignedFactorRule
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-">) +
                 assign<&ast::UnaryExpr::operand>(ref<FactorRule>))  //
             | ref<FactorRule>> {};

  // Constant and runtime expressions each collapse relation/add/mul into
  // one precedence table, so a relation may chain here (`a < b < c`);
  // Modula-2 allows at most one -- the same low-price normalization as the
  // sign above.
  struct ConstExpressionRule
      : infix<SignedConstFactorRule, &ast::BinaryExpr::left,
              &ast::BinaryExpr::op, &ast::BinaryExpr::right,
              (left(MulOp) >> left(AddOp) >> left(RelOp))> {};
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

  struct CallStmtRule
      : rule<create<ast::CallStmt>(
            assign<&ast::CallStmt::value>(ref<DesignatorCallRule>))> {};

  struct IfElsifRule
      : rule<create<ast::ElsifClause>(
            kw<"ELSIF"> +
            assign<&ast::ElsifClause::cond>(ref<ExpressionRule>) +
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

  struct CaseLabelRangeRule
      : rule<create<ast::CaseLabelRange>(
            assign<&ast::CaseLabelRange::low>(ref<ConstExpressionRule>) +
            option(lit<".."> + assign<&ast::CaseLabelRange::high>(
                                    ref<ConstExpressionRule>)))> {};

  struct CCaseRule
      : rule<create<ast::CCase>(
            some(append<&ast::CCase::labels>(ref<CaseLabelRangeRule>),
                 lit<",">) +
            lit<":"> +
            some(append<&ast::CCase::body>(ref<StatementRule>), lit<";">))> {};

  struct CaseStmtRule
      : rule<create<ast::CaseStmt>(
            kw<"CASE"> + assign<&ast::CaseStmt::value>(ref<ExpressionRule>) +
            kw<"OF"> +
            some(append<&ast::CaseStmt::arms>(ref<CCaseRule>), lit<"|">) +
            option(kw<"ELSE"> +
                   some(append<&ast::CaseStmt::elseBranch>(ref<StatementRule>),
                        lit<";">)) +
            kw<"END">)> {};

  struct WhileStmtRule
      : rule<create<ast::WhileStmt>(
            kw<"WHILE"> +
            assign<&ast::WhileStmt::cond>(ref<ExpressionRule>) + kw<"DO"> +
            some(append<&ast::WhileStmt::body>(ref<StatementRule>),
                 lit<";">) +
            kw<"END">)> {};

  struct RepeatStmtRule
      : rule<create<ast::RepeatStmt>(
            kw<"REPEAT"> +
            some(append<&ast::RepeatStmt::body>(ref<StatementRule>),
                 lit<";">) +
            kw<"UNTIL"> +
            assign<&ast::RepeatStmt::cond>(ref<ExpressionRule>))> {};

  struct LoopStmtRule
      : rule<create<ast::LoopStmt>(
            kw<"LOOP"> +
            some(append<&ast::LoopStmt::body>(ref<StatementRule>), lit<";">) +
            kw<"END">)> {};

  struct ForStmtRule
      : rule<create<ast::ForStmt>(
            kw<"FOR"> + assign<&ast::ForStmt::variable>(ref<IDENT>) +
            lit<":="> + assign<&ast::ForStmt::from>(ref<ExpressionRule>) +
            kw<"TO"> + assign<&ast::ForStmt::to>(ref<ExpressionRule>) +
            option(kw<"BY"> +
                   assign<&ast::ForStmt::by>(ref<ConstExpressionRule>)) +
            kw<"DO"> +
            some(append<&ast::ForStmt::body>(ref<StatementRule>), lit<";">) +
            kw<"END">)> {};

  struct WithStmtRule
      : rule<create<ast::WithStmt>(
            kw<"WITH"> + assign<&ast::WithStmt::target>(ref<DesignatorRule>) +
            kw<"DO"> +
            some(append<&ast::WithStmt::body>(ref<StatementRule>), lit<";">) +
            kw<"END">)> {};

  struct ExitStmtRule : rule<create<ast::ExitStmt> + kw<"EXIT">> {};

  struct ReturnStmtRule
      : rule<create<ast::ReturnStmt>(
            kw<"RETURN"> +
            option(assign<&ast::ReturnStmt::value>(ref<ExpressionRule>)))> {
  };

  struct StatementRule
      : rule<ref<AssignStmtRule>   //
             | ref<CallStmtRule>   //
             | ref<IfStmtRule>     //
             | ref<CaseStmtRule>   //
             | ref<WhileStmtRule>  //
             | ref<RepeatStmtRule>  //
             | ref<LoopStmtRule>    //
             | ref<ForStmtRule>     //
             | ref<WithStmtRule>    //
             | ref<ExitStmtRule>    //
             | ref<ReturnStmtRule>  //
             | create<ast::EmptyStmt>> {};

  // -- types ----

  struct NamedTypeRule
      : rule<assign<&ast::NamedType::name>(ref<QualidentRule>)> {};

  struct EnumTypeRule
      : rule<create<ast::EnumType>(
            lit<"("> +
            some(append<&ast::EnumType::values>(ref<IDENT>), lit<",">) +
            lit<")">)> {};

  struct SubrangeTypeRule
      : rule<create<ast::SubrangeType>(
            lit<"["> + assign<&ast::SubrangeType::low>(
                           ref<ConstExpressionRule>) +
            lit<".."> +
            assign<&ast::SubrangeType::high>(ref<ConstExpressionRule>) +
            lit<"]">)> {};

  struct SimpleTypeRule
      : rule<ref<NamedTypeRule> | ref<EnumTypeRule> |
             ref<SubrangeTypeRule>> {};

  struct ArrayTypeRule
      : rule<create<ast::ArrayType>(
            kw<"ARRAY"> +
            some(append<&ast::ArrayType::indexTypes>(ref<SimpleTypeRule>),
                 lit<",">) +
            kw<"OF"> +
            assign<&ast::ArrayType::component>(ref<TypeRule>))> {};

  struct FieldGroupRule
      : rule<create<ast::RecordField>(
            some(append<&ast::RecordField::names>(ref<IDENT>),
                 lit<",">) +
            lit<":"> + assign<&ast::RecordField::type>(ref<TypeRule>))> {};

  struct VariantRule
      : rule<create<ast::Variant>(
            some(append<&ast::Variant::labels>(ref<CaseLabelRangeRule>),
                 lit<",">) +
            lit<":"> +
            some(append<&ast::Variant::fields>(ref<FieldItemRule>),
                 lit<";">))> {};

  struct CaseFieldRule
      : rule<create<ast::CaseField>(
            kw<"CASE"> + assign<&ast::CaseField::tag>(ref<IDENT>) +
            option((lit<":"> | lit<".">) +
                   assign<&ast::CaseField::tagType>(ref<QualidentRule>)) +
            kw<"OF"> +
            append<&ast::CaseField::variants>(ref<VariantRule>) +
            many(lit<"|"> +
                 append<&ast::CaseField::variants>(ref<VariantRule>)) +
            option(kw<"ELSE"> +
                   some(append<&ast::CaseField::elseFields>(ref<FieldItemRule>),
                        lit<";">)) +
            kw<"END">)> {};

  struct FieldItemRule
      : rule<ref<FieldGroupRule> | ref<CaseFieldRule> |
             create<ast::EmptyField>> {};

  struct RecordTypeRule
      : rule<create<ast::RecordType>(
            kw<"RECORD"> +
            some(append<&ast::RecordType::fields>(ref<FieldItemRule>),
                 lit<";">) +
            kw<"END">)> {};

  struct SetTypeRule
      : rule<create<ast::SetType>(
            kw<"SET"> + kw<"OF"> +
            assign<&ast::SetType::base>(ref<SimpleTypeRule>))> {};

  struct PointerTypeRule
      : rule<create<ast::PointerType>(
            kw<"POINTER"> + kw<"TO"> +
            assign<&ast::PointerType::target>(ref<TypeRule>))> {};

  // formalType: (ARRAY OF)? qualident -- a data rule, the whole spelling
  // kept as one span since this port does not resolve types.
  struct FormalTypeRule
      : rule<option(kw<"ARRAY"> + kw<"OF">) + ref<QualidentRule>> {};

  struct ProcTypeParamRule
      : rule<create<ast::ProcTypeParam>(
            option(flag<&ast::ProcTypeParam::byRef>(kw<"VAR">)) +
            assign<&ast::ProcTypeParam::type>(ref<FormalTypeRule>))> {};

  struct FormalTypeListRule
      : rule<create<ast::FormalTypeList>(
            lit<"("> +
            option(some(append<&ast::FormalTypeList::params>(
                            ref<ProcTypeParamRule>),
                        lit<",">)) +
            lit<")"> +
            option(lit<":"> + assign<&ast::FormalTypeList::resultType>(
                                   ref<QualidentRule>)))> {};

  struct ProcedureTypeRule
      : rule<create<ast::ProcedureType> + kw<"PROCEDURE"> +
             option(assign<&ast::ProcedureType::signature>(
                 ref<FormalTypeListRule>))> {};

  struct TypeRule
      : rule<ref<SimpleTypeRule> | ref<ArrayTypeRule> | ref<RecordTypeRule> |
             ref<SetTypeRule> | ref<PointerTypeRule> |
             ref<ProcedureTypeRule>> {};

  // -- declarations ----

  struct ConstDeclRule
      : rule<create<ast::ConstDecl>(
            assign<&ast::ConstDecl::name>(ref<IDENT>) + lit<"="> +
            assign<&ast::ConstDecl::value>(ref<ConstExpressionRule>))> {};

  struct TypeDeclRule
      : rule<create<ast::TypeDecl>(
            assign<&ast::TypeDecl::name>(ref<IDENT>) + lit<"="> +
            assign<&ast::TypeDecl::type>(ref<TypeRule>))> {};

  // A definition module's TYPE entry may leave the type opaque.
  struct DefTypeDeclRule
      : rule<create<ast::TypeDecl>(
            assign<&ast::TypeDecl::name>(ref<IDENT>) +
            option(lit<"="> + assign<&ast::TypeDecl::type>(ref<TypeRule>)))> {
  };

  struct VarDeclRule
      : rule<create<ast::VarDecl>(
            some(append<&ast::VarDecl::names>(ref<IDENT>), lit<",">) +
            lit<":"> + assign<&ast::VarDecl::type>(ref<TypeRule>))> {};

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
            option(lit<":"> + assign<&ast::FormalParams::resultType>(
                                   ref<QualidentRule>)))> {};

  struct ProcHeadingRule
      : rule<create<ast::ProcHeading>(
            kw<"PROCEDURE"> + assign<&ast::ProcHeading::name>(ref<IDENT>) +
            option(assign<&ast::ProcHeading::params>(
                ref<FormalParametersRule>)))> {};

  struct ProcDeclRule
      : rule<create<ast::ProcDecl>(
            assign<&ast::ProcDecl::heading>(ref<ProcHeadingRule>) +
            lit<";"> + assign<&ast::ProcDecl::block>(ref<BlockRule>) +
            assign<&ast::ProcDecl::endName>(ref<IDENT>))> {};

  struct ImportRule
      : rule<create<ast::Import>(
            option(kw<"FROM"> + assign<&ast::Import::from>(ref<IDENT>)) +
            kw<"IMPORT"> +
            some(append<&ast::Import::names>(ref<IDENT>), lit<",">) +
            lit<";">)> {};

  // block: declaration* (BEGIN statementSequence)? END -- each declaration
  // is a whole CONST/TYPE/VAR section, a procedure or a nested module, and
  // the sections may repeat and interleave in any order.
  struct BlockRule
      : rule<create<ast::Block>(
            many((kw<"CONST"> +
                  many(append<&ast::Block::decls>(ref<ConstDeclRule>) +
                       lit<";">))  //
                 | (kw<"TYPE"> +
                    many(append<&ast::Block::decls>(ref<TypeDeclRule>) +
                         lit<";">))  //
                 | (kw<"VAR"> +
                    many(append<&ast::Block::decls>(ref<VarDeclRule>) +
                         lit<";">))  //
                 | (append<&ast::Block::decls>(ref<ProcDeclRule>) +
                    lit<";">)  //
                 | (append<&ast::Block::decls>(ref<ModuleDeclRule>) +
                    lit<";">)) +
            option(kw<"BEGIN"> +
                   some(append<&ast::Block::body>(ref<StatementRule>),
                        lit<";">)) +
            kw<"END">)> {};

  struct ModuleDeclRule
      : rule<create<ast::ModuleDecl>(
            kw<"MODULE"> + assign<&ast::ModuleDecl::name>(ref<IDENT>) +
            option(lit<"["> +
                   assign<&ast::ModuleDecl::priority>(
                       ref<ConstExpressionRule>) +
                   lit<"]">) +
            lit<";"> +
            many(append<&ast::ModuleDecl::imports>(ref<ImportRule>)) +
            option(kw<"EXPORT"> +
                   option(flag<&ast::ModuleDecl::exportQualified>(
                       kw<"QUALIFIED">)) +
                   some(append<&ast::ModuleDecl::exports>(ref<IDENT>),
                        lit<",">) +
                   lit<";">) +
            assign<&ast::ModuleDecl::block>(ref<BlockRule>) +
            assign<&ast::ModuleDecl::endName>(ref<IDENT>))> {};

  // The optional IMPLEMENTATION prefix folds into this rule, so a
  // compilation unit itself is a plain two-way choice.
  struct ProgramModuleRule
      : rule<create<ast::ProgramModule>(
            option(flag<&ast::ProgramModule::isImplementation>(
                kw<"IMPLEMENTATION">)) +
            kw<"MODULE"> +
            assign<&ast::ProgramModule::name>(ref<IDENT>) +
            option(lit<"["> +
                   assign<&ast::ProgramModule::priority>(
                       ref<ConstExpressionRule>) +
                   lit<"]">) +
            lit<";"> +
            many(append<&ast::ProgramModule::imports>(ref<ImportRule>)) +
            assign<&ast::ProgramModule::block>(ref<BlockRule>) +
            assign<&ast::ProgramModule::endName>(ref<IDENT>) +
            lit<".">)> {};

  struct DefinitionModuleRule
      : rule<create<ast::DefinitionModule>(
            kw<"DEFINITION"> + kw<"MODULE"> +
            assign<&ast::DefinitionModule::name>(ref<IDENT>) +
            lit<";"> +
            many(append<&ast::DefinitionModule::imports>(ref<ImportRule>)) +
            option(kw<"EXPORT"> +
                   option(flag<&ast::DefinitionModule::exportQualified>(
                       kw<"QUALIFIED">)) +
                   some(append<&ast::DefinitionModule::exports>(
                            ref<IDENT>),
                        lit<",">) +
                   lit<";">) +
            many((kw<"CONST"> +
                  many(append<&ast::DefinitionModule::decls>(
                           ref<ConstDeclRule>) +
                       lit<";">))  //
                 | (kw<"TYPE"> +
                    many(append<&ast::DefinitionModule::decls>(
                             ref<DefTypeDeclRule>) +
                         lit<";">))  //
                 | (kw<"VAR"> +
                    many(append<&ast::DefinitionModule::decls>(
                             ref<VarDeclRule>) +
                         lit<";">))  //
                 | (append<&ast::DefinitionModule::decls>(
                        ref<ProcHeadingRule>) +
                    lit<";">)) +
            kw<"END"> +
            assign<&ast::DefinitionModule::endName>(ref<IDENT>) +
            lit<".">)> {};

  struct CompilationUnitRule
      : rule<ref<DefinitionModuleRule> | ref<ProgramModuleRule>> {};

  using entry = CompilationUnitRule;
  using ignore = WS;
  using hidden = list<COMMENT>;
};

}  // namespace modula2pim4
