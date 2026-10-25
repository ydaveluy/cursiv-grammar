#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "zig/core/ast.hpp"

namespace zig {

inline constexpr auto rawText = [](cursiv::Parts parts) -> std::string_view {
  if (parts.empty()) return {};
  const auto first = parts.front();
  const auto last = parts.back();
  return {first.data(), static_cast<std::size_t>(last.data() + last.size() - first.data())};
};
using RawText = cursiv::parts_conv<rawText>;

struct Zig : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  // Empty line comments stop before the newline; /// and //! stay documentation.
  struct LineComment
      : terminal<lit<"//"> + !(lit<"/", !lit<"/">> | lit<"!">) + many(cr<R"(^\r\n)">)> {};

  struct DocComment
      : terminal<lit<"///"> + option(cr<R"(^/\r\n)"> + many(cr<R"(^\r\n)">))> {};
  struct ContainerDocComment
      : terminal<lit<"//!"> + many(cr<R"(^\r\n)">)> {};

  struct DocComments : rule<some(ref<DocComment>),
                            RawText> {};
  struct ContainerDocComments
      : rule<some(ref<ContainerDocComment>),
             RawText> {};

  static constexpr auto Letter = cr<"a-zA-Z_">;
  static constexpr auto LetterOrDigit = cr<"a-zA-Z0-9_">;
  static constexpr auto QuotedBody =
      lit<"\""> + many(cr<R"(^\\"\n)"> | (lit<"\\"> + any)) + lit<"\"">;
  static constexpr auto Reserved =
      kw<"addrspace"> | kw<"align"> | kw<"allowzero"> | kw<"and"> |
      kw<"anyframe"> | kw<"anytype"> | kw<"asm"> | kw<"break"> |
      kw<"callconv"> | kw<"catch"> | kw<"comptime"> | kw<"const"> |
      kw<"continue"> | kw<"defer"> | kw<"else"> | kw<"enum"> |
      kw<"errdefer"> | kw<"error"> | kw<"export"> | kw<"extern"> |
      kw<"fn"> | kw<"for"> | kw<"if"> | kw<"inline"> |
      kw<"noalias"> | kw<"noinline"> | kw<"nosuspend"> | kw<"opaque"> |
      kw<"or"> | kw<"orelse"> | kw<"packed"> | kw<"pub"> |
      kw<"resume"> | kw<"return"> | kw<"linksection"> | kw<"struct"> |
      kw<"suspend"> | kw<"switch"> | kw<"test"> | kw<"threadlocal"> |
      kw<"try"> | kw<"union"> | kw<"unreachable"> | kw<"var"> |
      kw<"volatile"> | kw<"while">;
  struct IdentifierToken
      : terminal<(Letter + many(LetterOrDigit)) | (lit<"@"> + QuotedBody)> {};
  struct ID : rule<!Reserved + ref<IdentifierToken>,
                   RawText> {};
  static constexpr auto word_boundary = LetterOrDigit;

  static constexpr auto LineString = lit<"\\\\"> + many(cr<R"(^\n)">);
  struct QuotedString : terminal<QuotedBody> {};
  struct LineStringLiteral : terminal<LineString> {};
  struct STRINGLITERAL
      : rule<ref<QuotedString> | some(ref<LineStringLiteral>),
             RawText> {};
  struct CHARLITERAL
      : terminal<lit<"'"> + some(cr<R"(^\\'\n)"> | (lit<"\\"> + any)) +
                 lit<"'">> {};
  static constexpr auto NumberChars = cr<"a-zA-Z0-9_">;
  static constexpr auto SignedExponent = cr<"eEpP"> + cr<"+-">;
  static constexpr auto FloatTail = many(NumberChars | SignedExponent);
  struct NUMBER
      : terminal<cr<"0-9"> + many(NumberChars) +
                 option((SignedExponent + FloatTail) |
                        (lit<"."> + (NumberChars | SignedExponent) + FloatTail))> {};
  struct BUILTINIDENT
      : terminal<lit<"@"> + Letter + many(LetterOrDigit)> {};

  // --- forward declarations -------------------------------------------

  struct ContainerMembers;
  struct ContainerDeclaration;
  struct ContainerField;
  struct FnProto;
  struct ParamDeclRule;
  struct VarDeclProto;
  struct Block;
  struct Statement;
  struct VarDeclExprStmtRule;
  struct Expr;
  struct AssignExpr;
  struct SimpleAssignExpr;
  struct TypeExpr;
  struct PrefixTypeExpr;
  struct ErrorUnionExpr;
  struct SuffixExpr;
  struct PrimaryTypeExpr;
  struct PrimaryExpr;
  struct PrefixExpr;
  struct BitwiseChain;
  struct IfExpr;
  struct IfTypeExpr;
  struct IfPrefixRule;
  struct LoopExpr;
  struct ForPrefixRule;
  struct WhilePrefixRule;
  struct SwitchExprRule;
  struct InitList;
  struct FieldInit;
  struct ContainerDecl;
  struct ErrorSetDecl;
  struct AsmExpr;

  // --- operator groups ---------------------------------------------------

  // Every `=` is this one terminal: a second spelling lexes as a second token.
  static constexpr auto Assign = lit<"=", !cr<"=>">>;

  static constexpr auto AssignOp =
      lit<"<<|="> | lit<"<<="> | lit<">>="> | lit<"*|="> | lit<"+|="> |
      lit<"-|="> | lit<"*%="> | lit<"+%="> | lit<"-%="> | lit<"*="> |
      lit<"/="> | lit<"%="> | lit<"+="> | lit<"-="> | lit<"&="> |
      lit<"^="> | lit<"|="> | Assign;

