
#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "rust/core/CoreServices.hpp"
#include "rust/core/ast.hpp"
// The smoke test of the language: every file in example/ parses clean and
// builds a Crate.

#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"

namespace {

int fail(const std::string& path, const char* what) {
  std::fprintf(stderr, "rust: %s: %s\n", path.c_str(), what);
  return 1;
}

std::vector<std::string> split(const std::string& s, char sep) {
  std::vector<std::string> parts;
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, sep))
    if (!item.empty()) parts.push_back(item);
  return parts;
}

const rust::ast::Expr* initializer(const cursiv::ParseResult& result) {
  if (!result.accepted || !result.diagnostics.empty()) return nullptr;
  const auto* root = result.value_as<rust::ast::Crate>();
  if (!root || root->items.size() != 1) return nullptr;
  const auto* fn = cursiv::ast_cast<rust::ast::Function>(root->items[0]);
  if (!fn || !fn->body || fn->body->statements.size() != 1) return nullptr;
  const auto* stmt =
      cursiv::ast_cast<rust::ast::LetStmt>(fn->body->statements[0]);
  return stmt ? stmt->value : nullptr;
}

int float_dot_guard(const cursiv::Parser& parser) {
  for (const std::string_view name : {"x", "_x", "é", "Ω", "日", "𝒳"}) {
    for (const bool call : {false, true}) {
      const std::string text = "fn main() { let _ = 1." + std::string(name) +
                               (call ? "(); }" : "; }");
      const auto result = parser.parse(text);
      const auto* expr = initializer(result);
      const rust::ast::Expr* receiver = nullptr;
      if (call) {
        const auto* method = cursiv::ast_cast<rust::ast::MethodCallExpr>(expr);
        if (!method || method->method != name)
          return fail(text, "expected a method call after the integer");
        receiver = method->receiver;
      } else {
        const auto* field = cursiv::ast_cast<rust::ast::FieldExpr>(expr);
        if (!field || field->field != name)
          return fail(text, "expected a field access after the integer");
        receiver = field->receiver;
      }
      const auto* integer = cursiv::ast_cast<rust::ast::LiteralExpr>(receiver);
      if (!integer || integer->text != "1")
        return fail(text, "the float consumed the member-access dot");
    }
  }
  for (const std::string_view literal :
       {"1.", "1.0", "1.0e2", "1e2", "1f32", "1.0f32"}) {
    for (const std::string_view gap : {"", " ", "/* comment */"}) {
      const std::string text = "fn main() { let _ = " + std::string(literal) +
                               std::string(gap) + "; }";
      const auto result = parser.parse(text);
      const auto* value =
          cursiv::ast_cast<rust::ast::LiteralExpr>(initializer(result));
      if (!value || value->text != literal)
        return fail(text, "expected the complete float literal");
    }
  }
  for (const bool inclusive : {false, true}) {
    const std::string text = inclusive ? "fn main() { let _ = 1..=2; }"
                                        : "fn main() { let _ = 1..2; }";
    const auto result = parser.parse(text);
    const auto* range = cursiv::ast_cast<rust::ast::RangeExpr>(initializer(result));
    if (!range || range->inclusive != inclusive)
      return fail(text, "expected a range after the integer");
    const auto* start = cursiv::ast_cast<rust::ast::LiteralExpr>(range->start);
    const auto* end = cursiv::ast_cast<rust::ast::LiteralExpr>(range->end);
    if (!start || !end || start->text != "1" || end->text != "2")
      return fail(text, "the float consumed the range delimiter");
  }
  return 0;
}

int comparison_chaining(const cursiv::Parser& parser) {
  {
    const std::string text = "fn main() { let _ = a < b < c; }";
    const auto result = parser.parse(text);
    const auto* outer =
        cursiv::ast_cast<rust::ast::BinaryExpr>(initializer(result));
    const auto* inner =
        outer ? cursiv::ast_cast<rust::ast::BinaryExpr>(outer->left) : nullptr;
    if (!outer || !inner || outer->op != rust::ast::Operator::lt ||
        inner->op != rust::ast::Operator::lt)
      return fail(text, "expected the complete comparison chain in the AST");
  }
  {
    const std::string text = "fn main() { let _ = (a < b) < c; }";
    const auto result = parser.parse(text);
    const auto* const outer =
        cursiv::ast_cast<rust::ast::BinaryExpr>(initializer(result));
    if (!outer || outer->op != rust::ast::Operator::lt)
      return fail(text, "expected < at the root");
    const auto* const paren =
        cursiv::ast_cast<rust::ast::GroupedExpr>(outer->left);
    const auto* const inner =
        paren ? cursiv::ast_cast<rust::ast::BinaryExpr>(paren->inner)
              : nullptr;
    if (!inner || inner->op != rust::ast::Operator::lt)
      return fail(text, "expected < under the parenthesised left operand");
  }
  {
    // Bitwise `&` still binds tighter than a comparison beside it.
    const std::string text = "fn main() { let _ = a & b < c; }";
    const auto result = parser.parse(text);
    const auto* const cmp =
        cursiv::ast_cast<rust::ast::BinaryExpr>(initializer(result));
    if (!cmp || cmp->op != rust::ast::Operator::lt)
      return fail(text, "expected < at the root");
    const auto* const band =
        cursiv::ast_cast<rust::ast::BinaryExpr>(cmp->left);
    if (!band || band->op != rust::ast::Operator::bit_and)
      return fail(text, "expected & under <");
  }
  return 0;
}

