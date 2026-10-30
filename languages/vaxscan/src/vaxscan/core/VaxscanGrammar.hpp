#pragma once

#include <charconv>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "vaxscan/core/ast.hpp"

namespace vaxscan {
inline constexpr auto nameValue = [](std::string_view text) {
  using Result = cursiv::ConversionResult<std::string_view>;
  if (text.size() > 31) return Result::err("A SCAN name cannot exceed 31 characters.");
  constexpr std::string_view reserved[] = {
      "ALLOCATE", "AND", "ANSWER", "CALL", "CASE", "CLOSE", "COLLATE", "CONSTANT",
      "DECLARE", "ELSE", "END", "ERROR", "EXTERNAL", "FAIL", "FALSE", "FILE",
      "FOR", "FORWARD", "FREE", "FROM", "GOTO", "GROUP", "IF", "INCLUDE",
      "INRANGE", "LIST", "MACRO", "MODULE", "NIL", "NOT", "OPEN", "OR",
      "OUTRANGE", "PROCEDURE", "PROMPT", "PRUNE", "READ", "REDEFINE", "RETURN",
      "SCAN", "SET", "START", "STEP", "STOP", "THEN", "TO", "TOKEN", "TRIGGER",
      "TRUE", "TYPE", "WHILE", "WRITE", "XOR"};
  for (const auto word : reserved) {
    if (word.size() != text.size()) continue;
    bool equal = true;
    for (std::size_t i = 0; i < text.size(); ++i) {
      const char c = text[i] >= 'a' && text[i] <= 'z' ? char(text[i] - 'a' + 'A') : text[i];
      if (c != word[i]) { equal = false; break; }
    }
    if (equal) return Result::err("A SCAN reserved word cannot be used as a name.");
  }
  return Result::ok(text);
};
inline constexpr auto integerValue = [](std::string_view text) {
  std::int32_t value = 0;
  const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
  using Result = cursiv::ConversionResult<std::int64_t>;
  if (error != std::errc{} || end != text.data() + text.size())
    return Result::err("SCAN integer magnitude cannot exceed 2147483647.");
  return Result::ok(value);
};

struct Vaxscan : cursiv::GrammarSpec {
  static constexpr auto word_boundary = cr<"a-zA-Z0-9_$">;

  struct WS : terminal<some(cr<R"( \t\n\r\f)">)> {};
  struct BangComment : terminal<lit<"!"> + many(cr<R"(^\r\n)">)> {};
  struct BlockComment : terminal<lit<"/*"> + many(!lit<"*/"> + cr<R"(^\r\n)">) +
      option(lit<"*/">)> {};

