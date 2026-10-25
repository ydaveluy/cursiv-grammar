#pragma once

#include <cstdint>
#include <span>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "ocl/core/ast.hpp"

namespace ocl {

inline constexpr auto strip_last = [](std::string_view t) {
  return t.substr(0, t.size() - 1);
};
using StripLast = cursiv::text_conv<strip_last>;

// STRING1_LITERAL and STRING2_LITERAL share one escape grammar: strip the
// surrounding quote, then decode \b\t\n\f\r\"\'\\ , octal and \uXXXX.
inline constexpr auto unescape = [](std::string_view t,
                                     std::pmr::memory_resource* mem) {
  const std::string_view body = t.substr(1, t.size() - 2);
  if (body.find('\\') == std::string_view::npos) return body;
  const std::span<char> room = cursiv::text_room(mem, body.size() * 3);
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    if (body[i] != '\\') {
      room[n++] = body[i];
      continue;
    }
    const char e = body[++i];
    switch (e) {
      case 'b':
        room[n++] = '\b';
        break;
      case 't':
        room[n++] = '\t';
        break;
      case 'n':
        room[n++] = '\n';
        break;
      case 'f':
        room[n++] = '\f';
        break;
      case 'r':
        room[n++] = '\r';
        break;
      case '"':
      case '\'':
      case '\\':
        room[n++] = e;
        break;
      case 'u': {
        unsigned cp = 0;
        for (int k = 0; k < 4 && i + 1 < body.size(); ++k) {
          const char h = body[++i];
          cp <<= 4;
          if (h >= '0' && h <= '9')
            cp |= static_cast<unsigned>(h - '0');
          else if (h >= 'a' && h <= 'f')
            cp |= static_cast<unsigned>(h - 'a' + 10);
          else if (h >= 'A' && h <= 'F')
            cp |= static_cast<unsigned>(h - 'A' + 10);
        }
        if (cp < 0x80) {
          room[n++] = static_cast<char>(cp);
        } else if (cp < 0x800) {
          room[n++] = static_cast<char>(0xC0 | (cp >> 6));
          room[n++] = static_cast<char>(0x80 | (cp & 0x3F));
        } else {
          room[n++] = static_cast<char>(0xE0 | (cp >> 12));
          room[n++] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
          room[n++] = static_cast<char>(0x80 | (cp & 0x3F));
        }
        break;
      }
      default: {
        unsigned v = static_cast<unsigned>(e - '0');
        for (int k = 0; k < 2 && i + 1 < body.size() && body[i + 1] >= '0' &&
                        body[i + 1] <= '7';
             ++k)
          v = v * 8 + static_cast<unsigned>(body[++i] - '0');
        room[n++] = static_cast<char>(v);
        break;
      }
    }
  }
  return std::string_view(room.data(), n);
};
using Unescaped = cursiv::text_conv<unescape>;

struct Ocl : cursiv::GrammarSpec {
  // ID runs past `$` and `@`, past the default word class: a keyword must
  // refuse those too, or `context$x` would misread as `context` + `$x`.
  static constexpr auto word_boundary = cr<"a-zA-Z0-9_$@">;

  struct WS : terminal<some(s)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  struct LINE_COMMENT : terminal<lit<"--"> + many(cr<R"(^\r\n)">)> {};

  static constexpr auto IdBody = cr<"a-zA-Z_$"> + many(cr<"a-zA-Z0-9_$@">);
  struct ID : terminal<IdBody> {};
  struct ENUMERATION_LITERAL : terminal<IdBody + lit<"::"> + IdBody> {};

  static constexpr auto Digits = some(d);
  struct INT : terminal<Digits, std::int64_t> {};
  struct FLOAT_LITERAL : terminal<Digits + lit<"."> + Digits, double> {};

  static constexpr auto EscapeSequence =
      lit<"\\"> + (cr<R"(btnfr"'\\)"> |
                   (option(option(cr<"0-3">) + cr<"0-7">) + cr<"0-7">) |
                   (lit<"u"> + repeat<4>(cr<"0-9a-fA-F">)));
  struct STRING1_LITERAL
      : terminal<lit<"\""> + many(EscapeSequence | cr<R"(^"\\\r\n)">) +
                     lit<"\"">,
                 Unescaped> {};
  struct STRING2_LITERAL
      : terminal<lit<"'"> + many(EscapeSequence | cr<R"(^'\\\r\n)">) + lit<"'">,
                 Unescaped> {};

