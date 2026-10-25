// The GQL language: its grammar. GQL (ISO/IEC 39075) layers catalog DDL,
// session/transaction control and a composable query pipeline over a shared
// value-expression and graph-pattern language. Lexing is per LR state, so
// most of GQL's own soft/contextual keywords need no special handling -- a
// word is a keyword only where the state also reads that keyword.
#pragma once

#include <cstdint>
#include <memory_resource>
#include <span>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "gql/core/ast.hpp"

namespace gql {

namespace detail {

constexpr unsigned hex_val(char c) {
  if (c >= '0' && c <= '9')
    return static_cast<unsigned>(c - '0');
  if (c >= 'a' && c <= 'f')
    return static_cast<unsigned>(c - 'a' + 10);
  return static_cast<unsigned>(c - 'A' + 10);
}

constexpr void append_utf8(std::span<char> room, std::size_t &n, unsigned cp) {
  if (cp < 0x80) {
    room[n++] = static_cast<char>(cp);
  } else if (cp < 0x800) {
    room[n++] = static_cast<char>(0xC0 | (cp >> 6));
    room[n++] = static_cast<char>(0x80 | (cp & 0x3F));
  } else if (cp < 0x10000) {
    room[n++] = static_cast<char>(0xE0 | (cp >> 12));
    room[n++] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
    room[n++] = static_cast<char>(0x80 | (cp & 0x3F));
  } else {
    room[n++] = static_cast<char>(0xF0 | (cp >> 18));
    room[n++] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
    room[n++] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
    room[n++] = static_cast<char>(0x80 | (cp & 0x3F));
  }
}

} // namespace detail

// A quoted sequence's content, one converter for all three delimiters
// (single/double/accent quote) and the optional leading `@` marker: the
// quote character is whatever the last byte of the match is, doubling it
// escapes it, and `\t \b \n \r \f \\ \' \" \` \uXXXX \UXXXXXX` decode as
// written. `@` is accepted but changes nothing here -- GQL's own lexical
// form does not condition the body on it either.
inline constexpr auto decode_quoted =
    [](std::string_view t, std::pmr::memory_resource *mem) -> std::string_view {
  std::size_t start = 0;
  if (!t.empty() && t.front() == '@')
    start = 1;
  const char q = t.back();
  const std::string_view body = t.substr(start + 1, t.size() - start - 2);
  if (body.find('\\') == std::string_view::npos &&
      body.find(q) == std::string_view::npos)
    return body;
  const std::span<char> room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    const char c = body[i];
    if (c == q && i + 1 < body.size() && body[i + 1] == q) {
      room[n++] = q;
      ++i;
    } else if (c == '\\' && i + 1 < body.size()) {
      const char e = body[i + 1];
      switch (e) {
      case '\\':
        room[n++] = '\\';
        ++i;
        break;
      case '\'':
        room[n++] = '\'';
        ++i;
        break;
      case '"':
        room[n++] = '"';
        ++i;
        break;
      case '`':
        room[n++] = '`';
        ++i;
        break;
      case 't':
        room[n++] = '\t';
        ++i;
        break;
      case 'b':
        room[n++] = '\b';
        ++i;
        break;
      case 'n':
        room[n++] = '\n';
        ++i;
        break;
      case 'r':
        room[n++] = '\r';
        ++i;
        break;
      case 'f':
        room[n++] = '\f';
        ++i;
        break;
      case 'u':
        if (i + 5 < body.size()) {
          unsigned cp = 0;
          for (int k = 0; k < 4; ++k)
            cp = (cp << 4) |
                 detail::hex_val(body[i + 2 + static_cast<std::size_t>(k)]);
          detail::append_utf8(room, n, cp);
          i += 5;
        }
        break;
      case 'U':
        if (i + 7 < body.size()) {
          unsigned cp = 0;
          for (int k = 0; k < 6; ++k)
            cp = (cp << 4) |
                 detail::hex_val(body[i + 2 + static_cast<std::size_t>(k)]);
          detail::append_utf8(room, n, cp);
          i += 7;
        }
        break;
      default:
        room[n++] = c;
        break;
      }
    } else {
      room[n++] = c;
    }
  }
  return std::string_view(room.data(), n);
};
using Unquoted = cursiv::text_conv<decode_quoted>;

struct Gql : cursiv::GrammarSpec {
  // ---- trivia -----------------------------------------------------------

  struct WS : terminal<some(s | cr<R"(\x1c-\x1f)"> | uc::Zs | uc::Zl | uc::Zp |
                            uc::Cf)> {};
  struct BLOCK_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  struct LINE_COMMENT_SLASH : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct LINE_COMMENT_DASH : terminal<lit<"--"> + many(cr<R"(^\r\n)">)> {};

  // ---- identifiers -------------------------------------------------------

  struct IDENT : terminal<(uc::XID_Start | uc::Pc) + many(uc::XID_Continue)> {};

  static constexpr auto NonReservedWordsAlt =
      ikw<"ACYCLIC"> | ikw<"BINDING"> | ikw<"BINDINGS"> | ikw<"CONNECTING"> |
      ikw<"DESTINATION"> | ikw<"DIFFERENT"> | ikw<"DIRECTED"> | ikw<"EDGE"> |
      ikw<"EDGES"> | ikw<"ELEMENT"> | ikw<"ELEMENTS"> | ikw<"FIRST"> |
      ikw<"GRAPH"> | ikw<"GROUPS"> | ikw<"KEEP"> | ikw<"LABEL"> |
      ikw<"LABELED"> | ikw<"LABELS"> | ikw<"LAST"> | ikw<"NFC"> | ikw<"NFD"> |
      ikw<"NFKC"> | ikw<"NFKD"> | ikw<"NO"> | ikw<"NODE"> | ikw<"NORMALIZED"> |
      ikw<"ONLY"> | ikw<"ORDINALITY"> | ikw<"PROPERTY"> | ikw<"READ"> |
      ikw<"RELATIONSHIP"> | ikw<"RELATIONSHIPS"> | ikw<"REPEATABLE"> |
      ikw<"SHORTEST"> | ikw<"SIMPLE"> | ikw<"SOURCE"> | ikw<"TABLE"> |
      ikw<"TO"> | ikw<"TRAIL"> | ikw<"TRANSACTION"> | ikw<"TYPE"> |
      ikw<"UNDIRECTED"> | ikw<"VERTEX"> | ikw<"WALK"> | ikw<"WITHOUT"> |
      ikw<"WRITE"> | ikw<"ZONE">;

  // A name may spell a non-reserved word: spelled once here, reused by every
  // construct that reads one of these words as a keyword.
  struct Ident : rule<ref<IDENT> | NonReservedWordsAlt> {};
  static constexpr auto RegularIdentifier = ref<Ident>;

  static constexpr auto HexDigit = cr<"0-9a-f">;
  static constexpr auto QuoteEscape =
      lit<"\\"> + (cr<R"(\\'"`tbnrf)"> | (lit<"u"> + repeat<4>(HexDigit)) |
                   (lit<"U"> + repeat<6>(HexDigit)));
  struct SINGLE_QUOTED
      : terminal<option(lit<"@">) + lit<"'"> +
                     many(QuoteEscape | cr<R"(^'\\\r\n)"> | lit<"''">) +
                     lit<"'">,
                 Unquoted> {};
  struct DOUBLE_QUOTED
      : terminal<option(lit<"@">) + lit<"\""> +
                     many(QuoteEscape | cr<R"(^"\\\r\n)"> | lit<"\"\"">) +
                     lit<"\"">,
                 Unquoted> {};
  struct ACCENT_QUOTED
      : terminal<option(lit<"@">) + lit<"`"> +
                     many(QuoteEscape | cr<R"(^`\\\r\n)"> | lit<"``">) +
                     lit<"`">,
                 Unquoted> {};

  // A name: an identifier, or one of these words used as one. `Identifier`
  // additionally admits a delimited (quoted) spelling.
  static constexpr auto Name = RegularIdentifier;
  static constexpr auto Identifier =
      RegularIdentifier | ref<DOUBLE_QUOTED> | ref<ACCENT_QUOTED>;

  static constexpr auto ExtendedIdentBody = some(uc::XID_Continue);
  static constexpr auto ParamNameBody =
      (lit<"\""> + many(QuoteEscape | cr<R"(^"\\\r\n)"> | lit<"\"\"">) +
       lit<"\"">) |
      (lit<"`"> + many(QuoteEscape | cr<R"(^`\\\r\n)"> | lit<"``">) +
       lit<"`">) |
      ExtendedIdentBody;
  struct GENERAL_PARAM : terminal<lit<"$"> + ParamNameBody> {};
  struct SUBSTITUTED_PARAM : terminal<lit<"$$"> + ParamNameBody> {};

  // ---- literals -----------------------------------------------------------

  struct BOOL_LIT : terminal<ikw<"TRUE"> | ikw<"FALSE"> | ikw<"UNKNOWN">> {};

  static constexpr auto DecDigits = d + many(option(lit<"_">) + d);
  static constexpr auto DecCommon =
      (DecDigits + option(lit<"."> + option(DecDigits))) |
      (lit<"."> + DecDigits);
  struct NUMERIC
      : terminal<DecCommon + option(cr<"eE"> + option(cr<"+-">) + DecDigits) +
                 option(cr<"mMfFdD">)> {};
  struct HEX_INT
      : terminal<lit<"0x"> + some(option(lit<"_">) + cr<"0-9a-fA-F">)> {};
  struct OCT_INT : terminal<lit<"0o"> + some(option(lit<"_">) + cr<"0-7">)> {};
  struct BIN_INT : terminal<lit<"0b"> + some(option(lit<"_">) + cr<"0-1">)> {};
  static constexpr auto NumberText =
      ref<HEX_INT> | ref<OCT_INT> | ref<BIN_INT> | ref<NUMERIC>;

  struct BYTE_STRING : terminal<cr<"xX"> + lit<"'"> + many(cr<" ">) +
                                many(cr<"0-9a-fA-F"> + many(cr<" ">) +
                                     cr<"0-9a-fA-F"> + many(cr<" ">)) +
                                lit<"'">> {};

  static constexpr auto StringLiteral = ref<SINGLE_QUOTED> | ref<DOUBLE_QUOTED>;

  static constexpr auto IMPLIES_TOK = lit<"=>"> | ikw<"IMPLIES">;

  // ---- literals -- rules ---------------------------------------------------

  struct NullLit : rule<create<ast::NullLit> + ikw<"NULL">> {};
  struct BoolLit : rule<assign<&ast::BoolLit::value>(ref<BOOL_LIT>)> {};
  struct NumberLit : rule<assign<&ast::NumberLit::text>(NumberText)> {};
  struct StringLit : rule<assign<&ast::StringLit::value>(StringLiteral)> {};
  struct ByteStringLit
      : rule<assign<&ast::ByteStringLit::text>(ref<BYTE_STRING>)> {};
  struct TemporalLit : rule<assign<&ast::TemporalLit::kind>(
                                ikw<"DATETIME"> | ikw<"TIMESTAMP"> |
                                ikw<"DATE"> | ikw<"TIME">) +
                            assign<&ast::TemporalLit::value>(StringLiteral)> {};
  struct DurationLit : rule<ikw<"DURATION"> +
                            assign<&ast::DurationLit::value>(StringLiteral)> {};

