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
#include "gtin/core/CoreServices.hpp"
#include "gtin/core/ast.hpp"

namespace {

int fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "gtin: %s: %s\n", file.string().c_str(), what);
  return 1;
}

}  // namespace

int main() {
  {
    const auto audit_language =
        cursiv::create_language<gtin::GtinCoreServices,
                                gtin::GtinSharedCoreServices,
                                gtin::install_core_module>();
    const auto audit_result =
        audit_language.language.parser->parse("9991234567890");
    if (!audit_result.accepted || audit_result.cst().recovered() ||
        !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
    const auto* code = audit_result.value_as<gtin::ast::Gtin>();
    if (!code || code->text != "9991234567890") return 1;
    for (const auto text :
         {"4006381333931", "123456", "01234565", "1234565", "22", "12345",
          "0-42100-00526-4", "4006381333931+22", "042100005264 22",
          "400638133393112345"}) {
      const auto valid =
          cursiv::test::validationHelper(audit_language.language, text);
      const auto parsed = audit_language.language.parser->parse(text);
      const auto* value = parsed.value_as<gtin::ast::Gtin>();
      if (!valid.diagnostics.empty() || !parsed.accepted ||
          parsed.cst().recovered() || !parsed.diagnostics.empty() || !value ||
          value->text != text || value->parent != nullptr ||
          parsed.nodes.size() != 1)
        return 1;
    }
    for (const auto text : {"", "123", "4006381333932", "01234566", "120003",
                            "123004", "123405", "12a34", "4006381333931+123"}) {
      const auto parsed = audit_language.language.parser->parse(text);
      const auto invalid =
          cursiv::test::validationHelper(audit_language.language, text);
      if (!parsed.accepted || parsed.cst().recovered() ||
          !parsed.diagnostics.empty() || invalid.diagnostics.empty())
        return 1;
    }
  }

  const auto language = cursiv::create_language<gtin::GtinCoreServices,
                                                gtin::GtinSharedCoreServices,
                                                gtin::install_core_module>();

  int total = 0;
  int failures = 0;
  for (const std::filesystem::directory_entry& entry :
       std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR)) {
    if (!entry.is_regular_file()) continue;
    const std::filesystem::path& file = entry.path();
    ++total;

    std::ifstream in(file);
    if (!in) {
      failures += fail(file, "the document is not readable");
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(in)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      failures += fail(file, "the document does not parse clean");
      continue;
    }
    if (!result.diagnostics.empty()) {
      failures += fail(file, "the document left diagnostics");
      continue;
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<gtin::ast::Gtin> ||
        result.value_as<gtin::ast::Gtin>()->text != text)
      failures += fail(file, "the parse built no Gtin");
  }

  if (total == 0) return fail(CURSIV_EXAMPLE_DIR, "no example documents");
  if (failures > 0) return 1;

  std::printf("gtin: %d example(s) parse\n", total);
  return 0;
}
