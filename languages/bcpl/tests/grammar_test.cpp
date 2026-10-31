// The grammar end to end: every document under example/ (the upstream
// corpus) and tests/ (constructs the corpus never reaches) parses clean
// (recovered() false, no diagnostics, the right root type), and a few real
// field values come back out of two documents' ASTs.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "bcpl/core/ast.hpp"
#include "bcpl/core/CoreServices.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "section_tags.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const std::string& what) {
  std::fprintf(stderr, "bcpl: %s\n", what.c_str());
  return 1;
}

const bcpl::BcplCoreServices& services() {
  static const auto language = cursiv::create_language<
      bcpl::BcplCoreServices, bcpl::BcplSharedCoreServices,
      bcpl::install_core_module>();
  return language.language;
}

const cursiv::Parser& engine() { return *services().parser; }

std::string read_file(const fs::path& path) {
  std::ifstream file(path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

const bcpl::ast::FunctionDef* find_function(const bcpl::ast::Program& program,
                                            std::string_view name) {
  for (const bcpl::ast::Decl* const decl : program.declarations) {
    if (decl->type != &cursiv::ast_type_tag<bcpl::ast::LetDecl>) continue;
    const auto& let = *static_cast<const bcpl::ast::LetDecl*>(decl);
    for (const bcpl::ast::Definition* const def : let.definitions) {
      if (def->type != &cursiv::ast_type_tag<bcpl::ast::FunctionDef>) continue;
      const auto& fn = *static_cast<const bcpl::ast::FunctionDef*>(def);
      if (fn.name != nullptr && fn.name->name == name) return &fn;
    }
  }
  return nullptr;
}

}  // namespace

int main() {
  if (!bcpl_sections::run(services())) return 1;
  // Richards, BCPL Cintsys/Cintpos manual, chapter 2: radix separators,
  // floating constants, nested comments, and escaped string delimiters.
  for (const char* literal : {"1_234_456", "#B_1011_1100", "#o377",
                              "#x_DEADC0DE", "1234.0", "12.", ".34",
                              "1.234_456e-5", "10e6"}) {
    const std::string source = std::string("LET native() = ") + literal;
    const auto r = engine().parse(source);
    const auto* program = cursiv::ast_cast<bcpl::ast::Program>(r.value);
    const auto* function = program != nullptr
                               ? find_function(*program, "native") : nullptr;
    const auto* value = function != nullptr
                            ? cursiv::ast_cast<bcpl::ast::NumberLit>(function->body)
                            : nullptr;
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        value == nullptr || value->text != literal)
      return fail(source + ": native numeric spelling lost");
  }
  for (const char* source : {
           "/* outer /* nested */ outer */ LET native() = 1",
           R"bcpl(LET native() = "He said *"hi*"**")bcpl",
           R"bcpl(LET native() = '*x41')bcpl",
           R"bcpl(LET native() = '*377')bcpl",
           R"bcpl(LET native() = '*#C13F')bcpl",
           "LET native(a) = a #/ 2.0",
           "LET native() BE { x -:= 1; x #-:= 2.0 }",
           "LET native(x) = MATCH(x) : 1..3 -> 1 : ? -> 0 ."}) {
    const auto r = engine().parse(source);
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        cursiv::ast_cast<bcpl::ast::Program>(r.value) == nullptr)
      return fail(std::string(source) + ": native lexical form refused");
  }
  {
    const auto r = engine().parse("LET FLT native(FLT x) = x #-> 1.0, 2.0");
    const auto* program = cursiv::ast_cast<bcpl::ast::Program>(r.value);
    const auto* function = program != nullptr
                               ? find_function(*program, "native") : nullptr;
    const auto* conditional = function != nullptr
        ? cursiv::ast_cast<bcpl::ast::CondExpr>(function->body) : nullptr;
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        function == nullptr || !function->name->floating ||
        function->params.size() != 1 || !function->params[0]->floating ||
        conditional == nullptr || conditional->op != "#->")
      return fail("FLT tags or floating conditional operator lost");
  }
  {
    const char* source = "LET native(o) = 1#(o,2)";
    const auto r = engine().parse(source);
    const auto* program = cursiv::ast_cast<bcpl::ast::Program>(r.value);
    const auto* function = program != nullptr
                               ? find_function(*program, "native") : nullptr;
    const auto* call = function != nullptr
        ? cursiv::ast_cast<bcpl::ast::CallExpr>(function->body) : nullptr;
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        call == nullptr || !call->method || call->args.size() != 2) {
      std::fprintf(stderr,
                   "bcpl method AST: source=%s accepted=%d recovered=%d "
                   "diagnostics=%zu function=%d call=%d method=%d args=%zu "
                   "bodyNumber=%d bodyVector=%d nodes=%zu\n",
                   source, r.accepted, r.cst().recovered(),
                   r.diagnostics.size(), function != nullptr, call != nullptr,
                   call != nullptr && call->method,
                   call != nullptr ? call->args.size() : 0,
                   function != nullptr &&
                       cursiv::ast_cast<bcpl::ast::NumberLit>(function->body),
                   function != nullptr &&
                       cursiv::ast_cast<bcpl::ast::VecAllocExpr>(function->body),
                   r.nodes.size());
      for (const auto& diagnostic : r.diagnostics)
        std::fprintf(stderr, "bcpl method diagnostic: kind=%d begin=%zu end=%zu\n",
                     static_cast<int>(diagnostic.kind),
                     static_cast<std::size_t>(diagnostic.begin),
                     static_cast<std::size_t>(diagnostic.end));
      return fail("method call marker or arguments lost");
    }
  }
  {
    // The legacy cg8086 corpus used another `/*` to label an outer
    // closer. Under native nested-comment rules that leaves it unclosed.
    const auto r = engine().parse("/*<8086\nLET x = 1\n/*8086>*/");
    if (r.accepted && !r.cst().recovered() && r.diagnostics.empty())
      return fail("an unfinished nested comment was accepted");
  }

  std::vector<fs::path> docs;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const fs::directory_entry& entry :
        fs::recursive_directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".bcpl")
        docs.push_back(entry.path());
  if (docs.empty()) return fail("no example documents found");

  fs::path fact_doc;
  fs::path match_doc;
  for (const fs::path& doc : docs) {
    const std::string text = read_file(doc);
    const cursiv::ParseResult result = engine().parse(text);
    if (!result.accepted)
      return fail(doc.string() + ": the parse was refused");
    if (result.cst().recovered()) {
      for (const auto& diagnostic : result.diagnostics)
        std::fprintf(stderr, "bcpl diagnostic %u..%u (kind %u)\n",
                     diagnostic.begin, diagnostic.end, unsigned(diagnostic.kind));
      return fail(doc.string() + ": the parse needed repair");
    }
    if (!result.diagnostics.empty())
      return fail(doc.string() + ": the parse left diagnostics");
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<bcpl::ast::Program>)
      return fail(doc.string() + ": the parse built no program");
    if (doc.filename() == "fact.bcpl") fact_doc = doc;
    if (doc.filename() == "match.bcpl") match_doc = doc;
  }
  if (fact_doc.empty()) return fail("fact.bcpl was not found among the examples");
  if (match_doc.empty()) return fail("match.bcpl was not found under tests/");

  // Read a few real field values out of fact.bcpl: a converter or a
  // structural mistake would still leave `recovered()` false and the root
  // type right, so only reading the fields back catches it.
  {
    const std::string text = read_file(fact_doc);
    const cursiv::ParseResult result = engine().parse(text);
    const auto& program = *static_cast<const bcpl::ast::Program*>(result.value);
    if (program.directives.size() != 1 ||
        program.directives[0]->kw != "GET")
      return fail("fact.bcpl: wrong directive");
    const bcpl::ast::FunctionDef* const factorial =
        find_function(program, "factorial");
    if (factorial == nullptr)
      return fail("fact.bcpl: no `factorial` definition");
    const auto& fn = *factorial;
    if (fn.params.size() != 1 || fn.params[0]->name != "n")
      return fail("fact.bcpl: `factorial` has the wrong parameters");
    if (fn.body->type != &cursiv::ast_type_tag<bcpl::ast::CondExpr>)
      return fail("fact.bcpl: `factorial`'s body is not a conditional");
  }

  // ...and from match.bcpl, that a pattern clause and its result survive.
  {
    const std::string text = read_file(match_doc);
    const cursiv::ParseResult result = engine().parse(text);
    const auto& program = *static_cast<const bcpl::ast::Program*>(result.value);
    const bcpl::ast::FunctionDef* const classify =
        find_function(program, "classify");
    if (classify == nullptr)
      return fail("match.bcpl: no `classify` definition");
    const auto& fn = *classify;
    if (fn.body->type != &cursiv::ast_type_tag<bcpl::ast::ValofExpr>)
      return fail("match.bcpl: `classify`'s body is not VALOF");
  }

  std::printf("bcpl: %zu documents parse clean\n", docs.size());
  return 0;
}
