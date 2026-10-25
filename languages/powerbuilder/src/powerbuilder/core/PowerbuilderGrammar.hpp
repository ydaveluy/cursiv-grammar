// The powerbuilder language: its grammar. PowerScript source exports read as
// a flat sequence of declarations and statements; `&` before a line break and
// comments are the only trivia beyond plain whitespace -- there is no
// newline-sensitive construct to spell with `gap::`.
#pragma once

#include <charconv>
#include <cstddef>
#include <memory_resource>
#include <span>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "powerbuilder/core/ast.hpp"

namespace powerbuilder {

inline constexpr auto parse_number = [](std::string_view t) -> double {
  if (!t.empty()) {
    const char c = t.back();
    if (c == 'D' || c == 'd' || c == 'F' || c == 'f') t.remove_suffix(1);
  }
  double v = 0.0;
  std::from_chars(t.data(), t.data() + t.size(), v);
  return v;
};
using NumberValue = cursiv::text_conv<parse_number>;

// `~~` doubles a literal tilde, `~"` escapes an embedded quote.
inline constexpr auto unescape_dquoted = [](std::string_view t,
                                            std::pmr::memory_resource* mem) {
  const std::string_view body = t.substr(1, t.size() - 2);
  if (body.find('~') == std::string_view::npos) return body;
  const std::span<char> room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    if (body[i] == '~' && i + 1 < body.size() &&
        (body[i + 1] == '~' || body[i + 1] == '"')) {
      room[n++] = body[i + 1];
      ++i;
    } else {
      room[n++] = body[i];
    }
  }
  return std::string_view(room.data(), n);
};
using DQuotedValue = cursiv::text_conv<unescape_dquoted>;

// `~'` escapes an embedded quote in a single-quoted string.
inline constexpr auto unescape_quoted = [](std::string_view t,
                                           std::pmr::memory_resource* mem) {
  const std::string_view body = t.substr(1, t.size() - 2);
  if (body.find('~') == std::string_view::npos) return body;
  const std::span<char> room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    if (body[i] == '~' && i + 1 < body.size() && body[i + 1] == '\'') {
      room[n++] = '\'';
      ++i;
    } else {
      room[n++] = body[i];
    }
  }
  return std::string_view(room.data(), n);
};
using QuotedValue = cursiv::text_conv<unescape_quoted>;

struct Powerbuilder : cursiv::GrammarSpec {
  // Identifiers spell `-$#%_` past letters and digits (ID_PARTS), so a
  // keyword's boundary must cover the same set or e.g. `ikw<"IF">` would
  // wrongly close before `-` inside a hyphenated name.
  static constexpr auto word_boundary = cr<"A-Za-z0-9$#%_-">;

  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct LineContinuation : terminal<lit<"&"> + many(cr<R"( \t)">) + eol> {};
  struct SlComment : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct MlComment : terminal<through(lit<"/*">, lit<"*/">)> {};
  // `HA$PBExportHeader$w.srw`, `$PBExportComments$...`: a directive line
  // introduced by `$`, kept as trivia like a comment.
  struct ExportHeader
      : terminal<option(cr<"Hh"> + cr<"Aa">) + lit<"$"> + many(cr<R"(^\r\n)">)> {
  };

  static constexpr auto IdCont = cr<"A-Za-z0-9$#%_-">;
  // ID_PARTS starts on a letter; a bare leading `_` (UNDERSCORE in the
  // reference) is folded in here rather than kept as its own token.
  struct ID : terminal<cr<"A-Za-z_"> + many(IdCont)> {};
  struct ENUM : terminal<cr<"A-Za-z_"> + many(IdCont) + lit<"!">> {};

  static constexpr auto NumCore =
      (some(d) + lit<"."> + some(d)) | (lit<"."> + some(d)) | some(d);
  struct NUMBER
      : terminal<NumCore + option(cr<"Ee"> + option(cr<"+-">) + some(d)) +
                     option(cr<"DFdf">),
                 NumberValue> {};

  struct DATE : terminal<repeat<4>(d) + lit<"-"> + repeat<2>(d) + lit<"-"> +
                         repeat<2>(d)> {};
  struct TIME : terminal<repeat<2>(d) + lit<":"> + repeat<2>(d) + lit<":"> +
                         repeat<2>(d) + option(lit<"."> + repeat<6>(d))> {};

  struct DQUOTED_STRING
      : terminal<lit<"\""> + many(lit<"~\""> | cr<R"(^")">) + lit<"\"">,
                 DQuotedValue> {};
  struct QUOTED_STRING
      : terminal<lit<"'"> + many(lit<"~'"> | cr<"^'">) + lit<"'">,
                 QuotedValue> {};

  static constexpr auto DataTypeKw =
      ikw<"ANY"> | ikw<"BLOB"> | ikw<"BOOLEAN"> | ikw<"BYTE"> |
      ikw<"CHARACTER"> | ikw<"CHAR"> | ikw<"DATETIME"> | ikw<"DATE"> |
      ikw<"DECIMAL"> | ikw<"DEC"> | ikw<"DOUBLE"> | ikw<"INTEGER"> |
      ikw<"INT"> | ikw<"LONGLONG"> | ikw<"LONG"> | ikw<"REAL"> |
      ikw<"STRING"> | ikw<"TIME"> | ikw<"UNSIGNEDINTEGER"> | ikw<"UINT"> |
      ikw<"UNSIGNEDLONG"> | ikw<"ULONG"> | ikw<"WINDOW">;

  // The soft-keyword set of identifier_name_ex: each spelling is the SAME
  // `ikw<>` used at that word's own keyword call sites, so a state that
  // reads both a name and the keyword resolves it, never two tokens for one
  // spelling.
  static constexpr auto IdentifierNameEx =
      ref<ID> | ikw<"SELECT"> | ikw<"TYPE"> | ikw<"UPDATE"> | ikw<"DELETE"> |
      ikw<"OPEN"> | ikw<"CLOSE"> | ikw<"GOTO"> | ikw<"INSERT"> |
      ikw<"TIME"> | ikw<"READONLY"> | ikw<"SQLCA"> | ikw<"CREATE"> |
      ikw<"VALUES"> | ikw<"WINDOW"> | ikw<"SYSTEM"> | ikw<"DATE">;

  static constexpr auto FunctionNameKw =
      IdentifierNameEx | ikw<"POST"> | DataTypeKw;

  static constexpr auto AccessTypeKw =
      ikw<"PUBLIC"> | ikw<"PRIVATE"> | ikw<"PROTECTED">;
  static constexpr auto AccessModifPartKw =
      ikw<"PRIVATEWRITE"> | ikw<"PRIVATEREAD"> | ikw<"PRIVATE"> |
      ikw<"PROTECTEDREAD"> | ikw<"PROTECTEDWRITE"> | ikw<"PROTECTED"> |
      ikw<"PUBLIC">;
  static constexpr auto ScopeModifKw = ikw<"GLOBAL"> | ikw<"LOCAL">;

