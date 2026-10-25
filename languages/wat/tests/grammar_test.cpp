#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <variant>

#include "audit_regressions.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "gc_cases.hpp"
#include "modern_cases.hpp"
#include "wat/core/CoreServices.hpp"
#include "wat/core/ast.hpp"

namespace {

namespace ast = wat::ast;

int fail(const std::string& what) {
  std::fprintf(stderr, "wat: %s\n", what.c_str());
  return 1;
}

std::string read_file(const std::string& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

template <class T>
const T* as(const cursiv::AstNode* n) {
  return (n != nullptr && n->type == &cursiv::ast_type_tag<T>)
             ? static_cast<const T*>(n)
             : nullptr;
}

ast::Module* module_of(const ast::Cmd& c) {
  const ast::Module* const* m = std::get_if<ast::Module*>(&c);
  return m != nullptr ? const_cast<ast::Module*>(*m) : nullptr;
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  const auto language =
      cursiv::create_language<wat::WatCoreServices, wat::WatSharedCoreServices,
                              wat::install_core_module>();
  if (!wat_gc_audit::run(language)) return 1;

  if (!wat_audit::modern_cases(language)) return 1;
  const std::string corpusPath =
      std::string(CURSIV_EXAMPLE_DIR) + "/shared1.wat";
  const std::string corpusText = read_file(corpusPath);
  if (corpusText.empty()) return fail("shared1.wat: empty or unreadable");

  const cursiv::ParseResult corpus =
      language.language.parser->parse(corpusText);
  if (corpus.cst().recovered())
    return fail("shared1.wat: does not parse clean");
  if (!corpus.diagnostics.empty()) return fail("shared1.wat: has diagnostics");

  const ast::Script* const script = as<ast::Script>(corpus.value);
  if (script == nullptr || script->cmds.size() != 1)
    return fail("shared1.wat: expected one command");
  ast::Module* const mod = module_of(script->cmds[0]);
  if (mod == nullptr)
    return fail("shared1.wat: the one command is not a module");
  if (mod->fields.size() != 4)
    return fail("shared1.wat: expected 4 module fields");

  ast::Import* const memImport = std::get<ast::Import*>(mod->fields[0]);
  if (memImport == nullptr || memImport->kind != "memory" ||
      memImport->module_ != "js" || memImport->name != "memory" ||
      memImport->min != "1")
    return fail("shared1.wat: wrong memory import");
  ast::Import* const tableImport = std::get<ast::Import*>(mod->fields[1]);
  if (tableImport == nullptr || tableImport->kind != "table" ||
      tableImport->elemType != "funcref")
    return fail("shared1.wat: wrong table import");
  ast::TypeDef* const typeDef = std::get<ast::TypeDef*>(mod->fields[2]);
  if (typeDef == nullptr || typeDef->name != "$void_to_i32" ||
      typeDef->results.size() != 1 || typeDef->results[0] != "i32")
    return fail("shared1.wat: wrong type definition");
  ast::Func* const doIt = std::get<ast::Func*>(mod->fields[3]);
  if (doIt == nullptr || doIt->exports.size() != 1 ||
      doIt->exports[0] != "doIt")
    return fail("shared1.wat: wrong function export");
  if (doIt->body.size() != 5)
    return fail("shared1.wat: expected 5 instructions in the body");
  if (doIt->body[0]->op != "i32.const" || doIt->body[0]->args.size() != 1 ||
      std::get<std::string_view>(doIt->body[0]->args[0]) != "0")
    return fail("shared1.wat: wrong first instruction");
  if (doIt->body[2]->op != "i32.store")
    return fail("shared1.wat: expected i32.store as the third instruction");
  if (doIt->body[4]->op != "call_indirect" || doIt->body[4]->args.size() != 1 ||
      (std::get_if<ast::Instr*>(&doIt->body[4]->args[0]) == nullptr ||
       std::get<ast::Instr*>(doIt->body[4]->args[0])->op != "type" ||
       std::get<ast::Instr*>(doIt->body[4]->args[0])->args.size() != 1 ||
       std::get<std::string_view>(
           std::get<ast::Instr*>(doIt->body[4]->args[0])->args[0]) !=
           "$void_to_i32"))
    return fail("shared1.wat: wrong call_indirect");

  const std::string instrText =
      read_file(std::string(CURSIV_TEST_DIR) + "/instructions.wat");
  if (instrText.empty()) return fail("instructions.wat: empty or unreadable");
  const cursiv::ParseResult instr = language.language.parser->parse(instrText);
  if (instr.cst().recovered())
    return fail("instructions.wat: does not parse clean");
  if (!instr.diagnostics.empty())
    return fail("instructions.wat: has diagnostics");
  const ast::Script* const instrScript = as<ast::Script>(instr.value);
  ast::Module* const instrMod =
      instrScript != nullptr && instrScript->cmds.size() == 1
          ? module_of(instrScript->cmds[0])
          : nullptr;
  if (instrMod == nullptr) return fail("instructions.wat: expected one module");
  if (instrMod->fields.size() != 18)
    return fail("instructions.wat: expected 18 module fields");
  ast::Global* mutGlobal = nullptr;
  ast::Table* table = nullptr;
  ast::Memory* memory = nullptr;
  ast::Elem* elem = nullptr;
  ast::Data* data = nullptr;
  ast::Start* start = nullptr;
  int funcCount = 0;
  for (const ast::ModuleField& f : instrMod->fields) {
    if (ast::Global* const* g = std::get_if<ast::Global*>(&f);
        g != nullptr && *g != nullptr && (*g)->name == "$g")
      mutGlobal = *g;
    else if (ast::Table* const* t = std::get_if<ast::Table*>(&f); t != nullptr)
      table = *t;
    else if (ast::Memory* const* m = std::get_if<ast::Memory*>(&f);
             m != nullptr)
      memory = *m;
    else if (ast::Elem* const* e = std::get_if<ast::Elem*>(&f); e != nullptr)
      elem = *e;
    else if (ast::Data* const* d = std::get_if<ast::Data*>(&f); d != nullptr)
      data = *d;
    else if (ast::Start* const* s = std::get_if<ast::Start*>(&f); s != nullptr)
      start = *s;
    else if (std::holds_alternative<ast::Func*>(f))
      ++funcCount;
  }
  if (mutGlobal == nullptr || !mutGlobal->mutable_ ||
      mutGlobal->valueType != "i32")
    return fail("instructions.wat: wrong mutable global");
  if (table == nullptr || table->min != "2" || table->max != "10" ||
      table->elemType != "funcref")
    return fail("instructions.wat: wrong table");
  if (memory == nullptr || memory->exports.size() != 1 ||
      memory->exports[0] != "mem" || memory->min != "1" || memory->max != "4")
    return fail("instructions.wat: wrong memory");
  if (elem == nullptr || elem->funcVars.size() != 2)
    return fail("instructions.wat: wrong elem segment");
  if (data == nullptr || data->strings.size() != 1 || data->strings[0] != "hi")
    return fail("instructions.wat: wrong data segment");
  if (start == nullptr || start->func != "$init")
    return fail("instructions.wat: wrong start section");
  if (funcCount != 9) return fail("instructions.wat: expected 9 functions");

  const std::string headerlessText =
      read_file(std::string(CURSIV_TEST_DIR) + "/headerless.wat");
  const cursiv::ParseResult headerless =
      language.language.parser->parse(headerlessText);
  if (headerless.cst().recovered() || !headerless.diagnostics.empty())
    return fail("headerless.wat: does not parse clean");
  const ast::Script* const headerlessScript = as<ast::Script>(headerless.value);
  if (headerlessScript == nullptr || !headerlessScript->cmds.empty() ||
      headerlessScript->fields.size() != 3)
    return fail("headerless.wat: expected 3 bare module fields");

  const std::string scriptText =
      read_file(std::string(CURSIV_TEST_DIR) + "/script.wat");
  const cursiv::ParseResult scr = language.language.parser->parse(scriptText);
  if (scr.cst().recovered() || !scr.diagnostics.empty())
    return fail("script.wat: does not parse clean");
  const ast::Script* const scrRoot = as<ast::Script>(scr.value);
  if (scrRoot == nullptr || scrRoot->cmds.size() != 11)
    return fail("script.wat: expected 11 commands");
  if (module_of(scrRoot->cmds[0]) == nullptr)
    return fail("script.wat: the first command is not a module");
  const ast::Assertion* const ret =
      std::get_if<ast::Assertion*>(&scrRoot->cmds[2]) != nullptr
          ? *std::get_if<ast::Assertion*>(&scrRoot->cmds[2])
          : nullptr;
  if (ret == nullptr || ret->kind != "assert_return" ||
      ret->results.size() != 1)
    return fail("script.wat: wrong assert_return");
  bool sawRegister = false, sawBinQuote = false;
  for (const ast::Cmd& c : scrRoot->cmds) {
    if (std::holds_alternative<ast::Register*>(c)) sawRegister = true;
    if (const ast::Assertion* const* a = std::get_if<ast::Assertion*>(&c);
        a != nullptr && *a != nullptr &&
        std::holds_alternative<ast::BinQuoteModule*>((*a)->module_))
      sawBinQuote = true;
  }
  if (!sawRegister) return fail("script.wat: no register command found");
  if (!sawBinQuote) return fail("script.wat: no binary/quote module found");

  const std::string_view bad = "(module (func $f)";
  const cursiv::ParseResult badResult = language.language.parser->parse(bad);
  if (!badResult.cst().recovered())
    return fail("the unbalanced document should not parse clean");

  std::puts("wat: the corpus and coverage documents parse");
  return 0;
}
