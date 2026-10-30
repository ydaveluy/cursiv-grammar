#include "audit_regressions.hpp"
// The grammar's end to end check: every document under example/ parses
// clean, with no recovery and no diagnostics, and builds a Document root.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "thrift/core/CoreServices.hpp"
#include "thrift/core/ast.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "thrift: %s\n", what.c_str());
  return 1;
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  namespace fs = std::filesystem;

  const auto language = cursiv::create_language<
      thrift::ThriftCoreServices, thrift::ThriftSharedCoreServices,
      thrift::install_core_module>();

  for (const char *source : {"const i32 missing", "const i32 missing;", "const string missing ="}) {
    const auto r = language.language.parser->parse(source);
    if (r.accepted && !r.cst().recovered() && r.diagnostics.empty())
      return fail("a constant requires an explicit value");
  }
  for (const char *source : {"const i32 zero = 0", "const string empty = ''"}) {
    const auto r = language.language.parser->parse(source);
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty())
      return fail("explicit zero or empty constant rejected");
  }
  int count = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR}) {
    for (const fs::directory_entry& entry : fs::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".thrift")
        continue;
      const fs::path path = entry.path();

      std::ifstream file(path, std::ios::binary);
      if (!file) return fail(path.filename().string() + " is not readable");
      const std::string text((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());

      const cursiv::ParseResult result = language.language.parser->parse(text);
      if (result.cst().recovered())
        return fail(path.filename().string() + " does not parse clean");
      // BrokenConstants.thrift declares an i64 literal past INT64_MAX: the
      // conversion diagnostic is the point of the file, so it is required,
      // not merely tolerated.
      const bool wants_diagnostic = path.filename() == "BrokenConstants.thrift";
      if (result.diagnostics.empty() == wants_diagnostic)
        return fail(path.filename().string() +
                    (wants_diagnostic ? " left no diagnostic" : " left diagnostics"));

      const cursiv::AstNode* const root = result.value;
      if (root == nullptr ||
          root->type != &cursiv::ast_type_tag<thrift::ast::Document>)
        return fail(path.filename().string() + " built no document");
      ++count;
    }
  }
  if (count == 0) return fail("no example documents found");

  std::printf("thrift: %d examples parse\n", count);
  return 0;
}
