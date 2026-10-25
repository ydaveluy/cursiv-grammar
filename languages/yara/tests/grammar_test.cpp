#include "audit_regressions.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "modern_cases.hpp"
#include "yara/core/CoreServices.hpp"
#include "yara/core/ast.hpp"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <variant>
#include <vector>

namespace {
int fail(const char *message) {
  std::fprintf(stderr, "yara: %s\n", message);
  return 1;
}
std::string read(const char *path) {
  std::ifstream file(path);
  return {std::istreambuf_iterator<char>(file), {}};
}
auto rules(const yara::ast::Document *doc) {
  std::vector<const yara::ast::RuleDecl *> result;
  if (doc)
    for (const auto &declaration : doc->declarations)
      if (auto rule = std::get_if<yara::ast::RuleDecl *>(&declaration))
        result.push_back(*rule);
  return result;
}
} // namespace
int main() {
  if (!audit_regressions::run())
    return 1;
  auto language = cursiv::create_language<yara::YaraCoreServices,
                                          yara::YaraSharedCoreServices,
                                          yara::install_core_module>();
  if (!yara_audit::modern_cases(language))
    return 1;
  auto source = read(CURSIV_SAMPLE);
  auto result = language.language.parser->parse(source);
  if (!result.accepted || result.cst().recovered() ||
      !result.diagnostics.empty())
    return fail("sample does not parse cleanly");
  auto doc = cursiv::ast_cast<yara::ast::Document>(result.value);
  auto declarations = rules(doc);
  unsigned imports = 0, includes = 0;
  if (doc)
    for (const auto &entry : doc->declarations) {
      imports += std::holds_alternative<yara::ast::ImportDecl *>(entry);
      includes += std::holds_alternative<yara::ast::IncludeDecl *>(entry);
    }
  if (!doc || declarations.size() != 15 || imports != 2 || includes != 6)
    return fail("sample declaration counts");
  const yara::ast::RuleDecl *dummy = nullptr;
  const yara::ast::RuleDecl *rr = nullptr;
  for (auto rule : declarations) {
    if (rule->name == "_dummy3")
      dummy = rule;
    if (rule->name == "rr2")
      rr = rule;
  }
  if (!dummy || !dummy->isPrivate || dummy->tags.size() != 2 ||
      dummy->tags[0] != "Tag1" || dummy->tags[1] != "Tag2" ||
      dummy->meta.size() != 5)
    return fail("sample tags and metadata");
  auto one = std::get_if<std::string_view>(&dummy->meta[0]->value);
  auto hex = std::get_if<yara::ast::IntegerLiteral *>(&dummy->meta[2]->value);
  auto boolean = std::get_if<bool>(&dummy->meta[4]->value);
  if (!one || *one != "a" || !hex || !*hex ||
      yara::integerValue((*hex)->text) != 0 || !boolean || *boolean)
    return fail("sample metadata values and types");
  auto of = rr ? cursiv::ast_cast<yara::ast::OfExpr>(rr->condition) : nullptr;
  if (!of || of->set.size() != 1 || of->set[0] != "rr*")
    return fail("sample rule set");
  auto coverage = read(CURSIV_TEST_DIR "/coverage.yar");
  auto covered = language.language.parser->parse(coverage);
  if (!covered.accepted || covered.cst().recovered() ||
      !covered.diagnostics.empty())
    return fail("coverage does not parse cleanly");
  auto coverageRules =
      rules(cursiv::ast_cast<yara::ast::Document>(covered.value));
  if (coverageRules.size() != 1 || coverageRules[0]->strings.size() != 2)
    return fail("coverage string count");
  auto regex =
      std::get_if<yara::ast::RegexLit *>(&coverageRules[0]->strings[0]->value);
  if (!regex || !*regex || (*regex)->text != "/abc[0-9]+/i")
    return fail("coverage regex spelling");
  std::puts("yara: sample and coverage parse with their captured values");
  return 0;
}