  struct NAME : terminal<as<nameValue>(cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9$_">))> {};
  struct INTEGER_LITERAL : terminal<as<integerValue>(some(d))> {};

  inline static constexpr auto undouble = [](std::string_view t,
                                              std::pmr::memory_resource* mem) {
    if (t.size() < 2) return t;
    const std::string_view body = t.substr(1, t.size() - 2);
    if (body.find('\'') == std::string_view::npos) return body;
    const std::span<char> room = cursiv::text_room(mem, body.size());
    std::size_t n = 0;
    for (std::size_t i = 0; i < body.size(); ++i) {
      room[n++] = body[i];
      if (body[i] == '\'') ++i;
    }
    return std::string_view(room.data(), n);
  };

  struct CHARACTER_LITERAL
      : terminal<as<undouble>(lit<"'"> + many(lit<"''"> | cr<R"(^'\r\n)">) +
                              lit<"'">)> {};

  struct SCAN_LITERAL
      : terminal<ikw<"s'eol'"> | ikw<"s'sos'"> | ikw<"s'eos'">> {};

  // Every control-character literal spelling, flattened into one
  // terminal: which named lexer rule owned which spelling carries no
  // meaning past the lexer.
  struct CTRL_CHAR_LITERAL
      : terminal<ikw<"s'nul'"> | ikw<"s'soh'"> | ikw<"s'stx'"> |
                 ikw<"s'etx'"> | ikw<"s'eot'"> | ikw<"s'enq'"> |
                 ikw<"s'ack'"> | ikw<"s'bel'"> | ikw<"s'bs'"> |
                 ikw<"s'ht'"> | ikw<"s'lf'"> | ikw<"s'ff'"> | ikw<"s'vt'"> |
                 ikw<"s'cr'"> | ikw<"s'so'"> | ikw<"s'si'"> |
                 ikw<"s'dle'"> | ikw<"s'dc1'"> | ikw<"s'dc2'"> |
                 ikw<"s'dc3'"> | ikw<"s'dc4'"> | ikw<"s'nak'"> |
                 ikw<"s'syn'"> | ikw<"s'etb'"> | ikw<"s'can'"> |
                 ikw<"s'em'"> | ikw<"s'sub'"> | ikw<"s'esc'"> |
                 ikw<"s'fs'"> | ikw<"s'gs'"> | ikw<"s'rs'"> | ikw<"s'us'"> |
                 ikw<"s'del'"> | ikw<"s'ind'"> | ikw<"s'nel'"> |
                 ikw<"s'ssa'"> | ikw<"s'esa'"> | ikw<"s'hts'"> |
                 ikw<"s'htj'"> | ikw<"s'vts'"> | ikw<"s'pid'"> |
                 ikw<"s'plu'"> | ikw<"s'ri'"> | ikw<"s'ss2'"> |
                 ikw<"s'ss3'"> | ikw<"s'dcs'"> | ikw<"s'pu1'"> |
                 ikw<"s'pu2'"> | ikw<"s'sts'"> | ikw<"s'cch'"> |
                 ikw<"s'mw'"> | ikw<"s'spa'"> | ikw<"s'epa'"> |
                 ikw<"s'csi'"> | ikw<"s'st'"> | ikw<"s'osc'"> |
                 ikw<"s'pm'"> | ikw<"s'apc'">> {};

  static constexpr auto StringLiteralText =
      ref<CHARACTER_LITERAL> | ref<CTRL_CHAR_LITERAL> | ref<SCAN_LITERAL>;

  // Soft keywords: every one of these also reads as a plain identifier,
  // so each is spelled once here and referenced both from `Id` and from
  // every place it is a literal keyword -- never re-spelled as a second
  // terminal or an inline `ikw<>`.
  struct NATIVE : terminal<ikw<"NATIVE">> {};
  struct ASCII : terminal<ikw<"ASCII">> {};
  struct EBCDIC : terminal<ikw<"EBCDIC">> {};
  struct DEC_MULTI : terminal<ikw<"DEC_MULTI">> {};
  struct USER : terminal<ikw<"USER">> {};
  struct TITLE : terminal<ikw<"TITLE">> {};
  struct ON : terminal<ikw<"ON">> {};
  struct OFF : terminal<ikw<"OFF">> {};
  struct PAGE : terminal<ikw<"PAGE">> {};
  struct OF : terminal<ikw<"OF">> {};
  struct TREE : terminal<ikw<"TREE">> {};
  struct RECORD : terminal<ikw<"RECORD">> {};
  struct OVERLAY : terminal<ikw<"OVERLAY">> {};
  struct INTEGER : terminal<ikw<"INTEGER">> {};
  struct STRING : terminal<ikw<"STRING">> {};
  struct BOOLEAN : terminal<ikw<"BOOLEAN">> {};
  struct POINTER : terminal<ikw<"POINTER">> {};
  struct TREEPTR : terminal<ikw<"TREEPTR">> {};
  struct FILL : terminal<ikw<"FILL">> {};
  struct VARYING : terminal<ikw<"VARYING">> {};
  struct FIXED : terminal<ikw<"FIXED">> {};
  struct DYNAMIC : terminal<ikw<"DYNAMIC">> {};
  struct COMMON : terminal<ikw<"COMMON">> {};
  struct STATIC : terminal<ikw<"STATIC">> {};
  struct AUTOMATIC : terminal<ikw<"AUTOMATIC">> {};
  struct GLOBAL : terminal<ikw<"GLOBAL">> {};
  struct EXPOSE : terminal<ikw<"EXPOSE">> {};
  struct MAIN : terminal<ikw<"MAIN">> {};
  struct CASELESS : terminal<ikw<"CASELESS">> {};
  struct ALIAS : terminal<ikw<"ALIAS">> {};
  struct VALUE : terminal<ikw<"VALUE">> {};
  struct REFERENCE : terminal<ikw<"REFERENCE">> {};
  struct DESCRIPTOR : terminal<ikw<"DESCRIPTOR">> {};
  struct SYNTAX : terminal<ikw<"SYNTAX">> {};
  struct INPUT : terminal<ikw<"INPUT">> {};
  struct OUTPUT : terminal<ikw<"OUTPUT">> {};
  struct DATA : terminal<ikw<"DATA">> {};
  struct STACK : terminal<ikw<"STACK">> {};
  struct AS : terminal<ikw<"AS">> {};
  struct IDENT : terminal<ikw<"IDENT">> {};
  struct IGNORE : terminal<ikw<"IGNORE">> {};
  struct WIDTH : terminal<ikw<"WIDTH">> {};

  static constexpr auto Id =
      ref<NAME> | ref<NATIVE> | ref<ASCII> | ref<EBCDIC> | ref<DEC_MULTI> |
      ref<USER> | ref<TITLE> | ref<ON> | ref<OFF> | ref<PAGE> | ref<OF> |
      ref<TREE> | ref<RECORD> | ref<OVERLAY> | ref<INTEGER> | ref<STRING> |
      ref<BOOLEAN> | ref<POINTER> | ref<TREEPTR> | ref<FILL> |
      ref<VARYING> | ref<FIXED> | ref<DYNAMIC> | ref<COMMON> |
      ref<STATIC> | ref<AUTOMATIC> | ref<GLOBAL> | ref<EXPOSE> |
      ref<MAIN> | ref<CASELESS> | ref<ALIAS> | ref<VALUE> |
      ref<REFERENCE> | ref<DESCRIPTOR> | ref<SYNTAX> | ref<INPUT> |
      ref<OUTPUT> | ref<DATA> | ref<STACK> | ref<AS> | ref<IDENT> |
      ref<IGNORE> | ref<WIDTH>;

  static constexpr auto PicId = lit<"*"> | Id;

  static constexpr auto LevelTypeText = ref<STRING> | ref<INTEGER>;

  static constexpr auto PassingMech = ref<VALUE> | ref<REFERENCE> | ref<DESCRIPTOR>;

  // ---- Expressions ----------------------------------------------------

  struct Expression;
  struct NotChain;
  struct Compare;
  struct Unary;
  struct Postfix;
  struct Primary;
  struct PartRef;
  struct Arg;
  struct NameExpr;
  struct RefChain;
  struct VariableRef;

  struct NameExpr : rule<assign<&ast::NameExpr::name>(Id)> {};

  struct RefChain
      : rule<ref<NameExpr> +
             many(nest<ast::CallExpr, &ast::CallExpr::callee>(
                      lit<"("> +
                      many(append<&ast::CallExpr::args>(ref<Arg>),
                           lit<",">) +
                      lit<")">) |
                  nest<ast::FieldExpr, &ast::FieldExpr::object>(
                      lit<"."> + assign<&ast::FieldExpr::field>(Id)) |
                  nest<ast::DerefExpr, &ast::DerefExpr::object>(lit<"->">))> {
  };

  struct Arg
      : rule<ref<Expression> | (create<ast::OmittedArg> + lit<"*">)> {};

  struct PartRef
      : rule<(assign<&ast::IndexPart::index>(ref<Expression>) + lit<"]">) |
             (assign<&ast::RangePart::start>(ref<Expression>) + lit<"..">  +
              (lit<"]"> |
               (assign<&ast::RangePart::end>(ref<Expression>) +
                lit<"]">)))> {};

  struct VariableRef
      : rule<create<ast::SubscriptExpr>(
                 assign<&ast::SubscriptExpr::base>(ref<RefChain>) +
                 lit<"["> +
                 assign<&ast::SubscriptExpr::part>(ref<PartRef>)) |
             ref<RefChain>> {};

  struct IntLit : rule<assign<&ast::IntLit::value>(ref<INTEGER_LITERAL>)> {
  };
  struct StrLit : rule<assign<&ast::StrLit::value>(StringLiteralText)> {};
  struct NilLit : rule<create<ast::NilLit> + ikw<"NIL">> {};
  struct BoolLit
      : rule<create<ast::BoolLit> +
             (flag<&ast::BoolLit::value>(ikw<"TRUE">) |
              ikw<"FALSE">)> {};

  struct Primary
      : rule<(lit<"("> + ref<Expression> + lit<")">) | ref<IntLit> |
             ref<StrLit> | ref<NilLit> | ref<BoolLit> |
             ref<RefChain>> {};

  struct Postfix
      : rule<create<ast::SubscriptExpr>(
                 assign<&ast::SubscriptExpr::base>(ref<Primary>) + lit<"["> +
                 assign<&ast::SubscriptExpr::part>(ref<PartRef>)) |
             ref<Primary>> {};

  struct Unary
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"-"> | lit<"+">) +
                 assign<&ast::UnaryExpr::operand>(ref<Unary>)) |
             ref<Postfix>> {};

