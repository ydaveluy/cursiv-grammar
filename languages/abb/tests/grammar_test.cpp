// The grammar's end to end check: every document under example/ parses
// clean, with no recovery and no diagnostics, and builds a Module root.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory_resource>
#include <string>

#include "abb/core/AbbGrammar.hpp"
#include "abb/core/CoreServices.hpp"
#include "abb/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"

#include "native_regressions.hpp"

namespace {

int fail(const std::string &what) {
  std::fprintf(stderr, "abb: %s\n", what.c_str());
  return 1;
}

} // namespace

int main() {
  namespace fs = std::filesystem;

  const auto language =
      cursiv::create_language<abb::AbbCoreServices, abb::AbbSharedCoreServices,
                              abb::install_core_module>();

  // The converter must also reject incomplete recovery fragments safely.
  {
    std::pmr::monotonic_buffer_resource memory;
    for (std::string_view text : {"", "\"", "plain", "\"unterminated",
                                  "\"\\\"", "\"\\4\"", "\"\\GG\"",
                                  "\"bad\"quote\""})
      if (abb::string_value(text, &memory))
        return fail("incomplete string conversion unexpectedly succeeded");
    for (std::string_view text : {"", "'", "'x", "x'"})
      if (abb::unquote(text))
        return fail("incomplete character conversion unexpectedly succeeded");
    const auto empty = abb::string_value("\"\"", &memory);
    if (!empty || !empty.value->empty())
      return fail("empty string conversion failed");
  }

  // ABB RAPID kernel, RobotWare 7, section 2.7: quoted delimiters are
  // doubled and \HH codes denote characters, including non-ASCII values.
  {
    const auto result = language.language.parser->parse(
        R"RAPID(MODULE Strings
VAR string quote := "Contains a "" character";
VAR string slash := "Contains a \\ character";
VAR string codes := "\00\07\41\E9";
ENDMODULE)RAPID");
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty())
      return fail("native RAPID strings did not parse cleanly");
    const auto *module = result.value_as<abb::ast::Module>();
    const std::string expected[] = {"Contains a \" character",
                                    "Contains a \\ character",
                                    std::string("\0\aA\xc3\xa9", 5)};
    if (!module || module->declarations.size() != 3)
      return fail("native RAPID strings lost declarations");
    for (std::size_t i = 0; i < 3; ++i) {
      const auto &value = module->declarations[i]->value;
      const auto *text = value ? std::get_if<std::string_view>(&*value) : nullptr;
      if (!text || *text != expected[i])
        return fail("native RAPID string has the wrong decoded value");
    }
    for (const char *source : {
             R"RAPID(MODULE M VAR string s := "\n"; ENDMODULE)RAPID",
             R"RAPID(MODULE M VAR string s := "\4"; ENDMODULE)RAPID"}) {
      const auto bad = language.language.parser->parse(source);
      if (bad.accepted && !bad.cst().recovered() && bad.diagnostics.empty())
        return fail("non-native RAPID escape parsed cleanly");
    }
  }

  // Regression: inspect the semantic payload, not only clean acceptance.
  {
    const auto result = language.language.parser->parse(
        "MODULE T\nVAR num x := +12;\nVAR num y := -'H1F';\nVAR num z := "
        "999999999999999999999999;\nVAR tooldata a := [-1,+2];\n"
        "PROC Q()\nCall -3;\nENDPROC\nENDMODULE");
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty())
      return fail("integer regression did not parse");
    const auto *module = result.value_as<abb::ast::Module>();
    if (!module || module->declarations.size() != 4 ||
        module->procedures.size() != 1)
      return fail("integer regression lost declarations or procedure");
    const std::string_view expected[] = {"+12", "-'H1F'",
                                         "999999999999999999999999"};
    for (std::size_t i = 0; i < 3; ++i) {
      const auto *decl = module->declarations[i];
      const auto *integer =
          decl->value ? std::get_if<abb::ast::Integer *>(&*decl->value)
                      : nullptr;
      if (decl->type != "num" || !integer || !*integer ||
          (*integer)->text != expected[i])
        return fail("integer declaration lost its exact value");
    }
    const auto &arrayValue = module->declarations[3]->value;
    const auto *array =
        arrayValue ? std::get_if<abb::ast::Array *>(&*arrayValue) : nullptr;
    if (!array || !*array || (*array)->items.size() != 2)
      return fail("integer array was lost");
    for (std::size_t i = 0; i < 2; ++i) {
      const auto *value = std::get_if<abb::ast::Integer *>(&(*array)->items[i]);
      if (!value || !*value || (*value)->text != (i == 0 ? "-1" : "+2"))
        return fail("integer array item lost its sign");
    }
    const auto &calls = module->procedures[0]->statements;
    const auto *call = calls.size() == 1
                           ? cursiv::ast_cast<abb::ast::Call>(calls[0])
                           : nullptr;
    if (!call || call->arguments.size() != 1)
      return fail("integer call argument was lost");
    const auto *argument =
        std::get_if<abb::ast::Integer *>(&call->arguments[0]);
    if (!argument || !*argument || (*argument)->text != "-3")
      return fail("integer call argument lost its value");
  }

  if (!abb_tests::nativeRegressions(language.language))
    return fail("native semantic regressions failed");

  int count = 0;
  for (const char *const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const fs::directory_entry &entry : fs::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".sys")
        continue;
      const fs::path path = entry.path();

      std::ifstream file(path, std::ios::binary);
      if (!file)
        return fail(path.filename().string() + " is not readable");
      const std::string text((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

      const cursiv::ParseResult result = language.language.parser->parse(text);
      if (result.cst().recovered())
        return fail(path.filename().string() + " does not parse clean");
      if (!result.diagnostics.empty())
        return fail(path.filename().string() + " left diagnostics");

      const cursiv::AstNode *const root = result.value;
      if (root == nullptr ||
          root->type != &cursiv::ast_type_tag<abb::ast::Module>)
        return fail(path.filename().string() + " built no module");
      ++count;
    }
  if (count == 0)
    return fail("no example documents found");

  std::printf("abb: %d examples parse\n", count);
  return 0;
}
