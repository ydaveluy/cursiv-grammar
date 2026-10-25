#pragma once
#include <cstdio>
#include <functional>
#include <vector>

#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "terraform/core/CoreServices.hpp"
#include "terraform/core/StringValue.hpp"
#include "terraform/generated/Terraform/Module.hpp"
namespace code_audit {
namespace ast = terraform::ast;
inline bool run() {
  const auto parser = terraform::generated::Terraform::create_parser();
  const auto reflection =
      terraform::generated::Terraform::create_ast_reflection();
  bool ok = true;
  const auto check = [&](bool value, const char* message) {
    if (!value) {
      std::fprintf(stderr, "terraform code audit: %s\n", message);
      ok = false;
    }
  };
  const auto clean = [&](const cursiv::ParseResult& r) {
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
          "clean parse");
  };
  const auto visit = [&](const cursiv::AstNode* root, auto&& callback) {
    std::function<void(const cursiv::AstNode*)> walk =
        [&](const cursiv::AstNode* n) {
          if (!n) return;
          callback(n);
          reflection->visitMembers(
              *n, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
                check(child.parent == n, "parent ownership");
                walk(&child);
              });
        };
    walk(root);
  };
  const char* const cases[] = {
      R"hcl(moved {
 from = old.x
 to = new.x
}
import {
 to = test.x
 id = "thing"
}
custom unquoted "line\nlabel" {}
_top = 42
)hcl",
      R"hcl(locals {
 a = f(1,2,)
 b = provider::aws::fn(xs...)
 c = true()
 d = f()[0].field
}
)hcl",
      R"hcl(locals {
 a = <<-END
  hello
  END
}
)hcl",
      R"hcl(# empty configuration
)hcl",
      R"hcl(locals {
 tuple = [for k,v in var.rows : v if v != null]
 object = {for k,v in var.rows : k => v... if v != null}
}
)hcl",
      R"hcl(locals {
 a = <<EOT
EOT
 b = <<-TAG-name
    text
    TAG-name-more
    TAG-name
 c = <<OTHER
