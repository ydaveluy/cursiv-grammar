// The kuka language: its grammar. KRL keywords are case-insensitive and
// identifiers admit `$` (system variables like $VEL_AXIS), so word_boundary
// is widened to match; NEWLINE is an ordinary mandatory token, not trivia.
#pragma once

#include <cstdint>
#include <limits>
#include <memory_resource>
#include <span>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "kuka/core/ast.hpp"

namespace kuka {

inline std::string_view decode_krl_text(std::string_view body,
                                        std::pmr::memory_resource* mem) {
  if (body.find('\\') == std::string_view::npos) return body;
  const std::span<char> room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    if (body[i] != '\\') {
      room[n++] = body[i];
      continue;
    }
    ++i;
    const char c = body[i];
    switch (c) {
      case 'b': room[n++] = '\b'; break;
      case 't': room[n++] = '\t'; break;
      case 'n': room[n++] = '\n'; break;
      case 'f': room[n++] = '\f'; break;
      case 'r': room[n++] = '\r'; break;
      case '"': room[n++] = '"'; break;
      case '\'': room[n++] = '\''; break;
      case '\\': room[n++] = '\\'; break;
      default: {
        int value = c - '0';
        std::size_t digits = 1;
        while (digits < 3 && i + 1 < body.size() && body[i + 1] >= '0' &&
               body[i + 1] <= '7') {
          ++i;
          value = value * 8 + (body[i] - '0');
          ++digits;
        }
        room[n++] = static_cast<char>(value);
        break;
      }
    }
  }
  return std::string_view(room.data(), n);
}

inline constexpr auto decode_quoted = [](std::string_view t,
                                         std::pmr::memory_resource* mem) {
  return decode_krl_text(t.substr(1, t.size() - 2), mem);
};

inline constexpr auto decode_int = [](std::string_view t)
    -> cursiv::ConversionResult<std::int64_t> {
  using Result = cursiv::ConversionResult<std::int64_t>;
  unsigned base = 10;
  if (t.front() == '\'') {
    base = t[1] == 'H' || t[1] == 'h' ? 16 : 2;
    t = t.substr(2, t.size() - 3);
  }
  std::int64_t value = 0;
  for (const char c : t) {
    const unsigned digit = c >= '0' && c <= '9' ? c - '0' :
        c >= 'a' && c <= 'f' ? c - 'a' + 10 : c - 'A' + 10;
    if (value > (std::numeric_limits<std::int64_t>::max() - digit) / base)
      return Result::err("Integer literal exceeds the AST int64 range");
    value = value * base + digit;
  }
  return Result::ok(value);
};

struct Kuka : cursiv::GrammarSpec {
  static constexpr auto word_boundary = cr<"a-zA-Z0-9_$">;

  struct WS : terminal<some(cr<R"( \t\f)">)> {};
  struct HEADERLINE : terminal<lit<"&"> + many(cr<R"(^\r\n)">) + option(eol)> {};
  struct LINE_COMMENT : terminal<lit<";"> + many(cr<R"(^\r\n)">)> {};
  struct NEWLINE : terminal<option(lit<"\r">) + lit<"\n">> {};

