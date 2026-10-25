#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "vaxscan/core/ast.hpp"

namespace vaxscan {

struct Vaxscan : cursiv::GrammarSpec {
  static constexpr auto word_boundary = cr<"a-zA-Z0-9_$">;

  struct WS : terminal<some(cr<R"( \t\n\r\f)">)> {};
  struct BangComment : terminal<through(lit<"!">, lit<"\n">)> {};
  struct BlockComment : terminal<through(lit<"/*">, lit<"*/">)> {};

  struct NAME : terminal<cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9$_">)> {};
  struct INTEGER_LITERAL : terminal<some(d), std::int64_t> {};

  inline static constexpr auto undouble = [](std::string_view t,
                                              std::pmr::memory_resource* mem) {
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
  using Undoubled = cursiv::text_conv<undouble>;

  struct CHARACTER_LITERAL
      : terminal<lit<"'"> + many(lit<"''"> | cr<"^'">) + lit<"'">, Undoubled> {
  };

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
  struct PartRefRule;
  struct ArgRule;
  struct NameExprRule;
  struct RefChainRule;
  struct VariableRefRule;

  struct NameExprRule : rule<assign<&ast::NameExpr::name>(Id)> {};

  struct RefChainRule
      : rule<ref<NameExprRule> +
             many(nest<ast::CallExpr, &ast::CallExpr::callee>(
                      lit<"("> +
                      many(append<&ast::CallExpr::args>(ref<ArgRule>),
                           lit<",">) +
                      lit<")">) |
                  nest<ast::FieldExpr, &ast::FieldExpr::object>(
                      lit<"."> + assign<&ast::FieldExpr::field>(Id)) |
                  nest<ast::DerefExpr, &ast::DerefExpr::object>(lit<"->">))> {
  };

  struct ArgRule
      : rule<ref<Expression> | (create<ast::OmittedArg> + lit<"*">)> {};

  struct PartRefRule
      : rule<(assign<&ast::IndexPart::index>(ref<Expression>) + lit<"]">) |
             (assign<&ast::RangePart::start>(ref<Expression>) + lit<"..">  +
              (lit<"]"> |
               (assign<&ast::RangePart::end>(ref<Expression>) +
                lit<"]">)))> {};

  struct VariableRefRule
      : rule<create<ast::SubscriptExpr>(
                 assign<&ast::SubscriptExpr::base>(ref<RefChainRule>) +
                 lit<"["> +
                 assign<&ast::SubscriptExpr::part>(ref<PartRefRule>)) |
             ref<RefChainRule>> {};

  struct IntLitRule : rule<assign<&ast::IntLit::value>(ref<INTEGER_LITERAL>)> {
  };
  struct StrLitRule : rule<assign<&ast::StrLit::value>(StringLiteralText)> {};
  struct NilLitRule : rule<create<ast::NilLit> + ikw<"NIL">> {};
  struct BoolLitRule
      : rule<create<ast::BoolLit> +
             (flag<&ast::BoolLit::value>(ikw<"TRUE">) |
              ikw<"FALSE">)> {};

  struct Primary
      : rule<(lit<"("> + ref<Expression> + lit<")">) | ref<IntLitRule> |
             ref<StrLitRule> | ref<NilLitRule> | ref<BoolLitRule> |
             ref<RefChainRule>> {};

  struct Postfix
      : rule<create<ast::SubscriptExpr>(
                 assign<&ast::SubscriptExpr::base>(ref<Primary>) + lit<"["> +
                 assign<&ast::SubscriptExpr::part>(ref<PartRefRule>)) |
             ref<Primary>> {};

  struct Unary
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"-"> | lit<"+">) +
                 assign<&ast::UnaryExpr::operand>(ref<Unary>)) |
             ref<Postfix>> {};

