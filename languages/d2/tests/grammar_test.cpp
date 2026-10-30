// The grammar test of this language: every document under example/ parses
// clean, and the tree's root is a Diagram.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "d2/core/CoreServices.hpp"
#include "d2/core/ast.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const fs::path& file, const char* what) {
  std::fprintf(stderr, "d2: %s: %s\n", file.string().c_str(), what);
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      d2::D2CoreServices, d2::D2SharedCoreServices, d2::install_core_module>();

  int failures = 0;
  int count = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR}) {
    for (const fs::directory_entry& entry : fs::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".d2")
        continue;
      ++count;
      std::ifstream file(entry.path());
      if (!file) {
        failures += fail(entry.path(), "the document is not readable");
        continue;
      }
      const std::string text((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

      const cursiv::ParseResult result = language.language.parser->parse(text);
      if (result.cst().recovered()) {
        failures += fail(entry.path(), "did not parse clean");
        continue;
      }
      const cursiv::AstNode* const root = result.value;
      if (root == nullptr ||
          root->type != &cursiv::ast_type_tag<d2::ast::Diagram>) {
        failures += fail(entry.path(), "did not build a Diagram root");
      }
    }
  }
  {
    const auto result = language.language.parser->parse(
        R"(vars: {x: value}
plain: $ {x}
quoted: "before $  {x} after"
path: 'C:\'
block: |🔥md text | remains inside🔥|
)");
    const auto* doc = cursiv::ast_cast<d2::ast::Diagram>(result.value);
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !doc || doc->statements.size() != 5)
      return fail("native-followup", "native strings/substitutions need repair");
    const auto* plain = cursiv::ast_cast<d2::ast::Key>(doc->statements[1]);
    const auto* quoted = cursiv::ast_cast<d2::ast::Key>(doc->statements[2]);
    const auto* path = cursiv::ast_cast<d2::ast::Key>(doc->statements[3]);
    const auto* block = cursiv::ast_cast<d2::ast::Key>(doc->statements[4]);
    const auto* text = plain ? cursiv::ast_cast<d2::ast::Text>(plain->value) : nullptr;
    const auto* quote = quoted ? cursiv::ast_cast<d2::ast::QuotedText>(quoted->value) : nullptr;
    const auto* single = path ? cursiv::ast_cast<d2::ast::SingleText>(path->value) : nullptr;
    const auto* multiline = block ? cursiv::ast_cast<d2::ast::BlockText>(block->value) : nullptr;
    const auto* sub = text && text->parts.size() == 1
                          ? cursiv::ast_cast<d2::ast::Substitution>(text->parts[0])
                          : nullptr;
    const auto* raw = single && single->parts.size() == 1
                          ? cursiv::ast_cast<d2::ast::Chunk>(single->parts[0])
                          : nullptr;
    if (!sub || !sub->path || sub->path->parts.size() != 1 ||
        !quote || quote->parts.size() != 3 ||
        !cursiv::ast_cast<d2::ast::Substitution>(quote->parts[1]) ||
        !raw || raw->raw != R"(C:\)" || !multiline || multiline->parts.size() != 1)
      return fail("native-followup", "native string/substitution AST was lost");
  }
  for (const std::string source : {
           R"($key: value
single: 'price $x'
escaped: \$x
quoted: "price \$x"
block: |md price $x |
final: price$
)",
           "a: $x\n", "a: price $x\n", "a: \"price $x\"\n"}) {
    const bool invalid = source.starts_with("a:");
    auto document = language.shared->documentFactory->fromString(
        source, "file:///native-dollar.d2");
    if (!document->parseResult.accepted ||
        document->parseResult.cst().recovered() ||
        !document->parseResult.diagnostics.empty())
      return fail("native-dollar", "invalid source must retain its AST");
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(*document, diagnostics);
    bool substitution_error = false;
    for (const auto& diagnostic : diagnostics)
      substitution_error |= diagnostic.code &&
          *diagnostic.code == "d2.substitution";
    if (substitution_error != invalid || (!invalid && !diagnostics.empty()))
      return fail("native-dollar", "wrong literal dollar/substitution diagnosis");
  }
  for (const auto source : {R"(a: "x" b)", R"(a: {b: "x" c})",
                            R"(a: ["x" "y"])"}) {
    const auto result = language.language.parser->parse(source);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return fail("native-separator", "adjacent statements or values need a separator");
  }
  {
    const auto result = language.language.parser->parse(
        "config: [\"note\"; 42; true]\n"
        "node: {shape: circle}\n"
        "start -> end: connects {style.stroke: blue}\n");
    const auto* doc = cursiv::ast_cast<d2::ast::Diagram>(result.value);
    const auto* config = doc && doc->statements.size() == 3
        ? cursiv::ast_cast<d2::ast::Key>(doc->statements[0]) : nullptr;
    const auto* values = config ? cursiv::ast_cast<d2::ast::Array>(config->value) : nullptr;
    const auto* edge = doc && doc->statements.size() == 3
        ? cursiv::ast_cast<d2::ast::Key>(doc->statements[2]) : nullptr;
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !values || values->values.size() != 3 || !edge || edge->hops.size() != 1 ||
        !edge->value || !edge->block || edge->block->statements.size() != 1)
      return fail("native-attributes", "native array/map/edge attribute AST was lost");
  }
  for (const auto source : {R"(["bare"])",
                            R"(config: ["note"] {shape: circle})",
                            R"(start -> end [ : "connects" ])", "a: {} {}"}) {
    const auto result = language.language.parser->parse(source);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return fail("native-attributes", "non-native bracket/map attachment accepted");
  }
  if (count == 0) return fail(fs::path(CURSIV_EXAMPLE_DIR), "no example found");
  if (failures != 0) return 1;

  std::printf("d2: %d example(s) parse clean\n", count);
  return 0;
}
