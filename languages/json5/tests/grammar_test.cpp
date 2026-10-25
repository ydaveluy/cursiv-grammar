// The smoke test of the language: every document in example/ parses clean (no
// recovery, no diagnostic -- the same verdict the cli prints "ok" for) and
// builds a Doc root.

#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/test/Check.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "json5/core/CoreServices.hpp"
#include "json5/core/ast.hpp"
#include "whitespace_regressions.hpp"

namespace {

int fail(const std::string &path, const char *what) {
  std::fprintf(stderr, "json5: %s: %s\n", path.c_str(), what);
  return 1;
}

bool decoded_values(const cursiv::Parser &parser) {
  const std::string source = R"json5({'\u0061':'A\n\x42\uD83D\uDE00\0Z\v',
  \u0062:'x\qz', Ⅰ:1, ᛮ:2, Ⅻ:3})json5";
  const auto result = parser.parse(source);
  if (!result.accepted || result.cst().recovered() ||
      !result.diagnostics.empty())
    return false;
  const auto *doc = result.value_as<json5::ast::Doc>();
  const auto *held = doc && doc->value
                         ? std::get_if<json5::ast::Value *>(&*doc->value)
                         : nullptr;
  const auto *object =
      held ? cursiv::ast_cast<json5::ast::Object>(*held) : nullptr;
  if (!object || object->members.size() != 5 ||
      object->members[0]->name != "a" || object->members[1]->name != "b" ||
      object->members[2]->name != "Ⅰ" || object->members[3]->name != "ᛮ" ||
      object->members[4]->name != "Ⅻ")
    return false;
  const auto *a = std::get_if<std::string_view>(&object->members[0]->value);
  const auto *b = std::get_if<std::string_view>(&object->members[1]->value);
  if (!a || *a != std::string_view("A\nB😀\0Z\v", 10) || !b || *b != "xqz")
    return false;
  for (const std::string& newline :
       {std::string("\n"), std::string("\r\n"), std::string("\r"),
        std::string(" "), std::string(" ")}) {
    const std::string continued = "'one\\" + newline + "two'";
    const auto parsed = parser.parse(continued);
    const auto *root = parsed.value_as<json5::ast::Doc>();
    const auto *text = root && root->value
                           ? std::get_if<std::string_view>(&*root->value)
                           : nullptr;
    if (!parsed.accepted || parsed.cst().recovered() ||
        !parsed.diagnostics.empty() || !text || *text != "onetwo")
      return false;
  }
  return true;
}

} // namespace

int main() {
  {
    const auto audit_language =
        cursiv::create_language<json5::Json5CoreServices,
                                json5::Json5SharedCoreServices,
                                json5::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse(
        "[1e, 1e+, 0xFFFFFFFFFFFFFFFFFFFFFF]");
    if (!audit_result.accepted || audit_result.cst().recovered() ||
        !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
    bool incomplete = false, wide = false;
    for (const auto *node : audit_result.nodes)
      if (const auto *n = cursiv::ast_cast<json5::ast::Number>(node)) {
        incomplete = incomplete || n->text == "1e+";
        wide = wide || n->text == "0xFFFFFFFFFFFFFFFFFFFFFF";
      }
    if (!incomplete || !wide)
      return 1;
    const auto invalid =
        cursiv::test::validationHelper(audit_language.language, "[1e,1e+]");
    if (invalid.diagnostics.size() != 2)
      return 1;
  }

  const auto language = cursiv::create_language<json5::Json5CoreServices,
                                                json5::Json5SharedCoreServices,
                                                json5::install_core_module>();
  if (!decoded_values(*language.language.parser))
    return 1;
  if (!json5_whitespace_regressions::run(*language.language.parser))
    return 1;

  std::vector<std::filesystem::path> files;
  for (const char *const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto &entry : std::filesystem::directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".json5")
        files.push_back(entry.path());

  if (files.empty())
    return fail(CURSIV_EXAMPLE_DIR, "no examples found");

  for (const std::filesystem::path &path : files) {
    const std::string text = cursiv::test::read_file(path.string());
    const cursiv::ParseResult result = language.language.parser->parse(text);
    // `recovered()` is set when the parser had to repair the input to keep
    // going: a clean document leaves it false.
    if (result.cst().recovered())
      return fail(path.string(), "the document does not parse");
    // A conversion error leaves `recovered()` untouched (the tree parsed --
    // only a value could not be made of it), so the cli's own "clean"
    // verdict is checked too: a diagnostic here is a document that does not
    // belong in example/.
    if (!result.diagnostics.empty())
      return fail(path.string(), "the document carries a diagnostic");

    const cursiv::AstNode *const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<json5::ast::Doc>)
      return fail(path.string(), "the parse built no Doc");
  }

  std::printf("json5: %zu examples parse\n", files.size());
  return 0;
}
