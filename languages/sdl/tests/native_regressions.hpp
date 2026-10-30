#pragma once
#include <cstdio>
#include <string>
#include <vector>
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
#include <string_view>
#include "cursiv/core/parser/Parser.hpp"
#include "sdl/core/ast.hpp"

inline bool sdl_native_regressions(const auto& language) {
  namespace ast = sdl::ast;
  bool ok = true;
  const auto check = [&](bool good, const char* what) {
    if (!good) { std::fprintf(stderr, "sdl native: %s\n", what); ok = false; }
  };
  const auto result = language.language.parser->parse(
      "MODULE sample IDENT \"a\"\"b\ncd\";\n"
      "CONSTANT low EQUALS %Ag; CONSTANT digit EQUALS %A0; CONSTANT bang EQUALS %A!;\n"
      "CONSTANT hexall EQUALS %XFFFFFFFF; CONSTANT octall EQUALS %O37777777777;\n"
      "CONSTANT binall EQUALS %B11111111111111111111111111111111;\n"
      "ITEM bytes BYTE UNSIGNED; ITEM floating G_FLOATING COMPLEX;\n"
      "ITEM ptr POINTER_QUAD(WORD); ITEM \"length\" CHARACTER LENGTH 3;\n"
      "LITERAL;\n#define X \"hello\"\n{ raw comment\n/* also raw\nEND_LITERAL;\nEND_MODULE sample;");
  check(result.accepted && !result.cst().recovered() && result.diagnostics.empty(),
        "native quotes, ASCII characters, longword bit patterns and raw LITERAL text");
  const auto* program = result.template value_as<ast::Program>();
  const auto* module = program && program->items.size() == 1
      ? cursiv::ast_cast<ast::Module>(program->items[0]) : nullptr;
  check(module && module->ident == "a\"bcd" && module->body.size() == 11,
        "doubled quotes and physical record joining; all module items retained");
  if (module && module->body.size() == 11) {
    const int expected[] = {103, 48, 33, -1, -1, -1};
    for (std::size_t i = 0; i < 6; ++i) {
      const auto* statement = cursiv::ast_cast<ast::ConstantStmt>(module->body[i]);
      const auto* value = statement && statement->phrases.size() == 1
          ? cursiv::ast_cast<ast::NumberLit>(statement->phrases[0]->value) : nullptr;
      check(value && value->value == expected[i], "ASCII case/value and signed longword conversion");
    }
    const auto primitive = [&](std::size_t i) {
      const auto* declaration = cursiv::ast_cast<ast::ItemDecl>(module->body[i]);
      return declaration ? cursiv::ast_cast<ast::PrimitiveType>(declaration->type) : nullptr;
    };
    const auto* byte = primitive(6); const auto* real = primitive(7); const auto* ptr = primitive(8);
    check(byte && byte->keyword == "BYTE" && byte->unsigned_ &&
              real && real->keyword == "G_FLOATING" && real->complex_ &&
              ptr && ptr->keyword == "POINTER_QUAD" && ptr->object && ptr->object->type,
          "factored primitive families retain keyword/options/object AST fields");
    const auto* literal = cursiv::ast_cast<ast::LiteralStmt>(module->body[10]);
    check(literal && literal->text == "\n#define X \"hello\"\n{ raw comment\n/* also raw\n",
          "LITERAL passes quotes, comments, newlines and punctuation through unchanged");
  }
  {
    const auto boundary = language.language.parser->parse("MODULE x;CONSTANT n EQUALS 2147483647;END_MODULE;");
    check(boundary.accepted && !boundary.cst().recovered() && boundary.diagnostics.empty(),
          "maximum signed longword remains valid after explicit decimal conversion");
  }
  for (const char* source : {"MODULE 1bad;END_MODULE;", "MODULE x;ITEM LENGTH BYTE;END_MODULE;",
                             "MODULE x;CONSTANT n EQUALS 2147483648;END_MODULE;",
                             "MODULE x;CONSTANT n EQUALS %X100000000;END_MODULE;"}) {
    const auto invalid = language.language.parser->parse(source);
    const bool rejected = !invalid.accepted || invalid.cst().recovered() || !invalid.diagnostics.empty();
    if (!rejected) std::fprintf(stderr, "sdl native: accepted negative source=[%s] accepted=%d recovered=%d diagnostics=%zu\n",
        source, invalid.accepted, invalid.cst().recovered(), invalid.diagnostics.size());
    check(rejected, "invalid source identifier, reserved word or longword overflow rejected");
  }
  int index = 0;
  for (const char* source : {"MODULE x;END_MODULE y;", "MODULE x;AGGREGATE a STRUCTURE;END b;END_MODULE;",
                             "MODULE x;IFLANGUAGE C C;END_IFLANGUAGE;END_MODULE;",
                             "MODULE x;IFLANGUAGE C;END_IFLANGUAGE PASCAL;END_MODULE;"}) {
    const auto document = language.shared->documentFactory->fromString(
        source, "file:///native-sdl/" + std::to_string(index++) + ".sdl");
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(*document, diagnostics);
    check(!diagnostics.empty(), "native end-name and language consistency diagnostics");
  }
  return ok;
}