  static constexpr auto CompareOp = lit<"=="> | lit<"!="> | lit<"<="> |
                                    lit<">="> | lit<"<", !cr<"<">> |
                                    lit<">", !cr<">">>;

  // An infix operator hands over text; catch's optional `|payload|` gives it
  // structure, so it needs a data rule (a converter, not a node) rather than
  // a plain literal union.
  struct CatchOpRule
      : rule<kw<"catch"> + option(lit<"|"> + ref<ID> + lit<"|">),
             RawText> {};
  static constexpr auto BitwiseOp = lit<"&", !cr<"=">> | lit<"^", !cr<"=">> |
                                    lit<"|", !cr<"|=">> | kw<"orelse"> |
                                    ref<CatchOpRule>;

  static constexpr auto BitShiftOp =
      lit<"<<|"> | lit<"<<", !cr<"|=">> | lit<">>", !cr<"=">>;

  static constexpr auto AdditionOp = lit<"++"> | lit<"+%"> | lit<"+|"> |
                                     lit<"+", !cr<"%|=">> | lit<"-%"> |
                                     lit<"-|"> | lit<"-", !cr<"%|=>">>;

  static constexpr auto MultiplyOp = lit<"||"> | lit<"*%"> |
                                     lit<"*|"> | lit<"*", !cr<"%|=">> |
                                     lit<"/", !cr<"=">> | lit<"%", !cr<"=">>;

  static constexpr auto PrefixOp = lit<"!"> | lit<"-%"> |
                                   lit<"-", !cr<"%|=>">> | lit<"~"> |
                                   lit<"&", !cr<"=">> | kw<"try">;

  // --- payloads, prefixes shared across statement/expr/type positions ---

  struct IfPrefixRule
      : rule<kw<"if"> + lit<"("> +
             assign<&ast::IfPrefix::cond>(ref<Expr>) + lit<")"> +
             option(lit<"|"> +
                    option(flag<&ast::IfPrefix::payloadPtr>(lit<"*">)) +
                    assign<&ast::IfPrefix::payload>(ref<ID>) + lit<"|">)> {};

  struct WhilePrefixRule
      : rule<kw<"while"> + lit<"("> +
             assign<&ast::WhilePrefix::cond>(ref<Expr>) + lit<")"> +
             option(lit<"|"> +
                    option(flag<&ast::WhilePrefix::payloadPtr>(
                        lit<"*">)) +
                    assign<&ast::WhilePrefix::payload>(ref<ID>) + lit<"|">) +
             option(lit<":"> + lit<"("> +
                    assign<&ast::WhilePrefix::continueExpr>(
                        ref<AssignExpr>) +
                    lit<")">)> {};

  struct ForItemRule
      : rule<create<ast::ForItem>(
            assign<&ast::ForItem::value>(ref<Expr>) +
            option(flag<&ast::ForItem::isRange>(lit<"..">) +
                   option(assign<&ast::ForItem::rangeEnd>(ref<Expr>))))> {};

  struct PtrListItemRule
      : rule<create<ast::PtrListItem>(
            option(flag<&ast::PtrListItem::ptr>(lit<"*">)) +
            assign<&ast::PtrListItem::name>(ref<ID>))> {};

  struct ForPrefixRule
      : rule<kw<"for"> + lit<"("> +
             append<&ast::ForPrefix::items>(ref<ForItemRule>) +
             many(lit<","> +
                  append<&ast::ForPrefix::items>(ref<ForItemRule>)) +
             option(lit<",">) + lit<")"> + lit<"|"> +
             append<&ast::ForPrefix::payload>(ref<PtrListItemRule>) +
             many(lit<","> +
                  append<&ast::ForPrefix::payload>(ref<PtrListItemRule>)) +
             option(lit<",">) + lit<"|">> {};

  // --- switch ---------------------------------------------------------

  struct SwitchItemRule
      : rule<create<ast::SwitchItem>(
            assign<&ast::SwitchItem::value>(ref<Expr>) +
            option(flag<&ast::SwitchItem::isRange>(lit<"...">) +
                   assign<&ast::SwitchItem::rangeEnd>(ref<Expr>)))> {};

  struct SwitchProngRule
      : rule<create<ast::SwitchProng>(
            option(flag<&ast::SwitchProng::inlineProng>(
                kw<"inline">)) +
            ((append<&ast::SwitchProng::items>(ref<SwitchItemRule>) +
              many(lit<","> + append<&ast::SwitchProng::items>(
                                   ref<SwitchItemRule>)) +
              option(lit<",">)) |
             flag<&ast::SwitchProng::isElse>(kw<"else">)) +
            lit<"=>"> +
            option(lit<"|"> +
                   append<&ast::SwitchProng::payload>(ref<PtrListItemRule>) +
                   option(lit<","> + append<&ast::SwitchProng::payload>(
                                          ref<PtrListItemRule>)) +
                   lit<"|">) +
            assign<&ast::SwitchProng::value>(ref<SimpleAssignExpr>))> {};

  static constexpr auto SwitchExprBody =
      kw<"switch"> + lit<"("> + assign<&ast::SwitchExpr::value>(ref<Expr>) +
      lit<")"> + lit<"{"> +
      many(append<&ast::SwitchExpr::prongs>(ref<SwitchProngRule>) + lit<",">) +
      option(append<&ast::SwitchExpr::prongs>(ref<SwitchProngRule>)) + lit<"}">;
  struct SwitchExprRule : rule<SwitchExprBody> {};

