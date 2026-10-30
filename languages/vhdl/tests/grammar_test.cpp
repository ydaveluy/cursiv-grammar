#include "native_followup.hpp"
#include "modern_cases.hpp"
#include "audit_regressions.hpp"
// The smoke test of the language: parse every document under example/ and
// check the tree is clean -- no repair, no diagnostic, the right root.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "vhdl/core/CoreServices.hpp"
#include "vhdl/core/ast.hpp"

namespace {

const char* const kExamples[] = {
    "add_pkg.vhd",       "generic_mux.vhd",
    "literals.vhd",      "misc.vhd",            "numeric_bit-body.vhd",
    "numeric_bit.vhd",   "numeric_std.vhd",     "signed.vhd",
    "standard.vhd",      "std_logic_1164_body.vhd", "std_logic_1164.vhd",
    "std_logic_textio.vhd", "test_generate.vhd", "test_pkg.vhd",
    "textio.vhd",        "unsigned.vhd",
};

// coverage.vhd is this port's own document, not the upstream corpus.
const char* const kTestFiles[] = {
    "coverage.vhd",
};

int fail(const std::string& file, const char* what) {
  std::fprintf(stderr, "vhdl: %s: %s\n", file.c_str(), what);
  return 1;
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  const auto language = cursiv::create_language<
      vhdl::VhdlCoreServices, vhdl::VhdlSharedCoreServices,
      vhdl::install_core_module>();
  if (!vhdl_audit::modern_cases(language)) return 1;
  if (!vhdl_native_followup::run(language)) return 1;

  auto checkSample = [&](const std::string& path) -> int {
    std::ifstream file(path);
    if (!file) return fail(path, "the sample document is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) return fail(path, "the sample does not parse");
    if (!result.diagnostics.empty())
      return fail(path, "the sample carries a diagnostic");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<vhdl::ast::DesignFile>)
      return fail(path, "the parse built no design file");
    return 0;
  };

  for (const char* const name : kExamples)
    if (const int rc = checkSample(std::string(CURSIV_EXAMPLE_DIR) + "/" + name); rc != 0)
      return rc;
  for (const char* const name : kTestFiles)
    if (const int rc = checkSample(std::string(CURSIV_TEST_DIR) + "/" + name); rc != 0)
      return rc;

  std::puts("vhdl: every sample parses");
  return 0;
}
