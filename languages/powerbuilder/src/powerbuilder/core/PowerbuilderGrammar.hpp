// The powerbuilder language: its grammar. PowerScript source exports read as
// a flat sequence of declarations and statements; `&` before a line break and
// nested comments are trivia. RETURN observes a physical statement break,
// unless an explicit ampersand continues the statement.
#pragma once

#include <cstddef>
#include <memory_resource>
#include <span>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "powerbuilder/core/ast.hpp"

namespace powerbuilder {

inline constexpr auto parse_number = [](std::string_view t) {
  if (!t.empty()) {
    const char c = t.back();
    if (c == 'D' || c == 'd' || c == 'F' || c == 'f') t.remove_suffix(1);
  }
  return cursiv::default_convert<double>(t);
};

// PowerScript tilde escapes are shared by both quote styles. Numeric escapes
// in the ASCII range have a portable value; extended code-page bytes remain
// spelled as entered rather than being assigned a guessed Unicode character.
inline constexpr auto unescape_string = [](std::string_view t,
                                            std::pmr::memory_resource* mem) {
  const std::string_view body = t.substr(1, t.size() - 2);
  if (body.find_first_of("~&") == std::string_view::npos) return body;
  const std::span<char> room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    if(body[i]=='&') {
      std::size_t j=i+1;
      while(j<body.size()&&(body[j]==' '||body[j]=='\t'))++j;
      if(j<body.size()&&body[j]=='\r')++j;
      if(j<body.size()&&body[j]=='\n') { i=j; continue; }
    }
    if (body[i] != '~' || i + 1 == body.size()) { room[n++]=body[i]; continue; }
    const char c=body[i+1];
    char decoded{};
    bool known=true;
    switch(c) {
      case '~': case '"': case '\'': decoded=c; break;
      case 'n': decoded='\n'; break; case 'r': decoded='\r'; break;
      case 't': decoded='\t'; break; case 'v': decoded='\v'; break;
      case 'f': decoded='\f'; break; case 'b': decoded='\b'; break;
      default: known=false;
    }
    if(known) { room[n++]=decoded; ++i; continue; }
    const unsigned base=c=='h'?16:c=='o'?8:10;
    const std::size_t begin=i+(base==10?1:2), count=base==16?2:3;
    unsigned value=0; bool digits=begin+count<=body.size();
    for(std::size_t j=begin;digits&&j<begin+count;++j) {
      const char d=body[j];
      const unsigned digit=d>='0'&&d<='9'?unsigned(d-'0'):
          d>='a'&&d<='f'?unsigned(d-'a'+10):d>='A'&&d<='F'?unsigned(d-'A'+10):99;
      if(digit>=base) digits=false; else value=value*base+digit;
    }
    if(digits&&value<128) { room[n++]=static_cast<char>(value); i=begin+count-1; }
    else room[n++]=body[i];
  }
  return std::string_view(room.data(), n);
};

struct Powerbuilder : cursiv::GrammarSpec {
  // Identifiers spell `-$#%_` past letters and digits (ID_PARTS), so a
  // keyword's boundary must cover the same set or e.g. `ikw<"IF">` would
  // wrongly close before `-` inside a hyphenated name.
  static constexpr auto word_boundary = cr<"A-Za-z0-9$#%_-">;

  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct LineContinuation : terminal<lit<"&"> + many(cr<R"( \t)">) + eol> {};
  struct SlComment : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct MlComment : terminal<lit<"/*"> + many(ref<MlComment> | (!(lit<"/*"> | lit<"*/">) + any)) + lit<"*/">> {};
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
      (some(d) + lit<"."> + many(d)) | (lit<"."> + some(d)) | some(d);
  struct NUMBER : terminal<NumCore + option(cr<"Ee"> + option(cr<"+-">) + some(d)) +
                      option(cr<"DFdf">)> {};
  struct ConvertedNumber : rule<as<parse_number>(ref<NUMBER>)> {};

  struct DATE : terminal<repeat<4>(d) + lit<"-"> + repeat<2>(d) + lit<"-"> +
                         repeat<2>(d)> {};
  struct TIME : terminal<repeat<2>(d) + lit<":"> + repeat<2>(d) + lit<":"> +
                         repeat<2>(d) + option(lit<"."> + repeat<6>(d))> {};

  struct DQUOTED_STRING
      : terminal<as<unescape_string>(
            lit<"\""> + many((lit<"~"> + cr<R"(^\r\n)">) | (lit<"&"> + many(cr<R"( \t)">) + eol) |
                cr<R"(^"~\r\n)">) + lit<"\"">)> {};
  struct QUOTED_STRING
      : terminal<as<unescape_string>(lit<"'"> + many((lit<"~"> + cr<R"(^\r\n)">) |
                 (lit<"&"> + many(cr<R"( \t)">) + eol) | cr<R"(^'~\r\n)">) +
                                     lit<"'">)> {};

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

  static constexpr auto OpCompare =
      as<ast::Operator::ne>(lit<"<>">) | as<ast::Operator::le>(lit<"<=">) | as<ast::Operator::ge>(lit<">=">) | as<ast::Operator::eq>(lit<"=">) | as<ast::Operator::lt>(lit<"<">) |
      as<ast::Operator::gt>(lit<">">);
  static constexpr auto OpAdd = as<ast::Operator::add>(lit<"+">) | as<ast::Operator::subtract>(lit<"-">);
  static constexpr auto OpMul = as<ast::Operator::multiply>(lit<"*">) | as<ast::Operator::divide>(lit<"/">);
  static constexpr auto OpPow = as<ast::Operator::power>(lit<"^">);

  // ------------------------------------------------------- paths and names

  struct PathSegment;
  struct Path;
  struct ArrayIndex;
  struct Argument;
  struct Expression;
  struct TypeSpec;

  struct ArrayIndex
      : rule<create<ast::ArrayIndex>(
            lit<"["> +
            many(append<&ast::ArrayIndex::items>(ref<Expression>),
                       lit<",">) +
            lit<"]">)> {};

