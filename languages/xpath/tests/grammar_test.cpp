#include "audit_regressions.hpp"
// The grammar end to end: every document under example/ and tests/ parses
// clean (recovered() false, no diagnostics, the right root type), and a few
// real field values come back out of one document's AST.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "xpath/core/CoreServices.hpp"
#include "xpath/core/ast.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const std::string& what) {
  std::fprintf(stderr, "xpath: %s\n", what.c_str());
  return 1;
}

std::string read_file(const fs::path& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  const auto language = cursiv::create_language<
      xpath::XpathCoreServices, xpath::XpathSharedCoreServices,
      xpath::install_core_module>();

  std::vector<fs::path> docs;
  // `features.xpath` lives beside this test, not in the corpus: it is
  // written here for the constructs the corpus never reaches.
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const fs::directory_entry& entry :
        fs::recursive_directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".xpath")
        docs.push_back(entry.path());
  if (docs.empty()) return fail("no example documents found");

  fs::path readback_doc;
  for (const fs::path& doc : docs) {
    const std::string text = read_file(doc);
    const cursiv::ParseResult result =
        language.language.parser->parse(text);
    if (!result.accepted) return fail(doc.string() + ": the parse was refused");
    if (result.cst().recovered())
      return fail(doc.string() + ": the parse needed repair");
    if (!result.diagnostics.empty())
      return fail(doc.string() + ": the parse left diagnostics");
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<xpath::ast::Document>)
      return fail(doc.string() + ": the parse built no document");
    if (doc.filename() == "expressions_028.xpath") readback_doc = doc;
  }

  if (readback_doc.empty())
    return fail("expressions_028.xpath was not found among the examples");

  // Read a few real field values out of one document's AST: a converter or a
  // guard that silently misfires would still leave `recovered()` false and
  // the root type right, so only reading the fields back catches it.
  const std::string text = read_file(readback_doc);
  const cursiv::ParseResult result = language.language.parser->parse(text);
  const auto& doc = *static_cast<const xpath::ast::Document*>(result.value);
  if (doc.expressions.empty())
    return fail("expressions_028.xpath: no expressions read");

  bool found_arrow = false;
  bool found_treat = false;
  // Each top-level entry is `expr;`, and `expr` always builds a
  // SequenceExpr (even for one item), so the interesting node is inside it.
  for (const xpath::ast::Expr* const stmt : doc.expressions) {
    if (stmt->type != &cursiv::ast_type_tag<xpath::ast::SequenceExpr>)
      continue;
    for (const xpath::ast::Expr* const expr :
        static_cast<const xpath::ast::SequenceExpr*>(stmt)->items) {
      if (expr->type == &cursiv::ast_type_tag<xpath::ast::ArrowExpr>)
        found_arrow = true;
      if (expr->type->derivesFrom(&cursiv::ast_type_tag<xpath::ast::TreatExpr>))
        found_treat = true;
    }
  }
  if (!found_arrow) return fail("expressions_028.xpath: no arrow expression read");
  if (!found_treat) return fail("expressions_028.xpath: no treat expression read");

  // `text()` must read as a kind test, not a call to a function named
  // "text": ReservedFunctionName in the grammar exists for exactly this.
  {
    const std::string kt = "text();";
    const cursiv::ParseResult ktResult =
        language.language.parser->parse(kt);
    if (!ktResult.accepted || ktResult.cst().recovered())
      return fail("text(): did not parse");
    const auto& ktDoc =
        *static_cast<const xpath::ast::Document*>(ktResult.value);
    if (ktDoc.expressions.size() != 1 ||
        ktDoc.expressions[0]->type !=
            &cursiv::ast_type_tag<xpath::ast::SequenceExpr>)
      return fail("text(): wrong shape");
    const auto& seq =
        *static_cast<const xpath::ast::SequenceExpr*>(ktDoc.expressions[0]);
    if (seq.items.size() != 1 ||
        seq.items[0]->type != &cursiv::ast_type_tag<xpath::ast::AxisStep>)
      return fail("text(): did not read as a kind test");
  }

  for (const std::string source : {std::string("\"\xe6\"", 3),
      std::string("'\x80'", 3), std::string("Q{u\xc3}x"),
      std::string("1(: outer (: \xc3 :) :) +2")}) {
    const auto r = language.language.parser->parse(source);
    if (r.accepted && !r.cst().recovered() && r.diagnostics.empty())
      return fail("malformed UTF-8 accepted in an XPath text body");
  }
  for (const auto source : {"\"é😀\"", "Q{urn:é}名", "1(: é (: 😀 :) :)+2"}) {
    const auto r = language.language.parser->parse(source);
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty())
      return fail("valid Unicode XPath text rejected");
  }
  std::printf("xpath: %zu documents parse clean\n", docs.size());
  return 0;
}