a ${format("%s", "b")}
OTHER
}
)hcl",
      R"hcl(locals {
 a = [1,2][0]
 b = (var.a).b[0][1]
 c = var.rows[*].items[0].name
 d = var.rows.*.items[0].name
 e = var.rows.0.name
}
)hcl",
      R"hcl(locals {
 object = {(var.name) = 1, ("x"+"y") : 2, true = 3, "a\n\u03bb\U0001F600" = 4}
}
)hcl",
      R"hcl(locals {
 _internal = var._name
 café = ᢅ.foo-bar
 ͺ = 2
}
)hcl",
      R"hcl(locals {
 precise = 123456789012345678901234567890
 exponent = 1.25E+100
 tiny = 1e-100
}
)hcl",
      R"hcl(locals {
 x = -var.a * 2 + 3 >= 4 == false && true || false ? 1 : true ? 2 : 3
}
)hcl",
      R"hcl(locals {
 x = "a\n\r\t\"\\\u03bb\U0001F600$${literal}%%{literal}"
 nested = "hello ${format("%s", "world")}!"
 wrapped = "${var.x}"
 trimmed = "before ${~ var.x ~} after"
 directives = "%{ for k,v in var.x ~}${k}:${v}%{ if v }yes%{ else }no%{ endif }%{ endfor }"
}
)hcl",
  };
  for (const auto* source : cases) {
    const auto r = parser->parse(source);
    clean(r);
    check(r.value_as<ast::File>() != nullptr, "generic body root");
    visit(r.value, [](const auto*) {});
  }
  for (const auto* source :
       {"a = 1 # ignored\rb = 2\n", "a = 1 // ignored\rb = 2\n"}) {
    const auto r = parser->parse(source);
    clean(r);
    const auto* f = r.value_as<ast::File>();
    const auto* a = f && f->items.size() == 1
                        ? cursiv::ast_cast<ast::Argument>(f->items[0])
                        : nullptr;
    check(a && a->name == "a", "carriage return stays inside line comments");
  }
  {
    const auto r = parser->parse(
        "moved { from = old.x }\ncustom unquoted \"line\\nlabel\" {}\n_top = "
        "1\n");
    clean(r);
    const auto* f = r.value_as<ast::File>();
    const auto* b = f && f->items.size() == 3
                        ? cursiv::ast_cast<ast::Block>(f->items[1])
                        : nullptr;
    const auto* label = b && b->labels.size() == 2
                            ? cursiv::ast_cast<ast::StringLit>(b->labels[1])
                            : nullptr;
    const auto* text =
        label && label->parts.size() == 1
            ? cursiv::ast_cast<ast::TemplateText>(label->parts[0])
            : nullptr;
    check(b && b->blockType == "custom" &&
              cursiv::ast_cast<ast::Identifier>(b->labels[0]) && text &&
              terraform::decode_fragment(text->raw) == "line\nlabel",
          "generic block labels decode");
    check(f && cursiv::ast_cast<ast::Argument>(f->items[2]),
          "top-level attribute");
  }
  {
    const auto r = parser->parse(
        "locals { a = 123456789012345678901234567890 b = 1.25E+100 }");
    clean(r);
    std::vector<std::string_view> texts;
    visit(r.value, [&](const auto* n) {
      if (const auto* x = cursiv::ast_cast<ast::NumberLit>(n))
        texts.push_back(x->text);
    });
    check(
        texts == std::vector<std::string_view>{"123456789012345678901234567890",
                                               "1.25E+100"},
        "exact numbers");
  }
  {
    const auto r = parser->parse(
        "locals { a = [for k,v in rows : v if v != null] b = {for k,v in rows "
        ": k => v... if v != null} }");
    clean(r);
    std::vector<const ast::ForLoop*> forms;
    visit(r.value, [&](const auto* n) {
      if (const auto* x = cursiv::ast_cast<ast::ForLoop>(n)) forms.push_back(x);
    });
    check(forms.size() == 2, "two comprehension nodes");
    if (forms.size() == 2) {
      for (const auto* n : forms)
        check(n->variables.size() == 2 && n->variables[0] == "k" &&
                  n->variables[1] == "v" && n->collection && n->result &&
                  n->condition,
              "comprehension fields");
      check(!forms[0]->object && !forms[0]->key && forms[1]->object &&
                forms[1]->key && forms[1]->grouping,
            "tuple and object comprehensions");
    }
  }
  {
    const auto r = parser->parse(
        "locals { a = rows[*].items[0].name b = rows.*.items[0].name }");
    clean(r);
    const auto* f = r.value_as<ast::File>();
    const auto* b = f && f->items.size() == 1
                        ? cursiv::ast_cast<ast::Block>(f->items[0])
                        : nullptr;
    const auto* a0 = b && b->body.size() == 2
                         ? cursiv::ast_cast<ast::Argument>(b->body[0])
                         : nullptr;
    const auto* a1 = b && b->body.size() == 2
                         ? cursiv::ast_cast<ast::Argument>(b->body[1])
                         : nullptr;
    const auto* full =
        a0 ? cursiv::ast_cast<ast::SplatExpr>(a0->value) : nullptr;
    const auto* name =
        a1 ? cursiv::ast_cast<ast::MemberExpr>(a1->value) : nullptr;
    const auto* index =
        name ? cursiv::ast_cast<ast::IndexExpr>(name->object) : nullptr;
    const auto* legacy =
        index ? cursiv::ast_cast<ast::SplatExpr>(index->object) : nullptr;
    check(full && !full->legacy &&
              cursiv::ast_cast<ast::MemberExpr>(full->each) && legacy &&
              legacy->legacy && cursiv::ast_cast<ast::MemberExpr>(legacy->each),
          "full and attribute-only splat grouping");
  }
  {
    const auto r = parser->parse(
        "locals { x = \"hello ${format(\"%s\", \"world\")}!\" y = [\"abc\", "
        "\"def\"] z = provider::aws::fn(xs...) }");
    clean(r);
    std::vector<const ast::FuncCall*> calls;
    visit(r.value, [&](const auto* n) {
      if (const auto* x = cursiv::ast_cast<ast::FuncCall>(n))
        calls.push_back(x);
    });
    check(calls.size() == 2 && calls[0]->args.size() == 2 &&
              calls[0]->name.size() == 1 && calls[0]->name[0] == "format" &&
              calls[1]->name.size() == 3 && calls[1]->name[1] == "aws" &&
              calls[1]->expandFinal,
          "nested template quotes, namespaced call and expansion");
  }
  {
    const auto r =
        parser->parse("locals { x = -(a+b)[0] * 2 + 3 ? 1 : true ? 2 : 3 }");
    clean(r);
    const ast::TernaryExpr* ternary = nullptr;
    visit(r.value, [&](const auto* n) {
      if (!ternary) ternary = cursiv::ast_cast<ast::TernaryExpr>(n);
    });
    const auto* add =
        ternary ? cursiv::ast_cast<ast::BinaryExpr>(ternary->cond) : nullptr;
    const auto* mul =
        add ? cursiv::ast_cast<ast::BinaryExpr>(add->left) : nullptr;
    const auto* neg =
        mul ? cursiv::ast_cast<ast::UnaryExpr>(mul->left) : nullptr;
    check(add && add->op == ast::Operator::add && mul &&
              mul->op == ast::Operator::multiply && neg &&
              cursiv::ast_cast<ast::IndexExpr>(neg->operand) &&
              cursiv::ast_cast<ast::TernaryExpr>(ternary->elseValue),
          "traversal, unary, infix and conditional precedence");
  }
  {
    auto language =
        cursiv::create_language<terraform::TerraformCoreServices,
                                terraform::TerraformSharedCoreServices,
                                terraform::install_core_module>();
    const std::string source =
        "locals { a = +1 a = [for k,v,w in xs : v...] b = \"\\q%{ endif }%{ if "
        "true }\" }";
    cursiv::Document doc(cursiv::TextDocument::create("memory:terraform",
                                                      "terraform", 0, source));
    doc.parseResult = parser->parse(source);
    clean(doc.parseResult);
    unsigned errors = 0;
    auto sink = [&](cursiv::Diagnostic&&) { ++errors; };
    cursiv::ValidationAcceptor accept(sink, doc, source);
    visit(doc.parseResult.value, [&](const auto* n) {
      if (const auto* x = cursiv::ast_cast<ast::File>(n))
        language.language.validator->checkFile(*x, accept);
      if (const auto* x = cursiv::ast_cast<ast::Block>(n))
        language.language.validator->checkBlock(*x, accept);
      if (const auto* x = cursiv::ast_cast<ast::ForLoop>(n))
        language.language.validator->checkFor(*x, accept);
      if (const auto* x = cursiv::ast_cast<ast::UnaryExpr>(n))
        language.language.validator->checkUnary(*x, accept);
      if (const auto* x = cursiv::ast_cast<ast::StringLit>(n))
        language.language.validator->checkTemplate(*x, accept);
      if (const auto* x = cursiv::ast_cast<ast::TemplateText>(n))
        language.language.validator->checkText(*x, accept);
    });
    check(errors == 7, "precise runtime validation after permissive parse");
  }
  return ok;
}
}  // namespace code_audit
