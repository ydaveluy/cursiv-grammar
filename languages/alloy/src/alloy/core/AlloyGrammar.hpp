#pragma once

#include <cstdint>
#include <memory_resource>
#include <string_view>

#include "alloy/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace alloy {

struct Alloy : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"(\u0000-\u0020)">)> {};
  struct SL_COMMENT : terminal<(lit<"//"> | lit<"--">)+many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  static constexpr auto IdentifierStart =
      uc::L | uc::Nl | uc::Sc | uc::Pc | cr<"\u1885\u1886\u30FB\uFF65">;
  static constexpr auto IdentifierPart = IdentifierStart | uc::Nd | uc::Mn |
                                         uc::Mc | uc::Cf | cr<"\u1369-\u1371"> |
                                         cr<R"('"\u007F-\u009F)">;
  static constexpr auto word_boundary = IdentifierPart;
  static constexpr auto Reserved =
      (lit<"abstract"> | lit<"all"> | lit<"and"> | lit<"as"> | lit<"assert"> |
       lit<"but"> | lit<"check"> | lit<"disj"> | lit<"disjoint"> | lit<"else"> |
       lit<"enum"> | lit<"exactly"> | lit<"expect"> | lit<"extends"> |
       lit<"fact"> | lit<"for"> | lit<"fun"> | lit<"iden"> | lit<"iff"> |
       lit<"implies"> | lit<"in"> | lit<"int"> | lit<"Int"> | lit<"let"> |
       lit<"lone"> | lit<"module"> | lit<"no"> | lit<"none"> | lit<"not"> |
       lit<"one"> | lit<"open"> | lit<"or"> | lit<"pred"> | lit<"private"> |
       lit<"run"> | lit<"seq"> | lit<"set"> | lit<"sig"> | lit<"some"> |
       lit<"String"> | lit<"sum"> | lit<"this"> | lit<"univ">)+!IdentifierPart;
  struct ID : terminal<!Reserved + IdentifierStart + many(IdentifierPart)> {};
  struct NUMBER : terminal<as<std::int64_t>(some(cr<"0-9">))> {};
  struct STRING : terminal<lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\)">) +
                           lit<"\"">> {};
  inline static constexpr auto string_name =
      [](std::string_view text, std::pmr::memory_resource *resource) {
        auto *value = static_cast<char *>(resource->allocate(text.size()));
        std::size_t size = 0;
        for (std::size_t i = 0; i < text.size(); ++i) {
          unsigned char c = text[i];
          if (c == '\\' && i + 1 < text.size()) {
            c = text[++i];
            if (c == 'n')
              c = '\n';
          } else if (c < 32 && c != '\r' && c != '\n' && c != '\t')
            c = ' ';
          value[size++] = static_cast<char>(c);
        }
        return std::string_view(value, size);
      };
  // Quoted paragraph names pass through the native string lexer too;
  // their raw token is otherwise absent from the semantic AST validator.
  static constexpr auto NameEscape = lit<"\\"> + cr<R"(n'"\\)">;
  struct QuotedName : terminal<as<string_name>(
      lit<"\""> + some(NameEscape | cr<R"(^"\\\r\n)">) + lit<"\"">)> {};
  struct QualName : rule<option(lit<"this/"> | lit<"seq/">) + ref<ID> +
                         many(lit<"/"> + ref<ID>)> {};
  struct SigName : rule<ref<QualName> | kw<"Int"> | kw<"String"> | kw<"univ"> |
                        kw<"none"> | kw<"seq/Int">> {};

  struct CmpNotGt
      : rule<as<ast::CompareOp::not_gt>((lit<"!"> | kw<"not">)+lit<">">)> {};
  struct CmpNotGe
      : rule<as<ast::CompareOp::not_ge>((lit<"!"> | kw<"not">)+lit<">=">)> {};
  static constexpr auto CompareOp = as<ast::CompareOp::neq>((lit<"!"> | kw<"not">)+lit<"=">) | as<ast::CompareOp::eq>(lit<"=">) | as<ast::CompareOp::not_lt>((lit<"!"> | kw<"not">)+lit<"<">) |
                                    as<ast::CompareOp::lt>(lit<"<">) | ref<CmpNotGt> | as<ast::CompareOp::gt>(lit<">">) |
                                    as<ast::CompareOp::not_le>((lit<"!"> | kw<"not">)+(lit<"=<"> | lit<"<=">)) | as<ast::CompareOp::le>((lit<"=<"> | lit<"<=">)) | ref<CmpNotGe> |
                                    as<ast::CompareOp::ge>(lit<">=">) | as<ast::CompareOp::not_in>((lit<"!"> | kw<"not">)+kw<"in">) | as<ast::CompareOp::in>(kw<"in">);

  static constexpr auto Mult = as<ast::Mult::lone>(kw<"lone">) | as<ast::Mult::some>(kw<"some">) | as<ast::Mult::one>(kw<"one">);
  static constexpr auto MultOrSet =
      as<ast::Mult::lone>(kw<"lone">) | as<ast::Mult::some>(kw<"some">) | as<ast::Mult::one>(kw<"one">) | as<ast::Mult::set>(kw<"set">);

  static constexpr auto Quant = as<ast::Quantifier::all>(kw<"all">) | as<ast::Quantifier::no>(kw<"no">) | as<ast::Quantifier::sum>(kw<"sum">) |
                                as<ast::Quantifier::lone>(kw<"lone">) | as<ast::Quantifier::some>(kw<"some">) | as<ast::Quantifier::one>(kw<"one">);


  struct Expr;
  struct Iff;
  struct Implies;
  struct And;
  struct GeneralNeg;
  struct CompareExpr;
  struct QuantPrefix;
  struct AddSub;
  struct Cardinality;
  struct Multiply;
  struct Shift;
  struct Range;
  struct IntersectOverride;
  struct ArrowExpr;
  struct Restrict;
  struct BoxJoin;
  struct UnaryTight;
  struct PrimaryExpr;
  struct Decl;
  struct ComprehensionDecl;
  struct LetDecl;
  struct Block;
  struct BlockOrBar;

  struct DeclNames
      : rule<create<ast::Decl> +
             option(flag<&ast::Decl::private_>(kw<"private">)) +
             option(flag<&ast::Decl::namesDisjoint>(kw<"disj"> |
                                                         kw<"disjoint">)) +
             some(append<&ast::Decl::names>(ref<ID>), lit<",">)> {};
  static constexpr auto DeclType =
      option(flag<&ast::Decl::typeDisjoint>(kw<"disj"> | kw<"disjoint">)) +
      assign<&ast::Decl::type>(ref<Expr>);
  struct Decl
      : rule<ref<DeclNames> +
             (lit<":"> | flag<&ast::Decl::defined>(lit<"=">)) + DeclType> {
  };
  struct ComprehensionDecl
      : rule<ref<DeclNames> + lit<":"> + DeclType> {};

  struct LetDecl
      : rule<create<ast::LetDecl> + assign<&ast::LetDecl::name>(ref<ID>) +
             lit<"="> + assign<&ast::LetDecl::value>(ref<Expr>)> {};

  struct Block
      : rule<create<ast::Block> + lit<"{"> +
             many(append<&ast::Block::exprs>(ref<Expr>)) + lit<"}">> {};
  struct BlockOrBar
      : rule<ref<Block> |
             create<ast::Block>(lit<"|"> +
                                append<&ast::Block::exprs>(ref<Expr>))> {};

  struct PrimaryExpr
      : rule<create<ast::NumberExpr>(
                 option(flag<&ast::NumberExpr::negative>(lit<"-">)) +
                 assign<&ast::NumberExpr::value>(ref<NUMBER>)) |
             create<ast::StringExpr>(
                 assign<&ast::StringExpr::raw>(ref<STRING>)) |
             create<ast::NoneExpr>(kw<"none">) |
             create<ast::UnivExpr>(kw<"univ">) |
             create<ast::IdenExpr>(kw<"iden">) |
             create<ast::FieldExpr>(
                 lit<"@"> + assign<&ast::FieldExpr::name>(ref<QualName>)) |
             create<ast::ThisExpr>(kw<"this">) |
             create<ast::LetExpr>(
                 kw<"let"> +
                 some(append<&ast::LetExpr::decls>(ref<LetDecl>),
                      lit<",">) +
                 assign<&ast::LetExpr::body>(ref<BlockOrBar>)) |
             create<ast::QuantExpr>(
                 assign<&ast::QuantExpr::quant>(Quant) +
                 some(append<&ast::QuantExpr::decls>(ref<Decl>), lit<",">) +
                 assign<&ast::QuantExpr::body>(ref<BlockOrBar>)) |
             create<ast::SetComprExpr>(
                 lit<"{"> +
                 some(append<&ast::SetComprExpr::decls>(
                          ref<ComprehensionDecl>),
                      lit<",">) +
                 option(assign<&ast::SetComprExpr::body>(ref<BlockOrBar>)) +
                 lit<"}">) |
             (lit<"("> + ref<Expr> + lit<")">) | ref<Block> |
             create<ast::NameExpr>(assign<&ast::NameExpr::name>(
                 ref<SigName> | kw<"disj"> | kw<"disjoint"> |
                 kw<"pred/totalOrder"> | kw<"fun/min"> | kw<"fun/max"> |
                 kw<"fun/next"> | kw<"int"> | kw<"sum">))> {};
  struct UnaryTight
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(as<ast::UnaryOp::transpose>(lit<"~">) |
                                             as<ast::UnaryOp::rclosure>(lit<"*">) | as<ast::UnaryOp::closure>(lit<"^">)) +
                 assign<&ast::UnaryExpr::operand>(ref<UnaryTight>)) |
             ref<PrimaryExpr>> {};

  struct BoxJoin
      : rule<ref<UnaryTight> +
             many(nest<ast::UnaryExpr, &ast::UnaryExpr::operand>(
                      (lit<"."> |
                       lit<"::">)+assign<&ast::UnaryExpr::op>(as<ast::UnaryOp::integer>(kw<"sum"> | kw<"int">))) |
                  nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                      assign<&ast::BinaryExpr::op>(as<ast::BinOp::join>(lit<"."> | lit<"::">)) +
                      assign<&ast::BinaryExpr::right>(ref<UnaryTight>)) |
                  nest<ast::BoxJoinExpr, &ast::BoxJoinExpr::callee>(
                      lit<"["> +
                      many(append<&ast::BoxJoinExpr::args>(ref<Expr>),
                           lit<",">) +
                      lit<"]">))> {};

  struct Range : infix<BoxJoin, ast::BinaryExpr,
                       left(as<ast::BinOp::range_restrict>(lit<":>">))> {};
  struct Restrict : infix<Range, ast::BinaryExpr,
                          left(as<ast::BinOp::domain_restrict>(lit<"<:">))> {};

  struct ArrowExpr
      : rule<ref<Restrict> +
             option(nest<ast::ArrowExpr, &ast::ArrowExpr::left>(
                 option(assign<&ast::ArrowExpr::leftMult>(MultOrSet)) +
                 lit<"->"> +
                 option(assign<&ast::ArrowExpr::rightMult>(MultOrSet)) +
                 assign<&ast::ArrowExpr::right>(ref<ArrowExpr>)))> {};

  struct IntersectOverride
      : infix<ArrowExpr, ast::BinaryExpr,
              levels(left(as<ast::BinOp::intersect>(lit<"&">)), left(as<ast::BinOp::override_>(lit<"++">)))> {};
  struct Cardinality
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(as<ast::UnaryOp::card>(lit<"#">) | as<ast::UnaryOp::integer>(kw<"sum"> | kw<"int">)) +
                 assign<&ast::UnaryExpr::operand>(ref<Cardinality>)) |
             ref<IntersectOverride>> {};

  struct Multiply
      : infix<Cardinality, ast::BinaryExpr,
              left(as<ast::BinOp::multiply>(kw<"fun/mul">) | as<ast::BinOp::divide>(kw<"fun/div">) | as<ast::BinOp::remainder>(kw<"fun/rem">))> {};
  struct AddSub : infix<Multiply, ast::BinaryExpr,
                            left(as<ast::BinOp::add>(lit<"+">) | as<ast::BinOp::subtract>(lit<"-">) |
                                 as<ast::BinOp::integer_add>(kw<"fun/add">) | as<ast::BinOp::integer_subtract>(kw<"fun/sub">))> {
  };
  struct Shift : infix<AddSub, ast::BinaryExpr,
                           left(as<ast::BinOp::shift_left>(lit<"<<">) | as<ast::BinOp::shift_right>(lit<">>>">) |
                                as<ast::BinOp::shift_arithmetic>(lit<">>">))> {};
  struct QuantPrefix
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(as<ast::UnaryOp::no>(kw<"no">) | as<ast::UnaryOp::lone>(kw<"lone">) |
                                             as<ast::UnaryOp::some>(kw<"some">) | as<ast::UnaryOp::one>(kw<"one">) |
                                             as<ast::UnaryOp::set>(kw<"set">) | as<ast::UnaryOp::sequence>(kw<"seq">)) +
                 assign<&ast::UnaryExpr::operand>(ref<QuantPrefix>)) |
             ref<Shift>> {};

  struct CompareExpr
      : infix<QuantPrefix, ast::CompareExpr, left(CompareOp)> {};
  struct GeneralNeg
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(as<ast::UnaryOp::not_>(lit<"!"> | kw<"not">)) +
                 assign<&ast::UnaryExpr::operand>(ref<GeneralNeg>)) |
             ref<CompareExpr>> {};

  struct And : infix<GeneralNeg, ast::BinaryExpr,
                     left(as<ast::BinOp::and_>(lit<"&&"> | kw<"and">))> {};
  struct Implies
      : rule<ref<And> +
             option(nest<ast::IfExpr, &ast::IfExpr::cond>(
                 (lit<"=>"> |
                  kw<"implies">)+assign<&ast::IfExpr::then>(ref<Implies>) +
                 option(kw<"else"> +
                        assign<&ast::IfExpr::else_>(ref<Implies>))))> {};

  struct Iff : infix<Implies, ast::BinaryExpr,
                     left(as<ast::BinOp::iff>(lit<"<=>"> | kw<"iff">))> {};
  struct Expr : infix<Iff, ast::BinaryExpr,
                      left(as<ast::BinOp::or_>(lit<"||"> | kw<"or">))> {};

  struct SigExt
      : rule<create<ast::SigExtends>(
                 kw<"extends"> +
                 assign<&ast::SigExtends::target>(ref<SigName>)) |
             create<ast::SigIn>(
                 (kw<"in"> | flag<&ast::SigIn::exact>(lit<"=">)) +
                 some(append<&ast::SigIn::targets>(ref<SigName>), lit<"+">))> {
  };

  struct SigDecl
      : rule<create<ast::SigDecl>(
            many(append<&ast::SigDecl::qualifiers>(
                as<ast::SigQualifier::abstract_>(kw<"abstract">) | as<ast::SigQualifier::lone>(kw<"lone">) | as<ast::SigQualifier::one>(kw<"one">) |
                as<ast::SigQualifier::some>(kw<"some">) | as<ast::SigQualifier::private_>(kw<"private">))) +
            kw<"sig"> + some(append<&ast::SigDecl::names>(ref<ID>), lit<",">) +
            option(assign<&ast::SigDecl::ext>(ref<SigExt>)) + lit<"{"> +
            many(lit<",">) +
            option(append<&ast::SigDecl::decls>(ref<Decl>) +
                   many(lit<","> +
                        option(append<&ast::SigDecl::decls>(ref<Decl>)))) +
            lit<"}"> + option(assign<&ast::SigDecl::facts>(ref<Block>)))> {
  };

  struct FactDecl
      : rule<create<ast::FactDecl>(
            kw<"fact"> +
            option(assign<&ast::FactDecl::name>(ref<ID> | ref<QuotedName>)) +
            assign<&ast::FactDecl::body>(ref<Block>))> {};

  struct PredDecl
      : rule<create<ast::PredDecl>(
            option(flag<&ast::PredDecl::private_>(kw<"private">)) +
            kw<"pred"> +
            option(assign<&ast::PredDecl::qualifier>(ref<SigName>) + lit<".">) +
            assign<&ast::PredDecl::name>(ref<ID>) +
            option(
                (lit<"("> + many(lit<",">) +
                 option(append<&ast::PredDecl::params>(ref<Decl>) +
                        many(lit<","> + option(append<&ast::PredDecl::params>(
                                            ref<Decl>)))) +
                 lit<")">) |
                (lit<"["> + many(lit<",">) +
                 option(append<&ast::PredDecl::params>(ref<Decl>) +
                        many(lit<","> + option(append<&ast::PredDecl::params>(
                                            ref<Decl>)))) +
                 lit<"]">)) +
            assign<&ast::PredDecl::body>(ref<Block>))> {};

  struct FunDecl
      : rule<create<ast::FunDecl>(
            option(flag<&ast::FunDecl::private_>(kw<"private">)) +
            kw<"fun"> +
            option(assign<&ast::FunDecl::qualifier>(ref<SigName>) + lit<".">) +
            assign<&ast::FunDecl::name>(ref<ID>) +
            option((lit<"("> + many(lit<",">) +
                    option(append<&ast::FunDecl::params>(ref<Decl>) +
                           many(lit<","> + option(append<&ast::FunDecl::params>(
                                               ref<Decl>)))) +
                    lit<")">) |
                   (lit<"["> + many(lit<",">) +
                    option(append<&ast::FunDecl::params>(ref<Decl>) +
                           many(lit<","> + option(append<&ast::FunDecl::params>(
                                               ref<Decl>)))) +
                    lit<"]">)) +
            lit<":"> + assign<&ast::FunDecl::returnType>(ref<Expr>) +
            assign<&ast::FunDecl::body>(ref<Block>))> {};

  struct AssertDecl
      : rule<create<ast::AssertDecl>(
            kw<"assert"> +
            option(assign<&ast::AssertDecl::name>(ref<ID> | ref<QuotedName>)) +
            assign<&ast::AssertDecl::body>(ref<Block>))> {};

  struct TypeScope
      : rule<create<ast::TypeScope>(
            option(flag<&ast::TypeScope::exactly_>(kw<"exactly">)) +
            assign<&ast::TypeScope::count>(ref<NUMBER>) +
            option(lit<".."> + assign<&ast::TypeScope::end>(ref<NUMBER>)) +
            option(lit<":"> + assign<&ast::TypeScope::step>(ref<NUMBER>)) +
            assign<&ast::TypeScope::type>(ref<SigName> | kw<"int"> |
                                          kw<"seq">))> {};

  struct Scope
      : rule<create<ast::Scope>(
            kw<"for"> +
            (some(append<&ast::Scope::typescopes>(ref<TypeScope>),
                  lit<",">) |
             (assign<&ast::Scope::overall>(ref<NUMBER>) +
                 option(kw<"but"> + some(append<&ast::Scope::typescopes>(
                                             ref<TypeScope>),
                                         lit<",">)))))> {};

  struct CmdDecl
      : rule<create<ast::CmdDecl>(
            option(assign<&ast::CmdDecl::name>(ref<ID>) + lit<":">) +
            option(assign<&ast::CmdDecl::kind>(as<ast::CmdKind::run>(kw<"run">) | as<ast::CmdKind::check>(kw<"check">))) +
            assign<&ast::CmdDecl::target>(ref<QualName> | ref<Block>) +
            option(assign<&ast::CmdDecl::scope>(ref<Scope>)) +
            option(kw<"expect"> + assign<&ast::CmdDecl::expects>(ref<NUMBER>)) +
            option(lit<"=>"> +
                   assign<&ast::CmdDecl::next>(ref<CmdDecl>)))> {};

  struct EnumDecl
      : rule<create<ast::EnumDecl> +
             option(flag<&ast::EnumDecl::private_>(kw<"private">)) +
             kw<"enum"> + assign<&ast::EnumDecl::name>(ref<ID>) + lit<"{"> +
             many(append<&ast::EnumDecl::members>(ref<ID>), lit<",">) +
             lit<"}">> {};
  struct MacroDecl
      : rule<create<ast::MacroDecl> +
             option(flag<&ast::MacroDecl::private_>(kw<"private">)) +
             kw<"let"> + assign<&ast::MacroDecl::name>(ref<ID>) +
             option(
                 (lit<"("> +
                     many(append<&ast::MacroDecl::params>(ref<ID>), lit<",">) +
                     lit<")">) |
                 (lit<"["> +
                     many(append<&ast::MacroDecl::params>(ref<ID>), lit<",">) +
                     lit<"]">)) +
             assign<&ast::MacroDecl::body>(ref<Block> |
                                           (lit<"="> + ref<Expr>))> {};

  struct Paragraph
      : rule<ref<SigDecl> | ref<FactDecl> | ref<PredDecl> |
             ref<FunDecl> | ref<AssertDecl> | ref<EnumDecl> |
             ref<MacroDecl> | ref<CmdDecl>> {};

  struct Import
      : rule<create<ast::Import>(
            option(flag<&ast::Import::private_>(kw<"private">)) +
            kw<"open"> +
            some(append<&ast::Import::paths>(ref<QualName>), lit<",">) +
            option(lit<"["> +
                   many(append<&ast::Import::args>(ref<SigName>), lit<",">) +
                   lit<"]">) +
            option(kw<"as"> + assign<&ast::Import::alias>(ref<ID>)))> {};

  struct ModuleParam
      : rule<create<ast::ModuleParam> +
             option(flag<&ast::ModuleParam::exactly_>(kw<"exactly">)) +
             assign<&ast::ModuleParam::name>(ref<QualName>)> {};
  struct ModuleDecl
      : rule<create<ast::ModuleDecl>(
            kw<"module"> + assign<&ast::ModuleDecl::path>(ref<QualName>) +
            option(lit<"["> +
                   many(append<&ast::ModuleDecl::names>(ref<ModuleParam>),
                        lit<",">) +
                   lit<"]">))> {};

  struct AlloyModule
      : rule<create<ast::Module> +
             option(assign<&ast::Module::moduleDecl>(ref<ModuleDecl>)) +
             many(append<&ast::Module::imports>(ref<Import>) |
                  append<&ast::Module::paragraphs>(ref<Paragraph>))> {};

  using entry = AlloyModule;
  using ignored = list<WS, SL_COMMENT, ML_COMMENT>;
};

} // namespace alloy
