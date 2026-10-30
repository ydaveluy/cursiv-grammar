#include "audit_regressions.hpp"
// The desktopentry grammar end to end: every document under example/ parses
// clean -- no repair, no diagnostic -- and builds a Document.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "desktopentry/core/CoreServices.hpp"
#include "desktopentry/core/ast.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"

namespace {

int failures = 0;

int fail(const char* message) {
  std::fprintf(stderr, "desktopentry: %s\n", message);
  return 1;
}

void check_file(const cursiv::Parser &parser,
                const std::filesystem::path &path) {
  std::ifstream file(path);
  const std::string text((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

  const cursiv::ParseResult result = parser.parse(text);
  if (result.cst().recovered()) {
    std::fprintf(stderr, "desktopentry: %s did not parse clean\n",
                 path.filename().string().c_str());
    ++failures;
    return;
  }
  if (!result.diagnostics.empty()) {
    std::fprintf(stderr, "desktopentry: %s left %zu diagnostic(s)\n",
                 path.filename().string().c_str(), result.diagnostics.size());
    ++failures;
    return;
  }
  const cursiv::AstNode *const root = result.value;
  if (root == nullptr ||
      root->type != &cursiv::ast_type_tag<desktopentry::ast::Document>) {
    std::fprintf(stderr, "desktopentry: %s built no Document\n",
                 path.filename().string().c_str());
    ++failures;
  }
}

} // namespace

int main() {
  {
    const auto audit_language =
        cursiv::create_language<desktopentry::DesktopEntryCoreServices,
                                desktopentry::DesktopEntrySharedCoreServices,
                                desktopentry::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse(
        "# heading\n[Desktop Entry]\nName=001\n# between "
        "entries\nComment=#literal;with\\;semi;\n");
    if (!audit_result.accepted || audit_result.cst().recovered() ||
        !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
    bool found = false, raw = false;
    for (const auto *node : audit_result.nodes)
      if (const auto *e = cursiv::ast_cast<desktopentry::ast::Entry>(node)) {
        found = found || (e->key == "Name" && e->rawValue == "001");
        raw = raw ||
              (e->key == "Comment" && e->rawValue == "#literal;with\\;semi;");
      }
    if (!found || !raw)
      return 1;
  }

  const auto language =
      cursiv::create_language<desktopentry::DesktopEntryCoreServices,
                              desktopentry::DesktopEntrySharedCoreServices,
                              desktopentry::install_core_module>();

  if (!audit_regressions::run(language.language))
    return 1;

  for (const char* source : {
           "[Other]\nName=App\n",
           "[Desktop Entry]\nName=App\n[Desktop Entry]\nName=Other\n",
           "[Desktop Entry]\nName=App\nName=Other\n",
           "[Desktop Entry]\nName[fr]=App\nName[fr]=Other\n",
           "[Desktop Entry]\nBad_Key=value\n",
           "[Desktop Entry]\nName=App\n[Équipe]\nName=Other\n"}) {
    if (cursiv::test::validationHelper(language.language, std::string(source)).diagnostics.empty())
      return fail("Desktop Entry 1.5 structural violation was not diagnosed");
  }
  if (!cursiv::test::validationHelper(language.language,
          "[Desktop Entry]\nName=App\nName[fr]=Appli\n"
          "[Desktop Action Open]\nName=Open\n").diagnostics.empty())
    return fail("localized and independent group keys are distinct");

  int count = 0;
  for (const char *const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".desktop")
        continue;
      check_file(*language.language.parser, entry.path());
      ++count;
    }
  if (count == 0) {
    std::fprintf(stderr, "desktopentry: no example found\n");
    return 1;
  }

  if (failures != 0) {
    std::printf("desktopentry: %d failure(s) over %d example(s)\n", failures,
                count);
    return 1;
  }
  std::printf("desktopentry: %d example(s) parse clean\n", count);
  return 0;
}
