#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "sieve/core/CoreServices.hpp"
#include "sieve/core/Values.hpp"
#include "sieve/core/ast.hpp"
#include "sieve/generated/Sieve/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = sieve::generated::Sieve::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = sieve::ast;
  auto r =
      parser->parse("/* hi */ redirect \"\"; if size :over 2k { discard; }");
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
        "sieve valid syntax regression");

  {
    const std::string source = "redirect text: # header\n  leading\n\n# "
                               "literal\n/* data */\n..dot\n.end\n.\n;";
    const auto result = parser->parse(source);
    check(result.accepted && !result.cst().recovered(), "multiline parse");
    const auto *root = result.value_as<ast::Script>();
    check(root && root->commands.size() == 1, "multiline command");
    const auto *args = root && root->commands.size() == 1
                           ? root->commands[0]->arguments
                           : nullptr;
    const auto *string = args && args->items.size() == 1
                             ? cursiv::ast_cast<ast::String>(args->items[0])
                             : nullptr;
    check(string && string->raw == source.substr(9, source.size() - 10),
          "multiline exact source");
    check(string && string->parent == args, "multiline parent");
    check(
        string &&
            sieve::string_value(*string) ==
                "  leading\r\n\r\n# literal\r\n/* data */\r\n.dot\r\n.end\r\n",
        "multiline value");
  }
  {
    const std::string source =
        "redirect \"a\\\"b\\\\c\\q\"; if size :over 18446744073709551615 {}";
    const auto result = parser->parse(source);
    const auto *root = result.value_as<ast::Script>();
    check(result.accepted && !result.cst().recovered() && root &&
              root->commands.size() == 2,
          "typed values parse");
    if (root && root->commands.size() == 2) {
      const auto *string =
          cursiv::ast_cast<ast::String>(root->commands[0]->arguments->items[0]);
      check(string && sieve::string_value(*string) == "a\"b\\cq",
            "quoted escapes");
      const auto *test = root->commands[1]->arguments->tests[0];
      const auto *tag = cursiv::ast_cast<ast::Tag>(test->arguments->items[0]);
      const auto *number =
          cursiv::ast_cast<ast::Number>(test->arguments->items[1]);
      check(tag && tag->text == ":over" && tag->parent == test->arguments,
            "typed tag");
      check(number && number->text == "18446744073709551615" &&
                sieve::number_value(*number) == UINT64_MAX,
            "uint64 value");
    }
  }
  for (const std::string source :
       {"", "# comment\r hidden\n", "/* comment */"}) {
    const auto result = parser->parse(source);
    const auto *root = result.value_as<ast::Script>();
    check(result.accepted && !result.cst().recovered() && root &&
              root->commands.empty(),
          "empty script");
  }
  const auto language = cursiv::create_language<sieve::SieveCoreServices,
                                                sieve::SieveSharedCoreServices,
                                                sieve::install_core_module>();
  for (const auto &[source, code] :
       {std::pair{"if true;", "sieve.block"},
        {"else {}", "sieve.branch"},
        {"if allof () {}", "sieve.test-arguments"},
        {"if allof true {}", "sieve.test-arguments"},
        {"if not (true) {}", "sieve.test-arguments"},
        {"if (true) {}", "sieve.command-arguments"},
        {"require [];", "sieve.string-list"},
        {"if size :over 18446744073709551616 {}", "sieve.number"},
        {"redirect;", "sieve.command-arguments"},
        {"require \"fileinto\"; fileinto [\"Box\"];",
         "sieve.command-arguments"},
        {"if true {} else {}", ""},
        {"redirect text: # header\r words\n\n.\n;", ""}}) {
    cursiv::Document document(cursiv::TextDocument::create(
        "memory:sieve-values", "sieve", 0, source));
    document.language = &language.language;
    document.parseResult = parser->parse(document.textDocument().getText());
    check(document.parseResult.accepted &&
              !document.parseResult.cst().recovered(),
          "runtime syntax permissive");
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(document,
                                                          diagnostics);
    check(diagnostics.size() == (std::string_view(code).empty() ? 0 : 1),
          "runtime count");
    if (diagnostics.size() == 1)
      check(diagnostics[0].code && diagnostics[0].code->string() == code &&
                diagnostics[0].range.end > diagnostics[0].range.begin &&
                diagnostics[0].range.end <= std::string_view(source).size(),
            "runtime code/range");
  }
  return ok;
}
} // namespace audit_regressions