  // ---- value types --------------------------------------------------------

  struct Expression;
  struct TypeRef;

  static constexpr auto NotNull =
      flag<&ast::TypeRef::notNull>(ikw<"NOT"> + ikw<"NULL">);
  static constexpr auto Typed = ikw<"TYPED"> | lit<"::">;

  struct PropertyTypeField
      : rule<assign<&ast::PropertyTypeField::name>(Name) + option(Typed) +
             assign<&ast::PropertyTypeField::type>(ref<TypeRef>)> {};
  static constexpr auto PropertyTypeFields =
      lit<"{"> +
      option(some(append<&ast::TypeRef::fields>(ref<PropertyTypeField>),
                  lit<",">)) +
      lit<"}">;

  struct ElementTypeSpec;
  static constexpr auto NestedGraphTypeSpec =
      lit<"{"> +
      some(append<&ast::TypeRef::elements>(ref<ElementTypeSpec>), lit<",">) +
      lit<"}">;

  // Every predefined, constructed, reference and dynamic-union value type,
  // in one rule: `name` carries the leading keyword or type name, `args` its
  // length/precision/scale, `params` a list-element or union member type.
  struct TypeRef
      : rule<
            // list / array, prefix and postfix forms
            (assign<&ast::TypeRef::name>(ikw<"LIST"> | ikw<"ARRAY">) +
             lit<"<"> + append<&ast::TypeRef::params>(ref<TypeRef>) + lit<">"> +
             option(lit<"["> + append<&ast::TypeRef::args>(ref<NumberLit>) +
                    lit<"]">) +
             option(NotNull)) |
            (append<&ast::TypeRef::params>(ref<TypeRef>) +
             flag<&ast::TypeRef::postfixList>(ikw<"LIST"> | ikw<"ARRAY">) +
             option(lit<"["> + append<&ast::TypeRef::args>(ref<NumberLit>) +
                    lit<"]">) +
             option(NotNull)) |
            // record: a bare keyword (the generic record type), or an
            // optionally-kept keyword before the field list
            (option(ikw<"ANY">) + assign<&ast::TypeRef::name>(ikw<"RECORD">) +
             option(NotNull)) |
            (option(ikw<"RECORD">) + PropertyTypeFields + option(NotNull)) |
            // dynamic union types (open, property, closed)
            (assign<&ast::TypeRef::name>(ikw<"ANY">) + option(ikw<"VALUE">) +
             lit<"<"> + append<&ast::TypeRef::params>(ref<TypeRef>) +
             some(lit<"|"> + append<&ast::TypeRef::params>(ref<TypeRef>)) +
             lit<">">) |
            (assign<&ast::TypeRef::name>(ikw<"ANY">) + option(ikw<"VALUE">) +
             option(NotNull)) |
            (option(ikw<"ANY">) + ikw<"PROPERTY"> +
             assign<&ast::TypeRef::name>(ikw<"VALUE">) + option(NotNull)) |
            // graph / binding table / node / edge reference types
            (option(ikw<"PROPERTY">) +
             assign<&ast::TypeRef::name>(ikw<"GRAPH">) +
             option(NestedGraphTypeSpec) + option(NotNull)) |
            (assign<&ast::TypeRef::name>(ikw<"ANY">) + option(ikw<"PROPERTY">) +
             ikw<"GRAPH"> + option(NotNull)) |
            (option(ikw<"BINDING">) +
             assign<&ast::TypeRef::name>(ikw<"TABLE">) + PropertyTypeFields +
             option(NotNull)) |
            (option(ikw<"ANY">) +
             assign<&ast::TypeRef::name>(ikw<"NODE"> | ikw<"VERTEX">) +
             option(NotNull)) |
            (option(ikw<"ANY">) +
             assign<&ast::TypeRef::name>(ikw<"EDGE"> | ikw<"RELATIONSHIP">) +
             option(NotNull)) |
            (assign<&ast::TypeRef::name>(ikw<"PATH">) + option(NotNull)) |
            // immaterial
            (assign<&ast::TypeRef::name>(ikw<"NULL">) + option(NotNull)) |
            (assign<&ast::TypeRef::name>(ikw<"NOTHING">)) |
            // boolean
            (assign<&ast::TypeRef::name>(ikw<"BOOL"> | ikw<"BOOLEAN">) +
             option(NotNull)) |
            // character/byte strings with length
            (assign<&ast::TypeRef::name>(ikw<"STRING">) +
             option(lit<"("> +
                    option(append<&ast::TypeRef::args>(ref<NumberLit>) +
                           lit<",">) +
                    append<&ast::TypeRef::args>(ref<NumberLit>) + lit<")">) +
             option(NotNull)) |
            (assign<&ast::TypeRef::name>(ikw<"CHAR">) +
             option(lit<"("> + append<&ast::TypeRef::args>(ref<NumberLit>) +
                    lit<")">) +
             option(NotNull)) |
            (assign<&ast::TypeRef::name>(ikw<"VARCHAR">) +
             option(lit<"("> + append<&ast::TypeRef::args>(ref<NumberLit>) +
                    lit<")">) +
             option(NotNull)) |
            (assign<&ast::TypeRef::name>(ikw<"BYTES">) +
             option(lit<"("> +
                    option(append<&ast::TypeRef::args>(ref<NumberLit>) +
                           lit<",">) +
                    append<&ast::TypeRef::args>(ref<NumberLit>) + lit<")">) +
             option(NotNull)) |
            (assign<&ast::TypeRef::name>(ikw<"BINARY">) +
             option(lit<"("> + append<&ast::TypeRef::args>(ref<NumberLit>) +
                    lit<")">) +
             option(NotNull)) |
            (assign<&ast::TypeRef::name>(ikw<"VARBINARY">) +
             option(lit<"("> + append<&ast::TypeRef::args>(ref<NumberLit>) +
                    lit<")">) +
             option(NotNull)) |
            // exact numeric
            (assign<&ast::TypeRef::name>(
                 ikw<"INT8"> | ikw<"INT16"> | ikw<"INT32"> | ikw<"INT64"> |
                 ikw<"INT128"> | ikw<"INT256"> | ikw<"SMALLINT"> |
                 ikw<"BIGINT"> | ikw<"UINT8"> | ikw<"UINT16"> | ikw<"UINT32"> |
                 ikw<"UINT64"> | ikw<"UINT128"> | ikw<"UINT256"> |
                 ikw<"USMALLINT"> | ikw<"UBIGINT"> | ikw<"INTEGER8"> |
                 ikw<"INTEGER16"> | ikw<"INTEGER32"> | ikw<"INTEGER64"> |
                 ikw<"INTEGER128"> | ikw<"INTEGER256">) +
             option(NotNull)) |
            (option(ikw<"SIGNED">) + assign<&ast::TypeRef::name>(ikw<"INT">) +
             option(lit<"("> + append<&ast::TypeRef::args>(ref<NumberLit>) +
                    lit<")">) +
             option(NotNull)) |
            (option(ikw<"UNSIGNED">) +
             assign<&ast::TypeRef::name>(ikw<"UINT">) +
             option(lit<"("> + append<&ast::TypeRef::args>(ref<NumberLit>) +
                    lit<")">) +
             option(NotNull)) |
            (option(ikw<"SIGNED">) +
             assign<&ast::TypeRef::name>(ikw<"INTEGER">) +
             option(lit<"("> + append<&ast::TypeRef::args>(ref<NumberLit>) +
                    lit<")">) +
             option(NotNull)) |
            (assign<&ast::TypeRef::name>(ikw<"SMALL">) + ikw<"INTEGER"> +
             option(NotNull)) |
            (assign<&ast::TypeRef::name>(ikw<"BIG">) + ikw<"INTEGER"> +
             option(NotNull)) |
            (assign<&ast::TypeRef::name>(ikw<"DECIMAL"> | ikw<"DEC">) +
             option(lit<"("> + append<&ast::TypeRef::args>(ref<NumberLit>) +
                    option(lit<","> +
                           append<&ast::TypeRef::args>(ref<NumberLit>)) +
                    lit<")">) +
             option(NotNull)) |
            // approximate numeric
            (assign<&ast::TypeRef::name>(ikw<"FLOAT16"> | ikw<"FLOAT32"> |
                                         ikw<"FLOAT64"> | ikw<"FLOAT128"> |
                                         ikw<"FLOAT256"> | ikw<"REAL">) +
             option(NotNull)) |
            (assign<&ast::TypeRef::name>(ikw<"FLOAT">) +
             option(lit<"("> + append<&ast::TypeRef::args>(ref<NumberLit>) +
                    option(lit<","> +
                           append<&ast::TypeRef::args>(ref<NumberLit>)) +
                    lit<")">) +
             option(NotNull)) |
            (assign<&ast::TypeRef::name>(ikw<"DOUBLE">) +
             option(ikw<"PRECISION">) + option(NotNull)) |
            // temporal
            (assign<&ast::TypeRef::name>(ikw<"ZONED">) + ikw<"DATETIME"> +
             option(NotNull)) |
            (assign<&ast::TypeRef::name>(ikw<"TIMESTAMP">) + ikw<"WITH"> +
             ikw<"TIME"> + ikw<"ZONE"> + option(NotNull)) |
            (assign<&ast::TypeRef::name>(ikw<"LOCAL">) + ikw<"DATETIME"> +
             option(NotNull)) |
            (assign<&ast::TypeRef::name>(ikw<"TIMESTAMP">) +
             option(ikw<"WITHOUT"> + ikw<"TIME"> + ikw<"ZONE">) +
             option(NotNull)) |
            (assign<&ast::TypeRef::name>(ikw<"DATE">) + option(NotNull)) |
            (assign<&ast::TypeRef::name>(ikw<"ZONED">) + ikw<"TIME"> +
             option(NotNull)) |
            (assign<&ast::TypeRef::name>(ikw<"TIME">) + ikw<"WITH"> +
             ikw<"TIME"> + ikw<"ZONE"> + option(NotNull)) |
            (assign<&ast::TypeRef::name>(ikw<"LOCAL">) + ikw<"TIME"> +
             option(NotNull)) |
            (assign<&ast::TypeRef::name>(ikw<"TIME">) +
             option(ikw<"WITHOUT"> + ikw<"TIME"> + ikw<"ZONE">) +
             option(NotNull)) |
            (assign<&ast::TypeRef::name>(ikw<"DURATION">) + lit<"("> +
             assign<&ast::TypeRef::qualifier>(ikw<"YEAR"> | ikw<"DAY">) +
             ikw<"TO"> + (ikw<"MONTH"> | ikw<"SECOND">)+lit<")"> +
             option(NotNull))> {};

  // ---- element type specifications (graph type DDL) -----------------------

  static constexpr auto NodeSynonym = ikw<"NODE"> | ikw<"VERTEX">;
  static constexpr auto EdgeSynonym = ikw<"EDGE"> | ikw<"RELATIONSHIP">;

