// The oberon language: its grammar. Case-sensitive uppercase keywords, one
// native sign/relation boundaries, nested `(* *)` comments.
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

  // Keywords and identifiers end after the same letter/digit class.
  static constexpr auto word_boundary = cr<"a-zA-Z0-9">;

  static constexpr auto Reserved =
      kw<"ARRAY"> | kw<"BEGIN"> | kw<"BY"> | kw<"CASE"> | kw<"CONST"> |
      kw<"DIV"> | kw<"DO"> | kw<"ELSE"> | kw<"ELSIF"> | kw<"END"> |
      kw<"FALSE"> | kw<"FOR"> | kw<"IF"> | kw<"IMPORT"> | kw<"IN"> |
      kw<"IS"> | kw<"MOD"> | kw<"MODULE"> | kw<"NIL"> | kw<"OF"> |
      kw<"OR"> | kw<"POINTER"> | kw<"PROCEDURE"> | kw<"RECORD"> |
      kw<"REPEAT"> | kw<"RETURN"> | kw<"THEN"> | kw<"TO"> | kw<"TRUE"> |
      kw<"TYPE"> | kw<"UNTIL"> | kw<"VAR"> | kw<"WHILE">;
  struct IDENT
      : terminal<!Reserved + cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9">)> {};

  static constexpr auto parseInt = [](std::string_view t)
      -> cursiv::ConversionResult<std::int64_t> {
    const int base = t.back() == 'H' ? 16 : 10;
    if (base == 16) t.remove_suffix(1);
    std::int64_t v = 0;
    const auto [end, ec] = std::from_chars(t.data(), t.data() + t.size(), v, base);
    if (ec != std::errc{} || end != t.data() + t.size())
      return cursiv::ConversionResult<std::int64_t>::err("Integer literal out of range.");
    return cursiv::ConversionResult<std::int64_t>::ok(v);
  };
  struct INTEGER
      : terminal<as<parseInt>(some(d) | (d + many(cr<"0-9A-F">) + lit<"H">))> {
  };

  struct REAL
      : terminal<as<double>(some(d) + lit<".", !lit<".">> + many(d) +
                            option(lit<"E"> + option(cr<"+-">) + some(d)))> {};

  static constexpr auto decodeString =
      [](std::string_view t, std::pmr::memory_resource* mem)
          -> cursiv::ConversionResult<std::string_view> {
        if (t.back() == 'X') {
          std::uint8_t code = 0;
          const auto [end, ec] =
              std::from_chars(t.data(), t.data() + t.size() - 1, code, 16);
          if (ec != std::errc{} || end != t.data() + t.size() - 1)
            return cursiv::ConversionResult<std::string_view>::err("Character literal out of range.");
          const std::span<char> room = cursiv::text_room(mem, 1);
          room[0] = static_cast<char>(code);
          return cursiv::ConversionResult<std::string_view>::ok({room.data(), 1});
        }
        return cursiv::ConversionResult<std::string_view>::ok(t.substr(1, t.size() - 2));
      };
  struct STRING
      : terminal<as<decodeString>((lit<"\""> + many(cr<R"(^"\r\n)">) + lit<"\"">) |
                                  (d + many(cr<"0-9A-F">) + lit<"X">))> {};

  // identdef: an identifier with its optional export marker, kept in the
  // text -- this port does not resolve names, so there is nothing else to
  // do with the marker.
  struct IdentDef : rule<ref<IDENT> + option(lit<"*">)> {};

  // qualident: at most one module qualifier, kept as one span of text.
  struct Qualident : rule<option(ref<IDENT> + lit<".">) + ref<IDENT>> {};

  static constexpr auto MulOp =
      lit<"*"> | lit<"/"> | kw<"DIV"> | kw<"MOD"> | lit<"&">;
  static constexpr auto AddOp = lit<"+"> | lit<"-"> | kw<"OR">;
  static constexpr auto RelOp = lit<"<="> | lit<">="> | lit<"="> | lit<"#"> |
                                lit<"<"> | lit<">"> | kw<"IN"> | kw<"IS">;

  struct Expression;
  struct Factor;
  struct Designator;
  struct DesignatorCall;
  struct Type;
  struct Statement;
  struct FormalParameters;

  // -- designators and factors ----

  // A postfix chain over a qualident: indexing, field access, dereference,
  // type guard -- each `nest<>` wraps the node built so far, so `a[i].b^`
  // reads left to right into DerefExpr(FieldExpr(IndexExpr(...))).
  struct Designator
      : rule<assign<&ast::NameExpr::name>(ref<Qualident>) +
             many(
                 nest<ast::IndexExpr, &ast::IndexExpr::array>(
                     lit<"["> +
                     some(append<&ast::IndexExpr::indices>(ref<Expression>),
                          lit<",">) +
                     lit<"]">)  //
                 |
                 nest<ast::FieldExpr, &ast::FieldExpr::object>(
                     lit<"."> + assign<&ast::FieldExpr::field>(ref<IDENT>))  //
                 | nest<ast::DerefExpr, &ast::DerefExpr::operand>(lit<"^">)  //
                 | nest<ast::ApplyExpr, &ast::ApplyExpr::operand>(
                       lit<"("> +
                       many(append<&ast::ApplyExpr::args>(ref<Expression>),
                            lit<",">) +
                       lit<")">))> {};
  struct DesignatorCall : rule<ref<Designator>> {};

  struct IntLit : rule<assign<&ast::IntLit::value>(ref<INTEGER>)> {};
  struct RealLit : rule<assign<&ast::RealLit::value>(ref<REAL>)> {};
  struct StringLit
      : rule<assign<&ast::StringLit::value>(ref<STRING>)> {};
  struct NilLit : rule<create<ast::NilLit> + kw<"NIL">> {};
  struct BoolLit
      : rule<create<ast::BoolLit> +
             (flag<&ast::BoolLit::value>(kw<"TRUE">) |
              kw<"FALSE">)> {};

  struct SetElement
      : rule<create<ast::SetElement>(
            assign<&ast::SetElement::low>(ref<Expression>) +
            option(lit<".."> +
                   assign<&ast::SetElement::high>(ref<Expression>)))> {
  };
  struct SetLit
      : rule<create<ast::SetLit>(
            lit<"{"> +
            many(append<&ast::SetLit::elements>(ref<SetElement>),
                        lit<",">) +
            lit<"}">)> {};

  struct Factor
      : rule<ref<IntLit>                                          //
             | ref<RealLit>                                       //
             | ref<StringLit>                                     //
             | ref<NilLit>                                        //
             | ref<BoolLit>                                       //
             | ref<SetLit>                                        //
             | ref<DesignatorCall>                                //
             | (lit<"("> + ref<Expression> + lit<")">)             //
             | create<ast::UnaryExpr>(
                   assign<&ast::UnaryExpr::op>(lit<"~">) +
                   assign<&ast::UnaryExpr::operand>(ref<Factor>))> {};

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

  // -- statements ----

  struct AssignStmt
      : rule<create<ast::AssignStmt>(
            assign<&ast::AssignStmt::target>(ref<Designator>) +
            lit<":="> +
            assign<&ast::AssignStmt::value>(ref<Expression>))> {};

  struct ExprStmt
      : rule<assign<&ast::ExprStmt::value>(ref<DesignatorCall>)> {};

  struct IfElsif
      : rule<create<ast::ElsifClause>(
            kw<"ELSIF"> + assign<&ast::ElsifClause::cond>(ref<Expression>) +
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

  struct Label
      : rule<ref<IntLit> | ref<StringLit> |
             assign<&ast::NameExpr::name>(ref<Qualident>)> {};
  struct LabelRange
      : rule<create<ast::LabelRange>(
            assign<&ast::LabelRange::low>(ref<Label>) +
            option(lit<".."> +
                   assign<&ast::LabelRange::high>(ref<Label>)))> {};
  struct CaseArm
      : rule<create<ast::CaseArm>(option(
            some(append<&ast::CaseArm::labels>(ref<LabelRange>),
                 lit<",">) +
            lit<":"> +
            some(append<&ast::CaseArm::body>(ref<Statement>),
                 lit<";">)))> {};
  struct CaseStmt
      : rule<create<ast::CaseStmt>(
            kw<"CASE"> + assign<&ast::CaseStmt::value>(ref<Expression>) +
            kw<"OF"> +
            some(append<&ast::CaseStmt::arms>(ref<CaseArm>), lit<"|">) +
            kw<"END">)> {};

  struct WhileElsif
      : rule<create<ast::ElsifClause>(
            kw<"ELSIF"> + assign<&ast::ElsifClause::cond>(ref<Expression>) +
            kw<"DO"> +
            some(append<&ast::ElsifClause::body>(ref<Statement>),
                 lit<";">))> {};
  struct WhileStmt
      : rule<create<ast::WhileStmt>(
            kw<"WHILE"> +
            assign<&ast::WhileStmt::cond>(ref<Expression>) + kw<"DO"> +
            some(append<&ast::WhileStmt::body>(ref<Statement>),
                 lit<";">) +
            many(append<&ast::WhileStmt::elsifs>(ref<WhileElsif>)) +
            kw<"END">)> {};

  struct RepeatStmt
      : rule<create<ast::RepeatStmt>(
            kw<"REPEAT"> +
            some(append<&ast::RepeatStmt::body>(ref<Statement>),
                 lit<";">) +
            kw<"UNTIL"> +
            assign<&ast::RepeatStmt::cond>(ref<Expression>))> {};

  struct ForStmt
      : rule<create<ast::ForStmt>(
            kw<"FOR"> + assign<&ast::ForStmt::variable>(ref<IDENT>) +
            lit<":="> + assign<&ast::ForStmt::from>(ref<Expression>) +
            kw<"TO"> + assign<&ast::ForStmt::to>(ref<Expression>) +
            option(kw<"BY"> +
                   assign<&ast::ForStmt::by>(ref<Expression>)) +
            kw<"DO"> +
            some(append<&ast::ForStmt::body>(ref<Statement>), lit<";">) +
            kw<"END">)> {};

  struct Statement
      : rule<ref<AssignStmt>  //
             | ref<ExprStmt>  //
             | ref<IfStmt>    //
             | ref<CaseStmt>  //
             | ref<WhileStmt>  //
             | ref<RepeatStmt>  //
             | ref<ForStmt>  //
             | create<ast::EmptyStmt>> {};

  // -- types ----

  struct NamedType
      : rule<assign<&ast::NamedType::name>(ref<Qualident>)> {};

  struct ArrayType
      : rule<create<ast::ArrayType>(
            kw<"ARRAY"> +
            some(append<&ast::ArrayType::lengths>(ref<Expression>),
                 lit<",">) +
            kw<"OF"> +
            assign<&ast::ArrayType::component>(ref<Type>))> {};

  struct RecordField
      : rule<create<ast::RecordField>(
            some(append<&ast::RecordField::names>(ref<IdentDef>),
                 lit<",">) +
            lit<":"> + assign<&ast::RecordField::type>(ref<Type>))> {};

  struct RecordType
      : rule<create<ast::RecordType>(
            kw<"RECORD"> +
            option(lit<"("> + assign<&ast::RecordType::base>(
                                   ref<Qualident>) +
                   lit<")">) +
            many(append<&ast::RecordType::fields>(
                            ref<RecordField>),
                        lit<";">) +
            kw<"END">)> {};

  struct PointerType
      : rule<create<ast::PointerType>(
            kw<"POINTER"> + kw<"TO"> +
            assign<&ast::PointerType::target>(ref<Type>))> {};

  // Keep separators between ARRAY/OF and the following type. Joining data
  // tokens without them would conflate ARRAY OF CHAR with the name ARRAYOFCHAR.
  struct FormalType
      : rule<as<[](std::span<const std::string_view> parts,
                    std::pmr::memory_resource* mem) {
          std::size_t first = 0;
          while (first + 1 < parts.size() && parts[first] == "ARRAY" &&
                 parts[first + 1] == "OF") first += 2;
          std::size_t size = (first / 2) * 9;
          for (std::size_t i = first; i < parts.size(); ++i) size += parts[i].size();
          const auto room = cursiv::text_room(mem, size);
          std::size_t n = 0;
          for (std::size_t i = 0; i < first; i += 2)
            for (const char c : std::string_view("ARRAY OF ")) room[n++] = c;
          for (std::size_t i = first; i < parts.size(); ++i)
            for (const char c : parts[i]) room[n++] = c;
          return std::string_view(room.data(), n);
        }>(many(kw<"ARRAY"> + kw<"OF">) + ref<Qualident>)> {};

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
            option(lit<":"> +
                   assign<&ast::FormalParams::resultType>(
                       ref<Qualident>)))> {};

  struct ProcedureType
      : rule<create<ast::ProcedureType>(
            kw<"PROCEDURE"> +
            option(assign<&ast::ProcedureType::params>(
                ref<FormalParameters>)))> {};

  struct Type
      : rule<ref<NamedType> | ref<ArrayType> | ref<RecordType> |
             ref<PointerType> | ref<ProcedureType>> {};

  // -- declarations ----

  struct ConstDecl
      : rule<create<ast::ConstDecl>(
            assign<&ast::ConstDecl::name>(ref<IdentDef>) + lit<"="> +
            assign<&ast::ConstDecl::value>(ref<Expression>))> {};

  struct TypeDecl
      : rule<create<ast::TypeDecl>(
            assign<&ast::TypeDecl::name>(ref<IdentDef>) + lit<"="> +
            assign<&ast::TypeDecl::type>(ref<Type>))> {};

  struct VarDecl
      : rule<create<ast::VarDecl>(
            some(append<&ast::VarDecl::names>(ref<IdentDef>), lit<",">) +
            lit<":"> + assign<&ast::VarDecl::type>(ref<Type>))> {};

  struct ProcDecl
      : rule<create<ast::ProcDecl>(
            kw<"PROCEDURE"> +
            assign<&ast::ProcDecl::name>(ref<IdentDef>) +
            option(assign<&ast::ProcDecl::params>(
                ref<FormalParameters>)) +
            lit<";"> +
            option(kw<"CONST"> +
                   many(append<&ast::ProcDecl::decls>(ref<ConstDecl>) +
                        lit<";">)) +
            option(kw<"TYPE"> +
                   many(append<&ast::ProcDecl::decls>(ref<TypeDecl>) +
                        lit<";">)) +
            option(kw<"VAR"> +
                   many(append<&ast::ProcDecl::decls>(ref<VarDecl>) +
                        lit<";">)) +
            many(append<&ast::ProcDecl::decls>(ref<ProcDecl>) +
                 lit<";">) +
            option(kw<"BEGIN"> +
                   some(append<&ast::ProcDecl::body>(ref<Statement>),
                        lit<";">)) +
            option(kw<"RETURN"> +
                   assign<&ast::ProcDecl::returnExpr>(ref<Expression>)) +
            kw<"END"> +
            assign<&ast::ProcDecl::endName>(ref<IDENT>))> {};

  struct Import
      : rule<create<ast::Import>(
            assign<&ast::Import::name>(ref<IDENT>) +
            option(lit<":="> +
                   assign<&ast::Import::actual>(ref<IDENT>)))> {};

  struct Module
      : rule<create<ast::Module>(
            kw<"MODULE"> + assign<&ast::Module::name>(ref<IDENT>) +
            lit<";"> +
            option(kw<"IMPORT"> +
                   some(append<&ast::Module::imports>(ref<Import>),
                        lit<",">) +
                   lit<";">) +
            option(kw<"CONST"> +
                   many(append<&ast::Module::decls>(ref<ConstDecl>) +
                        lit<";">)) +
            option(kw<"TYPE"> +
                   many(append<&ast::Module::decls>(ref<TypeDecl>) +
                        lit<";">)) +
            option(kw<"VAR"> +
                   many(append<&ast::Module::decls>(ref<VarDecl>) +
                        lit<";">)) +
            many(append<&ast::Module::decls>(ref<ProcDecl>) + lit<";">) +
            option(kw<"BEGIN"> +
                   some(append<&ast::Module::body>(ref<Statement>),
                        lit<";">)) +
            kw<"END"> + assign<&ast::Module::endName>(ref<IDENT>) +
            lit<".">)> {};

  using entry = Module;
  using ignored = WS;
  using hidden = list<COMMENT>;
};

}  // namespace oberon