  struct COLLECTION_OPEN
      : terminal<lit<"OrderedSet{"> | lit<"Bag{"> | lit<"Set{"> |
                     lit<"Sequence{"> | lit<"Map{">,
                 StripLast> {};

  struct Expression;
  struct TypeRule;
  struct IdentOptType;

  struct TypeRule
      : rule<create<ast::ParamType>(
                 assign<&ast::ParamType::keyword>(
                     kw<"Sequence"> | kw<"Set"> | kw<"Bag"> |
                     kw<"OrderedSet"> | kw<"Ref">) +
                 lit<"("> + append<&ast::ParamType::args>(ref<TypeRule>) +
                 lit<")">)  //
             | create<ast::ParamType>(
                   assign<&ast::ParamType::keyword>(kw<"Map"> |
                                                     kw<"Function">) +
                   lit<"("> + append<&ast::ParamType::args>(ref<TypeRule>) +
                   lit<","> + append<&ast::ParamType::args>(ref<TypeRule>) +
                   lit<")">)  //
             | create<ast::NamedType>(
                   assign<&ast::NamedType::name>(ref<ID>))> {};

  struct IdentOptType
      : rule<create<ast::IdentOptType> +
             assign<&ast::IdentOptType::name>(ref<ID>) +
             option(lit<":"> +
                    assign<&ast::IdentOptType::type>(ref<TypeRule>))> {};

  // Literals and names -- the leaves a postfix chain climbs from.
  struct PrimaryFactor
      : rule<create<ast::NullLit>(kw<"null">)  //
             | create<ast::BoolLit>(
                   assign<&ast::BoolLit::value>(kw<"true"> | kw<"false">))  //
             | create<ast::IntLit>(assign<&ast::IntLit::value>(ref<INT>))  //
             | create<ast::FloatLit>(
                   assign<&ast::FloatLit::value>(ref<FLOAT_LITERAL>))  //
             | create<ast::StringLit>(assign<&ast::StringLit::value>(
                   ref<STRING1_LITERAL> | ref<STRING2_LITERAL>))  //
             | create<ast::EnumLit>(
                   assign<&ast::EnumLit::text>(ref<ENUMERATION_LITERAL>))  //
             | create<ast::SetLiteral>(
                   assign<&ast::SetLiteral::kind>(ref<COLLECTION_OPEN>) +
                   many(append<&ast::SetLiteral::items>(ref<Expression>),
                        lit<",">) +
                   lit<"}">)  //
             | create<ast::DirectCall>(
                   assign<&ast::DirectCall::name>(ref<ID>) + lit<"("> +
                   many(append<&ast::DirectCall::args>(ref<Expression>),
                        lit<",">) +
                   lit<")">)  //
             | create<ast::NameRef>(
                   assign<&ast::NameRef::name>(ref<ID>))  //
             | (lit<"("> + ref<Expression> + lit<")">)> {};

  // One postfix chain, not two separate suffix loops over the same span:
  // two derivations of `a.b` would fork the parse on every occurrence. A
  // bare call (`name(args)`) is a PrimaryFactor, not a step here, for the
  // same reason: chained onto any suffix it would again derive `.n(a)` two
  // ways.
  struct NavigationExpression
      : rule<ref<PrimaryFactor> +
             many(
                 nest<ast::Call, &ast::Call::object>(
                     (lit<"."> | flag<&ast::Call::arrow>(lit<"->">)) +
                     assign<&ast::Call::name>(ref<ID>) + lit<"("> +
                     option(some(append<&ast::Call::params>(ref<IdentOptType>),
                                 lit<",">) +
                            lit<"|">) +
                     many(append<&ast::Call::args>(ref<Expression>),
                          lit<",">) +
                     lit<")">)  //
                 | nest<ast::IterateCall, &ast::IterateCall::object>(
                       lit<"->"> + kw<"iterate"> + lit<"("> +
                       assign<&ast::IterateCall::accumVar>(ref<ID>) +
                       lit<";"> +
                       assign<&ast::IterateCall::accumParam>(
                           ref<IdentOptType>) +
                       lit<"="> +
                       assign<&ast::IterateCall::init>(ref<Expression>) +
                       lit<"|"> +
                       assign<&ast::IterateCall::body>(ref<Expression>) +
                       lit<")">)  //
                 | nest<ast::MemberAccess, &ast::MemberAccess::object>(
                       lit<"."> +
                       assign<&ast::MemberAccess::name>(ref<ID>))  //
                 | nest<ast::IndexExpr, &ast::IndexExpr::object>(
                       lit<"["> +
                       assign<&ast::IndexExpr::index>(ref<Expression>) +
                       lit<"]">))> {};