  static constexpr auto ElementLabelList =
      append<&ast::ElementTypeSpec::labels>(Name) +
      many(lit<"&"> + append<&ast::ElementTypeSpec::labels>(Name));
  static constexpr auto ElementLabelSetPhrase =
      (ikw<"LABEL"> + append<&ast::ElementTypeSpec::labels>(Name)) |
      (ikw<"LABELS"> + ElementLabelList) |
      ((ikw<"IS"> | lit<":">)+ElementLabelList);
  static constexpr auto ElementKeyLabelSet =
      option(ElementLabelSetPhrase) +
      flag<&ast::ElementTypeSpec::keyLabelSet>(IMPLIES_TOK);
  static constexpr auto ElementPropertyFields =
      lit<"{"> +
      option(some(
          append<&ast::ElementTypeSpec::properties>(ref<PropertyTypeField>),
          lit<",">)) +
      lit<"}">;
  static constexpr auto ElementImpliedContent =
      (ElementLabelSetPhrase + option(ElementPropertyFields)) |
      ElementPropertyFields;
  static constexpr auto ElementFillerAlt =
      (ElementKeyLabelSet + option(ElementImpliedContent)) |
      ElementImpliedContent;

  static constexpr auto EndpointAlias = RegularIdentifier;
  static constexpr auto SourceNodeTypeRef =
      lit<"("> +
      (assign<&ast::ElementTypeSpec::sourceAlias>(EndpointAlias) |
       option(ElementFillerAlt)) +
      lit<")">;
  static constexpr auto DestNodeTypeRef =
      lit<"("> +
      (assign<&ast::ElementTypeSpec::destAlias>(EndpointAlias) |
       option(ElementFillerAlt)) +
      flag<&ast::ElementTypeSpec::isEdge>(lit<")">);

  struct ElementTypeSpec
      : rule<
            // edge, pattern form: (edgeKind? edgeSynonym TYPE? name)?
            // (src)-[filler]->(dst) | (src)<-[filler]-(dst) |
            // (src)~[filler]~(dst)
            (option(option(flag<&ast::ElementTypeSpec::directedKind>(
                                ikw<"DIRECTED">) |
                            (ikw<"UNDIRECTED">)) +
                    EdgeSynonym + option(ikw<"TYPE">) +
                    assign<&ast::ElementTypeSpec::typeName>(Name)) +
             ((SourceNodeTypeRef +
               ((assign<&ast::ElementTypeSpec::openMark>(lit<"-[">) +
                 option(ElementFillerAlt) +
                 assign<&ast::ElementTypeSpec::closeMark>(lit<"]->">)) |
                (assign<&ast::ElementTypeSpec::openMark>(lit<"~[">) +
                 option(ElementFillerAlt) +
                 assign<&ast::ElementTypeSpec::closeMark>(lit<"]~">))) +
               DestNodeTypeRef) |
              (DestNodeTypeRef +
               assign<&ast::ElementTypeSpec::openMark>(lit<"<-[">) +
               option(ElementFillerAlt) +
               assign<&ast::ElementTypeSpec::closeMark>(lit<"]-">) +
               SourceNodeTypeRef))) |
            // edge, phrase form: edgeKind edgeSynonym TYPE? filler
            //   CONNECTING (src TO dst | src <- dst | src ~ dst)
            ((flag<&ast::ElementTypeSpec::directedKind>(ikw<"DIRECTED">) |
              ikw<"UNDIRECTED">) +
             flag<&ast::ElementTypeSpec::isEdge>(EdgeSynonym) +
             option(ikw<"TYPE">) +
             ((assign<&ast::ElementTypeSpec::typeName>(Name) +
               option(ElementFillerAlt)) |
              ElementFillerAlt) +
             ikw<"CONNECTING"> + lit<"("> +
             ((assign<&ast::ElementTypeSpec::sourceAlias>(EndpointAlias) +
               assign<&ast::ElementTypeSpec::openMark>(ikw<"TO"> | lit<"->"> |
                                                       lit<"~">) +
               assign<&ast::ElementTypeSpec::destAlias>(EndpointAlias)) |
              (assign<&ast::ElementTypeSpec::destAlias>(EndpointAlias) +
               assign<&ast::ElementTypeSpec::openMark>(lit<"<-">) +
               assign<&ast::ElementTypeSpec::sourceAlias>(EndpointAlias))) +
             lit<")">) |
            // node, pattern form: (nodeSynonym TYPE? name)? (alias? filler?)
            (option(NodeSynonym + option(ikw<"TYPE">) +
                    assign<&ast::ElementTypeSpec::typeName>(Name)) +
             lit<"("> +
             option(assign<&ast::ElementTypeSpec::alias>(EndpointAlias)) +
             option(ElementFillerAlt) + lit<")">) |
            // node, phrase form: nodeSynonym TYPE? (name filler? | filler) (AS
            // alias)?
            (NodeSynonym + option(ikw<"TYPE">) +
             ((assign<&ast::ElementTypeSpec::typeName>(Name) +
               option(ElementFillerAlt)) |
              ElementFillerAlt) +
             option(ikw<"AS"> +
                    assign<&ast::ElementTypeSpec::alias>(EndpointAlias)))> {};

  // ---- expressions: primaries and calls ------------------------------------

  struct Procedure;
  struct GraphExpr;
  struct BindingTableExpr;
  struct LabelExpr;
  struct Primary;
  struct ValuePrimary;
  struct GraphPattern;
  struct SimpleStatement;
  struct SimplifiedPathExpr;
  struct PathPatternExpr;

  struct ListLit
      : rule<option(
                 assign<&ast::ListLit::typeName>(ikw<"LIST"> | ikw<"ARRAY">)) +
             lit<"["> +
             option(some(append<&ast::ListLit::items>(ref<Expression>),
                         lit<",">)) +
             lit<"]">> {};
  struct RecordField : rule<assign<&ast::RecordField::name>(Name) + lit<":"> +
                            assign<&ast::RecordField::value>(ref<Expression>)> {
  };
  struct RecordLit
      : rule<option(ikw<"RECORD">) + lit<"{"> +
             option(some(append<&ast::RecordLit::fields>(ref<RecordField>),
                         lit<",">)) +
             lit<"}">> {};
  struct PathStep
      : rule<lit<","> + assign<&ast::PathStep::edge>(ref<Primary>) + lit<","> +
             assign<&ast::PathStep::node>(ref<Primary>)> {};
  struct PathLit
      : rule<ikw<"PATH"> + lit<"["> +
             assign<&ast::PathLit::start>(ref<Primary>) +
             many(append<&ast::PathLit::steps>(ref<PathStep>)) + lit<"]">> {};

  static constexpr auto GeneralLiteral =
      ref<BoolLit> | ref<StringLit> | ref<ByteStringLit> | ref<TemporalLit> |
      ref<DurationLit> | ref<NullLit> | ref<ListLit> | ref<RecordLit>;
  static constexpr auto UnsignedLiteral = ref<NumberLit> | GeneralLiteral;

  struct ParamRef : rule<assign<&ast::ParamRef::name>(ref<GENERAL_PARAM>)> {};
  struct SessionUserRef
      : rule<create<ast::NameRef> +
             assign<&ast::NameRef::name>(ikw<"SESSION_USER">)> {};
  struct NameRef : rule<assign<&ast::NameRef::name>(RegularIdentifier)> {};

  static constexpr auto Args0 = lit<"("> + lit<")">;
  static constexpr auto Args1 =
      lit<"("> + append<&ast::CallExpr::args>(ref<Expression>) + lit<")">;
  static constexpr auto Args2 =
      lit<"("> + append<&ast::CallExpr::args>(ref<Expression>) + lit<","> +
      append<&ast::CallExpr::args>(ref<Expression>) + lit<")">;
  static constexpr auto ArgsAtLeast1 =
      lit<"("> + some(append<&ast::CallExpr::args>(ref<Expression>), lit<",">) +
      lit<")">;
  static constexpr auto ArgsOpt =
      lit<"("> +
      option(some(append<&ast::CallExpr::args>(ref<Expression>), lit<",">)) +
      lit<")">;

  static constexpr auto SetQuantifier =
      assign<&ast::CallExpr::extra>(ikw<"DISTINCT"> | ikw<"ALL">);

  // Every keyword-headed `NAME(args)` shape -- aggregate, numeric, datetime,
  // duration, character/byte-string and list functions, plus ELEMENT_ID,
  // COALESCE and NULLIF -- collapses to one generic call.
  struct CallExpr
      : rule<(assign<&ast::CallExpr::name>(ikw<"COUNT">) + lit<"("> +
              flag<&ast::CallExpr::star>(lit<"*">) + lit<")">) |
             (assign<&ast::CallExpr::name>(
                  ikw<"AVG"> | ikw<"COUNT"> | ikw<"MAX"> | ikw<"MIN"> |
                  ikw<"SUM"> | ikw<"COLLECT_LIST"> | ikw<"STDDEV_SAMP"> |
                  ikw<"STDDEV_POP">) +
              lit<"("> + option(SetQuantifier) +
              append<&ast::CallExpr::args>(ref<Expression>) + lit<")">) |
             (assign<&ast::CallExpr::name>(ikw<"PERCENTILE_CONT"> |
                                           ikw<"PERCENTILE_DISC">) +
              lit<"("> + option(SetQuantifier) +
              append<&ast::CallExpr::args>(ref<Expression>) + lit<","> +
              append<&ast::CallExpr::args>(ref<Expression>) + lit<")">) |
             (assign<&ast::CallExpr::name>(
                  ikw<"CHAR_LENGTH"> | ikw<"CHARACTER_LENGTH"> |
                  ikw<"BYTE_LENGTH"> | ikw<"OCTET_LENGTH"> |
                  ikw<"PATH_LENGTH"> | ikw<"CARDINALITY"> | ikw<"ABS"> |
                  ikw<"SIN"> | ikw<"COS"> | ikw<"TAN"> | ikw<"COT"> |
                  ikw<"SINH"> | ikw<"COSH"> | ikw<"TANH"> | ikw<"ASIN"> |
                  ikw<"ACOS"> | ikw<"ATAN"> | ikw<"DEGREES"> | ikw<"RADIANS"> |
                  ikw<"LOG10"> | ikw<"LN"> | ikw<"EXP"> | ikw<"SQRT"> |
                  ikw<"FLOOR"> | ikw<"CEIL"> | ikw<"CEILING"> | ikw<"UPPER"> |
                  ikw<"LOWER"> | ikw<"ELEMENT_ID"> | ikw<"ELEMENTS">) +
              Args1) |
             (assign<&ast::CallExpr::name>(ikw<"SIZE">) + Args1) |
             (assign<&ast::CallExpr::name>(ikw<"MOD"> | ikw<"POWER">) + Args2) |
             (assign<&ast::CallExpr::name>(ikw<"LOG">) + Args2) |
             (assign<&ast::CallExpr::name>(ikw<"DURATION_BETWEEN">) + Args2 +
              option(assign<&ast::CallExpr::extra>(ikw<"YEAR"> | ikw<"DAY">) +
                     ikw<"TO"> + (ikw<"MONTH"> | ikw<"SECOND">))) |
             (assign<&ast::CallExpr::name>((ikw<"LEFT"> | ikw<"RIGHT">)) +
              Args2) |
             (assign<&ast::CallExpr::name>(ikw<"BTRIM"> | ikw<"LTRIM"> |
                                           ikw<"RTRIM">) +
              lit<"("> + append<&ast::CallExpr::args>(ref<Expression>) +
              option(lit<","> + append<&ast::CallExpr::args>(ref<Expression>)) +
              lit<")">) |
             (assign<&ast::CallExpr::name>(ikw<"TRIM">) + lit<"("> +
              option(assign<&ast::CallExpr::extra>(
                  ikw<"LEADING"> | ikw<"TRAILING"> | ikw<"BOTH">)) +
              option(append<&ast::CallExpr::args>(ref<Expression>)) +
              option(ikw<"FROM">) +
              append<&ast::CallExpr::args>(ref<Expression>) + lit<")">) |
             (assign<&ast::CallExpr::name>(ikw<"NORMALIZE">) + lit<"("> +
              append<&ast::CallExpr::args>(ref<Expression>) +
              option(lit<","> +
                     assign<&ast::CallExpr::extra>(ikw<"NFC"> | ikw<"NFD"> |
                                                   ikw<"NFKC"> | ikw<"NFKD">)) +
              lit<")">) |
             (assign<&ast::CallExpr::name>(ikw<"COALESCE">) + ArgsAtLeast1) |
             (assign<&ast::CallExpr::name>(ikw<"NULLIF">) + Args2) |
             (assign<&ast::CallExpr::name>(
                 ikw<"CURRENT_DATE"> | ikw<"CURRENT_TIME"> |
                 ikw<"CURRENT_TIMESTAMP"> | ikw<"LOCAL_TIMESTAMP">)) |
             (assign<&ast::CallExpr::name>(
                  ikw<"DATE"> | ikw<"ZONED_TIME"> | ikw<"ZONED_DATETIME"> |
                  ikw<"LOCAL_DATETIME"> | ikw<"DURATION">) +
              ArgsOpt) |
             (assign<&ast::CallExpr::name>(ikw<"LOCAL_TIME">) +
              option(ArgsOpt))> {};

