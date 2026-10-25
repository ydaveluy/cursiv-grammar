#pragma once

#include <cstdio>
#include <functional>

#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "solidity/core/CoreServices.hpp"
#include "solidity/generated/Solidity/Module.hpp"

namespace code_audit {
namespace ast = solidity::ast;
inline bool run() {
  const auto parser = solidity::generated::Solidity::create_parser();
  auto reflection = solidity::generated::Solidity::create_ast_reflection();
  bool ok = true;
  const auto check = [&](bool value, const char* message) {
    if (!value) {
      std::fprintf(stderr, "solidity code audit: %s\n", message);
      ok = false;
    }
  };
  const auto clean = [&](const cursiv::ParseResult& result) {
    check(result.accepted && !result.cst().recovered() &&
              result.diagnostics.empty(),
          "clean parse");
  };
  {
    const auto result = parser->parse(
        "contract C { ufixed8x1 a; fixed8x0 b; ufixed128x0 c; uint7 d; bytes33 "
        "e; }");
    clean(result);
    const auto* unit = result.value_as<ast::SourceUnit>();
    const auto* contract =
        unit && unit->items.size() == 1
            ? cursiv::ast_cast<ast::ContractDefinition>(unit->items[0])
            : nullptr;
    check(contract && contract->body.size() == 5, "type declarations");
    const std::string_view expected[] = {"ufixed8x1", "fixed8x0", "ufixed128x0",
                                         "uint7", "bytes33"};
    if (contract && contract->body.size() == 5)
      for (unsigned i = 0; i < 5; ++i) {
        const auto* variable =
            cursiv::ast_cast<ast::StateVariableDeclaration>(contract->body[i]);
        const auto* type =
            variable ? cursiv::ast_cast<ast::ElementaryType>(variable->type)
                     : nullptr;
        const auto* named =
            variable ? cursiv::ast_cast<ast::NamedType>(variable->type)
                     : nullptr;
        check(variable && variable->parent == contract &&
                  (i < 3 ? type && type->parent == variable &&
                               type->text == expected[i]
                         : named && named->parent == variable &&
                               named->path == expected[i]),
              "elementary and named type identity and spelling");
      }
  }
  {
    const auto result = parser->parse(
        "contract B {} contract C layout at 1 + 2 * 3 is B {} contract D is B "
        "layout at 42 {}");
    clean(result);
    const auto* unit = result.value_as<ast::SourceUnit>();
    check(unit && unit->items.size() == 3, "storage layout contracts");
    if (unit && unit->items.size() == 3) {
      for (unsigned i = 1; i < 3; ++i) {
        const auto* contract =
            cursiv::ast_cast<ast::ContractDefinition>(unit->items[i]);
        check(contract && contract->baseContracts.size() == 1 &&
                  contract->baseContracts[0]->name == "B" &&
                  contract->storageLayouts.size() == 1,
              "layout before or after inheritance");
        if (!contract || contract->storageLayouts.size() != 1) continue;
        const auto* layout = contract->storageLayouts[0];
        check(layout && layout->parent == contract && layout->baseSlot &&
                  layout->baseSlot->parent == layout,
              "storage layout parents");
        if (i == 1 && layout) {
          const auto* add = cursiv::ast_cast<ast::BinaryExpr>(layout->baseSlot);
          const auto* multiply =
              add ? cursiv::ast_cast<ast::BinaryExpr>(add->right) : nullptr;
          const auto* one =
              add ? cursiv::ast_cast<ast::NumberLit>(add->left) : nullptr;
          check(add && add->op == "+" && multiply && multiply->op == "*" &&
                    one && one->text == "1" && multiply->parent == add,
                "layout expression precedence");
        }
      }
    }
  }
  {
    const std::string source =
        "contract C { uint constant A = 1e+2; uint constant B = .2E+3; string "
        "constant S = \"a\\\r\nb\"; }";
    const auto result = parser->parse(source);
    clean(result);
    const auto* unit = result.value_as<ast::SourceUnit>();
    const auto* contract =
        unit && unit->items.size() == 1
            ? cursiv::ast_cast<ast::ContractDefinition>(unit->items[0])
            : nullptr;
    check(contract && contract->body.size() == 3, "literal declarations");
    if (contract && contract->body.size() == 3) {
      for (unsigned i = 0; i < 2; ++i) {
        const auto* variable =
            cursiv::ast_cast<ast::StateVariableDeclaration>(contract->body[i]);
        const auto* number =
            variable ? cursiv::ast_cast<ast::NumberLit>(variable->initialValue)
                     : nullptr;
        check(number && number->text == (i == 0 ? "1e+2" : ".2E+3") &&
                  number->parent == variable,
              "signed exponent lexeme");
      }
      const auto* variable =
          cursiv::ast_cast<ast::StateVariableDeclaration>(contract->body[2]);
      const auto* string =
          variable ? cursiv::ast_cast<ast::StringLit>(variable->initialValue)
                   : nullptr;
      check(string && string->pieces.size() == 1 &&
                string->pieces[0] == "\"a\\\r\nb\"",
            "CRLF continuation preserved");
    }
  }
  {
    const auto result = parser->parse(
        "contract C { function f(bytes calldata a) external { a[:]; a[1:]; "
        "a[:2]; a[1:2]; } }");
    clean(result);
    const auto* unit = result.value_as<ast::SourceUnit>();
    const auto* contract =
        unit && unit->items.size() == 1
            ? cursiv::ast_cast<ast::ContractDefinition>(unit->items[0])
            : nullptr;
    const auto* function =
        contract && contract->body.size() == 1
            ? cursiv::ast_cast<ast::FunctionDefinition>(contract->body[0])
            : nullptr;
    const auto* block =
        function ? cursiv::ast_cast<ast::Block>(function->body) : nullptr;
    check(block && block->statements.size() == 4, "four slice forms");
    if (block && block->statements.size() == 4)
      for (unsigned i = 0; i < 4; ++i) {
        const auto* statement =
            cursiv::ast_cast<ast::ExpressionStatement>(block->statements[i]);
        const auto* range =
            statement ? cursiv::ast_cast<ast::IndexRangeExpr>(statement->value)
                      : nullptr;
        check(range && range->start && range->end &&
                  range->start->parent == range && range->end->parent == range,
              "slice bound parents");
        if (!range) continue;
        check((cursiv::ast_cast<ast::EmptyExpr>(range->start) != nullptr) ==
                  (i % 2 == 0),
              "omitted start bound");
        check((cursiv::ast_cast<ast::EmptyExpr>(range->end) != nullptr) ==
                  (i < 2),
              "omitted end bound");
      }
  }
  {
    const std::string source =
        "contract C { function f() public pure { assembly 'evmasm' { let x := "
        "1 } assembly \"\\x65vmasm\" (\"memory-safe\") { pop(1) } } }";
    const auto result = parser->parse(source);
    clean(result);
    const auto* unit = result.value_as<ast::SourceUnit>();
    const auto* contract =
        unit && unit->items.size() == 1
            ? cursiv::ast_cast<ast::ContractDefinition>(unit->items[0])
            : nullptr;
    const auto* function =
        contract && contract->body.size() == 1
            ? cursiv::ast_cast<ast::FunctionDefinition>(contract->body[0])
            : nullptr;
    const auto* block =
        function ? cursiv::ast_cast<ast::Block>(function->body) : nullptr;
    check(block && block->statements.size() == 2, "assembly dialect spellings");
    if (block && block->statements.size() == 2) {
      const auto* a = cursiv::ast_cast<ast::AssemblyStmt>(block->statements[0]);
      const auto* b = cursiv::ast_cast<ast::AssemblyStmt>(block->statements[1]);
      check(a && a->dialect == "'evmasm'" && a->parent == block,
            "single quoted assembly dialect");
      check(b && b->dialect == "\"\\x65vmasm\"" && b->flags.size() == 1 &&
                b->flags[0] == "\"memory-safe\"",
            "escaped dialect and memory flag");
    }
  }
  {
    auto language =
        cursiv::create_language<solidity::SolidityCoreServices,
                                solidity::SoliditySharedCoreServices,
                                solidity::install_core_module>();
    const std::string source =
        "uint constant M; type T is T; contract C layout at 1 layout at 2 { "
        "address view a; "
        "uint constant c = 1__2; function f() public { []; assembly \"other\" "
        "{} } }";
    cursiv::Document document(
        cursiv::TextDocument::create("memory:solidity", "solidity", 0, source));
    document.parseResult = parser->parse(source);
    clean(document.parseResult);
    unsigned errors = 0;
    auto sink = [&](cursiv::Diagnostic&&) { ++errors; };
    cursiv::ValidationAcceptor accept(sink, document, source);
    std::function<void(const cursiv::AstNode&)> visit =
        [&](const cursiv::AstNode& node) {
          if (const auto* n =
                  cursiv::ast_cast<ast::ConstantVariableDeclaration>(&node))
            language.language.validator->checkConstant(*n, accept);
          if (const auto* n = cursiv::ast_cast<ast::AssemblyStmt>(&node))
            language.language.validator->checkAssembly(*n, accept);
          if (const auto* n = cursiv::ast_cast<ast::ElementaryType>(&node))
            language.language.validator->checkType(*n, accept);
          if (const auto* n =
                  cursiv::ast_cast<ast::UserDefinedValueTypeDefinition>(&node))
            language.language.validator->checkValueType(*n, accept);
          if (const auto* n = cursiv::ast_cast<ast::InlineArrayExpr>(&node))
            language.language.validator->checkArray(*n, accept);
          if (const auto* n = cursiv::ast_cast<ast::NumberLit>(&node))
            language.language.validator->checkNumber(*n, accept);
          if (const auto* n = cursiv::ast_cast<ast::ContractDefinition>(&node))
            language.language.validator->checkContract(*n, accept);
          reflection->visitMembers(
              node, [&](std::string_view, std::uint32_t,
                        cursiv::AstNode& child) { visit(child); });
        };
    if (document.parseResult.value) visit(*document.parseResult.value);
    check(errors == 7,
          "type constraints, empty arrays, separators and duplicate layouts "
          "validate at runtime");
  }
  return ok;
}
}  // namespace code_audit
