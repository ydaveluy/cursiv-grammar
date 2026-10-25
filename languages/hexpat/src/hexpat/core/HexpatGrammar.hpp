// The hexpat language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "hexpat/core/ast.hpp"

namespace hexpat {

struct Hexpat : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  struct PREPROCESSOR : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};

  struct ID : terminal<cr<"a-zA-Z_"> + many(w)> {};
  struct PRIMITIVE
      : terminal<kw<"u128"> | kw<"u64"> | kw<"u32"> | kw<"u16"> | kw<"u8"> |
                 kw<"s128"> | kw<"s64"> | kw<"s32"> | kw<"s16"> | kw<"s8"> |
                 kw<"double"> | kw<"float"> | kw<"char"> | kw<"str"> |
                 kw<"bool"> | kw<"auto">> {};

  // A digit run with an optional single `'` separator between digits, never
  // leading or trailing.
  static constexpr auto HexDigits =
      cr<"0-9a-fA-F"> + many(option(lit<"'">) + cr<"0-9a-fA-F">);
  static constexpr auto OctDigits =
      cr<"0-7"> + many(option(lit<"'">) + cr<"0-7">);
  static constexpr auto BinDigits =
      cr<"01"> + many(option(lit<"'">) + cr<"01">);

  struct INT_LIT : terminal<(lit<"0"> + cr<"xX"> + HexDigits) |
                            (lit<"0"> + cr<"oO"> + OctDigits) |
                            (lit<"0"> + cr<"bB"> + BinDigits) | some(d)> {};

  struct FLOAT_LIT : terminal<many(d) + lit<".", !lit<".">> + some(d) +
                              option(cr<"eE"> + option(cr<"+-">) + some(d))> {};

  static constexpr auto OctalEscape =
      (cr<"0-3"> + repeat<2>(cr<"0-7">)) | (cr<"0-7"> + option(cr<"0-7">));
  static constexpr auto CharEscape =
      lit<"\\"> + (cr<R"(btnfr"'\\\\)"> | OctalEscape |
                   (lit<"x"> + repeat<2>(cr<"0-9a-fA-F">)) | any);
  struct CHAR_LIT
      : terminal<lit<"'"> + (CharEscape | (!cr<R"('\\\\)"> + any)) + lit<"'">> {
  };

  static constexpr auto StringEscape = lit<"\\"> + any;
  struct STRING_LIT
      : terminal<lit<"\""> + many(StringEscape | (!cr<R"("\\\\)"> + any)) +
                 lit<"\"">> {};

  static constexpr auto EndianKw = kw<"le"> | kw<"be">;
  static constexpr auto IoKw = kw<"in"> | kw<"out">;

  static constexpr auto OpMul = lit<"*"> | lit<"/"> | lit<"%">;
  static constexpr auto OpAdd = lit<"+"> | lit<"-">;
  static constexpr auto OpShift = lit<">>>"> | lit<"<<"> | lit<">>">;
  static constexpr auto OpRel = lit<"<="> | lit<">="> | lit<">"> | lit<"<">;
  static constexpr auto OpEq = lit<"=="> | lit<"!=">;
  static constexpr auto OpXorXor = lit<"^^">;
  static constexpr auto OpBAnd = lit<"&", !lit<"&">>;
  static constexpr auto OpBXor = lit<"^", !lit<"^">>;
  static constexpr auto OpBOr = lit<"|", !lit<"|">>;
  static constexpr auto OpLAnd = lit<"&&">;
  static constexpr auto OpLOr = lit<"||">;

  static constexpr auto OpAssign = lit<"<<="> | lit<">>>="> | lit<">>="> |
                                   lit<"+="> | lit<"-="> | lit<"*="> |
                                   lit<"/="> | lit<"&="> | lit<"|="> |
                                   lit<"^="> | lit<"%="> | lit<"=", !lit<"=">>;

  struct Expression;
  struct Ternary;
  struct InfixExpr;
  struct Unary;
  struct Postfix;
  struct Primary;
  struct ParExpr;
  struct TypeNameRule;
  struct TypeArgRule;
  struct WhileSizeRule;
  struct AtRule;
  struct AttributeRule;
  struct PatternItemRule;
  struct PatternElemRule;
  struct PaddingRule;
  struct Block;
  struct StructBlockRule;
  struct UnionBlockRule;
  struct BitfieldBlockRule;
  struct StructStatementRule;
  struct UnionStatementRule;
  struct BitfieldStatementRule;
  struct StmtRule;
  struct StructMemberRule;
  struct NamespaceRule;

  // ---- expressions ----

  struct IdentExpr : rule<assign<&ast::Ident::name>(ref<ID>)> {};
  struct IntLitExpr : rule<assign<&ast::IntLit::text>(ref<INT_LIT>)> {};
  struct FloatLitExpr : rule<assign<&ast::FloatLit::text>(ref<FLOAT_LIT>)> {};
  struct CharLitExpr : rule<assign<&ast::CharLit::text>(ref<CHAR_LIT>)> {};
  struct StringLitExpr : rule<assign<&ast::StringLit::text>(ref<STRING_LIT>)> {
  };
  struct BoolLitExpr
      : rule<create<ast::BoolLit> +
             (flag<&ast::BoolLit::value>(kw<"true">) | kw<"false">)> {};

  struct Primary
      : rule<(lit<"("> + ref<Expression> + lit<")">) //
             | ref<FloatLitExpr>                     //
             | ref<IntLitExpr>                       //
             | ref<CharLitExpr>                      //
             | ref<StringLitExpr>                    //
             | ref<BoolLitExpr>                      //
             | create<ast::DollarExpr>(lit<"$">)     //
             | create<ast::AddressofExpr>(
                   kw<"addressof"> + lit<"("> +
                   assign<&ast::AddressofExpr::operand>(ref<Expression>) +
                   lit<")">) //
             | create<ast::SizeofExpr>(
                   kw<"sizeof"> + lit<"("> +
                   (assign<&ast::SizeofExpr::operand>(ref<Expression>) |
                    assign<&ast::SizeofExpr::primType>(ref<PRIMITIVE>)) +
                   lit<")">) //
             | create<ast::TypenameofExpr>(
                   kw<"typenameof"> + lit<"("> +
                   assign<&ast::TypenameofExpr::operand>(ref<Expression>) +
                   lit<")">) //
             | create<ast::CastCallExpr>(
                   option(assign<&ast::CastCallExpr::endian>(EndianKw)) +
                   assign<&ast::CastCallExpr::type>(ref<PRIMITIVE>) + lit<"("> +
                   assign<&ast::CastCallExpr::operand>(ref<Expression>) +
                   lit<")">) //
             | ref<IdentExpr>> {};

  struct Postfix
      : rule<ref<Primary> +
             many(nest<ast::IndexExpr, &ast::IndexExpr::array>(
                      lit<"["> +
                      assign<&ast::IndexExpr::index>(ref<Expression>) +
                      lit<"]">) //
                  |
                  nest<ast::CallExpr, &ast::CallExpr::callee>(
                      lit<"("> +
                      option(some(append<&ast::CallExpr::args>(ref<Expression>),
                                  lit<",">)) +
                      lit<")">) //
                  | nest<ast::MemberExpr, &ast::MemberExpr::object>(
                        lit<"."> + assign<&ast::MemberExpr::field>(ref<ID>)) //
                  | nest<ast::MemberExpr, &ast::MemberExpr::object>(
                        flag<&ast::MemberExpr::scoped>(lit<"::">) +
                        assign<&ast::MemberExpr::field>(ref<ID>)))> {};

  struct Unary : rule<create<ast::UnaryExpr>(
                          assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-">) +
                          assign<&ast::UnaryExpr::operand>(ref<Unary>)) //
                      | create<ast::UnaryExpr>(
                            assign<&ast::UnaryExpr::op>(lit<"~"> | lit<"!">) +
                            assign<&ast::UnaryExpr::operand>(ref<Unary>)) //
                      | ref<Postfix>> {};

  struct InfixExpr : infix<Unary, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
                           &ast::BinaryExpr::right,
                           (left(OpMul) >>    //
                            left(OpAdd) >>    //
                            left(OpShift) >>  //
                            left(OpRel) >>    //
                            left(OpEq) >>     //
                            left(OpXorXor) >> //
                            left(OpBAnd) >>   //
                            left(OpBXor) >>   //
                            left(OpBOr) >>    //
                            left(OpLAnd) >>   //
                            left(OpLOr))> {};

  struct Ternary
      : rule<create<ast::CondExpr>(
                 assign<&ast::CondExpr::cond>(ref<InfixExpr>) + lit<"?"> +
                 assign<&ast::CondExpr::thenValue>(ref<Expression>) + lit<":"> +
                 assign<&ast::CondExpr::elseValue>(ref<Ternary>)) //
             | ref<InfixExpr>> {};

  struct Expression
      : rule<create<ast::AssignExpr>(
                 assign<&ast::AssignExpr::target>(ref<Unary>) +
                 assign<&ast::AssignExpr::op>(OpAssign) +
                 assign<&ast::AssignExpr::value>(ref<Expression>)) //
             | ref<Ternary>> {};

  struct ParExpr : rule<lit<"("> + ref<Expression> + lit<")">> {};

  // ---- types, attributes, addresses, patterns ----

  struct TypeNameRule
      : rule<many(append<&ast::TypeRef::path>(ref<ID>) + lit<"::">) +
             (assign<&ast::TypeRef::name>(ref<PRIMITIVE>) |
              assign<&ast::TypeRef::name>(ref<ID>))> {};

  struct TypeArgRule
      : rule<(option(flag<&ast::TypeArg::isAuto>(kw<"auto">)) +
              assign<&ast::TypeArg::type>(ref<TypeNameRule>) +
              option(lit<"<"> +
                     some(append<&ast::TypeArg::typeArgs>(ref<TypeArgRule>),
                          lit<",">) +
                     lit<">">)) //
             | assign<&ast::TypeArg::expr>(ref<Expression>)> {};

  struct TypeParamRule
      : rule<option(flag<&ast::TypeParam::isAuto>(kw<"auto">)) +
             assign<&ast::TypeParam::type>(ref<TypeNameRule>)> {};

  struct AttrItemRule
      : rule<(assign<&ast::AttrItem::type>(ref<TypeNameRule>) + lit<"("> +
              option(some(append<&ast::AttrItem::args>(ref<Expression>),
                          lit<",">)) +
              lit<")">) //
             | assign<&ast::AttrItem::name>(ref<ID>)> {};

  struct AttributeRule
      : rule<lit<"["> + lit<"["> +
             some(append<&ast::Attribute::items>(ref<AttrItemRule>), lit<",">) +
             lit<"]"> + lit<"]">> {};

  struct WhileSizeRule
      : rule<kw<"while"> + lit<"("> + ref<Expression> + lit<")">> {};

  struct AtRule
      : rule<lit<"@"> +
             ((assign<&ast::At::intText>(ref<INT_LIT>) +
               option(kw<"in"> + assign<&ast::At::inSection>(ref<ID>))) //
              | assign<&ast::At::expr>(ref<Expression>))> {};

  struct PatternItemRule
      : rule<create<ast::PatternItem>(
            assign<&ast::PatternItem::from>(ref<Expression>) +
            option(lit<"..."> +
                   assign<&ast::PatternItem::to>(ref<Expression>)))> {};

  struct PatternElemRule
      : rule<some(append<&ast::PatternElem::alts>(ref<PatternItemRule>),
                  lit<"|">)> {};

  // ---- declarators ----

  struct StructMemberDeclaratorRule
      : rule<option(
                 flag<&ast::StructMemberDeclarator::isPointer>(lit<"*">)) +
             option(assign<&ast::StructMemberDeclarator::name>(ref<ID>)) +
             option(flag<&ast::StructMemberDeclarator::hasBrackets>(
                        lit<"[">) +
                    option(assign<&ast::StructMemberDeclarator::size>(
                               ref<Expression>) |
                           assign<&ast::StructMemberDeclarator::sizeWhile>(
                               ref<WhileSizeRule>)) +
                    lit<"]">) +
             option(lit<":"> + assign<&ast::StructMemberDeclarator::colonType>(
                                   ref<TypeNameRule>)) +
             option(lit<"="> + assign<&ast::StructMemberDeclarator::init>(
                                   ref<Expression>)) +
             option(assign<&ast::StructMemberDeclarator::at>(ref<AtRule>)) +
             option(assign<&ast::StructMemberDeclarator::attr>(
                 ref<AttributeRule>))> {};

  struct PaddingRule
      : rule<kw<"padding"> + lit<"["> +
             (assign<&ast::Padding::size>(ref<Expression>) |
              assign<&ast::Padding::whileCond>(ref<WhileSizeRule>)) +
             lit<"]">> {};

  struct StructMemberDeclRule
      : rule<option(flag<&ast::StructMember::isConst>(kw<"const">)) +
             option(assign<&ast::StructMember::endian>(EndianKw)) +
             option(assign<&ast::StructMember::type>(ref<TypeNameRule>) +
                    option(lit<"<"> +
                           some(append<&ast::StructMember::typeArgs>(
                                    ref<TypeArgRule>),
                                lit<",">) +
                           lit<">">)) +
             some(append<&ast::StructMember::declarators>(
                      ref<StructMemberDeclaratorRule>),
                  lit<",">) +
             option(assign<&ast::StructMember::io>(IoKw)) + lit<";">> {};

  struct StructMemberRule
      : rule<ref<StructMemberDeclRule> | (ref<PaddingRule> + lit<";">)> {};

  struct UnionMemberOrPtrRule
      : rule<
            (assign<&ast::UnionMember::type>(ref<TypeNameRule>) +
             assign<&ast::UnionMember::name>(ref<ID>) +
             option(lit<"["> +
                    (assign<&ast::UnionMember::size>(ref<Expression>) |
                     assign<&ast::UnionMember::sizeWhile>(ref<WhileSizeRule>)) +
                    lit<"]">) +
             option(assign<&ast::UnionMember::at>(ref<AtRule>)) +
             option(assign<&ast::UnionMember::attr>(ref<AttributeRule>))) //
            |
            (assign<&ast::UnionPtrMember::pointedType>(ref<TypeNameRule>) +
             lit<"*"> + assign<&ast::UnionPtrMember::name>(ref<ID>) + lit<":"> +
             assign<&ast::UnionPtrMember::sizeType>(ref<TypeNameRule>) +
             option(assign<&ast::UnionPtrMember::attr>(ref<AttributeRule>)))> {
  };

  struct BitfieldMemberRule
      : rule<(option(assign<&ast::BitfieldMember::type>(ref<TypeNameRule>)) +
              assign<&ast::BitfieldMember::name>(ref<ID>) +
              option(lit<"="> +
                     assign<&ast::BitfieldMember::init>(ref<Expression>)) +
              option(lit<":"> +
                     assign<&ast::BitfieldMember::width>(ref<Expression>)) +
              option(assign<&ast::BitfieldMember::attr>(ref<AttributeRule>))) //
             |
             (flag<&ast::BitfieldMember::isPadding>(kw<"padding">) +
              lit<":"> + assign<&ast::BitfieldMember::width>(ref<Expression>) +
              option(assign<&ast::BitfieldMember::attr>(ref<AttributeRule>)))> {
  };

  // The control syntax is common to all member contexts; only its recursive
  // statement rule differs.
  template <class S>
  static constexpr auto ContextBlock =
      lit<"{"> + many(append<&ast::Block::items>(ref<S>)) + lit<"}">;
  template <class S>
  static constexpr auto ContextIf =
      kw<"if"> + assign<&ast::IfStmt::cond>(ref<ParExpr>) +
      assign<&ast::IfStmt::thenBranch>(ref<S>) +
      option(commit(kw<"else">, assign<&ast::IfStmt::elseBranch>(ref<S>)));
  template <class S>
  static constexpr auto ContextCase =
      lit<"("> +
      some(append<&ast::MatchCase::patterns>(ref<PatternElemRule>), lit<",">) +
      lit<")"> + lit<":"> + assign<&ast::MatchCase::body>(ref<S>);
  template <class C>
  static constexpr auto ContextMatch =
      kw<"match"> + lit<"("> +
      some(append<&ast::MatchStmt::subjects>(ref<Expression>), lit<",">) +
      lit<")"> + lit<"{"> + some(append<&ast::MatchStmt::cases>(ref<C>)) +
      lit<"}">;

  // ---- struct-context statements ----

  struct StructBlockRule : rule<ContextBlock<StructStatementRule>> {};

  struct StructIfRule : rule<ContextIf<StructStatementRule>> {};

  struct StructMatchCaseRule : rule<ContextCase<StructStatementRule>> {};

  struct StructMatchRule : rule<ContextMatch<StructMatchCaseRule>> {};

  struct StructTryCatchRule
      : rule<kw<"try"> +
             assign<&ast::TryCatchStmt::tryBlock>(ref<StructBlockRule>) +
             kw<"catch"> +
             assign<&ast::TryCatchStmt::catchBlock>(ref<StructBlockRule>)> {};

  struct StructStatementRule
      : rule<ref<StructBlockRule>                                   //
             | ref<StructMemberRule>                                //
             | ref<StructIfRule>                                    //
             | ref<StructMatchRule>                                 //
             | create<ast::BreakStmt>(kw<"break"> + lit<";">)       //
             | create<ast::ContinueStmt>(kw<"continue"> + lit<";">) //
             | create<ast::ReturnStmt>(kw<"return"> + lit<";">)     //
             | create<ast::ExprStmt>(
                   assign<&ast::ExprStmt::value>(ref<Expression>) + lit<";">) //
             | ref<StructTryCatchRule>> {};

  // ---- union-context statements ----

  struct UnionBlockRule : rule<ContextBlock<UnionStatementRule>> {};

  struct UnionIfRule : rule<ContextIf<UnionStatementRule>> {};

  struct UnionMatchCaseRule : rule<ContextCase<UnionStatementRule>> {};

  struct UnionMatchRule : rule<ContextMatch<UnionMatchCaseRule>> {};

  struct UnionStatementRule
      : rule<ref<UnionBlockRule>                                    //
             | (ref<UnionMemberOrPtrRule> + lit<";">)               //
             | ref<UnionIfRule>                                     //
             | ref<UnionMatchRule>                                  //
             | create<ast::BreakStmt>(kw<"break"> + lit<";">)       //
             | create<ast::ContinueStmt>(kw<"continue"> + lit<";">) //
             | create<ast::ReturnStmt>(kw<"return"> + lit<";">)     //
             | create<ast::ExprStmt>(
                   assign<&ast::ExprStmt::value>(ref<Expression>) + lit<";">)> {
  };

  // ---- bitfield-context statements ----

  struct BitfieldBlockRule : rule<ContextBlock<BitfieldStatementRule>> {};

  struct BitfieldIfRule : rule<ContextIf<BitfieldStatementRule>> {};

  struct BitfieldMatchCaseRule : rule<ContextCase<BitfieldStatementRule>> {};

  struct BitfieldMatchRule : rule<ContextMatch<BitfieldMatchCaseRule>> {};

  struct BitfieldStatementRule
      : rule<ref<BitfieldBlockRule>                                 //
             | (ref<BitfieldMemberRule> + lit<";">)                 //
             | ref<BitfieldIfRule>                                  //
             | ref<BitfieldMatchRule>                               //
             | create<ast::BreakStmt>(kw<"break"> + lit<";">)       //
             | create<ast::ContinueStmt>(kw<"continue"> + lit<";">) //
             | create<ast::ReturnStmt>(kw<"return"> + lit<";">)     //
             | create<ast::ExprStmt>(
                   assign<&ast::ExprStmt::value>(ref<Expression>) + lit<";">)> {
  };

  // ---- function/block-context statements ----

  struct VarDeclaratorRule
      : rule<assign<&ast::VarDeclarator::name>(ref<ID>) +
             option(
                 flag<&ast::VarDeclarator::isArray>(lit<"["> + lit<"]">)) +
             option(assign<&ast::VarDeclarator::at>(ref<AtRule>)) +
             option(lit<"="> +
                    assign<&ast::VarDeclarator::init>(ref<Expression>))> {};

  struct VarDeclCoreRule
      : rule<option(flag<&ast::VarDecl::isConst>(kw<"const">)) +
             assign<&ast::VarDecl::type>(ref<TypeNameRule>) +
             some(append<&ast::VarDecl::declarators>(ref<VarDeclaratorRule>),
                  lit<",">)> {};

  struct VarDeclStmtRule : rule<ref<VarDeclCoreRule> + lit<";">> {};

  struct BlockRule : rule<lit<"{"> +
                          many(append<&ast::Block::items>(ref<VarDeclStmtRule> |
                                                          ref<StmtRule>)) +
                          lit<"}">> {};

  struct IfRule : rule<kw<"if"> + assign<&ast::IfStmt::cond>(ref<ParExpr>) +
                       assign<&ast::IfStmt::thenBranch>(ref<StmtRule>) +
                       option(commit(kw<"else">, assign<&ast::IfStmt::elseBranch>(
                                                 ref<StmtRule>)))> {};

  struct WhileRule
      : rule<kw<"while"> + assign<&ast::WhileStmt::cond>(ref<ParExpr>) +
             assign<&ast::WhileStmt::body>(ref<StmtRule>)> {};

  struct ForRule
      : rule<kw<"for"> + lit<"("> +
             option(assign<&ast::ForStmt::initDecl>(ref<VarDeclCoreRule>) |
                    some(append<&ast::ForStmt::initExprs>(ref<Expression>),
                         lit<",">)) +
             lit<","> + option(assign<&ast::ForStmt::cond>(ref<Expression>)) +
             lit<","> +
             option(some(append<&ast::ForStmt::update>(ref<Expression>),
                         lit<",">)) +
             lit<")"> + assign<&ast::ForStmt::body>(ref<StmtRule>)> {};

  struct MatchCaseRule
      : rule<lit<"("> +
             some(append<&ast::MatchCase::patterns>(ref<PatternElemRule>),
                  lit<",">) +
             lit<")"> + lit<":"> +
             assign<&ast::MatchCase::body>(ref<StmtRule>)> {};

  struct MatchStatementRule
      : rule<kw<"match"> + lit<"("> +
             some(append<&ast::MatchStmt::subjects>(ref<Expression>),
                  lit<",">) +
             lit<")"> + lit<"{"> +
             some(append<&ast::MatchStmt::cases>(ref<MatchCaseRule>)) +
             lit<"}">> {};

  struct TryCatchRule
      : rule<kw<"try"> + assign<&ast::TryCatchStmt::tryBlock>(ref<BlockRule>) +
             kw<"catch"> +
             assign<&ast::TryCatchStmt::catchBlock>(ref<BlockRule>)> {};

  struct StmtRule
      : rule<ref<BlockRule>            //
             | ref<IfRule>             //
             | ref<MatchStatementRule> //
             | ref<ForRule>            //
             | ref<WhileRule>          //
             | create<ast::ReturnStmt>(
                   kw<"return"> +
                   option(assign<&ast::ReturnStmt::value>(ref<Expression>)) +
                   lit<";">)                                        //
             | create<ast::BreakStmt>(kw<"break"> + lit<";">)       //
             | create<ast::ContinueStmt>(kw<"continue"> + lit<";">) //
             | create<ast::ExprStmt>(
                   assign<&ast::ExprStmt::value>(ref<Expression>) + lit<";">) //
             | (ref<PaddingRule> + lit<";">)                                  //
             | ref<TryCatchRule>> {};

  // ---- top-level declarations ----

  struct ParamRule : rule<option(flag<&ast::Param::isRef>(kw<"ref">)) +
                          assign<&ast::Param::type>(ref<TypeNameRule>) +
                          assign<&ast::Param::name>(ref<ID>)> {};

  struct FnRule
      : rule<kw<"fn"> + assign<&ast::Fn::name>(ref<ID>) + lit<"("> +
             option(some(append<&ast::Fn::params>(ref<ParamRule>), lit<",">)) +
             lit<")"> + assign<&ast::Fn::body>(ref<BlockRule>) + lit<";">> {};

  struct StructRule
      : rule<kw<"struct"> + assign<&ast::Struct::name>(ref<ID>) +
             option(lit<"<"> +
                    some(append<&ast::Struct::typeParams>(ref<TypeParamRule>),
                         lit<",">) +
                    lit<">">) +
             option(lit<":"> + assign<&ast::Struct::base>(ref<TypeNameRule>)) +
             assign<&ast::Struct::body>(ref<StructBlockRule>) +
             option(assign<&ast::Struct::attr>(ref<AttributeRule>)) +
             lit<";">> {};

  struct UnionRule
      : rule<kw<"union"> + assign<&ast::Union::name>(ref<ID>) +
             option(lit<"<"> +
                    some(append<&ast::Union::typeParams>(ref<TypeParamRule>),
                         lit<",">) +
                    lit<">">) +
             assign<&ast::Union::body>(ref<UnionBlockRule>) +
             option(assign<&ast::Union::attr>(ref<AttributeRule>)) + lit<";">> {
  };

  struct BitfieldRule
      : rule<kw<"bitfield"> + assign<&ast::Bitfield::name>(ref<ID>) +
             option(lit<"<"> +
                    some(append<&ast::Bitfield::typeParams>(ref<TypeParamRule>),
                         lit<",">) +
                    lit<">">) +
             assign<&ast::Bitfield::body>(ref<BitfieldBlockRule>) +
             option(assign<&ast::Bitfield::attr>(ref<AttributeRule>)) +
             lit<";">> {};

  struct EnumEntryRule : rule<assign<&ast::EnumEntry::name>(ref<ID>) +
                              option(lit<"="> + assign<&ast::EnumEntry::value>(
                                                    ref<PatternItemRule>))> {};

  struct EnumRule
      : rule<kw<"enum"> + option(assign<&ast::Enum::name>(ref<ID>)) + lit<":"> +
             assign<&ast::Enum::type>(ref<TypeNameRule>) + lit<"{"> +
             many(append<&ast::Enum::entries>(ref<EnumEntryRule>) + lit<",">) +
             option(append<&ast::Enum::entries>(ref<EnumEntryRule>)) +
             lit<"}"> + option(assign<&ast::Enum::attr>(ref<AttributeRule>)) +
             lit<";">> {};

  struct ImportRule
      : rule<kw<"import"> +
             some(append<&ast::Import::parts>(ref<ID>), lit<".">) + lit<";">> {
  };

  struct UsingAliasRule
      : rule<assign<&ast::UsingAlias::name>(ref<ID>) +
             option(
                 lit<"="> + assign<&ast::UsingAlias::type>(ref<TypeNameRule>) +
                 option(
                     lit<"<"> +
                     some(append<&ast::UsingAlias::typeArgs>(ref<TypeArgRule>),
                          lit<",">) +
                     lit<">">)) +
             option(assign<&ast::UsingAlias::attr>(ref<AttributeRule>)) +
             lit<";">> {};

  struct UsingGenericRule
      : rule<assign<&ast::UsingGeneric::fromType>(ref<TypeNameRule>) +
             lit<"<"> +
             some(append<&ast::UsingGeneric::fromArgs>(ref<TypeArgRule>),
                  lit<",">) +
             lit<">"> + lit<"="> +
             assign<&ast::UsingGeneric::toType>(ref<TypeNameRule>) + lit<"<"> +
             some(append<&ast::UsingGeneric::toArgs>(ref<TypeArgRule>),
                  lit<",">) +
             lit<">">> {};

  struct UsingStatementRule
      : rule<kw<"using"> + (ref<UsingAliasRule> | ref<UsingGenericRule>)> {};

  struct NamespaceItemRule
      : rule<ref<StructRule> | ref<StructMemberRule> | ref<EnumRule> |
             ref<BitfieldRule> | ref<UnionRule> | ref<FnRule> |
             ref<UsingStatementRule> | ref<NamespaceRule>> {};

  struct NamespaceRule
      : rule<kw<"namespace"> + assign<&ast::Namespace::name>(ref<ID>) +
             lit<"{"> +
             many(append<&ast::Namespace::items>(ref<NamespaceItemRule>)) +
             lit<"}">> {};

  struct DocumentItemRule
      : rule<ref<ImportRule> | ref<UsingStatementRule> | ref<NamespaceRule> |
             ref<StructRule> | ref<EnumRule> | ref<BitfieldRule> |
             ref<UnionRule> | ref<FnRule> | ref<StructMemberRule> |
             ref<StmtRule>> {};

  struct DocumentRule
      : rule<many(append<&ast::Document::items>(ref<DocumentItemRule>))> {};

  using entry = DocumentRule;
  using ignore = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT, PREPROCESSOR>;
};

} // namespace hexpat
