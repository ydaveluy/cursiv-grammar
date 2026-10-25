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

  struct STRING_CONST : terminal<lit<"\""> + many(cr<R"(^")">) + lit<"\"">> {};
  struct CONSTANT : terminal<lit<"'"> + many(cr<"^'">) + lit<"'">> {};
  struct ARITHMETIC_CONST
      : terminal<some(d) + option(lit<"."> + many(d)) +
                 option(lit<"e"> + some(d))> {};

  static constexpr auto SClassKw =
      kw<"extern"> | kw<"intern"> | kw<"private">;

  struct Pointer;
  struct ArraySuffix;
  struct Declarator;
  struct XtnameRule;
  struct TypeCastRule;
  struct TNameRule;
  struct NamedTypeRule;
  struct ArgRule;
  struct ArgListItemRule;
  struct ArgListRule;
  struct AutoDeclRule;
  struct VarInitRule;
  struct ZElistRule;
  struct InitItemRule;
  struct StatementRule;
  struct BlockRule;
  struct CaseClauseRule;
  struct CaseBodyRule;
  struct TypeCaseClauseRule;
  struct TypeCaseBodyRule;
  struct ExprRule;
  struct CastExprRule;
  struct MonExprRule;
  struct TermRule;
  struct PrimaryRule;
  struct DeclRule;

  struct Pointer : rule<create<ast::Pointer>(lit<"*">)> {};

  struct ArraySuffix
      : rule<create<ast::ArraySuffix>(
            lit<"["> + option(assign<&ast::ArraySuffix::size>(ref<ExprRule>)) +
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
                       ref<ArgListRule>)) +
                   lit<")">)> {};

  // `lint`/`ulint` join their five siblings here -- lexed, but otherwise
  // unreachable from any parser rule.
  struct XtnameRule
      : rule<create<ast::PrimType>(assign<&ast::PrimType::keyword>(
                 kw<"int"> | kw<"uint"> | kw<"sint"> | kw<"usint"> |
                 kw<"byte"> | kw<"float"> | kw<"void"> | kw<"lint"> |
                 kw<"ulint">))  //
             | create<ast::ArrayOfType>(
                   assign<&ast::ArrayOfType::name>(ref<ID>) + lit<"["> +
                   some(append<&ast::ArrayOfType::args>(ref<TypeCastRule>),
                        lit<",">) +
                   lit<"]">)  //
             | create<ast::ChanType>(
                   kw<"chan"> + lit<"("> +
                   some(append<&ast::ChanType::args>(ref<TypeCastRule>),
                        lit<",">) +
                   lit<")"> +
                   option(lit<"["> +
                          assign<&ast::ChanType::bufSize>(ref<ExprRule>) +
                          lit<"]">))  //
             | create<ast::NamedType>(
                   assign<&ast::NamedType::name>(ref<ID>))> {};

  struct NamedTypeRule
      : rule<create<ast::NamedType>(assign<&ast::NamedType::name>(ref<ID>))> {
  };

  struct TypeCastRule
      : rule<create<ast::TypeCast>(
                 assign<&ast::TypeCast::type>(ref<XtnameRule>) +
                 many(append<&ast::TypeCast::pointers>(ref<Pointer>)))  //
             | create<ast::TypeCast>(
                   assign<&ast::TypeCast::type>(ref<XtnameRule>) +
                   flag<&ast::TypeCast::isFuncType>(lit<"(">) +
                   some(append<&ast::TypeCast::innerPointers>(ref<Pointer>)) +
                   lit<")"> + lit<"("> +
                   option(assign<&ast::TypeCast::params>(ref<ArgListRule>)) +
                   lit<")">)  //
             | create<ast::TypeCast>(
                   kw<"tuple"> +
                   assign<&ast::TypeCast::tuple>(ref<TNameRule>))> {};

  struct TNameRule
      : rule<create<ast::TName>(
                 option(assign<&ast::TName::sclass>(SClassKw)) +
                 assign<&ast::TName::type>(ref<XtnameRule>))  //
             | create<ast::TName>(
                   option(assign<&ast::TName::sclass>(SClassKw)) +
                   flag<&ast::TName::tupleKeyword>(kw<"tuple">) +
                   lit<"("> +
                   some(append<&ast::TName::tupleTypes>(ref<TypeCastRule>),
                        lit<",">) +
                   lit<")">)  //
             | create<ast::TName>(
                   option(assign<&ast::TName::sclass>(SClassKw)) + lit<"("> +
                   some(append<&ast::TName::tupleTypes>(ref<TypeCastRule>),
                        lit<",">) +
                   lit<")">)> {};

  struct ArgListRule
      : rule<create<ast::ArgList>(
            many(append<&ast::ArgList::params>(ref<ArgListItemRule>) +
                 lit<",">) +
            append<&ast::ArgList::params>(ref<ArgRule>))> {};

  struct ArgListItemRule
      : rule<ref<ArgRule>  //
             | create<ast::Arg>(
                   flag<&ast::Arg::isRestStar>(lit<"*">) +
                   assign<&ast::Arg::type>(ref<XtnameRule>))  //
             | create<ast::Arg>(
                   flag<&ast::Arg::isRestDot>(lit<".">) +
                   assign<&ast::Arg::type>(ref<XtnameRule>))> {};

  struct ArgRule
      : rule<create<ast::Arg>(
                 assign<&ast::Arg::type>(ref<XtnameRule>) +
                 many(append<&ast::Arg::pointers>(ref<Pointer>)) +
                 many(append<&ast::Arg::dims>(ref<ArraySuffix>)))  //
             | create<ast::Arg>(
                   assign<&ast::Arg::type>(ref<XtnameRule>) +
                   many(append<&ast::Arg::pointers>(ref<Pointer>)) +
                   flag<&ast::Arg::isFuncPtr>(lit<"(">) +
                   some(append<&ast::Arg::innerPointers>(ref<Pointer>)) +
                   lit<")"> + lit<"("> +
                   option(assign<&ast::Arg::funcPtrParams>(ref<ArgListRule>)) +
                   lit<")">)  //
             | create<ast::Arg>(kw<"tuple"> +
                                 assign<&ast::Arg::tupleType>(ref<TNameRule>))  //
             | create<ast::Arg>(
                   kw<"tuple"> + assign<&ast::Arg::tupleType>(ref<TNameRule>) +
                   flag<&ast::Arg::isFuncPtr>(lit<"(">) +
                   some(append<&ast::Arg::innerPointers>(ref<Pointer>)) +
                   lit<")"> + lit<"("> +
                   option(assign<&ast::Arg::funcPtrParams>(ref<ArgListRule>)) +
                   lit<")">)  //
             | create<ast::Arg>(
                   kw<"tuple"> + assign<&ast::Arg::tupleType>(ref<TNameRule>) +
                   assign<&ast::Arg::declarator>(ref<Declarator>))  //
             | create<ast::Arg>(assign<&ast::Arg::type>(ref<XtnameRule>) +
                                 assign<&ast::Arg::declarator>(ref<Declarator>))  //
             | create<ast::Arg>(
                   flag<&ast::Arg::isVariadic>(lit<"...">))> {};

  struct VarInitRule
      : rule<assign<&ast::VarInit::declarator>(ref<Declarator>) +
             option(lit<"="> +
                    assign<&ast::VarInit::init>(ref<ZElistRule>))> {};

  // Empty initializer positions are real items. Each repeated tail also
  // consumes a comma, so even an empty item's position is retained.
  struct ZElistRule
      : rule<create<ast::InitList>(append<&ast::InitList::items>(ref<InitItemRule>) +
            many(lit<","> + append<&ast::InitList::items>(ref<InitItemRule>)))> {
  };

  struct InitItemRule
      : rule<create<ast::InitItem>(
                 lit<"["> + assign<&ast::InitItem::index>(ref<ExprRule>) +
                 lit<"]"> + lit<"{"> +
                 assign<&ast::InitItem::braced>(ref<ZElistRule>) +
                 lit<"}">)  //
             | create<ast::InitItem>(
                   lit<"["> + assign<&ast::InitItem::index>(ref<ExprRule>) +
                   lit<"]"> +
                   assign<&ast::InitItem::indexedValue>(ref<ExprRule>))  //
             | create<ast::InitItem>(
                   lit<"."> + assign<&ast::InitItem::field>(ref<ID>) +
                   assign<&ast::InitItem::fieldValue>(ref<ExprRule>))  //
             | create<ast::InitItem>(
                   lit<"{"> + assign<&ast::InitItem::braced>(ref<ZElistRule>) +
                   lit<"}">)  //
             | create<ast::InitItem>(
                   assign<&ast::InitItem::value>(ref<ExprRule>)) |
             create<ast::InitItem>> {};

  struct AutoDeclRule
      : rule<create<ast::AutoDecl>(
                 assign<&ast::AutoDecl::type>(ref<XtnameRule>) +
                 many(append<&ast::AutoDecl::vars>(ref<VarInitRule>),
                      lit<",">) +
                 lit<";">)  //
             | create<ast::AutoDecl>(
                   kw<"tuple"> +
                   assign<&ast::AutoDecl::tupleType>(ref<TNameRule>) +
                   many(append<&ast::AutoDecl::vars>(ref<VarInitRule>),
                        lit<",">) +
                   lit<";">)> {};

  // -- expressions --

  struct TcompRule;
  struct BracedTupleRule
      : rule<create<ast::TupleExpr>(
            flag<&ast::TupleExpr::braced>(lit<"{">) +
            option(some(append<&ast::TupleExpr::items>(ref<TcompRule>),
                        lit<",">)) +
            lit<"}">)> {};
  struct TcompRule : rule<ref<ExprRule> | ref<BracedTupleRule>> {};

  struct TupleParenRule
      : rule<create<ast::TupleExpr>(
            lit<"("> +
            some(append<&ast::TupleExpr::items>(ref<TcompRule>), lit<",">) +
            lit<")">)> {};

  struct SizeofTypeExprRule
      : rule<create<ast::SizeofTypeExpr>(
            kw<"sizeof"> + lit<"("> +
            assign<&ast::SizeofTypeExpr::type>(ref<TypeCastRule>) +
            lit<")">)> {};

  struct QualifiedRefRule
      : rule<create<ast::QualifiedRef>(
            lit<"."> + assign<&ast::QualifiedRef::scope>(ref<ID>) + lit<"."> +
            assign<&ast::QualifiedRef::field>(ref<ID>))> {};

  struct EllipsisExprRule : rule<create<ast::EllipsisExpr>(lit<"...">)> {};
  struct NilExprRule : rule<create<ast::NilExpr>(kw<"nil">)> {};
  struct IdentRule : rule<assign<&ast::Ident::name>(ref<ID>)> {};
  struct NumberLitRule
      : rule<assign<&ast::NumberLit::text>(ref<ARITHMETIC_CONST>)> {};
  struct CharLitRule : rule<assign<&ast::CharLit::text>(ref<CONSTANT>)> {};
  struct StringLitRule
      : rule<assign<&ast::StringLit::text>(ref<STRING_CONST>)> {};
  struct DollarStringRule
      : rule<create<ast::StringLit>(
            flag<&ast::StringLit::dollar>(lit<"$">) +
            assign<&ast::StringLit::text>(ref<STRING_CONST>))> {};

  struct PrimaryRule
      : rule<ref<TupleParenRule>       //
             | ref<SizeofTypeExprRule>  //
             | ref<QualifiedRefRule>    //
             | ref<EllipsisExprRule>    //
             | ref<NilExprRule>         //
             | ref<NumberLitRule>       //
             | ref<CharLitRule>         //
             | ref<DollarStringRule>    //
             | ref<StringLitRule>       //
             | ref<IdentRule>> {};

  struct TermRule
      : rule<ref<PrimaryRule> +
             many(nest<ast::CallExpr, &ast::CallExpr::callee>(
                      lit<"("> +
                      option(some(append<&ast::CallExpr::args>(ref<ExprRule>),
                                  lit<",">)) +
                      lit<")">)  //
                  | nest<ast::IndexExpr, &ast::IndexExpr::array>(
                        lit<"["> +
                        assign<&ast::IndexExpr::index>(ref<ExprRule>) +
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

  struct MonExprRule
      : rule<ref<TermRule>  //
             | create<ast::PrefixExpr>(
                   assign<&ast::PrefixExpr::op>(lit<"*"> | lit<"&"> |
                                                 lit<"+"> | lit<"-">) +
                   assign<&ast::PrefixExpr::operand>(ref<CastExprRule>))  //
             | create<ast::PrefixExpr>(
                   assign<&ast::PrefixExpr::op>(lit<"--"> | lit<"++">) +
                   assign<&ast::PrefixExpr::operand>(ref<CastExprRule>))  //
             | create<ast::PrefixExpr>(
                   assign<&ast::PrefixExpr::op>(kw<"zerox">) +
                   assign<&ast::PrefixExpr::operand>(ref<CastExprRule>))  //
             | create<ast::PrefixExpr>(
                   assign<&ast::PrefixExpr::op>(lit<"!"> | lit<"~">) +
                   assign<&ast::PrefixExpr::operand>(ref<CastExprRule>))  //
             | create<ast::SizeofExpr>(
                   kw<"sizeof"> +
                   assign<&ast::SizeofExpr::operand>(ref<MonExprRule>))  //
             | create<ast::PrefixExpr>(
                   assign<&ast::PrefixExpr::op>(lit<"<-">) +
                   assign<&ast::PrefixExpr::operand>(ref<CastExprRule>))  //
             | create<ast::PrefixExpr>(
                   assign<&ast::PrefixExpr::op>(lit<"?">) +
                   assign<&ast::PrefixExpr::operand>(ref<CastExprRule>))> {};

  struct CastExprRule
      : rule<create<ast::CastExpr>(
                 lit<"("> + assign<&ast::CastExpr::type>(ref<TypeCastRule>) +
                 lit<")"> +
                 assign<&ast::CastExpr::operand>(ref<CastExprRule>))  //
             | create<ast::CastExpr>(
                   lit<"("> +
                   flag<&ast::CastExpr::isAlloc>(kw<"alloc">) +
                   assign<&ast::CastExpr::type>(ref<TypeCastRule>) +
                   lit<")"> +
                   assign<&ast::CastExpr::operand>(ref<CastExprRule>))  //
             | ref<MonExprRule>> {};

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
  struct ExprRule
      : infix<CastExprRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(OpMul) >> left(OpAdd) >> left(OpShift) >>
               right(OpIterate) >> left(OpRel) >> left(OpEq) >>
               left(OpBitAnd) >> left(OpBitXor) >> left(OpBitOr) >>
               left(OpLogAnd) >> left(OpLogOr) >> right(OpAssign))> {};

  // -- statements --

  struct ExprStmtRule
      : rule<create<ast::ExprStmt>(
            option(assign<&ast::ExprStmt::value>(ref<ExprRule>)) +
            lit<";">)> {};

  struct CheckStmtRule
      : rule<create<ast::CheckStmt>(
            kw<"check"> + assign<&ast::CheckStmt::cond>(ref<ExprRule>) +
            option(lit<","> +
                   assign<&ast::CheckStmt::info>(ref<STRING_CONST>)) +
            lit<";">)> {};

  struct AllocStmtRule
      : rule<create<ast::AllocStmt>(
            kw<"alloc"> +
            some(append<&ast::AllocStmt::targets>(ref<ExprRule>), lit<",">) +
            lit<";">)> {};
  struct UnallocStmtRule
      : rule<create<ast::UnallocStmt>(
            kw<"unalloc"> +
            some(append<&ast::UnallocStmt::targets>(ref<ExprRule>),
                 lit<",">) +
            lit<";">)> {};

  struct RescueStmtRule
      : rule<create<ast::RescueStmt>(
                 kw<"rescue"> +
                 assign<&ast::RescueStmt::body>(ref<StatementRule>))  //
             | create<ast::RescueStmt>(
                   kw<"rescue"> + assign<&ast::RescueStmt::name>(ref<ID>) +
                   assign<&ast::RescueStmt::namedBlock>(ref<BlockRule>))> {};

  struct RaiseStmtRule
      : rule<create<ast::RaiseStmt>(
            kw<"raise"> + option(assign<&ast::RaiseStmt::label>(ref<ID>)) +
            lit<";">)> {};

  struct GotoStmtRule
      : rule<create<ast::GotoStmt>(kw<"goto"> +
                                    assign<&ast::GotoStmt::target>(ref<ID>) +
                                    lit<";">)> {};

  struct ProcStmtRule
      : rule<create<ast::ProcStmt>(
            kw<"proc"> +
            some(append<&ast::ProcStmt::args>(ref<ExprRule>), lit<",">) +
            lit<";">)> {};
  struct TaskStmtRule
      : rule<create<ast::TaskStmt>(
            kw<"task"> +
            some(append<&ast::TaskStmt::args>(ref<ExprRule>), lit<",">) +
            lit<";">)> {};

  struct BecomeStmtRule
      : rule<create<ast::BecomeStmt>(
            kw<"become"> + assign<&ast::BecomeStmt::value>(ref<ExprRule>) +
            lit<";">)> {};

  struct AltStmtRule
      : rule<create<ast::AltStmt>(
            kw<"alt"> + assign<&ast::AltStmt::body>(ref<CaseBodyRule>))> {};

  struct ReturnStmtRule
      : rule<create<ast::ReturnStmt>(
            kw<"return"> +
            option(assign<&ast::ReturnStmt::value>(ref<ExprRule>)) +
            lit<";">)> {};

  struct ForStmtRule
      : rule<create<ast::ForStmt>(
            kw<"for"> + lit<"("> +
            option(assign<&ast::ForStmt::init>(ref<ExprRule>)) + lit<";"> +
            option(assign<&ast::ForStmt::cond>(ref<ExprRule>)) + lit<";"> +
            option(assign<&ast::ForStmt::step>(ref<ExprRule>)) + lit<")"> +
            assign<&ast::ForStmt::body>(ref<StatementRule>))> {};

  struct WhileStmtRule
      : rule<create<ast::WhileStmt>(
            kw<"while"> + lit<"("> +
            assign<&ast::WhileStmt::cond>(ref<ExprRule>) + lit<")"> +
            assign<&ast::WhileStmt::body>(ref<StatementRule>))> {};

  struct DoStmtRule
      : rule<create<ast::DoStmt>(
            kw<"do"> + assign<&ast::DoStmt::body>(ref<StatementRule>) +
            kw<"while"> + lit<"("> +
            assign<&ast::DoStmt::cond>(ref<ExprRule>) + lit<")">)> {};

  struct IfStmtRule
      : rule<create<ast::IfStmt>(
            kw<"if"> + lit<"("> + assign<&ast::IfStmt::cond>(ref<ExprRule>) +
            lit<")"> + assign<&ast::IfStmt::thenBranch>(ref<StatementRule>) +
            option(commit(kw<"else">, assign<&ast::IfStmt::elseBranch>(ref<StatementRule>))))> {
  };

  struct ParStmtRule
      : rule<create<ast::ParStmt>(kw<"par"> +
                                   assign<&ast::ParStmt::body>(ref<BlockRule>))> {
  };

  struct SwitchStmtRule
      : rule<create<ast::SwitchStmt>(
            kw<"switch"> + assign<&ast::SwitchStmt::value>(ref<ExprRule>) +
            assign<&ast::SwitchStmt::body>(ref<CaseBodyRule>))> {};

  struct TypeofStmtRule
      : rule<create<ast::TypeofStmt>(
            kw<"typeof"> + assign<&ast::TypeofStmt::value>(ref<ExprRule>) +
            assign<&ast::TypeofStmt::body>(ref<TypeCaseBodyRule>))> {};

  struct ContinueStmtRule
      : rule<create<ast::ContinueStmt>(
            kw<"continue"> +
            option(assign<&ast::ContinueStmt::level>(ref<CONSTANT>)) +
            lit<";">)> {};
  struct BreakStmtRule
      : rule<create<ast::BreakStmt>(
            kw<"break"> +
            option(assign<&ast::BreakStmt::level>(ref<CONSTANT>)) +
            lit<";">)> {};

  struct NlStmntRule
      : rule<ref<ExprStmtRule>      //
             | ref<BlockRule>        //
             | ref<CheckStmtRule>    //
             | ref<AllocStmtRule>    //
             | ref<UnallocStmtRule>  //
             | ref<RescueStmtRule>   //
             | ref<RaiseStmtRule>    //
             | ref<GotoStmtRule>     //
             | ref<ProcStmtRule>     //
             | ref<TaskStmtRule>     //
             | ref<BecomeStmtRule>   //
             | ref<AltStmtRule>      //
             | ref<ReturnStmtRule>   //
             | ref<ForStmtRule>      //
             | ref<WhileStmtRule>    //
             | ref<DoStmtRule>       //
             | ref<IfStmtRule>       //
             | ref<ParStmtRule>      //
             | ref<SwitchStmtRule>   //
             | ref<TypeofStmtRule>   //
             | ref<ContinueStmtRule>  //
             | ref<BreakStmtRule>> {};

  struct StatementRule
      : rule<create<ast::LabeledStmt>(
                 assign<&ast::LabeledStmt::name>(ref<ID>) + lit<":"> +
                 assign<&ast::LabeledStmt::body>(ref<StatementRule>))  //
             | ref<NlStmntRule>> {};

  struct CaseClauseRule
      : rule<create<ast::CaseClause>(
                 kw<"case"> + assign<&ast::CaseClause::value>(ref<ExprRule>) +
                 lit<":"> +
                 many(append<&ast::CaseClause::stmts>(ref<StatementRule>)))  //
             | create<ast::CaseClause>(
                   flag<&ast::CaseClause::isDefault>(kw<"default">) +
                   lit<":"> +
                   many(
                       append<&ast::CaseClause::stmts>(ref<StatementRule>)))> {
  };

  struct CaseBodyRule
      : rule<create<ast::CaseBody>(
                 lit<"{"> +
                 many(append<&ast::CaseBody::cases>(ref<CaseClauseRule>)) +
                 lit<"}">)  //
             | create<ast::CaseBody>(
                   flag<&ast::CaseBody::bang>(lit<"!">) + lit<"{"> +
                   many(append<&ast::CaseBody::cases>(ref<CaseClauseRule>)) +
                   lit<"}">)> {};

  struct TypeCaseClauseRule
      : rule<create<ast::TypeCaseClause>(
                 kw<"case"> +
                 assign<&ast::TypeCaseClause::type>(ref<TypeCastRule>) +
                 lit<":"> +
                 many(append<&ast::TypeCaseClause::stmts>(
                     ref<StatementRule>)))  //
             | create<ast::TypeCaseClause>(
                   flag<&ast::TypeCaseClause::isDefault>(kw<"default">) +
                   lit<":"> +
                   many(append<&ast::TypeCaseClause::stmts>(
                       ref<StatementRule>)))> {};

  struct TypeCaseBodyRule
      : rule<create<ast::TypeCaseBody>(
                 lit<"{"> +
                 many(append<&ast::TypeCaseBody::cases>(
                     ref<TypeCaseClauseRule>)) +
                 lit<"}">)  //
             | create<ast::TypeCaseBody>(
                   flag<&ast::TypeCaseBody::bang>(lit<"!">) + lit<"{"> +
                   many(append<&ast::TypeCaseBody::cases>(
                       ref<TypeCaseClauseRule>)) +
                   lit<"}">)> {};

  struct BlockRule
      : rule<create<ast::Block>(
                 lit<"{"> +
                 many(append<&ast::Block::autos>(ref<AutoDeclRule>)) +
                 many(append<&ast::Block::stmts>(ref<StatementRule>)) +
                 lit<"}">)  //
             | create<ast::Block>(
                   flag<&ast::Block::bang>(lit<"!">) + lit<"{"> +
                   many(append<&ast::Block::autos>(ref<AutoDeclRule>)) +
                   many(append<&ast::Block::stmts>(ref<StatementRule>)) +
                   lit<"}">)> {};

  // -- declarations --

  struct VarDeclRule
      : rule<create<ast::VarDecl>(
            assign<&ast::VarDecl::type>(ref<TNameRule>) +
            many(append<&ast::VarDecl::vars>(ref<VarInitRule>), lit<",">) +
            lit<";">)> {};

  struct FuncDeclRule
      : rule<create<ast::FuncDecl>(
                 assign<&ast::FuncDecl::returnType>(ref<TNameRule>) +
                 assign<&ast::FuncDecl::declarator>(ref<Declarator>) +
                 lit<"("> +
                 option(assign<&ast::FuncDecl::params>(ref<ArgListRule>)) +
                 lit<")"> +
                 (assign<&ast::FuncDecl::body>(ref<BlockRule>) |
                  lit<";">))  //
             | create<ast::FuncDecl>(
                   assign<&ast::FuncDecl::returnType>(ref<TNameRule>) +
                   assign<&ast::FuncDecl::receiver>(ref<NamedTypeRule>) +
                   lit<"."> +
                   assign<&ast::FuncDecl::methodName>(ref<ID>) + lit<"("> +
                   option(assign<&ast::FuncDecl::params>(ref<ArgListRule>)) +
                   lit<")"> +
                   assign<&ast::FuncDecl::body>(ref<BlockRule>))  //
             | create<ast::FuncDecl>(
                   assign<&ast::FuncDecl::returnType>(ref<TNameRule>) +
                   flag<&ast::FuncDecl::receiverIndirect>(
                       some(lit<"*">)) +
                   assign<&ast::FuncDecl::receiver>(ref<NamedTypeRule>) +
                   lit<"."> +
                   assign<&ast::FuncDecl::methodName>(ref<ID>) + lit<"("> +
                   option(assign<&ast::FuncDecl::params>(ref<ArgListRule>)) +
                   lit<")"> +
                   assign<&ast::FuncDecl::body>(ref<BlockRule>))> {};

  struct AggrTypeRule
      : rule<create<ast::AggrType>(
            kw<"aggr"> + option(append<&ast::AggrType::tags>(ref<ID>)) +
            lit<"{"> +
            some(append<&ast::AggrType::members>(ref<DeclRule>)) + lit<"}"> +
            option(append<&ast::AggrType::tags>(ref<ID>)))> {};

  struct UnionTypeRule
      : rule<create<ast::UnionType>(
            kw<"union"> + option(append<&ast::UnionType::tags>(ref<ID>)) +
            lit<"{"> +
            some(append<&ast::UnionType::members>(ref<DeclRule>)) + lit<"}"> +
            option(append<&ast::UnionType::tags>(ref<ID>)))> {};

  struct AdtTypeRule
      : rule<create<ast::AdtType>(
            kw<"adt"> + option(append<&ast::AdtType::tags>(ref<ID>)) +
            option(lit<"["> +
                   some(append<&ast::AdtType::typeParams>(ref<ID>), lit<",">) +
                   lit<"]">) +
            lit<"{"> +
            some(append<&ast::AdtType::members>(ref<DeclRule>)) + lit<"}"> +
            option(append<&ast::AdtType::tags>(ref<ID>)))> {};

  struct EnumMemberRule
      : rule<assign<&ast::EnumMember::name>(ref<ID>) +
             option(lit<"="> +
                    assign<&ast::EnumMember::value>(ref<ExprRule>))> {};

  // An enum member may be empty too (`enum{a, , b}`) -- same fix as the
  // initializer list above.
  struct EnumTypeRule
      : rule<create<ast::EnumType>(
            kw<"enum"> + option(append<&ast::EnumType::tags>(ref<ID>)) +
            lit<"{"> +
            option(append<&ast::EnumType::members>(ref<EnumMemberRule>)) +
            many(lit<","> + option(append<&ast::EnumType::members>(
                                 ref<EnumMemberRule>))) +
            lit<"}">)> {};

  struct TypeSpecRule
      : rule<ref<AggrTypeRule>   //
             | ref<UnionTypeRule>  //
             | ref<AdtTypeRule>    //
             | ref<EnumTypeRule>> {};

  struct TypeDeclRule
      : rule<create<ast::TypeDecl>(
            assign<&ast::TypeDecl::type>(ref<TypeSpecRule>) + lit<";">)> {};

  struct TypedefDeclRule
      : rule<create<ast::TypedefDecl>(
                 kw<"typedef"> +
                 assign<&ast::TypedefDecl::type>(ref<TNameRule>) +
                 assign<&ast::TypedefDecl::declarator>(ref<Declarator>) +
                 option(lit<"("> +
                        option(assign<&ast::TypedefDecl::params>(
                            ref<ArgListRule>)) +
                        lit<")">) +
                 lit<";">)  //
             | create<ast::TypedefDecl>(
                   kw<"typedef"> +
                   assign<&ast::TypedefDecl::bareKeyword>(
                       kw<"aggr"> | kw<"adt"> | kw<"union">) +
                   assign<&ast::TypedefDecl::declarator>(ref<Declarator>) +
                   option(lit<"("> +
                          option(assign<&ast::TypedefDecl::params>(
                              ref<ArgListRule>)) +
                          lit<")">) +
                   lit<";">)  //
             | create<ast::TypedefDecl>(
                   kw<"typedef"> +
                   assign<&ast::TypedefDecl::opaqueName>(ref<ID>) +
                   lit<";">)> {};

  struct DeclRule
      : rule<ref<FuncDeclRule>     //
             | ref<VarDeclRule>     //
             | ref<TypeDeclRule>    //
             | ref<TypedefDeclRule>> {};

  struct ProgramRule
      : rule<create<ast::Program>(
            many(append<&ast::Program::decls>(ref<DeclRule>)))> {};

  using entry = ProgramRule;
  using ignore = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace alef
