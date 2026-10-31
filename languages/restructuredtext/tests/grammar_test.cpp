
#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "restructuredtext/core/CoreServices.hpp"
#include "restructuredtext/core/ast.hpp"
// The smoke test of the language: build the container the way a workspace
// does, read every sample, and check the trees.

#include <fstream>
#include <iterator>
#include <string>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

using restructuredtext::ast::Block;
using restructuredtext::ast::Document;
using restructuredtext::ast::Section;

int fail(const char* what) {
  std::fprintf(stderr, "restructuredtext: %s\n", what);
  return 1;
}

// A paragraph joined across lines holds the line break between them.
bool joined(const restructuredtext::ast::Paragraph& p) {
  for (const auto& span : p.spans)
    if (const auto* text = std::get_if<std::string_view>(&span);
        text != nullptr && (*text == "\n" || *text == "\r\n"))
      return true;
  return false;
}

std::string read(const char* path) {
  std::ifstream file(path);
  return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

// Parses `path`, checks it came through clean with a Document root, and
// hands the root to `check` for the field-level reads a structural pass
// alone would miss.
template <class Language, class Check>
int expect_clean(const Language& language, const char* path, Check check) {
  const std::string text = read(path);
  if (text.empty()) return fail("a sample is empty or unreadable");

  const cursiv::ParseResult result = language.parser->parse(text);
  if (result.cst().recovered()) return fail("a sample does not parse cleanly");
  if (!result.diagnostics.empty()) return fail("a sample carries diagnostics");

  const cursiv::AstNode* const root = result.value;
  if (root == nullptr || root->type != &cursiv::ast_type_tag<Document>)
    return fail("a sample built no document");

  return check(*static_cast<const Document*>(root));
}

}  // namespace

#include "native_inline_regressions.hpp"
#include "native_followup.hpp"
#include "native_block_regressions.hpp"

static int audit_regression() {
  const auto language = cursiv::create_language<
      restructuredtext::RestructuredtextCoreServices,
      restructuredtext::RestructuredtextSharedCoreServices,
      restructuredtext::install_core_module>();
  {
    const auto r = language.language.parser->parse(
        ".. code-block:: python\n\n   print(1)\n");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
    const auto* doc = r.value_as<restructuredtext::ast::Document>();
    if (!doc || doc->blocks.empty() ||
        !std::holds_alternative<restructuredtext::ast::Directive*>(
            doc->blocks[0]))
      return 1;
    const auto* directive =
        std::get<restructuredtext::ast::Directive*>(doc->blocks[0]);
    if (!directive || directive->header != "code-block:: python" ||
        !directive->body || directive->body->entries.empty() ||
        !std::holds_alternative<std::string_view>(
            directive->body->entries[0]) ||
        std::get<std::string_view>(directive->body->entries[0]) != "print(1)")
      return 1;
  }
  return 0;
}

