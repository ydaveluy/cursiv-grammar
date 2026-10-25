#include "audit_regressions.hpp"
// The smoke test of the language: build the container the way a workspace
// does, read every file of example/, and check each tree.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "propcalc/core/CoreServices.hpp"
#include "propcalc/core/ast.hpp"

namespace {

namespace fs = std::filesystem;

bool checkFile(const cursiv::CoreLanguage<propcalc::PropcalcCoreServices,
                                          propcalc::PropcalcSharedCoreServices>&
                   language,
               const fs::path& path) {
  std::ifstream file(path);
  if (!file) {
    std::fprintf(stderr, "propcalc: %s is not readable\n", path.c_str());
    return false;
  }
  const std::string text((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

  const cursiv::ParseResult result = language.language.parser->parse(text);
  if (result.cst().recovered()) {
    std::fprintf(stderr, "propcalc: %s does not parse clean\n", path.c_str());
    return false;
  }

  const cursiv::AstNode* const root = result.value;
  if (root == nullptr ||
      root->type != &cursiv::ast_type_tag<propcalc::ast::Proposition>) {
    std::fprintf(stderr, "propcalc: %s built no proposition\n", path.c_str());
    return false;
  }
  return true;
}

bool isEmptyVariable(const propcalc::ast::Expr* const expr) {
  return expr != nullptr &&
         expr->type == &cursiv::ast_type_tag<propcalc::ast::VariableRef> &&
         static_cast<const propcalc::ast::VariableRef*>(expr)->name.empty();
}

// An operand left out is an empty variable, never a null.
template <class Language>
bool checkEmptyOperands(const Language& language) {
  using propcalc::ast::Proposition;
  const cursiv::Parser& parser = *language.language.parser;

  const cursiv::ParseResult bare = parser.parse("|-");
  const Proposition* const sequent = bare.value_as<Proposition>();
  if (!bare.accepted || bare.cst().recovered() || sequent == nullptr || sequent->premises == nullptr ||
      sequent->conclusion == nullptr ||
      !isEmptyVariable(sequent->premises) ||
      !isEmptyVariable(sequent->conclusion)) {
    std::fprintf(stderr, "propcalc: `|-` lost its empty operands\n");
    return false;
  }

  const cursiv::ParseResult negated = parser.parse("! |- p");
  const Proposition* const negation = negated.value_as<Proposition>();
  const auto* const not_ =
      negation == nullptr ? nullptr : negation->premises;
  if (!negated.accepted || negated.cst().recovered() || not_ == nullptr ||
      not_->type != &cursiv::ast_type_tag<propcalc::ast::Negation> ||
      !isEmptyVariable(
          static_cast<const propcalc::ast::Negation*>(not_)->operand)) {
    std::fprintf(stderr, "propcalc: `!` lost its empty operand\n");
    return false;
  }

  const cursiv::ParseResult chain = parser.parse("p ^ |- q");
  const Proposition* const open = chain.value_as<Proposition>();
  const auto* binary = open ? cursiv::ast_cast<propcalc::ast::Binary>(open->premises) : nullptr;
  if (!chain.accepted || chain.cst().recovered() || !binary || binary->op != propcalc::ast::Connective::conjunction ||
      !isEmptyVariable(binary->right)) {
    std::fprintf(stderr, "propcalc: `p ^` lost its empty operand\n");
    return false;
  }
  return true;
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  const auto language = cursiv::create_language<
      propcalc::PropcalcCoreServices, propcalc::PropcalcSharedCoreServices,
      propcalc::install_core_module>();

  std::vector<fs::path> files;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const fs::directory_entry& entry : fs::directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".propcalc")
        files.push_back(entry.path());
  if (files.empty()) {
    std::fprintf(stderr, "propcalc: no example found in %s\n",
                 CURSIV_EXAMPLE_DIR);
    return 1;
  }

  bool ok = checkEmptyOperands(language);
  for (const fs::path& path : files)
    ok = checkFile(language, path) && ok;
  if (!ok) return 1;

  std::printf("propcalc: %zu examples parse\n", files.size());
  return 0;
}
