// The smoke test of the language: build the container the way a workspace
// does, parse every document under example/, and check the tree.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "telephone/core/CoreServices.hpp"
#include "telephone/core/ast.hpp"

namespace {

int fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "telephone: %s: %s\n", file.string().c_str(), what);
  return 1;
}

}  // namespace

int main() {
  // The whole language in one call: the defaults, then `install_core_module`.
  const auto language = cursiv::create_language<
      telephone::TelephoneCoreServices, telephone::TelephoneSharedCoreServices,
      telephone::install_core_module>();

  int total = 0;
  int failures = 0;
  // The author's accepted groups have no domestic NANP NXX constraint.
  for (const char* source : {"011 123 045 6789", "010 123 045 6789",
                             "+1+0111230456789", "+1 0101230456789"}) {
    cursiv::Document document(cursiv::TextDocument::create(
        "memory:telephone", "telephone", 0, source));
    document.language = &language.language;
    document.parseResult = language.language.parser->parse(source);
    const auto& result = document.parseResult;
    const auto* number = result.value_as<telephone::ast::Number>();
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !number || !number->variation ||
        number->variation->areaCode != "123" ||
        number->variation->exchange != "045" ||
        number->variation->subscriber != "6789") {
      failures += fail(source, "teaching grammar groups were lost");
      continue;
    }
    const std::string_view spelling(source);
    if (number->hasCountryCode != spelling.starts_with("+1") ||
        number->hasPlus != spelling.starts_with("+1+") || number->parent ||
        number->variation->parent != number || result.nodes.size() != 2 ||
        result.cst().text() != spelling)
      failures += fail(source, "prefix flags or ownership were lost");
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(document, diagnostics);
    if (!diagnostics.empty())
      failures += fail(source, "unjustified numbering-plan diagnostic");
  }
  for (const char* source : {"4038675309", "011\t1230456789",
                             "011123045678", "01012304567890"}) {
    const auto result = language.language.parser->parse(source);
    if (result.accepted && !result.cst().recovered() &&
        result.diagnostics.empty())
      failures += fail(source, "source outside teaching grammar parsed cleanly");
  }
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
    // `recovered()` is set when the parser had to repair the input to keep
    // going: a clean document leaves it false.
    if (result.cst().recovered()) {
      failures += fail(file, "the document does not parse clean");
      continue;
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<telephone::ast::Number>)
      failures += fail(file, "the parse built no Number");
  }

  if (total == 0) return fail(CURSIV_EXAMPLE_DIR, "no example documents");
  if (failures > 0) return 1;

  std::printf("telephone: %d example(s) parse\n", total);
  return 0;
}