int main() {
  if (native_inline_regressions()) return 1;
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  const auto language = cursiv::create_language<
      restructuredtext::RestructuredtextCoreServices,
      restructuredtext::RestructuredtextSharedCoreServices,
      restructuredtext::install_core_module>();

  if (const int rc = rst_native_followup(language.language)) return rc;
  if (!rst_blocks::run(language.language)) return 1;

  if (const int rc = expect_clean(
          language.language, CURSIV_SECTIONS_SAMPLE, [](const Document& doc) {
            int sections = 0;
            const Section* overview = nullptr;
            const Section* nested = nullptr;
            for (const Block& block : doc.blocks) {
              const auto* const s = std::get_if<Section*>(&block);
              if (s == nullptr) continue;
              ++sections;
              if (overview == nullptr)
                overview = *s;
              else if (nested == nullptr)
                nested = *s;
            }
            if (sections != 2)
              return fail("sections.rst: expected 2 top-level sections");
            if (overview->underline != "=")
              return fail("sections.rst: Overview's underline misread");
            if (nested->underline != "-")
              return fail("sections.rst: Nested Section's underline misread");

            // Overview's own body: five block-start markers -- and, since
            // every one of them was refused right after running prose with
            // no blank line, a multi-line paragraph precedes each. Nested
            // Section is not among these: it opens a sibling of its own.
            const auto body =
                restructuredtext::ast::section_body(doc, overview);
            int bullets = 0, enums = 0, lineBlocks = 0, comments = 0,
                paragraphs = 0;
            bool sawJoined = false;
            for (const auto& element : body) {
              if (std::holds_alternative<restructuredtext::ast::BulletItem*>(
                      element))
                ++bullets;
              else if (std::holds_alternative<restructuredtext::ast::EnumItem*>(
                           element))
                ++enums;
              else if (std::holds_alternative<restructuredtext::ast::LineBlock*>(
                           element))
                ++lineBlocks;
              else if (std::holds_alternative<restructuredtext::ast::Comment*>(
                           element))
                ++comments;
              else if (const auto* const p = std::get_if<
                           restructuredtext::ast::Paragraph*>(&element)) {
                ++paragraphs;
                sawJoined = sawJoined || joined(**p);
              }
            }
            if (bullets != 3) return fail("sections.rst: expected 3 bullet items");
            if (enums != 2) return fail("sections.rst: expected 2 numbered items");
            if (lineBlocks != 1) return fail("sections.rst: expected 1 line block");
            if (comments != 2) return fail("sections.rst: expected 2 comments");
            if (paragraphs != 6)
              return fail("sections.rst: expected 6 paragraphs in Overview's body");
            if (!sawJoined)
              return fail("sections.rst: no paragraph reads as multi-line");

            const auto nestedBody =
                restructuredtext::ast::section_body(doc, nested);
            if (nestedBody.size() != 1)
              return fail("sections.rst: Nested Section expected one paragraph");
            const auto* const nestedParagraph = std::get_if<
                restructuredtext::ast::Paragraph*>(&nestedBody[0]);
            if (nestedParagraph == nullptr || !joined(**nestedParagraph))
              return fail("sections.rst: Nested Section's paragraph is not joined");
            return 0;
          }))
    return rc;

  if (const int rc = expect_clean(
          language.language, CURSIV_INLINE_SAMPLE, [](const Document& doc) {
            bool sawEmphasis = false, sawStrong = false, sawLiteral = false,
                 sawReference = false, sawTarget = false, sawHyperlink = false,
                 sawInterpretedText = false;
            for (const Block& block : doc.blocks) {
              const auto* const p =
                  std::get_if<restructuredtext::ast::Paragraph*>(&block);
              if (p == nullptr || *p == nullptr) continue;
              for (const auto& span : (*p)->spans) {
                if (std::holds_alternative<restructuredtext::ast::Emphasis*>(span))
                  sawEmphasis = true;
                else if (std::holds_alternative<restructuredtext::ast::Strong*>(
                             span))
                  sawStrong = true;
                else if (std::holds_alternative<restructuredtext::ast::Literal*>(
                             span))
                  sawLiteral = true;
                else if (std::holds_alternative<
                             restructuredtext::ast::Reference*>(span))
                  sawReference = true;
                else if (std::holds_alternative<
                             restructuredtext::ast::HyperlinkTarget*>(span))
                  sawTarget = true;
                else if (std::holds_alternative<
                             restructuredtext::ast::Hyperlink*>(span))
                  sawHyperlink = true;
                else if (std::holds_alternative<
                             restructuredtext::ast::InterpretedText*>(span))
                  sawInterpretedText = true;
              }
            }
            if (!sawEmphasis || !sawStrong || !sawLiteral || !sawReference ||
                !sawTarget || !sawHyperlink || !sawInterpretedText)
              return fail("inline.rst: a span kind is missing from the tree");
            return 0;
          }))
    return rc;

  // A comment's body is the indented block after it, at any indentation and
  // across a blank line: none of its lines opens a paragraph of its own.
  {
    const std::string text =
        ".. a comment\n  its body, indented two\n\n  and after a blank line\n"
        "\nTitle\n=====\n";
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto* const doc = static_cast<const Document*>(result.value);
    if (result.cst().recovered() || doc == nullptr || doc->blocks.size() != 2)
      return fail("a comment's body should not open blocks of its own");
    const auto* const comment =
        std::get_if<restructuredtext::ast::Comment*>(&doc->blocks[0]);
    if (comment == nullptr || (*comment)->bodyLines.size() != 2 ||
        !std::holds_alternative<Section*>(doc->blocks[1]))
      return fail("a comment's indented lines should be its body");
  }

  // A transition between two blocks, and list items with no text.
  {
    const std::string text = "A paragraph.\n\n-----\n\n-\n- b\n\n1.\n";
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto* const doc = static_cast<const Document*>(result.value);
    if (result.cst().recovered() || doc == nullptr || doc->blocks.size() != 5)
      return fail("a transition or an empty item does not parse");
    const auto* const empty =
        std::get_if<restructuredtext::ast::BulletItem*>(&doc->blocks[2]);
    const auto* const number =
        std::get_if<restructuredtext::ast::EnumItem*>(&doc->blocks[4]);
    if (!std::holds_alternative<restructuredtext::ast::Transition*>(
            doc->blocks[1]) ||
        empty == nullptr || !(*empty)->body.empty() || number == nullptr ||
        !(*number)->body.empty())
      return fail("a transition or an empty item is misread");
  }

  // A title's underline or overline is any punctuation byte, the text ending
  // on it or not.
  for (const char* const sample : {"Title\n~~~~~\n\npara\n", "Title\n~~~~~",
                                   "#####\nTitle\n#####\n\npara\n"}) {
    const std::string text = sample;
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto* const doc = static_cast<const Document*>(result.value);
    if (result.cst().recovered() || doc == nullptr || doc->blocks.empty())
      return fail("a title underlined by punctuation does not parse");
    const auto* const section = std::get_if<Section*>(&doc->blocks[0]);
    if (section == nullptr || (*section)->title.size() != 1 ||
        ((*section)->underline != "~" && (*section)->underline != "#"))
      return fail("a title underlined by punctuation is not a section");
  }

  // Indentation nests: an item's second paragraph, a definition, and a quote
  // indented past the definition, inside it.
  {
    const std::string text =
        "- item\n\n  second paragraph\n\nterm\n  definition\n\n   a quote\n";
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto* const doc = static_cast<const Document*>(result.value);
    if (result.cst().recovered() || doc == nullptr || doc->blocks.size() != 2)
      return fail("indented blocks do not parse");
    const auto* const item =
        std::get_if<restructuredtext::ast::BulletItem*>(&doc->blocks[0]);
    const auto* const definition =
        std::get_if<restructuredtext::ast::DefinitionItem*>(&doc->blocks[1]);
    if (item == nullptr || (*item)->body.size() != 2 ||
        definition == nullptr || (*definition)->body.size() != 2 ||
        !std::holds_alternative<restructuredtext::ast::BlockQuote*>(
            (*definition)->body[1]))
      return fail("indented blocks are misread");
  }

  // Docutils preserves a dangling backtick as literal text.
  {
    const std::string broken = "See `code with no closer.\n";
    const cursiv::ParseResult result = language.language.parser->parse(broken);
    if (!result.accepted || result.cst().recovered())
      return fail("an unmatched backtick should remain literal text");
  }

  std::puts("restructuredtext: the samples parse");
  return 0;
}
