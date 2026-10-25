// The smoke test of the language: every file under example/ parses clean
// and builds a Document. The exit code is the verdict.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <variant>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "html/core/CoreServices.hpp"
#include "html/core/ast.hpp"
#include "raw_text_regressions.hpp"

namespace {

constexpr const char *kExamples[] = {
    CURSIV_EXAMPLE_DIR "/example/abc.com.html",
    CURSIV_EXAMPLE_DIR "/example/freebsd.html",
    CURSIV_EXAMPLE_DIR "/example/gnu.html",
    CURSIV_EXAMPLE_DIR "/example/html4.html",
    CURSIV_EXAMPLE_DIR "/example/nbc.com.html",
    CURSIV_EXAMPLE_DIR "/example/script1.html",
    CURSIV_EXAMPLE_DIR "/example/style1.html",
    CURSIV_EXAMPLE_DIR "/example/wikipedia.html",
    CURSIV_TEST_DIR "/custom-constructs.html",
};

void fail(const char *file, const char *what, bool &ok) {
  std::fprintf(stderr, "html: %s: %s\n", file, what);
  ok = false;
}

// custom-constructs.html is the only example exercising a scriptlet, a
// tag/attribute name outside ASCII, and a CDATA section outside <script>.
void check_custom_constructs(const html::ast::Document &doc, bool &ok) {
  if (doc.doctypes.empty())
    fail("custom-constructs.html", "no doctype read back", ok);

  int scriptlets = 0;
  bool sawUnicodeElement = false;
  for (const auto &item : doc.content) {
    if (const auto *const *el = std::get_if<html::ast::Element *>(&item);
        el != nullptr && *el != nullptr && (*el)->name == "html") {
      for (const auto &inner : (*el)->content) {
        if (std::holds_alternative<html::ast::Scriptlet *>(inner))
          ++scriptlets;
        const auto *const *body = std::get_if<html::ast::Element *>(&inner);
        if (body == nullptr || *body == nullptr || (*body)->name != "body")
          continue;
        for (const auto &child : (*body)->content) {
          const auto *const *childEl =
              std::get_if<html::ast::Element *>(&child);
          if (childEl == nullptr || *childEl == nullptr)
            continue;
          const html::ast::Element &e = **childEl;
          if (e.name == "書籍") {
            sawUnicodeElement = true;
            if (e.attributes.size() != 2 || e.attributes[0]->name != "版·号")
              fail("custom-constructs.html",
                   "unicode attribute name read back wrong", ok);
          }
          if (e.name == "data") {
            bool sawCdata = false;
            for (const auto &c : e.content)
              if (std::holds_alternative<html::ast::CData *>(c))
                sawCdata = true;
            if (!sawCdata)
              fail("custom-constructs.html", "no CDATA child under <data>", ok);
          }
        }
      }
    }
  }
  if (scriptlets != 2)
    fail("custom-constructs.html", "did not read back two scriptlets", ok);
  if (!sawUnicodeElement)
    fail("custom-constructs.html", "no element named 書籍", ok);
}

} // namespace

int main() {
  if (!html_raw_text_regressions::run()) return 1;
  {
    const auto language = cursiv::create_language<
        html::HtmlCoreServices, html::HtmlSharedCoreServices,
        html::install_core_module>();
    const auto duplicate = cursiv::test::validationHelper(
        language.language, "<!DOCTYPE html><!DOCTYPE html><br>");
    const auto duplicate_xml = cursiv::test::validationHelper(
        language.language, "<?xml version='1.0'?><?xml version='1.1'?><br>");
    if (duplicate.diagnostics.size() != 1 ||
        duplicate_xml.diagnostics.size() != 1) return 1;
  }
  {
    const auto audit_language =
        cursiv::create_language<html::HtmlCoreServices,
                                html::HtmlSharedCoreServices,
                                html::install_core_module>();
    const auto audit_result =
        audit_language.language.parser->parse("<DIV>x</div><BR>");
    if (!audit_result.accepted || audit_result.cst().recovered() ||
        !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
    const auto valid = cursiv::test::validationHelper(audit_language.language,
                                                      "<DIV>x</div><BR>");
    const auto mismatch = cursiv::test::validationHelper(
        audit_language.language, "<DIV>x</span>");
    if (!valid.diagnostics.empty() || mismatch.diagnostics.size() != 1)
      return 1;
  }

  const auto language = cursiv::create_language<html::HtmlCoreServices,
                                                html::HtmlSharedCoreServices,
                                                html::install_core_module>();

  bool ok = true;
  for (const char *path : kExamples) {
    std::ifstream file(path);
    if (!file) {
      fail(path, "the sample document is not readable", ok);
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      fail(path, "does not parse clean", ok);
      continue;
    }
    if (!result.diagnostics.empty()) {
      fail(path, "parsed with non-empty diagnostics", ok);
      continue;
    }

    const cursiv::AstNode *const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<html::ast::Document>) {
      fail(path, "did not build a Document", ok);
      continue;
    }

    const auto &doc = *static_cast<const html::ast::Document *>(root);
    if (std::string_view(path).find("custom-constructs.html") !=
        std::string_view::npos)
      check_custom_constructs(doc, ok);
  }

  if (ok)
    std::puts("html: every example parses");
  return ok ? 0 : 1;
}
