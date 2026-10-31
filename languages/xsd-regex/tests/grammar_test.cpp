// The smoke test of the language: parse every document in example/, and fail
// on any that recovers (needed a repair) or builds the wrong root type.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "xsdregex/core/CoreServices.hpp"
#include "xsdregex/core/ast.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "xsdregex: %s\n", what.c_str());
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      xsdregex::XsdRegexCoreServices, xsdregex::XsdRegexSharedCoreServices,
      xsdregex::install_core_module>();

  int seen = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
      if (entry.path().extension() != ".xsdregex") continue;
      ++seen;

      std::ifstream file(entry.path());
      if (!file) return fail(entry.path().string() + ": not readable");
      const std::string text((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

      const cursiv::ParseResult result =
          language.language.parser->parse(text);
      if (result.cst().recovered())
        return fail(entry.path().string() + ": does not parse clean");
      if (!result.diagnostics.empty())
        return fail(entry.path().string() + ": has diagnostics");

      const cursiv::AstNode* const root = result.value;
      if (root == nullptr ||
          root->type != &cursiv::ast_type_tag<xsdregex::ast::RegExp>)
        return fail(entry.path().string() + ": did not build a RegExp");
    }
  for (const std::string& text : {std::string("\xc3", 1), std::string("\x80", 1),
      std::string("\xc0\xaf", 2), std::string("\xed\xa0\x80", 3),
      std::string("[\xc3]", 3), std::string("\xf4\x90\x80\x80", 4)}) {
    const auto result = language.language.parser->parse(text);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return fail("malformed UTF-8 accepted as a literal character");
  }
  for (const auto text : {"é", "😀", "[😀-🙏]"}) {
    const auto result = language.language.parser->parse(text);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty())
      return fail("valid Unicode scalar rejected");
  }
  if (seen == 0) return fail("no example found");

  std::printf("xsdregex: %d examples parse\n", seen);
  return 0;
}