  struct Compare
      : infix<Unary, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(lit<"*"> | lit<"/">) >>
               left(lit<"+"> | lit<"-">) >>
               left(lit<"&">) >>
               left(lit<"=="> | lit<"<>"> | lit<"><"> | lit<"<="> |
                    lit<"=<"> | lit<">="> | lit<"=>"> | lit<"="> |
                    lit<"<"> | lit<">">))> {};

  struct NotChain
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ikw<"NOT">) +
                 assign<&ast::UnaryExpr::operand>(ref<NotChain>)) |
             ref<Compare>> {};

  struct Expression
      : infix<NotChain, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(ikw<"AND">) >> left(ikw<"OR"> | ikw<"XOR">))> {};

  // ---- Types ------------------------------------------------------------

  struct TypeDecRule;
  struct CommonTypeDecRule;

  struct LengthRule : rule<lit<"("> + ref<Expression> + lit<")">> {};

  struct UserDefinedTypeRule
      : rule<create<ast::SimpleType>(assign<&ast::SimpleType::keyword>(
            ref<NAME> | ref<NATIVE> | ref<ASCII> | ref<EBCDIC> |
            ref<DEC_MULTI> | ref<USER> | ref<TITLE> | ref<ON> | ref<OFF> |
            ref<PAGE> | ref<OF> | ref<EXPOSE> | ref<MAIN> |
            ref<CASELESS> | ref<ALIAS> | ref<SYNTAX> | ref<INPUT> |
            ref<OUTPUT> | ref<DATA> | ref<STACK> | ref<WIDTH>))> {};

  struct ComponentRule
      : rule<create<ast::RecordComponent>(
            assign<&ast::RecordComponent::name>(Id) + lit<":"> +
            assign<&ast::RecordComponent::type>(ref<CommonTypeDecRule>) +
            lit<",">)> {};

  struct CommonTypeDecRule
      : rule<create<ast::SimpleType>(
                 assign<&ast::SimpleType::keyword>(ref<INTEGER>)) |
             create<ast::StringType>(
                 flag<&ast::StringType::varying>(ref<VARYING>) +
                 ref<STRING> +
                 assign<&ast::StringType::length>(ref<LengthRule>)) |
             create<ast::StringType>(
                 ref<STRING> +
                 assign<&ast::StringType::length>(ref<LengthRule>)) |
             create<ast::StringType>(
                 flag<&ast::StringType::fixed>(ref<FIXED>) +
                 ref<STRING> +
                 assign<&ast::StringType::length>(ref<LengthRule>)) |
             create<ast::SimpleType>(
                 assign<&ast::SimpleType::keyword>(ref<BOOLEAN>)) |
             create<ast::FillType>(
                 ref<FILL> +
                 assign<&ast::FillType::length>(ref<LengthRule>)) |
             create<ast::PointerType>(
                 ref<POINTER> + ikw<"TO"> +
                 assign<&ast::PointerType::target>(ref<TypeDecRule>)) |
             create<ast::TreePtrType>(
                 ref<TREEPTR> + lit<"("> +
                 assign<&ast::TreePtrType::level>(LevelTypeText) +
                 lit<")"> + ikw<"TO"> +
                 assign<&ast::TreePtrType::target>(ref<TypeDecRule>)) |
             create<ast::RecordType>(
                 ref<RECORD> +
                 some(append<&ast::RecordType::components>(
                     ref<ComponentRule>)) +
                 ikw<"END"> + ref<RECORD>) |
             create<ast::RecordType>(
                 flag<&ast::RecordType::overlay>(ref<OVERLAY>) +
                 some(append<&ast::RecordType::components>(
                     ref<ComponentRule>)) +
                 ikw<"END"> + ref<OVERLAY>) |
             ref<UserDefinedTypeRule>> {};

  struct TypeDecRule
      : rule<ref<CommonTypeDecRule> |
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
                 assign<&ast::TreeType::of>(ref<TypeDecRule>))> {};

  // ---- SET / GROUP boolean algebras -------------------------------------

  struct SetExprRule;
  struct GroupExprRule;

  struct SetItemRule
      : rule<assign<&ast::BoolRef::name>(Id) |
             (assign<&ast::BoolRange::low>(StringLiteralText) + lit<"..">  +
              assign<&ast::BoolRange::high>(StringLiteralText)) |
             assign<&ast::BoolLiteral::text>(StringLiteralText) |
             (lit<"("> + ref<SetExprRule> + lit<")">)> {};

  struct SetPrimaryRule
      : rule<ref<SetItemRule> |
             create<ast::BoolNot>(
                 ikw<"NOT"> +
                 assign<&ast::BoolNot::operand>(ref<SetItemRule>))> {};

  struct SetTermRule
      : rule<ref<SetPrimaryRule> |
             create<ast::BoolBinary>(
                 assign<&ast::BoolBinary::left>(ref<SetTermRule>) +
                 assign<&ast::BoolBinary::op>(ikw<"AND">) +
                 assign<&ast::BoolBinary::right>(ref<SetPrimaryRule>))> {};

  struct SetExprRule
      : rule<ref<SetTermRule> |
             create<ast::BoolBinary>(
                 assign<&ast::BoolBinary::left>(ref<SetExprRule>) +
                 assign<&ast::BoolBinary::op>(ikw<"OR">) +
                 assign<&ast::BoolBinary::right>(ref<SetTermRule>))> {};

  struct GroupItemRule
      : rule<assign<&ast::BoolRef::name>(Id) |
             assign<&ast::BoolLiteral::text>(ref<CHARACTER_LITERAL>) |
             (lit<"("> + ref<GroupExprRule> + lit<")">)> {};

  struct GroupPrimaryRule
      : rule<ref<GroupItemRule> |
             create<ast::BoolNot>(
                 ikw<"NOT"> +
                 assign<&ast::BoolNot::operand>(ref<GroupItemRule>))> {};

  struct GroupTermRule
      : rule<ref<GroupPrimaryRule> |
             create<ast::BoolBinary>(
                 assign<&ast::BoolBinary::left>(ref<GroupTermRule>) +
                 assign<&ast::BoolBinary::op>(ikw<"AND">) +
                 assign<&ast::BoolBinary::right>(ref<GroupPrimaryRule>))> {};

  struct GroupExprRule
      : rule<ref<GroupTermRule> |
             create<ast::BoolBinary>(
                 assign<&ast::BoolBinary::left>(ref<GroupExprRule>) +
                 assign<&ast::BoolBinary::op>(ikw<"OR">) +
                 assign<&ast::BoolBinary::right>(ref<GroupTermRule>))> {};

  // ---- TOKEN patterns and COLLATE USER sequences -------------------------

  struct TokenExprRule;
  struct UserSeqRule;

  struct TokenAtomRule
      : rule<assign<&ast::PatternLiteral::text>(StringLiteralText) |
             assign<&ast::PatternRef::name>(Id) |
             create<ast::PatternOptional>(
                 lit<"["> +
                 assign<&ast::PatternOptional::operand>(ref<TokenExprRule>) +
                 lit<"]">) |
             (lit<"{"> + ref<TokenExprRule> + lit<"}">)> {};

  struct TokenUnitRule
      : rule<ref<TokenAtomRule> |
             create<ast::PatternRepeat>(
                 assign<&ast::PatternRepeat::operand>(ref<TokenAtomRule>) +
                 lit<"...">)> {};

  struct TokenSeqRule
      : rule<ref<TokenUnitRule> |
             create<ast::PatternSeq>(
                 assign<&ast::PatternSeq::left>(ref<TokenSeqRule>) +
                 assign<&ast::PatternSeq::right>(ref<TokenUnitRule>))> {};

  struct TokenExprRule
      : rule<ref<TokenSeqRule> |
             create<ast::PatternAlt>(
                 assign<&ast::PatternAlt::left>(ref<TokenExprRule>) +
                 assign<&ast::PatternAlt::op>(lit<"|"> | lit<"/">) +
                 assign<&ast::PatternAlt::right>(ref<TokenSeqRule>))> {};

  struct UserSeqAtomRule
      : rule<assign<&ast::PatternRef::name>(Id) |
             (assign<&ast::PatternRange::low>(StringLiteralText) +
              lit<"..">  +
              assign<&ast::PatternRange::high>(StringLiteralText)) |
             assign<&ast::PatternLiteral::text>(StringLiteralText) |
             (lit<"{"> + ref<UserSeqRule> + lit<"}">)> {};

  struct UserSeqTermRule
      : rule<ref<UserSeqAtomRule> |
             create<ast::PatternSeq>(
                 assign<&ast::PatternSeq::left>(ref<UserSeqTermRule>) +
                 assign<&ast::PatternSeq::right>(ref<UserSeqAtomRule>))> {};

  struct UserSeqRule
      : rule<ref<UserSeqTermRule> |
             create<ast::PatternAlt>(
                 assign<&ast::PatternAlt::left>(ref<UserSeqRule>) +
                 assign<&ast::PatternAlt::op>(lit<"|"> | lit<"/">) +
                 assign<&ast::PatternAlt::right>(ref<UserSeqTermRule>))> {};

  struct TokenAttrRule
      : rule<create<ast::TokenAttr>(
                 assign<&ast::TokenAttr::kind>(ref<CASELESS>)) |
             create<ast::TokenAttr>(
                 assign<&ast::TokenAttr::kind>(ref<IGNORE>)) |
             create<ast::TokenAttr>(
                 assign<&ast::TokenAttr::kind>(ref<ALIAS>) +
                 assign<&ast::TokenAttr::alias>(ref<CHARACTER_LITERAL>))> {};

  struct TokenDeclRule
      : rule<create<ast::TokenDecl>(
            ikw<"TOKEN"> + assign<&ast::TokenDecl::name>(Id) +
            many(append<&ast::TokenDecl::attrs>(ref<TokenAttrRule>)) +
            lit<"{"> +
            assign<&ast::TokenDecl::pattern>(ref<TokenExprRule>) +
            lit<"}">)> {};

  // ---- MACRO pictures -----------------------------------------------------

  struct PictureRule;

  struct MacroItemRule
      : rule<create<ast::PicCall>(
                 assign<&ast::PicCall::name>(Id) + lit<"("> +
                 many(append<&ast::PicCall::args>(ref<Expression>),
                      lit<",">) +
                 lit<")">) |
             assign<&ast::PicName::name>(Id) |
             assign<&ast::PicAlias::text>(ref<CHARACTER_LITERAL>) |
             create<ast::PicOptional>(
                 lit<"["> +
                 assign<&ast::PicOptional::operand>(ref<PictureRule>) +
                 lit<"]">) |
             (lit<"{"> + ref<PictureRule> + lit<"}">)> {};

  struct Pic4Rule
      : rule<ref<MacroItemRule> |
             create<ast::PicBinding>(
                 assign<&ast::PicBinding::name>(Id) + lit<":"> +
                 assign<&ast::PicBinding::item>(ref<MacroItemRule>)) |
             create<ast::PicBinding>(
                 assign<&ast::PicBinding::name>(Id) + lit<","> +
                 some(append<&ast::PicBinding::names>(PicId), lit<",">) +
                 lit<":"> +
                 assign<&ast::PicBinding::item>(ref<MacroItemRule>)) |
             create<ast::PicBinding>(
                 flag<&ast::PicBinding::wildcard>(lit<"*">) +
                 lit<","> +
                 some(append<&ast::PicBinding::names>(PicId), lit<",">) +
                 lit<":"> +
                 assign<&ast::PicBinding::item>(ref<MacroItemRule>))> {};

  struct Pic3Rule
      : rule<ref<Pic4Rule> |
             create<ast::PicRepeat>(
                 assign<&ast::PicRepeat::operand>(ref<Pic4Rule>) +
                 lit<"...">)> {};

  struct Pic2Rule
      : rule<ref<Pic3Rule> |
             create<ast::PicSeq>(
                 assign<&ast::PicSeq::left>(ref<Pic2Rule>) +
                 assign<&ast::PicSeq::right>(ref<Pic3Rule>))> {};

  struct Pic1Rule
      : rule<ref<Pic2Rule> |
             create<ast::PicSplit>(
                 assign<&ast::PicSplit::left>(ref<Pic2Rule>) + lit<"\\"> +
                 assign<&ast::PicSplit::right>(ref<Pic2Rule>))> {};

  struct PictureRule
      : rule<ref<Pic1Rule> |
             create<ast::PicAlt>(
                 assign<&ast::PicAlt::left>(ref<PictureRule>) +
                 assign<&ast::PicAlt::op>(lit<"|"> | lit<"/">) +
                 assign<&ast::PicAlt::right>(ref<Pic1Rule>))> {};

  // ---- Statements ---------------------------------------------------------

  struct StatementRule;
  struct ExecutableRule;
  struct ExecListItemRule;
  struct DirectiveRule;
  struct DeclarationRule;
  struct ProcedureDeclRule;
  struct MacroDeclRule;

  struct FileRefRule
      : rule<ikw<"FILE"> + lit<"("> + ref<VariableRefRule> + lit<")">> {};

  struct PromptRule : rule<ikw<"PROMPT"> + lit<"("> + ref<Expression> + lit<")">> {
  };

  struct IfStmtRule
      : rule<create<ast::IfStmt>(
            ikw<"IF"> + assign<&ast::IfStmt::cond>(ref<Expression>) +
            ikw<"THEN"> +
            many(append<&ast::IfStmt::thenBody>(ref<ExecListItemRule>)) +
            option(flag<&ast::IfStmt::hasElse>(ikw<"ELSE">) +
                   many(append<&ast::IfStmt::elseBody>(
                       ref<ExecListItemRule>))) +
            ikw<"END"> + ikw<"IF">)> {};

  struct ForStmtRule
      : rule<create<ast::ForStmt>(
            ikw<"FOR"> + assign<&ast::ForStmt::var>(Id) + lit<"="> +
            assign<&ast::ForStmt::from>(ref<Expression>) + ikw<"TO"> +
            assign<&ast::ForStmt::to>(ref<Expression>) +
            option(ikw<"STEP"> +
                   assign<&ast::ForStmt::step>(ref<Expression>)) +
            lit<";"> +
            many(append<&ast::ForStmt::body>(ref<ExecListItemRule>)) +
            ikw<"END"> + ikw<"FOR">)> {};

  struct WhileStmtRule
      : rule<create<ast::WhileStmt>(
            ikw<"WHILE"> + assign<&ast::WhileStmt::cond>(ref<Expression>) +
            lit<";"> +
            many(append<&ast::WhileStmt::body>(ref<ExecListItemRule>)) +
            ikw<"END"> + ikw<"WHILE">)> {};

  struct CallStmtRule
      : rule<create<ast::CallStmt>(
            ikw<"CALL"> + assign<&ast::CallStmt::name>(Id) +
            option(lit<"("> +
                   many(append<&ast::CallStmt::args>(ref<ArgRule>),
                        lit<",">) +
                   lit<")">))> {};

  struct OpenStmtRule
      : rule<create<ast::OpenStmt>(
            ikw<"OPEN"> + assign<&ast::OpenStmt::fileRef>(ref<FileRefRule>) +
            ref<AS> + assign<&ast::OpenStmt::asExpr>(ref<Expression>) +
            ikw<"FOR"> +
            (flag<&ast::OpenStmt::forInput>(ikw<"INPUT">) |
             ikw<"OUTPUT">))> {};

  struct CloseStmtRule
      : rule<create<ast::CloseStmt>(
            ikw<"CLOSE"> +
            assign<&ast::CloseStmt::fileRef>(ref<FileRefRule>))> {};

  struct ReadStmtRule
      : rule<create<ast::ReadStmt>(
            ikw<"READ"> +
            option(assign<&ast::ReadStmt::fileRef>(ref<FileRefRule>)) +
            option(assign<&ast::ReadStmt::prompt>(ref<PromptRule>)) +
            assign<&ast::ReadStmt::target>(ref<VariableRefRule>))> {};

  struct WriteStmtRule
      : rule<create<ast::WriteStmt>(
            ikw<"WRITE"> +
            option(assign<&ast::WriteStmt::fileRef>(ref<FileRefRule>)) +
            many(append<&ast::WriteStmt::values>(ref<Expression>),
                 lit<",">))> {};

  struct GotoStmtRule
      : rule<create<ast::GotoStmt>(
            ikw<"GOTO"> + assign<&ast::GotoStmt::label>(Id))> {};

  struct CaseExprRule
      : rule<(create<ast::CaseInRange> + ikw<"INRANGE">) |
             (create<ast::CaseOutRange> + ikw<"OUTRANGE">) |
             create<ast::CaseRange>(
                 assign<&ast::CaseRange::low>(ref<Expression>) + lit<"..">  +
                 assign<&ast::CaseRange::high>(ref<Expression>)) |
             assign<&ast::CaseValue::value>(ref<Expression>)> {};

  struct CaseItemRule
      : rule<create<ast::CaseItem>(
            lit<"["> +
            some(append<&ast::CaseItem::tests>(ref<CaseExprRule>), lit<",">) +
            lit<"]"> + lit<":"> +
            many(append<&ast::CaseItem::body>(ref<ExecListItemRule>)))> {};

  struct CaseStmtRule
      : rule<create<ast::CaseStmt>(
            ikw<"CASE"> + assign<&ast::CaseStmt::selector>(ref<Expression>) +
            ikw<"FROM"> + assign<&ast::CaseStmt::from>(ref<Expression>) +
            ikw<"TO"> + assign<&ast::CaseStmt::to>(ref<Expression>) +
            lit<";"> +
            some(append<&ast::CaseStmt::items>(ref<CaseItemRule>)) +
            ikw<"END"> + ikw<"CASE">)> {};

  struct ReturnStmtRule
      : rule<create<ast::ReturnStmt>(
            ikw<"RETURN"> +
            option(assign<&ast::ReturnStmt::value>(ref<Expression>)))> {};

  struct AnswerStmtRule
      : rule<create<ast::AnswerStmt>(
            ikw<"ANSWER"> +
            option(flag<&ast::AnswerStmt::trigger>(ikw<"TRIGGER">)) +
            some(append<&ast::AnswerStmt::values>(ref<Expression>),
                 lit<",">))> {};

  struct FailStmtRule : rule<create<ast::FailStmt> + ikw<"FAIL">> {};

  struct ScanAttrRule
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
                   assign<&ast::ScanAttr::value>(ref<VariableRefRule>)) |
                  (assign<&ast::ScanAttr::kind>(ikw<"PROCEDURE">) +
                   assign<&ast::ScanAttr::procName>(Id)) |
                  (assign<&ast::ScanAttr::kind>(ikw<"WIDTH">) +
                   assign<&ast::ScanAttr::value>(ref<Expression>)))) |
             create<ast::ScanAttr>(
                 assign<&ast::ScanAttr::direction>(ikw<"DATA">) +
                 assign<&ast::ScanAttr::kind>(ikw<"STACK">) +
                 assign<&ast::ScanAttr::value>(ref<Expression>))> {};

  struct StartScanStmtRule
      : rule<create<ast::StartScanStmt>(
            ikw<"START"> + ikw<"SCAN"> +
            many(append<&ast::StartScanStmt::attrs>(ref<ScanAttrRule>)))> {};

  struct StopScanStmtRule
      : rule<create<ast::StopScanStmt> + ikw<"STOP"> + ikw<"SCAN">> {};

  struct PruneStmtRule
      : rule<create<ast::PruneStmt>(
            ikw<"PRUNE"> +
            some(append<&ast::PruneStmt::values>(ref<Expression>),
                 lit<",">))> {};

  struct AllocateStmtRule
      : rule<create<ast::AllocateStmt>(
            ikw<"ALLOCATE"> +
            some(append<&ast::AllocateStmt::targets>(ref<VariableRefRule>),
                 lit<",">))> {};

  struct FreeStmtRule
      : rule<create<ast::FreeStmt>(
            ikw<"FREE"> +
            some(append<&ast::FreeStmt::targets>(ref<VariableRefRule>),
                 lit<",">))> {};

  struct ExecutableRule
      : rule<ref<IfStmtRule> | ref<ForStmtRule> | ref<WhileStmtRule> |
             ref<CallStmtRule> | ref<ReadStmtRule> | ref<WriteStmtRule> |
             ref<OpenStmtRule> | ref<CloseStmtRule> | ref<GotoStmtRule> |
             ref<CaseStmtRule> | ref<ReturnStmtRule> |
             ref<StopScanStmtRule> | ref<AnswerStmtRule> |
             ref<FailStmtRule> | ref<StartScanStmtRule> |
             ref<PruneStmtRule> | ref<AllocateStmtRule> |
             ref<FreeStmtRule> | (create<ast::EmptyStmt>) |
             create<ast::LabelStmt>(
                 assign<&ast::LabelStmt::label>(Id) + lit<":"> +
                 assign<&ast::LabelStmt::inner>(ref<ExecutableRule>)) |
             create<ast::AssignStmt>(
                 assign<&ast::AssignStmt::target>(ref<VariableRefRule>) +
                 lit<"="> +
                 assign<&ast::AssignStmt::value>(ref<Expression>))> {};

  struct IncludeDirectiveRule
      : rule<create<ast::IncludeDirective>(
            ikw<"INCLUDE"> + ikw<"FILE"> +
            assign<&ast::IncludeDirective::file>(ref<Expression>))> {};

  struct ListDirectiveRule
      : rule<ikw<"LIST"> +
             (create<ast::ListDirective>(assign<&ast::ListDirective::mode>(
                  ikw<"ON"> | ikw<"OFF"> | ikw<"PAGE">)) |
              create<ast::ListDirective>(
                  assign<&ast::ListDirective::mode>(ikw<"TITLE">) +
                  assign<&ast::ListDirective::title>(ref<Expression>)))> {};

  struct DirectiveRule
      : rule<ref<IncludeDirectiveRule> | ref<ListDirectiveRule>> {};

  struct ExecListItemRule
      : rule<(ref<ExecutableRule> | ref<DirectiveRule>) + lit<";">> {};

  struct StatementRule
      : rule<(ref<ExecutableRule> | ref<DeclarationRule> |
              ref<ProcedureDeclRule> | ref<MacroDeclRule> |
              ref<DirectiveRule>) +
             lit<";">> {};

  // ---- Declarations ---------------------------------------------------

  struct ConstantDeclRule
      : rule<create<ast::ConstantDecl>(
            ikw<"CONSTANT"> + assign<&ast::ConstantDecl::name>(Id) +
            ((lit<"="> + assign<&ast::ConstantDecl::value>(ref<Expression>)) |
             (ikw<"EXTERNAL"> +
              assign<&ast::ConstantDecl::externalType>(ref<TypeDecRule>)) |
             (flag<&ast::ConstantDecl::isGlobal>(ref<GLOBAL>) +
              lit<"="> +
              assign<&ast::ConstantDecl::value>(ref<Expression>))))> {};

  struct VariableDeclRule
      : rule<create<ast::VariableDecl>(
            ikw<"DECLARE"> +
            some(append<&ast::VariableDecl::names>(PicId), lit<",">) +
            lit<":"> +
            option(assign<&ast::VariableDecl::storageAttr>(
                ref<STATIC> | ref<AUTOMATIC> | ref<COMMON> | ref<GLOBAL> |
                ikw<"EXTERNAL">)) +
            assign<&ast::VariableDecl::type>(ref<TypeDecRule>))> {};

  struct TypeDeclRule
      : rule<create<ast::TypeDecl>(
            ikw<"TYPE"> + assign<&ast::TypeDecl::name>(Id) + lit<":"> +
            assign<&ast::TypeDecl::type>(ref<TypeDecRule>))> {};

  struct FormalRule
      : rule<create<ast::Formal>(
            option(assign<&ast::Formal::passingMech>(PassingMech)) +
            assign<&ast::Formal::type>(ref<TypeDecRule>))> {};

  struct ProcedureFormDeclRule
      : rule<create<ast::ProcedureFormDecl>(
                 ikw<"EXTERNAL"> + ikw<"PROCEDURE"> +
                 assign<&ast::ProcedureFormDecl::name>(Id) +
                 option(lit<"("> +
                        many(append<&ast::ProcedureFormDecl::formals>(
                                 ref<FormalRule>),
                             lit<",">) +
                        lit<")">) +
                 option(ref<OF> + assign<&ast::ProcedureFormDecl::resultType>(
                                       ref<TypeDecRule>))) |
             create<ast::ProcedureFormDecl>(
                 flag<&ast::ProcedureFormDecl::isForward>(
                     ikw<"FORWARD">) +
                 ikw<"PROCEDURE"> +
                 assign<&ast::ProcedureFormDecl::name>(Id) +
                 option(lit<"("> +
                        many(append<&ast::ProcedureFormDecl::formals>(
                                 ref<FormalRule>),
                             lit<",">) +
                        lit<")">) +
                 option(ref<OF> + assign<&ast::ProcedureFormDecl::resultType>(
                                       ref<TypeDecRule>)))> {};

  struct DeclarationRule
      : rule<ref<ConstantDeclRule> | ref<VariableDeclRule> |
             ref<TypeDeclRule> | ref<ProcedureFormDeclRule>> {};

  struct ParamRule
      : rule<create<ast::Param>(
            assign<&ast::Param::name>(Id) + lit<":"> +
            option(assign<&ast::Param::passingMech>(PassingMech)) +
            assign<&ast::Param::type>(ref<TypeDecRule>))> {};

  struct ProcedureDeclRule
      : rule<create<ast::ProcedureDecl>(
            ikw<"PROCEDURE"> + assign<&ast::ProcedureDecl::name>(Id) +
            option(flag<&ast::ProcedureDecl::isMain>(ref<MAIN>)) +
            option(lit<"("> +
                   many(append<&ast::ProcedureDecl::params>(ref<ParamRule>),
                        lit<",">) +
                   lit<")">) +
            option(ref<OF> +
                   assign<&ast::ProcedureDecl::resultType>(ref<TypeDecRule>)) +
            lit<";"> +
            many(append<&ast::ProcedureDecl::body>(ref<StatementRule>)) +
            ikw<"END"> + ikw<"PROCEDURE">)> {};

  struct MacroDeclRule
      : rule<create<ast::MacroDecl>(
            ikw<"MACRO"> + assign<&ast::MacroDecl::name>(Id) +
            assign<&ast::MacroDecl::kind>(ikw<"TRIGGER"> | ref<SYNTAX>) +
            option(flag<&ast::MacroDecl::expose>(ref<EXPOSE>)) +
            lit<"{"> +
            option(assign<&ast::MacroDecl::picture>(ref<PictureRule>)) +
            lit<"}"> + lit<";"> +
            many(append<&ast::MacroDecl::body>(ref<StatementRule>)) +
            ikw<"END"> + ikw<"MACRO">)> {};

  struct SetDeclRule
      : rule<create<ast::SetDecl>(
            ikw<"SET"> + assign<&ast::SetDecl::name>(Id) + lit<"("> +
            assign<&ast::SetDecl::expr>(ref<SetExprRule>) + lit<")">)> {};

  struct GroupDeclRule
      : rule<create<ast::GroupDecl>(
            ikw<"GROUP"> + assign<&ast::GroupDecl::name>(Id) + lit<"("> +
            assign<&ast::GroupDecl::expr>(ref<GroupExprRule>) +
            lit<")">)> {};

  struct CollateDirectiveRule
      : rule<ikw<"COLLATE"> +
             (create<ast::CollateDirective>(
                  assign<&ast::CollateDirective::seq>(
                      ref<NATIVE> | ref<ASCII> | ref<EBCDIC> |
                      ref<DEC_MULTI>)) |
              create<ast::CollateDirective>(
                  assign<&ast::CollateDirective::seq>(ref<USER>) +
                  assign<&ast::CollateDirective::userSeq>(
                      ref<UserSeqRule>)))> {};

  struct RedefineDirectiveRule
      : rule<create<ast::RedefineDirective>(
            ikw<"REDEFINE"> +
            assign<&ast::RedefineDirective::charLiteral>(ref<SCAN_LITERAL>) +
            lit<"="> +
            assign<&ast::RedefineDirective::ctrlChar>(
                ref<CTRL_CHAR_LITERAL>))> {};

  struct ModuleStatementRule
      : rule<ref<DeclarationRule> | ref<SetDeclRule> | ref<TokenDeclRule> |
             ref<GroupDeclRule> | ref<ProcedureDeclRule> |
             ref<MacroDeclRule> | ref<DirectiveRule> |
             ref<CollateDirectiveRule> | ref<RedefineDirectiveRule>> {};

  struct ModuleRule
      : rule<create<ast::Module>(
            ikw<"MODULE"> + assign<&ast::Module::name>(Id) +
            (lit<";"> |
             (ref<IDENT> + assign<&ast::Module::ident>(ref<CHARACTER_LITERAL>) +
              lit<";">)) +
            some(append<&ast::Module::statements>(ref<ModuleStatementRule>) +
                 lit<";">) +
            ikw<"END"> + ikw<"MODULE"> + lit<";">)> {};

  using entry = ModuleRule;
  using ignore = WS;
  using hidden = list<BangComment, BlockComment>;
};

}  // namespace vaxscan
