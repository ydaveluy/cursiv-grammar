#pragma once

#include <cstdio>
#include <string>
#include <unordered_set>

#include "bcpl/core/SectionTags.hpp"
#include "bcpl/core/ast.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"

namespace bcpl_sections {
namespace ast = bcpl::ast;
inline bool fail(const char* message, std::string_view source) {
  std::fprintf(stderr, "bcpl section tags: %s\n%.*s\n", message,
               static_cast<int>(source.size()), source.data());
  return false;
}
inline bool ownership(const auto& services, const cursiv::ParseResult& parsed,
                      std::string_view source) {
  if (!parsed.value || parsed.value->parent || parsed.cst().text() != source) return false;
  bool valid = true;
  std::unordered_set<const cursiv::AstNode*> seen;
  const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
    if (!seen.insert(&node).second || node.cstNodeId >= parsed.cst().size()) {
      valid = false;
      return;
    }
    const auto& span = parsed.cst()[node.cstNodeId];
    valid &= span.begin <= span.end && span.end <= source.size();
    services.astReflection->visitMembers(node,
        [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
          valid &= child.parent == &node &&
                   parsed.cst().contains(node.cstNodeId, child.cstNodeId);
          self(self, child);
        });
  };
  visit(visit, *parsed.value);
  return valid && seen.size() == parsed.nodes.size();
}
inline bool run(const auto& services) {
  const std::string ancestor = "LET start() BE $(A $(B $(C RETURN $)A";
  const auto close = ancestor.rfind("$)A");
  const std::string expanded = ancestor.substr(0, close) + "} } " + ancestor.substr(close);
  cursiv::SourceTranslation translation(ancestor);
  bcpl::translate_section_tags(ancestor, translation);
  translation.finish();
  const auto mapped = translation.original({static_cast<cursiv::TextOffset>(close),
                                           static_cast<cursiv::TextOffset>(close + 4)});
  if (!translation.errors().empty() || translation.text() != expanded ||
      mapped.begin != close || mapped.end != close)
    return fail("two implicit closers and zero-width original source mapping", ancestor);
  const auto parsed = services.parser->parse(ancestor);
  if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty() ||
      !ownership(services, parsed, ancestor) || !parsed.cst().translation() ||
      parsed.cst().translation()->text() != expanded)
    return fail("ancestor close, original CST text or ownership", ancestor);
  unsigned outer = 0, implicit = 0;
  for (const auto* node : parsed.nodes) {
    if (const auto* block = cursiv::ast_cast<ast::CompoundStmt>(node)) {
      if (block->openTag == "A" && block->closeTag == "A") ++outer;
      else if ((block->openTag == "B" || block->openTag == "C") && block->closeTag.empty()) {
        if (parsed.cst()[block->cstNodeId].end != close)
          return fail("implicit block ends at written ancestor close", ancestor);
        ++implicit;
      }
    }
  }
  if (outer != 1 || implicit != 2) return fail("explicit and implicit close tags retained", ancestor);

  const std::string families =
      "MANIFEST $(M x=1 $)M\nSTATIC $(S y=2 $)S\nGLOBAL $(G z:3 $)G\n"
      "LET start() BE $(B LET a=1\n$(C RETURN $)C $)B";
  const auto family = services.parser->parse(families);
  unsigned found = 0;
  for (const auto* node : family.nodes) {
    if (const auto* n = cursiv::ast_cast<ast::ManifestDecl>(node))
      found += n->openTag == "M" && n->closeTag == "M" && n->items.size() == 1;
    if (const auto* n = cursiv::ast_cast<ast::StaticDecl>(node))
      found += n->openTag == "S" && n->closeTag == "S" && n->items.size() == 1;
    if (const auto* n = cursiv::ast_cast<ast::GlobalDecl>(node))
      found += n->openTag == "G" && n->closeTag == "G" && n->items.size() == 1;
    if (const auto* n = cursiv::ast_cast<ast::Block>(node))
      found += n->openTag == "B" && n->closeTag == "B" && n->declarations.size() == 1;
    if (const auto* n = cursiv::ast_cast<ast::CompoundStmt>(node))
      found += n->openTag == "C" && n->closeTag == "C" && n->commands.size() == 1;
  }
  if (!family.accepted || family.cst().recovered() || !family.diagnostics.empty() ||
      found != 5 || !ownership(services, family, families))
    return fail("all five section-container AST fields", families);

  const std::string declarations = "LET start() BE $(B MANIFEST $(M x=1 $)B";
  const auto declarationResult = services.parser->parse(declarations);
  unsigned declarationOuter = 0, declarationInner = 0;
  for (const auto* node : declarationResult.nodes) {
    if (const auto* n = cursiv::ast_cast<ast::Block>(node))
      declarationOuter += n->openTag == "B" && n->closeTag == "B" &&
                          n->declarations.size() == 1 && n->commands.empty();
    if (const auto* n = cursiv::ast_cast<ast::ManifestDecl>(node))
      declarationInner += n->openTag == "M" && n->closeTag.empty() && n->items.size() == 1;
  }
  if (!declarationResult.accepted || declarationResult.cst().recovered() ||
      !declarationResult.diagnostics.empty() || declarationOuter != 1 || declarationInner != 1 ||
      !ownership(services, declarationResult, declarations))
    return fail("implicit declaration close needs no invented command separator", declarations);

  const std::string empty = "MANIFEST $(M $)M\nSTATIC $(S $)S\nGLOBAL $(G $)G";
  const auto emptyResult = services.parser->parse(empty);
  unsigned empties = 0;
  for (const auto* node : emptyResult.nodes) {
    if (const auto* n = cursiv::ast_cast<ast::ManifestDecl>(node))
      empties += n->openTag == "M" && n->closeTag == "M" && n->items.empty();
    if (const auto* n = cursiv::ast_cast<ast::StaticDecl>(node))
      empties += n->openTag == "S" && n->closeTag == "S" && n->items.empty();
    if (const auto* n = cursiv::ast_cast<ast::GlobalDecl>(node))
      empties += n->openTag == "G" && n->closeTag == "G" && n->items.empty();
  }
  if (!emptyResult.accepted || emptyResult.cst().recovered() || !emptyResult.diagnostics.empty() ||
      empties != 3 || !ownership(services, emptyResult, empty))
    return fail("native empty declaration sections keep tags and AST", empty);

  for (const char* text : {
           "LET start() BE $(outer RETURN $)outer",
           "LET start() BE $(outer $(inner RETURN $)outer",
           "LET start() BE $(A { RETURN $)A",
           "LET start() BE $(A $(A RETURN $)A $)A",
           "LET start() BE $(12._tag RETURN $)12._tag",
           "LET start() BE $(A /* $(bad /* $)A */ $)bad */ // $)A\nRETURN $)A",
           R"bcpl(LET text() = "$(fake $)fake")bcpl",
           "LET text() = \"left* // $(fake $)A *\"quote*\"\n *right\"",
           "LET dollar() = '$'", "LET brace() = '}'",
           "LET start() BE $( RETURN }", "LET start() BE { RETURN $)",
           "LET start() BE { LET x=1 }",
           "MANIFEST $(M x=1; $)M\nSTATIC $(S y=2; $)S\nGLOBAL $(G z:3; $)G",
           "SECTION \"$(fake $)fake\"\nLET start() BE { RETURN }"}) {
    const auto result = services.parser->parse(text);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !ownership(services, result, text)) return fail("native section/noninterference input", text);
  }

  // Unknown/mismatched tags receive a local diagnostic. The old native
  // frontend's outermost recovery is not established as an invalidity oracle.
  for (const char* text : {"LET start() BE $(A RETURN }",
                           "LET start() BE $(A RETURN $)B",
                           "LET start() BE $(A RETURN $)a"}) {
    const std::string source(text);
    const auto at = source.rfind("$)");
    const auto closer = at == std::string::npos ? source.rfind('}') : at;
    const auto result = services.parser->parse(source);
    bool located = false;
    for (const auto& diagnostic : result.diagnostics)
      located |= diagnostic.kind == cursiv::DiagnosticKind::conversion_error &&
                 diagnostic.begin == closer && diagnostic.end == source.size() &&
                 !diagnostic.message.empty();
    if (!located) return fail("local unmatched-tag diagnostic in original source", source);
  }
  // An unclosed source is not silently completed at EOF by the translator.
  const std::string range = "$(tag..";
  cursiv::SourceTranslation untouched(range);
  bcpl::translate_section_tags(range, untouched);
  untouched.finish();
  if (untouched.text() != range || !untouched.errors().empty())
    return fail("no invented EOF closure", range);
  return true;
}
}  // namespace bcpl_sections
