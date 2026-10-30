#pragma once
#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "wren/core/ast.hpp"

namespace wren {
inline constexpr auto stringValue = [](std::string_view text, std::pmr::memory_resource* mem) {
  if (text.find('\\') == text.npos) return text;
  auto room = cursiv::text_room(mem, text.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (text[i] != '\\' || i + 1 == text.size()) { room[n++] = text[i]; continue; }
    const char c = text[++i];
    switch (c) {
      case '0': room[n++] = '\0'; break;
      case 'a': room[n++] = '\a'; break;
      case 'b': room[n++] = '\b'; break;
      case 'e': room[n++] = '\33'; break;
      case 'f': room[n++] = '\f'; break;
      case 'n': room[n++] = '\n'; break;
      case 'r': room[n++] = '\r'; break;
      case 't': room[n++] = '\t'; break;
      case 'v': room[n++] = '\v'; break;
      case '"': case '\\': case '%': room[n++] = c; break;
      case 'x': case 'u': case 'U': {
        const unsigned digits = c == 'x' ? 2 : c == 'u' ? 4 : 8;
        std::uint32_t value = 0;
        bool valid = i + digits < text.size();
        for (unsigned d = 1; valid && d <= digits; ++d) {
          const char h = text[i + d];
          const unsigned v = h >= '0' && h <= '9' ? h - '0'
                             : h >= 'a' && h <= 'f' ? h - 'a' + 10
                             : h >= 'A' && h <= 'F' ? h - 'A' + 10 : 16;
          valid = v < 16;
          value = (value << 4) | v;
        }
        if (!valid) { room[n++] = '\\'; room[n++] = c; break; }
        i += digits;
        if (c == 'x') room[n++] = static_cast<char>(value);
        else if (value <= 0x7f) room[n++] = static_cast<char>(value);
        else if (value <= 0x7ff) {
          room[n++] = static_cast<char>(0xc0 | (value >> 6));
          room[n++] = static_cast<char>(0x80 | (value & 63));
        } else if (value <= 0xffff) {
          room[n++] = static_cast<char>(0xe0 | (value >> 12));
          room[n++] = static_cast<char>(0x80 | ((value >> 6) & 63));
          room[n++] = static_cast<char>(0x80 | (value & 63));
        } else if (value <= 0x10ffff) {
          room[n++] = static_cast<char>(0xf0 | (value >> 18));
          room[n++] = static_cast<char>(0x80 | ((value >> 12) & 63));
          room[n++] = static_cast<char>(0x80 | ((value >> 6) & 63));
          room[n++] = static_cast<char>(0x80 | (value & 63));
        }
        break;
      }
      default: room[n++] = '\\'; room[n++] = c; break;
    }
  }
  return std::string_view(room.data(), n);
};
inline constexpr auto quotedValue = [](std::string_view text, std::pmr::memory_resource* mem) {
  if (text.size() < 2) return text;
  return stringValue(text.substr(1, text.size() - 2), mem);
};
inline constexpr auto rawBody = [](std::string_view text) {
  if (text.size() < 3) return text;
  auto inner = text.substr(0, text.size() - 3);
  const auto first = inner.find('\n');
  const auto last = inner.rfind('\n');
  std::size_t begin = 0, end = inner.size();
  if (first != inner.npos && inner.substr(0, first).find_first_not_of(" \t") == inner.npos)
    begin = first + 1;
  if (last != inner.npos && inner.substr(last + 1).find_first_not_of(" \t") == inner.npos)
    end = last;
  return inner.substr(begin, end > begin ? end - begin : 0);
};

struct Wren : cursiv::GrammarSpec {
  static constexpr auto word_boundary = cr<"a-zA-Z0-9_">;
  struct WS : terminal<some(cr<R"( \t\r)">)> {};
  struct NL : terminal<as<[] { return std::string_view{}; }>(lit<"\n">)> {};
  struct ML_COMMENT : terminal<lit<"/*"> +
      many(ref<ML_COMMENT> | (!lit<"/*"> + !lit<"*/"> + any)) + lit<"*/">> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\n)">)> {};
  struct Shebang : terminal<lit<"#!/"> + many(cr<R"(^\n)">)> {};
  static constexpr auto NLs = many(ref<NL>);
  static constexpr auto Lines = some(ref<NL>);
  static constexpr auto Comma = lit<","> + NLs;
  static constexpr auto Reserved =
      kw<"as"> | kw<"break"> | kw<"class"> | kw<"construct"> | kw<"continue"> |
      kw<"else"> | kw<"false"> | kw<"for"> | kw<"foreign"> | kw<"if"> |
      kw<"import"> | kw<"in"> | kw<"is"> | kw<"null"> | kw<"return"> |
      kw<"static"> | kw<"super"> | kw<"this"> | kw<"true"> | kw<"var"> | kw<"while">;
  struct IdentifierToken : terminal<cr<"a-zA-Z_"> + many(cr<"a-zA-Z0-9_">)> {};
  struct ID : rule<as<[](std::string_view s) { return s; }>(
                  !Reserved + ref<IdentifierToken>)> {};
  static constexpr auto Digits = some(cr<"0-9">);
  struct NUMBER : terminal<(lit<"0x"> + some(cr<"0-9a-fA-F">)) |
                            (Digits + option(lit<"."> + Digits) +
                             option(cr<"eE"> + option(cr<"+-">) + Digits))> {};
  static constexpr auto Hex = cr<"0-9a-fA-F">;
  static constexpr auto Escape = lit<"\\"> +
      (cr<R"(0abefnrtv"\\%)"> | (lit<"x"> + repeat<2>(Hex)) |
       (lit<"u"> + repeat<4>(Hex)) | (lit<"U"> + repeat<8>(Hex)));
  static constexpr auto StringChar = cr<R"(^"\\%)"> | Escape;
  struct StringText : terminal<as<stringValue>(some(StringChar))> {};
  struct STRING_LITERAL
      : terminal<as<quotedValue>(lit<"\""> + many(StringChar) + lit<"\"">)> {};
  struct TEXT_BLOCK : terminal<as<[](std::string_view text) {
    return text.size() < 3 ? text : rawBody(text.substr(3));
  }>(through(lit<"\"\"\"">, lit<"\"\"\"">))> {};
  struct RawBody
      : terminal<as<rawBody>(many(!lit<"\"\"\""> + any) + lit<"\"\"\"">)> {};

  struct Expression;
  struct Block;
  struct FileItem;
  struct StringTextRule : rule<create<ast::StringText>(
      assign<&ast::StringText::value>(ref<StringText>))> {};
  struct RawText : rule<create<ast::StringText>(
      assign<&ast::StringText::value>(ref<RawBody>))> {};
  struct StringInterpolation : rule<create<ast::StringInterpolation>(
      skip(ignore<WS>, hide<SL_COMMENT, ML_COMMENT>, lit<"%("> + NLs +
      assign<&ast::StringInterpolation::expression>(ref<Expression>) + NLs + lit<")">))> {};
  struct StringExpr : rule<create<ast::StringExpr>(
      (flag<&ast::StringExpr::raw>(lit<"\"\"\"">) + tight(append<&ast::StringExpr::parts>(ref<RawText>))) |
      (lit<"\""> + tight(many(append<&ast::StringExpr::parts>(ref<StringTextRule> | ref<StringInterpolation>)) +
          lit<"\"">)))> {};
  struct NameExpr : rule<create<ast::NameExpr>(
      assign<&ast::NameExpr::name>(ref<ID> | kw<"this"> | kw<"super">))> {};
  struct NullExpr : rule<create<ast::NullExpr>(kw<"null">)> {};
  struct BoolExpr : rule<create<ast::BoolExpr>(
      flag<&ast::BoolExpr::value>(kw<"true">) | kw<"false">)> {};
  struct NumExpr : rule<create<ast::NumExpr>(assign<&ast::NumExpr::text>(ref<NUMBER>))> {};
  struct BreakExpr : rule<create<ast::BreakExpr>(kw<"break">)> {};
  struct ContinueExpr : rule<create<ast::ContinueExpr>(kw<"continue">)> {};
  struct ListExpr : rule<create<ast::ListExpr>(lit<"["> + NLs +
      option(some(append<&ast::ListExpr::items>(ref<Expression>) + NLs, Comma) + option(Comma)) + lit<"]">)> {};
  struct MapEntry : rule<create<ast::MapEntry>(
      assign<&ast::MapEntry::key>(ref<Expression>) + lit<":"> + NLs +
      assign<&ast::MapEntry::value>(ref<Expression>))> {};
  struct MapExpr : rule<create<ast::MapExpr>(lit<"{"> + NLs +
      option(some(append<&ast::MapExpr::entries>(ref<MapEntry>) + NLs, Comma) + option(Comma)) + lit<"}">)> {};
  struct ImportVariable : rule<create<ast::ImportVariable>(assign<&ast::ImportVariable::name>(ref<ID>) +
      option(kw<"as"> + assign<&ast::ImportVariable::alias>(ref<ID>)))> {};
  struct ImportExpr : rule<create<ast::ImportExpr>(kw<"import"> + NLs +
      assign<&ast::ImportExpr::module>(ref<STRING_LITERAL> | ref<TEXT_BLOCK>) +
      option(kw<"for"> + NLs + some(append<&ast::ImportExpr::variables>(ref<ImportVariable>), Comma)))> {};
  struct Primary : rule<ref<NameExpr> |
      (lit<"("> + NLs + create<ast::ParenExpr>(assign<&ast::ParenExpr::inner>(ref<Expression>)) + NLs + lit<")">) |
      ref<NullExpr> | ref<BoolExpr> | ref<StringExpr> | ref<NumExpr> |
      ref<ListExpr> | ref<MapExpr> | ref<BreakExpr> | ref<ContinueExpr> | ref<ImportExpr>> {};
  static constexpr auto CallArgs = NLs +
      option(some(append<&ast::CallExpr::args>(ref<Expression>) + NLs, Comma));
  static constexpr auto FieldSuffix =
      nest<ast::FieldExpr, &ast::FieldExpr::object>(lit<"."> + NLs + assign<&ast::FieldExpr::field>(ref<ID>));
  static constexpr auto CallSuffix =
      nest<ast::CallExpr, &ast::CallExpr::callee>(lit<"("> + CallArgs + lit<")">);
  static constexpr auto BlockSuffix =
      nest<ast::BlockCallExpr, &ast::BlockCallExpr::callee>(assign<&ast::BlockCallExpr::block>(ref<Block>));
  static constexpr auto IndexSuffix =
      nest<ast::IndexExpr, &ast::IndexExpr::object>(lit<"["> + NLs +
          some(append<&ast::IndexExpr::indices>(ref<Expression>) + NLs, Comma) + lit<"]">);
  struct Postfix : rule<ref<Primary> + many(FieldSuffix | CallSuffix | BlockSuffix | IndexSuffix)> {};
  struct Superclass : rule<
      create<ast::UnaryExpr>(assign<&ast::UnaryExpr::op>(lit<"-"> | lit<"!"> | lit<"~">) +
                            NLs + assign<&ast::UnaryExpr::operand>(ref<Superclass>)) |
      (ref<Primary> + many((FieldSuffix + many(CallSuffix) + option(BlockSuffix)) | CallSuffix | IndexSuffix))> {};
  struct Unary : rule<create<ast::UnaryExpr>(assign<&ast::UnaryExpr::op>(lit<"-"> | lit<"!"> | lit<"~">) +
      NLs + assign<&ast::UnaryExpr::operand>(ref<Unary>)) | ref<Postfix>> {};
  template <auto Tokens>
  struct Op : rule<as<[](std::string_view s) { return s; }>(Tokens + NLs)> {};
  struct BinaryExpression : infix<Unary, ast::BinaryExpr,
      levels(left(ref<Op<lit<"*"> | lit<"/"> | lit<"%">>>),
             left(ref<Op<lit<"+"> | lit<"-">>>),
             left(ref<Op<lit<"..."> | lit<"..">>>),
             left(ref<Op<lit<"<<"> | lit<">>">>>),
             left(ref<Op<lit<"&">>>), left(ref<Op<lit<"^">>>), left(ref<Op<lit<"|">>>),
             left(ref<Op<lit<"<"> | lit<">"> | lit<"<="> | lit<">=">>>),
             left(ref<Op<kw<"is">>>), left(ref<Op<lit<"=="> | lit<"!=">>>),
             left(ref<Op<lit<"&&">>>), left(ref<Op<lit<"||">>>))> {};
  struct Expression : rule<ref<BinaryExpression> + option(
      nest<ast::AssignStat, &ast::AssignStat::target>(
          assign<&ast::AssignStat::op>(lit<"=", !lit<"=">>) + NLs +
          assign<&ast::AssignStat::value>(ref<Expression>)) |
      nest<ast::CondExpr, &ast::CondExpr::cond>(lit<"?"> + NLs +
          assign<&ast::CondExpr::thenValue>(ref<Expression>) + lit<":"> + NLs +
          assign<&ast::CondExpr::elseValue>(ref<Expression>)))> {};
  struct Assignment : rule<create<ast::AssignStat>(flag<&ast::AssignStat::isVar>(kw<"var">) +
      assign<&ast::AssignStat::target>(ref<NameExpr>) +
      option(assign<&ast::AssignStat::op>(lit<"=", !lit<"=">>) + NLs + assign<&ast::AssignStat::value>(ref<Expression>)))> {};
  struct ExprStat : rule<create<ast::ExprStat>(assign<&ast::ExprStat::expr>(ref<Assignment> | ref<Expression>))> {};
  struct Statement;
  struct IfClause : rule<create<ast::IfClause>(kw<"if"> + lit<"("> +
      assign<&ast::IfClause::cond>(ref<Expression>) + lit<")"> + assign<&ast::IfClause::body>(ref<Statement>))> {};
  struct IfStat : rule<create<ast::IfStat>(append<&ast::IfStat::clauses>(ref<IfClause>) +
      many(kw<"else"> + append<&ast::IfStat::clauses>(ref<IfClause>)) +
      option(kw<"else"> + assign<&ast::IfStat::elseBody>(ref<Statement>)))> {};
  struct WhileStat : rule<create<ast::WhileStat>(kw<"while"> + lit<"("> +
      assign<&ast::WhileStat::cond>(ref<Expression>) + lit<")"> + assign<&ast::WhileStat::body>(ref<Statement>))> {};
  struct ForStat : rule<create<ast::ForStat>(kw<"for"> + lit<"("> + assign<&ast::ForStat::var>(ref<ID>) +
      kw<"in"> + assign<&ast::ForStat::iterable>(ref<Expression>) + lit<")"> +
      assign<&ast::ForStat::body>(ref<Statement>))> {};
  struct Block : rule<create<ast::Block>(lit<"{"> +
      option(lit<"|"> + some(append<&ast::Block::params>(ref<ID>), Comma) + lit<"|">) +
      ((Lines + many(append<&ast::Block::statements>(ref<FileItem>) + Lines) +
                option(append<&ast::Block::statements>(ref<FileItem>)) + lit<"}">) |
       (append<&ast::Block::statements>(ref<ExprStat>) + flag<&ast::Block::expressionBody>(lit<"}">)) |
       lit<"}">))> {};
  struct Statement : rule<ref<IfStat> | ref<WhileStat> | ref<ForStat> |
      create<ast::ReturnStat>(kw<"return"> + option(assign<&ast::ReturnStat::value>(ref<Expression>))) |
      create<ast::BlockStat>(assign<&ast::BlockStat::body>(ref<Block>)) | ref<ExprStat>> {};
  struct AttrValue : rule<create<ast::AttrValue>(assign<&ast::AttrValue::name>(ref<ID>) +
      option(lit<"="> + assign<&ast::AttrValue::value>(ref<Primary>)))> {};
  struct Attribute : rule<create<ast::Attribute>(lit<"#"> + option(flag<&ast::Attribute::runtime>(lit<"!">)) +
      ((assign<&ast::Attribute::group>(ref<ID>) + lit<"("> + NLs +
        some(append<&ast::Attribute::values>(ref<AttrValue>) + NLs, Comma) + option(Comma) + lit<")">) |
       append<&ast::Attribute::values>(ref<AttrValue>)) + Lines)> {};
  static constexpr auto ForeignStatic = option(flag<&ast::Member::isForeign>(kw<"foreign">)) +
                                       option(flag<&ast::Member::isStatic>(kw<"static">));
  static constexpr auto UnaryOp = lit<"-"> | lit<"~"> | lit<"!">;
  static constexpr auto BinaryOp = lit<"-"> | lit<"*"> | lit<"/"> | lit<"%"> | lit<"+"> |
      lit<"..."> | lit<".."> | lit<"<<"> | lit<">>"> | lit<"&"> | lit<"^"> | lit<"|"> |
      lit<">"> | lit<"<"> | lit<"=="> | lit<"<="> | lit<">="> | lit<"!="> | kw<"is">;
  static constexpr auto ConstructorDefBody = ForeignStatic + kw<"construct"> +
      assign<&ast::ConstructorDef::name>(ref<ID>) + lit<"("> + NLs +
      option(some(append<&ast::ConstructorDef::params>(ref<ID>), Comma)) + lit<")"> +
      option(assign<&ast::ConstructorDef::body>(ref<Block>));
  static constexpr auto MethodDefBody = ForeignStatic + assign<&ast::MethodDef::name>(ref<ID>) +
      lit<"("> + NLs + option(some(append<&ast::MethodDef::params>(ref<ID>), Comma)) + lit<")"> +
      option(assign<&ast::MethodDef::body>(ref<Block>));
  static constexpr auto GetterDefBody = ForeignStatic +
      assign<&ast::GetterDef::name>(UnaryOp | ref<ID>) + option(assign<&ast::GetterDef::body>(ref<Block>));
  static constexpr auto SetterDefBody = ForeignStatic +
      ((assign<&ast::SetterDef::name>(ref<ID>) + lit<"=">) | assign<&ast::SetterDef::name>(BinaryOp)) +
      lit<"("> + NLs + assign<&ast::SetterDef::param>(ref<ID>) + NLs + lit<")"> +
      option(assign<&ast::SetterDef::body>(ref<Block>));
  static constexpr auto SubscriptGetDefBody = ForeignStatic + lit<"["> + NLs +
      some(append<&ast::SubscriptGetDef::params>(ref<ID>), Comma) + lit<"]"> +
      option(assign<&ast::SubscriptGetDef::body>(ref<Block>));
  static constexpr auto SubscriptSetDefBody = ForeignStatic + lit<"["> + NLs +
      some(append<&ast::SubscriptSetDef::params>(ref<ID>), Comma) + lit<"]"> + lit<"="> +
      lit<"("> + NLs + assign<&ast::SubscriptSetDef::valueParam>(ref<ID>) + NLs + lit<")"> +
      option(assign<&ast::SubscriptSetDef::body>(ref<Block>));
  struct Member : rule<many(append<&ast::Member::attributes>(ref<Attribute>)) +
      (create<ast::ConstructorDef>(ConstructorDefBody) |
       create<ast::SubscriptSetDef>(SubscriptSetDefBody) |
       create<ast::SubscriptGetDef>(SubscriptGetDefBody) |
       create<ast::SetterDef>(SetterDefBody) |
       create<ast::GetterDef>(GetterDefBody) |
       create<ast::MethodDef>(MethodDefBody))> {};
  struct ClassDef : rule<create<ast::ClassDef>(many(append<&ast::ClassDef::attributes>(ref<Attribute>)) +
      option(flag<&ast::ClassDef::isForeign>(kw<"foreign">)) + kw<"class"> + assign<&ast::ClassDef::name>(ref<ID>) +
      option(kw<"is"> + assign<&ast::ClassDef::superclass>(ref<Superclass>)) + lit<"{"> + NLs +
      many(append<&ast::ClassDef::members>(ref<Member>) + Lines) +
      option(append<&ast::ClassDef::members>(ref<Member>)) + lit<"}">)> {};
  struct FileItem : rule<ref<ClassDef> | ref<Statement>> {};
  struct Script : rule<create<ast::Script>(option(ref<Shebang>) + NLs +
      many(append<&ast::Script::statements>(ref<FileItem>) + Lines) +
      option(append<&ast::Script::statements>(ref<FileItem>)))> {};
  using entry = Script;
  using ignored = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};
}  // namespace wren