  static constexpr auto reserved = ikw<"AND"> | ikw<"ANIN"> | ikw<"ANOUT"> | ikw<"B_AND"> | ikw<"B_NOT"> | ikw<"B_OR"> | ikw<"B_EXOR"> | ikw<"BOOL"> | ikw<"BRAKE"> | ikw<"C_DIS"> | ikw<"C_ORI"> | ikw<"C_PTP"> | ikw<"C_VEL"> | ikw<"CASE"> | ikw<"CAST_FROM"> | ikw<"CAST_TO"> | ikw<"CHAR"> | ikw<"CIRC_REL"> | ikw<"CIRC"> | ikw<"CONST"> | ikw<"CONTINUE"> | ikw<"DELAY"> | ikw<"DECL"> | ikw<"DEF"> | ikw<"DEFAULT"> | ikw<"DEFDAT"> | ikw<"DEFFCT"> | ikw<"DO"> | ikw<"ELSE"> | ikw<"END"> | ikw<"ENDDAT"> | ikw<"ENDFCT"> | ikw<"ENDFOR"> | ikw<"ENDIF"> | ikw<"ENDLOOP"> | ikw<"ENDSWITCH"> | ikw<"ENDWHILE"> | ikw<"ENUM"> | ikw<"EXIT"> | ikw<"EXT"> | ikw<"EXTFCT"> | ikw<"FALSE"> | ikw<"FOR"> | ikw<"GLOBAL"> | ikw<"GOTO"> | ikw<"HALT"> | ikw<"IF"> | ikw<"IMPORT"> | ikw<"INTERRUPT"> | ikw<"INT"> | ikw<"IS"> | ikw<"LIN_REL"> | ikw<"LIN"> | ikw<"LOOP"> | ikw<"MAXIMUM"> | ikw<"MINIMUM"> | ikw<"NOT"> | ikw<"OR"> | ikw<"PRIO"> | ikw<"PTP_REL"> | ikw<"PTP"> | ikw<"PUBLIC"> | ikw<"REAL"> | ikw<"REPEAT"> | ikw<"RETURN"> | ikw<"SEC"> | ikw<"SIGNAL"> | ikw<"STRUC"> | ikw<"SWITCH"> | ikw<"THEN"> | ikw<"TO"> | ikw<"TRIGGER"> | ikw<"TRUE"> | ikw<"UNTIL"> | ikw<"WAIT"> | ikw<"WHEN"> | ikw<"WHILE"> | ikw<"EXOR">;
  struct IDENTIFIER : terminal<!reserved + cr<"a-zA-Z_$"> + many(cr<"a-zA-Z_$0-9">)> {};

  static constexpr auto EscapeSeq =
      lit<"\\"> + (cr<R"(btnfr"'\\)">                 |
                   (cr<"0-3"> + cr<"0-7"> + cr<"0-7">) |
                   (cr<"0-7"> + cr<"0-7">)            |
                   cr<"0-7">);

  struct CHARLITERAL
      : terminal<as<decode_quoted>(
            lit<"'"> + (EscapeSeq | cr<R"(^'\\\r\n)">)+lit<"'">)> {};
  struct STRINGLITERAL
      : terminal<as<decode_quoted>(
            lit<"\""> + many(EscapeSeq | cr<R"(^"\\\r\n)">) + lit<"\"">)> {};

  static constexpr auto Exponent = cr<"eE"> + option(cr<"+-">) + some(d);
  struct FLOATLITERAL
      : terminal<as<double>((some(d) + lit<"."> + many(d) + option(Exponent)) |
                            (lit<"."> + some(d) + option(Exponent)) |
                            (some(d) + Exponent))> {};

  struct INTLITERAL
      : terminal<as<decode_int>(
            some(d) | (lit<"'"> + cr<"hH"> + some(cr<"0-9a-fA-F">) + lit<"'">) |
            (lit<"'"> + cr<"bB"> + some(cr<"01">) + lit<"'">))> {};

  // Operators, each denoting the constant `op` carries -- longest first
  // inside a level so `==`/`<=`/`>=`/`<>` never lex as `=`/`<`/`>`.
  struct OpGeom : terminal<as<ast::Op::geom>(lit<":">)> {};
  // An infix table's operator terminals belong to that one table across the
  // whole grammar, so the motion-target ladder below needs its own type for
  // `:` even though it spells and means the same as OpGeom.

  static constexpr auto RelOp =
      as<ast::Op::eq>(lit<"==">) | as<ast::Op::ne>(lit<"<>">) | as<ast::Op::le>(lit<"<=">) | as<ast::Op::ge>(lit<">=">) | as<ast::Op::lt>(lit<"<">) | as<ast::Op::gt>(lit<">">);
  static constexpr auto OrOp = as<ast::Op::log_or>(ikw<"OR">) | as<ast::Op::bit_or>(ikw<"B_OR">);
  static constexpr auto ExorOp = as<ast::Op::log_exor>(ikw<"EXOR">) | as<ast::Op::bit_exor>(ikw<"B_EXOR">);
  static constexpr auto AndOp = as<ast::Op::log_and>(ikw<"AND">) | as<ast::Op::bit_and>(ikw<"B_AND">);
  static constexpr auto AddOp = as<ast::Op::add>(lit<"+">) | as<ast::Op::sub>(lit<"-">);
  static constexpr auto MulOp = as<ast::Op::mul>(lit<"*">) | as<ast::Op::div>(lit<"/">);
  static constexpr auto GeomOp = ref<OpGeom>;
  static constexpr auto NotOp = ikw<"NOT"> | ikw<"B_NOT">;
  static constexpr auto PrimitiveKw =
      ikw<"BOOL"> | ikw<"CHAR"> | ikw<"INT"> | ikw<"REAL">;