  // ---- operators (longer spellings declared first: `<>`/`<=`/`>=` before
  // `<`/`>`, so the shorter reading never wins a shared prefix) ----
  struct NE_OP : terminal<lit<"<>">, value<ast::Operator::ne>> {};
  struct LE_OP : terminal<lit<"<=">, value<ast::Operator::le>> {};
  struct GE_OP : terminal<lit<">=">, value<ast::Operator::ge>> {};
  struct EQ_OP : terminal<lit<"=">, value<ast::Operator::eq>> {};
  struct LT_OP : terminal<lit<"<">, value<ast::Operator::lt>> {};
  struct GT_OP : terminal<lit<">">, value<ast::Operator::gt>> {};
  struct ADD_OP : terminal<lit<"+">, value<ast::Operator::add>> {};
  struct SUB_OP : terminal<lit<"-">, value<ast::Operator::subtract>> {};
  struct MUL_OP : terminal<lit<"*">, value<ast::Operator::multiply>> {};
  struct DIV_OP : terminal<lit<"/">, value<ast::Operator::divide>> {};
  struct POW_OP : terminal<lit<"^">, value<ast::Operator::power>> {};
  struct AND_OP : terminal<ikw<"AND">, value<ast::Operator::logical_and>> {};
  struct OR_OP : terminal<ikw<"OR">, value<ast::Operator::logical_or>> {};

  static constexpr auto OpCompare =
      ref<NE_OP> | ref<LE_OP> | ref<GE_OP> | ref<EQ_OP> | ref<LT_OP> |
      ref<GT_OP>;
  static constexpr auto OpAdd = ref<ADD_OP> | ref<SUB_OP>;
  static constexpr auto OpMul = ref<MUL_OP> | ref<DIV_OP>;
  static constexpr auto OpPow = ref<POW_OP>;

  // ------------------------------------------------------- paths and names

  struct PathSegmentRule;
  struct PathRule;
  struct ArrayIndexRule;
  struct ArgumentRule;
  struct ExpressionRule;
  struct TypeSpecRule;

  struct ArrayIndexRule
      : rule<create<ast::ArrayIndex>(
            lit<"["> +
            option(some(append<&ast::ArrayIndex::items>(ref<ExpressionRule>),
                       lit<",">)) +
            lit<"]">)> {};

  struct PathSegmentRule
      : rule<assign<&ast::PathSegment::name>(IdentifierNameEx) +
             option(assign<&ast::PathSegment::index>(ref<ArrayIndexRule>))> {
  };

  struct PathRule
      : rule<create<ast::Path>(
            some(append<&ast::Path::segments>(ref<PathSegmentRule>),
                lit<".">))> {};

  struct BindParamRule
      : rule<create<ast::BindParam>(
            lit<":"> + assign<&ast::BindParam::name>(ref<PathRule>))> {};

  // -------------------------------------------------------- literal atoms

  struct EnumLitRule
      : rule<create<ast::EnumLit>(assign<&ast::EnumLit::value>(ref<ENUM>))> {
  };
  struct NumberLitRule
      : rule<create<ast::NumberLit>(
            assign<&ast::NumberLit::value>(ref<NUMBER>))> {};
  struct BoolLitRule
      : rule<create<ast::BoolLit>(
            flag<&ast::BoolLit::value>(ikw<"TRUE">) | ikw<"FALSE">)> {};
  struct DateLitRule
      : rule<create<ast::DateLit>(assign<&ast::DateLit::value>(ref<DATE>))> {
  };
  struct TimeLitRule
      : rule<create<ast::TimeLit>(assign<&ast::TimeLit::value>(ref<TIME>))> {
  };
  struct StringLitRule
      : rule<create<ast::StringLit>(assign<&ast::StringLit::value>(
            ref<DQUOTED_STRING> | ref<QUOTED_STRING>))> {};

  struct BraceListRule
      : rule<create<ast::BraceList>(
            lit<"{"> +
            some(append<&ast::BraceList::items>(ref<ExpressionRule>),
                lit<",">) +
            lit<"}">)> {};

  struct IsNullTestRule
      : rule<create<ast::IsNullTest>(
            assign<&ast::IsNullTest::operand>(ref<PathRule>) + ikw<"IS"> +
            option(flag<&ast::IsNullTest::negated>(ikw<"NOT">)) +
            ikw<"NULL">)> {};

  // `name = :param`: kept out of the general expression atoms (it would
  // clash with `=` comparison on the same span) and reachable only through
  // the SQL SET/WHERE list, `SqlSetItemRule`, below.
  struct BindAssignRule
      : rule<create<ast::BindAssign>(
            assign<&ast::BindAssign::target>(ref<PathRule>) + lit<"="> +
            assign<&ast::BindAssign::value>(ref<BindParamRule>))> {};

  struct SqlSetItemRule : rule<ref<IsNullTestRule> | ref<BindAssignRule>> {};

  struct WhereClauseRule
      : rule<create<ast::WhereClause>(
            ikw<"WHERE"> +
            (some(append<&ast::WhereClause::setValues>(ref<SqlSetItemRule>),
                 lit<",">) |
             assign<&ast::WhereClause::cond>(ref<ExpressionRule>)))> {};

  // ----------------------------------------------------------- call chain

  struct ArgumentRule
      : rule<create<ast::Argument>(
            option(flag<&ast::Argument::byRef>(ikw<"REF">)) +
            assign<&ast::Argument::value>(ref<ExpressionRule>))> {};

  // EVENT and TRIGGER/POST order differently across the shapes this unifies
  // (`FUNCTION? POST? DYNAMIC? EVENT?`, `(POST|TRIGGER) EVENT?`,
  // `(EVENT|FUNCTION)? (TRIGGER|POST)?`, `TRIGGER EVENT | DYNAMIC EVENT? |
  // EVENT TRIGGER? DYNAMIC`): both orderings are accepted here.
  static constexpr auto CallModifiers =
      option(flag<&ast::Call::isFunctionKw>(ikw<"FUNCTION">)) +
      option(flag<&ast::Call::isPost>(ikw<"POST">)) +
      option(flag<&ast::Call::isDynamic>(ikw<"DYNAMIC">)) +
      option((flag<&ast::Call::isEvent>(ikw<"EVENT">) +
              option(flag<&ast::Call::isTrigger>(ikw<"TRIGGER">))) |
             (flag<&ast::Call::isTrigger>(ikw<"TRIGGER">) +
              option(flag<&ast::Call::isEvent>(ikw<"EVENT">))));

