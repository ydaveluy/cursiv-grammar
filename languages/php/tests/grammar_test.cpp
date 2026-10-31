
#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "php/core/CoreServices.hpp"
#include "php/core/ast.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "port_audit_cases.hpp"
#include "dnf_hook_cases.hpp"
// The smoke test of the language: parse every file under example/, the way a
// workspace would build one.

#include <deque>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "php: %s\n", what.c_str());
  return 1;
}

}  // namespace

static int audit_regression() {
  const auto language =
      cursiv::create_language<php::PhpCoreServices, php::PhpSharedCoreServices,
                              php::install_core_module>();
  {
    const auto r =
        language.language.parser->parse("<?php if (true) { ?>hello<?php } ?>");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
    const auto* doc = r.value_as<php::ast::Document>();
    const auto* phpBlock =
        doc && doc->items.size() == 1
            ? cursiv::ast_cast<php::ast::PhpBlock>(doc->items[0])
            : nullptr;
    const auto* conditional =
        phpBlock && phpBlock->items.size() == 1
            ? cursiv::ast_cast<php::ast::IfStmt>(phpBlock->items[0])
            : nullptr;
    const auto* block =
        conditional
            ? cursiv::ast_cast<php::ast::BlockStmt>(conditional->thenBranch)
            : nullptr;
    const auto* html =
        block && block->items.size() == 1
            ? cursiv::ast_cast<php::ast::InlineHtmlStmt>(block->items[0])
            : nullptr;
    if (!html || html->text != "hello") return 1;
  }
  {
    const auto r = language.language.parser->parse("<?php $x = $a?->b;");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
    const auto* doc = r.value_as<php::ast::Document>();
    const auto* block =
        doc && doc->items.size() == 1
            ? cursiv::ast_cast<php::ast::PhpBlock>(doc->items[0])
            : nullptr;
    const auto* stmt =
        block && block->items.size() == 1
            ? cursiv::ast_cast<php::ast::ExprStmt>(block->items[0])
            : nullptr;
    const auto* assignment =
        stmt ? cursiv::ast_cast<php::ast::AssignExpr>(stmt->value) : nullptr;
    const auto* member =
        assignment ? cursiv::ast_cast<php::ast::MemberExpr>(assignment->value)
                   : nullptr;
    if (!member || !member->nullsafe) return 1;
  }
  {
    const auto r =
        language.language.parser->parse("<?php $x = $a ?? $b ? $c : $d;");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
    const auto* doc = r.value_as<php::ast::Document>();
    const auto* block =
        doc && doc->items.size() == 1
            ? cursiv::ast_cast<php::ast::PhpBlock>(doc->items[0])
            : nullptr;
    const auto* stmt =
        block && block->items.size() == 1
            ? cursiv::ast_cast<php::ast::ExprStmt>(block->items[0])
            : nullptr;
    const auto* assignment =
        stmt ? cursiv::ast_cast<php::ast::AssignExpr>(stmt->value) : nullptr;
    const auto* conditional =
        assignment
            ? cursiv::ast_cast<php::ast::ConditionalExpr>(assignment->value)
            : nullptr;
    const auto* coalesce =
        conditional ? cursiv::ast_cast<php::ast::BinaryExpr>(conditional->cond)
                    : nullptr;
    if (!coalesce || coalesce->op != "??") return 1;
  }
  // The PHP lexer allows [ \t]* after <<< in all three heredoc/nowdoc
  // forms. The gap belongs to the opener, not the converted string value.
  const auto heredoc_value = [&](std::string_view gap, std::string_view quote,
                                std::string_view body, bool contentLine,
                                std::string_view newline = "\n",
                                std::string_view tag = "TAG") {
    const std::string input = "<?php $x = <<<" + std::string(gap) +
        std::string(quote) + std::string(tag) + std::string(quote) + std::string(newline) +
        std::string(body) + (contentLine ? std::string(newline) : "") +
        std::string(tag) + ";\n";
    const auto r = language.language.parser->parse(input);
    const auto* doc = r.value_as<php::ast::Document>();
    const auto* block = doc && doc->items.size() == 1
        ? cursiv::ast_cast<php::ast::PhpBlock>(doc->items[0]) : nullptr;
    const auto* stmt = block && block->items.size() == 1
        ? cursiv::ast_cast<php::ast::ExprStmt>(block->items[0]) : nullptr;
    const auto* assignment = stmt
        ? cursiv::ast_cast<php::ast::AssignExpr>(stmt->value) : nullptr;
    const auto* literal = assignment
        ? cursiv::ast_cast<php::ast::StringLit>(assignment->value) : nullptr;
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        !literal || literal->text != body) {
      std::fprintf(stderr, "php: heredoc value mismatch gap=%zu quote=%.*s body=%zu contentLine=%d\n",
                   gap.size(), static_cast<int>(quote.size()), quote.data(),
                   body.size(), contentLine);
      return false;
    }
    return true;
  };
  for (const std::string_view gap : {"", " ", "\t", "  ", " \t", "\t\t", "\t \t  \t"})
    for (const std::string_view quote : {"", "\"", "'"})
      for (const std::string_view body : {"", "alpha\nbeta", "body with $name and quotes\"'"})
        if (!heredoc_value(gap, quote, body, true)) return 1;
  // No content line is needed: the opener's newline can also start the closer.
  for (const std::string_view gap : {"", " \t"})
    for (const std::string_view quote : {"", "\"", "'"})
      if (!heredoc_value(gap, quote, "", false)) return 1;
  // A tag prefix followed by another label byte stays inside the body.
  for (const std::string_view gap : {"", " \t"})
    for (const std::string_view quote : {"", "\"", "'"})
      for (const std::string_view body : {"TAGsuffix", "first\nTAGsuffix", "TAG_", "TAG9", "TAGé", "TAG€"})
        if (!heredoc_value(gap, quote, body, true)) return 1;
  // CRLF and CR are line breaks too; internal line endings stay unchanged.
  for (const std::string_view newline : {"\r\n", "\r"})
    for (const std::string_view gap : {"", " \t"})
      for (const std::string_view quote : {"", "\"", "'"})
        if (!heredoc_value(gap, quote, "", false, newline) ||
            !heredoc_value(gap, quote,
                           "alpha" + std::string(newline) + "beta", true, newline) ||
            !heredoc_value(gap, quote, "a\r\nb\rc\nd", true, newline)) return 1;
  // PHP label bytes may also be non-ASCII, including multibyte UTF-8.
  for (const std::string_view tag : {"TÉG", "标签", "€"})
    for (const std::string_view quote : {"", "\"", "'"})
      if (!heredoc_value(" \t", quote, "", false, "\n", tag) ||
          !heredoc_value(" \t", quote,
                         std::string(tag) + "suffix\nbody $name", true, "\n", tag)) return 1;
  return 0;
}