  // --- blocks and statements -------------------------------------------

  static constexpr auto BlockBody =
      lit<"{"> + many(append<&ast::Block::statements>(ref<Statement>)) +
      lit<"}">;
  struct Block : rule<BlockBody> {};

  struct LabeledBlock
      : rule<option(assign<&ast::Block::label>(ref<ID>) + lit<":">) +
             BlockBody> {};

  static constexpr auto ForBlockTail =
      option(kw<"else"> + assign<&ast::Loop::elseBranch>(ref<Statement>));
  static constexpr auto ForAssignTail =
      lit<";"> | (kw<"else"> + assign<&ast::Loop::elseBranch>(ref<Statement>));
  static constexpr auto WhileElsePayload =
      option(lit<"|"> + assign<&ast::Loop::elsePayload>(ref<ID>) +
             lit<"|">);
  static constexpr auto WhileBlockTail =
      option(kw<"else"> + WhileElsePayload +
             assign<&ast::Loop::elseBranch>(ref<Statement>));
  static constexpr auto WhileAssignTail =
      lit<";"> | (kw<"else"> + WhileElsePayload +
                  assign<&ast::Loop::elseBranch>(ref<Statement>));

  static constexpr auto LoopStmtBody =
      option(flag<&ast::Loop::inlineLoop>(kw<"inline">)) +
      ((assign<&ast::Loop::forPrefix>(ref<ForPrefixRule>) +
        ((assign<&ast::Loop::thenBranch>(ref<LabeledBlock>) + ForBlockTail)  //
         |
         (assign<&ast::Loop::thenBranch>(ref<AssignExpr>) + ForAssignTail)))  //
       | (assign<&ast::Loop::whilePrefix>(ref<WhilePrefixRule>) +
          ((assign<&ast::Loop::thenBranch>(ref<LabeledBlock>) +
            WhileBlockTail)  //
           | (assign<&ast::Loop::thenBranch>(ref<AssignExpr>) +
              WhileAssignTail))));

  struct LabeledLoopStmt
      : rule<option(assign<&ast::Loop::label>(ref<ID>) + lit<":">) +
             LoopStmtBody> {};

  struct LabeledSwitch
      : rule<option(assign<&ast::SwitchExpr::label>(ref<ID>) + lit<":">) +
             SwitchExprBody> {};

  struct LabeledStatement
      : rule<ref<LabeledBlock> | ref<LabeledLoopStmt> | ref<LabeledSwitch>> {
  };

  struct IfStmt
      : rule<create<ast::IfExpr>(
            assign<&ast::IfExpr::prefix>(ref<IfPrefixRule>) +
            ((assign<&ast::IfExpr::thenBranch>(ref<LabeledBlock>) +
              option(kw<"else"> +
                     option(lit<"|"> +
                            assign<&ast::IfExpr::elsePayload>(ref<ID>) +
                            lit<"|">) +
                     assign<&ast::IfExpr::elseBranch>(ref<Statement>))) |
             (assign<&ast::IfExpr::thenBranch>(ref<AssignExpr>) +
              (lit<";"> |
               (kw<"else"> +
                option(lit<"|"> +
                       assign<&ast::IfExpr::elsePayload>(ref<ID>) +
                       lit<"|">) +
                assign<&ast::IfExpr::elseBranch>(ref<Statement>))))))> {};

  struct ComptimeStmtRule
      : rule<create<ast::ComptimeStmt>(
            kw<"comptime"> +
            (assign<&ast::ComptimeStmt::body>(ref<LabeledBlock>) |
             assign<&ast::ComptimeStmt::body>(ref<VarDeclExprStmtRule>)))> {};

  struct BlockExprStatement
      : rule<ref<LabeledBlock> | (ref<AssignExpr> + lit<";">)> {};

  struct VarDeclExprStmtRule
      : rule<create<ast::VarDeclExprStmt>(
            (append<&ast::VarDeclExprStmt::lhs>(ref<VarDeclProto>) +
             many(lit<","> +
                  append<&ast::VarDeclExprStmt::lhs>(ref<VarDeclProto> |
                                                     ref<Expr>)) +
             assign<&ast::VarDeclExprStmt::op>(Assign) +
             assign<&ast::VarDeclExprStmt::value>(ref<Expr>)) |
            (append<&ast::VarDeclExprStmt::lhs>(ref<Expr>) +
             option((assign<&ast::VarDeclExprStmt::op>(AssignOp) +
                     assign<&ast::VarDeclExprStmt::value>(ref<Expr>)) |
                    (some(lit<","> + append<&ast::VarDeclExprStmt::lhs>(
                                          ref<VarDeclProto> | ref<Expr>)) +
                     assign<&ast::VarDeclExprStmt::op>(Assign) +
                     assign<&ast::VarDeclExprStmt::value>(ref<Expr>)))))  //
             + lit<";">> {};

