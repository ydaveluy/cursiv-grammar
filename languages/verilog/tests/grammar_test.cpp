#include "modern_cases.hpp"
#include "audit_regressions.hpp"
// The smoke test of the language: every document in `example/` parses
// clean -- no repair, no diagnostic, and the right root type.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "verilog/core/CoreServices.hpp"
#include "verilog/core/ast.hpp"

namespace {

int fail(const char* what) {
  std::fprintf(stderr, "verilog: %s\n", what);
  return 1;
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  static constexpr const char* kSamples[] = {
      CURSIV_EXAMPLE_DIR "/axiluart.v",   CURSIV_EXAMPLE_DIR "/rxuart.v",
      CURSIV_EXAMPLE_DIR "/rxuartlite.v", CURSIV_EXAMPLE_DIR "/txuart.v",
      CURSIV_EXAMPLE_DIR "/txuartlite.v", CURSIV_EXAMPLE_DIR "/skidbuffer.v",
      CURSIV_EXAMPLE_DIR "/udp_lfsr.v",   CURSIV_EXAMPLE_DIR "/ufifo.v",
      CURSIV_EXAMPLE_DIR "/wbuart.v",

      CURSIV_TEST_DIR "/gates.v",  CURSIV_TEST_DIR "/udp.v",
      CURSIV_TEST_DIR "/specify.v", CURSIV_TEST_DIR "/config.v",
      CURSIV_TEST_DIR "/constructs.v", CURSIV_TEST_DIR "/ifdef.v",
  };

  const auto language = cursiv::create_language<
      verilog::VerilogCoreServices, verilog::VerilogSharedCoreServices,
      verilog::install_core_module>();
  if (!verilog_audit::modern_cases(language)) return 1;

  for (const char* const path : kSamples) {
    std::ifstream file(path);
    if (!file) return fail("a sample document is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      std::fprintf(stderr, "verilog: recovered input %s\n", path);
      for (const auto& diagnostic : result.diagnostics) std::fprintf(stderr, "%s\n", diagnostic.message.c_str());
      return fail("a sample does not parse clean");
    }
    if (!result.diagnostics.empty())
      return fail("a sample carries diagnostics");

    const cursiv::AstNode* const root = result.value;
    const bool isModuleRoot =
        root != nullptr &&
        root->type == &cursiv::ast_type_tag<verilog::ast::SourceText>;
    if (!isModuleRoot) return fail("the parse built no source text");
  }

  std::puts("verilog: the samples parse");
  return 0;
}
