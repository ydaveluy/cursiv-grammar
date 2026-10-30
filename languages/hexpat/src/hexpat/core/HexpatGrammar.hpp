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
      : terminal<kw<"u128"> | kw<"u96"> | kw<"u64"> | kw<"u48"> | kw<"u32"> | kw<"u24"> | kw<"u16"> | kw<"u8"> |
                 kw<"s128"> | kw<"s96"> | kw<"s64"> | kw<"s48"> | kw<"s32"> | kw<"s24"> | kw<"s16"> | kw<"s8"> |
                 kw<"double"> | kw<"float"> | kw<"char16"> | kw<"char"> | kw<"str"> |
                 kw<"bool"> | kw<"auto">> {};

  // Numeric spellings are retained in the AST, including native separators
  // and u/f/d suffixes; their values are not evaluated by this port.
  static constexpr auto HexDigits =
      cr<"0-9a-fA-F"> + many(option(lit<"'">) + cr<"0-9a-fA-F">);
  static constexpr auto OctDigits =
      cr<"0-7"> + many(option(lit<"'">) + cr<"0-7">);
  static constexpr auto BinDigits =
      cr<"01"> + many(option(lit<"'">) + cr<"01">);
  static constexpr auto DecimalDigits = d + many(option(lit<"'">) + d);
  struct INT_LIT : terminal<((lit<"0"> + cr<"xX"> + HexDigits) |
                            (lit<"0"> + cr<"oO"> + OctDigits) |
                            (lit<"0"> + cr<"bB"> + BinDigits) | DecimalDigits) +
                            option(cr<"uU">)> {};
  static constexpr auto Exponent = cr<"eE"> + option(cr<"+-">) + some(d);
  struct FLOAT_LIT
      : terminal<(some(d) + lit<".", !lit<".">> + many(d) +
                  option(Exponent) + option(cr<"fFdD">)) |
                 (some(d) + option(Exponent) + cr<"fFdD">)> {};

  static constexpr auto CharEscape =
      lit<"\\"> + (cr<R"(abfntr0"'\\)"> |
                   (lit<"x"> + repeat<2>(cr<"0-9a-fA-F">)) |
                   (lit<"u"> + repeat<4>(cr<"0-9a-fA-F">)) |
                   (lit<"U"> + repeat<8>(cr<"0-9a-fA-F">)));
  struct CHAR_LIT
      : terminal<lit<"'"> + (CharEscape | cr<R"(^'\\)">) + lit<"'">> {};
  struct STRING_LIT
      : terminal<lit<"\""> + many(CharEscape | cr<R"(^"\\\r\n\x00)">) +
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
  struct TypeName;
  struct TypeArg;
  struct WhileSize;
  struct At;
  struct Attribute;
  struct PatternItem;
  struct PatternElem;
  struct Padding;
  struct Block;
  struct StructBlock;
  struct UnionBlock;
  struct BitfieldBlock;
  struct StructStatement;
  struct UnionStatement;
  struct BitfieldStatement;
  struct Stmt;
  struct StructMember;
  struct Namespace;
  struct DocumentItem;

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
                   (assign<&ast::TypenameofExpr::operand>(ref<Expression>) |
                    assign<&ast::TypenameofExpr::primType>(ref<PRIMITIVE>)) +
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

  struct InfixExpr : infix<Unary, ast::BinaryExpr,
                           levels(left(OpMul),     //
                                  left(OpAdd),     //
                                  left(OpShift),   //
                                  left(OpRel),     //
                                  left(OpEq),      //
                                  left(OpXorXor),  //
                                  left(OpBAnd),    //
                                  left(OpBXor),    //
                                  left(OpBOr),     //
                                  left(OpLAnd),    //
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

  struct TypeName
      : rule<many(append<&ast::TypeRef::path>(ref<ID>) + lit<"::">) +
             assign<&ast::TypeRef::name>(ref<PRIMITIVE> | ref<ID>)> {};


  struct TypeArg
      : rule<(option(flag<&ast::TypeArg::isAuto>(kw<"auto">)) +
              assign<&ast::TypeArg::type>(ref<TypeName>) +
              option(lit<"<"> +
                     some(append<&ast::TypeArg::typeArgs>(ref<TypeArg>),
                          lit<",">) +
                     lit<">">)) //
             | assign<&ast::TypeArg::expr>(ref<Expression>)> {};

  struct TypeParam
      : rule<option(flag<&ast::TypeParam::isAuto>(kw<"auto">)) +
             assign<&ast::TypeParam::type>(ref<TypeName>)> {};

  struct AttrItem
      : rule<(assign<&ast::AttrItem::type>(ref<TypeName>) + lit<"("> +
              option(some(append<&ast::AttrItem::args>(ref<Expression>),
                          lit<",">)) +
              lit<")">) //
             | assign<&ast::AttrItem::name>(ref<ID>)> {};

  struct Attribute
      : rule<lit<"["> + lit<"["> +
             some(append<&ast::Attribute::items>(ref<AttrItem>), lit<",">) +
             lit<"]"> + lit<"]">> {};

  struct WhileSize
      : rule<kw<"while"> + lit<"("> + ref<Expression> + lit<")">> {};

  struct At
      : rule<lit<"@"> + assign<&ast::At::expr>(ref<Expression>) +
             option(kw<"in"> + assign<&ast::At::section>(ref<Expression>))> {};

  struct PatternItem
      : rule<create<ast::PatternItem>(
            assign<&ast::PatternItem::from>(ref<Expression>) +
            option(lit<"..."> +
                   assign<&ast::PatternItem::to>(ref<Expression>)))> {};

  struct PatternElem
      : rule<some(append<&ast::PatternElem::alts>(ref<PatternItem>),
                  lit<"|">)> {};

  // ---- declarators ----

  struct StructMemberDeclarator
      : rule<option(
                 flag<&ast::StructMemberDeclarator::isPointer>(lit<"*">)) +
             option(assign<&ast::StructMemberDeclarator::name>(ref<ID>)) +
             option(flag<&ast::StructMemberDeclarator::hasBrackets>(
                        lit<"[">) +
                    option(assign<&ast::StructMemberDeclarator::size>(
                               ref<Expression>) |
                           assign<&ast::StructMemberDeclarator::sizeWhile>(
                               ref<WhileSize>)) +
                    lit<"]">) +
             option(lit<":"> + assign<&ast::StructMemberDeclarator::colonType>(
                                   ref<TypeName>)) +
             option(lit<"="> + assign<&ast::StructMemberDeclarator::init>(
                                   ref<Expression>)) +
             option(assign<&ast::StructMemberDeclarator::at>(ref<At>)) +
             option(assign<&ast::StructMemberDeclarator::attr>(
                 ref<Attribute>))> {};

  struct Padding
      : rule<kw<"padding"> + lit<"["> +
             (assign<&ast::Padding::size>(ref<Expression>) |
              assign<&ast::Padding::whileCond>(ref<WhileSize>)) +
             lit<"]">> {};

  struct StructMemberDecl
      : rule<option(flag<&ast::StructMember::isConst>(kw<"const">)) +
             option(assign<&ast::StructMember::endian>(EndianKw)) +
             option(assign<&ast::StructMember::type>(ref<TypeName>) +
                    option(lit<"<"> +
                           some(append<&ast::StructMember::typeArgs>(
                                    ref<TypeArg>),
                                lit<",">) +
                           lit<">">)) +
             some(append<&ast::StructMember::declarators>(
                      ref<StructMemberDeclarator>),
                  lit<",">) +
             option(assign<&ast::StructMember::io>(IoKw)) + lit<";">> {};

  struct StructMember
      : rule<ref<StructMemberDecl> | (ref<Padding> + lit<";">)> {};

  struct UnionMemberOrPtr
      : rule<
            (assign<&ast::UnionMember::type>(ref<TypeName>) +
             assign<&ast::UnionMember::name>(ref<ID>) +
             option(lit<"["> +
                    (assign<&ast::UnionMember::size>(ref<Expression>) |
                     assign<&ast::UnionMember::sizeWhile>(ref<WhileSize>)) +
                    lit<"]">) +
             option(assign<&ast::UnionMember::at>(ref<At>)) +
             option(assign<&ast::UnionMember::attr>(ref<Attribute>))) //
            |
            (assign<&ast::UnionPtrMember::pointedType>(ref<TypeName>) +
             lit<"*"> + assign<&ast::UnionPtrMember::name>(ref<ID>) + lit<":"> +
             assign<&ast::UnionPtrMember::sizeType>(ref<TypeName>) +
             option(assign<&ast::UnionPtrMember::attr>(ref<Attribute>)))> {
  };

  struct BitfieldMember
      : rule<(option(assign<&ast::BitfieldMember::type>(ref<TypeName>)) +
              assign<&ast::BitfieldMember::name>(ref<ID>) +
              option(lit<"="> +
                     assign<&ast::BitfieldMember::init>(ref<Expression>)) +
              option(lit<":"> +
                     assign<&ast::BitfieldMember::width>(ref<Expression>)) +
              option(assign<&ast::BitfieldMember::attr>(ref<Attribute>))) //
             |
             (flag<&ast::BitfieldMember::isPadding>(kw<"padding">) +
              lit<":"> + assign<&ast::BitfieldMember::width>(ref<Expression>) +
              option(assign<&ast::BitfieldMember::attr>(ref<Attribute>)))> {
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
      some(append<&ast::MatchCase::patterns>(ref<PatternElem>), lit<",">) +
      lit<")"> + lit<":"> + assign<&ast::MatchCase::body>(ref<S>);
  template <class C>
  static constexpr auto ContextMatch =
      kw<"match"> + lit<"("> +
      some(append<&ast::MatchStmt::subjects>(ref<Expression>), lit<",">) +
      lit<")"> + lit<"{"> + some(append<&ast::MatchStmt::cases>(ref<C>)) +
      lit<"}">;

  // ---- struct-context statements ----

  struct StructBlock : rule<ContextBlock<StructStatement>> {};

  struct StructIf : rule<ContextIf<StructStatement>> {};

  struct StructMatchCase : rule<ContextCase<StructStatement>> {};

  struct StructMatch : rule<ContextMatch<StructMatchCase>> {};

  struct StructTryCatch
      : rule<kw<"try"> +
             assign<&ast::TryCatchStmt::tryBlock>(ref<StructBlock>) +
             kw<"catch"> +
             assign<&ast::TryCatchStmt::catchBlock>(ref<StructBlock>)> {};

  struct StructStatement
      : rule<ref<StructBlock>                                   //
             | ref<StructMember>                                //
             | ref<StructIf>                                    //
             | ref<StructMatch>                                 //
             | create<ast::BreakStmt>(kw<"break"> + lit<";">)       //
             | create<ast::ContinueStmt>(kw<"continue"> + lit<";">) //
             | create<ast::ReturnStmt>(kw<"return"> + lit<";">)     //
             | create<ast::ExprStmt>(
                   assign<&ast::ExprStmt::value>(ref<Expression>) + lit<";">) //
             | ref<StructTryCatch>> {};

  // ---- union-context statements ----

  struct UnionBlock : rule<ContextBlock<UnionStatement>> {};

  struct UnionIf : rule<ContextIf<UnionStatement>> {};

  struct UnionMatchCase : rule<ContextCase<UnionStatement>> {};

  struct UnionMatch : rule<ContextMatch<UnionMatchCase>> {};

  struct UnionStatement
      : rule<ref<UnionBlock>                                    //
             | (ref<UnionMemberOrPtr> + lit<";">)               //
             | ref<UnionIf>                                     //
             | ref<UnionMatch>                                  //
             | create<ast::BreakStmt>(kw<"break"> + lit<";">)       //
             | create<ast::ContinueStmt>(kw<"continue"> + lit<";">) //
             | create<ast::ReturnStmt>(kw<"return"> + lit<";">)     //
             | create<ast::ExprStmt>(
                   assign<&ast::ExprStmt::value>(ref<Expression>) + lit<";">)> {
  };

  // ---- bitfield-context statements ----

  struct BitfieldBlock : rule<ContextBlock<BitfieldStatement>> {};

  struct BitfieldIf : rule<ContextIf<BitfieldStatement>> {};

  struct BitfieldMatchCase : rule<ContextCase<BitfieldStatement>> {};

  struct BitfieldMatch : rule<ContextMatch<BitfieldMatchCase>> {};

  struct BitfieldStatement
      : rule<ref<BitfieldBlock>                                 //
             | (ref<BitfieldMember> + lit<";">)                 //
             | ref<BitfieldIf>                                  //
             | ref<BitfieldMatch>                               //
             | create<ast::BreakStmt>(kw<"break"> + lit<";">)       //
             | create<ast::ContinueStmt>(kw<"continue"> + lit<";">) //
             | create<ast::ReturnStmt>(kw<"return"> + lit<";">)     //
             | create<ast::ExprStmt>(
                   assign<&ast::ExprStmt::value>(ref<Expression>) + lit<";">)> {
  };

  // ---- function/block-context statements ----

  struct VarDeclarator
      : rule<assign<&ast::VarDeclarator::name>(ref<ID>) +
             option(
                 flag<&ast::VarDeclarator::isArray>(lit<"[">) +
                 option(assign<&ast::VarDeclarator::size>(ref<Expression>) |
                        assign<&ast::VarDeclarator::sizeWhile>(ref<WhileSize>)) + lit<"]">) +
             option(assign<&ast::VarDeclarator::at>(ref<At>)) +
             option(lit<"="> +
                    assign<&ast::VarDeclarator::init>(ref<Expression>))> {};

  struct VarDeclCore
      : rule<option(flag<&ast::VarDecl::isConst>(kw<"const">)) +
             assign<&ast::VarDecl::type>(ref<TypeName>) +
             some(append<&ast::VarDecl::declarators>(ref<VarDeclarator>),
                  lit<",">)> {};

  struct VarDeclStmt : rule<ref<VarDeclCore> + lit<";">> {};

  struct BlockRule : rule<lit<"{"> +
                          many(append<&ast::Block::items>(ref<VarDeclStmt> |
                                                          ref<Stmt>)) +
                          lit<"}">> {};

  struct If : rule<kw<"if"> + assign<&ast::IfStmt::cond>(ref<ParExpr>) +
                       assign<&ast::IfStmt::thenBranch>(ref<Stmt>) +
                       option(commit(kw<"else">, assign<&ast::IfStmt::elseBranch>(
                                                 ref<Stmt>)))> {};

  struct While
      : rule<kw<"while"> + assign<&ast::WhileStmt::cond>(ref<ParExpr>) +
             assign<&ast::WhileStmt::body>(ref<Stmt>)> {};

  struct For
      : rule<kw<"for"> + lit<"("> +
             option(assign<&ast::ForStmt::initDecl>(ref<VarDeclCore>) |
                    some(append<&ast::ForStmt::initExprs>(ref<Expression>),
                         lit<",">)) +
             lit<","> + option(assign<&ast::ForStmt::cond>(ref<Expression>)) +
             lit<","> +
             option(some(append<&ast::ForStmt::update>(ref<Expression>),
                         lit<",">)) +
             lit<")"> + assign<&ast::ForStmt::body>(ref<Stmt>)> {};

  struct MatchCase
      : rule<lit<"("> +
             some(append<&ast::MatchCase::patterns>(ref<PatternElem>),
                  lit<",">) +
             lit<")"> + lit<":"> +
             assign<&ast::MatchCase::body>(ref<Stmt>)> {};

  struct MatchStatement
      : rule<kw<"match"> + lit<"("> +
             some(append<&ast::MatchStmt::subjects>(ref<Expression>),
                  lit<",">) +
             lit<")"> + lit<"{"> +
             some(append<&ast::MatchStmt::cases>(ref<MatchCase>)) +
             lit<"}">> {};

  struct TryCatch
      : rule<kw<"try"> + assign<&ast::TryCatchStmt::tryBlock>(ref<BlockRule>) +
             kw<"catch"> +
             assign<&ast::TryCatchStmt::catchBlock>(ref<BlockRule>)> {};

  struct Stmt
      : rule<ref<BlockRule>            //
             | ref<If>             //
             | ref<MatchStatement> //
             | ref<For>            //
             | ref<While>          //
             | create<ast::ReturnStmt>(
                   kw<"return"> +
                   option(assign<&ast::ReturnStmt::value>(ref<Expression>)) +
                   lit<";">)                                        //
             | create<ast::BreakStmt>(kw<"break"> + lit<";">)       //
             | create<ast::ContinueStmt>(kw<"continue"> + lit<";">) //
             | create<ast::ExprStmt>(
                   assign<&ast::ExprStmt::value>(ref<Expression>) + lit<";">) //
             | (ref<Padding> + lit<";">)                                  //
             | ref<TryCatch>> {};

  // ---- top-level declarations ----

  struct Param : rule<option(flag<&ast::Param::isRef>(kw<"ref">)) +
                          option(assign<&ast::Param::endian>(EndianKw)) +
                          assign<&ast::Param::type>(ref<TypeName>) +
                          option(lit<"<"> + some(append<&ast::Param::typeArgs>(ref<TypeArg>), lit<",">) + lit<">">) +
                          ((flag<&ast::Param::isPack>(lit<"...">) +
                            assign<&ast::Param::name>(ref<ID>)) |
                           (option(assign<&ast::Param::name>(ref<ID>)) +
                            option(lit<"="> + assign<&ast::Param::defaultValue>(ref<Expression>))))> {};

  struct Fn
      : rule<kw<"fn"> + assign<&ast::Fn::name>(ref<ID>) + lit<"("> +
             option(some(append<&ast::Fn::params>(ref<Param>), lit<",">)) +
             lit<")"> + assign<&ast::Fn::body>(ref<BlockRule>) + lit<";">> {};

  struct Struct
      : rule<kw<"struct"> + assign<&ast::Struct::name>(ref<ID>) +
             option(lit<"<"> +
                    some(append<&ast::Struct::typeParams>(ref<TypeParam>),
                         lit<",">) +
                    lit<">">) +
             option(lit<":"> + assign<&ast::Struct::base>(ref<TypeName>)) +
             assign<&ast::Struct::body>(ref<StructBlock>) +
             option(assign<&ast::Struct::attr>(ref<Attribute>)) +
             lit<";">> {};

  struct Union
      : rule<kw<"union"> + assign<&ast::Union::name>(ref<ID>) +
             option(lit<"<"> +
                    some(append<&ast::Union::typeParams>(ref<TypeParam>),
                         lit<",">) +
                    lit<">">) +
             assign<&ast::Union::body>(ref<UnionBlock>) +
             option(assign<&ast::Union::attr>(ref<Attribute>)) + lit<";">> {
  };

  struct Bitfield
      : rule<kw<"bitfield"> + assign<&ast::Bitfield::name>(ref<ID>) +
             option(lit<"<"> +
                    some(append<&ast::Bitfield::typeParams>(ref<TypeParam>),
                         lit<",">) +
                    lit<">">) +
             assign<&ast::Bitfield::body>(ref<BitfieldBlock>) +
             option(assign<&ast::Bitfield::attr>(ref<Attribute>)) +
             lit<";">> {};

  struct EnumEntry : rule<assign<&ast::EnumEntry::name>(ref<ID>) +
                              option(lit<"="> + assign<&ast::EnumEntry::value>(
                                                    ref<PatternItem>))> {};

  struct Enum
      : rule<kw<"enum"> + option(assign<&ast::Enum::name>(ref<ID>)) + lit<":"> +
             assign<&ast::Enum::type>(ref<TypeName>) + lit<"{"> +
             many(append<&ast::Enum::entries>(ref<EnumEntry>) + lit<",">) +
             option(append<&ast::Enum::entries>(ref<EnumEntry>)) +
             lit<"}"> + option(assign<&ast::Enum::attr>(ref<Attribute>)) +
             lit<";">> {};

  struct Import
      : rule<kw<"import"> +
             some(append<&ast::Import::parts>(ref<ID>), lit<".">) + lit<";">> {
  };

  struct UsingAlias
      : rule<kw<"using"> + assign<&ast::UsingAlias::name>(ref<ID>) +
             option(lit<"<"> + some(append<&ast::UsingAlias::typeParams>(ref<TypeParam>), lit<",">) + lit<">">) +
             option(lit<"="> + option(flag<&ast::UsingAlias::isRef>(kw<"ref">)) +
                    option(assign<&ast::UsingAlias::endian>(EndianKw)) +
                    assign<&ast::UsingAlias::type>(ref<TypeName>) +
                    option(lit<"<"> + some(append<&ast::UsingAlias::typeArgs>(ref<TypeArg>), lit<",">) + lit<">">)) +
             option(assign<&ast::UsingAlias::attr>(ref<Attribute>)) + lit<";">> {};

  struct NamespaceName : rule<some(ref<ID>, lit<"::">)> {};
  struct Namespace
      : rule<kw<"namespace"> + assign<&ast::Namespace::name>(ref<NamespaceName>) +
             lit<"{"> + many(append<&ast::Namespace::items>(ref<DocumentItem>)) + lit<"}">> {};

  struct DocumentItem
      : rule<ref<Import> | ref<UsingAlias> | ref<Namespace> |
             ref<Struct> | ref<Enum> | ref<Bitfield> |
             ref<Union> | ref<Fn> | ref<StructMember> |
             ref<Stmt>> {};

  struct Document
      : rule<many(append<&ast::Document::items>(ref<DocumentItem>))> {};

  using entry = Document;
  using ignored = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT, PREPROCESSOR>;
};

} // namespace hexpat