  // Mutually referencing: the expression ladder bottoms out at Primary,
  // which parenthesizes a full AssignmentExpr, which climbs the ladder again.
  struct Expression;
  struct GeomExpr;
  struct UnaryNot;
  struct UnarySign;
  struct Primary;
  struct AssignmentExpr;
  struct VarName;
  struct VarRef;
  struct Literal;
  struct StmtRule;

  struct Expression
      : infix<UnaryNot, ast::BinaryExpr,
              levels(left(GeomOp),  //
                     left(MulOp),   //
                     left(AddOp),   //
                     left(AndOp),   //
                     left(ExorOp),  //
                     left(OrOp),    //
                     left(RelOp))> {};

  // A motion target climbs only this far -- unary +/-/NOT and `:` -- though
  // a parenthesized operand still reaches Primary's paren branch and the
  // full Expression inside it.
  struct GeomExpr
      : infix<UnaryNot, ast::BinaryExpr, left(as<ast::Op::geom>(lit<":">))> {};

  struct UnaryNot
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(NotOp) +
                 assign<&ast::UnaryExpr::operand>(ref<UnaryNot>)) |
             ref<UnarySign>> {};

  struct UnarySign
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-">) +
                 assign<&ast::UnaryExpr::operand>(ref<UnarySign>)) |
             ref<Primary>> {};

  struct Primary
      : rule<(lit<"("> + ref<AssignmentExpr> + lit<")">) | ref<VarRef> |
             ref<Literal>> {};

  struct AssignmentExpr
      : rule<create<ast::Assign>(assign<&ast::Assign::target>(ref<Expression>) +
                                 lit<"="> +
                                 assign<&ast::Assign::value>(
                                     ref<AssignmentExpr>)) |
             ref<Expression>> {};

  struct VarName
      : rule<assign<&ast::VarName::id>(ref<IDENTIFIER>) +
             option(
                 lit<"["> +
                 option(assign<&ast::VarName::index0>(ref<Expression>) +
                        option(lit<","> +
                               option(assign<&ast::VarName::index1>(
                                          ref<Expression>) +
                                      option(lit<","> +
                                             option(assign<&ast::VarName::index2>(
                                                 ref<Expression>)))))) +
                 lit<"]">)> {};

  struct VarRef
      : rule<create<ast::VarRef>(
            some(append<&ast::VarRef::path>(ref<VarName>), lit<".">) +
            option(flag<&ast::VarRef::called>(lit<"(">) +
                   option(some(append<&ast::VarRef::args>(ref<AssignmentExpr>),
                               lit<",">)) +
                   lit<")">))> {};

  struct EnumLit
      : rule<create<ast::EnumLit>(lit<"#"> +
                                  assign<&ast::EnumLit::name>(ref<IDENTIFIER>))> {
  };
  struct IntLit : rule<assign<&ast::IntLit::value>(ref<INTLITERAL>)> {};
  struct FloatLit
      : rule<assign<&ast::FloatLit::value>(ref<FLOATLITERAL>)> {};
  struct CharLit : rule<assign<&ast::CharLit::value>(ref<CHARLITERAL>)> {};
  struct StringLit
      : rule<assign<&ast::StringLit::value>(ref<STRINGLITERAL>)> {};
  struct BoolLit
      : rule<create<ast::BoolLit> +
             (flag<&ast::BoolLit::value>(ikw<"TRUE">) | ikw<"FALSE">)> {};

  struct StructElement
      : rule<assign<&ast::StructElement::name>(ref<VarName>) +
             assign<&ast::StructElement::value>(ref<UnarySign>)> {};
  struct StructLit
      : rule<create<ast::StructLit>(
            lit<"{"> +
            option(assign<&ast::StructLit::typeName>(ref<IDENTIFIER>) +
                   lit<":">) +
            some(append<&ast::StructLit::elements>(ref<StructElement>),
                 lit<",">) +
            lit<"}">)> {};

  struct Literal
      : rule<ref<IntLit> | ref<FloatLit> | ref<CharLit> |
             ref<StringLit> | ref<StructLit> | ref<BoolLit> |
             ref<EnumLit>> {};

  struct TypeRef
      : rule<assign<&ast::TypeRef::name>(PrimitiveKw | ref<IDENTIFIER>) +
             option(flag<&ast::TypeRef::isArray>(lit<"[">) +
                    option(assign<&ast::TypeRef::arraySize>(ref<INTLITERAL>)) +
                    lit<"]">)> {};

  struct ParamType
      : rule<assign<&ast::ParamType::type>(ref<TypeRef>) +
             option(lit<":"> +
                    assign<&ast::ParamType::callType>(ref<IDENTIFIER>))> {};

  struct ForwardDecl
      : rule<create<ast::ForwardDecl>(
            (ikw<"EXT"> + assign<&ast::ForwardDecl::name>(ref<IDENTIFIER>) +
             lit<"("> +
             option(some(append<&ast::ForwardDecl::params>(ref<ParamType>),
                         lit<",">)) +
             lit<")">) |
            (flag<&ast::ForwardDecl::isFunction>(ikw<"EXTFCT">) +
             assign<&ast::ForwardDecl::returnType>(ref<TypeRef>) +
             assign<&ast::ForwardDecl::name>(ref<IDENTIFIER>) + lit<"("> +
             option(some(append<&ast::ForwardDecl::params>(ref<ParamType>),
                         lit<",">)) +
             lit<")">))> {};

  struct SignalDecl
      : rule<create<ast::SignalDecl>(
            ikw<"SIGNAL"> + assign<&ast::SignalDecl::name>(ref<IDENTIFIER>) +
            assign<&ast::SignalDecl::address>(ref<Primary>) +
            option(ikw<"TO"> +
                   assign<&ast::SignalDecl::to>(ref<Primary>)))> {};

  // A routine-local declaration: an optional DECL, then a typed name list
  // or a SIGNAL line -- GLOBAL/CONST and an initialiser are DEFDAT-only.
  struct VarDecl
      : rule<option(ikw<"DECL">) +
             (create<ast::TypedDecl>(
                  assign<&ast::TypedDecl::type>(ref<TypeRef>) +
                  some(append<&ast::TypedDecl::names>(ref<VarName>), lit<",">)) |
              ref<SignalDecl>)> {};

  // A DEFDAT declaration: DECL, optional GLOBAL/CONST, then a typed name
  // list or a single initialised name, or a SIGNAL line.
  struct DataVarDecl
      : rule<option(ikw<"DECL">) +
             ((option(flag<&ast::TypedDecl::global>(ikw<"GLOBAL">)) +
               option(flag<&ast::TypedDecl::isConst>(ikw<"CONST">)) +
               create<ast::TypedDecl>(
                   assign<&ast::TypedDecl::type>(ref<TypeRef>) +
                   append<&ast::TypedDecl::names>(ref<VarName>) +
                   (many(lit<","> + append<&ast::TypedDecl::names>(
                                        ref<VarName>)) |
                    (lit<"="> +
                     assign<&ast::TypedDecl::init>(ref<UnarySign>))))) |
              (option(flag<&ast::SignalDecl::global>(ikw<"GLOBAL">)) +
               option(flag<&ast::SignalDecl::isConst>(ikw<"CONST">)) +
               create<ast::SignalDecl>(
                   ikw<"SIGNAL"> +
                   assign<&ast::SignalDecl::name>(ref<IDENTIFIER>) +
                   assign<&ast::SignalDecl::address>(ref<Primary>) +
                   option(ikw<"TO"> +
                          assign<&ast::SignalDecl::to>(ref<Primary>)))))> {};

  struct FieldGroup
      : rule<assign<&ast::FieldGroup::type>(ref<TypeRef>) +
             some(append<&ast::FieldGroup::names>(ref<VarName>), lit<",">)> {
  };
  struct StructDef
      : rule<option(flag<&ast::StructDef::global>(ikw<"GLOBAL">)) +
             ikw<"STRUC"> + assign<&ast::StructDef::name>(ref<IDENTIFIER>) +
             some(append<&ast::StructDef::groups>(ref<FieldGroup>), lit<",">)> {};

  struct EnumDef
      : rule<option(flag<&ast::EnumDef::global>(ikw<"GLOBAL">)) +
             ikw<"ENUM"> + assign<&ast::EnumDef::name>(ref<IDENTIFIER>) +
             some(append<&ast::EnumDef::values>(ref<IDENTIFIER>), lit<",">)> {
  };

  static constexpr auto ArrayIndices =
      option(assign<&ast::ArrayInit::index0>(ref<Expression>) +
             option(lit<","> +
                    option(assign<&ast::ArrayInit::index1>(ref<Expression>) +
                           option(lit<","> +
                                  option(assign<&ast::ArrayInit::index2>(
                                      ref<Expression>))))));
  struct ArrayInit
      : rule<create<ast::ArrayInit>(
            assign<&ast::ArrayInit::name>(ref<IDENTIFIER>) + lit<"["> +
            ArrayIndices + lit<"]"> + lit<"="> +
            assign<&ast::ArrayInit::value>(ref<UnarySign>))> {};

  struct ImportDecl
      : rule<create<ast::ImportDecl>(
            ikw<"IMPORT"> + assign<&ast::ImportDecl::type>(ref<TypeRef>) +
            assign<&ast::ImportDecl::localName>(ref<VarName>) +
            ikw<"IS"> + lit<"/R1/"> +
            assign<&ast::ImportDecl::moduleName>(ref<IDENTIFIER>) +
            lit<".."> +
            assign<&ast::ImportDecl::remoteName>(ref<VarName>))> {};

  struct Param
      : rule<assign<&ast::Param::name>(ref<VarName>) +
             option(lit<":"> +
                    assign<&ast::Param::callType>(ref<IDENTIFIER>))> {};

  // -- Statements --------------------------------------------------------

  struct SimpleStmt
      : rule<assign<&ast::SimpleStmt::kind>(
            as<ast::SimpleKind::continue_>(ikw<"CONTINUE">) | as<ast::SimpleKind::exit_>(ikw<"EXIT">) | as<ast::SimpleKind::halt_>(ikw<"HALT">))> {};

  struct ForStmt
      : rule<create<ast::ForStmt>(
            ikw<"FOR"> + assign<&ast::ForStmt::var>(ref<IDENTIFIER>) +
            lit<"="> + assign<&ast::ForStmt::from>(ref<Expression>) +
            ikw<"TO"> + assign<&ast::ForStmt::to>(ref<Expression>) +
            option(assign<&ast::ForStmt::stepName>(ref<IDENTIFIER>) +
                   assign<&ast::ForStmt::step>(ref<Expression>)) +
            ref<NEWLINE> +
            many(append<&ast::ForStmt::body>(ref<StmtRule>) | ref<NEWLINE>) +
            ikw<"ENDFOR">)> {};

  struct GotoStmt
      : rule<create<ast::GotoStmt>(
            ikw<"GOTO"> + assign<&ast::GotoStmt::target>(ref<IDENTIFIER>) +
            ref<NEWLINE>)> {};

  struct LabelStmt
      : rule<create<ast::LabelStmt>(
            assign<&ast::LabelStmt::name>(ref<IDENTIFIER>) + lit<":"> +
            ref<NEWLINE>)> {};

  // A statement list's blank lines are their own alternative beside the
  // append, never folded into StmtRule -- StmtRule always builds a Stmt.
  struct IfStmt
      : rule<create<ast::IfStmt>(
            ikw<"IF"> + assign<&ast::IfStmt::cond>(ref<Expression>) +
            ikw<"THEN"> + ref<NEWLINE> +
            many(append<&ast::IfStmt::thenBody>(ref<StmtRule>) | ref<NEWLINE>) +
            option(ikw<"ELSE"> + ref<NEWLINE> +
                   many(append<&ast::IfStmt::elseBody>(ref<StmtRule>) |
                        ref<NEWLINE>)) +
            ikw<"ENDIF"> + ref<NEWLINE>)> {};

  struct LoopStmt
      : rule<create<ast::LoopStmt>(
            ikw<"LOOP"> + ref<NEWLINE> +
            many(append<&ast::LoopStmt::body>(ref<StmtRule>) | ref<NEWLINE>) +
            ikw<"ENDLOOP"> + ref<NEWLINE>)> {};

  struct RepeatStmt
      : rule<create<ast::RepeatStmt>(
            ikw<"REPEAT"> + ref<NEWLINE> +
            many(append<&ast::RepeatStmt::body>(ref<StmtRule>) | ref<NEWLINE>) +
            ikw<"UNTIL"> + assign<&ast::RepeatStmt::cond>(ref<Expression>) +
            ref<NEWLINE>)> {};

  struct CaseGroup
      : rule<ikw<"CASE"> +
             some(append<&ast::CaseGroup::values>(ref<Expression>), lit<",">) +
             ref<NEWLINE> +
             many(append<&ast::CaseGroup::body>(ref<StmtRule>) |
                  ref<NEWLINE>)> {};

  struct SwitchStmt
      : rule<create<ast::SwitchStmt>(
            ikw<"SWITCH"> + assign<&ast::SwitchStmt::subject>(ref<Expression>) +
            ref<NEWLINE> + many(ref<NEWLINE>) +
            some(append<&ast::SwitchStmt::cases>(ref<CaseGroup>)) +
            option(flag<&ast::SwitchStmt::hasDefault>(ikw<"DEFAULT">) +
                   ref<NEWLINE> +
                   many(append<&ast::SwitchStmt::defaultBody>(ref<StmtRule>) |
                        ref<NEWLINE>)) +
            ikw<"ENDSWITCH"> + ref<NEWLINE>)> {};

  struct WaitStmt
      : rule<create<ast::WaitStmt>(
            ikw<"WAIT"> +
            ((ikw<"FOR"> + assign<&ast::WaitStmt::value>(ref<Expression>)) |
             (flag<&ast::WaitStmt::seconds>(ikw<"SEC">) +
              assign<&ast::WaitStmt::value>(ref<Expression>))) +
            ref<NEWLINE>)> {};

  struct WhileStmt
      : rule<create<ast::WhileStmt>(
            ikw<"WHILE"> + assign<&ast::WhileStmt::cond>(ref<Expression>) +
            ref<NEWLINE> +
            many(append<&ast::WhileStmt::body>(ref<StmtRule>) | ref<NEWLINE>) +
            ikw<"ENDWHILE"> + ref<NEWLINE>)> {};

  struct ReturnStmt
      : rule<create<ast::ReturnStmt>(
            ikw<"RETURN"> +
            option(assign<&ast::ReturnStmt::value>(ref<AssignmentExpr>)) +
            ref<NEWLINE>)> {};

  struct BrakeStmt
      : rule<create<ast::BrakeStmt>(
            ikw<"BRAKE"> +
            option(assign<&ast::BrakeStmt::target>(ref<IDENTIFIER>)) +
            ref<NEWLINE>)> {};

  struct ExprStmt
      : rule<create<ast::ExprStmt>(
            assign<&ast::ExprStmt::value>(ref<AssignmentExpr>) +
            ref<NEWLINE>)> {};

  struct InterruptDeclStmt
      : rule<create<ast::InterruptDeclStmt>(
            option(flag<&ast::InterruptDeclStmt::global>(
                ikw<"GLOBAL">)) +
            ikw<"INTERRUPT"> + ikw<"DECL"> +
            assign<&ast::InterruptDeclStmt::number>(ref<Primary>) +
            ikw<"WHEN"> +
            assign<&ast::InterruptDeclStmt::condition>(ref<Expression>) +
            ikw<"DO"> +
            assign<&ast::InterruptDeclStmt::action>(ref<AssignmentExpr>) +
            ref<NEWLINE>)> {};

  struct InterruptCtrlStmt
      : rule<create<ast::InterruptCtrlStmt>(
            ikw<"INTERRUPT"> +
            assign<&ast::InterruptCtrlStmt::mode>(ref<IDENTIFIER>) +
            option(assign<&ast::InterruptCtrlStmt::number>(ref<Primary>)) +
            ref<NEWLINE>)> {};

  static constexpr auto ApproxMod = as<ast::MotionMod::dis>(ikw<"C_DIS">) | as<ast::MotionMod::ori>(ikw<"C_ORI">) | as<ast::MotionMod::vel>(ikw<"C_VEL">);

  struct PtpStmt
      : rule<create<ast::MotionStmt>(
            assign<&ast::MotionStmt::kind>(as<ast::MotionKind::ptp>(ikw<"PTP">) | as<ast::MotionKind::ptp_rel>(ikw<"PTP_REL">)) +
            assign<&ast::MotionStmt::target>(ref<GeomExpr>) +
            option(flag<&ast::MotionStmt::continuous>(ikw<"C_PTP">) +
                   option(assign<&ast::MotionStmt::approx>(ApproxMod))) +
            ref<NEWLINE>)> {};

  struct LinStmt
      : rule<create<ast::MotionStmt>(
            assign<&ast::MotionStmt::kind>(as<ast::MotionKind::lin>(ikw<"LIN">)) +
            assign<&ast::MotionStmt::target>(ref<GeomExpr>) +
            option(assign<&ast::MotionStmt::approx>(ApproxMod)) +
            ref<NEWLINE>)> {};

  struct LinRelStmt
      : rule<create<ast::MotionStmt>(
            assign<&ast::MotionStmt::kind>(as<ast::MotionKind::lin_rel>(ikw<"LIN_REL">)) +
            assign<&ast::MotionStmt::target>(ref<GeomExpr>) +
            option(assign<&ast::MotionStmt::approx>(ApproxMod)) +
            option(assign<&ast::MotionStmt::status>(ref<EnumLit>)) +
            ref<NEWLINE>)> {};

  struct CircStmt
      : rule<create<ast::MotionStmt>(
            assign<&ast::MotionStmt::kind>(as<ast::MotionKind::circ>(ikw<"CIRC">) | as<ast::MotionKind::circ_rel>(ikw<"CIRC_REL">)) +
            assign<&ast::MotionStmt::target>(ref<GeomExpr>) + lit<","> +
            assign<&ast::MotionStmt::auxTarget>(ref<GeomExpr>) +
            option(lit<","> +
                   assign<&ast::MotionStmt::auxName>(ref<IDENTIFIER>) +
                   assign<&ast::MotionStmt::auxValue>(ref<Primary>)) +
            option(assign<&ast::MotionStmt::approx>(ApproxMod)) +
            ref<NEWLINE>)> {};

  struct TriggerStmt
      : rule<create<ast::TriggerStmt>(
            ikw<"TRIGGER"> + ikw<"WHEN"> +
            assign<&ast::TriggerStmt::name>(ref<IDENTIFIER>) + lit<"="> +
            assign<&ast::TriggerStmt::value>(ref<Expression>) + ikw<"DELAY"> +
            lit<"="> + assign<&ast::TriggerStmt::delay>(ref<Expression>) +
            ikw<"DO"> +
            assign<&ast::TriggerStmt::action>(ref<AssignmentExpr>) +
            option(ikw<"PRIO"> + lit<"="> +
                   assign<&ast::TriggerStmt::priority>(ref<Expression>)) +
            ref<NEWLINE>)> {};

  struct AnalogParam
      : rule<assign<&ast::AnalogParam::name>(ref<IDENTIFIER>) + lit<"="> +
             assign<&ast::AnalogParam::value>(ref<Literal>)> {};

  // A bare name is already a valid expression, so the channel-link form
  // needs no alternative of its own here.
  struct AnoutStmt
      : rule<create<ast::AnalogStmt>(
            ikw<"ANOUT"> + assign<&ast::AnalogStmt::channel>(ref<IDENTIFIER>) +
            assign<&ast::AnalogStmt::value>(ref<AssignmentExpr>) +
            many(append<&ast::AnalogStmt::params>(ref<AnalogParam>)) +
            ref<NEWLINE>)> {};

  struct AninStmt
      : rule<create<ast::AnalogStmt>(
            flag<&ast::AnalogStmt::isInput>(ikw<"ANIN">) +
            assign<&ast::AnalogStmt::channel>(ref<IDENTIFIER>) +
            assign<&ast::AnalogStmt::value>(ref<AssignmentExpr>) +
            ref<NEWLINE>)> {};

  struct StmtRule
      : rule<(ref<SimpleStmt> + ref<NEWLINE>) | ref<ForStmt> |
             ref<GotoStmt> | ref<IfStmt> | ref<LoopStmt> |
             ref<RepeatStmt> | ref<SwitchStmt> | ref<WaitStmt> |
             ref<WhileStmt> | ref<ReturnStmt> | ref<BrakeStmt> |
             ref<InterruptDeclStmt> | ref<InterruptCtrlStmt> |
             ref<PtpStmt> | ref<LinStmt> | ref<LinRelStmt> |
             ref<CircStmt> | ref<TriggerStmt> | ref<AnoutStmt> |
             ref<AninStmt> | ref<LabelStmt> | ref<ExprStmt>> {};

  // A blank line here is one NEWLINE, like every sibling statement list.
  static constexpr auto RoutineData =
      many((append<&ast::Routine::data>(ref<VarDecl>) + ref<NEWLINE>) |
           (append<&ast::Routine::data>(ref<ForwardDecl>) + ref<NEWLINE>) |
           (append<&ast::Routine::data>(ref<ImportDecl>) + ref<NEWLINE>) |
           ref<NEWLINE>);
  static constexpr auto RoutineBody =
      RoutineData +
      many(append<&ast::Routine::body>(ref<StmtRule>) | ref<NEWLINE>);

  struct Routine
      : rule<option(flag<&ast::Routine::global>(ikw<"GLOBAL">)) +
             ((ikw<"DEF"> + assign<&ast::Routine::name>(ref<IDENTIFIER>) +
               lit<"("> +
               option(some(append<&ast::Routine::params>(ref<Param>),
                           lit<",">)) +
               lit<")"> + ref<NEWLINE> + RoutineBody + ikw<"END">) |
              (flag<&ast::Routine::isFunction>(ikw<"DEFFCT">) +
               assign<&ast::Routine::returnType>(ref<TypeRef>) +
               assign<&ast::Routine::name>(ref<IDENTIFIER>) + lit<"("> +
               option(some(append<&ast::Routine::params>(ref<Param>),
                           lit<",">)) +
               lit<")"> + ref<NEWLINE> + RoutineBody + ikw<"ENDFCT">))> {};

  struct RoutineModule
      : rule<create<ast::RoutineModule>(
            assign<&ast::RoutineModule::main>(ref<Routine>) +
            many(append<&ast::RoutineModule::subs>(ref<Routine>) |
                 ref<NEWLINE>))> {};

  static constexpr auto DataList =
      many((append<&ast::DataModule::items>(ref<ForwardDecl>) +
               ref<NEWLINE>) |
           (append<&ast::DataModule::items>(ref<StructDef>) +
               ref<NEWLINE>) |
           (append<&ast::DataModule::items>(ref<EnumDef>) + ref<NEWLINE>) |
           (append<&ast::DataModule::items>(ref<DataVarDecl>) +
               ref<NEWLINE>) |
           (append<&ast::DataModule::items>(ref<ArrayInit>) +
               ref<NEWLINE>) |
           (append<&ast::DataModule::items>(ref<ImportDecl>) +
               ref<NEWLINE>) |
           ref<NEWLINE>);

  struct DataModule
      : rule<create<ast::DataModule>(
            ikw<"DEFDAT"> + assign<&ast::DataModule::name>(ref<IDENTIFIER>) +
            option(flag<&ast::DataModule::isPublic>(ikw<"PUBLIC">)) +
            ref<NEWLINE> + DataList + ikw<"ENDDAT"> + many(ref<NEWLINE>))> {};

  struct Module
      : rule<assign<&ast::Module::data>(ref<DataModule>) |
             assign<&ast::Module::routines>(ref<RoutineModule>)> {};

  using entry = Module;
  using ignored = WS;
  using hidden = list<LINE_COMMENT, HEADERLINE>;
};

}  // namespace kuka