  // ---- expressions: case / cast / let / value-query ------------------------

  static constexpr auto NormalForm =
      ikw<"NFC"> | ikw<"NFD"> | ikw<"NFKC"> | ikw<"NFKD">;

  static constexpr auto IsWhen =
      assign<&ast::WhenOperand::partialOp>(ikw<"IS">) +
      option(flag<&ast::WhenOperand::negated>(ikw<"NOT">));
  static constexpr auto PartialWhenOperand =
      (assign<&ast::WhenOperand::partialOp>(lit<"="> | lit<"<>"> | lit<"<="> |
                                            lit<">="> | lit<"<"> | lit<">">) +
       assign<&ast::WhenOperand::value>(ref<Expression>)) |
      (IsWhen +
       assign<&ast::WhenOperand::predicate>(ikw<"NULL"> | ikw<"DIRECTED">)) |
      (IsWhen + assign<&ast::WhenOperand::predicate>(Typed) +
       assign<&ast::WhenOperand::typeTarget>(ref<TypeRef>)) |
      (IsWhen + option(assign<&ast::WhenOperand::normalForm>(NormalForm)) +
       assign<&ast::WhenOperand::predicate>(ikw<"NORMALIZED">)) |
      (((IsWhen + assign<&ast::WhenOperand::predicate>(ikw<"LABELED">)) |
        assign<&ast::WhenOperand::partialOp>(lit<":">)) +
       assign<&ast::WhenOperand::labelTarget>(ref<LabelExpr>)) |
      (IsWhen +
       assign<&ast::WhenOperand::predicate>(ikw<"SOURCE"> |
                                            ikw<"DESTINATION">) +
       ikw<"OF"> + assign<&ast::WhenOperand::value>(ref<Expression>)) |
      assign<&ast::WhenOperand::value>(ref<Expression>);
  struct WhenOperand : rule<PartialWhenOperand> {};

  struct WhenClauseSimple
      : rule<ikw<"WHEN"> +
             some(append<&ast::WhenClause::operands>(ref<WhenOperand>),
                  lit<",">) +
             ikw<"THEN"> +
             assign<&ast::WhenClause::result>(ref<Expression> | ref<NullLit>)> {
  };
  struct WhenClauseSearched
      : rule<ikw<"WHEN"> +
             assign<&ast::WhenClause::condition>(ref<Expression>) +
             ikw<"THEN"> +
             assign<&ast::WhenClause::result>(ref<Expression> | ref<NullLit>)> {
  };

  struct CaseExpr
      : rule<(flag<&ast::CaseExpr::searched>(ikw<"CASE">) +
              some(append<&ast::CaseExpr::whens>(ref<WhenClauseSearched>)) +
              option(ikw<"ELSE"> + assign<&ast::CaseExpr::elseResult>(
                                       ref<Expression> | ref<NullLit>)) +
              ikw<"END">) |
             (ikw<"CASE"> + assign<&ast::CaseExpr::operand>(ref<Expression>) +
              some(append<&ast::CaseExpr::whens>(ref<WhenClauseSimple>)) +
              option(ikw<"ELSE"> + assign<&ast::CaseExpr::elseResult>(
                                       ref<Expression> | ref<NullLit>)) +
              ikw<"END">)> {};

  struct CastExpr
      : rule<ikw<"CAST"> + lit<"("> +
             assign<&ast::CastExpr::operand>(ref<Expression> | ref<NullLit>) +
             ikw<"AS"> + assign<&ast::CastExpr::target>(ref<TypeRef>) +
             lit<")">> {};

  struct LetVarDef
      : rule<(assign<&ast::LetVarDef::name>(RegularIdentifier) +
              option(Typed + assign<&ast::LetVarDef::type>(ref<TypeRef>)) +
              lit<"="> + assign<&ast::LetVarDef::value>(ref<Expression>)) |
             (assign<&ast::LetVarDef::name>(RegularIdentifier) + lit<"="> +
              assign<&ast::LetVarDef::value>(ref<Expression>))> {};
  static constexpr auto LetVarDefList =
      some(append<&ast::LetExpr::defs>(ref<LetVarDef>), lit<",">);
  struct LetExpr
      : rule<ikw<"LET"> + LetVarDefList + ikw<"IN"> +
             assign<&ast::LetExpr::body>(ref<Expression>) + ikw<"END">> {};

  struct ValueQueryExpr
      : rule<ikw<"VALUE"> + lit<"{"> +
             assign<&ast::ValueQueryExpr::query>(ref<Procedure>) + lit<"}">> {};

  // ---- label expressions ---------------------------------------------------

  struct LabelExpr;

  // A parenthesized sub-expression is transparent: the parens are consumed,
  // and the inner node becomes the atom's own value, unwrapped.
  struct LabelAtom : rule<(assign<&ast::LabelName::name>(Name)) |
                          (create<ast::LabelWildcard> + lit<"%">) |
                          (lit<"("> + ref<LabelExpr> + lit<")">)> {};

  struct LabelNotExpr
      : rule<(create<ast::LabelNot>(lit<"!"> + assign<&ast::LabelNot::operand>(
                                                   ref<LabelNotExpr>))) |
             ref<LabelAtom>> {};

  struct LabelAndOp : terminal<lit<"&">, value<ast::LabelOp::and_>> {};
  struct LabelOrOp : terminal<lit<"|">, value<ast::LabelOp::or_>> {};

  struct LabelExpr : infix<LabelNotExpr, &ast::LabelBin::left,
                           &ast::LabelBin::op, &ast::LabelBin::right,
                           (left(ref<LabelAndOp>) >> left(ref<LabelOrOp>))> {};

  // ---- expressions: predicates ---------------------------------------------

  static constexpr auto ElementVarRef = ref<NameRef>;

  struct ExistsTest
      : rule<ikw<"EXISTS"> +
             ((lit<"{"> + assign<&ast::ExistsTest::pattern>(ref<GraphPattern>) +
               lit<"}">) |
              (lit<"("> + assign<&ast::ExistsTest::pattern>(ref<GraphPattern>) +
               lit<")">) |
              (lit<"{"> +
               some(append<&ast::ExistsTest::block>(ref<SimpleStatement>)) +
               lit<"}">) |
              (lit<"("> +
               some(append<&ast::ExistsTest::block>(ref<SimpleStatement>)) +
               lit<")">) |
              (lit<"{"> + assign<&ast::ExistsTest::nested>(ref<Procedure>) +
               lit<"}">))> {};

  struct AllDifferentTest
      : rule<ikw<"ALL_DIFFERENT"> + lit<"("> +
             some(append<&ast::AllDifferentTest::operands>(ElementVarRef),
                  lit<",">) +
             lit<")">> {};
  struct SameTest
      : rule<ikw<"SAME"> + lit<"("> +
             some(append<&ast::SameTest::operands>(ElementVarRef), lit<",">) +
             lit<")">> {};
  struct PropertyExistsTest
      : rule<ikw<"PROPERTY_EXISTS"> + lit<"("> +
             assign<&ast::PropertyExistsTest::operand>(ElementVarRef) +
             lit<","> + assign<&ast::PropertyExistsTest::property>(Name) +
             lit<")">> {};

  struct NullTest
      : rule<assign<&ast::NullTest::operand>(ref<ValuePrimary>) + ikw<"IS"> +
             option(flag<&ast::NullTest::negated>(ikw<"NOT">)) +
             ikw<"NULL">> {};
  struct TypedTest
      : rule<assign<&ast::TypedTest::operand>(ref<ValuePrimary>) + ikw<"IS"> +
             option(flag<&ast::TypedTest::negated>(ikw<"NOT">)) + Typed +
             assign<&ast::TypedTest::target>(ref<TypeRef>)> {};
  struct DirectedTest
      : rule<assign<&ast::DirectedTest::operand>(ElementVarRef) + ikw<"IS"> +
             option(flag<&ast::DirectedTest::negated>(ikw<"NOT">)) +
             ikw<"DIRECTED">> {};
  struct LabeledTest
      : rule<assign<&ast::LabeledTest::operand>(ElementVarRef) +
             ((ikw<"IS"> +
               option(flag<&ast::LabeledTest::negated>(ikw<"NOT">)) +
               ikw<"LABELED">) |
              lit<":">)+assign<&ast::LabeledTest::label>(ref<LabelExpr>)> {};
  struct SourceDestTest
      : rule<assign<&ast::SourceDestTest::node>(ElementVarRef) + ikw<"IS"> +
             option(flag<&ast::SourceDestTest::negated>(ikw<"NOT">)) +
             ((flag<&ast::SourceDestTest::isSource>(ikw<"SOURCE">)) |
              ikw<"DESTINATION">)+ikw<"OF"> +
             assign<&ast::SourceDestTest::edge>(ElementVarRef)> {};

  static constexpr auto Predicate =
      ref<ExistsTest> | ref<NullTest> | ref<TypedTest> | ref<DirectedTest> |
      ref<LabeledTest> | ref<SourceDestTest> | ref<AllDifferentTest> |
      ref<SameTest> | ref<PropertyExistsTest>;

  // ---- expressions: primary chain ------------------------------------------

  struct Primary
      : rule<(lit<"("> + ref<Expression> + lit<")">) | ref<CallExpr> |
             ref<CaseExpr> | ref<CastExpr> | ref<LetExpr> |
             ref<ValueQueryExpr> | UnsignedLiteral | ref<ParamRef> |
             ref<SessionUserRef> | (ikw<"VARIABLE"> + ref<Primary>) |
             (create<ast::GraphExprValue>(
                 option(ikw<"PROPERTY">) + ikw<"GRAPH"> +
                 assign<&ast::GraphExprValue::graph>(ref<GraphExpr>))) |
             (create<ast::BindingTableExprValue>(
                 option(ikw<"BINDING">) + ikw<"TABLE"> +
                 assign<&ast::BindingTableExprValue::table>(
                     ref<BindingTableExpr>))) |
             ref<PathLit> | ref<NameRef>> {};