  static constexpr auto CallTail =
      assign<&ast::Call::name>(FunctionNameKw) +
      flag<&ast::Call::hasParens>(lit<"(">) +
      option(some(append<&ast::Call::args>(ref<ArgumentRule>), lit<",">)) +
      lit<")">;

  struct CallHeadRule : rule<create<ast::Call>(CallModifiers + CallTail)> {};

  struct AncestorCallRule
      : rule<create<ast::Call>(
            flag<&ast::Call::ancestor>(lit<"::">) +
            option(flag<&ast::Call::isEvent>(ikw<"EVENT">)) +
            CallTail)> {};

  struct SuperCallRule
      : rule<create<ast::Call>(
            ikw<"SUPER"> + flag<&ast::Call::ancestor>(lit<"::">) +
            CallModifiers + CallTail)> {};

  struct SpecialCallRule
      : rule<create<ast::SpecialCall>(
                 assign<&ast::SpecialCall::keyword>(ikw<"OPEN">) +
                 flag<&ast::SpecialCall::hasParens>(lit<"(">) +
                 some(append<&ast::SpecialCall::args>(ref<ArgumentRule>),
                     lit<",">) +
                 lit<")">) |
             create<ast::SpecialCall>(
                 assign<&ast::SpecialCall::keyword>(ikw<"CLOSE">) +
                 flag<&ast::SpecialCall::hasParens>(lit<"(">) +
                 some(append<&ast::SpecialCall::args>(ref<ArgumentRule>),
                     lit<",">) +
                 lit<")">) |
             create<ast::SpecialCall>(
                 assign<&ast::SpecialCall::keyword>(ikw<"HALT">) +
                 option(flag<&ast::SpecialCall::haltClose>(ikw<"CLOSE">)))>{
  };

  struct PostfixPrimaryRule
      : rule<create<ast::Path>(append<&ast::Path::segments>(
                 ref<PathSegmentRule>)) |
             ref<CallHeadRule> | ref<AncestorCallRule> |
             ref<SuperCallRule>> {};

  struct PostfixChainRule
      : rule<ref<PostfixPrimaryRule> +
             many(nest<ast::MemberAccess, &ast::MemberAccess::object>(
                      lit<"."> +
                      assign<&ast::MemberAccess::field>(FunctionNameKw) +
                      option(assign<&ast::MemberAccess::index>(
                          ref<ArrayIndexRule>))) |
                  nest<ast::Call, &ast::Call::receiver>(
                      lit<"."> + CallModifiers + CallTail))> {};

  struct CreateExprRule
      : rule<create<ast::CreateExpr>(
            ikw<"CREATE">+
            ((ikw<"USING"> +
              assign<&ast::CreateExpr::usingExpr>(ref<ExpressionRule>)) |
             (option(ikw<"USING">) +
              option(assign<&ast::CreateExpr::qualifier>(ref<PathRule>) +
                     lit<".">) +
              assign<&ast::CreateExpr::type>(ref<TypeSpecRule>) +
              option(flag<&ast::CreateExpr::hasParens>(lit<"(">) +
                     option(some(
                         append<&ast::CreateExpr::args>(ref<ArgumentRule>),
                         lit<",">)) +
                     lit<")">))))> {};

  struct DescribeExprRule
      : rule<create<ast::DescribeExpr>(
                 option(assign<&ast::DescribeExpr::qualifier>(ref<PathRule>) +
                        lit<".">) +
                 ikw<"DESCRIBE"> + lit<"("> +
                 option(some(
                     append<&ast::DescribeExpr::args>(ref<ArgumentRule>),
                     lit<",">)) +
                 lit<")">) |
             create<ast::DescribeExpr>(
                 ikw<"DESCRIBE"> +
                 assign<&ast::DescribeExpr::target>(ref<PathRule>) +
                 ikw<"INTO"> +
                 assign<&ast::DescribeExpr::into>(ref<PathRule>))> {};

  // -------------------------------------------------------------- atoms

  struct AtomRule
      : rule<(lit<"("> + ref<ExpressionRule> + lit<")">) |
             ref<BraceListRule> | ref<IsNullTestRule> | ref<EnumLitRule> |
             ref<NumberLitRule> | ref<BoolLitRule> | ref<DateLitRule> |
             ref<TimeLitRule> | ref<StringLitRule> | ref<BindParamRule> |
             ref<CreateExprRule> | ref<DescribeExprRule> |
             ref<SpecialCallRule> | ref<PostfixChainRule>> {};

