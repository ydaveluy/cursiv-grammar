#pragma once

#include <string>
#include <string_view>

#include "cursiv/grammar/GrammarSpec.hpp"
#include "dot/ast.hpp"

namespace dot {

inline constexpr auto unquote = [](std::string_view text) {
  return text.substr(1, text.size() - 2);
};

inline constexpr auto quoted = [](std::string_view text) -> std::string {
  text = text.substr(1, text.size() - 2);
  if (text.find('\\') == text.npos) return std::string(text);
  std::string out;
  out.reserve(text.size());
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (text[i] == '\\' && i + 1 < text.size()) {
      const char c = text[++i];
      if (c == '\n') continue;
      if (c != '"') out += '\\';
      out += c;
    } else out += text[i];
  }
  return out;
};

inline constexpr auto concatenate = [](cursiv::Parts parts) -> std::string {
  if (parts.size() == 1) return std::string(parts[0]);
  std::size_t size = 0;
  for (std::size_t i = 0; i < parts.size(); i += 2) size += parts[i].size();
  std::string out;
  out.reserve(size);
  for (std::size_t i = 0; i < parts.size(); i += 2) out += parts[i];
  return out;
};

struct Dot : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\n\r)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\n)">)> {};
  struct PREPROC : terminal<lit<"#"> + many(cr<R"(^\n)">)> {};
  // The native scanner skips a standalone BOM, but a BOM adjoining a
  // name is part of that name (the longer NAME token wins).
  static constexpr auto NameContinue =
      cr<"a-zA-Z_0-9"> | cr<R"(\u{80}-\u{10FFFF})">;
  static constexpr auto StandaloneBom = lit<"\xEF\xBB\xBF"> + !NameContinue;
  struct BOM : terminal<StandaloneBom> {};

  struct ID : terminal<!StandaloneBom + (cr<"a-zA-Z_"> | cr<R"(\u{80}-\u{10FFFF})">) +
                       many(cr<"a-zA-Z_0-9"> | cr<R"(\u{80}-\u{10FFFF})">)> {};
  struct NUMBER
      : terminal<option(lit<"-">) +
                 ((lit<"."> + some(d)) | (some(d) + option(lit<"."> + many(d))))> {};
  struct STRING
      : terminal<as<quoted>(
            lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\)">) + lit<"\"">)> {
  };
  struct HTML_STRING
      : terminal<as<unquote>(lit<"<"> + many(ref<HTML_STRING> | cr<"^<>">) +
                             lit<">">)> {};
  static constexpr auto quotedPart = ref<STRING> | ref<HTML_STRING>;
  struct QuotedId
      : rule<as<concatenate>(quotedPart + many(lit<"+"> + quotedPart))> {};
  static constexpr auto Id = ref<ID> | ref<QuotedId> | ref<NUMBER>;

  // Graphviz treats a standalone HTML value differently from an ordinary
  // string with the same text. Concatenation produces an ordinary string.
  template <auto Value, auto Html>
  static constexpr auto AttributeValue =
      flag<Html>(assign<Value>(ref<HTML_STRING>)) | assign<Value>(Id);

  struct Stmt;
  struct AttrPair
      : rule<assign<&ast::AttrPair::name>(Id) +
             option(lit<"="> + AttributeValue<&ast::AttrPair::value,
                                             &ast::AttrPair::valueIsHtml>) +
             option(lit<";"> | lit<",">)> {};
  struct AttrList
      : rule<some(lit<"["> + many(append<&ast::AttrList::pairs>(ref<AttrPair>)) + lit<"]">)> {};
  struct NodeId
      : rule<assign<&ast::NodeId::id>(Id) +
             option(lit<":"> + assign<&ast::NodeId::portName>(Id) +
                    option(lit<":"> + assign<&ast::NodeId::compass>(Id)))> {};
  struct NodeGroup
      : rule<some(append<&ast::NodeGroup::nodes>(ref<NodeId>), lit<",">)> {};
  struct Subgraph;
  static constexpr auto Endpoint = ref<NodeGroup> | ref<Subgraph>;
  struct CompoundStmt
      : rule<append<&ast::CompoundStmt::endpoints>(Endpoint) +
             many(append<&ast::CompoundStmt::ops>(as<ast::EdgeOp::directed>(lit<"->">) | as<ast::EdgeOp::undirected>(lit<"--">)) +
                  append<&ast::CompoundStmt::endpoints>(Endpoint)) +
             option(assign<&ast::CompoundStmt::attrs>(ref<AttrList>))> {};
  struct AttrStmt
      : rule<assign<&ast::AttrStmt::target>(as<ast::AttrTarget::graphAttr>(ikw<"graph">) | as<ast::AttrTarget::nodeAttr>(ikw<"node">) | as<ast::AttrTarget::edgeAttr>(ikw<"edge">)) +
             option(assign<&ast::AttrStmt::macroName>(Id) + lit<"=">) +
             assign<&ast::AttrStmt::attrs>(ref<AttrList>)> {};
  struct AssignStmt
      : rule<assign<&ast::AssignStmt::name>(Id) + lit<"="> +
             AttributeValue<&ast::AssignStmt::value,
                            &ast::AssignStmt::valueIsHtml>> {};
  struct Subgraph
      : rule<option(ikw<"subgraph"> + option(assign<&ast::Subgraph::id>(Id))) +
             lit<"{"> + many(append<&ast::Subgraph::statements>(ref<Stmt>) + option(lit<";">)) + lit<"}">> {};
  struct Stmt : rule<ref<AttrStmt> | ref<AssignStmt> | ref<CompoundStmt>> {};
  struct Graph
      : rule<option(flag<&ast::Graph::strict_>(ikw<"strict">)) +
             (flag<&ast::Graph::directed>(ikw<"digraph">) | ikw<"graph">) +
             option(assign<&ast::Graph::id>(Id)) + lit<"{"> +
             many(append<&ast::Graph::statements>(ref<Stmt>) + option(lit<";">)) + lit<"}">> {};

  using entry = Graph;
  using ignored = list<WS, ML_COMMENT, SL_COMMENT, PREPROC, BOM>;
};

}  // namespace dot