  // `n.a.b` -- left recursion over the property-reference production.
  struct ValuePrimary
      : rule<(create<ast::PropertyRef>(
                 assign<&ast::PropertyRef::base>(ref<ValuePrimary>) + lit<"."> +
                 assign<&ast::PropertyRef::property>(Name))) |
             ref<Primary>> {};

  // ---- expressions: operator ladders ---------------------------------------

  struct SignExpr
      : rule<(create<ast::SignExpr>(
                  flag<&ast::SignExpr::negative>(lit<"-">) | lit<"+">) +
              assign<&ast::SignExpr::operand>(ref<SignExpr>)) |
             ref<ValuePrimary>> {};

  struct OpMul : terminal<lit<"*">, value<ast::BinOp::multiply>> {};
  struct OpDiv : terminal<lit<"/">, value<ast::BinOp::divide>> {};
  struct OpAdd : terminal<lit<"+">, value<ast::BinOp::add>> {};
  struct OpSub : terminal<lit<"-">, value<ast::BinOp::subtract>> {};
  struct OpConcat : terminal<lit<"||">, value<ast::BinOp::concat>> {};
  struct OpNe : terminal<lit<"<>">, value<ast::BinOp::ne>> {};
  struct OpLe : terminal<lit<"<=">, value<ast::BinOp::le>> {};
  struct OpGe : terminal<lit<">=">, value<ast::BinOp::ge>> {};
  struct OpEq : terminal<lit<"=">, value<ast::BinOp::eq>> {};
  struct OpLt : terminal<lit<"<">, value<ast::BinOp::lt>> {};
  struct OpGt : terminal<lit<">">, value<ast::BinOp::gt>> {};

  static constexpr auto MulOp = ref<OpMul> | ref<OpDiv>;
  static constexpr auto AddOp = ref<OpAdd> | ref<OpSub>;
  static constexpr auto ConcatOp = ref<OpConcat>;
  static constexpr auto CmpOp =
      ref<OpNe> | ref<OpLe> | ref<OpGe> | ref<OpEq> | ref<OpLt> | ref<OpGt>;