  struct PathSegment
      : rule<assign<&ast::PathSegment::name>(IdentifierNameEx) +
             option(assign<&ast::PathSegment::index>(ref<ArrayIndex>))> {
  };

  struct Path
      : rule<create<ast::Path>(
            some(append<&ast::Path::segments>(ref<PathSegment>),
                lit<".">))> {};

  struct BindParam
      : rule<create<ast::BindParam>(
            lit<":"> + assign<&ast::BindParam::name>(ref<Path>))> {};

  // -------------------------------------------------------- literal atoms

  struct EnumLit
      : rule<assign<&ast::EnumLit::value>(ref<ENUM>)> {
  };
  struct NumberLit
      : rule<assign<&ast::NumberLit::value>(ref<NUMBER>)> {};
  struct BoolLit
      : rule<create<ast::BoolLit>(
            flag<&ast::BoolLit::value>(ikw<"TRUE">) | ikw<"FALSE">)> {};
  struct DateLit
      : rule<assign<&ast::DateLit::value>(ref<DATE>)> {
  };
  struct TimeLit
      : rule<assign<&ast::TimeLit::value>(ref<TIME>)> {
  };
  struct StringLit
      : rule<create<ast::StringLit>(assign<&ast::StringLit::value>(
            ref<DQUOTED_STRING> | ref<QUOTED_STRING>))> {};

  struct BraceList
      : rule<create<ast::BraceList>(
            lit<"{"> +
            some(append<&ast::BraceList::items>(ref<Expression>),
                lit<",">) +
            lit<"}">)> {};

  struct IsNullTest
      : rule<create<ast::IsNullTest>(
            assign<&ast::IsNullTest::operand>(ref<Path>) + ikw<"IS"> +
            option(flag<&ast::IsNullTest::negated>(ikw<"NOT">)) +
            ikw<"NULL">)> {};

  // `name = :param`: kept out of the general expression atoms (it would
  // clash with `=` comparison on the same span) and reachable only through
  // the SQL SET/WHERE list, `SqlSetItem`, below.
  struct BindAssign
      : rule<create<ast::BindAssign>(
            assign<&ast::BindAssign::target>(ref<Path>) + lit<"="> +
            assign<&ast::BindAssign::value>(ref<BindParam>))> {};

  struct SqlSetItem : rule<ref<IsNullTest> | ref<BindAssign>> {};

  struct WhereClause
      : rule<create<ast::WhereClause>(
            ikw<"WHERE"> +
            (some(append<&ast::WhereClause::setValues>(ref<SqlSetItem>),
                 lit<",">) |
             assign<&ast::WhereClause::cond>(ref<Expression>)))> {};

  // ----------------------------------------------------------- call chain

  struct Argument
      : rule<create<ast::Argument>(
            option(flag<&ast::Argument::byRef>(ikw<"REF">)) +
            assign<&ast::Argument::value>(ref<Expression>))> {};

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
      many(append<&ast::Call::args>(ref<Argument>), lit<",">) +
      lit<")">;

  struct CallHead : rule<create<ast::Call>(CallModifiers + CallTail)> {};

  struct AncestorCall
      : rule<create<ast::Call>(
            flag<&ast::Call::ancestor>(lit<"::">) +
            option(flag<&ast::Call::isEvent>(ikw<"EVENT">)) +
            CallTail)> {};

  struct SuperCall
      : rule<create<ast::Call>(
            ikw<"SUPER"> + flag<&ast::Call::ancestor>(lit<"::">) +
            CallModifiers + CallTail)> {};

  struct SpecialCall
      : rule<create<ast::SpecialCall>(
                 assign<&ast::SpecialCall::keyword>(ikw<"OPEN">) +
                 flag<&ast::SpecialCall::hasParens>(lit<"(">) +
                 some(append<&ast::SpecialCall::args>(ref<Argument>),
                     lit<",">) +
                 lit<")">) |
             create<ast::SpecialCall>(
                 assign<&ast::SpecialCall::keyword>(ikw<"CLOSE">) +
                 flag<&ast::SpecialCall::hasParens>(lit<"(">) +
                 some(append<&ast::SpecialCall::args>(ref<Argument>),
                     lit<",">) +
                 lit<")">) |
             create<ast::SpecialCall>(
                 assign<&ast::SpecialCall::keyword>(ikw<"HALT">) +
                 option(flag<&ast::SpecialCall::haltClose>(ikw<"CLOSE">)))>{
  };

  struct PostfixPrimary
      : rule<create<ast::Path>(append<&ast::Path::segments>(
                 ref<PathSegment>)) |
             ref<CallHead> | ref<AncestorCall> |
             ref<SuperCall>> {};

  struct PostfixChain
      : rule<ref<PostfixPrimary> +
             many(nest<ast::MemberAccess, &ast::MemberAccess::object>(
                      lit<"."> +
                      assign<&ast::MemberAccess::field>(FunctionNameKw) +
                      option(assign<&ast::MemberAccess::index>(
                          ref<ArrayIndex>))) |
                  nest<ast::Call, &ast::Call::receiver>(
                      lit<"."> + CallModifiers + CallTail))> {};

  struct CreateExpr
      : rule<create<ast::CreateExpr>(
            ikw<"CREATE">+
            ((ikw<"USING"> +
              assign<&ast::CreateExpr::usingExpr>(ref<Expression>)) |
             (option(ikw<"USING">) +
              option(assign<&ast::CreateExpr::qualifier>(ref<Path>) +
                     lit<".">) +
              assign<&ast::CreateExpr::type>(ref<TypeSpec>) +
              option(flag<&ast::CreateExpr::hasParens>(lit<"(">) +
                     many(
                         append<&ast::CreateExpr::args>(ref<Argument>),
                         lit<",">) +
                     lit<")">))))> {};