int main() {
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  namespace fs = std::filesystem;
  std::vector<fs::path> files;
  for (const fs::directory_entry& entry :
       fs::directory_iterator(CURSIV_EXAMPLE_DIR))
    if (entry.path().extension() == ".php") files.push_back(entry.path());
  if (files.empty()) return fail("no example files found");

  const auto language = cursiv::create_language<
      php::PhpCoreServices, php::PhpSharedCoreServices,
      php::install_core_module>();
  for (const auto source : {"<?php $a?:$b?:$c;", "<?php $a?$b:($c?$d:$e);",
      "<?php $a?$b?$c:$d:$e;", "<?php $a?1:$b=2;", "<?php $a<$b && $b<$c;",
      "<?php class C {public string $x {get => 'x';}}",
      "<?php $x=new #[A] readonly class {};", "<?php clone($o,['p'=>1]);"}) {
    const auto r=language.language.parser->parse(source);
    if(!r.accepted||r.cst().recovered()||!r.diagnostics.empty())return fail(std::string("native form rejected: ")+source);
  }
  for (const auto source : {"<?php $a<$b<$c;", "<?php $a==$b==$c;", "<?php $a<=>$b==$c;", "<?php $a?$b:$c?$d:$e;",
      "<?php $a?:$b?$c:$d;", "<?php f(,);", "<?php function f(,){}", "<?php $a=[,];",
      "<?php class C {public int $x}", "<?php class C {public string $x {get => 'x';};}",
      "<?php class C {public string $x {get => 'x';},$y;}"}) {
    const auto r=language.language.parser->parse(source);
    if(r.accepted&&!r.cst().recovered()&&r.diagnostics.empty())return fail(std::string("native-invalid form accepted: ")+source);
  }
  for(const auto source:{"<?php function if(){}", "<?php class if{}", "<?php enum if{}"}) {
    const auto r=cursiv::test::validationHelper(language.language,source);
    if(r.diagnostics.empty())return fail("reserved declaration name needs a runtime diagnostic");
  }
  for(const auto source:{"<?php class C {function if(){} const match=1;}", "<?php function readonly(){}"}) {
    const auto r=cursiv::test::validationHelper(language.language,source);
    if(!r.diagnostics.empty())return fail("native member/contextual function name rejected");
  }
  {
    const auto r=language.language.parser->parse("<?php $a=&$b;$x=new #[A] readonly class {};clone($o,['p'=>1]);$a?:$b?:$c;");
    bool reference=false, anonymous=false, clone=false, shortChain=false;
    for(const auto* node:r.nodes){
      if(const auto* n=cursiv::ast_cast<php::ast::AssignExpr>(node))reference|=n->byRef&&n->op=="=";
      if(const auto* n=cursiv::ast_cast<php::ast::NewExpr>(node))anonymous|=n->anonClass&&n->anonymousAttributes.size()==1&&n->anonymousModifiers.size()==1&&n->anonymousModifiers[0]=="readonly";
      if(const auto* n=cursiv::ast_cast<php::ast::CloneExpr>(node))clone|=n->callSyntax&&n->args.size()==2&&n->args[0]->value&&n->args[1]->value;
      if(const auto* n=cursiv::ast_cast<php::ast::ConditionalExpr>(node))shortChain|=!n->thenValue&&cursiv::ast_cast<php::ast::ConditionalExpr>(n->cond);
    }
    if(!r.accepted||r.cst().recovered()||!r.diagnostics.empty()||!reference||!anonymous||!clone||!shortChain)
      return fail("native reference/anonymous/clone/short-conditional AST lost");
  }


  if (!port_audit_cases::run(*language.language.parser))
    return fail("port audit regression");
  if (!dnf_hook_cases::run(language.language))
    return fail("DNF/property-hook regression");

  for (const fs::path& path : files) {
    std::ifstream file(path);
    if (!file) return fail(path.string() + ": not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(path.string() + ": the parse needed repair");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<php::ast::Document>)
      return fail(path.string() + ": the parse built no document");
  }

  // PHP 7.3's indented closer: the tag's line may start with blanks, in each
  // of the three heredoc/nowdoc forms.
  {
    const std::string text =
        "<?php\nfunction q($t) {\n    $sql = <<<SQL\n        SELECT *\n"
        "          FROM $t\n        SQL;\n    $raw = <<<'EOT'\n      raw\n"
        "    \tEOT;\n    return foo(<<<\"X\"\n        a\n        X, 1);\n}\n";
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("an indented heredoc closer does not parse clean");
  }

  // Precedence, checked at AST level: `<=>` is a peer of
  // `==`/`!=`/`===`/`!==`/`<>`, not its own tier between `??` and `?:`;
  // `print`'s operand reaches an assignment.
  {
    std::deque<std::string> kept_sources;
    std::deque<cursiv::ParseResult> kept_results;
    const auto stmt_of = [&](const std::string& stmt) -> const php::ast::Item* {
      kept_sources.push_back("<?php\n" + stmt + "\n");
      kept_results.push_back(language.language.parser->parse(kept_sources.back()));
      const cursiv::ParseResult& result = kept_results.back();
      if (result.cst().recovered() || !result.diagnostics.empty())
        return nullptr;
      const auto* const doc =
          static_cast<const php::ast::Document*>(result.value);
      if (doc == nullptr || doc->items.empty() ||
          doc->items[0]->type != &cursiv::ast_type_tag<php::ast::PhpBlock>)
        return nullptr;
      const auto* const block =
          static_cast<const php::ast::PhpBlock*>(doc->items[0]);
      return block->items.empty() ? nullptr : block->items[0];
    };
    const auto value_of = [&](const std::string& stmt) -> const php::ast::Expr* {
      const php::ast::Item* const item = stmt_of(stmt);
      if (item == nullptr ||
          item->type != &cursiv::ast_type_tag<php::ast::ExprStmt>)
        return nullptr;
      return static_cast<const php::ast::ExprStmt*>(item)->value;
    };
    const auto binary = [](const php::ast::Expr* e,
                            const char* op) -> const php::ast::BinaryExpr* {
      const auto* const b =
          e != nullptr && e->type == &cursiv::ast_type_tag<php::ast::BinaryExpr>
              ? static_cast<const php::ast::BinaryExpr*>(e)
              : nullptr;
      return b != nullptr && b->op == op ? b : nullptr;
    };

    // Equality and spaceship share a NON-associative native level.
    // Their nested AST is valid only with explicit grouping.
    {
      const auto* const eq = binary(value_of("($a <=> $b) == $c;"), "==");
      if (eq == nullptr || binary(eq->left, "<=>") == nullptr)
        return fail("precedence: `<=>` is not a peer of `==`");
    }
    // `print $a = $b;` -- print's operand reaches an assignment.
    {
      const auto* const value = value_of("print $a = $b;");
      if (value == nullptr ||
          value->type != &cursiv::ast_type_tag<php::ast::PrintExpr>)
        return fail("precedence: `print` did not parse its operand");
      const auto* const operand =
          static_cast<const php::ast::PrintExpr*>(value)->operand;
      if (operand == nullptr ||
          operand->type != &cursiv::ast_type_tag<php::ast::AssignExpr>)
        return fail("precedence: `print`'s operand did not reach `=`");
    }
    // `-2 ** 2` is `-(2 ** 2)`: `**` binds tighter than unary minus.
    {
      const auto* const value = value_of("-2 ** 2;");
      if (value == nullptr ||
          value->type != &cursiv::ast_type_tag<php::ast::UnaryExpr>)
        return fail("precedence: `-2 ** 2` did not parse as a unary minus");
      const auto* const unary = static_cast<const php::ast::UnaryExpr*>(value);
      if (unary->op != "-" || binary(unary->operand, "**") == nullptr)
        return fail("precedence: `**` does not bind tighter than unary minus");
    }
    // `!$a instanceof B` is `!($a instanceof B)`: `!` is looser than
    // `instanceof`.
    {
      const auto* const value = value_of("!$a instanceof B;");
      if (value == nullptr ||
          value->type != &cursiv::ast_type_tag<php::ast::UnaryExpr>)
        return fail("precedence: `!$a instanceof B` did not parse as `!`");
      const auto* const unary = static_cast<const php::ast::UnaryExpr*>(value);
      if (unary->op != "!" || unary->operand == nullptr ||
          unary->operand->type !=
              &cursiv::ast_type_tag<php::ast::InstanceofExpr>)
        return fail("precedence: `!` is not looser than `instanceof`");
    }
    // `$a = $b and $c` is `($a = $b) and $c`: assignment binds tighter
    // than `and`/`xor`/`or`.
    {
      const auto* const op = binary(value_of("$a = $b and $c;"), "and");
      if (op == nullptr || op->left == nullptr ||
          op->left->type != &cursiv::ast_type_tag<php::ast::AssignExpr>)
        return fail("precedence: assignment does not bind tighter than `and`");
    }
    // `$a ?? include 'x.php';` -- include/require reach every operand
    // position, including `??`'s.
    {
      const auto* const op = binary(value_of("$a ?? include 'x.php';"), "??");
      if (op == nullptr || op->right == nullptr ||
          op->right->type != &cursiv::ast_type_tag<php::ast::IncludeExpr>)
        return fail("precedence: `include` did not reach `??`'s operand");
    }
    {
      const auto* include = cursiv::ast_cast<php::ast::IncludeExpr>(
          value_of("include 'x.php' or die();"));
      if (!include || !binary(include->operand, "or"))
        return fail("precedence: include must retain the logical expression as its operand");
    }

    // Destructuring stores into an `ArrayExpr`; a slot skipped between two
    // commas holds no value.
    const auto list_of = [&](const std::string& stmt) -> const php::ast::ArrayExpr* {
      const auto* const e = value_of(stmt);
      if (e == nullptr || e->type != &cursiv::ast_type_tag<php::ast::AssignExpr>)
        return nullptr;
      const auto* const target = static_cast<const php::ast::AssignExpr*>(e)->target;
      return target != nullptr &&
                     target->type == &cursiv::ast_type_tag<php::ast::ArrayExpr>
                 ? static_cast<const php::ast::ArrayExpr*>(target)
                 : nullptr;
    };
    {
      const auto* const list = list_of("list($a) = $c;");
      if (list == nullptr || list->items.size() != 1 || list->items[0]->value == nullptr)
        return fail("destructuring: `list($a) = $c` did not parse");
    }
    {
      const auto* const list = list_of("[, $b] = $c;");
      if (list == nullptr || list->items.size() != 2 || list->items[0]->value != nullptr ||
          list->items[1]->value == nullptr)
        return fail("destructuring: `[, $b] = $c` lost its skipped slot");
    }
    {
      const auto* const list = list_of("list($a, , $c, ) = $d;");
      if (list == nullptr || list->items.size() != 3 || list->items[1]->value != nullptr ||
          list->items[2]->value == nullptr)
        return fail("destructuring: `list($a, , $c, ) = $d` mis-sized its list");
    }
    // `foreach ($a as $k => $v)`: the key comes first.
    {
      const php::ast::Item* const item = stmt_of("foreach ($a as $k => $v) {}");
      if (item == nullptr || item->type != &cursiv::ast_type_tag<php::ast::ForeachStmt>)
        return fail("foreach: not a ForeachStmt");
      const auto* const each = static_cast<const php::ast::ForeachStmt*>(item);
      const auto name = [](const php::ast::Expr* e) -> std::string_view {
        return e != nullptr && e->type == &cursiv::ast_type_tag<php::ast::Var>
                   ? static_cast<const php::ast::Var*>(e)->name
                   : std::string_view{};
      };
      if (name(each->key) != "$k" || name(each->value) != "$v")
        return fail("foreach: `$k => $v` swapped its key and its value");
    }
  }

  std::printf("php: %zu example(s) parse clean\n", files.size());
  return 0;
}