  struct UnarySignRule
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-">) +
                 assign<&ast::UnaryExpr::operand>(ref<AtomRule>)) |
             ref<AtomRule>> {};

  struct ArithExprRule
      : infix<UnarySignRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(OpPow) >> left(OpMul) >> left(OpAdd))> {};

  struct ComparisonExprRule
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<ArithExprRule>) +
                 assign<&ast::BinaryExpr::op>(OpCompare) +
                 assign<&ast::BinaryExpr::right>(ref<ArithExprRule>)) |
             ref<ArithExprRule>> {};

  struct NotExprRule
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ikw<"NOT">) +
                 assign<&ast::UnaryExpr::operand>(ref<ComparisonExprRule>)) |
             ref<ComparisonExprRule>> {};

  struct ExpressionRule
      : infix<NotExprRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(ref<AND_OP>) >> left(ref<OR_OP>))> {};

  // ------------------------------------------------------------- typing

  struct TypeSpecRule
      : rule<create<ast::TypeSpec>(
            assign<&ast::TypeSpec::name>(DataTypeKw | ref<ID>) +
            option(lit<"{"> + assign<&ast::TypeSpec::scale>(ref<NUMBER>) +
                   lit<"}">))> {};
  static constexpr auto TypeSpecText =
      (DataTypeKw | ref<ID>) + option(lit<"{"> + ref<NUMBER> + lit<"}">);

  struct SignedNumberTextRule
      : rule<option(lit<"+"> | lit<"-">) + ref<NUMBER>> {};

  struct ArrayBoundRule
      : rule<create<ast::ArrayBound>(
            assign<&ast::ArrayBound::from>(ref<SignedNumberTextRule>) +
            option(ikw<"TO"> + assign<&ast::ArrayBound::to>(
                                    ref<SignedNumberTextRule>)))> {};

  struct ArrayDeclRule
      : rule<create<ast::ArrayDecl>(
            lit<"["> +
            option(some(append<&ast::ArrayDecl::bounds>(ref<ArrayBoundRule>),
                       lit<",">)) +
            lit<"]">)> {};

  struct ParameterRule
      : rule<create<ast::Parameter>(
            option(flag<&ast::Parameter::readOnly>(ikw<"READONLY">)) +
            option(flag<&ast::Parameter::byRef>(ikw<"REF">)) +
            assign<&ast::Parameter::type>(ref<TypeSpecRule>) +
            assign<&ast::Parameter::name>(ref<ID>) +
            option(assign<&ast::Parameter::array>(ref<ArrayDeclRule>)))> {};


  // ------------------------------------------------------- declarations

  struct AssignStmtRule;

  struct DeclaratorRule
      : rule<assign<&ast::Declarator::name>(ref<ID>) +
             option(assign<&ast::Declarator::array>(ref<ArrayDeclRule>))> {};

  // The four variable_decl_sub shapes and the modifiers ahead of them all
  // write directly into the enclosing ast::VariableDecl: spliced as plain
  // expressions, never their own `rule<>` (that would give the alternative a
  // second, competing head).

  // variable_decl_sub0: a shared type, one or more names, one shared
  // initializer for the whole list.
  static constexpr auto VarSub0 =
      assign<&ast::VariableDecl::type>(ref<TypeSpecRule>) +
      some(append<&ast::VariableDecl::declarators>(ref<DeclaratorRule>),
          lit<",">) +
      option(lit<"="> +
             assign<&ast::VariableDecl::sharedInit>(ref<ExpressionRule>));

  // variable_decl_sub2: a shared type, exactly one name, an optional
  // brace-list initializer attached to that one name.
  static constexpr auto VarSub2 =
      assign<&ast::VariableDecl::type>(ref<TypeSpecRule>) +
      append<&ast::VariableDecl::declarators>(create<ast::Declarator>(
          assign<&ast::Declarator::name>(IdentifierNameEx) +
          option(assign<&ast::Declarator::array>(ref<ArrayDeclRule>)) +
          option(option(lit<"=">) +
                 assign<&ast::Declarator::init>(ref<BraceListRule>))));

  // variable_decl_sub1: a shared type, one or more chained assignments.
  static constexpr auto VarSub1 =
      assign<&ast::VariableDecl::type>(ref<TypeSpecRule>) +
      append<&ast::VariableDecl::assigns>(ref<AssignStmtRule>) +
      many(lit<","> + option(TypeSpecText) +
           append<&ast::VariableDecl::assigns>(ref<AssignStmtRule>));

  // `EVENT name(args)` is an event call or a forward declaration, never this.
  static constexpr auto VarEvent =
      flag<&ast::VariableDecl::isEventForm>(ikw<"EVENT">) +
      assign<&ast::VariableDecl::eventName>(ref<PathRule>);

  static constexpr auto VariableDeclSub =
      option(flag<&ast::VariableDecl::indirect>(ikw<"INDIRECT">)) +
      option(assign<&ast::VariableDecl::accessModifPart>(
          AccessModifPartKw)) +
      option(assign<&ast::VariableDecl::scope>(ScopeModifKw)) +
      (VarSub0 | VarSub1 | VarSub2 | VarEvent);

  struct VariableDeclRule
      : rule<create<ast::VariableDecl>(
            option(assign<&ast::VariableDecl::accessType>(AccessTypeKw)) +
            VariableDeclSub) +
            option(lit<";">)> {};

  struct ConstantDeclRule
      : rule<create<ast::VariableDecl>(
                 option(assign<&ast::VariableDecl::accessType>(
                     AccessTypeKw)) +
                 flag<&ast::VariableDecl::isConstant>(ikw<"CONSTANT">) +
                 VariableDeclSub) +
             option(lit<";">)> {};

  struct AccessMarkerRule
      : rule<create<ast::AccessMarker>(
            assign<&ast::AccessMarker::kind>(AccessTypeKw) + lit<":">)> {};

  // ---------------------------------------------------------- statements

  struct AtAssignStmtRule
      : rule<create<ast::AtAssignStmt>(
            lit<"@"> + assign<&ast::AtAssignStmt::target>(ref<PathRule>) +
            lit<"="> +
            assign<&ast::AtAssignStmt::value>(ref<BindParamRule>) +
            lit<";">)> {};

  static constexpr auto AssignOp =
      lit<"="> | lit<"+="> | lit<"-="> | lit<"*="> | lit<"/=">;

  struct AssignStmtRule
      : rule<create<ast::AssignStmt>(
            assign<&ast::AssignStmt::target>(ref<PostfixChainRule>) +
            assign<&ast::AssignStmt::op>(AssignOp) +
            some(append<&ast::AssignStmt::rhs>(ref<ExpressionRule>),
                lit<",">) +
            option(lit<";">))> {};

  struct IncDecOpRule
      : rule<(lit<"+"> + lit<"+">) | (lit<"-"> + lit<"-">)> {};

  struct IncDecStmtRule
      : rule<create<ast::IncDecStmt>(
            assign<&ast::IncDecStmt::target>(ref<PathRule>) +
            assign<&ast::IncDecStmt::op>(ref<IncDecOpRule>))> {};

  struct ExprStmtRule
      : rule<create<ast::ExprStmt>(assign<&ast::ExprStmt::value>(
            ref<PostfixChainRule> | ref<SpecialCallRule> |
            ref<CreateExprRule> | ref<DescribeExprRule>))> {};

  // `CALL name(args)`, `CALL name::CREATE`, and the everyday
  // `CALL SUPER::eventname` (no parens at all -- the corpus's most common
  // shape, an ancestor's version of the handler in hand): call_statement and
  // super_call_statement's two CALL-prefixed forms.
  struct CallStmtRule
      : rule<create<ast::Call>(
            ikw<"CALL"> +
            ((flag<&ast::Call::ancestor>(ikw<"SUPER">) + lit<"::"> +
              (flag<&ast::Call::isEvent>(ikw<"EVENT">) |
               assign<&ast::Call::name>(ikw<"CREATE"> | ikw<"DESTROY"> |
                                        ikw<"OPEN"> | ikw<"CLOSE"> |
                                        IdentifierNameEx)) +
              option(flag<&ast::Call::hasParens>(lit<"(">) +
                     option(some(
                         append<&ast::Call::args>(ref<ArgumentRule>),
                         lit<",">)) +
                     lit<")">)) |
             (option(assign<&ast::Call::receiver>(ref<PathRule>) +
                     lit<"`">) +
              assign<&ast::Call::name>(ref<ID> | ikw<"DESCRIBE">) +
              option((flag<&ast::Call::ancestor>(lit<"::">) +
                      (ikw<"CREATE"> | ikw<"DESTROY"> | ikw<"OPEN"> |
                       ikw<"CLOSE"> | IdentifierNameEx)) |
                     (flag<&ast::Call::hasParens>(lit<"(">) +
                      option(some(
                          append<&ast::Call::args>(ref<ArgumentRule>),
                          lit<",">)) +
                      lit<")">)))) +
            option(lit<";">))> {};

  struct DescribeStmtRule
      : rule<create<ast::DescribeStmt>(
            ikw<"DESCRIBE"> +
            assign<&ast::DescribeStmt::name>(ref<PathRule>))> {};

  struct DestroyStmtRule
      : rule<create<ast::DestroyStmt>(
            ikw<"DESTROY"> +
            assign<&ast::DestroyStmt::value>(ref<ExpressionRule>))> {};

  struct ThrowStmtRule
      : rule<create<ast::ThrowStmt>(
            ikw<"THROW"> +
            assign<&ast::ThrowStmt::value>(ref<ExpressionRule>))> {};

  struct ReturnStmtRule
      : rule<create<ast::ReturnStmt>(
            ikw<"RETURN"> +
            option(assign<&ast::ReturnStmt::value>(ref<ExpressionRule>)))> {
  };

  struct ContinueStmtRule : rule<create<ast::ContinueStmt> + ikw<"CONTINUE">> {
  };
  struct ExitStmtRule : rule<create<ast::ExitStmt> + ikw<"EXIT">> {};

  struct GotoStmtRule
      : rule<create<ast::GotoStmt>(
            ikw<"GOTO"> +
            assign<&ast::GotoStmt::target>(ref<PathRule>))> {};

  struct LabelStmtRule
      : rule<create<ast::LabelStmt>(
            assign<&ast::LabelStmt::name>(ref<ID>) + lit<":">)> {};

  struct StatementRule;

  struct StmtLineRule : rule<ref<StatementRule> + option(lit<";">)> {};

  struct ElseIfRule
      : rule<create<ast::ElseIf>(
            ikw<"ELSEIF"> +
            assign<&ast::ElseIf::cond>(ref<ExpressionRule>) + ikw<"THEN"> +
            many(append<&ast::ElseIf::body>(ref<StmtLineRule>)))> {};

  struct IfBlockStmtRule
      : rule<create<ast::IfStmt>(
            ikw<"IF"> + assign<&ast::IfStmt::cond>(ref<ExpressionRule>) +
            ikw<"THEN"> +
            many(append<&ast::IfStmt::thenBody>(ref<StmtLineRule>)) +
            many(append<&ast::IfStmt::elseIfs>(ref<ElseIfRule>)) +
            option(ikw<"ELSE"> +
                   many(append<&ast::IfStmt::elseBody>(ref<StmtLineRule>))) +
            flag<&ast::IfStmt::blockForm>(ikw<"END"> + ikw<"IF">) +
            option(lit<";">))> {};

  struct IfSimpleStmtRule
      : rule<create<ast::IfStmt>(
            ikw<"IF"> + assign<&ast::IfStmt::cond>(ref<ExpressionRule>) +
            ikw<"THEN"> +
            append<&ast::IfStmt::thenBody>(ref<StatementRule>) +
            option(ikw<"ELSE"> +
                   append<&ast::IfStmt::elseBody>(ref<StatementRule>)) +
            option(lit<";">))> {};

  struct CaseValueRangeRule
      : rule<create<ast::CaseValueRange>(
            assign<&ast::CaseValueRange::from>(ref<ExpressionRule>) +
            option(ikw<"TO"> + assign<&ast::CaseValueRange::to>(
                                    ref<ExpressionRule>)))> {};

  struct ChooseCaseValueRule
      : rule<create<ast::ChooseCaseValue>(
            ikw<"CASE"> +
            some(append<&ast::ChooseCaseValue::ranges>(
                     ref<CaseValueRangeRule>),
                lit<",">) +
            many(append<&ast::ChooseCaseValue::body>(ref<StmtLineRule>)))> {
  };

  struct ChooseCaseCondRule
      : rule<create<ast::ChooseCaseCond>(
            ikw<"CASE"> + ikw<"IS"> +
            assign<&ast::ChooseCaseCond::op>(OpCompare) +
            assign<&ast::ChooseCaseCond::value>(ref<ExpressionRule>) +
            many(append<&ast::ChooseCaseCond::body>(ref<StmtLineRule>)))> {};

  struct ChooseCaseElseRule
      : rule<create<ast::ChooseCaseElse>(
            ikw<"CASE"> + ikw<"ELSE"> +
            many(append<&ast::ChooseCaseElse::body>(ref<StmtLineRule>)))> {};

  struct ChooseStmtRule
      : rule<create<ast::ChooseStmt>(
            ikw<"CHOOSE"> + ikw<"CASE"> +
            assign<&ast::ChooseStmt::value>(ref<ExpressionRule>) +
            some(append<&ast::ChooseStmt::cases>(ref<ChooseCaseCondRule>) |
                 append<&ast::ChooseStmt::cases>(ref<ChooseCaseElseRule>) |
                 append<&ast::ChooseStmt::cases>(ref<ChooseCaseValueRule>)) +
            ikw<"END"> + ikw<"CHOOSE">)> {};

  struct ForStmtRule
      : rule<create<ast::ForStmt>(
            ikw<"FOR"> + assign<&ast::ForStmt::var>(ref<PathRule>) +
            lit<"="> + assign<&ast::ForStmt::from>(ref<ExpressionRule>) +
            ikw<"TO"> + assign<&ast::ForStmt::to>(ref<ExpressionRule>) +
            option(ikw<"STEP"> +
                   assign<&ast::ForStmt::step>(ref<ExpressionRule>)) +
            many(append<&ast::ForStmt::body>(ref<StmtLineRule>)) +
            (ikw<"NEXT"> | (ikw<"END"> + ikw<"FOR">)))> {};

  struct DoWhileStmtRule
      : rule<create<ast::DoWhileStmt>(
            flag<&ast::DoWhileStmt::testFirst>(ikw<"DO">) +
            (ikw<"WHILE"> |
             flag<&ast::DoWhileStmt::isUntil>(ikw<"UNTIL">)) +
            assign<&ast::DoWhileStmt::cond>(ref<ExpressionRule>) +
            many(append<&ast::DoWhileStmt::body>(ref<StmtLineRule>)) +
            ikw<"LOOP">)> {};

  struct DoLoopWhileStmtRule
      : rule<create<ast::DoWhileStmt>(
            ikw<"DO"> +
            many(append<&ast::DoWhileStmt::body>(ref<StmtLineRule>)) +
            ikw<"LOOP"> +
            (ikw<"WHILE"> |
             flag<&ast::DoWhileStmt::isUntil>(ikw<"UNTIL">)) +
            assign<&ast::DoWhileStmt::cond>(ref<ExpressionRule>))> {};

  struct CatchClauseRule
      : rule<create<ast::CatchClause>(
            ikw<"CATCH"> + lit<"("> +
            assign<&ast::CatchClause::exceptionVar>(
                create<ast::VariableDecl>(VariableDeclSub)) +
            lit<")"> +
            many(append<&ast::CatchClause::body>(ref<StmtLineRule>)))> {};

  struct TryStmtRule
      : rule<create<ast::TryStmt>(
            ikw<"TRY"> +
            many(append<&ast::TryStmt::body>(ref<StmtLineRule>)) +
            many(append<&ast::TryStmt::catches>(ref<CatchClauseRule>)) +
            option(flag<&ast::TryStmt::hasFinally>(ikw<"FINALLY">) +
                   many(append<&ast::TryStmt::finallyBody>(
                       ref<StmtLineRule>))) +
            ikw<"END"> + ikw<"TRY">)> {};

  // ---------------------------------------------------------------- SQL

  struct SqlInsertStmtRule
      : rule<create<ast::SqlInsertStmt>(
            ikw<"INSERT"> + ikw<"INTO"> +
            assign<&ast::SqlInsertStmt::table>(ref<PathRule>) + lit<"("> +
            some(append<&ast::SqlInsertStmt::columns>(ref<PathRule>),
                lit<",">) +
            lit<")"> + ikw<"VALUES"> + lit<"("> +
            some(append<&ast::SqlInsertStmt::values>(
                     ref<ExpressionRule> | ref<BindParamRule>),
                lit<",">) +
            lit<")"> + option(lit<";">))> {};

  struct SqlDeleteStmtRule
      : rule<create<ast::SqlDeleteStmt>(
            ikw<"DELETE"> + ikw<"FROM"> +
            assign<&ast::SqlDeleteStmt::table>(ref<PathRule>) +
            assign<&ast::SqlDeleteStmt::where>(ref<WhereClauseRule>) +
            option(lit<";">))> {};

  struct SqlSelectStmtRule
      : rule<create<ast::SqlSelectStmt>(
            (ikw<"SELECT"> |
             flag<&ast::SqlSelectStmt::isBlob>(ikw<"SELECTBLOB">)) +
            (some(append<&ast::SqlSelectStmt::columns>(ref<PathRule>),
                 lit<",">) |
             assign<&ast::SqlSelectStmt::funcCall>(ref<CallHeadRule>)) +
            ikw<"INTO"> +
            some(append<&ast::SqlSelectStmt::into>(ref<BindParamRule>),
                lit<",">) +
            ikw<"FROM"> +
            some(append<&ast::SqlSelectStmt::from>(ref<PathRule>), lit<",">) +
            option(assign<&ast::SqlSelectStmt::where>(ref<WhereClauseRule>)) +
            option(ikw<"USING"> +
                   assign<&ast::SqlSelectStmt::usingConn>(ref<PathRule>)) +
            option(lit<";">))> {};

  struct SqlUpdateStmtRule
      : rule<create<ast::SqlUpdateStmt>(
            (ikw<"UPDATE"> |
             flag<&ast::SqlUpdateStmt::isBlob>(ikw<"UPDATEBLOB">)) +
            assign<&ast::SqlUpdateStmt::table>(ref<PathRule>) + ikw<"SET"> +
            some(append<&ast::SqlUpdateStmt::setValues>(ref<SqlSetItemRule>),
                lit<",">) +
            option(
                assign<&ast::SqlUpdateStmt::where>(ref<WhereClauseRule>)))> {
  };

  struct SqlConnectStmtRule
      : rule<create<ast::SqlConnectStmt>(
            assign<&ast::SqlConnectStmt::keyword>(
                ikw<"CONNECT"> | ikw<"DISCONNECT"> | ikw<"ROLLBACK">) +
            option(flag<&ast::SqlConnectStmt::usingClause>(
                       ikw<"USING">) +
                   (flag<&ast::SqlConnectStmt::usingSqlca>(
                        ikw<"SQLCA">) |
                    assign<&ast::SqlConnectStmt::usingName>(ref<PathRule>))) +
            lit<";">)> {};

  struct SqlCommitStmtRule
      : rule<create<ast::SqlCommitStmt>(
            ikw<"COMMIT"> +
            option(flag<&ast::SqlCommitStmt::usingClause>(
                ikw<"USING">)) +
            option(flag<&ast::SqlCommitStmt::usingSqlca>(ikw<"SQLCA">) |
                   assign<&ast::SqlCommitStmt::usingName>(ref<PathRule>)) +
            option(lit<";">))> {};

  struct ExecuteStmtRule
      : rule<create<ast::ExecuteStmt>(
            ikw<"EXECUTE"> +
            (option(flag<&ast::ExecuteStmt::immediate>(
                 ikw<"IMMEDIATE">)) +
                 ((assign<&ast::ExecuteStmt::value>(
                       ref<PathRule> | ref<AtomRule>) +
                   option(lit<";">)) |
                  (assign<&ast::ExecuteStmt::value>(ref<BindParamRule>) +
                   option(ikw<"USING"> +
                          (flag<&ast::ExecuteStmt::usingSqlca>(
                               ikw<"SQLCA">) |
                           assign<&ast::ExecuteStmt::usingName>(
                               ref<PathRule>))) +
                   lit<";">)) |
             (option(flag<&ast::ExecuteStmt::dynamic>(
                  ikw<"DYNAMIC">)) +
              assign<&ast::ExecuteStmt::identifier>(ref<PathRule>) +
              option(ikw<"USING"> +
                     option(flag<&ast::ExecuteStmt::usingDescriptor>(
                         ikw<"DESCRIPTOR">)) +
                     (flag<&ast::ExecuteStmt::usingSqlca>(
                          ikw<"SQLCA">) |
                      assign<&ast::ExecuteStmt::usingName>(
                          ref<PathRule>))) +
              option(lit<";">))))> {};

  struct CloseSqlStmtRule
      : rule<create<ast::CloseSqlStmt>(
            ikw<"CLOSE"> +
            assign<&ast::CloseSqlStmt::cursor>(ref<PathRule>) +
            option(lit<";">))> {};

  struct DeclareProcedureStmtRule
      : rule<create<ast::DeclareProcedureStmt>(
            ikw<"DECLARE"> +
            assign<&ast::DeclareProcedureStmt::handle>(ref<PathRule>) +
            option(flag<&ast::DeclareProcedureStmt::dynamic>(
                ikw<"DYNAMIC">)) +
            ikw<"PROCEDURE"> + ikw<"FOR"> +
            assign<&ast::DeclareProcedureStmt::procName>(ref<PathRule>) +
            option(lit<";">))> {};

  struct DeclareCursorStmtRule
      : rule<create<ast::DeclareCursorStmt>(
            ikw<"DECLARE"> +
            assign<&ast::DeclareCursorStmt::cursor>(ref<PathRule>) +
            option(flag<&ast::DeclareCursorStmt::dynamic>(
                ikw<"DYNAMIC">)) +
            ikw<"CURSOR"> + ikw<"FOR"> +
            assign<&ast::DeclareCursorStmt::forName>(ref<PathRule>) +
            lit<";">)> {};

  struct OpenCursorStmtRule
      : rule<create<ast::OpenCursorStmt>(
            ikw<"OPEN"> +
            option(flag<&ast::OpenCursorStmt::dynamic>(
                ikw<"DYNAMIC">)) +
            assign<&ast::OpenCursorStmt::cursor>(ref<PathRule>) +
            option(ikw<"USING"> +
                   (flag<&ast::OpenCursorStmt::usingDescriptor>(
                        ikw<"DESCRIPTOR">) |
                    assign<&ast::OpenCursorStmt::usingName>(
                        ref<PathRule>))) +
            option(assign<&ast::OpenCursorStmt::trailingIdent>(
                ref<PathRule>)) +
            option(lit<";">))> {};

  struct FetchStmtRule
      : rule<create<ast::FetchStmt>(
            ikw<"FETCH"> +
            ((assign<&ast::FetchStmt::cursor>(ref<PathRule>) + ikw<"INTO"> +
              assign<&ast::FetchStmt::into>(ref<BindParamRule>)) |
             (assign<&ast::FetchStmt::usingIdent>(ref<PathRule>) +
              flag<&ast::FetchStmt::usingDescriptorForm>(ikw<"USING">) +
              option(ikw<"DESCRIPTOR">) +
              assign<&ast::FetchStmt::usingTarget>(ref<PathRule>))) +
            option(lit<";">))> {};

  struct PrepareStmtRule
      : rule<create<ast::PrepareStmt>(
            ikw<"PREPARE"> +
            assign<&ast::PrepareStmt::stmt>(ref<PathRule>) + ikw<"FROM"> +
            assign<&ast::PrepareStmt::from>(ref<BindParamRule>) +
            ikw<"USING"> +
            assign<&ast::PrepareStmt::usingName>(ref<PathRule>) + lit<";">)>{
  };

  // ------------------------------------------------------------ statement

  struct StatementRule
      : rule<ref<IncDecStmtRule> | ref<AccessMarkerRule> |
             ref<IfSimpleStmtRule> | ref<ExecuteStmtRule> |
             ref<ThrowStmtRule> | ref<DescribeStmtRule> |
             ref<AtAssignStmtRule> | ref<AssignStmtRule> | ref<TryStmtRule> |
             ref<CloseSqlStmtRule> | ref<IfBlockStmtRule> |
             ref<VariableDeclRule> | ref<ConstantDeclRule> |
             ref<DoLoopWhileStmtRule> | ref<DoWhileStmtRule> |
             ref<DestroyStmtRule> | ref<LabelStmtRule> | ref<GotoStmtRule> |
             ref<ChooseStmtRule> | ref<ReturnStmtRule> | ref<ForStmtRule> |
             ref<ContinueStmtRule> | ref<ExitStmtRule> |
             ref<SqlInsertStmtRule> | ref<SqlDeleteStmtRule> |
             ref<SqlSelectStmtRule> | ref<SqlUpdateStmtRule> |
             ref<SqlConnectStmtRule> | ref<SqlCommitStmtRule> |
             ref<OpenCursorStmtRule> | ref<PrepareStmtRule> |
             ref<DeclareCursorStmtRule> | ref<DeclareProcedureStmtRule> |
             ref<FetchStmtRule> | ref<CallStmtRule> | ref<ExprStmtRule>> {};

  // -------------------------------------------------- top-level constructs

  struct DataTypeNameRule
      : rule<create<ast::TypeSpec>(
            assign<&ast::TypeSpec::name>(DataTypeKw | ref<ID>))> {};

  static constexpr auto EventForwardParams =
      some(append<&ast::EventForwardDecl::params>(ref<ParameterRule>),
          lit<",">) +
      option(lit<","> +
             flag<&ast::EventForwardDecl::varargs>(lit<"...">));

  struct EventForwardDeclRule
      : rule<create<ast::EventForwardDecl>(
            ikw<"EVENT"> +
            ((assign<&ast::EventForwardDecl::name>(ref<ID> | ikw<"CREATE"> |
                                                    ikw<"DESTROY">) +
              option(assign<&ast::EventForwardDecl::basedOn>(ref<ID>)) +
              option(flag<&ast::EventForwardDecl::hasParens>(
                         lit<"(">) +
                     option(EventForwardParams) + lit<")">)) |
             (ikw<"TYPE"> +
              assign<&ast::EventForwardDecl::type>(ref<DataTypeNameRule>) +
              assign<&ast::EventForwardDecl::name>(ref<ID>) +
              flag<&ast::EventForwardDecl::hasParens>(lit<"(">) +
              option(EventForwardParams) + lit<")">)))> {};

  struct TypeDeclRule
      : rule<create<ast::TypeDecl>(
            option(assign<&ast::TypeDecl::scope>(ScopeModifKw)) +
            ikw<"TYPE"> + assign<&ast::TypeDecl::name>(ref<ID>) +
            ikw<"FROM"> +
            option(assign<&ast::TypeDecl::libraryQualifier>(ref<ID>) +
                   lit<"`">) +
            assign<&ast::TypeDecl::super_>(ref<DataTypeNameRule>) +
            option(ikw<"WITHIN"> +
                   assign<&ast::TypeDecl::within>(ref<ID>)) +
            option(flag<&ast::TypeDecl::autoInstantiate>(
                ikw<"AUTOINSTANTIATE">)) +
            option(ikw<"DESCRIPTOR"> +
                   assign<&ast::TypeDecl::descriptorKey>(
                       ref<DQUOTED_STRING>) +
                   lit<"="> +
                   assign<&ast::TypeDecl::descriptorValue>(
                       ref<DQUOTED_STRING>)) +
            many(append<&ast::TypeDecl::members>(ref<VariableDeclRule>) |
                 append<&ast::TypeDecl::members>(
                     ref<EventForwardDeclRule>)) +
            ikw<"END"> + ikw<"TYPE">)> {};

  struct ForwardDeclRule
      : rule<create<ast::ForwardDecl>(
            ikw<"FORWARD"> +
            some(append<&ast::ForwardDecl::items>(ref<TypeDeclRule>) |
                 append<&ast::ForwardDecl::items>(ref<VariableDeclRule>)) +
            ikw<"END"> + ikw<"FORWARD">)> {};

  struct TypeVariablesDeclRule
      : rule<create<ast::TypeVariablesDecl>(
            ikw<"TYPE"> + ikw<"VARIABLES"> +
            many(append<&ast::TypeVariablesDecl::items>(
                     ref<VariableDeclRule>) |
                 append<&ast::TypeVariablesDecl::items>(
                     ref<ConstantDeclRule>) |
                 append<&ast::TypeVariablesDecl::items>(
                     ref<AccessMarkerRule>)) +
            ikw<"END"> + ikw<"VARIABLES">)> {};

  struct GlobalVariablesDeclRule
      : rule<create<ast::GlobalVariablesDecl>(
                 ikw<"GLOBAL"> +
                 assign<&ast::GlobalVariablesDecl::single>(
                     ref<VariableDeclRule>)) |
             create<ast::GlobalVariablesDecl>(
                 (ikw<"GLOBAL"> |
                  flag<&ast::GlobalVariablesDecl::shared_>(
                      ikw<"SHARED">)) +
                 ikw<"VARIABLES"> +
                 many(append<&ast::GlobalVariablesDecl::items>(
                     ref<VariableDeclRule>)) +
                 ikw<"END"> + ikw<"VARIABLES">)> {};

  struct FunctionForwardDeclRule
      : rule<create<ast::FunctionForwardDecl>(
            option(assign<&ast::FunctionForwardDecl::accessModifPart>(
                AccessModifPartKw)) +
            option(assign<&ast::FunctionForwardDecl::scope>(ScopeModifKw)) +
            ((ikw<"FUNCTION"> +
              assign<&ast::FunctionForwardDecl::returnType>(
                  ref<DataTypeNameRule>)) |
             flag<&ast::FunctionForwardDecl::isSubroutine>(
                 ikw<"SUBROUTINE">)) +
            assign<&ast::FunctionForwardDecl::name>(ref<ID>) + lit<"("> +
            option(some(append<&ast::FunctionForwardDecl::params>(
                            ref<ParameterRule>),
                       lit<",">) +
                   option(lit<","> +
                          flag<&ast::FunctionForwardDecl::varargs>(
                              lit<"...">))) +
            lit<")"> +
            option(
                (ikw<"ALIAS"> + ikw<"FOR"> +
                 assign<&ast::FunctionForwardDecl::aliasText>(ref<ID>) +
                 ikw<"LIBRARY"> +
                 (ref<DQUOTED_STRING> | ref<QUOTED_STRING>)) |
                (ikw<"LIBRARY"> + (ref<DQUOTED_STRING> | ref<QUOTED_STRING>) +
                 option(ikw<"ALIAS"> + ikw<"FOR"> +
                        (ref<DQUOTED_STRING> | ref<QUOTED_STRING>))) |
                (ikw<"RPCFUNC"> + ikw<"ALIAS"> + ikw<"FOR"> +
                 option(ref<DQUOTED_STRING> | ref<QUOTED_STRING>) +
                 option(ikw<"THROWS"> + ref<ID>)) |
                (ikw<"THROWS"> + ref<ID>)))> {};

  struct FunctionsForwardDeclRule
      : rule<create<ast::FunctionsForwardDecl>(
            (flag<&ast::FunctionsForwardDecl::isForward>(
                 ikw<"FORWARD">) |
             ikw<"TYPE">) +
            ikw<"PROTOTYPES"> +
            many(append<&ast::FunctionsForwardDecl::items>(
                ref<FunctionForwardDeclRule>)) +
            ikw<"END"> + ikw<"PROTOTYPES">)> {};

  struct FunctionBodyRule
      : rule<create<ast::FunctionBody>(
            option(assign<&ast::FunctionBody::accessType>(AccessTypeKw)) +
            option(assign<&ast::FunctionBody::scope>(ScopeModifKw)) +
            ((ikw<"FUNCTION"> +
              assign<&ast::FunctionBody::returnType>(
                  ref<DataTypeNameRule>)) |
             flag<&ast::FunctionBody::isSubroutine>(
                 ikw<"SUBROUTINE">)) +
            assign<&ast::FunctionBody::name>(ref<ID>) + lit<"("> +
            option(some(append<&ast::FunctionBody::params>(ref<ParameterRule>),
                       lit<",">) +
                   option(lit<","> +
                          flag<&ast::FunctionBody::varargs>(
                              lit<"...">))) +
            lit<")"> +
            option(ikw<"THROWS"> +
                   assign<&ast::FunctionBody::throwsName>(ref<ID>)) +
            option(lit<";">) +
            many(append<&ast::FunctionBody::body>(ref<StmtLineRule>)) +
            ikw<"END"> + (ikw<"FUNCTION"> | ikw<"SUBROUTINE">))> {};

  struct OnBodyRule
      : rule<create<ast::OnBody>(
            ikw<"ON"> + assign<&ast::OnBody::target>(ref<PathRule>) +
            option((lit<"."> +
                    assign<&ast::OnBody::suffix>(ikw<"CREATE"> |
                                                 ikw<"DESTROY">)) |
                   assign<&ast::OnBody::suffix>(ikw<"OPEN"> |
                                                ikw<"CLOSE">)) +
            option(lit<";">) +
            many(append<&ast::OnBody::body>(ref<StmtLineRule>)) +
            ikw<"END"> + ikw<"ON">)> {};

  struct EventBodyRule
      : rule<create<ast::EventBody>(
            ikw<"EVENT"> +
            option(ikw<"TYPE"> +
                   assign<&ast::EventBody::type>(ref<DataTypeNameRule>)) +
            option(assign<&ast::EventBody::qualifier>(ref<ID>) +
                   lit<"::">) +
            assign<&ast::EventBody::name>(ref<ID> | ikw<"OPEN"> |
                                          ikw<"CLOSE">) +
            option(flag<&ast::EventBody::hasParens>(lit<"(">) +
                   option(some(append<&ast::EventBody::params>(
                                   ref<ParameterRule>),
                              lit<",">) +
                          option(lit<","> +
                                 flag<&ast::EventBody::varargs>(
                                     lit<"...">))) +
                   lit<")">) +
            option(lit<";">) +
            many(append<&ast::EventBody::body>(ref<StmtLineRule>)) +
            ikw<"END"> + ikw<"EVENT">)> {};

  struct BodyRule
      : rule<ref<TypeDeclRule> | ref<AccessMarkerRule> |
             ref<ForwardDeclRule> | ref<TypeVariablesDeclRule> |
             ref<GlobalVariablesDeclRule> | ref<VariableDeclRule> |
             ref<ConstantDeclRule> | ref<FunctionForwardDeclRule> |
             ref<FunctionsForwardDeclRule> | ref<FunctionBodyRule> |
             ref<OnBodyRule> | ref<EventBodyRule>> {};

  struct DocumentRule
      : rule<create<ast::Document>(
            option(ikw<"RELEASE"> +
                   assign<&ast::Document::release>(ref<NUMBER>) +
                   lit<";">) +
            some(append<&ast::Document::items>(ref<BodyRule>)))> {};

  using entry = DocumentRule;
  using ignore = WS;
  using hidden = list<SlComment, MlComment, LineContinuation, ExportHeader>;
};

}  // namespace powerbuilder
