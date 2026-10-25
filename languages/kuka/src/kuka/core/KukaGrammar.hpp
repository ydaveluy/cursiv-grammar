// The kuka language: its grammar. KRL keywords are case-insensitive and
// identifiers admit `$` (system variables like $VEL_AXIS), so word_boundary
// is widened to match; NEWLINE is an ordinary mandatory token, not trivia.
#pragma once

#include <cstdint>
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
using DecodedText = cursiv::text_conv<decode_quoted>;

inline constexpr auto decode_int = [](std::string_view t) -> std::int64_t {
  if (t.front() != '\'') {
    std::int64_t v = 0;
    for (const char c : t) v = v * 10 + (c - '0');
    return v;
  }
  const char kind = t[1];
  const std::string_view digits = t.substr(2, t.size() - 3);
  const int base = (kind == 'H' || kind == 'h') ? 16 : 2;
  std::int64_t v = 0;
  for (const char c : digits) {
    const int d = (c >= '0' && c <= '9')   ? c - '0'
                  : (c >= 'a' && c <= 'f') ? c - 'a' + 10
                                           : c - 'A' + 10;
    v = v * base + d;
  }
  return v;
};
using DecodedInt = cursiv::text_conv<decode_int>;

struct Kuka : cursiv::GrammarSpec {
  static constexpr auto word_boundary = cr<"a-zA-Z0-9_$">;

  struct WS : terminal<some(cr<R"( \t\f)">)> {};
  struct HEADERLINE : terminal<lit<"&"> + many(cr<R"(^\r\n)">) + option(eol)> {};
  struct LINE_COMMENT : terminal<lit<";"> + many(cr<R"(^\r\n)">)> {};
  struct NEWLINE : terminal<option(lit<"\r">) + lit<"\n">> {};

  struct IDENTIFIER : terminal<cr<"a-zA-Z_$"> + many(cr<"a-zA-Z_$0-9">)> {};

  static constexpr auto EscapeSeq =
      lit<"\\"> + (cr<R"(btnfr"'\\)">                 |
                   (cr<"0-3"> + cr<"0-7"> + cr<"0-7">) |
                   (cr<"0-7"> + cr<"0-7">)            |
                   cr<"0-7">);

  struct CHARLITERAL
      : terminal<lit<"'"> + (EscapeSeq | cr<R"(^'\\\r\n)">) + lit<"'">,
                 DecodedText> {};
  struct STRINGLITERAL
      : terminal<lit<"\""> + many(EscapeSeq | cr<R"(^"\\\r\n)">) + lit<"\"">,
                 DecodedText> {};

  static constexpr auto Exponent = cr<"eE"> + option(cr<"+-">) + some(d);
  struct FLOATLITERAL
      : terminal<(some(d) + lit<"."> + many(d) + option(Exponent)) |
                     (lit<"."> + some(d) + option(Exponent)) |
                     (some(d) + Exponent),
                 double> {};

  struct INTLITERAL
      : terminal<some(d) |
                     (lit<"'"> + cr<"hH"> + some(cr<"0-9a-fA-F">) + lit<"'">) |
                     (lit<"'"> + cr<"bB"> + some(cr<"01">) + lit<"'">),
                 DecodedInt> {};

