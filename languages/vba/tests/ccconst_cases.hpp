#pragma once
#include <cstdio>
#include <string_view>
#include <unordered_set>
#include <vector>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
#include "vba/core/CoreServices.hpp"
#include "vba/core/ast.hpp"
namespace vba_ccconst {
inline bool run(const vba::VbaCoreServices& services,
                const vba::VbaSharedCoreServices& shared) {
  namespace ast = vba::ast;
  struct Case {
    const char* id;
    const char* source;
    const char* name;
    const char* suffix;
    const void* parentType;
    const void* valueType;
  };
  const Case cases[] = {
    {"module-first", "#Const Build = 1\nDim ordinary As Integer\n", "Build", "", &cursiv::ast_type_tag<ast::Module>, &cursiv::ast_type_tag<ast::IntLit>},
    {"module-between", "Dim first As Integer\n#Const Build = 1\nDim last As Integer\n", "Build", "", &cursiv::ast_type_tag<ast::Module>, &cursiv::ast_type_tag<ast::IntLit>},
    {"procedure", "Sub M()\n#Const Build = 1\nDim ordinary As Integer\nEnd Sub\n", "Build", "", &cursiv::ast_type_tag<ast::Procedure>, &cursiv::ast_type_tag<ast::IntLit>},
    {"block-context", "Sub M()\nIf True Then\n#Const Build = 1\nEnd If\nEnd Sub\n", "Build", "", &cursiv::ast_type_tag<ast::IfStmt>, &cursiv::ast_type_tag<ast::IntLit>},
    {"typed-name", "#Const Build% = 1\n", "Build", "%", &cursiv::ast_type_tag<ast::Module>, &cursiv::ast_type_tag<ast::IntLit>},
    {"case-insensitive", "#cOnSt Build = 1\n", "Build", "", &cursiv::ast_type_tag<ast::Module>, &cursiv::ast_type_tag<ast::IntLit>},
    {"prefix-name", "#Const ConstValue = 1\n", "ConstValue", "", &cursiv::ast_type_tag<ast::Module>, &cursiv::ast_type_tag<ast::IntLit>},
    {"quoted-name", "#Const [Build Flag] = 1\n", "[Build Flag]", "", &cursiv::ast_type_tag<ast::Module>, &cursiv::ast_type_tag<ast::IntLit>},
    {"intrinsic-call", "#Const Build = CInt(1)\n", "Build", "", &cursiv::ast_type_tag<ast::Module>, &cursiv::ast_type_tag<ast::IndexExpr>},
    {"constant-reference", "#Const Build = OtherConstant\n", "Build", "", &cursiv::ast_type_tag<ast::Module>, &cursiv::ast_type_tag<ast::NameExpr>},
    {"date-expression", "#Const Build = #1/1/2000#\n", "Build", "", &cursiv::ast_type_tag<ast::Module>, &cursiv::ast_type_tag<ast::DateLit>},
    {"arithmetic", "#Const Build = (1 + 2) * 3\n", "Build", "", &cursiv::ast_type_tag<ast::Module>, &cursiv::ast_type_tag<ast::BinaryExpr>},
    {"string", "#Const Build = \"Rem is text\"\n", "Build", "", &cursiv::ast_type_tag<ast::Module>, &cursiv::ast_type_tag<ast::StringLit>},
    {"single-quote-comment", "#Const Build = 1 ' Rem is ordinary comment text\n", "Build", "", &cursiv::ast_type_tag<ast::Module>, &cursiv::ast_type_tag<ast::IntLit>},
    {"continued-expression", "#Const Build = 1 + _\n 2\n", "Build", "", &cursiv::ast_type_tag<ast::Module>, &cursiv::ast_type_tag<ast::BinaryExpr>},
    {"continued-tail", "#Const Build = 1 _\n ' trailing comment\n", "Build", "", &cursiv::ast_type_tag<ast::Module>, &cursiv::ast_type_tag<ast::IntLit>},
    {"unicode-line-end", "#Const Build = 1 Dim ordinary As Integer ", "Build", "", &cursiv::ast_type_tag<ast::Module>, &cursiv::ast_type_tag<ast::IntLit>},
    {"unicode-indent", "　#Const Build = 1\n", "Build", "", &cursiv::ast_type_tag<ast::Module>, &cursiv::ast_type_tag<ast::IntLit>},
    {"eof-profile", "#Const Build = 1", "Build", "", &cursiv::ast_type_tag<ast::Module>, &cursiv::ast_type_tag<ast::IntLit>},
  };
  for (const auto& test : cases) {
    auto document = shared.documentFactory->fromString(test.source, "file:///ccconst.bas");
    const auto& r = document->parseResult;
    std::vector<cursiv::Diagnostic> diagnostics;
    services.documentValidator->validateDocument(*document, diagnostics);
    const auto fail = [&](const char* reason) {
      std::fprintf(stderr, "vba #Const: %s %s\n%s\n", test.id, reason, test.source);
      for (const auto& d : r.diagnostics)
        std::fprintf(stderr, "  parse kind=%d bytes=%zu..%zu\n", int(d.kind),
                     std::size_t(d.begin), std::size_t(d.end));
      for (const auto& d : diagnostics) std::fprintf(stderr, "  %s\n", d.message.c_str());
      return false;
    };
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() || !diagnostics.empty())
      return fail("specification-positive rejected");
    std::unordered_set<const cursiv::AstNode*> seen;
    const ast::CompilationConst* directive = nullptr;
    std::size_t count = 0;
    bool parents = cursiv::ast_cast<ast::Module>(r.value) != nullptr;
    const auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { parents = false; return; }
      if (const auto* n = cursiv::ast_cast<ast::CompilationConst>(&node)) { directive = n; ++count; }
      services.astReflection->visitMembers(node,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            parents &= child.parent == &node && r.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    if (r.value) walk(walk, *r.value);
    if (!parents || seen.size() != r.nodes.size() || count != 1 || !directive ||
        directive->name != test.name || !directive->parent ||
        directive->parent->type != test.parentType || !directive->value ||
        directive->value->type != test.valueType ||
        (directive->typeChar ? *directive->typeChar : std::string_view{}) != test.suffix)
      return fail("directive/value/owner/spelling AST");
  }
  struct Invalid { const char* id; const char* source; const char* localCode; };
  for (const auto& test : {
      Invalid{"missing-name", "#Const = 1\n", ""},
      Invalid{"missing-equals", "#Const Build 1\n", ""},
      Invalid{"missing-value", "#Const Build =\n", ""},
      Invalid{"keyword-prefix", "#ConstBuild = 1\n", ""},
      Invalid{"as-type", "#Const Build As Integer = 1\n", ""},
      Invalid{"two-bindings", "#Const Build = 1, Other = 2\n", ""},
      Invalid{"trailing-colon", "#Const Build = 1: Dim x\n", ""},
      Invalid{"after-statement", "Dim x: #Const Build = 1\n", "vba.cc-line-start"},
      Invalid{"after-label", "Sub M()\nL: #Const Build = 1\nEnd Sub\n", "vba.cc-line-start"},
      Invalid{"after-line-number", "Sub M()\n10 #Const Build = 1\nEnd Sub\n", "vba.cc-line-start"},
      Invalid{"trailing-rem", "#Const Build = 1 Rem comment\n", "vba.cc-line-end"},
  }) {
    auto document = shared.documentFactory->fromString(test.source, "file:///bad-ccconst.bas");
    const auto& r = document->parseResult;
    std::vector<cursiv::Diagnostic> diagnostics;
    services.documentValidator->validateDocument(*document, diagnostics);
    const bool parsed = r.accepted && !r.cst().recovered() && r.diagnostics.empty();
    bool precise = false;
    for (const auto& d : diagnostics)
      precise |= d.code && d.code->isString() && d.code->string() == test.localCode;
    if ((parsed && diagnostics.empty()) ||
        (parsed && std::string_view(test.localCode).size() && !precise)) {
      std::fprintf(stderr, "vba #Const: %s missing rejection/local diagnostic parsed=%d precise=%d\n%s\n",
                   test.id, parsed, precise, test.source);
      return false;
    }
  }
  return true;
}
}  // namespace vba_ccconst
