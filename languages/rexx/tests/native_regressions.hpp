#pragma once
#include <cstdio>
#include <string>
#include <vector>
#include "cursiv/core/parser/Parser.hpp"
#include "rexx/core/ast.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"

inline bool rexx_native_regressions(const auto& language) {
  namespace ast = rexx::ast;
  bool ok = true;
  auto check = [&](bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "rexx native: %s\n", message); ok = false; }
  };
  const auto result = language.language.parser->parse(
      "drop x\nfoo\noptions\n'start':\nsay 1.2.3\nsay 1e2\nsay ''x\nsay ''b\n");
  check(result.accepted && !result.cst().recovered() && result.diagnostics.empty(),
        "native DROP/command/OPTIONS/labels/symbols/empty encoded strings");
  const auto* root = result.template value_as<ast::Block>();
  check(root && root->body.size() == 8, "all native clauses retained");
  if (root && root->body.size() == 8) {
    const auto* drop = cursiv::ast_cast<ast::DropStmt>(root->body[0]);
    const auto* command = cursiv::ast_cast<ast::CommandStmt>(root->body[1]);
    const auto* options = cursiv::ast_cast<ast::OptionsStmt>(root->body[2]);
    const auto* label = cursiv::ast_cast<ast::Label>(root->body[3]);
    const auto* symbolSay = cursiv::ast_cast<ast::SayStmt>(root->body[4]);
    const auto* symbol = symbolSay ? cursiv::ast_cast<ast::NameRef>(symbolSay->value) : nullptr;
    const auto* numberSay = cursiv::ast_cast<ast::SayStmt>(root->body[5]);
    const auto* number = numberSay ? cursiv::ast_cast<ast::NumberLit>(numberSay->value) : nullptr;
    const auto* hexSay = cursiv::ast_cast<ast::SayStmt>(root->body[6]);
    const auto* binarySay = cursiv::ast_cast<ast::SayStmt>(root->body[7]);
    check(drop && drop->vars.size() == 1 && drop->vars[0]->name == "x" && command &&
              options && !options->value && label && label->name == "start",
          "DROP keyword is not a dropped variable and bare commands are not DROP");
    check(symbol && symbol->name == "1.2.3" && number && number->text == "1e2" &&
              hexSay && cursiv::ast_cast<ast::HexLit>(hexSay->value) &&
              binarySay && cursiv::ast_cast<ast::BinaryLit>(binarySay->value),
          "nonnumeric/numeric symbol categories and empty encoded literal kinds");
  }
  for (const char* source : {"say length ('x')\n", "say length/* c */('x')\n",
                             "say length/*outer /* inner */ tail*/('x')\n",
                             "say 'length'/*c*/('x')\n"}) {
    const auto parsed = language.language.parser->parse(source);
    const auto* block = parsed.template value_as<ast::Block>();
    const auto* say = block && block->body.size() == 1
        ? cursiv::ast_cast<ast::SayStmt>(block->body[0]) : nullptr;
    const bool commentCall = std::string_view(source).find("/*") != std::string_view::npos;
    const auto* call = say ? cursiv::ast_cast<ast::Call>(say->value) : nullptr;
    const auto* concat = say ? cursiv::ast_cast<ast::Concat>(say->value) : nullptr;
    check(parsed.accepted && !parsed.cst().recovered() &&
              (commentCall ? (call && call->name == "length" && call->args.size() == 1) :
                             (concat && concat->blank && cursiv::ast_cast<ast::ParenExpr>(concat->right))),
          "comments preserve function adjacency, a real blank produces concatenation");
  }
  for (const char* source : {"say 'a'/*c*/'b'\n", "say 'a' /*c*/'b'\n", "say 'a'/*c*/ 'b'\n"}) {
    const auto parsed = language.language.parser->parse(source);
    const auto* block = parsed.template value_as<ast::Block>();
    const auto* say = block && block->body.size() == 1
        ? cursiv::ast_cast<ast::SayStmt>(block->body[0]) : nullptr;
    const auto* concat = say ? cursiv::ast_cast<ast::Concat>(say->value) : nullptr;
    check(parsed.accepted && !parsed.cst().recovered() && concat &&
              concat->blank == (std::string_view(source).find(" ", 4) != std::string_view::npos),
          "comments do not create blanks or erase surrounding blanks");
  }
  for (const char* source : {"say 1 ^= 2\n", "say ~0\n", "say '1'xylophone\n"}) {
    const auto parsed = language.language.parser->parse(source);
    check(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty(),
          "native NOT aliases and word-followed suffix name");
  }
  const char* invalid[] = {
      "binary_bad1 = '0 1 0 1'b\n",
      "binary_bad2 = \"1 0 1 0\"B\n",
      "binary_bad3 = \"10 10\"b\n",
      "binary_bad4 = '10 10'B\n",
      "binary_bad5 = \"0 101\"b\n",
      "binary_bad6 = '0 101'B\n",
      "binary_bad7 = '1 1'b\n",
      "binary_bad8 = '1 11'b\n",
      "binary_bad9 = '11 11'b\n",
      "binary_bad10 = '1111 1'b\n",
      "binary_bad11 = '111 111'b\n",
      "binary_bad12 = '1111 111'b\n",
      "binary_bad13 = '11 1111 11'b\n",
      "binary_bad14 = '11 1111 111'b\n",
      "binary_bad15 = '1 1111 1111 1'b\n",
      "hex_bad1  = ' de af'x\n",
      "hex_bad2  = \"de af \"X\n",
      "hex_bad3  = \"d e af\"x\n",
      "hex_bad4  = 'de a f'X\n",
      "hex_bad5  = \"hi mom!\"x\t/* Not a hex string - invalid characters */\n",
      "say 'C1\"x\n", "say ' C1 'x\n"
  };
  for (const char* source : invalid) {
    const auto parsed = language.language.parser->parse(source);
    check(!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty(),
          "original malformed encoded literal must not become string/name concatenation");
  }
  int index = 0;
  for (const char* source : {"1.2.3 = 4\n", "do i = 1 to 2\nend j\n", "do\nend i\n"}) {
    auto doc = language.shared->documentFactory->fromString(
        source, "file:///native-rexx/" + std::to_string(index++) + ".rex");
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(*doc, diagnostics);
    check(!diagnostics.empty(), "constant assignment and mismatched END need diagnostics");
  }
  return ok;
}
