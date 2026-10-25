#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "dot/core/ast.hpp"

namespace dot {

inline constexpr auto unquote = [](std::string_view text) {
  return text.substr(1, text.size() - 2);
};
using Unquoted = cursiv::text_conv<unquote>;

inline constexpr auto quoted = [](std::string_view text,
                                    std::pmr::memory_resource* memory) {
  text = text.substr(1, text.size() - 2);
  if (text.find('\\') == text.npos) return text;
  const auto room = cursiv::text_room(memory, text.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (text[i] == '\\' && i + 1 < text.size()) {
      const char c = text[++i];
      if (c == '\n') continue;
      if (c != '"') room[n++] = '\\';
      room[n++] = c;
    } else room[n++] = text[i];
  }
  return std::string_view(room.data(), n);
};

inline constexpr auto concatenate = [](cursiv::Parts parts,
                                         std::pmr::memory_resource* memory) {
  if (parts.size() == 1) return parts[0];
  std::size_t size = 0;
  for (std::size_t i = 0; i < parts.size(); i += 2) size += parts[i].size();
  const auto room = cursiv::text_room(memory, size);
  std::size_t n = 0;
  for (std::size_t i = 0; i < parts.size(); i += 2)
    for (char c : parts[i]) room[n++] = c;
  return std::string_view(room.data(), n);
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
      : terminal<lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\)">) + lit<"\"">,
                 text_conv<quoted>> {};
  struct HTML_STRING
      : terminal<lit<"<"> + many(ref<HTML_STRING> | cr<"^<>">) + lit<">">,
                 Unquoted> {};
  static constexpr auto quotedPart = ref<STRING> | ref<HTML_STRING>;
  struct QuotedId : rule<quotedPart + many(lit<"+"> + quotedPart),
                         parts_conv<concatenate>> {};
  static constexpr auto Id = ref<ID> | ref<QuotedId> | ref<NUMBER>;

  struct ArrowOp : terminal<lit<"->">, value<ast::EdgeOp::directed>> {};
  struct DashOp : terminal<lit<"--">, value<ast::EdgeOp::undirected>> {};
  struct GraphAttrKw : terminal<ikw<"graph">, value<ast::AttrTarget::graphAttr>> {};
  struct NodeAttrKw : terminal<ikw<"node">, value<ast::AttrTarget::nodeAttr>> {};
  struct EdgeAttrKw : terminal<ikw<"edge">, value<ast::AttrTarget::edgeAttr>> {};

  struct StmtRule;
  struct AttrPairRule
      : rule<assign<&ast::AttrPair::name>(Id) +
             option(lit<"="> + assign<&ast::AttrPair::value>(Id)) +
             option(lit<";"> | lit<",">)> {};
  struct AttrListRule
      : rule<some(lit<"["> + many(append<&ast::AttrList::pairs>(ref<AttrPairRule>)) + lit<"]">)> {};
  struct NodeIdRule
      : rule<assign<&ast::NodeId::id>(Id) +
             option(lit<":"> + assign<&ast::NodeId::portName>(Id) +
                    option(lit<":"> + assign<&ast::NodeId::compass>(Id)))> {};
  struct NodeGroupRule
      : rule<some(append<&ast::NodeGroup::nodes>(ref<NodeIdRule>), lit<",">)> {};
  struct SubgraphRule;
  static constexpr auto Endpoint = ref<NodeGroupRule> | ref<SubgraphRule>;
  struct CompoundStmtRule
      : rule<append<&ast::CompoundStmt::endpoints>(Endpoint) +
             many(append<&ast::CompoundStmt::ops>(ref<ArrowOp> | ref<DashOp>) +
                  append<&ast::CompoundStmt::endpoints>(Endpoint)) +
             option(assign<&ast::CompoundStmt::attrs>(ref<AttrListRule>))> {};
  struct AttrStmtRule
      : rule<assign<&ast::AttrStmt::target>(ref<GraphAttrKw> | ref<NodeAttrKw> | ref<EdgeAttrKw>) +
             option(assign<&ast::AttrStmt::macroName>(Id) + lit<"=">) +
             assign<&ast::AttrStmt::attrs>(ref<AttrListRule>)> {};
  struct AssignStmtRule
      : rule<assign<&ast::AssignStmt::name>(Id) + lit<"="> +
             assign<&ast::AssignStmt::value>(Id)> {};
  struct SubgraphRule
      : rule<option(ikw<"subgraph"> + option(assign<&ast::Subgraph::id>(Id))) +
             lit<"{"> + many(append<&ast::Subgraph::statements>(ref<StmtRule>) + option(lit<";">)) + lit<"}">> {};
  struct StmtRule : rule<ref<AttrStmtRule> | ref<AssignStmtRule> | ref<CompoundStmtRule>> {};
  struct GraphRule
      : rule<option(flag<&ast::Graph::strict_>(ikw<"strict">)) +
             (flag<&ast::Graph::directed>(ikw<"digraph">) | ikw<"graph">) +
             option(assign<&ast::Graph::id>(Id)) + lit<"{"> +
             many(append<&ast::Graph::statements>(ref<StmtRule>) + option(lit<";">)) + lit<"}">> {};

  using entry = GraphRule;
  using ignore = list<WS, ML_COMMENT, SL_COMMENT, PREPROC, BOM>;
};

}  // namespace dot