  struct UnaryExpression
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(kw<"not"> | lit<"-"> | lit<"+"> |
                                             lit<"?"> | lit<"!">) +
                 assign<&ast::UnaryExpr::operand>(ref<UnaryExpression>))  //
             | ref<NavigationExpression>> {};

  // Eight levels, tightest to loosest: multiplicative,
  // additive, relational, equality, `and`, `or`, `xor`, `implies`.
  // `<:`/`/:` sit with relational, `:` with equality.
  struct LogicalExpression
      : infix<UnaryExpression, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(lit<"*"> | lit<"/"> | kw<"mod"> | kw<"div">) >>
               left(lit<"+"> | lit<"-"> | lit<".."> | lit<"|->">) >>
               left(lit<"<="> | lit<">="> | lit<"<"> | lit<">"> |
                    lit<"<:"> | lit<"/:">) >>
               left(lit<"="> | lit<"<>"> | lit<"/="> | lit<":">) >>
               left(kw<"and"> | lit<"&">) >>
               left(kw<"or">) >>
               left(kw<"xor">) >>
               left(lit<"=>"> | kw<"implies">))> {};

  struct ConditionalExpression
      : rule<kw<"if"> + assign<&ast::IfExpr::cond>(ref<Expression>) +
             kw<"then"> + assign<&ast::IfExpr::thenExpr>(ref<Expression>) +
             kw<"else"> + assign<&ast::IfExpr::elseExpr>(ref<Expression>) +
             kw<"endif">> {};

  struct LetBindingRule
      : rule<create<ast::LetBinding> +
             assign<&ast::LetBinding::name>(ref<ID>) +
             option(lit<":"> +
                    assign<&ast::LetBinding::type>(ref<TypeRule>)) +
             lit<"="> + assign<&ast::LetBinding::value>(ref<Expression>)> {};

  struct LetExpression
      : rule<kw<"let"> +
             some(append<&ast::LetExpr::bindings>(ref<LetBindingRule>),
                  lit<",">) +
             kw<"in"> + assign<&ast::LetExpr::body>(ref<Expression>)> {};

  struct Expression
      : rule<ref<ConditionalExpression>  //
             | ref<LetExpression>        //
             | ref<LogicalExpression>> {};

  struct SingleInvariant
      : rule<kw<"context"> +
             assign<&ast::Invariant::contextName>(ref<ID>) + kw<"inv"> +
             option(assign<&ast::Invariant::name>(ref<ID>)) + lit<":"> +
             assign<&ast::Invariant::body>(ref<Expression>)> {};

  struct SingleDerivedAttribute
      : rule<kw<"context"> +
             assign<&ast::DerivedAttribute::qualifiedName>(
                 ref<ENUMERATION_LITERAL>) +
             lit<":"> +
             assign<&ast::DerivedAttribute::type>(ref<TypeRule>) +
             option(lit<"init:"> + assign<&ast::DerivedAttribute::init>(
                                       ref<Expression>)) +
             lit<"derive:"> +
             assign<&ast::DerivedAttribute::derive>(ref<Expression>)> {};

  struct ContextSpec
      : rule<ref<SingleInvariant> | ref<SingleDerivedAttribute>> {};

  struct ModelRule
      : rule<create<ast::Model> +
             some(append<&ast::Model::specs>(ref<ContextSpec>))> {};

  using entry = ModelRule;
  using ignore = WS;
  using hidden = list<ML_COMMENT, LINE_COMMENT>;
};

}  // namespace ocl