  struct Compare
      : infix<Unary, ast::BinaryExpr,
              levels(left(lit<"*"> | lit<"/">),
                     left(lit<"+"> | lit<"-">),
                     left(lit<"&">),
                     left(lit<"=="> | lit<"<>"> | lit<"><"> | lit<"<="> |
                          lit<"=<"> | lit<">="> | lit<"=>"> | lit<"="> |
                          lit<"<"> | lit<">">))> {};

  struct NotChain
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ikw<"NOT">) +
                 assign<&ast::UnaryExpr::operand>(ref<NotChain>)) |
             ref<Compare>> {};

  struct Expression
      : infix<NotChain, ast::BinaryExpr,
              levels(left(ikw<"AND">), left(ikw<"OR"> | ikw<"XOR">))> {};

  // ---- Types ------------------------------------------------------------

  struct TypeDec;
  struct CommonTypeDec;

  struct Length : rule<lit<"("> + ref<Expression> + lit<")">> {};

  struct UserDefinedType
      : rule<create<ast::SimpleType>(assign<&ast::SimpleType::keyword>(
            ref<NAME> | ref<NATIVE> | ref<ASCII> | ref<EBCDIC> |
            ref<DEC_MULTI> | ref<USER> | ref<TITLE> | ref<ON> | ref<OFF> |
            ref<PAGE> | ref<OF> | ref<EXPOSE> | ref<MAIN> |
            ref<CASELESS> | ref<ALIAS> | ref<SYNTAX> | ref<INPUT> |
            ref<OUTPUT> | ref<DATA> | ref<STACK> | ref<WIDTH>))> {};

  struct Component
      : rule<create<ast::RecordComponent>(
            assign<&ast::RecordComponent::name>(Id) + lit<":"> +
            assign<&ast::RecordComponent::type>(ref<CommonTypeDec>) +
            lit<",">)> {};

  struct CommonTypeDec
      : rule<create<ast::SimpleType>(
                 assign<&ast::SimpleType::keyword>(ref<INTEGER>)) |
             create<ast::StringType>(
                 flag<&ast::StringType::varying>(ref<VARYING>) +
                 ref<STRING> +
                 assign<&ast::StringType::length>(ref<Length>)) |
             create<ast::StringType>(
                 ref<STRING> +
                 assign<&ast::StringType::length>(ref<Length>)) |
             create<ast::StringType>(
                 flag<&ast::StringType::fixed>(ref<FIXED>) +
                 ref<STRING> +
                 assign<&ast::StringType::length>(ref<Length>)) |
             create<ast::SimpleType>(
                 assign<&ast::SimpleType::keyword>(ref<BOOLEAN>)) |
             create<ast::FillType>(
                 ref<FILL> +
                 assign<&ast::FillType::length>(ref<Length>)) |
             create<ast::PointerType>(
                 ref<POINTER> + ikw<"TO"> +
                 assign<&ast::PointerType::target>(ref<TypeDec>)) |
             create<ast::TreePtrType>(
                 ref<TREEPTR> + lit<"("> +
                 assign<&ast::TreePtrType::level>(LevelTypeText) +
                 lit<")"> + ikw<"TO"> +
                 assign<&ast::TreePtrType::target>(ref<TypeDec>)) |
             create<ast::RecordType>(
                 ref<RECORD> +
                 some(append<&ast::RecordType::components>(
                     ref<Component>)) +
                 ikw<"END"> + ref<RECORD>) |
             create<ast::RecordType>(
                 flag<&ast::RecordType::overlay>(ref<OVERLAY>) +
                 some(append<&ast::RecordType::components>(
                     ref<Component>)) +
                 ikw<"END"> + ref<OVERLAY>) |
             ref<UserDefinedType>> {};

  struct TypeDec
      : rule<ref<CommonTypeDec> |
             create<ast::SimpleType>(
                 assign<&ast::SimpleType::keyword>(ref<STRING>)) |
             create<ast::SimpleType>(
                 assign<&ast::SimpleType::keyword>(ikw<"FILE">)) |
             create<ast::StringType>(
                 flag<&ast::StringType::dynamic>(ref<DYNAMIC>) +
                 ref<STRING>) |
             create<ast::TreeType>(
                 ref<TREE> + lit<"("> +
                 some(append<&ast::TreeType::levels>(LevelTypeText),
                      lit<",">) +
                 lit<")"> + ref<OF> +
                 assign<&ast::TreeType::of>(ref<TypeDec>))> {};

  // ---- SET / GROUP boolean algebras -------------------------------------

  struct SetExpr;
  struct GroupExpr;

  struct SetItem
      : rule<assign<&ast::BoolRef::name>(Id) |
             (assign<&ast::BoolRange::low>(StringLiteralText) + lit<"..">  +
              assign<&ast::BoolRange::high>(StringLiteralText)) |
             assign<&ast::BoolLiteral::text>(StringLiteralText) |
             (lit<"("> + ref<SetExpr> + lit<")">)> {};

  struct SetPrimary
      : rule<ref<SetItem> |
             create<ast::BoolNot>(
                 ikw<"NOT"> +
                 assign<&ast::BoolNot::operand>(ref<SetItem>))> {};

  struct SetExpr : infix<SetPrimary, ast::BoolBinary,
      levels(left(ikw<"AND">), left(ikw<"OR">))> {};

  struct GroupItem
      : rule<assign<&ast::BoolRef::name>(Id) |
             assign<&ast::BoolLiteral::text>(ref<CHARACTER_LITERAL>) |
             (lit<"("> + ref<GroupExpr> + lit<")">)> {};

  struct GroupPrimary
      : rule<ref<GroupItem> |
             create<ast::BoolNot>(
                 ikw<"NOT"> +
                 assign<&ast::BoolNot::operand>(ref<GroupItem>))> {};

  struct GroupExpr : infix<GroupPrimary, ast::BoolBinary,
      levels(left(ikw<"AND">), left(ikw<"OR">))> {};

  // ---- TOKEN patterns and COLLATE USER sequences -------------------------

  struct TokenExpr;
  struct UserSeq;

  struct TokenAtom
      : rule<assign<&ast::PatternLiteral::text>(StringLiteralText) |
             assign<&ast::PatternRef::name>(Id) |
             create<ast::PatternOptional>(
                 lit<"["> +
                 assign<&ast::PatternOptional::operand>(ref<TokenExpr>) +
                 lit<"]">) |
             (lit<"{"> + ref<TokenExpr> + lit<"}">)> {};

  struct TokenUnit
      : rule<ref<TokenAtom> |
             create<ast::PatternRepeat>(
                 assign<&ast::PatternRepeat::operand>(ref<TokenAtom>) +
                 lit<"...">)> {};

  struct TokenSeq
      : rule<ref<TokenUnit> |
             create<ast::PatternSeq>(
                 assign<&ast::PatternSeq::left>(ref<TokenSeq>) +
                 assign<&ast::PatternSeq::right>(ref<TokenUnit>))> {};

  struct TokenExpr
      : rule<ref<TokenSeq> |
             create<ast::PatternAlt>(
                 assign<&ast::PatternAlt::left>(ref<TokenExpr>) +
                 assign<&ast::PatternAlt::op>(lit<"|"> | lit<"/">) +
                 assign<&ast::PatternAlt::right>(ref<TokenSeq>))> {};

  struct UserSeqAtom
      : rule<assign<&ast::PatternRef::name>(Id) |
             (assign<&ast::PatternRange::low>(StringLiteralText) +
              lit<"..">  +
              assign<&ast::PatternRange::high>(StringLiteralText)) |
             assign<&ast::PatternLiteral::text>(StringLiteralText) |
             (lit<"{"> + ref<UserSeq> + lit<"}">)> {};

  struct UserSeqTerm
      : rule<ref<UserSeqAtom> |
             create<ast::PatternSeq>(
                 assign<&ast::PatternSeq::left>(ref<UserSeqTerm>) +
                 assign<&ast::PatternSeq::right>(ref<UserSeqAtom>))> {};

  struct UserSeq
      : rule<ref<UserSeqTerm> |
             create<ast::PatternAlt>(
                 assign<&ast::PatternAlt::left>(ref<UserSeq>) +
                 assign<&ast::PatternAlt::op>(lit<"|"> | lit<"/">) +
                 assign<&ast::PatternAlt::right>(ref<UserSeqTerm>))> {};

  struct TokenAttr
      : rule<create<ast::TokenAttr>(
                 assign<&ast::TokenAttr::kind>(ref<CASELESS>)) |
             create<ast::TokenAttr>(
                 assign<&ast::TokenAttr::kind>(ref<IGNORE>)) |
             create<ast::TokenAttr>(
                 assign<&ast::TokenAttr::kind>(ref<ALIAS>) +
                 assign<&ast::TokenAttr::alias>(ref<CHARACTER_LITERAL>))> {};

  struct TokenDecl
      : rule<create<ast::TokenDecl>(
            ikw<"TOKEN"> + assign<&ast::TokenDecl::name>(Id) +
            many(append<&ast::TokenDecl::attrs>(ref<TokenAttr>)) +
            lit<"{"> +
            assign<&ast::TokenDecl::pattern>(ref<TokenExpr>) +
            lit<"}">)> {};

  // ---- MACRO pictures -----------------------------------------------------

  struct Picture;

  struct MacroItem
      : rule<create<ast::PicCall>(
                 assign<&ast::PicCall::name>(Id) + lit<"("> +
                 many(append<&ast::PicCall::args>(ref<Expression>),
                      lit<",">) +
                 lit<")">) |
             assign<&ast::PicName::name>(Id) |
             assign<&ast::PicAlias::text>(ref<CHARACTER_LITERAL>) |
             create<ast::PicOptional>(
                 lit<"["> +
                 assign<&ast::PicOptional::operand>(ref<Picture>) +
                 lit<"]">) |
             (lit<"{"> + ref<Picture> + lit<"}">)> {};

  struct Pic4
      : rule<ref<MacroItem> |
             create<ast::PicBinding>(
                 assign<&ast::PicBinding::name>(Id) + lit<":"> +
                 assign<&ast::PicBinding::item>(ref<MacroItem>)) |
             create<ast::PicBinding>(
                 assign<&ast::PicBinding::name>(Id) + lit<","> +
                 some(append<&ast::PicBinding::names>(PicId), lit<",">) +
                 lit<":"> +
                 assign<&ast::PicBinding::item>(ref<MacroItem>)) |
             create<ast::PicBinding>(
                 flag<&ast::PicBinding::wildcard>(lit<"*">) +
                 lit<","> +
                 some(append<&ast::PicBinding::names>(PicId), lit<",">) +
                 lit<":"> +
                 assign<&ast::PicBinding::item>(ref<MacroItem>))> {};

  struct Pic3
      : rule<ref<Pic4> |
             create<ast::PicRepeat>(
                 assign<&ast::PicRepeat::operand>(ref<Pic4>) +
                 lit<"...">)> {};

  struct Pic2
      : rule<ref<Pic3> |
             create<ast::PicSeq>(
                 assign<&ast::PicSeq::left>(ref<Pic2>) +
                 assign<&ast::PicSeq::right>(ref<Pic3>))> {};

  struct Pic1
      : rule<ref<Pic2> |
             create<ast::PicSplit>(
                 assign<&ast::PicSplit::left>(ref<Pic2>) + lit<"\\"> +
                 assign<&ast::PicSplit::right>(ref<Pic2>))> {};

  struct Picture
      : rule<ref<Pic1> |
             create<ast::PicAlt>(
                 assign<&ast::PicAlt::left>(ref<Picture>) +
                 assign<&ast::PicAlt::op>(lit<"|"> | lit<"/">) +
                 assign<&ast::PicAlt::right>(ref<Pic1>))> {};

  // ---- Statements ---------------------------------------------------------

  struct Statement;
  struct Executable;
  struct ExecListItem;
  struct Directive;
  struct Declaration;
  struct ProcedureDecl;
  struct MacroDecl;

  struct FileRef
      : rule<ikw<"FILE"> + lit<"("> + ref<VariableRef> + lit<")">> {};

  struct Prompt : rule<ikw<"PROMPT"> + lit<"("> + ref<Expression> + lit<")">> {
  };

  struct IfStmt
      : rule<create<ast::IfStmt>(
            ikw<"IF"> + assign<&ast::IfStmt::cond>(ref<Expression>) +
            ikw<"THEN"> +
            many(append<&ast::IfStmt::thenBody>(ref<ExecListItem>)) +
            option(flag<&ast::IfStmt::hasElse>(ikw<"ELSE">) +
                   many(append<&ast::IfStmt::elseBody>(
                       ref<ExecListItem>))) +
            ikw<"END"> + ikw<"IF">)> {};

  struct ForStmt
      : rule<create<ast::ForStmt>(
            ikw<"FOR"> + assign<&ast::ForStmt::var>(Id) + lit<"="> +
            assign<&ast::ForStmt::from>(ref<Expression>) + ikw<"TO"> +
            assign<&ast::ForStmt::to>(ref<Expression>) +
            option(ikw<"STEP"> +
                   assign<&ast::ForStmt::step>(ref<Expression>)) +
            lit<";"> +
            many(append<&ast::ForStmt::body>(ref<ExecListItem>)) +
            ikw<"END"> + ikw<"FOR">)> {};

  struct WhileStmt
      : rule<create<ast::WhileStmt>(
            ikw<"WHILE"> + assign<&ast::WhileStmt::cond>(ref<Expression>) +
            lit<";"> +
            many(append<&ast::WhileStmt::body>(ref<ExecListItem>)) +
            ikw<"END"> + ikw<"WHILE">)> {};

  struct CallStmt
      : rule<create<ast::CallStmt>(
            ikw<"CALL"> + assign<&ast::CallStmt::name>(Id) +
            option(lit<"("> +
                   many(append<&ast::CallStmt::args>(ref<Arg>),
                        lit<",">) +
                   lit<")">))> {};

  struct OpenStmt
      : rule<create<ast::OpenStmt>(
            ikw<"OPEN"> + assign<&ast::OpenStmt::fileRef>(ref<FileRef>) +
            ref<AS> + assign<&ast::OpenStmt::asExpr>(ref<Expression>) +
            ikw<"FOR"> +
            (flag<&ast::OpenStmt::forInput>(ikw<"INPUT">) |
             ikw<"OUTPUT">))> {};

  struct CloseStmt
      : rule<create<ast::CloseStmt>(
            ikw<"CLOSE"> +
            assign<&ast::CloseStmt::fileRef>(ref<FileRef>))> {};

  struct ReadStmt
      : rule<create<ast::ReadStmt>(
            ikw<"READ"> +
            option(assign<&ast::ReadStmt::fileRef>(ref<FileRef>)) +
            option(assign<&ast::ReadStmt::prompt>(ref<Prompt>)) +
            assign<&ast::ReadStmt::target>(ref<VariableRef>))> {};

  struct WriteStmt
      : rule<create<ast::WriteStmt>(
            ikw<"WRITE"> +
            option(assign<&ast::WriteStmt::fileRef>(ref<FileRef>)) +
            many(append<&ast::WriteStmt::values>(ref<Expression>),
                 lit<",">))> {};

  struct GotoStmt
      : rule<create<ast::GotoStmt>(
            ikw<"GOTO"> + assign<&ast::GotoStmt::label>(Id))> {};

  struct CaseExpr
      : rule<(create<ast::CaseInRange> + ikw<"INRANGE">) |
             (create<ast::CaseOutRange> + ikw<"OUTRANGE">) |
             create<ast::CaseRange>(
                 assign<&ast::CaseRange::low>(ref<Expression>) + lit<"..">  +
                 assign<&ast::CaseRange::high>(ref<Expression>)) |
             assign<&ast::CaseValue::value>(ref<Expression>)> {};

  struct CaseItem
      : rule<create<ast::CaseItem>(
            lit<"["> +
            some(append<&ast::CaseItem::tests>(ref<CaseExpr>), lit<",">) +
            lit<"]"> + lit<":"> +
            many(append<&ast::CaseItem::body>(ref<ExecListItem>)))> {};

  struct CaseStmt
      : rule<create<ast::CaseStmt>(
            ikw<"CASE"> + assign<&ast::CaseStmt::selector>(ref<Expression>) +
            ikw<"FROM"> + assign<&ast::CaseStmt::from>(ref<Expression>) +
            ikw<"TO"> + assign<&ast::CaseStmt::to>(ref<Expression>) +
            lit<";"> +
            some(append<&ast::CaseStmt::items>(ref<CaseItem>)) +
            ikw<"END"> + ikw<"CASE">)> {};

  struct ReturnStmt
      : rule<create<ast::ReturnStmt>(
            ikw<"RETURN"> +
            option(assign<&ast::ReturnStmt::value>(ref<Expression>)))> {};

  struct AnswerStmt
      : rule<create<ast::AnswerStmt>(
            ikw<"ANSWER"> +
            option(flag<&ast::AnswerStmt::trigger>(ikw<"TRIGGER">)) +
            some(append<&ast::AnswerStmt::values>(ref<Expression>),
                 lit<",">))> {};

  struct FailStmt : rule<create<ast::FailStmt> + ikw<"FAIL">> {};

  struct ScanAttr
      : rule<create<ast::ScanAttr>(
                 assign<&ast::ScanAttr::direction>(ikw<"INPUT">) +
                 ((assign<&ast::ScanAttr::kind>(ikw<"FILE">) +
                   assign<&ast::ScanAttr::value>(ref<Expression>)) |
                  (assign<&ast::ScanAttr::kind>(ikw<"STRING">) +
                   assign<&ast::ScanAttr::value>(ref<Expression>)) |
                  (assign<&ast::ScanAttr::kind>(ikw<"PROCEDURE">) +
                   assign<&ast::ScanAttr::procName>(Id)) |
                  (assign<&ast::ScanAttr::kind>(ikw<"WIDTH">) +
                   assign<&ast::ScanAttr::value>(ref<Expression>)))) |
             create<ast::ScanAttr>(
                 assign<&ast::ScanAttr::direction>(ikw<"OUTPUT">) +
                 ((assign<&ast::ScanAttr::kind>(ikw<"FILE">) +
                   assign<&ast::ScanAttr::value>(ref<Expression>)) |
                  (assign<&ast::ScanAttr::kind>(ikw<"STRING">) +
                   assign<&ast::ScanAttr::value>(ref<VariableRef>)) |
                  (assign<&ast::ScanAttr::kind>(ikw<"PROCEDURE">) +
                   assign<&ast::ScanAttr::procName>(Id)) |
                  (assign<&ast::ScanAttr::kind>(ikw<"WIDTH">) +
                   assign<&ast::ScanAttr::value>(ref<Expression>)))) |
             create<ast::ScanAttr>(
                 assign<&ast::ScanAttr::direction>(ikw<"DATA">) +
                 assign<&ast::ScanAttr::kind>(ikw<"STACK">) +
                 assign<&ast::ScanAttr::value>(ref<Expression>))> {};

  struct StartScanStmt
      : rule<create<ast::StartScanStmt>(
            ikw<"START"> + ikw<"SCAN"> +
            many(append<&ast::StartScanStmt::attrs>(ref<ScanAttr>)))> {};

  struct StopScanStmt
      : rule<create<ast::StopScanStmt> + ikw<"STOP"> + ikw<"SCAN">> {};

  struct PruneStmt
      : rule<create<ast::PruneStmt>(
            ikw<"PRUNE"> +
            some(append<&ast::PruneStmt::values>(ref<Expression>),
                 lit<",">))> {};

  struct AllocateStmt
      : rule<create<ast::AllocateStmt>(
            ikw<"ALLOCATE"> +
            some(append<&ast::AllocateStmt::targets>(ref<VariableRef>),
                 lit<",">))> {};

  struct FreeStmt
      : rule<create<ast::FreeStmt>(
            ikw<"FREE"> +
            some(append<&ast::FreeStmt::targets>(ref<VariableRef>),
                 lit<",">))> {};

  struct Executable
      : rule<ref<IfStmt> | ref<ForStmt> | ref<WhileStmt> |
             ref<CallStmt> | ref<ReadStmt> | ref<WriteStmt> |
             ref<OpenStmt> | ref<CloseStmt> | ref<GotoStmt> |
             ref<CaseStmt> | ref<ReturnStmt> |
             ref<StopScanStmt> | ref<AnswerStmt> |
             ref<FailStmt> | ref<StartScanStmt> |
             ref<PruneStmt> | ref<AllocateStmt> |
             ref<FreeStmt> | (create<ast::EmptyStmt>) |
             create<ast::LabelStmt>(
                 assign<&ast::LabelStmt::label>(Id) + lit<":"> +
                 assign<&ast::LabelStmt::inner>(ref<Executable>)) |
             create<ast::AssignStmt>(
                 assign<&ast::AssignStmt::target>(ref<VariableRef>) +
                 lit<"="> +
                 assign<&ast::AssignStmt::value>(ref<Expression>))> {};

  struct IncludeDirective
      : rule<create<ast::IncludeDirective>(
            ikw<"INCLUDE"> + ikw<"FILE"> +
            assign<&ast::IncludeDirective::file>(ref<Expression>))> {};

  struct ListDirective
      : rule<ikw<"LIST"> +
             (create<ast::ListDirective>(assign<&ast::ListDirective::mode>(
                  ikw<"ON"> | ikw<"OFF"> | ikw<"PAGE">)) |
              create<ast::ListDirective>(
                  assign<&ast::ListDirective::mode>(ikw<"TITLE">) +
                  assign<&ast::ListDirective::title>(ref<Expression>)))> {};

  struct Directive
      : rule<ref<IncludeDirective> | ref<ListDirective>> {};

  struct ExecListItem
      : rule<(ref<Executable> | ref<Directive>) + lit<";">> {};

  struct Statement
      : rule<(ref<Executable> | ref<Declaration> |
              ref<ProcedureDecl> | ref<MacroDecl> |
              ref<Directive>) +
             lit<";">> {};

  // ---- Declarations ---------------------------------------------------

  struct ConstantDecl
      : rule<create<ast::ConstantDecl>(
            ikw<"CONSTANT"> + assign<&ast::ConstantDecl::name>(Id) +
            ((lit<"="> + assign<&ast::ConstantDecl::value>(ref<Expression>)) |
             (ikw<"EXTERNAL"> +
              assign<&ast::ConstantDecl::externalType>(ref<TypeDec>)) |
             (flag<&ast::ConstantDecl::isGlobal>(ref<GLOBAL>) +
              lit<"="> +
              assign<&ast::ConstantDecl::value>(ref<Expression>))))> {};

  struct VariableDecl
      : rule<create<ast::VariableDecl>(
            ikw<"DECLARE"> +
            some(append<&ast::VariableDecl::names>(PicId), lit<",">) +
            lit<":"> +
            option(assign<&ast::VariableDecl::storageAttr>(
                ref<STATIC> | ref<AUTOMATIC> | ref<COMMON> | ref<GLOBAL> |
                ikw<"EXTERNAL">)) +
            assign<&ast::VariableDecl::type>(ref<TypeDec>))> {};

  struct TypeDecl
      : rule<create<ast::TypeDecl>(
            ikw<"TYPE"> + assign<&ast::TypeDecl::name>(Id) + lit<":"> +
            assign<&ast::TypeDecl::type>(ref<TypeDec>))> {};

  struct Formal
      : rule<create<ast::Formal>(
            option(assign<&ast::Formal::passingMech>(PassingMech)) +
            assign<&ast::Formal::type>(ref<TypeDec>))> {};

  struct ProcedureFormDecl
      : rule<create<ast::ProcedureFormDecl>(
                 ikw<"EXTERNAL"> + ikw<"PROCEDURE"> +
                 assign<&ast::ProcedureFormDecl::name>(Id) +
                 option(lit<"("> +
                        many(append<&ast::ProcedureFormDecl::formals>(
                                 ref<Formal>),
                             lit<",">) +
                        lit<")">) +
                 option(ref<OF> + assign<&ast::ProcedureFormDecl::resultType>(
                                       ref<TypeDec>))) |
             create<ast::ProcedureFormDecl>(
                 flag<&ast::ProcedureFormDecl::isForward>(
                     ikw<"FORWARD">) +
                 ikw<"PROCEDURE"> +
                 assign<&ast::ProcedureFormDecl::name>(Id) +
                 option(lit<"("> +
                        many(append<&ast::ProcedureFormDecl::formals>(
                                 ref<Formal>),
                             lit<",">) +
                        lit<")">) +
                 option(ref<OF> + assign<&ast::ProcedureFormDecl::resultType>(
                                       ref<TypeDec>)))> {};

  struct Declaration
      : rule<ref<ConstantDecl> | ref<VariableDecl> |
             ref<TypeDecl> | ref<ProcedureFormDecl>> {};

  struct Param
      : rule<create<ast::Param>(
            assign<&ast::Param::name>(Id) + lit<":"> +
            option(assign<&ast::Param::passingMech>(PassingMech)) +
            assign<&ast::Param::type>(ref<TypeDec>))> {};

  struct ProcedureDecl
      : rule<create<ast::ProcedureDecl>(
            ikw<"PROCEDURE"> + assign<&ast::ProcedureDecl::name>(Id) +
            option(flag<&ast::ProcedureDecl::isMain>(ref<MAIN>)) +
            option(lit<"("> +
                   many(append<&ast::ProcedureDecl::params>(ref<Param>),
                        lit<",">) +
                   lit<")">) +
            option(ref<OF> +
                   assign<&ast::ProcedureDecl::resultType>(ref<TypeDec>)) +
            lit<";"> +
            many(append<&ast::ProcedureDecl::body>(ref<Statement>)) +
            ikw<"END"> + ikw<"PROCEDURE">)> {};

  struct MacroDecl
      : rule<create<ast::MacroDecl>(
            ikw<"MACRO"> + assign<&ast::MacroDecl::name>(Id) +
            assign<&ast::MacroDecl::kind>(ikw<"TRIGGER"> | ref<SYNTAX>) +
            option(flag<&ast::MacroDecl::expose>(ref<EXPOSE>)) +
            lit<"{"> +
            option(assign<&ast::MacroDecl::picture>(ref<Picture>)) +
            lit<"}"> + lit<";"> +
            many(append<&ast::MacroDecl::body>(ref<Statement>)) +
            ikw<"END"> + ikw<"MACRO">)> {};

  struct SetDecl
      : rule<create<ast::SetDecl>(
            ikw<"SET"> + assign<&ast::SetDecl::name>(Id) + lit<"("> +
            assign<&ast::SetDecl::expr>(ref<SetExpr>) + lit<")">)> {};

  struct GroupDecl
      : rule<create<ast::GroupDecl>(
            ikw<"GROUP"> + assign<&ast::GroupDecl::name>(Id) + lit<"("> +
            assign<&ast::GroupDecl::expr>(ref<GroupExpr>) +
            lit<")">)> {};

  struct CollateDirective
      : rule<ikw<"COLLATE"> +
             (create<ast::CollateDirective>(
                  assign<&ast::CollateDirective::seq>(
                      ref<NATIVE> | ref<ASCII> | ref<EBCDIC> |
                      ref<DEC_MULTI>)) |
              create<ast::CollateDirective>(
                  assign<&ast::CollateDirective::seq>(ref<USER>) +
                  assign<&ast::CollateDirective::userSeq>(
                      ref<UserSeq>)))> {};

  struct RedefineDirective
      : rule<create<ast::RedefineDirective>(
            ikw<"REDEFINE"> +
            assign<&ast::RedefineDirective::charLiteral>(ref<SCAN_LITERAL>) +
            lit<"="> +
            assign<&ast::RedefineDirective::ctrlChar>(
                ref<CTRL_CHAR_LITERAL>))> {};

  struct ModuleStatement
      : rule<ref<Declaration> | ref<SetDecl> | ref<TokenDecl> |
             ref<GroupDecl> | ref<ProcedureDecl> |
             ref<MacroDecl> | ref<Directive> |
             ref<CollateDirective> | ref<RedefineDirective>> {};

  struct Module
      : rule<create<ast::Module>(
            ikw<"MODULE"> + assign<&ast::Module::name>(Id) +
            (lit<";"> |
             (ref<IDENT> + assign<&ast::Module::ident>(ref<CHARACTER_LITERAL>) +
              lit<";">)) +
            some(append<&ast::Module::statements>(ref<ModuleStatement>) +
                 lit<";">) +
            ikw<"END"> + ikw<"MODULE"> + lit<";">)> {};

  using entry = Module;
  using ignored = WS;
  using hidden = list<BangComment, BlockComment>;
};

}  // namespace vaxscan
