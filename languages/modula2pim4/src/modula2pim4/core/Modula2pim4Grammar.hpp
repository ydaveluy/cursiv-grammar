// The modula2pim4 language: its grammar. PIM (4th edition) Modula-2:
// case-sensitive uppercase keywords, one LR precedence table folding
// relation/add/mul (shared by constant and runtime expressions), nested
// `(* *)` comments.
#pragma once

#include <charconv>
#include <cstdint>
#include <memory_resource>
#include <span>
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

  // PIM4 identifiers contain letters/digits; keywords share that boundary.
  static constexpr auto word_boundary = cr<"a-zA-Z0-9">;

  static constexpr auto Reserved =
      kw<"AND"> | kw<"ARRAY"> | kw<"BEGIN"> | kw<"BY"> | kw<"CASE"> |
      kw<"CONST"> | kw<"DEFINITION"> | kw<"DIV"> | kw<"DO"> | kw<"ELSE"> |
      kw<"ELSIF"> | kw<"END"> | kw<"EXIT"> | kw<"EXPORT"> | kw<"FOR"> |
      kw<"FROM"> | kw<"IF"> | kw<"IMPLEMENTATION"> | kw<"IMPORT"> | kw<"IN"> |
      kw<"LOOP"> | kw<"MOD"> | kw<"MODULE"> | kw<"NOT"> | kw<"OF"> |
      kw<"OR"> | kw<"POINTER"> | kw<"PROCEDURE"> | kw<"QUALIFIED"> |
      kw<"RECORD"> | kw<"REPEAT"> | kw<"RETURN"> | kw<"SET"> | kw<"THEN"> |
      kw<"TO"> | kw<"TYPE"> | kw<"UNTIL"> | kw<"VAR"> | kw<"WHILE"> | kw<"WITH">;
  struct IDENT
      : terminal<!Reserved + cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9">)> {};

  static constexpr auto parseInt = [](std::string_view t)
      -> cursiv::ConversionResult<std::int64_t> {
    int base = 10;
    if (t.back() == 'H') {
      base = 16;
      t.remove_suffix(1);
    } else if (t.back() == 'B' || t.back() == 'C') {
      base = 8;
      t.remove_suffix(1);
    }
    std::int64_t v = 0;
    const auto [end, ec] = std::from_chars(t.data(), t.data() + t.size(), v, base);
    if (ec != std::errc{} || end != t.data() + t.size())
      return cursiv::ConversionResult<std::int64_t>::err("Integer literal out of range.");
    return cursiv::ConversionResult<std::int64_t>::ok(v);
  };
  struct INTEGER
      : terminal<as<parseInt>((some(cr<"0-7">) + lit<"B">)  //
                              | (d + many(cr<"0-9A-F">) + lit<"H">)      //
                              | some(d))> {};

  struct CHARACTER_CODE
      : terminal<as<parseInt>(some(cr<"0-7">) + lit<"C">)> {};

  // The dot is guarded so a subrange's `..` (`[1..5]`) is not eaten as the
  // start of a real literal.
  struct REAL
      : terminal<as<double>(some(d) + lit<".", !lit<".">> + many(d) +
                            option(lit<"E"> + option(cr<"+-">) + some(d)))> {};

  // Every printable byte but the two quotes: Modula-2 has no reason to
  // refuse a slash in a string.
  static constexpr auto StringByte = cr<" !#-&(-~">;
  static constexpr auto unquote = [](std::string_view t) {
    return t.substr(1, t.size() - 2);
  };
  struct STRING
      : terminal<as<unquote>(
            (lit<"'"> + many(StringByte | lit<"\"">) + lit<"'">)  //
            | (lit<"\""> + many(StringByte | lit<"'">) + lit<"\"">))> {};

  // A dotted chain of names, kept as one span of text: a module qualifier
  // (`Module.Ident`) and, absent any preceding index/deref, a plain
  // record-field chain both read through here -- a designator's postfix
  // tail only opens on `[` or `^`.
  struct Qualident : rule<ref<IDENT> + many(lit<"."> + ref<IDENT>)> {};

  static constexpr auto MulOp =
      lit<"*"> | lit<"/"> | kw<"DIV"> | kw<"MOD"> | kw<"AND"> | lit<"&">;
  static constexpr auto AddOp = lit<"+"> | lit<"-"> | kw<"OR">;
  static constexpr auto RelOp = lit<"="> | lit<"#"> | lit<"<>"> | lit<"<="> |
                                lit<"<"> | lit<">="> | lit<">"> | kw<"IN">;

  struct Expression;
  struct ConstExpression;
  struct Factor;
  struct Designator;
  struct Statement;
  struct Type;
  struct SimpleType;
  struct FieldItem;
  struct CaseField;
  struct Variant;
  struct Block;
  struct ModuleDecl;
  struct ProcDecl;

  // -- literals and sets, shared by constant and runtime expressions ----

  struct IntLit
      : rule<assign<&ast::IntLit::value>(ref<INTEGER>) |
             flag<&ast::IntLit::character>(
                 assign<&ast::IntLit::value>(ref<CHARACTER_CODE>))> {};
  struct RealLit : rule<assign<&ast::RealLit::value>(ref<REAL>)> {};
  struct StringLit
      : rule<assign<&ast::StringLit::value>(ref<STRING>)> {};

  // Set elements use expression syntax; constantness is checked by the
  // native type/constant rules rather than a duplicate expression grammar.
  struct SetElement
      : rule<create<ast::SetElement>(
            assign<&ast::SetElement::low>(ref<ConstExpression>) +
            option(lit<".."> +
                   assign<&ast::SetElement::high>(ref<ConstExpression>)))> {
  };
  static constexpr auto SetBody =
      lit<"{"> +
      many(append<&ast::SetLit::elements>(ref<SetElement>),
                  lit<",">) +
      lit<"}">;
  struct SetLit : rule<create<ast::SetLit> + SetBody> {};

  // -- designators (postfix chains) ----

  static constexpr auto IndexPostfix =
      nest<ast::IndexExpr, &ast::IndexExpr::array>(
          lit<"["> +
          some(append<&ast::IndexExpr::indices>(ref<Expression>),
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

  struct Designator
      : rule<assign<&ast::NameExpr::name>(ref<Qualident>) +
             option(DesignatorTailBody)> {};

  static constexpr auto CallBody =
      lit<"("> +
      many(append<&ast::CallExpr::args>(ref<Expression>),
                  lit<",">) +
      lit<")">;

  // designator (actualParameters)? -- shared by an expression-context call
  // and a statement-context one.
  struct DesignatorCall
      : rule<ref<Designator> +
             option(nest<ast::CallExpr, &ast::CallExpr::callee>(
                 CallBody))> {};

  // A qualident followed by a mandatory set literal -- the typed-set half
  // of setOrQualident/setOrDesignatorOrProcCall. A bare qualident (no set)
  // is disjoint by lookahead (only `{` continues into this branch), so the
  // two halves never overlap despite each reading a qualident. `nest<>`
  // needs an optional context to adopt through, so the base name is its own
  // `create<>` here instead.
  struct QualSet
      : rule<create<ast::SetLit>(
            assign<&ast::SetLit::baseType>(create<ast::NameExpr>(
                assign<&ast::NameExpr::name>(ref<Qualident>))) +
            SetBody)> {};

  // factor's setOrDesignatorOrProcCall: a bare set, a qualident with a
  // trailing typed set, or a designator with an optional call -- disjoint
  // from the typed-set form by the following brace.
  struct Factor
      : rule<ref<IntLit>                                            //
             | ref<RealLit>                                         //
             | ref<StringLit>                                       //
             | ref<SetLit>                                          //
             | ref<QualSet>                                         //
             | ref<DesignatorCall>                                  //
             | (lit<"("> + ref<Expression> + lit<")">)               //
             | create<ast::UnaryExpr>(
                   assign<&ast::UnaryExpr::op>(kw<"NOT"> | lit<"~">) +
                   assign<&ast::UnaryExpr::operand>(ref<Factor>))> {};

  // PIM4 SimpleExpression puts one optional sign before the whole term;
  // subsequent additive operands are unsigned terms, and relations do not
  // chain. ConstExpression has the same syntax (constantness is semantic).
  struct Term : infix<Factor, ast::BinaryExpr, left(MulOp)> {};
  struct SignedTerm
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-">) +
                 assign<&ast::UnaryExpr::operand>(ref<Term>)) |
             ref<Term>> {};
  struct SimpleExpression
      : rule<ref<SignedTerm> +
             many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 assign<&ast::BinaryExpr::op>(AddOp) +
                 assign<&ast::BinaryExpr::right>(ref<Term>)))> {};
  struct Expression
      : infix<SimpleExpression, ast::BinaryExpr, nonassoc(RelOp)> {};
  struct ConstExpression : rule<ref<Expression>> {};

  // -- statements ----

  struct AssignStmt
      : rule<create<ast::AssignStmt>(
            assign<&ast::AssignStmt::target>(ref<Designator>) +
            lit<":="> +
            assign<&ast::AssignStmt::value>(ref<Expression>))> {};

  struct CallStmt
      : rule<assign<&ast::CallStmt::value>(ref<DesignatorCall>)> {};

  struct IfElsif
      : rule<create<ast::ElsifClause>(
            kw<"ELSIF"> +
            assign<&ast::ElsifClause::cond>(ref<Expression>) +
            kw<"THEN"> +
            some(append<&ast::ElsifClause::body>(ref<Statement>),
                 lit<";">))> {};

  struct IfStmt
      : rule<create<ast::IfStmt>(
            kw<"IF"> + assign<&ast::IfStmt::cond>(ref<Expression>) +
            kw<"THEN"> +
            some(append<&ast::IfStmt::thenBranch>(ref<Statement>),
                 lit<";">) +
            many(append<&ast::IfStmt::elsifs>(ref<IfElsif>)) +
            option(kw<"ELSE"> +
                   some(append<&ast::IfStmt::elseBranch>(ref<Statement>),
                        lit<";">)) +
            kw<"END">)> {};

  struct CaseLabelRange
      : rule<create<ast::CaseLabelRange>(
            assign<&ast::CaseLabelRange::low>(ref<ConstExpression>) +
            option(lit<".."> + assign<&ast::CaseLabelRange::high>(
                                    ref<ConstExpression>)))> {};

  struct CCase
      : rule<create<ast::CCase>(
            option(some(append<&ast::CCase::labels>(ref<CaseLabelRange>),
                        lit<",">) + lit<":"> +
                   some(append<&ast::CCase::body>(ref<Statement>), lit<";">)))> {};

  struct CaseStmt
      : rule<create<ast::CaseStmt>(
            kw<"CASE"> + assign<&ast::CaseStmt::value>(ref<Expression>) +
            kw<"OF"> +
            some(append<&ast::CaseStmt::arms>(ref<CCase>), lit<"|">) +
            option(kw<"ELSE"> +
                   some(append<&ast::CaseStmt::elseBranch>(ref<Statement>),
                        lit<";">)) +
            kw<"END">)> {};

  struct WhileStmt
      : rule<create<ast::WhileStmt>(
            kw<"WHILE"> +
            assign<&ast::WhileStmt::cond>(ref<Expression>) + kw<"DO"> +
            some(append<&ast::WhileStmt::body>(ref<Statement>),
                 lit<";">) +
            kw<"END">)> {};

  struct RepeatStmt
      : rule<create<ast::RepeatStmt>(
            kw<"REPEAT"> +
            some(append<&ast::RepeatStmt::body>(ref<Statement>),
                 lit<";">) +
            kw<"UNTIL"> +
            assign<&ast::RepeatStmt::cond>(ref<Expression>))> {};

  struct LoopStmt
      : rule<create<ast::LoopStmt>(
            kw<"LOOP"> +
            some(append<&ast::LoopStmt::body>(ref<Statement>), lit<";">) +
            kw<"END">)> {};

  struct ForStmt
      : rule<create<ast::ForStmt>(
            kw<"FOR"> + assign<&ast::ForStmt::variable>(ref<IDENT>) +
            lit<":="> + assign<&ast::ForStmt::from>(ref<Expression>) +
            kw<"TO"> + assign<&ast::ForStmt::to>(ref<Expression>) +
            option(kw<"BY"> +
                   assign<&ast::ForStmt::by>(ref<ConstExpression>)) +
            kw<"DO"> +
            some(append<&ast::ForStmt::body>(ref<Statement>), lit<";">) +
            kw<"END">)> {};

  struct WithStmt
      : rule<create<ast::WithStmt>(
            kw<"WITH"> + assign<&ast::WithStmt::target>(ref<Designator>) +
            kw<"DO"> +
            some(append<&ast::WithStmt::body>(ref<Statement>), lit<";">) +
            kw<"END">)> {};

  struct ExitStmt : rule<create<ast::ExitStmt> + kw<"EXIT">> {};

  struct ReturnStmt
      : rule<create<ast::ReturnStmt>(
            kw<"RETURN"> +
            option(assign<&ast::ReturnStmt::value>(ref<Expression>)))> {
  };

  struct Statement
      : rule<ref<AssignStmt>   //
             | ref<CallStmt>   //
             | ref<IfStmt>     //
             | ref<CaseStmt>   //
             | ref<WhileStmt>  //
             | ref<RepeatStmt>  //
             | ref<LoopStmt>    //
             | ref<ForStmt>     //
             | ref<WithStmt>    //
             | ref<ExitStmt>    //
             | ref<ReturnStmt>  //
             | create<ast::EmptyStmt>> {};

  // -- types ----

  struct NamedType
      : rule<assign<&ast::NamedType::name>(ref<Qualident>)> {};

  struct EnumType
      : rule<create<ast::EnumType>(
            lit<"("> +
            some(append<&ast::EnumType::values>(ref<IDENT>), lit<",">) +
            lit<")">)> {};

  struct SubrangeType
      : rule<create<ast::SubrangeType>(
            option(assign<&ast::SubrangeType::base>(ref<IDENT>)) +
            lit<"["> + assign<&ast::SubrangeType::low>(
                           ref<ConstExpression>) +
            lit<".."> +
            assign<&ast::SubrangeType::high>(ref<ConstExpression>) +
            lit<"]">)> {};

  struct SimpleType
      : rule<ref<NamedType> | ref<EnumType> |
             ref<SubrangeType>> {};

  struct ArrayType
      : rule<create<ast::ArrayType>(
            kw<"ARRAY"> +
            some(append<&ast::ArrayType::indexTypes>(ref<SimpleType>),
                 lit<",">) +
            kw<"OF"> +
            assign<&ast::ArrayType::component>(ref<Type>))> {};

  struct FieldGroup
      : rule<create<ast::RecordField>(
            some(append<&ast::RecordField::names>(ref<IDENT>),
                 lit<",">) +
            lit<":"> + assign<&ast::RecordField::type>(ref<Type>))> {};

  struct Variant
      : rule<create<ast::Variant>(
            option(some(append<&ast::Variant::labels>(ref<CaseLabelRange>),
                        lit<",">) + lit<":"> +
                   some(append<&ast::Variant::fields>(ref<FieldItem>), lit<";">)))> {};

  struct CaseField
      : rule<create<ast::CaseField>(
            kw<"CASE"> + option(assign<&ast::CaseField::tag>(ref<IDENT>)) +
            lit<":"> + assign<&ast::CaseField::tagType>(ref<Qualident>) +
            kw<"OF"> +
            append<&ast::CaseField::variants>(ref<Variant>) +
            many(lit<"|"> +
                 append<&ast::CaseField::variants>(ref<Variant>)) +
            option(kw<"ELSE"> +
                   some(append<&ast::CaseField::elseFields>(ref<FieldItem>),
                        lit<";">)) +
            kw<"END">)> {};

  struct FieldItem
      : rule<ref<FieldGroup> | ref<CaseField> |
             create<ast::EmptyField>> {};

  struct RecordType
      : rule<create<ast::RecordType>(
            kw<"RECORD"> +
            some(append<&ast::RecordType::fields>(ref<FieldItem>),
                 lit<";">) +
            kw<"END">)> {};

  struct SetType
      : rule<create<ast::SetType>(
            kw<"SET"> + kw<"OF"> +
            assign<&ast::SetType::base>(ref<SimpleType>))> {};

  struct PointerType
      : rule<create<ast::PointerType>(
            kw<"POINTER"> + kw<"TO"> +
            assign<&ast::PointerType::target>(ref<Type>))> {};

  // Preserve the semantic distinction between open-array and plain types:
  // the default data-rule concatenation would make both ARRAY OF T and
  // ARRAYOFT spell ARRAYOFT.
  struct FormalType
      : rule<as<[](std::span<const std::string_view> parts,
                    std::pmr::memory_resource* mem) {
          const bool array = parts.size() >= 3 && parts[0] == "ARRAY" && parts[1] == "OF";
          std::size_t size = array ? 9 : 0;
          for (std::size_t i = array ? 2 : 0; i < parts.size(); ++i) size += parts[i].size();
          const auto room = cursiv::text_room(mem, size);
          std::size_t n = 0;
          if (array) for (const char c : std::string_view("ARRAY OF ")) room[n++] = c;
          for (std::size_t i = array ? 2 : 0; i < parts.size(); ++i)
            for (const char c : parts[i]) room[n++] = c;
          return std::string_view(room.data(), n);
        }>(option(kw<"ARRAY"> + kw<"OF">) + ref<Qualident>)> {};

  struct ProcTypeParam
      : rule<create<ast::ProcTypeParam>(
            option(flag<&ast::ProcTypeParam::byRef>(kw<"VAR">)) +
            assign<&ast::ProcTypeParam::type>(ref<FormalType>))> {};

  struct FormalTypeList
      : rule<create<ast::FormalTypeList>(
            lit<"("> +
            many(append<&ast::FormalTypeList::params>(
                            ref<ProcTypeParam>),
                        lit<",">) +
            lit<")"> +
            option(lit<":"> + assign<&ast::FormalTypeList::resultType>(
                                   ref<Qualident>)))> {};

  struct ProcedureType
      : rule<create<ast::ProcedureType> + kw<"PROCEDURE"> +
             option(assign<&ast::ProcedureType::signature>(
                 ref<FormalTypeList>))> {};

  struct Type
      : rule<ref<SimpleType> | ref<ArrayType> | ref<RecordType> |
             ref<SetType> | ref<PointerType> |
             ref<ProcedureType>> {};

  // -- declarations ----

  struct ConstDecl
      : rule<create<ast::ConstDecl>(
            assign<&ast::ConstDecl::name>(ref<IDENT>) + lit<"="> +
            assign<&ast::ConstDecl::value>(ref<ConstExpression>))> {};

  struct TypeDecl
      : rule<create<ast::TypeDecl>(
            assign<&ast::TypeDecl::name>(ref<IDENT>) + lit<"="> +
            assign<&ast::TypeDecl::type>(ref<Type>))> {};

  // A definition module's TYPE entry may leave the type opaque.
  struct DefTypeDecl
      : rule<create<ast::TypeDecl>(
            assign<&ast::TypeDecl::name>(ref<IDENT>) +
            option(lit<"="> + assign<&ast::TypeDecl::type>(ref<Type>)))> {
  };

  struct VarDecl
      : rule<create<ast::VarDecl>(
            some(append<&ast::VarDecl::names>(ref<IDENT>), lit<",">) +
            lit<":"> + assign<&ast::VarDecl::type>(ref<Type>))> {};

  struct FPSection
      : rule<create<ast::FormalParam>(
            option(flag<&ast::FormalParam::byRef>(kw<"VAR">)) +
            some(append<&ast::FormalParam::names>(ref<IDENT>),
                 lit<",">) +
            lit<":"> +
            assign<&ast::FormalParam::type>(ref<FormalType>))> {};

  struct FormalParameters
      : rule<create<ast::FormalParams>(
            lit<"("> +
            many(append<&ast::FormalParams::params>(
                            ref<FPSection>),
                        lit<";">) +
            lit<")"> +
            option(lit<":"> + assign<&ast::FormalParams::resultType>(
                                   ref<Qualident>)))> {};

  struct ProcHeading
      : rule<create<ast::ProcHeading>(
            kw<"PROCEDURE"> + assign<&ast::ProcHeading::name>(ref<IDENT>) +
            option(assign<&ast::ProcHeading::params>(
                ref<FormalParameters>)))> {};

  struct ProcDecl
      : rule<create<ast::ProcDecl>(
            assign<&ast::ProcDecl::heading>(ref<ProcHeading>) +
            lit<";"> + assign<&ast::ProcDecl::block>(ref<Block>) +
            assign<&ast::ProcDecl::endName>(ref<IDENT>))> {};

  struct Import
      : rule<create<ast::Import>(
            option(kw<"FROM"> + assign<&ast::Import::from>(ref<IDENT>)) +
            kw<"IMPORT"> +
            some(append<&ast::Import::names>(ref<IDENT>), lit<",">) +
            lit<";">)> {};

  // block: declaration* (BEGIN statementSequence)? END -- each declaration
  // is a whole CONST/TYPE/VAR section, a procedure or a nested module, and
  // the sections may repeat and interleave in any order.
  struct Block
      : rule<create<ast::Block>(
            many((kw<"CONST"> +
                  many(append<&ast::Block::decls>(ref<ConstDecl>) +
                       lit<";">))  //
                 | (kw<"TYPE"> +
                    many(append<&ast::Block::decls>(ref<TypeDecl>) +
                         lit<";">))  //
                 | (kw<"VAR"> +
                    many(append<&ast::Block::decls>(ref<VarDecl>) +
                         lit<";">))  //
                 | (append<&ast::Block::decls>(ref<ProcDecl>) +
                    lit<";">)  //
                 | (append<&ast::Block::decls>(ref<ModuleDecl>) +
                    lit<";">)) +
            option(kw<"BEGIN"> +
                   some(append<&ast::Block::body>(ref<Statement>),
                        lit<";">)) +
            kw<"END">)> {};

  struct ModuleDecl
      : rule<create<ast::ModuleDecl>(
            kw<"MODULE"> + assign<&ast::ModuleDecl::name>(ref<IDENT>) +
            option(lit<"["> +
                   assign<&ast::ModuleDecl::priority>(
                       ref<ConstExpression>) +
                   lit<"]">) +
            lit<";"> +
            many(append<&ast::ModuleDecl::imports>(ref<Import>)) +
            option(kw<"EXPORT"> +
                   option(flag<&ast::ModuleDecl::exportQualified>(
                       kw<"QUALIFIED">)) +
                   some(append<&ast::ModuleDecl::exports>(ref<IDENT>),
                        lit<",">) +
                   lit<";">) +
            assign<&ast::ModuleDecl::block>(ref<Block>) +
            assign<&ast::ModuleDecl::endName>(ref<IDENT>))> {};

  // The optional IMPLEMENTATION prefix folds into this rule, so a
  // compilation unit itself is a plain two-way choice.
  struct ProgramModule
      : rule<create<ast::ProgramModule>(
            option(flag<&ast::ProgramModule::isImplementation>(
                kw<"IMPLEMENTATION">)) +
            kw<"MODULE"> +
            assign<&ast::ProgramModule::name>(ref<IDENT>) +
            option(lit<"["> +
                   assign<&ast::ProgramModule::priority>(
                       ref<ConstExpression>) +
                   lit<"]">) +
            lit<";"> +
            many(append<&ast::ProgramModule::imports>(ref<Import>)) +
            assign<&ast::ProgramModule::block>(ref<Block>) +
            assign<&ast::ProgramModule::endName>(ref<IDENT>) +
            lit<".">)> {};

  struct DefinitionModule
      : rule<create<ast::DefinitionModule>(
            kw<"DEFINITION"> + kw<"MODULE"> +
            assign<&ast::DefinitionModule::name>(ref<IDENT>) +
            lit<";"> +
            many(append<&ast::DefinitionModule::imports>(ref<Import>)) +
            option(kw<"EXPORT"> +
                   option(flag<&ast::DefinitionModule::exportQualified>(
                       kw<"QUALIFIED">)) +
                   some(append<&ast::DefinitionModule::exports>(
                            ref<IDENT>),
                        lit<",">) +
                   lit<";">) +
            many((kw<"CONST"> +
                  many(append<&ast::DefinitionModule::decls>(
                           ref<ConstDecl>) +
                       lit<";">))  //
                 | (kw<"TYPE"> +
                    many(append<&ast::DefinitionModule::decls>(
                             ref<DefTypeDecl>) +
                         lit<";">))  //
                 | (kw<"VAR"> +
                    many(append<&ast::DefinitionModule::decls>(
                             ref<VarDecl>) +
                         lit<";">))  //
                 | (append<&ast::DefinitionModule::decls>(
                        ref<ProcHeading>) +
                    lit<";">)) +
            kw<"END"> +
            assign<&ast::DefinitionModule::endName>(ref<IDENT>) +
            lit<".">)> {};

  struct CompilationUnit
      : rule<ref<DefinitionModule> | ref<ProgramModule>> {};

  using entry = CompilationUnit;
  using ignored = WS;
  using hidden = list<COMMENT>;
};

}  // namespace modula2pim4