  // Operators, each denoting the constant `op` carries -- longest first
  // inside a level so `==`/`<=`/`>=`/`<>` never lex as `=`/`<`/`>`.
  struct OpEq : terminal<lit<"==">, value<ast::Op::eq>> {};
  struct OpNe : terminal<lit<"<>">, value<ast::Op::ne>> {};
  struct OpLe : terminal<lit<"<=">, value<ast::Op::le>> {};
  struct OpGe : terminal<lit<">=">, value<ast::Op::ge>> {};
  struct OpLt : terminal<lit<"<">, value<ast::Op::lt>> {};
  struct OpGt : terminal<lit<">">, value<ast::Op::gt>> {};
  struct OpOr : terminal<ikw<"OR">, value<ast::Op::log_or>> {};
  struct OpBOr : terminal<ikw<"B_OR">, value<ast::Op::bit_or>> {};
  struct OpExor : terminal<ikw<"EXOR">, value<ast::Op::log_exor>> {};
  struct OpBExor : terminal<ikw<"B_EXOR">, value<ast::Op::bit_exor>> {};
  struct OpAnd : terminal<ikw<"AND">, value<ast::Op::log_and>> {};
  struct OpBAnd : terminal<ikw<"B_AND">, value<ast::Op::bit_and>> {};
  struct OpAdd : terminal<lit<"+">, value<ast::Op::add>> {};
  struct OpSub : terminal<lit<"-">, value<ast::Op::sub>> {};
  struct OpMul : terminal<lit<"*">, value<ast::Op::mul>> {};
  struct OpDiv : terminal<lit<"/">, value<ast::Op::div>> {};
  struct OpGeom : terminal<lit<":">, value<ast::Op::geom>> {};
  // An infix table's operator terminals belong to that one table across the
  // whole grammar, so the motion-target ladder below needs its own type for
  // `:` even though it spells and means the same as OpGeom.
  struct TargetColonOp : terminal<lit<":">, value<ast::Op::geom>> {};

  static constexpr auto RelOp =
      ref<OpEq> | ref<OpNe> | ref<OpLe> | ref<OpGe> | ref<OpLt> | ref<OpGt>;
  static constexpr auto OrOp = ref<OpOr> | ref<OpBOr>;
  static constexpr auto ExorOp = ref<OpExor> | ref<OpBExor>;
  static constexpr auto AndOp = ref<OpAnd> | ref<OpBAnd>;
  static constexpr auto AddOp = ref<OpAdd> | ref<OpSub>;
  static constexpr auto MulOp = ref<OpMul> | ref<OpDiv>;
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
  struct VarNameRule;
  struct VarRefRule;
  struct LiteralRule;
  struct StmtRule;

