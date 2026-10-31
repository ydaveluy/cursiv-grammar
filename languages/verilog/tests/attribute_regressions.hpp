#pragma once
#include <algorithm>
#include <cstdio>
#include <string_view>
#include <unordered_set>
#include <vector>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
#include "verilog/core/CoreServices.hpp"
#include "verilog/core/ast.hpp"
namespace verilog_attributes {
struct Expected {
  std::string_view name;
  const void* ownerType;
  bool hasValue;
  std::string_view op;
};
template<class T>
Expected owned(std::string_view name, bool value = false, std::string_view op = {}) {
  return {name, &cursiv::ast_type_tag<T>, value, op};
}
inline bool run(const verilog::VerilogCoreServices& services,
                const verilog::VerilogSharedCoreServices& shared) {
  namespace ast = verilog::ast;
  struct Case {
    const char* id;
    const char* source;
    std::vector<Expected> expected;
    std::size_t instances;
  };
  const Case cases[] = {
    {"module-string-delimiter", "(* top = \"*)\", keep *) module m; endmodule", {owned<ast::ModuleDecl>("top", true, ""), owned<ast::ModuleDecl>("keep", false, "")}, 1},
    {"repeated-instances", "(* first *)(* second=2 *) module m; endmodule", {owned<ast::ModuleDecl>("first", false, ""), owned<ast::ModuleDecl>("second", true, "")}, 2},
    {"declarations", "module m((* port=1 *) input a, output b); (* net=2 *) wire n; (* varattr=3 *) reg q; (* ev *) event e; (* gen *) genvar g; (* par=4 *) parameter P=1; endmodule", {owned<ast::PortDecl>("port", true, ""), owned<ast::NetDecl>("net", true, ""), owned<ast::RegDecl>("varattr", true, ""), owned<ast::EventDecl>("ev", false, ""), owned<ast::GenvarDecl>("gen", false, ""), owned<ast::ParamDecl>("par", true, "")}, 6},
    {"function-task", "module m; (* fun *) function integer f; (* arg *) input a; (* localattr *) integer v; (* body *) f = a; endfunction (* taskattr *) task t; (* taskport *) input a; (* taskbody *) ; endtask endmodule", {owned<ast::FunctionDecl>("fun", false, ""), owned<ast::PortDecl>("arg", false, ""), owned<ast::VarDecl>("localattr", false, ""), owned<ast::AssignStmt>("body", false, ""), owned<ast::TaskDecl>("taskattr", false, ""), owned<ast::PortDecl>("taskport", false, ""), owned<ast::NullStmt>("taskbody", false, "")}, 7},
    {"statement-families", "module m; reg a,b; event e;\n(* process *) initial (* blockattr *) begin : named\n  (* decl *) integer i;\n  (* empty *) ;\n  (* assignment *) a=b;\n  (* ifattr *) if(a) (* thenattr *) a=b; else (* elseattr *) a=0;\n  (* caseattr *) case(a) 0: (* zeroattr *) a=0; default: a=1; endcase\n  (* forattr *) for(i=0;i<2;i=i+1) ;\n  (* whileattr *) while(a) ;\n  (* repeatattr *) repeat(1) ;\n  (* foreverattr *) forever ;\n  (* waitattr *) wait(a) ;\n  (* disableattr *) disable named;\n  (* triggerattr *) -> e;\n  (* callstmt *) $display(\"x\");\n  (* procattr *) force a=b;\n  (* delayattr *) #1 a=b;\n  (* forkattr *) fork ; join\nend\nendmodule", {owned<ast::InitialConstruct>("process", false, ""), owned<ast::BlockStmt>("blockattr", false, ""), owned<ast::VarDecl>("decl", false, ""), owned<ast::NullStmt>("empty", false, ""), owned<ast::AssignStmt>("assignment", false, ""), owned<ast::IfStmt>("ifattr", false, ""), owned<ast::AssignStmt>("thenattr", false, ""), owned<ast::AssignStmt>("elseattr", false, ""), owned<ast::CaseStmt>("caseattr", false, ""), owned<ast::AssignStmt>("zeroattr", false, ""), owned<ast::ForStmt>("forattr", false, ""), owned<ast::WhileStmt>("whileattr", false, ""), owned<ast::RepeatStmt>("repeatattr", false, ""), owned<ast::ForeverStmt>("foreverattr", false, ""), owned<ast::WaitStmt>("waitattr", false, ""), owned<ast::DisableStmt>("disableattr", false, ""), owned<ast::EventTriggerStmt>("triggerattr", false, ""), owned<ast::TaskEnableStmt>("callstmt", false, ""), owned<ast::ProcAssignStmt>("procattr", false, ""), owned<ast::TimingStmt>("delayattr", false, ""), owned<ast::BlockStmt>("forkattr", false, "")}, 21},
    {"instances-and-ports", "module child(input a); endmodule module m(input a, output y); (* instattr *) child u((* connectattr *) .a(a)); (* gateattr *) and g(y,a,a); (* alwaysattr *) always @a ; endmodule", {owned<ast::ModuleInst>("instattr", false, ""), owned<ast::NamedArg>("connectattr", false, ""), owned<ast::GateInst>("gateattr", false, ""), owned<ast::AlwaysConstruct>("alwaysattr", false, "")}, 4},
    {"call-unary-conditional", "module m(input a,b,c,output y); function f; input x; begin f=x; end endfunction assign y = f (* callattr=1 *)(a) + (* sum=2 *) - (* unaryattr=3 *) b ? (* condattr=4 *) a : c; endmodule", {owned<ast::CondExpr>("condattr", true, ""), owned<ast::BinaryExpr>("sum", true, "+"), owned<ast::CallExpr>("callattr", true, ""), owned<ast::UnaryExpr>("unaryattr", true, "-")}, 4},
    {"all-precedence-levels", "module m(input a,b,c,output y); assign y=a ** (* pow *) b * (* mul *) c + (* sum *) b << (* shift *) c < (* rel *) b == (* eq *) c & (* band *) b ^ (* bxor *) c | (* bor *) b && (* land *) c || (* lor *) b; endmodule", {owned<ast::BinaryExpr>("pow", false, "**"), owned<ast::BinaryExpr>("mul", false, "*"), owned<ast::BinaryExpr>("sum", false, "+"), owned<ast::BinaryExpr>("shift", false, "<<"), owned<ast::BinaryExpr>("rel", false, "<"), owned<ast::BinaryExpr>("eq", false, "=="), owned<ast::BinaryExpr>("band", false, "&"), owned<ast::BinaryExpr>("bxor", false, "^"), owned<ast::BinaryExpr>("bor", false, "|"), owned<ast::BinaryExpr>("land", false, "&&"), owned<ast::BinaryExpr>("lor", false, "||")}, 11},
    {"left-power", "module m(input a,b,c,output y); assign y=a ** (* first *) b ** (* second *) c; endmodule", {owned<ast::BinaryExpr>("first", false, "**"), owned<ast::BinaryExpr>("second", false, "**")}, 2},
    {"attribute-expression", "(* n /*before=*/ = (1+2)*3 *) module m; endmodule", {owned<ast::ModuleDecl>("n", true, "")}, 1},
    {"escaped-attribute-name", "(* \\wire = 1 *) module m; endmodule", {owned<ast::ModuleDecl>("wire", true, "")}, 1},
    {"duplicate-specs", "(* same=1,same=2 *)(* last *) module m; endmodule", {owned<ast::ModuleDecl>("same", true, ""), owned<ast::ModuleDecl>("same", true, ""), owned<ast::ModuleDecl>("last", false, "")}, 2},
    {"udp", "(* udpattr *) primitive p(q,a); (* outattr *) output q; (* inattr *) input a; table 0:0;1:1; endtable endprimitive", {owned<ast::UdpDecl>("udpattr", false, ""), owned<ast::PortDecl>("outattr", false, ""), owned<ast::PortDecl>("inattr", false, "")}, 3},
    {"specparam-outside-specify", "module m; (* timing *) specparam T=1; endmodule", {owned<ast::ParamDecl>("timing", false, "")}, 1},
    {"retained-module-contexts", "module m; parameter P=1; wire x; (* continuous *) assign x=1; (* overrideattr *) defparam P=2; (* genif *) if(1) begin : g wire q; end endmodule", {owned<ast::ContinuousAssign>("continuous", false, ""), owned<ast::ParamOverride>("overrideattr", false, ""), owned<ast::IfGenerate>("genif", false, "")}, 3},
  };
  for (const auto& test : cases) {
    const auto result = services.parser->parse(test.source);
    const auto fail = [&](const char* reason) {
      std::fprintf(stderr, "verilog attributes: %s: %s\n%s\n", test.id, reason, test.source);
      for (const auto& d : result.diagnostics)
        std::fprintf(stderr, "  kind=%d bytes=%zu..%zu element=%u\n", int(d.kind),
                     std::size_t(d.begin), std::size_t(d.end), unsigned(d.element));
      return false;
    };
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !cursiv::ast_cast<ast::SourceText>(result.value)) return fail("parse");
    std::vector<const ast::AttributeSpec*> specs;
    std::unordered_set<const cursiv::AstNode*> seen;
    std::size_t instances = 0;
    bool parents = result.value->parent == nullptr;
    const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { parents = false; return; }
      if (const auto* instance = cursiv::ast_cast<ast::AttributeInstance>(&node)) {
        ++instances;
        parents &= instance->parent &&
            (cursiv::ast_cast<ast::Item>(instance->parent) ||
             cursiv::ast_cast<ast::Stmt>(instance->parent) ||
             cursiv::ast_cast<ast::Expr>(instance->parent) ||
             cursiv::ast_cast<ast::NamedArg>(instance->parent));
      }
      if (const auto* spec = cursiv::ast_cast<ast::AttributeSpec>(&node)) {
        specs.push_back(spec);
        parents &= cursiv::ast_cast<ast::AttributeInstance>(spec->parent) != nullptr;
      }
      services.astReflection->visitMembers(node,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            parents &= child.parent == &node &&
                       result.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    visit(visit, *result.value);
    if (!parents || seen.size() != result.nodes.size()) return fail("ownership/CST");
    if (instances != test.instances || specs.size() != test.expected.size())
      return fail("attribute count");
    std::vector<bool> used(specs.size());
    for (const auto& expected : test.expected) {
      bool found = false;
      for (std::size_t i = 0; i < specs.size(); ++i) {
        const auto* spec = specs[i];
        const auto* owner = spec->parent ? spec->parent->parent : nullptr;
        if (used[i] || spec->name != expected.name || !owner ||
            owner->type != expected.ownerType || bool(spec->value) != expected.hasValue)
          continue;
        if (!expected.op.empty()) {
          const auto* binary = cursiv::ast_cast<ast::BinaryExpr>(owner);
          const auto* unary = cursiv::ast_cast<ast::UnaryExpr>(owner);
          if ((!binary || binary->op != expected.op) &&
              (!unary || unary->op != expected.op)) continue;
        }
        used[i] = true; found = true; break;
      }
      if (!found) return fail("attribute spec/owner/value/operator");
    }
    if (std::string_view(test.id) == "left-power") {
      const ast::BinaryExpr* outer = nullptr;
      for (const auto* spec : specs)
        if (spec->name == "second") outer = cursiv::ast_cast<ast::BinaryExpr>(spec->parent->parent);
      const auto* left = outer ? cursiv::ast_cast<ast::BinaryExpr>(outer->left) : nullptr;
      if (!left || left->op != "**" || left->attributes.size() != 1 ||
          left->attributes[0]->specs.size() != 1 || left->attributes[0]->specs[0]->name != "first")
        return fail("native left-associative power");
    }
    if (std::string_view(test.id) == "module-string-delimiter") {
      const auto* text = cursiv::ast_cast<ast::StringLit>(specs[0]->value);
      if (!text || text->text != "\"*)\"") return fail("string delimiter/value");
    }
    if (std::string_view(test.id) == "duplicate-specs") {
      if (specs[0]->name != "same" || specs[1]->name != "same" || specs[2]->name != "last")
        return fail("source order");
      const auto* first = cursiv::ast_cast<ast::IntLit>(specs[0]->value);
      const auto* second = cursiv::ast_cast<ast::IntLit>(specs[1]->value);
      if (!first || !second || first->text != "1" || second->text != "2")
        return fail("duplicate values retained");
    }
  }
  for (const char* source : {
      "(* *) module m; endmodule",
      "(* , *) module m; endmodule",
      "(* a, *) module m; endmodule",
      "(* a= *) module m; endmodule",
      "(* a b *) module m; endmodule",
      "(* 1=2 *) module m; endmodule",
      "(* a module m; endmodule",
      "(* a=(* b *) 1 *) module m; endmodule",
      "module m; wire x=1+(* a= *)2; endmodule",
      "module m; function f; input a; f=a; endfunction wire x=f(* , *)(1); endmodule",
  }) {
    const auto result = services.parser->parse(source);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty()) {
      std::fprintf(stderr, "verilog attributes: native malformed spec accepted: %s\n", source);
      return false;
    }
  }
  for (const char* source : {
      "(* outer = 1 + (* inner *) 2 *) module m; endmodule",
      "(* outer = 1 ? (* inner=2 *) 2 : 3 *) module m; endmodule",
      "(* outer = - (* inner *) 1 *) module m; endmodule",
  }) {
    auto document = shared.documentFactory->fromString(source, "file:///nested-attribute.v");
    if (!document->parseResult.accepted || document->parseResult.cst().recovered() ||
        !document->parseResult.diagnostics.empty()) {
      std::fprintf(stderr, "verilog attributes: nested spec must retain AST for validation: %s\n", source);
      return false;
    }
    std::vector<cursiv::Diagnostic> diagnostics;
    services.documentValidator->validateDocument(*document, diagnostics);
    std::size_t nested = 0;
    for (const auto& d : diagnostics)
      nested += d.code && d.code->isString() && d.code->string() == "verilog.nested-attribute";
    if (nested != 1 || diagnostics.size() != 1) {
      std::fprintf(stderr, "verilog attributes: missing precise nested-attribute diagnostic: %s\n", source);
      return false;
    }
  }
  {
    auto document = shared.documentFactory->fromString(
        "(* outer = (1+2)*3 *) module m; endmodule", "file:///constant-attribute.v");
    std::vector<cursiv::Diagnostic> diagnostics;
    services.documentValidator->validateDocument(*document, diagnostics);
    if (!document->parseResult.accepted || document->parseResult.cst().recovered() ||
        !diagnostics.empty()) return false;
  }
  return true;
}
}  // namespace verilog_attributes