  struct Statement
      : rule<create<ast::NosuspendStmt>(
                 kw<"nosuspend"> +
                 assign<&ast::NosuspendStmt::body>(ref<BlockExprStatement>))
             |
             create<ast::SuspendStmt>(
                 kw<"suspend"> +
                 assign<&ast::SuspendStmt::body>(ref<BlockExprStatement>))  //
             | create<ast::DeferStmt>(
                   kw<"defer"> +
                   assign<&ast::DeferStmt::body>(ref<BlockExprStatement>))  //
             | create<ast::ErrdeferStmt>(
                   kw<"errdefer"> +
                   option(lit<"|"> +
                          assign<&ast::ErrdeferStmt::payload>(ref<ID>) +
                          lit<"|">) +
                   assign<&ast::ErrdeferStmt::body>(ref<BlockExprStatement>))  //
             | ref<ComptimeStmtRule>                                          //
             | ref<IfStmt>                                                    //
             | ref<LabeledStatement>                                          //
             // The spec's choice is ordered: a statement opening on one of
             // these words is that statement form, never an expression.
             | (!(kw<"comptime"> | kw<"nosuspend"> | kw<"if"> | kw<"while"> |
                  kw<"for"> | kw<"inline"> | kw<"switch"> | lit<"{">) +
                ref<VarDeclExprStmtRule>)> {};

  // --- expression ladder -------------------------------------------------