int literal_regressions(const cursiv::Parser& parser) {
  for (const std::string_view comment :
       {"//\n", "//\r\n", "// body\rfn ignored(){}\n"}) {
    const std::string text = std::string(comment) + "fn main(){}";
    const auto result = parser.parse(text);
    const auto* root = result.value_as<rust::ast::Crate>();
    const auto* function = root && root->items.size() == 1
                               ? cursiv::ast_cast<rust::ast::Function>(root->items[0])
                               : nullptr;
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !function || function->name != "main")
      return fail(text, "an empty line comment must preserve the following item");
  }
  for (const std::string_view gap :
       {" ", "/* comment */", "/**/", "/***/", "// comment\n", "\u200e"}) {
    const std::string text = "#!" + std::string(gap) + "[allow(unused)] fn main(){}";
    const auto result = parser.parse(text);
    const auto* root = result.value_as<rust::ast::Crate>();
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !root || root->innerAttrs.size() != 1 ||
        root->items.size() != 1)
      return fail(text, "an inner attribute after trivia must not become a shebang");
  }
  for (const std::string_view comment : {"/** doc */", "/*! doc */"}) {
    const std::string text = "#! " + std::string(comment) + " [not Rust]\nfn main(){}";
    const auto result = parser.parse(text);
    const auto* root = result.value_as<rust::ast::Crate>();
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !root || !root->innerAttrs.empty() ||
        root->items.size() != 1)
      return fail(text, "documentation comments must preserve the shebang reading");
  }
  for (const std::string_view literal :
       {"b'a'", R"(b'\xff')", "'é'", "'日'", "'🦀'", R"('\u{1_f9_80}')",
        R"("\u{1_f9_80}")", "\"a\\\r\nb\"", "b\"a\\\r\nb\"",
        R"(c"hello")", R"(c"\xff")", R"(cr#"raw"#)"}) {
    const std::string text = "fn main() { let _ = " + std::string(literal) + "; }";
    const auto result = parser.parse(text);
    const auto* value =
        cursiv::ast_cast<rust::ast::LiteralExpr>(initializer(result));
    if (!value || value->text != literal)
      return fail(text, "expected the complete literal without recovery");
  }
  for (const std::string_view space :
       {"\t", "\v", "\f", "\u0085", "\u200e", "\u200f", "\u2028", "\u2029"}) {
    const std::string text = "fn" + std::string(space) + "main(){}";
    const auto result = parser.parse(text);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty())
      return fail(text, "expected Rust whitespace");
  }
  const auto bom = parser.parse("\xEF\xBB\xBF#!/usr/bin/env rustx\nfn main(){}");
  if (!bom.accepted || bom.cst().recovered() || !bom.diagnostics.empty())
    return fail("BOM and shebang", "expected a clean crate");
  for (const std::string_view literal :
       {"1tag", "1.0tag", R"("text"tag)", R"(r#"raw"#tag)", "b'x'tag",
        R"(b"bytes"tag)", R"(br#"raw"#tag)", R"(c"C"tag)",
        R"(cr#"raw"#tag)", "'é'tag"}) {
    const std::string text = "m!(" + std::string(literal) + ");";
    const auto result = parser.parse(text);
    const auto* root = result.value_as<rust::ast::Crate>();
    const auto* macro = root && root->items.size() == 1
                            ? cursiv::ast_cast<rust::ast::MacroStmt>(root->items[0])
                            : nullptr;
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !macro || !macro->tokens ||
        macro->tokens->children.size() != 1 ||
        macro->tokens->children[0]->token != literal)
      return fail(text, "a suffixed macro literal must remain one token");
  }
  return 0;
}