  // `<character string concatenation> ::= <character string value expression>
  // '||' <character string primary>` is left-recursive on its own left
  // operand, the same shape as `<term>`/`<numeric value expression>` above
  // it: `a || b || c` is `(a || b) || c`.
  struct ArithCompare
      : infix<SignExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(MulOp) >> left(AddOp) >> left(ConcatOp) >> left(CmpOp))> {};

  static constexpr auto ArithOrPredicate = ref<ArithCompare> | Predicate;

  struct NormalizedWrap
      : rule<(create<ast::NormalizedTest>(
                 assign<&ast::NormalizedTest::operand>(ArithOrPredicate) +
                 ikw<"IS"> +
                 option(flag<&ast::NormalizedTest::negated>(ikw<"NOT">)) +
                 option(assign<&ast::NormalizedTest::form>(NormalForm)) +
                 ikw<"NORMALIZED">)) |
             ArithOrPredicate> {};

  struct NotWrap
      : rule<(create<ast::NotExpr>(
                 ikw<"NOT"> + assign<&ast::NotExpr::operand>(ref<NotWrap>))) |
             ref<NormalizedWrap>> {};

  struct TruthWrap
      : rule<(create<ast::TruthTest>(
                 assign<&ast::TruthTest::operand>(ref<NotWrap>) + ikw<"IS"> +
                 option(flag<&ast::TruthTest::negated>(ikw<"NOT">)) +
                 assign<&ast::TruthTest::value>(ref<BOOL_LIT>))) |
             ref<NotWrap>> {};

  struct OpAnd : terminal<ikw<"AND">, value<ast::BinOp::and_>> {};
  struct OpOr : terminal<ikw<"OR">, value<ast::BinOp::or_>> {};
  struct OpXor : terminal<ikw<"XOR">, value<ast::BinOp::xor_>> {};

  struct Expression
      : infix<TruthWrap, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(ref<OpAnd>) >> left(ref<OpOr> | ref<OpXor>))> {};

  // ---- graph/binding-table expressions -------------------------------------

  static constexpr auto CatalogSegments =
      append<&ast::CatalogRef::segments>(Name) + lit<"."> +
      many(append<&ast::CatalogRef::segments>(Name) + lit<".">);
  // A schema path's directories are `/`-separated, the trailing schema name
  // bare -- unlike an object path's `.`-separated, all-trailing segments.
  static constexpr auto DirSegments =
      many(append<&ast::CatalogRef::segments>(Name) + lit<"/">);
  struct SchemaRef
      : rule<(create<ast::CatalogRef> + lit<"/">) |
             (flag<&ast::CatalogRef::absolute>(lit<"/">) + DirSegments +
              append<&ast::CatalogRef::segments>(Name)) |
             (flag<&ast::CatalogRef::home>(
                 ikw<"HOME_SCHEMA"> | ikw<"CURRENT_SCHEMA"> | lit<".">)) |
             (flag<&ast::CatalogRef::relative>(
                  lit<".."> + many(lit<"/"> + lit<"..">) + lit<"/">) +
              DirSegments + append<&ast::CatalogRef::segments>(Name)) |
             (assign<&ast::CatalogRef::paramName>(ref<SUBSTITUTED_PARAM>))> {};
  struct CatalogObjRef
      : rule<(assign<&ast::CatalogRef::paramName>(ref<SUBSTITUTED_PARAM>)) |
             (flag<&ast::CatalogRef::home>(ikw<"HOME_GRAPH"> |
                                                ikw<"HOME_PROPERTY_GRAPH">)) |
             (assign<&ast::CatalogRef::delimitedName>(ref<DOUBLE_QUOTED> |
                                                      ref<ACCENT_QUOTED>)) |
             (option(CatalogSegments) +
              append<&ast::CatalogRef::segments>(Name))> {};

  struct GraphExpr
      : rule<(flag<&ast::GraphExpr::current>(
                 ikw<"CURRENT_PROPERTY_GRAPH"> | ikw<"CURRENT_GRAPH">)) |
             (assign<&ast::GraphExpr::ref>(ref<CatalogObjRef>)) |
             (ikw<"VARIABLE"> + assign<&ast::GraphExpr::value>(ref<Primary>)) |
             (assign<&ast::GraphExpr::value>(ref<Primary>))> {};

  struct BindingTableExpr
      : rule<(lit<"{"> +
              assign<&ast::BindingTableExpr::nested>(ref<Procedure>) +
              lit<"}">) |
             (assign<&ast::BindingTableExpr::ref>(ref<CatalogObjRef>)) |
             (assign<&ast::BindingTableExpr::value>(ref<Primary>))> {};

  static constexpr auto UseGraphClauseExpr =
      ikw<"USE"> + assign<&ast::LinearStatement::useGraph>(ref<GraphExpr>);

  // ---- graph patterns -------------------------------------------------------

  static constexpr auto IsOrColon = ikw<"IS"> | lit<":">;
  struct PropertyKV : rule<assign<&ast::PropertyKV::name>(Name) + lit<":"> +
                           assign<&ast::PropertyKV::value>(ref<Expression>)> {};
  static constexpr auto ElementProperties =
      lit<"{"> +
      option(some(append<&ast::ElementFiller::properties>(ref<PropertyKV>),
                  lit<",">)) +
      lit<"}">;
  struct ElementFiller
      : rule<create<ast::ElementFiller> +
             option(assign<&ast::ElementFiller::variable>(RegularIdentifier)) +
             option(IsOrColon +
                    assign<&ast::ElementFiller::labels>(ref<LabelExpr>)) +
             option((ikw<"WHERE"> +
                     assign<&ast::ElementFiller::where>(ref<Expression>)) |
                    ElementProperties)> {};

  struct NodePattern
      : rule<lit<"("> + assign<&ast::NodePattern::filler>(ref<ElementFiller>) +
             lit<")">> {};

  static constexpr auto FullFiller = ref<ElementFiller>;
  struct EdgePattern
      : rule<(assign<&ast::EdgePattern::openMark>(lit<"<-[">) +
              assign<&ast::EdgePattern::filler>(FullFiller) +
              assign<&ast::EdgePattern::closeMark>(lit<"]-">)) |
             (assign<&ast::EdgePattern::openMark>(lit<"~[">) +
              assign<&ast::EdgePattern::filler>(FullFiller) +
              assign<&ast::EdgePattern::closeMark>(lit<"]~">)) |
             (assign<&ast::EdgePattern::openMark>(lit<"-[">) +
              assign<&ast::EdgePattern::filler>(FullFiller) +
              assign<&ast::EdgePattern::closeMark>(lit<"]->">)) |
             (assign<&ast::EdgePattern::openMark>(lit<"<~[">) +
              assign<&ast::EdgePattern::filler>(FullFiller) +
              assign<&ast::EdgePattern::closeMark>(lit<"]~">)) |
             (assign<&ast::EdgePattern::openMark>(lit<"~[">) +
              assign<&ast::EdgePattern::filler>(FullFiller) +
              assign<&ast::EdgePattern::closeMark>(lit<"]~>">)) |
             (assign<&ast::EdgePattern::openMark>(lit<"<-[">) +
              assign<&ast::EdgePattern::filler>(FullFiller) +
              assign<&ast::EdgePattern::closeMark>(lit<"]->">)) |
             (assign<&ast::EdgePattern::openMark>(lit<"-[">) +
              assign<&ast::EdgePattern::filler>(FullFiller) +
              assign<&ast::EdgePattern::closeMark>(lit<"]-">)) |
             flag<&ast::EdgePattern::abbreviated>(
                 assign<&ast::EdgePattern::openMark>(
                     lit<"<->"> | lit<"<-"> | lit<"->"> | lit<"<~"> |
                     lit<"~>"> | lit<"~"> | lit<"-">))> {};

  struct ElementPattern
      : rule<(create<ast::NodePattern>(
                 lit<"("> +
                 assign<&ast::NodePattern::filler>(ref<ElementFiller>) +
                 lit<")">)) |
             ref<EdgePattern>> {};

  struct Quantifier
      : rule<(create<ast::Quantifier> + assign<&ast::Quantifier::symbol>(
                                            lit<"*"> | lit<"+"> | lit<"?">)) |
             (create<ast::Quantifier> + lit<"{"> +
              assign<&ast::Quantifier::min>(ref<NumberLit>) + lit<"}">) |
             (create<ast::Quantifier> + lit<"{"> +
              option(assign<&ast::Quantifier::min>(ref<NumberLit>)) + lit<","> +
              option(assign<&ast::Quantifier::max>(ref<NumberLit>)) +
              lit<"}">)> {};

  static constexpr auto PathMode =
      ikw<"WALK"> | ikw<"TRAIL"> | ikw<"SIMPLE"> | ikw<"ACYCLIC">;
  static constexpr auto PathOrPaths = ikw<"PATH"> | ikw<"PATHS">;
  struct PathPrefix
      : rule<(assign<&ast::PathPrefix::mode>(PathMode) +
              option(assign<&ast::PathPrefix::pathOrPaths>(PathOrPaths))) |
             (assign<&ast::PathPrefix::search>(ikw<"ALL">) + ikw<"SHORTEST"> +
              option(assign<&ast::PathPrefix::mode>(PathMode)) +
              option(assign<&ast::PathPrefix::pathOrPaths>(PathOrPaths))) |
             (assign<&ast::PathPrefix::search>(ikw<"ANY">) + ikw<"SHORTEST"> +
              option(assign<&ast::PathPrefix::mode>(PathMode)) +
              option(assign<&ast::PathPrefix::pathOrPaths>(PathOrPaths))) |
             (assign<&ast::PathPrefix::search>(ikw<"SHORTEST">) +
              assign<&ast::PathPrefix::count>(ref<NumberLit>) +
              option(assign<&ast::PathPrefix::mode>(PathMode)) +
              option(assign<&ast::PathPrefix::pathOrPaths>(PathOrPaths))) |
             (assign<&ast::PathPrefix::search>(ikw<"SHORTEST">) +
              option(assign<&ast::PathPrefix::count>(ref<NumberLit>)) +
              option(assign<&ast::PathPrefix::mode>(PathMode)) +
              option(assign<&ast::PathPrefix::pathOrPaths>(PathOrPaths)) +
              flag<&ast::PathPrefix::group>(ikw<"GROUP"> |
                                                 ikw<"GROUPS">)) |
             (assign<&ast::PathPrefix::search>(ikw<"ANY">) +
              option(assign<&ast::PathPrefix::count>(ref<NumberLit>)) +
              option(assign<&ast::PathPrefix::mode>(PathMode)) +
              option(assign<&ast::PathPrefix::pathOrPaths>(PathOrPaths))) |
             (assign<&ast::PathPrefix::search>(ikw<"ALL">) +
              option(assign<&ast::PathPrefix::mode>(PathMode)) +
              option(assign<&ast::PathPrefix::pathOrPaths>(PathOrPaths)))> {};

  struct PathPatternExpr;

  struct PathPrimaryRule
      : rule<ref<ElementPattern> |
             (create<ast::ParenPathExpr>(
                 lit<"("> +
                 option(
                     assign<&ast::ParenPathExpr::variable>(RegularIdentifier) +
                     lit<"=">) +
                 option(assign<&ast::ParenPathExpr::prefix>(ref<PathPrefix>)) +
                 assign<&ast::ParenPathExpr::expr>(ref<PathPatternExpr>) +
                 option(ikw<"WHERE"> +
                        assign<&ast::ParenPathExpr::where>(ref<Expression>)) +
                 lit<")">)) |
             ref<SimplifiedPathExpr>> {};

  struct PathFactor
      : rule<create<ast::PathFactor> +
             assign<&ast::PathFactor::primary>(ref<PathPrimaryRule>) +
             option(assign<&ast::PathFactor::quantifier>(ref<Quantifier>))> {};

  struct PathTerm
      : rule<some(append<&ast::PathTerm::factors>(ref<PathFactor>))> {};

  struct PathPatternExpr
      : rule<(append<&ast::PathPatternExpr::terms>(ref<PathTerm>) +
              assign<&ast::PathPatternExpr::op>(lit<"|+|">) +
              append<&ast::PathPatternExpr::terms>(ref<PathTerm>) +
              many(lit<"|+|"> +
                   append<&ast::PathPatternExpr::terms>(ref<PathTerm>))) |
             (append<&ast::PathPatternExpr::terms>(ref<PathTerm>) +
              assign<&ast::PathPatternExpr::op>(lit<"|">) +
              append<&ast::PathPatternExpr::terms>(ref<PathTerm>) +
              many(lit<"|"> +
                   append<&ast::PathPatternExpr::terms>(ref<PathTerm>))) |
             append<&ast::PathPatternExpr::terms>(ref<PathTerm>)> {};

  // The simplified path pattern's five-tier internal precedence (union,
  // multiset alternation, concatenation, conjunction, quantifiers and
  // direction overrides) flattens to one associative level of `&`/`|`
  // between label atoms; a quantified/questioned/negated/overridden atom is
  // read but not distinguished from a plain one.
  static constexpr auto SimplifiedContents =
      append<&ast::SimplifiedPathExpr::terms>(ref<LabelExpr>) +
      option((assign<&ast::SimplifiedPathExpr::op>(lit<"&">) +
              some(append<&ast::SimplifiedPathExpr::terms>(ref<LabelExpr>),
                   lit<"&">)) |
             (assign<&ast::SimplifiedPathExpr::op>(lit<"|">) +
              some(append<&ast::SimplifiedPathExpr::terms>(ref<LabelExpr>),
                   lit<"|">)) |
             (assign<&ast::SimplifiedPathExpr::op>(lit<"|+|">) +
              some(append<&ast::SimplifiedPathExpr::terms>(ref<LabelExpr>),
                   lit<"|+|">)));

  struct SimplifiedPathExpr
      : rule<(assign<&ast::SimplifiedPathExpr::openMark>(lit<"<-/">) +
              SimplifiedContents +
              assign<&ast::SimplifiedPathExpr::closeMark>(lit<"/-">)) |
             (assign<&ast::SimplifiedPathExpr::openMark>(lit<"~/">) +
              SimplifiedContents +
              assign<&ast::SimplifiedPathExpr::closeMark>(lit<"/~">)) |
             (assign<&ast::SimplifiedPathExpr::openMark>(lit<"-/">) +
              SimplifiedContents +
              assign<&ast::SimplifiedPathExpr::closeMark>(lit<"/->">)) |
             (assign<&ast::SimplifiedPathExpr::openMark>(lit<"<~/">) +
              SimplifiedContents +
              assign<&ast::SimplifiedPathExpr::closeMark>(lit<"/~">)) |
             (assign<&ast::SimplifiedPathExpr::openMark>(lit<"~/">) +
              SimplifiedContents +
              assign<&ast::SimplifiedPathExpr::closeMark>(lit<"/~>">)) |
             (assign<&ast::SimplifiedPathExpr::openMark>(lit<"-/">) +
              SimplifiedContents +
              assign<&ast::SimplifiedPathExpr::closeMark>(lit<"/-">))> {};

  struct PathPattern
      : rule<option(assign<&ast::PathPattern::variable>(RegularIdentifier) +
                    lit<"=">) +
             option(assign<&ast::PathPattern::prefix>(ref<PathPrefix>)) +
             assign<&ast::PathPattern::expr>(ref<PathPatternExpr>)> {};

  static constexpr auto ElementBindingsOrElements =
      (ikw<"ELEMENT"> + option(ikw<"BINDINGS">)) | ikw<"ELEMENTS">;
  static constexpr auto EdgeBindingsOrEdges =
      ((ikw<"EDGE"> | ikw<"RELATIONSHIP">)+option(ikw<"BINDINGS">)) |
      ikw<"EDGES"> | ikw<"RELATIONSHIPS">;
  static constexpr auto MatchModeExpr =
      (assign<&ast::GraphPattern::matchMode>(ikw<"REPEATABLE">) +
       ElementBindingsOrElements) |
      (assign<&ast::GraphPattern::matchMode>(ikw<"DIFFERENT">) +
       EdgeBindingsOrEdges);

  struct GraphPattern
      : rule<option(MatchModeExpr) +
             some(append<&ast::GraphPattern::paths>(ref<PathPattern>),
                  lit<",">) +
             option(ikw<"KEEP"> +
                    assign<&ast::GraphPattern::keep>(ref<PathPrefix>)) +
             option(ikw<"WHERE"> +
                    assign<&ast::GraphPattern::where>(ref<Expression>))> {};

  // ---- insert pattern ---------------------------------------------------

  struct InsertNode
      : rule<lit<"("> + assign<&ast::InsertNode::filler>(ref<ElementFiller>) +
             lit<")">> {};
  struct InsertEdge
      : rule<(assign<&ast::InsertEdge::openMark>(lit<"<-[">) +
              assign<&ast::InsertEdge::filler>(ref<ElementFiller>) +
              lit<"]-">) |
             (assign<&ast::InsertEdge::openMark>(lit<"-[">) +
              assign<&ast::InsertEdge::filler>(ref<ElementFiller>) +
              lit<"]->">) |
             (assign<&ast::InsertEdge::openMark>(lit<"~[">) +
              assign<&ast::InsertEdge::filler>(ref<ElementFiller>) +
              lit<"]~">)> {};
  struct InsertStep : rule<assign<&ast::InsertStep::edge>(ref<InsertEdge>) +
                           assign<&ast::InsertStep::node>(ref<InsertNode>)> {};
  struct InsertPath
      : rule<assign<&ast::InsertPath::start>(ref<InsertNode>) +
             many(append<&ast::InsertPath::steps>(ref<InsertStep>))> {};
  struct InsertPattern
      : rule<some(append<&ast::InsertPattern::paths>(ref<InsertPath>),
                  lit<",">)> {};

  // ---- yield / order-by / page ----------------------------------------------

  struct YieldItem : rule<assign<&ast::YieldItem::name>(Name) +
                          option(ikw<"AS"> + assign<&ast::YieldItem::alias>(
                                                 RegularIdentifier))> {};
  struct YieldClause
      : rule<ikw<"YIELD"> +
             some(append<&ast::YieldClause::items>(ref<YieldItem>), lit<",">)> {
  };
  struct GraphPatternYieldClause
      : rule<ikw<"YIELD"> +
             some(append<&ast::YieldClause::items>(ref<YieldItem>), lit<",">)> {
  };

  struct SortSpec
      : rule<assign<&ast::SortSpec::key>(ref<Expression>) +
             option(assign<&ast::SortSpec::direction>(
                 ikw<"ASC"> | ikw<"ASCENDING"> | ikw<"DESC"> |
                 ikw<"DESCENDING">)) +
             option(ikw<"NULLS"> + assign<&ast::SortSpec::nulls>(
                                       ikw<"FIRST"> | ikw<"LAST">))> {};
  static constexpr auto OrderByClauseExpr =
      ikw<"ORDER"> + ikw<"BY"> +
      some(append<&ast::OrderByPage::orderBy>(ref<SortSpec>), lit<",">);
  static constexpr auto OffsetClauseExpr =
      (ikw<"OFFSET"> |
       ikw<"SKIP">)+assign<&ast::OrderByPage::offset>(ref<Expression>);
  static constexpr auto LimitClauseExpr =
      ikw<"LIMIT"> + assign<&ast::OrderByPage::limit>(ref<Expression>);
  struct OrderByPage
      : rule<(OrderByClauseExpr + option(OffsetClauseExpr) +
              option(LimitClauseExpr)) |
             (OffsetClauseExpr + option(LimitClauseExpr)) | LimitClauseExpr> {};

  struct ReturnItem : rule<assign<&ast::ReturnItem::value>(ref<Expression>) +
                           option(ikw<"AS"> + assign<&ast::ReturnItem::alias>(
                                                  RegularIdentifier))> {};
  static constexpr auto GroupByClauseExpr =
      ikw<"GROUP"> + ikw<"BY"> +
      ((some(append<&ast::ResultStmt::groupBy>(ref<Expression>), lit<",">)) |
       (lit<"("> + lit<")">));
  struct ResultStmt
      : rule<(create<ast::ResultStmt>(
                 flag<&ast::ResultStmt::finish>(ikw<"FINISH">))) |
             (ikw<"RETURN"> +
              option(flag<&ast::ResultStmt::distinct>(ikw<"DISTINCT">) |
                     ikw<"ALL">) +
              (flag<&ast::ResultStmt::star>(lit<"*">) |
               some(append<&ast::ResultStmt::items>(ref<ReturnItem>),
                    lit<",">)) +
              option(GroupByClauseExpr) +
              option(assign<&ast::ResultStmt::page>(ref<OrderByPage>)))> {};

  // ---- simple statements ----------------------------------------------------

  struct MatchStmt : rule<ikw<"MATCH"> +
                          assign<&ast::MatchStmt::pattern>(ref<GraphPattern>) +
                          option(assign<&ast::MatchStmt::yield>(
                              ref<GraphPatternYieldClause>))> {};
  struct OptionalMatchStmt
      : rule<ikw<"OPTIONAL"> +
             ((assign<&ast::OptionalMatchStmt::simple>(ref<MatchStmt>)) |
              (lit<"{"> +
               some(append<&ast::OptionalMatchStmt::block>(
                   ref<SimpleStatement>)) +
               lit<"}">) |
              (lit<"("> +
               some(append<&ast::OptionalMatchStmt::block>(
                   ref<SimpleStatement>)) +
               lit<")">))> {};

  struct LetStmt
      : rule<ikw<"LET"> +
             some(append<&ast::LetStmt::defs>(ref<LetVarDef>), lit<",">)> {};

  struct ForStmt
      : rule<ikw<"FOR"> + assign<&ast::ForStmt::variable>(RegularIdentifier) +
             ikw<"IN"> + assign<&ast::ForStmt::source>(ref<Expression>) +
             option(ikw<"WITH"> +
                    assign<&ast::ForStmt::ordinalityOrOffset>(
                        ikw<"ORDINALITY"> | ikw<"OFFSET">) +
                    assign<&ast::ForStmt::ordVariable>(RegularIdentifier))> {};

  struct FilterStmt
      : rule<ikw<"FILTER"> +
             assign<&ast::FilterStmt::condition>(
                 (ikw<"WHERE"> + ref<Expression>) | ref<Expression>)> {};

  static constexpr auto ProcedureArgs =
      lit<"("> +
      option(some(append<&ast::CallProcedureStmt::args>(ref<Expression>),
                  lit<",">)) +
      lit<")">;
  struct CallProcedureStmt
      : rule<option(flag<&ast::CallProcedureStmt::optionalCall>(
                 ikw<"OPTIONAL">)) +
             ikw<"CALL"> +
             ((option(lit<"("> +
                      option(some(append<&ast::CallProcedureStmt::scopeVars>(
                                      RegularIdentifier),
                                  lit<",">)) +
                      lit<")">) +
               flag<&ast::CallProcedureStmt::isInline>(lit<"{">) +
               assign<&ast::CallProcedureStmt::inlineBody>(ref<Procedure>) +
               lit<"}">) |
              (assign<&ast::CallProcedureStmt::procRef>(ref<CatalogObjRef>) +
               ProcedureArgs +
               option(assign<&ast::CallProcedureStmt::yield>(
                   ref<YieldClause>))))> {};

  struct InsertStmt : rule<ikw<"INSERT"> + assign<&ast::InsertStmt::pattern>(
                                               ref<InsertPattern>)> {};

  struct SetPropertyItem
      : rule<assign<&ast::SetPropertyItem::variable>(RegularIdentifier) +
             lit<"."> + assign<&ast::SetPropertyItem::property>(Name) +
             lit<"="> + assign<&ast::SetPropertyItem::value>(ref<Expression>)> {
  };
  struct SetAllPropertiesItem
      : rule<assign<&ast::SetAllPropertiesItem::variable>(RegularIdentifier) +
             lit<"="> + lit<"{"> +
             option(some(append<&ast::SetAllPropertiesItem::properties>(
                             ref<PropertyKV>),
                         lit<",">)) +
             lit<"}">> {};
  struct SetLabelItem
      : rule<assign<&ast::SetLabelItem::variable>(RegularIdentifier) +
             IsOrColon + assign<&ast::SetLabelItem::label>(Name)> {};
  struct SetItem : rule<ref<SetPropertyItem> | ref<SetAllPropertiesItem> |
                        ref<SetLabelItem>> {};
  struct SetStmt
      : rule<ikw<"SET"> +
             some(append<&ast::SetStmt::items>(ref<SetItem>), lit<",">)> {};

  struct RemovePropertyItem
      : rule<assign<&ast::RemovePropertyItem::variable>(RegularIdentifier) +
             lit<"."> + assign<&ast::RemovePropertyItem::property>(Name)> {};
  struct RemoveLabelItem
      : rule<assign<&ast::RemoveLabelItem::variable>(RegularIdentifier) +
             IsOrColon + assign<&ast::RemoveLabelItem::label>(Name)> {};
  struct RemoveItem : rule<ref<RemovePropertyItem> | ref<RemoveLabelItem>> {};
  struct RemoveStmt
      : rule<ikw<"REMOVE"> +
             some(append<&ast::RemoveStmt::items>(ref<RemoveItem>), lit<",">)> {
  };

  struct DeleteStmt
      : rule<option(flag<&ast::DeleteStmt::detach>(ikw<"DETACH">) |
                    flag<&ast::DeleteStmt::noDetach>(ikw<"NODETACH">)) +
             ikw<"DELETE"> +
             some(append<&ast::DeleteStmt::items>(ref<Expression>), lit<",">)> {
  };

  // ---- catalog DDL statements -----------------------------------------------

  struct CreateSchemaStmt
      : rule<ikw<"CREATE"> + ikw<"SCHEMA"> +
             option(ikw<"IF"> + ikw<"NOT"> +
                    flag<&ast::CreateSchemaStmt::ifNotExists>(
                        ikw<"EXISTS">)) +
             assign<&ast::CreateSchemaStmt::name>(ref<SchemaRef>)> {};
  struct DropSchemaStmt
      : rule<ikw<"DROP"> + ikw<"SCHEMA"> +
             option(ikw<"IF"> +
                    flag<&ast::DropSchemaStmt::ifExists>(ikw<"EXISTS">)) +
             assign<&ast::DropSchemaStmt::name>(ref<SchemaRef>)> {};

  static constexpr auto GraphTypeElements =
      lit<"{"> +
      some(append<&ast::CreateGraphStmt::typeElements>(ref<ElementTypeSpec>),
           lit<",">) +
      lit<"}">;
  struct CreateGraphStmt
      : rule<ikw<"CREATE"> +
             ((option(ikw<"PROPERTY">) + ikw<"GRAPH"> +
               option(ikw<"IF"> + ikw<"NOT"> +
                      flag<&ast::CreateGraphStmt::ifNotExists>(
                          ikw<"EXISTS">))) |
              (flag<&ast::CreateGraphStmt::orReplace>(ikw<"OR"> +
                                                           ikw<"REPLACE">) +
               option(ikw<"PROPERTY">) + ikw<"GRAPH">)) +
             assign<&ast::CreateGraphStmt::name>(ref<CatalogObjRef>) +
             ((flag<&ast::CreateGraphStmt::openType>(
                  option(Typed) + ikw<"ANY"> +
                  option(option(ikw<"PROPERTY">) + ikw<"GRAPH">))) |
              (ikw<"LIKE"> +
               assign<&ast::CreateGraphStmt::likeGraph>(ref<GraphExpr>)) |
              (option(Typed) +
               assign<&ast::CreateGraphStmt::typeRef>(ref<CatalogObjRef>)) |
              (option(Typed) + option(option(ikw<"PROPERTY">) + ikw<"GRAPH">) +
               GraphTypeElements)) +
             option(ikw<"AS"> + ikw<"COPY"> + ikw<"OF"> +
                    assign<&ast::CreateGraphStmt::source>(ref<GraphExpr>))> {};
  struct DropGraphStmt
      : rule<ikw<"DROP"> + option(ikw<"PROPERTY">) + ikw<"GRAPH"> +
             option(ikw<"IF"> +
                    flag<&ast::DropGraphStmt::ifExists>(ikw<"EXISTS">)) +
             assign<&ast::DropGraphStmt::name>(ref<CatalogObjRef>)> {};

  static constexpr auto GraphTypeSpecElements =
      lit<"{"> +
      some(append<&ast::CreateGraphTypeStmt::elements>(ref<ElementTypeSpec>),
           lit<",">) +
      lit<"}">;
  struct CreateGraphTypeStmt
      : rule<ikw<"CREATE"> +
             ((option(ikw<"PROPERTY">) + ikw<"GRAPH"> + ikw<"TYPE"> +
               option(ikw<"IF"> + ikw<"NOT"> +
                      flag<&ast::CreateGraphTypeStmt::ifNotExists>(
                          ikw<"EXISTS">))) |
              (flag<&ast::CreateGraphTypeStmt::orReplace>(ikw<"OR"> +
                                                               ikw<"REPLACE">) +
               option(ikw<"PROPERTY">) + ikw<"GRAPH"> + ikw<"TYPE">)) +
             assign<&ast::CreateGraphTypeStmt::name>(ref<CatalogObjRef>) +
             ((option(ikw<"AS">) + ikw<"COPY"> + ikw<"OF"> +
               assign<&ast::CreateGraphTypeStmt::copyOf>(ref<CatalogObjRef>)) |
              (ikw<"LIKE"> +
               assign<&ast::CreateGraphTypeStmt::likeGraph>(ref<GraphExpr>)) |
              (option(ikw<"AS">) + GraphTypeSpecElements))> {};
  struct DropGraphTypeStmt
      : rule<ikw<"DROP"> + option(ikw<"PROPERTY">) + ikw<"GRAPH"> +
             ikw<"TYPE"> +
             option(ikw<"IF"> + flag<&ast::DropGraphTypeStmt::ifExists>(
                                    ikw<"EXISTS">)) +
             assign<&ast::DropGraphTypeStmt::name>(ref<CatalogObjRef>)> {};

  static constexpr auto SimpleStatementAlt =
      ref<MatchStmt> | ref<OptionalMatchStmt> | ref<LetStmt> | ref<ForStmt> |
      ref<FilterStmt> | ref<OrderByPage> | ref<CallProcedureStmt> |
      ref<InsertStmt> | ref<SetStmt> | ref<RemoveStmt> | ref<DeleteStmt> |
      ref<CreateSchemaStmt> | ref<DropSchemaStmt> | ref<CreateGraphStmt> |
      ref<DropGraphStmt> | ref<CreateGraphTypeStmt> | ref<DropGraphTypeStmt>;
  struct SimpleStatement : rule<SimpleStatementAlt> {};

  // ---- linear statements, composite query, select ---------------------------

  struct LinearSegment
      : rule<create<ast::LinearSegment>(
            ikw<"USE"> + assign<&ast::LinearSegment::useGraph>(ref<GraphExpr>) +
            some(append<&ast::LinearSegment::simples>(ref<SimpleStatement>)))> {
  };

  static constexpr auto LinearStatementBody =
      (many(append<&ast::LinearStatement::segments>(ref<LinearSegment>)) +
       option(UseGraphClauseExpr) +
       some(append<&ast::LinearStatement::simples>(ref<SimpleStatement>)) +
       option(assign<&ast::LinearStatement::result>(ref<ResultStmt>))) |
      (option(UseGraphClauseExpr) + lit<"{"> +
       assign<&ast::LinearStatement::nested>(ref<Procedure>) + lit<"}">) |
      (option(UseGraphClauseExpr) +
       assign<&ast::LinearStatement::result>(ref<ResultStmt>));
  struct LinearStatement : rule<LinearStatementBody> {};

  struct GraphMatch : rule<assign<&ast::GraphMatch::graph>(ref<GraphExpr>) +
                           assign<&ast::GraphMatch::match>(
                               ref<MatchStmt> | ref<OptionalMatchStmt>)> {};

  static constexpr auto HavingClauseExpr =
      ikw<"HAVING"> + assign<&ast::SelectStmt::having>(ref<Expression>);
  static constexpr auto WhereClauseExpr =
      ikw<"WHERE"> + assign<&ast::SelectStmt::where>(ref<Expression>);
  static constexpr auto SelectGroupByExpr =
      ikw<"GROUP"> + ikw<"BY"> +
      ((some(append<&ast::SelectStmt::groupBy>(ref<Expression>), lit<",">)) |
       (lit<"("> + lit<")">));

  struct SelectStmt
      : rule<ikw<"SELECT"> +
             option(flag<&ast::SelectStmt::distinct>(ikw<"DISTINCT">) |
                    ikw<"ALL">) +
             (flag<&ast::SelectStmt::star>(lit<"*">) |
              some(append<&ast::SelectStmt::items>(ref<ReturnItem>),
                   lit<",">)) +
             option(ikw<"FROM"> +
                    ((some(append<&ast::SelectStmt::fromGraphMatches>(
                               ref<GraphMatch>),
                           lit<",">)) |
                     (lit<"{"> +
                      assign<&ast::SelectStmt::fromQuery>(ref<Procedure>) +
                      lit<"}">) |
                     (assign<&ast::SelectStmt::fromGraph>(ref<GraphExpr>) +
                      lit<"{"> +
                      assign<&ast::SelectStmt::fromQuery>(ref<Procedure>) +
                      lit<"}">)) +
                    option(WhereClauseExpr) + option(SelectGroupByExpr) +
                    option(HavingClauseExpr) +
                    option(assign<&ast::SelectStmt::page>(ref<OrderByPage>)))> {
  };

  static constexpr auto QueryOperand = ref<LinearStatement> | ref<SelectStmt>;
  struct QueryOperation
      : rule<assign<&ast::QueryOperation::op>(ikw<"UNION"> | ikw<"EXCEPT"> |
                                              ikw<"INTERSECT"> |
                                              ikw<"OTHERWISE">) +
             option(assign<&ast::QueryOperation::quantifier>(ikw<"ALL"> |
                                                             ikw<"DISTINCT">)) +
             assign<&ast::QueryOperation::right>(QueryOperand)> {};
  static constexpr auto CompositeQueryBody =
      assign<&ast::CompositeQuery::first>(QueryOperand) +
      many(append<&ast::CompositeQuery::operations>(ref<QueryOperation>));
  struct CompositeQuery : rule<CompositeQueryBody> {};

  // ---- procedure ------------------------------------------------------------

  struct VarKindGraph : terminal<option(ikw<"PROPERTY">) + ikw<"GRAPH">,
                                 value<ast::VarKind::graph>> {};
  struct VarKindTable : terminal<option(ikw<"BINDING">) + ikw<"TABLE">,
                                 value<ast::VarKind::bindingTable>> {};
  struct VarKindValue : terminal<ikw<"VALUE">, value<ast::VarKind::value>> {};

  struct VarDef
      : rule<(assign<&ast::VarDef::kind>(ref<VarKindGraph>) +
              assign<&ast::VarDef::name>(RegularIdentifier) +
              option(option(Typed) + assign<&ast::VarDef::type>(ref<TypeRef>)) +
              lit<"="> + assign<&ast::VarDef::graphInit>(ref<GraphExpr>)) |
             (assign<&ast::VarDef::kind>(ref<VarKindTable>) +
              assign<&ast::VarDef::name>(RegularIdentifier) +
              option(option(Typed) + assign<&ast::VarDef::type>(ref<TypeRef>)) +
              lit<"="> +
              assign<&ast::VarDef::tableInit>(ref<BindingTableExpr>)) |
             (assign<&ast::VarDef::kind>(ref<VarKindValue>) +
              assign<&ast::VarDef::name>(RegularIdentifier) +
              option(option(Typed) + assign<&ast::VarDef::type>(ref<TypeRef>)) +
              lit<"="> + assign<&ast::VarDef::valueInit>(ref<Expression>))> {};

  static constexpr auto StatementAlt =
      ref<CompositeQuery> | ref<LinearStatement>;
  struct NextStatement
      : rule<ikw<"NEXT"> +
             option(assign<&ast::Statement::precedingYield>(ref<YieldClause>)) +
             (create<ast::CompositeQuery>(CompositeQueryBody) |
              create<ast::LinearStatement>(LinearStatementBody))> {};
  struct StatementBlock
      : rule<append<&ast::StatementBlock::statements>(StatementAlt) +
             many(append<&ast::StatementBlock::statements>(
                 ref<NextStatement>))> {};

  struct Procedure
      : rule<option(ikw<"AT"> +
                    assign<&ast::Procedure::atSchema>(ref<SchemaRef>)) +
             option(some(append<&ast::Procedure::bindingVars>(ref<VarDef>))) +
             assign<&ast::Procedure::body>(ref<StatementBlock>)> {};

  // ---- session / transaction commands ---------------------------------------

  struct SessionSetCommand
      : rule<
            ikw<"SESSION"> + ikw<"SET"> +
            ((assign<&ast::SessionSetCommand::kind>(ikw<"SCHEMA">) +
              assign<&ast::SessionSetCommand::schema>(ref<SchemaRef>)) |
             (assign<&ast::SessionSetCommand::kind>(ikw<"TIME">) + ikw<"ZONE"> +
              assign<&ast::SessionSetCommand::timeZone>(ref<StringLit>)) |
             (option(ikw<"PROPERTY">) +
              assign<&ast::SessionSetCommand::kind>(ikw<"GRAPH">) +
              ((option(ikw<"IF"> + ikw<"NOT"> +
                       flag<&ast::SessionSetCommand::ifNotExists>(
                           ikw<"EXISTS">)) +
                assign<&ast::SessionSetCommand::paramName>(ref<GENERAL_PARAM>) +
                option(option(Typed) +
                       assign<&ast::SessionSetCommand::declaredType>(
                           ref<TypeRef>)) +
                lit<"="> +
                assign<&ast::SessionSetCommand::graph>(ref<GraphExpr>)) |
               assign<&ast::SessionSetCommand::graph>(ref<GraphExpr>))) |
             (option(ikw<"BINDING">) +
              assign<&ast::SessionSetCommand::kind>(ikw<"TABLE">) +
              option(ikw<"IF"> + ikw<"NOT"> +
                     flag<&ast::SessionSetCommand::ifNotExists>(
                         ikw<"EXISTS">)) +
              assign<&ast::SessionSetCommand::paramName>(ref<GENERAL_PARAM>) +
              option(
                  option(Typed) +
                  assign<&ast::SessionSetCommand::declaredType>(ref<TypeRef>)) +
              lit<"="> +
              assign<&ast::SessionSetCommand::tableInit>(
                  ref<BindingTableExpr>)) |
             (assign<&ast::SessionSetCommand::kind>(ikw<"VALUE">) +
              option(ikw<"IF"> + ikw<"NOT"> +
                     flag<&ast::SessionSetCommand::ifNotExists>(
                         ikw<"EXISTS">)) +
              assign<&ast::SessionSetCommand::paramName>(ref<GENERAL_PARAM>) +
              option(
                  option(Typed) +
                  assign<&ast::SessionSetCommand::declaredType>(ref<TypeRef>)) +
              lit<"="> +
              assign<&ast::SessionSetCommand::valueInit>(ref<Expression>)))> {};

  struct SessionResetCommand
      : rule<ikw<"SESSION"> + ikw<"RESET"> +
             option((option(ikw<"ALL">) +
                     assign<&ast::SessionResetCommand::scope>(
                         ikw<"PARAMETERS"> | ikw<"CHARACTERISTICS">)) |
                    (assign<&ast::SessionResetCommand::scope>(ikw<"SCHEMA">)) |
                    (option(ikw<"PROPERTY">) +
                     assign<&ast::SessionResetCommand::scope>(ikw<"GRAPH">)) |
                    (assign<&ast::SessionResetCommand::scope>(ikw<"TIME">) +
                     ikw<"ZONE">) |
                    (option(assign<&ast::SessionResetCommand::scope>(
                         ikw<"PARAMETER">)) +
                     assign<&ast::SessionResetCommand::paramName>(
                         ref<GENERAL_PARAM>)))> {};

  struct TransactionCharacteristic
      : rule<flag<&ast::TransactionCharacteristic::write>(ikw<"READ"> +
                                                               ikw<"WRITE">) |
             (create<ast::TransactionCharacteristic> + ikw<"READ"> +
              ikw<"ONLY">)> {};

  struct EndTransaction
      : rule<(create<ast::EndTransaction>(
                 flag<&ast::EndTransaction::commit>(ikw<"COMMIT">))) |
             (create<ast::EndTransaction> + ikw<"ROLLBACK">)> {};

  static constexpr auto SessionActivityExpr =
      some(append<&ast::Program::sessionResets>(ref<SessionResetCommand>)) |
      (some(append<&ast::Program::sessionSets>(ref<SessionSetCommand>)) +
       many(append<&ast::Program::sessionResets>(ref<SessionResetCommand>)));

  static constexpr auto TxCharacteristicsExpr = some(
      append<&ast::Program::txCharacteristics>(ref<TransactionCharacteristic>),
      lit<",">);

  static constexpr auto TransactionActivityExpr =
      (flag<&ast::Program::startTransaction>(ikw<"START"> +
                                                  ikw<"TRANSACTION">) +
       option(TxCharacteristicsExpr) +
       option(assign<&ast::Program::procedure>(ref<Procedure>) +
              option(assign<&ast::Program::endTransaction>(
                  ref<EndTransaction>)))) |
      (assign<&ast::Program::procedure>(ref<Procedure>) +
       option(assign<&ast::Program::endTransaction>(ref<EndTransaction>))) |
      assign<&ast::Program::endTransaction>(ref<EndTransaction>);

  static constexpr auto SessionCloseExpr =
      flag<&ast::Program::sessionClose>(ikw<"SESSION"> + ikw<"CLOSE">);

  struct Root : rule<create<ast::Program>(
                    ((SessionActivityExpr | TransactionActivityExpr) +
                     option(SessionCloseExpr)) |
                    SessionCloseExpr)> {};

  using entry = Root;
  using ignore = WS;
  using hidden = list<BLOCK_COMMENT, LINE_COMMENT_SLASH, LINE_COMMENT_DASH>;
};

} // namespace gql