  struct PrefixExpr
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(PrefixOp) +
                 assign<&ast::UnaryExpr::operand>(ref<PrefixExpr>))  //
             | ref<PrimaryExpr>> {};

  struct BitwiseChain
      : infix<PrefixExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(MultiplyOp) >> left(AdditionOp) >> left(BitShiftOp) >>
               left(BitwiseOp))> {};

  struct CompareExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<BitwiseChain>) +
                 assign<&ast::BinaryExpr::op>(CompareOp) +
                 assign<&ast::BinaryExpr::right>(ref<BitwiseChain>))  //
             | ref<BitwiseChain>> {};

  struct Expr
      : infix<CompareExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(kw<"and">) >> left(kw<"or">))> {};

  struct AssignExpr
      : rule<create<ast::AssignExpr>(
                 append<&ast::AssignExpr::lhs>(ref<Expr>) +
                 ((assign<&ast::AssignExpr::op>(AssignOp) +
                   assign<&ast::AssignExpr::value>(ref<Expr>))  //
                  | (some(lit<","> +
                          append<&ast::AssignExpr::lhs>(ref<Expr>)) +
                     assign<&ast::AssignExpr::op>(Assign) +
                     assign<&ast::AssignExpr::value>(ref<Expr>))))  //
             | ref<Expr>> {};

  struct SimpleAssignExpr
      : rule<create<ast::AssignExpr>(
                 append<&ast::AssignExpr::lhs>(ref<Expr>) +
                 assign<&ast::AssignExpr::op>(AssignOp) +
                 assign<&ast::AssignExpr::value>(ref<Expr>))  //
             | ref<Expr>> {};

  // --- primary_expr: control flow and blocks ------------------------

  struct AsmOutputItemRule
      : rule<create<ast::AsmOutputItem>(
            lit<"["> + assign<&ast::AsmOutputItem::symbol>(ref<ID>) +
            lit<"]"> +
            assign<&ast::AsmOutputItem::constraint>(ref<STRINGLITERAL>) +
            lit<"("> +
            ((lit<"->"> +
              assign<&ast::AsmOutputItem::type>(ref<TypeExpr>))  //
             | assign<&ast::AsmOutputItem::name>(ref<ID>)) +
            lit<")">)> {};

  struct AsmInputItemRule
      : rule<create<ast::AsmInputItem>(
            lit<"["> + assign<&ast::AsmInputItem::symbol>(ref<ID>) +
            lit<"]"> +
            assign<&ast::AsmInputItem::constraint>(ref<STRINGLITERAL>) +
            lit<"("> + assign<&ast::AsmInputItem::value>(ref<Expr>) +
            lit<")">)> {};

  struct AsmExpr
      : rule<create<ast::AsmExpr>(
            kw<"asm"> +
            option(flag<&ast::AsmExpr::isVolatile>(kw<"volatile">)) +
            lit<"("> + assign<&ast::AsmExpr::code>(ref<Expr>) +
            option(lit<":"> +
                   many(append<&ast::AsmExpr::outputs>(
                            ref<AsmOutputItemRule>) +
                        lit<",">) +
                   option(append<&ast::AsmExpr::outputs>(
                       ref<AsmOutputItemRule>)) +
                   option(lit<":"> +
                          many(append<&ast::AsmExpr::inputs>(
                                   ref<AsmInputItemRule>) +
                               lit<",">) +
                          option(append<&ast::AsmExpr::inputs>(
                              ref<AsmInputItemRule>)) +
                          option(lit<":"> +
                                 assign<&ast::AsmExpr::clobbers>(
                                     ref<Expr>)))) +
            lit<")">)> {};

  struct IfExprRule
      : rule<create<ast::IfExpr>(
            assign<&ast::IfExpr::prefix>(ref<IfPrefixRule>) +
            assign<&ast::IfExpr::thenBranch>(ref<Expr>) +
            option(kw<"else"> +
                   option(lit<"|"> +
                          assign<&ast::IfExpr::elsePayload>(ref<ID>) +
                          lit<"|">) +
                   assign<&ast::IfExpr::elseBranch>(ref<Expr>)))> {};

  static constexpr auto ForExprBody = create<ast::Loop>(
      assign<&ast::Loop::forPrefix>(ref<ForPrefixRule>) +
      assign<&ast::Loop::thenBranch>(ref<Expr>) +
      option(kw<"else"> + assign<&ast::Loop::elseBranch>(ref<Expr>)));

  static constexpr auto WhileExprBody = create<ast::Loop>(
      assign<&ast::Loop::whilePrefix>(ref<WhilePrefixRule>) +
      assign<&ast::Loop::thenBranch>(ref<Expr>) +
      option(kw<"else"> +
             option(lit<"|"> + assign<&ast::Loop::elsePayload>(ref<ID>) +
                    lit<"|">) +
             assign<&ast::Loop::elseBranch>(ref<Expr>)));

  static constexpr auto LoopExprBody =
      option(flag<&ast::Loop::inlineLoop>(kw<"inline">)) +
      (ForExprBody | WhileExprBody);

  struct LabeledLoopExpr
      : rule<option(assign<&ast::Loop::label>(ref<ID>) + lit<":">) +
             LoopExprBody> {};

  struct FieldInit
      : rule<create<ast::FieldInit>(
            lit<"."> + assign<&ast::FieldInit::name>(ref<ID>) + Assign +
            assign<&ast::FieldInit::value>(ref<Expr>))> {};

  struct InitList
      : rule<lit<"{"> +
             option((append<&ast::InitList::fields>(ref<FieldInit>) +
                     many(lit<","> + append<&ast::InitList::fields>(
                                          ref<FieldInit>)) +
                     option(lit<",">))  //
                    | (append<&ast::InitList::values>(ref<Expr>) +
                       many(lit<","> +
                            append<&ast::InitList::values>(ref<Expr>)) +
                       option(lit<",">))) +
             lit<"}">> {};

  struct CurlySuffixExpr
      : rule<create<ast::TypedInit>(
                 assign<&ast::TypedInit::type>(ref<PrefixTypeExpr>) +
                 assign<&ast::TypedInit::list>(ref<InitList>))  //
             | ref<PrefixTypeExpr>> {};

  struct PrimaryExpr
      : rule<ref<AsmExpr>  //
             | ref<IfExprRule>  //
             | create<ast::BreakExpr>(
                   kw<"break"> +
                   option(lit<":"> +
                          assign<&ast::BreakExpr::label>(ref<ID>)) +
                   option(assign<&ast::BreakExpr::value>(ref<Expr>)))  //
             | create<ast::ComptimeExpr>(
                   kw<"comptime"> +
                   assign<&ast::ComptimeExpr::value>(ref<Expr>))  //
             | create<ast::NosuspendExpr>(
                   kw<"nosuspend"> +
                   assign<&ast::NosuspendExpr::value>(ref<Expr>))  //
             | create<ast::ContinueExpr>(
                   kw<"continue"> +
                   option(lit<":"> +
                          assign<&ast::ContinueExpr::label>(ref<ID>)) +
                   option(assign<&ast::ContinueExpr::value>(ref<Expr>)))  //
             | create<ast::ResumeExpr>(
                   kw<"resume"> +
                   assign<&ast::ResumeExpr::value>(ref<Expr>))  //
             | create<ast::ReturnExpr>(
                   kw<"return"> +
                   option(assign<&ast::ReturnExpr::value>(ref<Expr>)))  //
             | ref<LabeledLoopExpr>                                     //
             | ref<Block>                                               //
             | ref<CurlySuffixExpr>> {};

  // --- type_expr chain (also every value's ultimate primary) -----------

  struct IdentifierListItemRule
      : rule<create<ast::IdentifierListItem>(
            option(assign<&ast::IdentifierListItem::docComment>(
                ref<DocComments>)) +
            assign<&ast::IdentifierListItem::name>(ref<ID>))> {};

  struct ErrorSetDecl
      : rule<create<ast::ErrorSetDecl>(
            kw<"error"> + lit<"{"> +
            many(append<&ast::ErrorSetDecl::errors>(
                     ref<IdentifierListItemRule>) +
                 lit<",">) +
            option(append<&ast::ErrorSetDecl::errors>(
                ref<IdentifierListItemRule>)) +
            lit<"}">)> {};

  struct NamedContainerMembers;
  static constexpr auto StructKind =
      (assign<&ast::ContainerDecl::kind>(kw<"struct">) +
       option(lit<"("> + assign<&ast::ContainerDecl::arg>(ref<Expr>) + lit<")">)) |
      assign<&ast::ContainerDecl::kind>(kw<"opaque">);
  static constexpr auto NamedKind =
      (assign<&ast::ContainerDecl::kind>(kw<"enum">) +
       option(lit<"("> + assign<&ast::ContainerDecl::arg>(ref<Expr>) + lit<")">)) |
      (assign<&ast::ContainerDecl::kind>(kw<"union">) +
       option(lit<"("> +
              ((flag<&ast::ContainerDecl::unionEnumTag>(kw<"enum">) +
                option(lit<"("> +
                       assign<&ast::ContainerDecl::unionEnumArg>(ref<Expr>) +
                       lit<")">)) |
               assign<&ast::ContainerDecl::arg>(ref<Expr>)) + lit<")">));
  struct ContainerDecl
      : rule<create<ast::ContainerDecl>(
            option(assign<&ast::ContainerDecl::extPacked>(
                kw<"extern"> | kw<"packed">)) +
            ((StructKind + lit<"{"> +
              assign<&ast::ContainerDecl::body>(ref<ContainerMembers>) + lit<"}">) |
             (NamedKind + lit<"{"> +
              assign<&ast::ContainerDecl::body>(ref<NamedContainerMembers>) + lit<"}">)))> {};

  struct GroupedExprRule
      : rule<create<ast::GroupedExpr>(
            lit<"("> + assign<&ast::GroupedExpr::value>(ref<Expr>) +
            lit<")">)> {};

  struct LabeledTypeExpr
      : rule<(assign<&ast::Block::label>(ref<ID>) + lit<":"> + BlockBody)  //
             | (option(assign<&ast::SwitchExpr::label>(ref<ID>) + lit<":">) +
                SwitchExprBody)> {};

  static constexpr auto ForTypeExprBody = create<ast::Loop>(
      assign<&ast::Loop::forPrefix>(ref<ForPrefixRule>) +
      assign<&ast::Loop::thenBranch>(ref<TypeExpr>) +
      option(kw<"else"> + assign<&ast::Loop::elseBranch>(ref<TypeExpr>)));

  static constexpr auto WhileTypeExprBody = create<ast::Loop>(
      assign<&ast::Loop::whilePrefix>(ref<WhilePrefixRule>) +
      assign<&ast::Loop::thenBranch>(ref<TypeExpr>) +
      option(kw<"else"> +
             option(lit<"|"> + assign<&ast::Loop::elsePayload>(ref<ID>) +
                    lit<"|">) +
             assign<&ast::Loop::elseBranch>(ref<TypeExpr>)));

  static constexpr auto LoopTypeExprBody =
      option(flag<&ast::Loop::inlineLoop>(kw<"inline">)) +
      (ForTypeExprBody | WhileTypeExprBody);

  struct LabeledLoopTypeExpr
      : rule<option(assign<&ast::Loop::label>(ref<ID>) + lit<":">) +
             LoopTypeExprBody> {};

  struct IfTypeExpr
      : rule<create<ast::IfExpr>(
            assign<&ast::IfExpr::prefix>(ref<IfPrefixRule>) +
            assign<&ast::IfExpr::thenBranch>(ref<TypeExpr>) +
            option(kw<"else"> +
                   option(lit<"|"> +
                          assign<&ast::IfExpr::elsePayload>(ref<ID>) +
                          lit<"|">) +
                   assign<&ast::IfExpr::elseBranch>(ref<TypeExpr>)))> {};

  struct FnProto
      : rule<create<ast::FnProto>(
            kw<"fn"> + option(assign<&ast::FnProto::name>(ref<ID>)) +
            lit<"("> +
            many(append<&ast::FnProto::params>(ref<ParamDeclRule>) +
                 lit<",">) +
            option(append<&ast::FnProto::params>(ref<ParamDeclRule>)) +
            lit<")"> +
            option(kw<"align"> + lit<"("> +
                   assign<&ast::FnProto::alignExpr>(ref<Expr>) + lit<")">) +
            option(kw<"addrspace"> + lit<"("> +
                   assign<&ast::FnProto::addrSpaceExpr>(ref<Expr>) +
                   lit<")">) +
            option(kw<"linksection"> + lit<"("> +
                   assign<&ast::FnProto::linkSectionExpr>(ref<Expr>) +
                   lit<")">) +
            option(kw<"callconv"> + lit<"("> +
                   assign<&ast::FnProto::callConvExpr>(ref<Expr>) +
                   lit<")">) +
            option(flag<&ast::FnProto::errorUnion>(lit<"!">)) +
            assign<&ast::FnProto::returnType>(ref<TypeExpr>))> {};

  // The modifier is possessive, as the spec's `?` is: `comptime u8` is a
  // comptime parameter, never a comptime type.
  struct ParamDeclRule
      : rule<create<ast::ParamDecl>(
                 option(assign<&ast::ParamDecl::docComment>(
                     ref<DocComments>)) +
                 option(flag<&ast::ParamDecl::noalias>(
                              kw<"noalias">) |
                          flag<&ast::ParamDecl::comptime>(
                              kw<"comptime">)) +
                 option(assign<&ast::ParamDecl::name>(ref<ID>) + lit<":">) +
                 (flag<&ast::ParamDecl::anytype>(kw<"anytype">)  //
                  | assign<&ast::ParamDecl::type>(ref<TypeExpr>)))  //
             | create<ast::ParamDecl>(
                   flag<&ast::ParamDecl::variadic>(lit<"...">))> {};

  struct PrimaryTypeExpr
      : rule<create<ast::BuiltinCall>(
                 assign<&ast::BuiltinCall::name>(ref<BUILTINIDENT>) +
                 lit<"("> +
                 many(append<&ast::BuiltinCall::args>(ref<Expr>) +
                      lit<",">) +
                 option(append<&ast::BuiltinCall::args>(ref<Expr>)) +
                 lit<")">)  //
             | create<ast::CharLiteral>(
                   assign<&ast::CharLiteral::text>(ref<CHARLITERAL>))  //
             | ref<ContainerDecl>                                     //
             | create<ast::EnumLiteral>(
                   lit<"."> + assign<&ast::EnumLiteral::name>(ref<ID>))  //
             | (lit<"."> + ref<InitList>)                               //
             | ref<ErrorSetDecl>                                        //
             | ref<FnProto>                                         //
             | ref<GroupedExprRule>                                 //
             | ref<LabeledTypeExpr>                                 //
             | create<ast::Identifier>(
                   assign<&ast::Identifier::name>(ref<ID>))  //
             | create<ast::NumberLiteral>(
                   assign<&ast::NumberLiteral::text>(ref<NUMBER>))  //
             | create<ast::ErrorValue>(
                   kw<"error"> + lit<"."> +
                   assign<&ast::ErrorValue::name>(ref<ID>))  //
             | create<ast::AnyframeLit>(kw<"anyframe">)  //
             | create<ast::UnreachableLit>(kw<"unreachable">)  //
             | create<ast::StringLiteral>(
                   assign<&ast::StringLiteral::text>(
                       ref<STRINGLITERAL>))> {};

  // A suffix binds to the operand before it: the expression never ends
  // before one (`..` and `...` still end it), which would hand the suffix to
  // a function type the operand ends.
  struct SuffixExpr
      : rule<ref<PrimaryTypeExpr> +
             many(nest<ast::SliceExpr, &ast::SliceExpr::object>(
                      lit<"["> +
                      assign<&ast::SliceExpr::start>(ref<Expr>) +
                      lit<".."> +
                      option(assign<&ast::SliceExpr::end>(ref<Expr>)) +
                      option(lit<":"> +
                             assign<&ast::SliceExpr::sentinel>(ref<Expr>)) +
                      lit<"]">)  //
                  | nest<ast::IndexExpr, &ast::IndexExpr::object>(
                        lit<"["> +
                        assign<&ast::IndexExpr::index>(ref<Expr>) +
                        lit<"]">)  //
                  | nest<ast::MemberExpr, &ast::MemberExpr::object>(
                        lit<"."> +
                        assign<&ast::MemberExpr::field>(ref<ID>))  //
                  | nest<ast::DerefExpr, &ast::DerefExpr::object>(
                        lit<".*">)  //
                  | nest<ast::OptionalUnwrapExpr,
                         &ast::OptionalUnwrapExpr::object>(lit<".?">)  //
                  | nest<ast::CallExpr, &ast::CallExpr::callee>(
                        lit<"("> +
                        many(append<&ast::CallExpr::args>(ref<Expr>) +
                             lit<",">) +
                        option(append<&ast::CallExpr::args>(ref<Expr>)) +
                        lit<")">)) +
             !(cr<"(["> | lit<".", !cr<".">>)> {};

  struct ErrorUnionExpr
      : rule<create<ast::ErrorUnionExpr>(
                 assign<&ast::ErrorUnionExpr::errorSet>(ref<SuffixExpr>) +
                 lit<"!"> +
                 assign<&ast::ErrorUnionExpr::payload>(ref<TypeExpr>))  //
             | ref<SuffixExpr>> {};

  // Shared by slice and many-ptr type modifiers: plain align, no bit range
  // (only a pointer type's align takes a bit range).
  struct TypeModifierRule
      : rule<create<ast::TypeModifier>(
            assign<&ast::TypeModifier::kind>(kw<"const"> | kw<"volatile"> |
                                             kw<"allowzero">))  //
            | create<ast::TypeModifier>(
                  assign<&ast::TypeModifier::kind>(kw<"addrspace">) +
                  lit<"("> + assign<&ast::TypeModifier::arg>(ref<Expr>) +
                  lit<")">)  //
            | create<ast::TypeModifier>(
                  assign<&ast::TypeModifier::kind>(kw<"align">) + lit<"("> +
                  assign<&ast::TypeModifier::arg>(ref<Expr>) +
                  lit<")">)> {};

  struct PtrTypeModifierRule
      : rule<ref<TypeModifierRule>  //
             | create<ast::TypeModifier>(
                   assign<&ast::TypeModifier::kind>(kw<"align">) + lit<"("> +
                   assign<&ast::TypeModifier::arg>(ref<Expr>) + lit<":"> +
                   assign<&ast::TypeModifier::bitStart>(ref<Expr>) +
                   lit<":"> +
                   assign<&ast::TypeModifier::bitEnd>(ref<Expr>) +
                   lit<")">)> {};

  struct PrefixTypeExpr
      : rule<create<ast::OptionalType>(
                 lit<"?"> +
                 assign<&ast::OptionalType::base>(ref<TypeExpr>))  //
             | create<ast::AnyframeType>(
                   kw<"anyframe"> + lit<"->"> +
                   assign<&ast::AnyframeType::base>(ref<TypeExpr>))  //
             | create<ast::ManyPtrType>(
                   lit<"["> + lit<"*"> +
                   option(flag<&ast::ManyPtrType::sentinelC>(
                              lit<"c">)  //
                          | (lit<":"> +
                             assign<&ast::ManyPtrType::sentinel>(
                                 ref<Expr>))) +
                   lit<"]"> +
                   many(append<&ast::ManyPtrType::modifiers>(
                       ref<TypeModifierRule>)) +
                   assign<&ast::ManyPtrType::base>(ref<TypeExpr>))  //
             | create<ast::SliceType>(
                   lit<"["> +
                   option(lit<":"> +
                          assign<&ast::SliceType::sentinel>(ref<Expr>)) +
                   lit<"]"> +
                   many(append<&ast::SliceType::modifiers>(
                       ref<TypeModifierRule>)) +
                   assign<&ast::SliceType::base>(ref<TypeExpr>))  //
             | create<ast::ArrayType>(
                   lit<"["> + assign<&ast::ArrayType::size>(ref<Expr>) +
                   option(lit<":"> +
                          assign<&ast::ArrayType::sentinel>(ref<Expr>)) +
                   lit<"]"> +
                   assign<&ast::ArrayType::base>(ref<TypeExpr>))  //
             | create<ast::PtrType>(
                   lit<"*"> +
                   many(append<&ast::PtrType::modifiers>(
                       ref<PtrTypeModifierRule>)) +
                   assign<&ast::PtrType::base>(ref<TypeExpr>))  //
             | ref<ErrorUnionExpr>> {};

  // The forms whose last operand is a type an expression form would read
  // further: an expression reaches them only as that expression form.
  struct TypeExpr
      : rule<ref<PrefixTypeExpr> | ref<IfTypeExpr> | ref<LabeledLoopTypeExpr> |
             create<ast::ComptimeExpr>(
                 kw<"comptime"> +
                 assign<&ast::ComptimeExpr::value>(ref<TypeExpr>))> {};

  // --- container level ---------------------------------------------------

  struct TestDeclRule
      : rule<create<ast::TestDecl>(
            kw<"test"> +
            option(assign<&ast::TestDecl::name>(ref<STRINGLITERAL> |
                                                ref<ID>)) +
            assign<&ast::TestDecl::body>(ref<Block>))> {};

  struct ComptimeDeclRule
      : rule<create<ast::ComptimeDecl>(
            kw<"comptime"> +
            assign<&ast::ComptimeDecl::body>(ref<Block>))> {};

  struct VarDeclProto
      : rule<(flag<&ast::VarDeclProto::isConst>(kw<"const">) |
              kw<"var">) +
            assign<&ast::VarDeclProto::name>(ref<ID>) +
            option(lit<":"> +
                   assign<&ast::VarDeclProto::type>(ref<TypeExpr>)) +
            option(kw<"align"> + lit<"("> +
                   assign<&ast::VarDeclProto::alignExpr>(ref<Expr>) +
                   lit<")">) +
            option(kw<"addrspace"> + lit<"("> +
                   assign<&ast::VarDeclProto::addrSpaceExpr>(ref<Expr>) +
                   lit<")">) +
            option(kw<"linksection"> + lit<"("> +
                   assign<&ast::VarDeclProto::linkSectionExpr>(ref<Expr>) +
                   lit<")">)> {};

  struct GlobalVarDeclRule
      : rule<create<ast::GlobalVarDecl>(
            assign<&ast::GlobalVarDecl::proto>(ref<VarDeclProto>) +
            option(Assign +
                   assign<&ast::GlobalVarDecl::value>(ref<Expr>)) +
            lit<";">)> {};

  static constexpr auto DeclBody =
      create<ast::Decl>(
          option(assign<&ast::Decl::modifier>(kw<"export">)  //
                 |
                 (assign<&ast::Decl::modifier>(kw<"extern">) +
                  option(assign<&ast::Decl::externLib>(ref<STRINGLITERAL>)))  //
                 | assign<&ast::Decl::modifier>(kw<"inline">)                 //
                 | assign<&ast::Decl::modifier>(kw<"noinline">)) +
          assign<&ast::Decl::proto>(ref<FnProto>) +
          (lit<";">                                  //
           | assign<&ast::Decl::body>(ref<Block>)))  //
      |
      create<ast::Decl>(
          option(assign<&ast::Decl::modifier>(kw<"export">)  //
                 |
                 (assign<&ast::Decl::modifier>(kw<"extern">) +
                  option(assign<&ast::Decl::externLib>(ref<STRINGLITERAL>)))) +
          option(flag<&ast::Decl::threadlocal>(kw<"threadlocal">)) +
          assign<&ast::Decl::varDecl>(ref<GlobalVarDeclRule>));

  struct ContainerDeclaration
      : rule<ref<TestDeclRule>     //
             | ref<ComptimeDeclRule>  //
             | (option(assign<&ast::Decl::docComment>(ref<DocComments>)) +
                option(flag<&ast::Decl::pub>(kw<"pub">)) +
                DeclBody)> {};

  struct ContainerFieldRule
      : rule<create<ast::ContainerField>(
            option(assign<&ast::ContainerField::docComment>(
                ref<DocComments>)) +
            option(flag<&ast::ContainerField::comptime>(
                kw<"comptime">)) +
            option(assign<&ast::ContainerField::name>(ref<ID>) + lit<":">) +
            assign<&ast::ContainerField::type>(ref<TypeExpr>) +
            option(kw<"align"> + lit<"("> +
                   assign<&ast::ContainerField::alignExpr>(ref<Expr>) +
                   lit<")">) +
            option(Assign +
                   assign<&ast::ContainerField::value>(ref<Expr>)))> {};

  struct NamedContainerFieldRule
      : rule<create<ast::ContainerField>(
            option(assign<&ast::ContainerField::docComment>(ref<DocComments>)) +
            option(flag<&ast::ContainerField::comptime>(kw<"comptime">)) +
            assign<&ast::ContainerField::name>(ref<ID>) +
            option(lit<":"> + assign<&ast::ContainerField::type>(ref<TypeExpr>)) +
            option(kw<"align"> + lit<"("> +
                   assign<&ast::ContainerField::alignExpr>(ref<Expr>) + lit<")">) +
            option(Assign + assign<&ast::ContainerField::value>(ref<Expr>)))> {};

  template <class Field>
  struct Members
      : rule<create<ast::ContainerMembers>(
            option(assign<&ast::ContainerMembers::docComment>(
                ref<ContainerDocComments>)) +
            many(append<&ast::ContainerMembers::members>(ref<ContainerDeclaration>) |
                 (append<&ast::ContainerMembers::members>(ref<Field>) + lit<",">)) +
            option(append<&ast::ContainerMembers::members>(ref<Field>)))> {};

  struct ContainerMembers : Members<ContainerFieldRule> {};
  struct NamedContainerMembers : Members<NamedContainerFieldRule> {};

  using entry = ContainerMembers;
  using ignore = WS;
  using hidden = list<LineComment>;
};

}  // namespace zig