int modern_statements(const cursiv::Parser& parser) {
  {
    const std::string text = "fn main(){ let _ = async move || 1; }";
    const auto result = parser.parse(text);
    const auto* closure =
        cursiv::ast_cast<rust::ast::ClosureExpr>(initializer(result));
    if (!closure || !closure->isAsync || !closure->isMove)
      return fail(text, "expected async closure flags in the AST");
  }
  const std::string text =
      "struct S<const N: usize = 4>; "
      "fn main(){ let Some(x) = Some(1) else { return; }; }";
  const auto result = parser.parse(text);
  const auto* root = result.value_as<rust::ast::Crate>();
  if (!root || !result.accepted || result.cst().recovered() ||
      !result.diagnostics.empty() || root->items.size() != 2)
    return fail(text, "expected generic default and let-else without recovery");
  const auto* structure = cursiv::ast_cast<rust::ast::Struct>(root->items[0]);
  const auto* parameter = structure && structure->generics &&
                                 structure->generics->params.size() == 1
                             ? cursiv::ast_cast<rust::ast::ConstParam>(
                                   structure->generics->params[0])
                             : nullptr;
  const auto* value = parameter
                          ? cursiv::ast_cast<rust::ast::LiteralExpr>(
                                parameter->defaultValue)
                          : nullptr;
  const auto* function = cursiv::ast_cast<rust::ast::Function>(root->items[1]);
  const auto* statement = function && function->body &&
                                 function->body->statements.size() == 1
                             ? cursiv::ast_cast<rust::ast::LetStmt>(
                                   function->body->statements[0])
                             : nullptr;
  if (!value || value->text != "4" || !statement || !statement->value ||
      !statement->elseBranch || statement->elseBranch->statements.size() != 1)
    return fail(text, "generic default or let-else body missing from the AST");
  return 0;
}

int comparison_validation(rust::RustCoreServices& language) {
  for (const bool parenthesized : {false, true}) {
    const std::string text = parenthesized ? "fn main(){ let _ = (a < b) < c; }"
                                           : "fn main(){ let _ = a < b < c; }";
    const auto document = language.shared.documentFactory->fromString(
        text, "file:///comparison.rs");
    if (!document->parseResult.accepted || document->parseResult.cst().recovered() ||
        !document->parseResult.diagnostics.empty())
      return fail(text, "expected a clean parse before validation");
    std::vector<cursiv::Diagnostic> diagnostics;
    language.documentValidator->validateDocument(*document, diagnostics);
    if (diagnostics.size() != (parenthesized ? 0u : 1u))
      return fail(text, "expected a validation error only for the unparenthesized chain");
    if (!parenthesized &&
        (diagnostics[0].message.find("cannot be chained") == std::string::npos ||
         diagnostics[0].range.begin != text.rfind('<') ||
         diagnostics[0].range.end != text.rfind('<') + 1))
      return fail(text, "expected the diagnostic at the second comparison operator");
  }
  return 0;
}

}  // namespace

static int audit_regression() {
  const auto language = cursiv::create_language<rust::RustCoreServices,
                                                rust::RustSharedCoreServices,
                                                rust::install_core_module>();
  {
    const auto r =
        language.language.parser->parse("macro_rules! m { () => { 1 } }");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
    const auto* crate = r.value_as<rust::ast::Crate>();
    if (!crate || crate->items.empty()) return 1;
    const auto* m = cursiv::ast_cast<rust::ast::MacroRulesDef>(crate->items[0]);
    if (!m || !m->tokens || m->tokens->open != "{" ||
        m->tokens->children.size() != 3 ||
        m->tokens->children[0]->open != "(" ||
        m->tokens->children[2]->open != "{" ||
        m->tokens->children[2]->children.size() != 1 ||
        m->tokens->children[2]->children[0]->token != "1")
      return 1;
  }
  return 0;
}

int main() {
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  const auto language = cursiv::create_language<
      rust::RustCoreServices, rust::RustSharedCoreServices,
      rust::install_core_module>();

  if (float_dot_guard(*language.language.parser) != 0) return 1;
  if (comparison_chaining(*language.language.parser) != 0) return 1;
  if (literal_regressions(*language.language.parser) != 0) return 1;
  if (modern_statements(*language.language.parser) != 0) return 1;
  if (comparison_validation(language.language) != 0) return 1;

  for (const std::string& path : split(CURSIV_SAMPLES, ',')) {
    std::ifstream file(path);
    if (!file) return fail(path, "the sample document is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) return fail(path, "the sample does not parse");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<rust::ast::Crate>)
      return fail(path, "the parse built no Crate");

    std::printf("rust: %s parses\n", path.c_str());
  }

  return 0;
}