  struct Expression
      : infix<UnaryNot, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(GeomOp) >>   //
               left(MulOp) >>   //
               left(AddOp) >>   //
               left(AndOp) >>   //
               left(ExorOp) >>  //
               left(OrOp) >>    //
               left(RelOp))> {};

  // A motion target climbs only this far -- unary +/-/NOT and `:` -- though
  // a parenthesized operand still reaches Primary's paren branch and the
  // full Expression inside it.
  struct GeomExpr
      : infix<UnaryNot, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(ref<TargetColonOp>)> {};

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
      : rule<(lit<"("> + ref<AssignmentExpr> + lit<")">) | ref<VarRefRule> |
             ref<LiteralRule>> {};

  struct AssignmentExpr
      : rule<create<ast::Assign>(assign<&ast::Assign::target>(ref<Expression>) +
                                 lit<"="> +
                                 assign<&ast::Assign::value>(
                                     ref<AssignmentExpr>)) |
             ref<Expression>> {};

  struct VarNameRule
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

  struct VarRefRule
      : rule<create<ast::VarRef>(
            append<&ast::VarRef::path>(ref<VarNameRule>) +
            many(lit<"."> + append<&ast::VarRef::path>(ref<VarNameRule>)) +
            option(flag<&ast::VarRef::called>(lit<"(">) +
                   option(some(append<&ast::VarRef::args>(ref<AssignmentExpr>),
                               lit<",">)) +
                   lit<")">))> {};

  struct EnumLitRule
      : rule<create<ast::EnumLit>(lit<"#"> +
                                  assign<&ast::EnumLit::name>(ref<IDENTIFIER>))> {
  };
  struct IntLitRule : rule<assign<&ast::IntLit::value>(ref<INTLITERAL>)> {};
  struct FloatLitRule
      : rule<assign<&ast::FloatLit::value>(ref<FLOATLITERAL>)> {};
  struct CharLitRule : rule<assign<&ast::CharLit::value>(ref<CHARLITERAL>)> {};
  struct StringLitRule
      : rule<assign<&ast::StringLit::value>(ref<STRINGLITERAL>)> {};
  struct BoolLitRule
      : rule<create<ast::BoolLit> +
             (flag<&ast::BoolLit::value>(ikw<"TRUE">) | ikw<"FALSE">)> {};

  struct StructElementRule
      : rule<assign<&ast::StructElement::name>(ref<VarNameRule>) +
             assign<&ast::StructElement::value>(ref<UnarySign>)> {};
  struct StructLitRule
      : rule<create<ast::StructLit>(
            lit<"{"> +
            option(assign<&ast::StructLit::typeName>(ref<IDENTIFIER>) +
                   lit<":">) +
            some(append<&ast::StructLit::elements>(ref<StructElementRule>),
                 lit<",">) +
            lit<"}">)> {};

  struct LiteralRule
      : rule<ref<IntLitRule> | ref<FloatLitRule> | ref<CharLitRule> |
             ref<StringLitRule> | ref<StructLitRule> | ref<BoolLitRule> |
             ref<EnumLitRule>> {};

  struct TypeRefRule
      : rule<assign<&ast::TypeRef::name>(PrimitiveKw | ref<IDENTIFIER>) +
             option(flag<&ast::TypeRef::isArray>(lit<"[">) +
                    option(assign<&ast::TypeRef::arraySize>(ref<INTLITERAL>)) +
                    lit<"]">)> {};

  struct ParamTypeRule
      : rule<assign<&ast::ParamType::type>(ref<TypeRefRule>) +
             option(lit<":"> +
                    assign<&ast::ParamType::callType>(ref<IDENTIFIER>))> {};

  struct ForwardDeclRule
      : rule<create<ast::ForwardDecl>(
            (ikw<"EXT"> + assign<&ast::ForwardDecl::name>(ref<IDENTIFIER>) +
             lit<"("> +
             option(some(append<&ast::ForwardDecl::params>(ref<ParamTypeRule>),
                         lit<",">)) +
             lit<")">) |
            (flag<&ast::ForwardDecl::isFunction>(ikw<"EXTFCT">) +
             assign<&ast::ForwardDecl::returnType>(ref<TypeRefRule>) +
             assign<&ast::ForwardDecl::name>(ref<IDENTIFIER>) + lit<"("> +
             option(some(append<&ast::ForwardDecl::params>(ref<ParamTypeRule>),
                         lit<",">)) +
             lit<")">))> {};

  struct SignalDeclRule
      : rule<create<ast::SignalDecl>(
            ikw<"SIGNAL"> + assign<&ast::SignalDecl::name>(ref<IDENTIFIER>) +
            assign<&ast::SignalDecl::address>(ref<Primary>) +
            option(ikw<"TO"> +
                   assign<&ast::SignalDecl::to>(ref<Primary>)))> {};

  // A routine-local declaration: an optional DECL, then a typed name list
  // or a SIGNAL line -- GLOBAL/CONST and an initialiser are DEFDAT-only.
  struct VarDeclRule
      : rule<option(ikw<"DECL">) +
             (create<ast::TypedDecl>(
                  assign<&ast::TypedDecl::type>(ref<TypeRefRule>) +
                  append<&ast::TypedDecl::names>(ref<VarNameRule>) +
                  many(lit<","> +
                       append<&ast::TypedDecl::names>(ref<VarNameRule>))) |
              ref<SignalDeclRule>)> {};

  // A DEFDAT declaration: DECL, optional GLOBAL/CONST, then a typed name
  // list or a single initialised name, or a SIGNAL line.
  struct DataVarDeclRule
      : rule<option(ikw<"DECL">) +
             ((option(flag<&ast::TypedDecl::global>(ikw<"GLOBAL">)) +
               option(flag<&ast::TypedDecl::isConst>(ikw<"CONST">)) +
               create<ast::TypedDecl>(
                   assign<&ast::TypedDecl::type>(ref<TypeRefRule>) +
                   append<&ast::TypedDecl::names>(ref<VarNameRule>) +
                   (many(lit<","> + append<&ast::TypedDecl::names>(
                                        ref<VarNameRule>)) |
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

  struct FieldGroupRule
      : rule<assign<&ast::FieldGroup::type>(ref<TypeRefRule>) +
             append<&ast::FieldGroup::names>(ref<VarNameRule>) +
             many(lit<","> + append<&ast::FieldGroup::names>(ref<VarNameRule>))> {
  };
  struct StructDefRule
      : rule<option(flag<&ast::StructDef::global>(ikw<"GLOBAL">)) +
             ikw<"STRUC"> + assign<&ast::StructDef::name>(ref<IDENTIFIER>) +
             append<&ast::StructDef::groups>(ref<FieldGroupRule>) +
             many(lit<","> +
                  append<&ast::StructDef::groups>(ref<FieldGroupRule>))> {};

  struct EnumDefRule
      : rule<option(flag<&ast::EnumDef::global>(ikw<"GLOBAL">)) +
             ikw<"ENUM"> + assign<&ast::EnumDef::name>(ref<IDENTIFIER>) +
             append<&ast::EnumDef::values>(ref<IDENTIFIER>) +
             many(lit<","> + append<&ast::EnumDef::values>(ref<IDENTIFIER>))> {
  };

  static constexpr auto ArrayIndices =
      option(assign<&ast::ArrayInit::index0>(ref<Expression>) +
             option(lit<","> +
                    option(assign<&ast::ArrayInit::index1>(ref<Expression>) +
                           option(lit<","> +
                                  option(assign<&ast::ArrayInit::index2>(
                                      ref<Expression>))))));
  struct ArrayInitRule
      : rule<create<ast::ArrayInit>(
            assign<&ast::ArrayInit::name>(ref<IDENTIFIER>) + lit<"["> +
            ArrayIndices + lit<"]"> + lit<"="> +
            assign<&ast::ArrayInit::value>(ref<UnarySign>))> {};

  struct ImportDeclRule
      : rule<create<ast::ImportDecl>(
            ikw<"IMPORT"> + assign<&ast::ImportDecl::type>(ref<TypeRefRule>) +
            assign<&ast::ImportDecl::localName>(ref<VarNameRule>) +
            ikw<"IS"> + lit<"/R1/"> +
            assign<&ast::ImportDecl::moduleName>(ref<IDENTIFIER>) +
            lit<".."> +
            assign<&ast::ImportDecl::remoteName>(ref<VarNameRule>))> {};

  struct ParamRule
      : rule<assign<&ast::Param::name>(ref<VarNameRule>) +
             option(lit<":"> +
                    assign<&ast::Param::callType>(ref<IDENTIFIER>))> {};

  // -- Statements --------------------------------------------------------

  struct KwContinue
      : terminal<ikw<"CONTINUE">, value<ast::SimpleKind::continue_>> {};
  struct KwExit : terminal<ikw<"EXIT">, value<ast::SimpleKind::exit_>> {};
  struct KwHalt : terminal<ikw<"HALT">, value<ast::SimpleKind::halt_>> {};
  struct SimpleStmtRule
      : rule<assign<&ast::SimpleStmt::kind>(
            ref<KwContinue> | ref<KwExit> | ref<KwHalt>)> {};

  struct ForStmtRule
      : rule<create<ast::ForStmt>(
            ikw<"FOR"> + assign<&ast::ForStmt::var>(ref<IDENTIFIER>) +
            lit<"="> + assign<&ast::ForStmt::from>(ref<Expression>) +
            ikw<"TO"> + assign<&ast::ForStmt::to>(ref<Expression>) +
            option(assign<&ast::ForStmt::stepName>(ref<IDENTIFIER>) +
                   assign<&ast::ForStmt::step>(ref<Expression>)) +
            ref<NEWLINE> +
            many(append<&ast::ForStmt::body>(ref<StmtRule>)) +
            ikw<"ENDFOR">)> {};

  struct GotoStmtRule
      : rule<create<ast::GotoStmt>(
            ikw<"GOTO"> + assign<&ast::GotoStmt::target>(ref<IDENTIFIER>) +
            ref<NEWLINE>)> {};

  struct LabelStmtRule
      : rule<create<ast::LabelStmt>(
            assign<&ast::LabelStmt::name>(ref<IDENTIFIER>) + lit<":"> +
            ref<NEWLINE>)> {};

  // A statement list's blank lines are their own alternative beside the
  // append, never folded into StmtRule -- StmtRule always builds a Stmt.
  struct IfStmtRule
      : rule<create<ast::IfStmt>(
            ikw<"IF"> + assign<&ast::IfStmt::cond>(ref<Expression>) +
            ikw<"THEN"> + ref<NEWLINE> +
            many(append<&ast::IfStmt::thenBody>(ref<StmtRule>) | ref<NEWLINE>) +
            option(ikw<"ELSE"> + ref<NEWLINE> +
                   many(append<&ast::IfStmt::elseBody>(ref<StmtRule>) |
                        ref<NEWLINE>)) +
            ikw<"ENDIF"> + ref<NEWLINE>)> {};

  struct LoopStmtRule
      : rule<create<ast::LoopStmt>(
            ikw<"LOOP"> + ref<NEWLINE> +
            many(append<&ast::LoopStmt::body>(ref<StmtRule>) | ref<NEWLINE>) +
            ikw<"ENDLOOP"> + ref<NEWLINE>)> {};

  struct RepeatStmtRule
      : rule<create<ast::RepeatStmt>(
            ikw<"REPEAT"> + ref<NEWLINE> +
            many(append<&ast::RepeatStmt::body>(ref<StmtRule>) | ref<NEWLINE>) +
            ikw<"UNTIL"> + assign<&ast::RepeatStmt::cond>(ref<Expression>) +
            ref<NEWLINE>)> {};

  struct CaseGroupRule
      : rule<ikw<"CASE"> +
             append<&ast::CaseGroup::values>(ref<Expression>) +
             many(lit<","> + append<&ast::CaseGroup::values>(ref<Expression>)) +
             ref<NEWLINE> +
             many(append<&ast::CaseGroup::body>(ref<StmtRule>) |
                  ref<NEWLINE>)> {};

  struct SwitchStmtRule
      : rule<create<ast::SwitchStmt>(
            ikw<"SWITCH"> + assign<&ast::SwitchStmt::subject>(ref<Expression>) +
            ref<NEWLINE> + many(ref<NEWLINE>) +
            some(append<&ast::SwitchStmt::cases>(ref<CaseGroupRule>)) +
            option(flag<&ast::SwitchStmt::hasDefault>(ikw<"DEFAULT">) +
                   ref<NEWLINE> +
                   many(append<&ast::SwitchStmt::defaultBody>(ref<StmtRule>) |
                        ref<NEWLINE>)) +
            ikw<"ENDSWITCH"> + ref<NEWLINE>)> {};

  struct WaitStmtRule
      : rule<create<ast::WaitStmt>(
            ikw<"WAIT"> +
            ((ikw<"FOR"> + assign<&ast::WaitStmt::value>(ref<Expression>)) |
             (flag<&ast::WaitStmt::seconds>(ikw<"SEC">) +
              assign<&ast::WaitStmt::value>(ref<Expression>))) +
            ref<NEWLINE>)> {};

  struct WhileStmtRule
      : rule<create<ast::WhileStmt>(
            ikw<"WHILE"> + assign<&ast::WhileStmt::cond>(ref<Expression>) +
            ref<NEWLINE> +
            many(append<&ast::WhileStmt::body>(ref<StmtRule>) | ref<NEWLINE>) +
            ikw<"ENDWHILE"> + ref<NEWLINE>)> {};

  struct ReturnStmtRule
      : rule<create<ast::ReturnStmt>(
            ikw<"RETURN"> +
            option(assign<&ast::ReturnStmt::value>(ref<AssignmentExpr>)) +
            ref<NEWLINE>)> {};

  struct BrakeStmtRule
      : rule<create<ast::BrakeStmt>(
            ikw<"BRAKE"> +
            option(assign<&ast::BrakeStmt::target>(ref<IDENTIFIER>)) +
            ref<NEWLINE>)> {};

  struct ExprStmtRule
      : rule<create<ast::ExprStmt>(
            assign<&ast::ExprStmt::value>(ref<AssignmentExpr>) +
            ref<NEWLINE>)> {};

  struct InterruptDeclStmtRule
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

  struct InterruptCtrlStmtRule
      : rule<create<ast::InterruptCtrlStmt>(
            ikw<"INTERRUPT"> +
            assign<&ast::InterruptCtrlStmt::mode>(ref<IDENTIFIER>) +
            option(assign<&ast::InterruptCtrlStmt::number>(ref<Primary>)) +
            ref<NEWLINE>)> {};

  struct KwPtp : terminal<ikw<"PTP">, value<ast::MotionKind::ptp>> {};
  struct KwPtpRel
      : terminal<ikw<"PTP_REL">, value<ast::MotionKind::ptp_rel>> {};
  struct KwLin : terminal<ikw<"LIN">, value<ast::MotionKind::lin>> {};
  struct KwLinRel
      : terminal<ikw<"LIN_REL">, value<ast::MotionKind::lin_rel>> {};
  struct KwCirc : terminal<ikw<"CIRC">, value<ast::MotionKind::circ>> {};
  struct KwCircRel
      : terminal<ikw<"CIRC_REL">, value<ast::MotionKind::circ_rel>> {};
  struct KwCDis : terminal<ikw<"C_DIS">, value<ast::MotionMod::dis>> {};
  struct KwCOri : terminal<ikw<"C_ORI">, value<ast::MotionMod::ori>> {};
  struct KwCVel : terminal<ikw<"C_VEL">, value<ast::MotionMod::vel>> {};
  static constexpr auto ApproxMod = ref<KwCDis> | ref<KwCOri> | ref<KwCVel>;

  struct PtpStmtRule
      : rule<create<ast::MotionStmt>(
            assign<&ast::MotionStmt::kind>(ref<KwPtp> | ref<KwPtpRel>) +
            assign<&ast::MotionStmt::target>(ref<GeomExpr>) +
            option(flag<&ast::MotionStmt::continuous>(ikw<"C_PTP">) +
                   option(assign<&ast::MotionStmt::approx>(ApproxMod))) +
            ref<NEWLINE>)> {};

  struct LinStmtRule
      : rule<create<ast::MotionStmt>(
            assign<&ast::MotionStmt::kind>(ref<KwLin>) +
            assign<&ast::MotionStmt::target>(ref<GeomExpr>) +
            option(assign<&ast::MotionStmt::approx>(ApproxMod)) +
            ref<NEWLINE>)> {};

  struct LinRelStmtRule
      : rule<create<ast::MotionStmt>(
            assign<&ast::MotionStmt::kind>(ref<KwLinRel>) +
            assign<&ast::MotionStmt::target>(ref<GeomExpr>) +
            option(assign<&ast::MotionStmt::approx>(ApproxMod)) +
            option(assign<&ast::MotionStmt::status>(ref<EnumLitRule>)) +
            ref<NEWLINE>)> {};

  struct CircStmtRule
      : rule<create<ast::MotionStmt>(
            assign<&ast::MotionStmt::kind>(ref<KwCirc> | ref<KwCircRel>) +
            assign<&ast::MotionStmt::target>(ref<GeomExpr>) + lit<","> +
            assign<&ast::MotionStmt::auxTarget>(ref<GeomExpr>) +
            option(lit<","> +
                   assign<&ast::MotionStmt::auxName>(ref<IDENTIFIER>) +
                   assign<&ast::MotionStmt::auxValue>(ref<Primary>)) +
            option(assign<&ast::MotionStmt::approx>(ApproxMod)) +
            ref<NEWLINE>)> {};

  struct TriggerStmtRule
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

  struct AnalogParamRule
      : rule<assign<&ast::AnalogParam::name>(ref<IDENTIFIER>) + lit<"="> +
             assign<&ast::AnalogParam::value>(ref<LiteralRule>)> {};

  // A bare name is already a valid expression, so the channel-link form
  // needs no alternative of its own here.
  struct AnoutStmtRule
      : rule<create<ast::AnalogStmt>(
            ikw<"ANOUT"> + assign<&ast::AnalogStmt::channel>(ref<IDENTIFIER>) +
            assign<&ast::AnalogStmt::value>(ref<AssignmentExpr>) +
            many(append<&ast::AnalogStmt::params>(ref<AnalogParamRule>)) +
            ref<NEWLINE>)> {};

  struct AninStmtRule
      : rule<create<ast::AnalogStmt>(
            flag<&ast::AnalogStmt::isInput>(ikw<"ANIN">) +
            assign<&ast::AnalogStmt::channel>(ref<IDENTIFIER>) +
            assign<&ast::AnalogStmt::value>(ref<AssignmentExpr>) +
            ref<NEWLINE>)> {};

  struct StmtRule
      : rule<(ref<SimpleStmtRule> + ref<NEWLINE>) | ref<ForStmtRule> |
             ref<GotoStmtRule> | ref<IfStmtRule> | ref<LoopStmtRule> |
             ref<RepeatStmtRule> | ref<SwitchStmtRule> | ref<WaitStmtRule> |
             ref<WhileStmtRule> | ref<ReturnStmtRule> | ref<BrakeStmtRule> |
             ref<InterruptDeclStmtRule> | ref<InterruptCtrlStmtRule> |
             ref<PtpStmtRule> | ref<LinStmtRule> | ref<LinRelStmtRule> |
             ref<CircStmtRule> | ref<TriggerStmtRule> | ref<AnoutStmtRule> |
             ref<AninStmtRule> | ref<LabelStmtRule> | ref<ExprStmtRule>> {};

  // A blank line here is one NEWLINE, like every sibling statement list.
  static constexpr auto RoutineData =
      many((append<&ast::Routine::data>(ref<VarDeclRule>) + ref<NEWLINE>) |
           (append<&ast::Routine::data>(ref<ForwardDeclRule>) + ref<NEWLINE>) |
           (append<&ast::Routine::data>(ref<ImportDeclRule>) + ref<NEWLINE>) |
           ref<NEWLINE>);
  static constexpr auto RoutineBody =
      RoutineData +
      many(append<&ast::Routine::body>(ref<StmtRule>) | ref<NEWLINE>);

  struct RoutineRule
      : rule<option(flag<&ast::Routine::global>(ikw<"GLOBAL">)) +
             ((ikw<"DEF"> + assign<&ast::Routine::name>(ref<IDENTIFIER>) +
               lit<"("> +
               option(some(append<&ast::Routine::params>(ref<ParamRule>),
                           lit<",">)) +
               lit<")"> + ref<NEWLINE> + RoutineBody + ikw<"END">) |
              (flag<&ast::Routine::isFunction>(ikw<"DEFFCT">) +
               assign<&ast::Routine::returnType>(ref<TypeRefRule>) +
               assign<&ast::Routine::name>(ref<IDENTIFIER>) + lit<"("> +
               option(some(append<&ast::Routine::params>(ref<ParamRule>),
                           lit<",">)) +
               lit<")"> + ref<NEWLINE> + RoutineBody + ikw<"ENDFCT">))> {};

  struct RoutineModuleRule
      : rule<create<ast::RoutineModule>(
            assign<&ast::RoutineModule::main>(ref<RoutineRule>) +
            many(append<&ast::RoutineModule::subs>(ref<RoutineRule>) |
                 ref<NEWLINE>))> {};

  static constexpr auto DataList =
      many((append<&ast::DataModule::items>(ref<ForwardDeclRule>) +
               ref<NEWLINE>) |
           (append<&ast::DataModule::items>(ref<StructDefRule>) +
               ref<NEWLINE>) |
           (append<&ast::DataModule::items>(ref<EnumDefRule>) + ref<NEWLINE>) |
           (append<&ast::DataModule::items>(ref<DataVarDeclRule>) +
               ref<NEWLINE>) |
           (append<&ast::DataModule::items>(ref<ArrayInitRule>) +
               ref<NEWLINE>) |
           (append<&ast::DataModule::items>(ref<ImportDeclRule>) +
               ref<NEWLINE>) |
           ref<NEWLINE>);

  struct DataModuleRule
      : rule<create<ast::DataModule>(
            ikw<"DEFDAT"> + assign<&ast::DataModule::name>(ref<IDENTIFIER>) +
            option(flag<&ast::DataModule::isPublic>(ikw<"PUBLIC">)) +
            ref<NEWLINE> + DataList + ikw<"ENDDAT"> + many(ref<NEWLINE>))> {};

  struct ModuleRule
      : rule<assign<&ast::Module::data>(ref<DataModuleRule>) |
             assign<&ast::Module::routines>(ref<RoutineModuleRule>)> {};

  using entry = ModuleRule;
  using ignore = WS;
  using hidden = list<LINE_COMMENT, HEADERLINE>;
};

}  // namespace kuka
