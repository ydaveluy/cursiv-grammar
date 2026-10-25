// The json conformance suite: every document under example/ and tests/ must
// parse without recovery and root a Doc.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "json/core/CoreServices.hpp"
#include "json/core/JsonString.hpp"
#include "json/core/ast.hpp"
#include "key_regressions.hpp"

namespace {

namespace fs = std::filesystem;

bool check(const fs::path &file) {
  std::ifstream stream(file);
  if (!stream) {
    std::fprintf(stderr, "json: %s: not readable\n", file.string().c_str());
    return false;
  }
  const std::string text((std::istreambuf_iterator<char>(stream)),
                         std::istreambuf_iterator<char>());

  const auto language = cursiv::create_language<json::JsonCoreServices,
                                                json::JsonSharedCoreServices,
                                                json::install_core_module>();

  const cursiv::ParseResult result = language.language.parser->parse(text);
  if (result.cst().recovered()) {
    std::fprintf(stderr, "json: %s: does not parse clean\n",
                 file.string().c_str());
    return false;
  }

  const cursiv::AstNode *const root = result.value;
  if (root == nullptr || root->type != &cursiv::ast_type_tag<json::ast::Doc>) {
    std::fprintf(stderr, "json: %s: did not build a Doc\n",
                 file.string().c_str());
    return false;
  }
  return true;
}

} // namespace

int main() {
  if (!json_key_regressions::run()) {
    std::fputs("json: escaped key equality regression\n", stderr);
    return 1;
  }
  if (json::decode_string(R"(\u0061)") != json::decode_string("a") ||
      json::decode_string(R"(\uD83D\uDE00)") != json::decode_string("😀"))
    return 1;

  {
    const auto audit_language =
        cursiv::create_language<json::JsonCoreServices,
                                json::JsonSharedCoreServices,
                                json::install_core_module>();
    const auto audit_result =
        audit_language.language.parser->parse("[1e-999,-1e-999,1e999]");
    if (!audit_result.accepted || audit_result.cst().recovered() ||
        !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
    const auto *doc = audit_result.value_as<json::ast::Doc>();
    auto *const *root = std::get_if<json::ast::Value *>(&doc->value);
    const auto *array =
        root ? cursiv::ast_cast<json::ast::Array>(*root) : nullptr;
    if (!array || array->items.size() != 3 ||
        std::get<double>(array->items[0]) != 0 ||
        std::get<double>(array->items[1]) != 0 ||
        std::get<double>(array->items[2]) <= 0)
      return 1;
    const auto duplicate = cursiv::test::validationHelper(
        audit_language.language, R"({"a":1,"\u0061":2})");
    const auto distinct = cursiv::test::validationHelper(
        audit_language.language, R"({"a":1,"b":2})");
    if (duplicate.diagnostics.size() != 2 || !distinct.diagnostics.empty())
      return 1;
  }

  bool ok = true;
  int count = 0;
  for (const char *const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR}) {
    for (const fs::directory_entry &entry : fs::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".json")
        continue;
      ++count;
      ok &= check(entry.path());
    }
  }

  if (count == 0) {
    std::fprintf(stderr, "json: no examples found under %s\n",
                 CURSIV_EXAMPLE_DIR);
    return 1;
  }
  if (!ok)
    return 1;

  std::printf("json: %d examples parse clean\n", count);
  return 0;
}
