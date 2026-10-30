// The alef language: its grammar.
#pragma once

#include "alef/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace alef {

struct Alef : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  struct ID : terminal<cr<"a-zA-Z_"> + many(w)> {};

  static constexpr auto Escape = lit<"\\"> + any;
  struct STRING_CONST
      : terminal<lit<"\""> + many(Escape | cr<R"(^"\\\r\n)">) + lit<"\"">> {};
  struct CONSTANT
      : terminal<lit<"'"> + many(Escape | cr<R"(^'\\\r\n)">) + lit<"'">> {};
  static constexpr auto Exponent =
      cr<"eE"> + option(cr<"+-">) + some(d);
  struct ARITHMETIC_CONST
      : terminal<(lit<"0"> + cr<"xX"> + some(cr<"0-9a-fA-F">)) |
                 (some(d) + option(lit<"."> + many(d)) + option(Exponent)) |
                 (lit<"."> + some(d) + option(Exponent))> {};

  static constexpr auto SClassKw =
      kw<"extern"> | kw<"intern"> | kw<"private">;

  struct Pointer;
  struct ArraySuffix;
  struct Declarator;
  struct Xtname;
  struct TypeCast;
  struct TName;
  struct NamedType;
  struct Arg;
  struct ArgListItem;
  struct ArgList;
  struct AutoDecl;
  struct VarInit;
  struct ZElist;
  struct InitItem;
  struct Statement;
  struct Block;
  struct CaseClause;
  struct CaseBody;
  struct TypeCaseClause;
  struct TypeCaseBody;
  struct Expr;
  struct CastExpr;
  struct MonExpr;
  struct Term;
  struct Primary;
  struct Decl;

  struct Pointer : rule<create<ast::Pointer>(lit<"*">)> {};

  struct ArraySuffix
      : rule<create<ast::ArraySuffix>(
            lit<"["> + option(assign<&ast::ArraySuffix::size>(ref<Expr>)) +
            lit<"]">)> {};

  // The two plain forms share `many(pointers)` (0 stars = the bare form);
  // the two parenthesised forms are a function-pointer declarator.
  struct Declarator
      : rule<create<ast::Declarator>(
                 many(append<&ast::Declarator::pointers>(ref<Pointer>)) +
                 assign<&ast::Declarator::name>(ref<ID>) +
                 many(append<&ast::Declarator::dims>(ref<ArraySuffix>)))  //
             | create<ast::Declarator>(
                   many(append<&ast::Declarator::pointers>(ref<Pointer>)) +
                   flag<&ast::Declarator::isFuncPtr>(lit<"(">) +
                   some(append<&ast::Declarator::innerPointers>(ref<Pointer>)) +
                   assign<&ast::Declarator::name>(ref<ID>) +
                   many(append<&ast::Declarator::dims>(ref<ArraySuffix>)) +
                   lit<")"> + lit<"("> +
                   option(assign<&ast::Declarator::funcPtrParams>(
                       ref<ArgList>)) +
                   lit<")">)> {};

  // `lint`/`ulint` join their five siblings here -- lexed, but otherwise
  // unreachable from any parser rule.
  struct Xtname
      : rule<create<ast::PrimType>(assign<&ast::PrimType::keyword>(
                 kw<"int"> | kw<"uint"> | kw<"sint"> | kw<"usint"> |
                 kw<"byte"> | kw<"float"> | kw<"void"> | kw<"lint"> |
                 kw<"ulint">))  //
             | create<ast::ArrayOfType>(
                   assign<&ast::ArrayOfType::name>(ref<ID>) + lit<"["> +
                   some(append<&ast::ArrayOfType::args>(ref<TypeCast>),
                        lit<",">) +
                   lit<"]">)  //
             | create<ast::ChanType>(
                   kw<"chan"> + lit<"("> +
                   some(append<&ast::ChanType::args>(ref<TypeCast>),
                        lit<",">) +
                   lit<")"> +
                   option(lit<"["> +
                          assign<&ast::ChanType::bufSize>(ref<Expr>) +
                          lit<"]">))  //
             | create<ast::NamedType>(
                   assign<&ast::NamedType::name>(ref<ID>))> {};

  struct NamedType
      : rule<create<ast::NamedType>(assign<&ast::NamedType::name>(ref<ID>))> {
  };

  struct TypeCast
      : rule<create<ast::TypeCast>(
                 assign<&ast::TypeCast::type>(ref<Xtname>) +
                 many(append<&ast::TypeCast::pointers>(ref<Pointer>)))  //
             | create<ast::TypeCast>(
                   assign<&ast::TypeCast::type>(ref<Xtname>) +
                   flag<&ast::TypeCast::isFuncType>(lit<"(">) +
                   some(append<&ast::TypeCast::innerPointers>(ref<Pointer>)) +
                   lit<")"> + lit<"("> +
                   option(assign<&ast::TypeCast::params>(ref<ArgList>)) +
                   lit<")">)  //
             | create<ast::TypeCast>(
                   kw<"tuple"> +
                   assign<&ast::TypeCast::tuple>(ref<TName>))> {};

  struct TName
      : rule<create<ast::TName>(
                 option(assign<&ast::TName::sclass>(SClassKw)) +
                 assign<&ast::TName::type>(ref<Xtname>))  //
             | create<ast::TName>(
                   option(assign<&ast::TName::sclass>(SClassKw)) +
                   flag<&ast::TName::tupleKeyword>(kw<"tuple">) +
                   lit<"("> +
                   some(append<&ast::TName::tupleTypes>(ref<TypeCast>),
                        lit<",">) +
                   lit<")">)  //
             | create<ast::TName>(
                   option(assign<&ast::TName::sclass>(SClassKw)) + lit<"("> +
                   some(append<&ast::TName::tupleTypes>(ref<TypeCast>),
                        lit<",">) +
                   lit<")">)> {};

  struct ArgList
      : rule<create<ast::ArgList>(
            many(append<&ast::ArgList::params>(ref<ArgListItem>) +
                 lit<",">) +
            append<&ast::ArgList::params>(ref<Arg>))> {};

  struct ArgListItem
      : rule<ref<Arg>  //
             | create<ast::Arg>(
                   flag<&ast::Arg::isRestStar>(lit<"*">) +
                   assign<&ast::Arg::type>(ref<Xtname>))  //
             | create<ast::Arg>(
                   flag<&ast::Arg::isRestDot>(lit<".">) +
                   assign<&ast::Arg::type>(ref<Xtname>))> {};

  struct Arg
      : rule<create<ast::Arg>(
                 assign<&ast::Arg::type>(ref<Xtname>) +
                 many(append<&ast::Arg::pointers>(ref<Pointer>)) +
                 many(append<&ast::Arg::dims>(ref<ArraySuffix>)))  //
             | create<ast::Arg>(
                   assign<&ast::Arg::type>(ref<Xtname>) +
                   many(append<&ast::Arg::pointers>(ref<Pointer>)) +
                   flag<&ast::Arg::isFuncPtr>(lit<"(">) +
                   some(append<&ast::Arg::innerPointers>(ref<Pointer>)) +
                   lit<")"> + lit<"("> +
                   option(assign<&ast::Arg::funcPtrParams>(ref<ArgList>)) +
                   lit<")">)  //
             | create<ast::Arg>(kw<"tuple"> +
                                 assign<&ast::Arg::tupleType>(ref<TName>))  //
             | create<ast::Arg>(
                   kw<"tuple"> + assign<&ast::Arg::tupleType>(ref<TName>) +
                   flag<&ast::Arg::isFuncPtr>(lit<"(">) +
                   some(append<&ast::Arg::innerPointers>(ref<Pointer>)) +
                   lit<")"> + lit<"("> +
                   option(assign<&ast::Arg::funcPtrParams>(ref<ArgList>)) +
                   lit<")">)  //
             | create<ast::Arg>(
                   kw<"tuple"> + assign<&ast::Arg::tupleType>(ref<TName>) +
                   assign<&ast::Arg::declarator>(ref<Declarator>))  //
             | create<ast::Arg>(assign<&ast::Arg::type>(ref<Xtname>) +
                                 assign<&ast::Arg::declarator>(ref<Declarator>))  //
             | create<ast::Arg>(
                   flag<&ast::Arg::isVariadic>(lit<"...">))> {};

  struct VarInit
      : rule<assign<&ast::VarInit::declarator>(ref<Declarator>) +
             option(lit<"="> +
                    assign<&ast::VarInit::init>(ref<ZElist>))> {};

  // Empty initializer positions are real items. Each repeated tail also
  // consumes a comma, so even an empty item's position is retained.
  struct ZElist
      : rule<create<ast::InitList>(append<&ast::InitList::items>(ref<InitItem>) +
            many(lit<","> + append<&ast::InitList::items>(ref<InitItem>)))> {
  };

  struct InitItem
      : rule<create<ast::InitItem>(
                 lit<"["> + assign<&ast::InitItem::index>(ref<Expr>) +
                 lit<"]"> + lit<"{"> +
                 assign<&ast::InitItem::braced>(ref<ZElist>) +
                 lit<"}">)  //
             | create<ast::InitItem>(
                   lit<"["> + assign<&ast::InitItem::index>(ref<Expr>) +
                   lit<"]"> +
                   assign<&ast::InitItem::indexedValue>(ref<Expr>))  //
             | create<ast::InitItem>(
                   lit<"."> + assign<&ast::InitItem::field>(ref<ID>) +
                   assign<&ast::InitItem::fieldValue>(ref<Expr>))  //
             | create<ast::InitItem>(
                   lit<"{"> + assign<&ast::InitItem::braced>(ref<ZElist>) +
                   lit<"}">)  //
             | create<ast::InitItem>(
                   assign<&ast::InitItem::value>(ref<Expr>)) |
             create<ast::InitItem>> {};

  struct AutoDecl
      : rule<create<ast::AutoDecl>(
                 assign<&ast::AutoDecl::type>(ref<Xtname>) +
                 many(append<&ast::AutoDecl::vars>(ref<VarInit>),
                      lit<",">) +
                 lit<";">)  //
             | create<ast::AutoDecl>(
                   kw<"tuple"> +
                   assign<&ast::AutoDecl::tupleType>(ref<TName>) +
                   many(append<&ast::AutoDecl::vars>(ref<VarInit>),
                        lit<",">) +
                   lit<";">)> {};

  // -- expressions --

  struct Tcomp;
  struct BracedTuple
      : rule<create<ast::TupleExpr>(
            flag<&ast::TupleExpr::braced>(lit<"{">) +
            option(some(append<&ast::TupleExpr::items>(ref<Tcomp>),
                        lit<",">)) +
            lit<"}">)> {};
  struct Tcomp : rule<ref<Expr> | ref<BracedTuple>> {};

  struct TupleParen
      : rule<create<ast::TupleExpr>(
            lit<"("> +
            some(append<&ast::TupleExpr::items>(ref<Tcomp>), lit<",">) +
            lit<")">)> {};

  struct SizeofTypeExpr
      : rule<create<ast::SizeofTypeExpr>(
            kw<"sizeof"> + lit<"("> +
            assign<&ast::SizeofTypeExpr::type>(ref<TypeCast>) +
            lit<")">)> {};

  struct QualifiedRef
      : rule<create<ast::QualifiedRef>(
            lit<"."> + assign<&ast::QualifiedRef::scope>(ref<ID>) + lit<"."> +
            assign<&ast::QualifiedRef::field>(ref<ID>))> {};

  struct EllipsisExpr : rule<create<ast::EllipsisExpr>(lit<"...">)> {};
  struct NilExpr : rule<create<ast::NilExpr>(kw<"nil">)> {};
  struct Ident : rule<assign<&ast::Ident::name>(ref<ID>)> {};
  struct NumberLit
      : rule<assign<&ast::NumberLit::text>(ref<ARITHMETIC_CONST>)> {};
  struct CharLit : rule<assign<&ast::CharLit::text>(ref<CONSTANT>)> {};
  struct StringLit
      : rule<assign<&ast::StringLit::text>(ref<STRING_CONST>)> {};
  struct DollarString
      : rule<create<ast::StringLit>(
            flag<&ast::StringLit::dollar>(lit<"$">) + gap::none +
            assign<&ast::StringLit::text>(ref<STRING_CONST>))> {};

  struct Primary
      : rule<ref<TupleParen>       //
             | ref<SizeofTypeExpr>  //
             | ref<QualifiedRef>    //
             | ref<EllipsisExpr>    //
             | ref<NilExpr>         //
             | ref<NumberLit>       //
             | ref<CharLit>         //
             | ref<DollarString>    //
             | ref<StringLit>       //
             | ref<Ident>> {};

  struct Term
      : rule<ref<Primary> +
             many(nest<ast::CallExpr, &ast::CallExpr::callee>(
                      lit<"("> +
                      option(some(append<&ast::CallExpr::args>(ref<Expr>),
                                  lit<",">)) +
                      lit<")">)  //
                  | nest<ast::IndexExpr, &ast::IndexExpr::array>(
                        lit<"["> +
                        assign<&ast::IndexExpr::index>(ref<Expr>) +
                        lit<"]">)  //
                  | nest<ast::MemberExpr, &ast::MemberExpr::object>(
                        flag<&ast::MemberExpr::arrow>(lit<"->">) +
                        assign<&ast::MemberExpr::field>(ref<ID>))  //
                  | nest<ast::MemberExpr, &ast::MemberExpr::object>(
                        lit<"."> +
                        assign<&ast::MemberExpr::field>(ref<ID>))  //
                  | nest<ast::PostfixExpr, &ast::PostfixExpr::operand>(
                        assign<&ast::PostfixExpr::op>(
                            lit<"--"> | lit<"++"> | lit<"?">)))> {};

  struct MonExpr
      : rule<ref<Term>  //
             | create<ast::PrefixExpr>(
                   assign<&ast::PrefixExpr::op>(lit<"*"> | lit<"&"> |
                                                 lit<"+"> | lit<"-">) +
                   assign<&ast::PrefixExpr::operand>(ref<CastExpr>))  //
             | create<ast::PrefixExpr>(
                   assign<&ast::PrefixExpr::op>(lit<"--"> | lit<"++">) +
                   assign<&ast::PrefixExpr::operand>(ref<CastExpr>))  //
             | create<ast::PrefixExpr>(
                   assign<&ast::PrefixExpr::op>(kw<"zerox">) +
                   assign<&ast::PrefixExpr::operand>(ref<CastExpr>))  //
             | create<ast::PrefixExpr>(
                   assign<&ast::PrefixExpr::op>(lit<"!"> | lit<"~">) +
                   assign<&ast::PrefixExpr::operand>(ref<CastExpr>))  //
             | create<ast::SizeofExpr>(
                   kw<"sizeof"> +
                   assign<&ast::SizeofExpr::operand>(ref<MonExpr>))  //
             | create<ast::PrefixExpr>(
                   assign<&ast::PrefixExpr::op>(lit<"<-">) +
                   assign<&ast::PrefixExpr::operand>(ref<CastExpr>))  //
             | create<ast::PrefixExpr>(
                   assign<&ast::PrefixExpr::op>(lit<"?">) +
                   assign<&ast::PrefixExpr::operand>(ref<CastExpr>))> {};

  struct CastExpr
      : rule<create<ast::CastExpr>(
                 lit<"("> + assign<&ast::CastExpr::type>(ref<TypeCast>) +
                 lit<")"> +
                 assign<&ast::CastExpr::operand>(ref<CastExpr>))  //
             | create<ast::CastExpr>(
                   lit<"("> +
                   flag<&ast::CastExpr::isAlloc>(kw<"alloc">) +
                   assign<&ast::CastExpr::type>(ref<TypeCast>) +
                   lit<")"> +
                   assign<&ast::CastExpr::operand>(ref<CastExpr>))  //
             | ref<MonExpr>> {};

  static constexpr auto OpMul =
      lit<"*", !lit<"=">> | lit<"/", !lit<"=">> | lit<"%", !lit<"=">>;
  static constexpr auto OpAdd = lit<"+", !lit<"=">> | lit<"-", !lit<"=">>;
  static constexpr auto OpShift =
      lit<"<<", !lit<"=">> | lit<">>", !lit<"=">>;
  static constexpr auto OpIterate = lit<"::">;
  static constexpr auto OpRel = lit<"<="> | lit<">="> | lit<"<", !cr<"<=">> |
                                 lit<">", !cr<">=">>;
  static constexpr auto OpEq = lit<"=="> | lit<"!=">;
  static constexpr auto OpBitAnd = lit<"&", !cr<"&=">>;
  static constexpr auto OpBitXor = lit<"^", !lit<"=">>;
  static constexpr auto OpBitOr = lit<"|", !cr<"|=">>;
  static constexpr auto OpLogAnd = lit<"&&">;
  static constexpr auto OpLogOr = lit<"||">;
  // The receive-assign operator is glued to one spelling: two separately
  // tokenized characters, `<-` then `=`, would let whitespace between them
  // (`a <- = b`) lex too, which no real Alef source writes.
  static constexpr auto OpAssign =
      lit<"=", !lit<"=">> | lit<":="> | lit<"<-="> | lit<"+="> | lit<"-="> |
      lit<"*="> | lit<"/="> | lit<"%="> | lit<">>="> | lit<"<<="> |
      lit<"&="> | lit<"|="> | lit<"^=">;

  // Alef's binary operators tier like a C family language: multiplicative,
  // additive, shift, the bounded-iteration `::`, relational, equality,
  // bitwise and/xor/or, logical and/or, then the thirteen assignment forms
  // lowest and right-associative.
  struct Expr
      : infix<CastExpr, ast::BinaryExpr,
              levels(left(OpMul), left(OpAdd), left(OpShift), right(OpIterate),
                     left(OpRel), left(OpEq), left(OpBitAnd), left(OpBitXor),
                     left(OpBitOr), left(OpLogAnd), left(OpLogOr),
                     right(OpAssign))> {};

  // -- statements --

  struct ExprStmt
      : rule<create<ast::ExprStmt>(
            option(assign<&ast::ExprStmt::value>(ref<Expr>)) +
            lit<";">)> {};

  struct CheckStmt
      : rule<create<ast::CheckStmt>(
            kw<"check"> + assign<&ast::CheckStmt::cond>(ref<Expr>) +
            option(lit<","> +
                   assign<&ast::CheckStmt::info>(ref<STRING_CONST>)) +
            lit<";">)> {};

  struct AllocStmt
      : rule<create<ast::AllocStmt>(
            kw<"alloc"> +
            some(append<&ast::AllocStmt::targets>(ref<Expr>), lit<",">) +
            lit<";">)> {};
  struct UnallocStmt
      : rule<create<ast::UnallocStmt>(
            kw<"unalloc"> +
            some(append<&ast::UnallocStmt::targets>(ref<Expr>),
                 lit<",">) +
            lit<";">)> {};

  struct RescueStmt
      : rule<create<ast::RescueStmt>(
                 kw<"rescue"> +
                 assign<&ast::RescueStmt::body>(ref<Statement>))  //
             | create<ast::RescueStmt>(
                   kw<"rescue"> + assign<&ast::RescueStmt::name>(ref<ID>) +
                   assign<&ast::RescueStmt::namedBlock>(ref<Block>))> {};

  struct RaiseStmt
      : rule<create<ast::RaiseStmt>(
            kw<"raise"> + option(assign<&ast::RaiseStmt::label>(ref<ID>)) +
            lit<";">)> {};

  struct GotoStmt
      : rule<create<ast::GotoStmt>(kw<"goto"> +
                                    assign<&ast::GotoStmt::target>(ref<ID>) +
                                    lit<";">)> {};

  struct ProcStmt
      : rule<create<ast::ProcStmt>(
            kw<"proc"> +
            some(append<&ast::ProcStmt::args>(ref<Expr>), lit<",">) +
            lit<";">)> {};
  struct TaskStmt
      : rule<create<ast::TaskStmt>(
            kw<"task"> +
            some(append<&ast::TaskStmt::args>(ref<Expr>), lit<",">) +
            lit<";">)> {};

  struct BecomeStmt
      : rule<create<ast::BecomeStmt>(
            kw<"become"> + assign<&ast::BecomeStmt::value>(ref<Expr>) +
            lit<";">)> {};

  struct AltStmt
      : rule<create<ast::AltStmt>(
            kw<"alt"> + assign<&ast::AltStmt::body>(ref<CaseBody>))> {};

  struct ReturnStmt
      : rule<create<ast::ReturnStmt>(
            kw<"return"> +
            option(assign<&ast::ReturnStmt::value>(ref<Expr>)) +
            lit<";">)> {};

  struct ForStmt
      : rule<create<ast::ForStmt>(
            kw<"for"> + lit<"("> +
            option(assign<&ast::ForStmt::init>(ref<Expr>)) + lit<";"> +
            option(assign<&ast::ForStmt::cond>(ref<Expr>)) + lit<";"> +
            option(assign<&ast::ForStmt::step>(ref<Expr>)) + lit<")"> +
            assign<&ast::ForStmt::body>(ref<Statement>))> {};

  struct WhileStmt
      : rule<create<ast::WhileStmt>(
            kw<"while"> + lit<"("> +
            assign<&ast::WhileStmt::cond>(ref<Expr>) + lit<")"> +
            assign<&ast::WhileStmt::body>(ref<Statement>))> {};

  struct DoStmt
      : rule<create<ast::DoStmt>(
            kw<"do"> + assign<&ast::DoStmt::body>(ref<Statement>) +
            kw<"while"> + lit<"("> +
            assign<&ast::DoStmt::cond>(ref<Expr>) + lit<")">)> {};

  struct IfStmt
      : rule<create<ast::IfStmt>(
            kw<"if"> + lit<"("> + assign<&ast::IfStmt::cond>(ref<Expr>) +
            lit<")"> + assign<&ast::IfStmt::thenBranch>(ref<Statement>) +
            option(commit(kw<"else">, assign<&ast::IfStmt::elseBranch>(ref<Statement>))))> {
  };

  struct ParStmt
      : rule<create<ast::ParStmt>(kw<"par"> +
                                   assign<&ast::ParStmt::body>(ref<Block>))> {
  };

  struct SwitchStmt
      : rule<create<ast::SwitchStmt>(
            kw<"switch"> + assign<&ast::SwitchStmt::value>(ref<Expr>) +
            assign<&ast::SwitchStmt::body>(ref<CaseBody>))> {};

  struct TypeofStmt
      : rule<create<ast::TypeofStmt>(
            kw<"typeof"> + assign<&ast::TypeofStmt::value>(ref<Expr>) +
            assign<&ast::TypeofStmt::body>(ref<TypeCaseBody>))> {};

  struct ContinueStmt
      : rule<create<ast::ContinueStmt>(
            kw<"continue"> +
            option(assign<&ast::ContinueStmt::level>(
                ref<ARITHMETIC_CONST> | ref<CONSTANT>)) +
            lit<";">)> {};
  struct BreakStmt
      : rule<create<ast::BreakStmt>(
            kw<"break"> +
            option(assign<&ast::BreakStmt::level>(
                ref<ARITHMETIC_CONST> | ref<CONSTANT>)) +
            lit<";">)> {};

  struct NlStmnt
      : rule<ref<ExprStmt>      //
             | ref<Block>        //
             | ref<CheckStmt>    //
             | ref<AllocStmt>    //
             | ref<UnallocStmt>  //
             | ref<RescueStmt>   //
             | ref<RaiseStmt>    //
             | ref<GotoStmt>     //
             | ref<ProcStmt>     //
             | ref<TaskStmt>     //
             | ref<BecomeStmt>   //
             | ref<AltStmt>      //
             | ref<ReturnStmt>   //
             | ref<ForStmt>      //
             | ref<WhileStmt>    //
             | ref<DoStmt>       //
             | ref<IfStmt>       //
             | ref<ParStmt>      //
             | ref<SwitchStmt>   //
             | ref<TypeofStmt>   //
             | ref<ContinueStmt>  //
             | ref<BreakStmt>> {};

  struct Statement
      : rule<create<ast::LabeledStmt>(
                 assign<&ast::LabeledStmt::name>(ref<ID>) + lit<":"> +
                 assign<&ast::LabeledStmt::body>(ref<Statement>))  //
             | ref<NlStmnt>> {};

  struct CaseClause
      : rule<create<ast::CaseClause>(
                 kw<"case"> + assign<&ast::CaseClause::value>(ref<Expr>) +
                 lit<":"> +
                 many(append<&ast::CaseClause::stmts>(ref<Statement>)))  //
             | create<ast::CaseClause>(
                   flag<&ast::CaseClause::isDefault>(kw<"default">) +
                   lit<":"> +
                   many(
                       append<&ast::CaseClause::stmts>(ref<Statement>)))> {
  };

  struct CaseBody
      : rule<create<ast::CaseBody>(
            option(flag<&ast::CaseBody::bang>(lit<"!">)) + lit<"{"> +
            many(append<&ast::CaseBody::cases>(ref<CaseClause>)) +
            lit<"}">)> {};

  struct TypeCaseClause
      : rule<create<ast::TypeCaseClause>(
                 kw<"case"> +
                 assign<&ast::TypeCaseClause::type>(ref<TypeCast>) +
                 lit<":"> +
                 many(append<&ast::TypeCaseClause::stmts>(
                     ref<Statement>)))  //
             | create<ast::TypeCaseClause>(
                   flag<&ast::TypeCaseClause::isDefault>(kw<"default">) +
                   lit<":"> +
                   many(append<&ast::TypeCaseClause::stmts>(
                       ref<Statement>)))> {};

  struct TypeCaseBody
      : rule<create<ast::TypeCaseBody>(
            option(flag<&ast::TypeCaseBody::bang>(lit<"!">)) + lit<"{"> +
            many(append<&ast::TypeCaseBody::cases>(ref<TypeCaseClause>)) +
            lit<"}">)> {};

  struct Block
      : rule<create<ast::Block>(
            option(flag<&ast::Block::bang>(lit<"!">)) + lit<"{"> +
            many(append<&ast::Block::autos>(ref<AutoDecl>)) +
            many(append<&ast::Block::stmts>(ref<Statement>)) +
            lit<"}">)> {};

  // -- declarations --

  struct VarDecl
      : rule<create<ast::VarDecl>(
            assign<&ast::VarDecl::type>(ref<TName>) +
            many(append<&ast::VarDecl::vars>(ref<VarInit>), lit<",">) +
            lit<";">)> {};

  struct FuncDecl
      : rule<create<ast::FuncDecl>(
                 assign<&ast::FuncDecl::returnType>(ref<TName>) +
                 assign<&ast::FuncDecl::declarator>(ref<Declarator>) +
                 lit<"("> +
                 option(assign<&ast::FuncDecl::params>(ref<ArgList>)) +
                 lit<")"> +
                 (assign<&ast::FuncDecl::body>(ref<Block>) |
                  lit<";">))  //
             | create<ast::FuncDecl>(
                   assign<&ast::FuncDecl::returnType>(ref<TName>) +
                   assign<&ast::FuncDecl::receiver>(ref<NamedType>) +
                   lit<"."> +
                   assign<&ast::FuncDecl::methodName>(ref<ID>) + lit<"("> +
                   option(assign<&ast::FuncDecl::params>(ref<ArgList>)) +
                   lit<")"> +
                   assign<&ast::FuncDecl::body>(ref<Block>))  //
             | create<ast::FuncDecl>(
                   assign<&ast::FuncDecl::returnType>(ref<TName>) +
                   flag<&ast::FuncDecl::receiverIndirect>(
                       some(lit<"*">)) +
                   assign<&ast::FuncDecl::receiver>(ref<NamedType>) +
                   lit<"."> +
                   assign<&ast::FuncDecl::methodName>(ref<ID>) + lit<"("> +
                   option(assign<&ast::FuncDecl::params>(ref<ArgList>)) +
                   lit<")"> +
                   assign<&ast::FuncDecl::body>(ref<Block>))> {};

  struct AggrType
      : rule<create<ast::AggrType>(
            kw<"aggr"> + option(append<&ast::AggrType::tags>(ref<ID>)) +
            lit<"{"> +
            some(append<&ast::AggrType::members>(ref<Decl>)) + lit<"}"> +
            option(append<&ast::AggrType::tags>(ref<ID>)))> {};

  struct UnionType
      : rule<create<ast::UnionType>(
            kw<"union"> + option(append<&ast::UnionType::tags>(ref<ID>)) +
            lit<"{"> +
            some(append<&ast::UnionType::members>(ref<Decl>)) + lit<"}"> +
            option(append<&ast::UnionType::tags>(ref<ID>)))> {};

  struct AdtType
      : rule<create<ast::AdtType>(
            kw<"adt"> + option(append<&ast::AdtType::tags>(ref<ID>)) +
            option(lit<"["> +
                   some(append<&ast::AdtType::typeParams>(ref<ID>), lit<",">) +
                   lit<"]">) +
            lit<"{"> +
            some(append<&ast::AdtType::members>(ref<Decl>)) + lit<"}"> +
            option(append<&ast::AdtType::tags>(ref<ID>)))> {};

  struct EnumMember
      : rule<assign<&ast::EnumMember::name>(ref<ID>) +
             option(lit<"="> +
                    assign<&ast::EnumMember::value>(ref<Expr>))> {};

  // An enum member may be empty too (`enum{a, , b}`) -- same fix as the
  // initializer list above.
  struct EnumType
      : rule<create<ast::EnumType>(
            kw<"enum"> + option(append<&ast::EnumType::tags>(ref<ID>)) +
            lit<"{"> +
            option(append<&ast::EnumType::members>(ref<EnumMember>)) +
            many(lit<","> + option(append<&ast::EnumType::members>(
                                 ref<EnumMember>))) +
            lit<"}">)> {};

  struct TypeSpec
      : rule<ref<AggrType>   //
             | ref<UnionType>  //
             | ref<AdtType>    //
             | ref<EnumType>> {};

  struct TypeDecl
      : rule<create<ast::TypeDecl>(
            assign<&ast::TypeDecl::type>(ref<TypeSpec>) + lit<";">)> {};

  struct TypedefDecl
      : rule<create<ast::TypedefDecl>(
                 kw<"typedef"> +
                 assign<&ast::TypedefDecl::type>(ref<TName>) +
                 assign<&ast::TypedefDecl::declarator>(ref<Declarator>) +
                 option(lit<"("> +
                        option(assign<&ast::TypedefDecl::params>(
                            ref<ArgList>)) +
                        lit<")">) +
                 lit<";">)  //
             | create<ast::TypedefDecl>(
                   kw<"typedef"> +
                   assign<&ast::TypedefDecl::bareKeyword>(
                       kw<"aggr"> | kw<"adt"> | kw<"union">) +
                   assign<&ast::TypedefDecl::declarator>(ref<Declarator>) +
                   option(lit<"("> +
                          option(assign<&ast::TypedefDecl::params>(
                              ref<ArgList>)) +
                          lit<")">) +
                   lit<";">)  //
             | create<ast::TypedefDecl>(
                   kw<"typedef"> +
                   assign<&ast::TypedefDecl::opaqueName>(ref<ID>) +
                   lit<";">)> {};

  struct Decl
      : rule<ref<FuncDecl>     //
             | ref<VarDecl>     //
             | ref<TypeDecl>    //
             | ref<TypedefDecl>> {};

  struct Program
      : rule<create<ast::Program>(
            many(append<&ast::Program::decls>(ref<Decl>)))> {};

  using entry = Program;
  using ignored = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace alef
