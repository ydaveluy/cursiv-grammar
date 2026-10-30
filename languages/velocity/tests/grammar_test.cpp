#include "audit_regressions.hpp"
#include "native_followup_cases.hpp"
// The grammar itself: every file of the upstream corpus parses clean, and a
// few field values are read back out of the tree -- a converter that
// silently misreads a value passes every structural check but not this one.

#include <algorithm>
#include <array>
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
#include "velocity/core/CoreServices.hpp"
#include "velocity/core/ast.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "velocity: %s\n", what.c_str());
  return 1;
}

std::string read_file(const std::filesystem::path& path) {
  std::ifstream file(path);
  if (!file) return {};
  return std::string((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
}

const velocity::ast::VariableRef* find_variable(const velocity::ast::Block& block,
                                                 std::string_view name) {
  for (const velocity::ast::Atom& atom : block.atoms) {
    const auto* const expr = std::get_if<velocity::ast::Expr*>(&atom);
    if (expr == nullptr || *expr == nullptr) continue;
    if ((*expr)->type != &cursiv::ast_type_tag<velocity::ast::VariableRef>)
      continue;
    const auto* const ref = static_cast<const velocity::ast::VariableRef*>(*expr);
    if (ref->name == name) return ref;
  }
  return nullptr;
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  const auto language = cursiv::create_language<
      velocity::VelocityCoreServices, velocity::VelocitySharedCoreServices,
      velocity::install_core_module>();
  if (!velocity_followup::run(language)) return 1;

  std::vector<std::filesystem::path> paths;
  for (const auto& entry :
       std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR))
    if (entry.is_regular_file()) paths.push_back(entry.path());
  std::sort(paths.begin(), paths.end());
  if (paths.empty()) return fail("the example directory is empty");

  // Kept alive for the whole run: every string_view in a parsed tree points
  // into its source text, and every AstNode* into the ParseResult's arena,
  // so both must outlive every read -- a ParseResult local to the loop body
  // would leave helloBlock/mathBlock dangling the moment it went out of
  // scope.
  std::vector<std::string> texts;
  texts.reserve(paths.size());
  std::vector<cursiv::ParseResult> results;
  results.reserve(paths.size());

  const velocity::ast::Block* helloBlock = nullptr;
  const velocity::ast::Block* mathBlock = nullptr;

  for (const std::filesystem::path& path : paths) {
    texts.push_back(read_file(path));
    const std::string& text = texts.back();
    if (text.empty()) return fail(path.string() + " is not readable");

    results.push_back(language.language.parser->parse(text));
    const cursiv::ParseResult& result = results.back();
    if (result.cst().recovered()) {
      for (const auto& diagnostic : result.diagnostics)
        std::fprintf(stderr, "velocity corpus: kind=%u element=%u bytes=%u..%u context=[%.*s]\n",
                     unsigned(diagnostic.kind), unsigned(diagnostic.element), unsigned(diagnostic.begin), unsigned(diagnostic.end),
                     int(std::min<std::size_t>(70, text.size() - std::min<std::size_t>(diagnostic.begin, text.size()))),
                     text.data() + std::min<std::size_t>(diagnostic.begin, text.size()));
      return fail(path.string() + " does not parse clean");
    }
    if (!result.diagnostics.empty())
      return fail(path.string() + " raised a diagnostic");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<velocity::ast::Block>)
      return fail(path.string() + " built no Block");

    const auto& block = *static_cast<const velocity::ast::Block*>(root);
    if (path.filename() == "hello_world.vm") helloBlock = &block;
    if (path.filename() == "math.vm") mathBlock = &block;
  }

  if (helloBlock == nullptr) return fail("hello_world.vm was not read");
  const velocity::ast::VariableRef* const foo = find_variable(*helloBlock, "foo");
  if (foo == nullptr) return fail("hello_world.vm did not read back $foo");
  if (foo->quiet) return fail("$foo misread as quiet");

  // hello_world.vm's `#set( $foo = "Velocity" )`: the assigned value should
  // read back as the unquoted string, not the raw quoted text.
  bool sawSetFoo = false;
  for (const velocity::ast::Atom& atom : helloBlock->atoms) {
    const auto* const directive = std::get_if<velocity::ast::Directive*>(&atom);
    if (directive == nullptr || *directive == nullptr) continue;
    if ((*directive)->type != &cursiv::ast_type_tag<velocity::ast::SetDirective>)
      continue;
    const auto& set = *static_cast<const velocity::ast::SetDirective*>(*directive);
    if (set.value == nullptr ||
        set.value->type != &cursiv::ast_type_tag<velocity::ast::StringLit>)
      continue;
    const auto& lit = *static_cast<const velocity::ast::StringLit*>(set.value);
    if (lit.value != "Velocity")
      return fail("#set's string value did not unquote to Velocity");
    sawSetFoo = true;
  }
  if (!sawSetFoo) return fail("hello_world.vm did not build a #set of a string");

  // math.vm's `#set( $foo = $bar lt 4 )`: the word-spelled operator reads
  // back with the same meaning as its symbol, `<`.
  if (mathBlock == nullptr) return fail("math.vm was not read");
  bool sawLt = false;
  for (const velocity::ast::Atom& atom : mathBlock->atoms) {
    const auto* const directive = std::get_if<velocity::ast::Directive*>(&atom);
    if (directive == nullptr || *directive == nullptr) continue;
    if ((*directive)->type != &cursiv::ast_type_tag<velocity::ast::SetDirective>)
      continue;
    const auto& set = *static_cast<const velocity::ast::SetDirective*>(*directive);
    if (set.value == nullptr ||
        set.value->type != &cursiv::ast_type_tag<velocity::ast::BinaryExpr>)
      continue;
    const auto& bin = *static_cast<const velocity::ast::BinaryExpr*>(set.value);
    if (bin.op == velocity::ast::Operator::lt) sawLt = true;
  }
  if (!sawLt) return fail("math.vm's word-spelled `lt` did not read back as lt");

  std::printf("velocity: %zu example files parse clean\n", paths.size());
  return 0;
}