  struct DescribeExpr
      : rule<create<ast::DescribeExpr>(
                 option(assign<&ast::DescribeExpr::qualifier>(ref<Path>) +
                        lit<".">) +
                 ikw<"DESCRIBE"> + lit<"("> +
                 many(
                     append<&ast::DescribeExpr::args>(ref<Argument>),
                     lit<",">) +
                 lit<")">) |
             create<ast::DescribeExpr>(
                 ikw<"DESCRIBE"> +
                 assign<&ast::DescribeExpr::target>(ref<Path>) +
                 ikw<"INTO"> +
                 assign<&ast::DescribeExpr::into>(ref<Path>))> {};

  // -------------------------------------------------------------- atoms

  struct Atom
      : rule<(lit<"("> + ref<Expression> + lit<")">) |
             ref<BraceList> | ref<IsNullTest> | ref<EnumLit> |
             ref<NumberLit> | ref<BoolLit> | ref<DateLit> |
             ref<TimeLit> | ref<StringLit> | ref<BindParam> |
             ref<CreateExpr> | ref<DescribeExpr> |
             ref<SpecialCall> | ref<PostfixChain>> {};

  struct UnarySign
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-">) +
                 assign<&ast::UnaryExpr::operand>(ref<Atom>)) |
             ref<Atom>> {};

  struct ArithExpr
      : infix<UnarySign, ast::BinaryExpr,
              levels(left(OpPow), left(OpMul), left(OpAdd))> {};

  struct ComparisonExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<ArithExpr>) +
                 assign<&ast::BinaryExpr::op>(OpCompare) +
                 assign<&ast::BinaryExpr::right>(ref<ArithExpr>)) |
             ref<ArithExpr>> {};

  struct NotExpr
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ikw<"NOT">) +
                 assign<&ast::UnaryExpr::operand>(ref<ComparisonExpr>)) |
             ref<ComparisonExpr>> {};

  struct Expression
      : infix<NotExpr, ast::BinaryExpr,
              levels(left(as<ast::Operator::logical_and>(ikw<"AND">)), left(as<ast::Operator::logical_or>(ikw<"OR">)))> {};

  // ------------------------------------------------------------- typing

  struct TypeSpec
      : rule<create<ast::TypeSpec>(
            assign<&ast::TypeSpec::name>(DataTypeKw | ref<ID>) +
            option(lit<"{"> + assign<&ast::TypeSpec::scale>(ref<ConvertedNumber>) +
                   lit<"}">))> {};
  static constexpr auto TypeSpecText =
      (DataTypeKw | ref<ID>) + option(lit<"{"> + ref<NUMBER> + lit<"}">);

  struct SignedNumberText
      : rule<option(lit<"+"> | lit<"-">) + ref<NUMBER>> {};

  struct ArrayBound
      : rule<create<ast::ArrayBound>(
            assign<&ast::ArrayBound::from>(ref<SignedNumberText>) +
            option(ikw<"TO"> + assign<&ast::ArrayBound::to>(
                                    ref<SignedNumberText>)))> {};

  struct ArrayDecl
      : rule<create<ast::ArrayDecl>(
            lit<"["> +
            many(append<&ast::ArrayDecl::bounds>(ref<ArrayBound>),
                       lit<",">) +
            lit<"]">)> {};

  struct Parameter
      : rule<create<ast::Parameter>(
            option(flag<&ast::Parameter::readOnly>(ikw<"READONLY">)) +
            option(flag<&ast::Parameter::byRef>(ikw<"REF">)) +
            assign<&ast::Parameter::type>(ref<TypeSpec>) +
            assign<&ast::Parameter::name>(ref<ID>) +
            option(assign<&ast::Parameter::array>(ref<ArrayDecl>)))> {};


  // ------------------------------------------------------- declarations

  struct AssignStmt;

  struct Declarator
      : rule<assign<&ast::Declarator::name>(ref<ID>) +
             option(assign<&ast::Declarator::array>(ref<ArrayDecl>))> {};

  // The four variable_decl_sub shapes and the modifiers ahead of them all
  // write directly into the enclosing ast::VariableDecl: spliced as plain
  // expressions, never their own `rule<>` (that would give the alternative a
  // second, competing head).

  // variable_decl_sub0: a shared type, one or more names, one shared
  // initializer for the whole list.
  static constexpr auto VarSub0 =
      assign<&ast::VariableDecl::type>(ref<TypeSpec>) +
      some(append<&ast::VariableDecl::declarators>(ref<Declarator>),
          lit<",">) +
      option(lit<"="> +
             assign<&ast::VariableDecl::sharedInit>(ref<Expression>));

  // variable_decl_sub2: a shared type, exactly one name, an optional
  // brace-list initializer attached to that one name.
  static constexpr auto VarSub2 =
      assign<&ast::VariableDecl::type>(ref<TypeSpec>) +
      append<&ast::VariableDecl::declarators>(create<ast::Declarator>(
          assign<&ast::Declarator::name>(IdentifierNameEx) +
          option(assign<&ast::Declarator::array>(ref<ArrayDecl>)) +
          option(option(lit<"=">) +
                 assign<&ast::Declarator::init>(ref<BraceList>))));

  // variable_decl_sub1: a shared type, one or more chained assignments.
  static constexpr auto VarSub1 =
      assign<&ast::VariableDecl::type>(ref<TypeSpec>) +
      append<&ast::VariableDecl::assigns>(ref<AssignStmt>) +
      many(lit<","> + option(TypeSpecText) +
           append<&ast::VariableDecl::assigns>(ref<AssignStmt>));

  // `EVENT name(args)` is an event call or a forward declaration, never this.
  static constexpr auto VarEvent =
      flag<&ast::VariableDecl::isEventForm>(ikw<"EVENT">) +
      assign<&ast::VariableDecl::eventName>(ref<Path>);

  static constexpr auto VariableDeclSub =
      option(flag<&ast::VariableDecl::indirect>(ikw<"INDIRECT">)) +
      option(assign<&ast::VariableDecl::accessModifPart>(
          AccessModifPartKw)) +
      option(assign<&ast::VariableDecl::scope>(ScopeModifKw)) +
      (VarSub0 | VarSub1 | VarSub2 | VarEvent);

  struct VariableDecl
      : rule<create<ast::VariableDecl>(
            option(assign<&ast::VariableDecl::accessType>(AccessTypeKw)) +
            VariableDeclSub) +
            option(lit<";">)> {};

  struct ConstantDecl
      : rule<create<ast::VariableDecl>(
                 option(assign<&ast::VariableDecl::accessType>(
                     AccessTypeKw)) +
                 flag<&ast::VariableDecl::isConstant>(ikw<"CONSTANT">) +
                 VariableDeclSub) +
             option(lit<";">)> {};

  struct AccessMarker
      : rule<create<ast::AccessMarker>(
            assign<&ast::AccessMarker::kind>(AccessTypeKw) + lit<":">)> {};

  // ---------------------------------------------------------- statements

  struct AtAssignStmt
      : rule<create<ast::AtAssignStmt>(
            lit<"@"> + assign<&ast::AtAssignStmt::target>(ref<Path>) +
            lit<"="> +
            assign<&ast::AtAssignStmt::value>(ref<BindParam>) +
            lit<";">)> {};

  static constexpr auto AssignOp =
      lit<"="> | lit<"+="> | lit<"-="> | lit<"*="> | lit<"/=">;

  struct AssignStmt
      : rule<create<ast::AssignStmt>(
            assign<&ast::AssignStmt::target>(ref<PostfixChain>) +
            assign<&ast::AssignStmt::op>(AssignOp) +
            some(append<&ast::AssignStmt::rhs>(ref<Expression>),
                lit<",">) +
            option(lit<";">))> {};

  struct IncDecOp
      : rule<(lit<"+"> + lit<"+">) | (lit<"-"> + lit<"-">)> {};

  struct IncDecStmt
      : rule<create<ast::IncDecStmt>(
            assign<&ast::IncDecStmt::target>(ref<Path>) +
            assign<&ast::IncDecStmt::op>(ref<IncDecOp>))> {};

  struct ExprStmt
      : rule<create<ast::ExprStmt>(assign<&ast::ExprStmt::value>(
            ref<PostfixChain> | ref<SpecialCall> |
            ref<CreateExpr> | ref<DescribeExpr>))> {};

  // `CALL name(args)`, `CALL name::CREATE`, and the everyday
  // `CALL SUPER::eventname` (no parens at all -- the corpus's most common
  // shape, an ancestor's version of the handler in hand): call_statement and
  // super_call_statement's two CALL-prefixed forms.
  struct CallStmt
      : rule<create<ast::Call>(
            ikw<"CALL"> +
            ((flag<&ast::Call::ancestor>(ikw<"SUPER">) + lit<"::"> +
              (flag<&ast::Call::isEvent>(ikw<"EVENT">) |
               assign<&ast::Call::name>(ikw<"CREATE"> | ikw<"DESTROY"> |
                                        ikw<"OPEN"> | ikw<"CLOSE"> |
                                        IdentifierNameEx)) +
              option(flag<&ast::Call::hasParens>(lit<"(">) +
                     many(
                         append<&ast::Call::args>(ref<Argument>),
                         lit<",">) +
                     lit<")">)) |
             (option(assign<&ast::Call::receiver>(ref<Path>) +
                     lit<"`">) +
              assign<&ast::Call::name>(ref<ID> | ikw<"DESCRIBE">) +
              option((flag<&ast::Call::ancestor>(lit<"::">) +
                      assign<&ast::Call::ancestorMethod>(ikw<"CREATE"> | ikw<"DESTROY"> | ikw<"OPEN"> |
                       ikw<"CLOSE"> | IdentifierNameEx)) |
                     (flag<&ast::Call::hasParens>(lit<"(">) +
                      many(
                          append<&ast::Call::args>(ref<Argument>),
                          lit<",">) +
                      lit<")">)))) +
            option(lit<";">))> {};

  struct DescribeStmt
      : rule<create<ast::DescribeStmt>(
            ikw<"DESCRIBE"> +
            assign<&ast::DescribeStmt::name>(ref<Path>))> {};

  struct DestroyStmt
      : rule<create<ast::DestroyStmt>(
            ikw<"DESTROY"> +
            assign<&ast::DestroyStmt::value>(ref<Expression>))> {};

  struct ThrowStmt
      : rule<create<ast::ThrowStmt>(
            ikw<"THROW"> +
            assign<&ast::ThrowStmt::value>(ref<Expression>))> {};

  struct ReturnStmt
      : rule<create<ast::ReturnStmt>(
            ikw<"RETURN"> +
            option(((!gap::line) | gap::has(lit<"&"> + many(cr<R"( \t)">) + eol)) +
                   assign<&ast::ReturnStmt::value>(ref<Expression>)))> {
  };

  struct ContinueStmt : rule<create<ast::ContinueStmt> + ikw<"CONTINUE">> {
  };
  struct ExitStmt : rule<create<ast::ExitStmt> + ikw<"EXIT">> {};

  struct GotoStmt
      : rule<create<ast::GotoStmt>(
            ikw<"GOTO"> +
            assign<&ast::GotoStmt::target>(ref<Path>))> {};

  struct LabelStmt
      : rule<create<ast::LabelStmt>(
            assign<&ast::LabelStmt::name>(ref<ID>) + lit<":">)> {};

  struct Statement;

  struct StmtLine : rule<ref<Statement> + option(lit<";">)> {};

  struct ElseIf
      : rule<create<ast::ElseIf>(
            ikw<"ELSEIF"> +
            assign<&ast::ElseIf::cond>(ref<Expression>) + ikw<"THEN"> +
            many(append<&ast::ElseIf::body>(ref<StmtLine>)))> {};

  struct IfBlockStmt
      : rule<create<ast::IfStmt>(
            ikw<"IF"> + assign<&ast::IfStmt::cond>(ref<Expression>) +
            ikw<"THEN"> +
            many(append<&ast::IfStmt::thenBody>(ref<StmtLine>)) +
            many(append<&ast::IfStmt::elseIfs>(ref<ElseIf>)) +
            option(ikw<"ELSE"> +
                   many(append<&ast::IfStmt::elseBody>(ref<StmtLine>))) +
            flag<&ast::IfStmt::blockForm>(ikw<"END"> + ikw<"IF">) +
            option(lit<";">))> {};

  struct IfSimpleStmt
      : rule<create<ast::IfStmt>(
            ikw<"IF"> + assign<&ast::IfStmt::cond>(ref<Expression>) +
            ikw<"THEN"> +
            append<&ast::IfStmt::thenBody>(ref<Statement>) +
            option(ikw<"ELSE"> +
                   append<&ast::IfStmt::elseBody>(ref<Statement>)) +
            option(lit<";">))> {};

  struct CaseValueRange
      : rule<create<ast::CaseValueRange>(
            assign<&ast::CaseValueRange::from>(ref<Expression>) +
            option(ikw<"TO"> + assign<&ast::CaseValueRange::to>(
                                    ref<Expression>)))> {};

  struct ChooseCaseValue
      : rule<create<ast::ChooseCaseValue>(
            ikw<"CASE"> +
            some(append<&ast::ChooseCaseValue::ranges>(
                     ref<CaseValueRange>),
                lit<",">) +
            many(append<&ast::ChooseCaseValue::body>(ref<StmtLine>)))> {
  };

  struct ChooseCaseCond
      : rule<create<ast::ChooseCaseCond>(
            ikw<"CASE"> + ikw<"IS"> +
            assign<&ast::ChooseCaseCond::op>(OpCompare) +
            assign<&ast::ChooseCaseCond::value>(ref<Expression>) +
            many(append<&ast::ChooseCaseCond::body>(ref<StmtLine>)))> {};

  struct ChooseCaseElse
      : rule<create<ast::ChooseCaseElse>(
            ikw<"CASE"> + ikw<"ELSE"> +
            many(append<&ast::ChooseCaseElse::body>(ref<StmtLine>)))> {};

  struct ChooseStmt
      : rule<create<ast::ChooseStmt>(
            ikw<"CHOOSE"> + ikw<"CASE"> +
            assign<&ast::ChooseStmt::value>(ref<Expression>) +
            some(append<&ast::ChooseStmt::cases>(ref<ChooseCaseCond>) |
                 append<&ast::ChooseStmt::cases>(ref<ChooseCaseElse>) |
                 append<&ast::ChooseStmt::cases>(ref<ChooseCaseValue>)) +
            ikw<"END"> + ikw<"CHOOSE">)> {};

  struct ForStmt
      : rule<create<ast::ForStmt>(
            ikw<"FOR"> + assign<&ast::ForStmt::var>(ref<Path>) +
            lit<"="> + assign<&ast::ForStmt::from>(ref<Expression>) +
            ikw<"TO"> + assign<&ast::ForStmt::to>(ref<Expression>) +
            option(ikw<"STEP"> +
                   assign<&ast::ForStmt::step>(ref<Expression>)) +
            many(append<&ast::ForStmt::body>(ref<StmtLine>)) +
            (ikw<"NEXT"> | (ikw<"END"> + ikw<"FOR">)))> {};

  struct DoWhileStmt
      : rule<create<ast::DoWhileStmt>(
            flag<&ast::DoWhileStmt::testFirst>(ikw<"DO">) +
            (ikw<"WHILE"> |
             flag<&ast::DoWhileStmt::isUntil>(ikw<"UNTIL">)) +
            assign<&ast::DoWhileStmt::cond>(ref<Expression>) +
            many(append<&ast::DoWhileStmt::body>(ref<StmtLine>)) +
            ikw<"LOOP">)> {};

  struct DoLoopWhileStmt
      : rule<create<ast::DoWhileStmt>(
            ikw<"DO"> +
            many(append<&ast::DoWhileStmt::body>(ref<StmtLine>)) +
            ikw<"LOOP"> +
            (ikw<"WHILE"> |
             flag<&ast::DoWhileStmt::isUntil>(ikw<"UNTIL">)) +
            assign<&ast::DoWhileStmt::cond>(ref<Expression>))> {};

  struct CatchClause
      : rule<create<ast::CatchClause>(
            ikw<"CATCH"> + lit<"("> +
            assign<&ast::CatchClause::exceptionVar>(
                create<ast::VariableDecl>(VariableDeclSub)) +
            lit<")"> +
            many(append<&ast::CatchClause::body>(ref<StmtLine>)))> {};

  struct TryStmt
      : rule<create<ast::TryStmt>(
            ikw<"TRY"> +
            many(append<&ast::TryStmt::body>(ref<StmtLine>)) +
            many(append<&ast::TryStmt::catches>(ref<CatchClause>)) +
            option(flag<&ast::TryStmt::hasFinally>(ikw<"FINALLY">) +
                   many(append<&ast::TryStmt::finallyBody>(
                       ref<StmtLine>))) +
            ikw<"END"> + ikw<"TRY">)> {};

  // ---------------------------------------------------------------- SQL

  struct SqlInsertStmt
      : rule<create<ast::SqlInsertStmt>(
            ikw<"INSERT"> + ikw<"INTO"> +
            assign<&ast::SqlInsertStmt::table>(ref<Path>) + lit<"("> +
            some(append<&ast::SqlInsertStmt::columns>(ref<Path>),
                lit<",">) +
            lit<")"> + ikw<"VALUES"> + lit<"("> +
            some(append<&ast::SqlInsertStmt::values>(
                     ref<Expression> | ref<BindParam>),
                lit<",">) +
            lit<")"> + option(lit<";">))> {};

  struct SqlDeleteStmt
      : rule<create<ast::SqlDeleteStmt>(
            ikw<"DELETE"> + ikw<"FROM"> +
            assign<&ast::SqlDeleteStmt::table>(ref<Path>) +
            assign<&ast::SqlDeleteStmt::where>(ref<WhereClause>) +
            option(lit<";">))> {};

  struct SqlSelectStmt
      : rule<create<ast::SqlSelectStmt>(
            (ikw<"SELECT"> |
             flag<&ast::SqlSelectStmt::isBlob>(ikw<"SELECTBLOB">)) +
            (some(append<&ast::SqlSelectStmt::columns>(ref<Path>),
                 lit<",">) |
             assign<&ast::SqlSelectStmt::funcCall>(ref<CallHead>)) +
            ikw<"INTO"> +
            some(append<&ast::SqlSelectStmt::into>(ref<BindParam>),
                lit<",">) +
            ikw<"FROM"> +
            some(append<&ast::SqlSelectStmt::from>(ref<Path>), lit<",">) +
            option(assign<&ast::SqlSelectStmt::where>(ref<WhereClause>)) +
            option(ikw<"USING"> +
                   assign<&ast::SqlSelectStmt::usingConn>(ref<Path>)) +
            option(lit<";">))> {};

  struct SqlUpdateStmt
      : rule<create<ast::SqlUpdateStmt>(
            (ikw<"UPDATE"> |
             flag<&ast::SqlUpdateStmt::isBlob>(ikw<"UPDATEBLOB">)) +
            assign<&ast::SqlUpdateStmt::table>(ref<Path>) + ikw<"SET"> +
            some(append<&ast::SqlUpdateStmt::setValues>(ref<SqlSetItem>),
                lit<",">) +
            option(
                assign<&ast::SqlUpdateStmt::where>(ref<WhereClause>)))> {
  };

  struct SqlConnectStmt
      : rule<create<ast::SqlConnectStmt>(
            assign<&ast::SqlConnectStmt::keyword>(
                ikw<"CONNECT"> | ikw<"DISCONNECT"> | ikw<"ROLLBACK">) +
            option(flag<&ast::SqlConnectStmt::usingClause>(
                       ikw<"USING">) +
                   (flag<&ast::SqlConnectStmt::usingSqlca>(
                        ikw<"SQLCA">) |
                    assign<&ast::SqlConnectStmt::usingName>(ref<Path>))) +
            lit<";">)> {};

  struct SqlCommitStmt
      : rule<create<ast::SqlCommitStmt>(
            ikw<"COMMIT"> +
            option(flag<&ast::SqlCommitStmt::usingClause>(
                ikw<"USING">)) +
            option(flag<&ast::SqlCommitStmt::usingSqlca>(ikw<"SQLCA">) |
                   assign<&ast::SqlCommitStmt::usingName>(ref<Path>)) +
            option(lit<";">))> {};

  struct ExecuteStmt
      : rule<create<ast::ExecuteStmt>(
            ikw<"EXECUTE"> +
            ((option(flag<&ast::ExecuteStmt::immediate>(
                 ikw<"IMMEDIATE">)) +
                 ((assign<&ast::ExecuteStmt::value>(
                       ref<Path> | ref<Atom>) +
                   option(lit<";">)) |
                  (assign<&ast::ExecuteStmt::value>(ref<BindParam>) +
                   option(ikw<"USING"> +
                          (flag<&ast::ExecuteStmt::usingSqlca>(
                               ikw<"SQLCA">) |
                           assign<&ast::ExecuteStmt::usingName>(
                               ref<Path>))) +
                   lit<";">))) |
             (option(flag<&ast::ExecuteStmt::dynamic>(
                  ikw<"DYNAMIC">)) +
              assign<&ast::ExecuteStmt::identifier>(ref<Path>) +
              option(ikw<"USING"> +
                     option(flag<&ast::ExecuteStmt::usingDescriptor>(
                         ikw<"DESCRIPTOR">)) +
                     (flag<&ast::ExecuteStmt::usingSqlca>(
                          ikw<"SQLCA">) |
                      assign<&ast::ExecuteStmt::usingName>(
                          ref<Path>))) +
              option(lit<";">))))> {};

  struct CloseSqlStmt
      : rule<create<ast::CloseSqlStmt>(
            ikw<"CLOSE"> +
            assign<&ast::CloseSqlStmt::cursor>(ref<Path>) +
            option(lit<";">))> {};

  struct DeclareProcedureStmt
      : rule<create<ast::DeclareProcedureStmt>(
            ikw<"DECLARE"> +
            assign<&ast::DeclareProcedureStmt::handle>(ref<Path>) +
            option(flag<&ast::DeclareProcedureStmt::dynamic>(
                ikw<"DYNAMIC">)) +
            ikw<"PROCEDURE"> + ikw<"FOR"> +
            assign<&ast::DeclareProcedureStmt::procName>(ref<Path>) +
            option(lit<";">))> {};

  struct DeclareCursorStmt
      : rule<create<ast::DeclareCursorStmt>(
            ikw<"DECLARE"> +
            assign<&ast::DeclareCursorStmt::cursor>(ref<Path>) +
            option(flag<&ast::DeclareCursorStmt::dynamic>(
                ikw<"DYNAMIC">)) +
            ikw<"CURSOR"> + ikw<"FOR"> +
            assign<&ast::DeclareCursorStmt::forName>(ref<Path>) +
            lit<";">)> {};

  struct OpenCursorStmt
      : rule<create<ast::OpenCursorStmt>(
            ikw<"OPEN"> +
            option(flag<&ast::OpenCursorStmt::dynamic>(
                ikw<"DYNAMIC">)) +
            assign<&ast::OpenCursorStmt::cursor>(ref<Path>) +
            option(ikw<"USING"> +
                   (flag<&ast::OpenCursorStmt::usingDescriptor>(
                        ikw<"DESCRIPTOR">) |
                    assign<&ast::OpenCursorStmt::usingName>(
                        ref<Path>))) +
            option(assign<&ast::OpenCursorStmt::trailingIdent>(
                ref<Path>)) +
            option(lit<";">))> {};

  struct FetchStmt
      : rule<create<ast::FetchStmt>(
            ikw<"FETCH"> +
            ((assign<&ast::FetchStmt::cursor>(ref<Path>) + ikw<"INTO"> +
              assign<&ast::FetchStmt::into>(ref<BindParam>)) |
             (assign<&ast::FetchStmt::usingIdent>(ref<Path>) +
              flag<&ast::FetchStmt::usingDescriptorForm>(ikw<"USING">) +
              option(flag<&ast::FetchStmt::usingDescriptor>(ikw<"DESCRIPTOR">)) +
              assign<&ast::FetchStmt::usingTarget>(ref<Path>))) +
            option(lit<";">))> {};

  struct PrepareStmt
      : rule<create<ast::PrepareStmt>(
            ikw<"PREPARE"> +
            assign<&ast::PrepareStmt::stmt>(ref<Path>) + ikw<"FROM"> +
            assign<&ast::PrepareStmt::from>(ref<BindParam>) +
            ikw<"USING"> +
            assign<&ast::PrepareStmt::usingName>(ref<Path>) + lit<";">)>{
  };

  // ------------------------------------------------------------ statement

  struct Statement
      : rule<ref<IncDecStmt> | ref<AccessMarker> |
             ref<IfSimpleStmt> | ref<ExecuteStmt> |
             ref<ThrowStmt> | ref<DescribeStmt> |
             ref<AtAssignStmt> | ref<AssignStmt> | ref<TryStmt> |
             ref<CloseSqlStmt> | ref<IfBlockStmt> |
             ref<VariableDecl> | ref<ConstantDecl> |
             ref<DoLoopWhileStmt> | ref<DoWhileStmt> |
             ref<DestroyStmt> | ref<LabelStmt> | ref<GotoStmt> |
             ref<ChooseStmt> | ref<ReturnStmt> | ref<ForStmt> |
             ref<ContinueStmt> | ref<ExitStmt> |
             ref<SqlInsertStmt> | ref<SqlDeleteStmt> |
             ref<SqlSelectStmt> | ref<SqlUpdateStmt> |
             ref<SqlConnectStmt> | ref<SqlCommitStmt> |
             ref<OpenCursorStmt> | ref<PrepareStmt> |
             ref<DeclareCursorStmt> | ref<DeclareProcedureStmt> |
             ref<FetchStmt> | ref<CallStmt> | ref<ExprStmt>> {};

  // -------------------------------------------------- top-level constructs

  struct DataTypeName
      : rule<create<ast::TypeSpec>(
            assign<&ast::TypeSpec::name>(DataTypeKw | ref<ID>))> {};

  static constexpr auto EventForwardParams =
      some(append<&ast::EventForwardDecl::params>(ref<Parameter>),
          lit<",">) +
      option(lit<","> +
             flag<&ast::EventForwardDecl::varargs>(lit<"...">));

  struct EventForwardDecl
      : rule<create<ast::EventForwardDecl>(
            ikw<"EVENT"> +
            ((assign<&ast::EventForwardDecl::name>(ref<ID> | ikw<"CREATE"> |
                                                    ikw<"DESTROY">) +
              option(assign<&ast::EventForwardDecl::basedOn>(ref<ID>)) +
              option(flag<&ast::EventForwardDecl::hasParens>(
                         lit<"(">) +
                     option(EventForwardParams) + lit<")">)) |
             (ikw<"TYPE"> +
              assign<&ast::EventForwardDecl::type>(ref<DataTypeName>) +
              assign<&ast::EventForwardDecl::name>(ref<ID>) +
              flag<&ast::EventForwardDecl::hasParens>(lit<"(">) +
              option(EventForwardParams) + lit<")">)))> {};

  struct TypeDecl
      : rule<create<ast::TypeDecl>(
            option(assign<&ast::TypeDecl::scope>(ScopeModifKw)) +
            ikw<"TYPE"> + assign<&ast::TypeDecl::name>(ref<ID>) +
            ikw<"FROM"> +
            option(assign<&ast::TypeDecl::libraryQualifier>(ref<ID>) +
                   lit<"`">) +
            assign<&ast::TypeDecl::super_>(ref<DataTypeName>) +
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
            many(append<&ast::TypeDecl::members>(ref<VariableDecl>) |
                 append<&ast::TypeDecl::members>(
                     ref<EventForwardDecl>)) +
            ikw<"END"> + ikw<"TYPE">)> {};

  struct ForwardDecl
      : rule<create<ast::ForwardDecl>(
            ikw<"FORWARD"> +
            some(append<&ast::ForwardDecl::items>(ref<TypeDecl>) |
                 append<&ast::ForwardDecl::items>(ref<VariableDecl>)) +
            ikw<"END"> + ikw<"FORWARD">)> {};

  struct TypeVariablesDecl
      : rule<create<ast::TypeVariablesDecl>(
            ikw<"TYPE"> + ikw<"VARIABLES"> +
            many(append<&ast::TypeVariablesDecl::items>(
                     ref<VariableDecl>) |
                 append<&ast::TypeVariablesDecl::items>(
                     ref<ConstantDecl>) |
                 append<&ast::TypeVariablesDecl::items>(
                     ref<AccessMarker>)) +
            ikw<"END"> + ikw<"VARIABLES">)> {};

  struct GlobalVariablesDecl
      : rule<create<ast::GlobalVariablesDecl>(
                 ikw<"GLOBAL"> +
                 assign<&ast::GlobalVariablesDecl::single>(
                     ref<VariableDecl>)) |
             create<ast::GlobalVariablesDecl>(
                 (ikw<"GLOBAL"> |
                  flag<&ast::GlobalVariablesDecl::shared_>(
                      ikw<"SHARED">)) +
                 ikw<"VARIABLES"> +
                 many(append<&ast::GlobalVariablesDecl::items>(
                     ref<VariableDecl>)) +
                 ikw<"END"> + ikw<"VARIABLES">)> {};

  struct FunctionForwardDecl
      : rule<create<ast::FunctionForwardDecl>(
            option(assign<&ast::FunctionForwardDecl::accessModifPart>(
                AccessModifPartKw)) +
            option(assign<&ast::FunctionForwardDecl::scope>(ScopeModifKw)) +
            ((ikw<"FUNCTION"> +
              assign<&ast::FunctionForwardDecl::returnType>(
                  ref<DataTypeName>)) |
             flag<&ast::FunctionForwardDecl::isSubroutine>(
                 ikw<"SUBROUTINE">)) +
            assign<&ast::FunctionForwardDecl::name>(ref<ID>) + lit<"("> +
            option(some(append<&ast::FunctionForwardDecl::params>(
                            ref<Parameter>),
                       lit<",">) +
                   option(lit<","> +
                          flag<&ast::FunctionForwardDecl::varargs>(
                              lit<"...">))) +
            lit<")"> +
            option(
                (ikw<"ALIAS"> + ikw<"FOR"> +
                 assign<&ast::FunctionForwardDecl::aliasText>(ref<ID>) +
                 ikw<"LIBRARY"> +
                 assign<&ast::FunctionForwardDecl::libraryName>(ref<DQUOTED_STRING> | ref<QUOTED_STRING>)) |
                (ikw<"LIBRARY"> + assign<&ast::FunctionForwardDecl::libraryName>(ref<DQUOTED_STRING> | ref<QUOTED_STRING>) +
                 option(ikw<"ALIAS"> + ikw<"FOR"> +
                        assign<&ast::FunctionForwardDecl::aliasText>(ref<DQUOTED_STRING> | ref<QUOTED_STRING>))) |
                (flag<&ast::FunctionForwardDecl::isRpc>(ikw<"RPCFUNC">) + ikw<"ALIAS"> + ikw<"FOR"> +
                 option(assign<&ast::FunctionForwardDecl::aliasText>(ref<DQUOTED_STRING> | ref<QUOTED_STRING>)) +
                 option(ikw<"THROWS"> + assign<&ast::FunctionForwardDecl::throwsName>(ref<ID>))) |
                (ikw<"THROWS"> + assign<&ast::FunctionForwardDecl::throwsName>(ref<ID>))))> {};

  struct FunctionsForwardDecl
      : rule<create<ast::FunctionsForwardDecl>(
            (flag<&ast::FunctionsForwardDecl::isForward>(
                 ikw<"FORWARD">) |
             ikw<"TYPE">) +
            ikw<"PROTOTYPES"> +
            many(append<&ast::FunctionsForwardDecl::items>(
                ref<FunctionForwardDecl>)) +
            ikw<"END"> + ikw<"PROTOTYPES">)> {};

  struct FunctionBody
      : rule<create<ast::FunctionBody>(
            option(assign<&ast::FunctionBody::accessType>(AccessTypeKw)) +
            option(assign<&ast::FunctionBody::scope>(ScopeModifKw)) +
            ((ikw<"FUNCTION"> +
              assign<&ast::FunctionBody::returnType>(
                  ref<DataTypeName>)) |
             flag<&ast::FunctionBody::isSubroutine>(
                 ikw<"SUBROUTINE">)) +
            assign<&ast::FunctionBody::name>(ref<ID>) + lit<"("> +
            option(some(append<&ast::FunctionBody::params>(ref<Parameter>),
                       lit<",">) +
                   option(lit<","> +
                          flag<&ast::FunctionBody::varargs>(
                              lit<"...">))) +
            lit<")"> +
            option(ikw<"THROWS"> +
                   assign<&ast::FunctionBody::throwsName>(ref<ID>)) +
            option(lit<";">) +
            many(append<&ast::FunctionBody::body>(ref<StmtLine>)) +
            ikw<"END"> + (ikw<"FUNCTION"> | ikw<"SUBROUTINE">))> {};

  struct OnBody
      : rule<create<ast::OnBody>(
            ikw<"ON"> + assign<&ast::OnBody::target>(ref<Path>) +
            option((lit<"."> +
                    assign<&ast::OnBody::suffix>(ikw<"CREATE"> |
                                                 ikw<"DESTROY">)) |
                   assign<&ast::OnBody::suffix>(ikw<"OPEN"> |
                                                ikw<"CLOSE">)) +
            option(lit<";">) +
            many(append<&ast::OnBody::body>(ref<StmtLine>)) +
            ikw<"END"> + ikw<"ON">)> {};

  struct EventBody
      : rule<create<ast::EventBody>(
            ikw<"EVENT"> +
            option(ikw<"TYPE"> +
                   assign<&ast::EventBody::type>(ref<DataTypeName>)) +
            option(assign<&ast::EventBody::qualifier>(ref<ID>) +
                   lit<"::">) +
            assign<&ast::EventBody::name>(ref<ID> | ikw<"OPEN"> |
                                          ikw<"CLOSE">) +
            option(flag<&ast::EventBody::hasParens>(lit<"(">) +
                   option(some(append<&ast::EventBody::params>(
                                   ref<Parameter>),
                              lit<",">) +
                          option(lit<","> +
                                 flag<&ast::EventBody::varargs>(
                                     lit<"...">))) +
                   lit<")">) +
            option(lit<";">) +
            many(append<&ast::EventBody::body>(ref<StmtLine>)) +
            ikw<"END"> + ikw<"EVENT">)> {};

  struct Body
      : rule<ref<TypeDecl> | ref<AccessMarker> |
             ref<ForwardDecl> | ref<TypeVariablesDecl> |
             ref<GlobalVariablesDecl> | ref<VariableDecl> |
             ref<ConstantDecl> | ref<FunctionForwardDecl> |
             ref<FunctionsForwardDecl> | ref<FunctionBody> |
             ref<OnBody> | ref<EventBody>> {};

  struct Document
      : rule<create<ast::Document>(
            option(ikw<"RELEASE"> +
                   assign<&ast::Document::release>(ref<ConvertedNumber>) +
                   lit<";">) +
            some(append<&ast::Document::items>(ref<Body>)))> {};

  using entry = Document;
  using ignored = WS;
  using hidden = list<SlComment, MlComment, LineContinuation, ExportHeader>;
};

}  // namespace powerbuilder
