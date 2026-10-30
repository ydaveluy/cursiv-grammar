#pragma once

#include <cstdio>
#include <string>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "oncrpc/core/CoreServices.hpp"
#include "oncrpc/core/ast.hpp"

namespace audit_regressions {
inline bool run() {
  namespace ast = oncrpc::ast;
  const auto language = cursiv::create_language<
      oncrpc::OncrpcCoreServices, oncrpc::OncrpcSharedCoreServices,
      oncrpc::install_core_module>();
  bool ok = true;
  auto check = [&](bool value, const char* message) {
    if (!value) {
      std::fprintf(stderr, "oncrpc regression: %s\n", message);
      ok = false;
    }
  };
  const std::string source =
      "const _negative = -077; "
      "const Huge = 999999999999999999999999999999999999999; "
      "const Text = \"hello world\"; const Ref = MAX; "
      "enum E { A, B = -077, C }; "
      "struct S { struct S *next; unsigned short int value; }; "
      "program P { version V { "
      "int GET(struct S *value, string text<32>, string other) = PROC; "
      "void PING() = 0; int POST(int value *) = 2; "
      "int VOID(void ignored) = 3; "
      "} = VERS; } = PROG;";
  const auto parsed = language.language.parser->parse(source);
  check(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty(),
        "rpcgen declarations parse");
  const auto* spec = cursiv::ast_cast<ast::Specification>(parsed.value);
  check(spec && spec->items.size() == 7, "ordered definitions");
  if (spec && spec->items.size() == 7) {
    const std::string_view values[]{"-077", "999999999999999999999999999999999999999",
                                    "\"hello world\"", "MAX"};
    for (std::size_t i = 0; i < 4; ++i) {
      const auto* node = cursiv::ast_cast<ast::ConstantDef>(spec->items[i]);
      check(node && node->value == values[i], "constant literal stays exact");
    }
    const auto* en = cursiv::ast_cast<ast::EnumDef>(spec->items[4]);
    check(en && en->body && en->body->consts.size() == 3 &&
              en->body->consts[0]->value.empty() &&
              en->body->consts[1]->value == "-077" &&
              en->body->consts[2]->value.empty(), "implicit enumeration values");
    const auto* st = cursiv::ast_cast<ast::StructDef>(spec->items[5]);
    const auto* member = st && st->body && st->body->members.size() == 2
        ? cursiv::ast_cast<ast::ValueDecl>(st->body->members[0]) : nullptr;
    const auto* type = member ? cursiv::ast_cast<ast::NamedType>(member->type) : nullptr;
    check(member && member->pointer && member->name == "next" && type &&
              type->prefix == "struct" && type->name == "S", "prefixed pointer type");
    const auto* program = cursiv::ast_cast<ast::ProgramDef>(spec->items[6]);
    const auto* version = program && program->versions.size() == 1
        ? program->versions[0] : nullptr;
    check(version && version->number == "VERS" && program->number == "PROG" &&
              version->procedures.size() == 4, "symbolic RPC numbers");
    if (version && version->procedures.size() == 4) {
      const auto* proc = version->procedures[0];
      check(proc->number == "PROC" && proc->arguments.size() == 3,
            "named procedure arguments");
      if (proc->arguments.size() == 3) {
        const auto* first = cursiv::ast_cast<ast::ValueDecl>(proc->arguments[0]);
        const auto* second = cursiv::ast_cast<ast::ValueDecl>(proc->arguments[1]);
        const auto* third = cursiv::ast_cast<ast::ValueDecl>(proc->arguments[2]);
        check(first && first->pointer && first->name == "value" && second &&
                  second->name == "text" && second->bound == ast::Bound::variable &&
                  second->size && *second->size == "32" && third &&
                  third->name == "other", "argument declarations retain their fields");
      }
      check(version->procedures[1]->arguments.empty(), "empty argument list");
      const auto& postfix = version->procedures[2]->arguments;
      const auto* arg = postfix.size() == 1 ? cursiv::ast_cast<ast::ValueDecl>(postfix[0]) : nullptr;
      check(arg && arg->pointer && arg->name == "value", "postfix pointer argument");
      const auto& voidArgs = version->procedures[3]->arguments;
      const auto* v = voidArgs.size() == 1 ? cursiv::ast_cast<ast::VoidDecl>(voidArgs[0]) : nullptr;
      check(v && v->name == "ignored", "named void argument is retained");
    }
  }
  check(cursiv::test::validationHelper(language.language, source).diagnostics.empty(),
        "native declarations validate");

  struct Invalid { std::string text; std::string_view code; std::string_view span; };
  for (const auto& item : {
      Invalid{"const X = 008;", "oncrpc.number", "008"},
      Invalid{"enum E { A = 2147483648 };", "oncrpc.range", "2147483648"},
      Invalid{"typedef int A<4294967296>;", "oncrpc.bound", "4294967296"},
      Invalid{"typedef int A<-1>;", "oncrpc.bound", "-1"},
      Invalid{"program P { version V { int GET(void) = 4294967296; } = 1; } = 1;",
              "oncrpc.range", "4294967296"},
      Invalid{"program P { version V { int GET(int first *second) = 1; } = 1; } = 1;",
              "oncrpc.argumentName", "second"},
      Invalid{"typedef int vector;", "oncrpc.reserved", "vector"},
      Invalid{"program P { version V { opaque GET(void) = 1; } = 1; } = 1;",
              "oncrpc.result", "opaque"},
      Invalid{"program P { version V { int GET(void, int value) = 1; } = 1; } = 1;",
              "oncrpc.voidArgument", "void"}}) {
    const auto result = language.language.parser->parse(item.text);
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty(),
          "invalid declaration stays parseable");
    const auto validation = cursiv::test::validationHelper(language.language, item.text);
    bool found = false;
    for (const auto& diagnostic : validation.diagnostics)
      if (diagnostic.code && diagnostic.code->isString() &&
          diagnostic.code->string() == item.code &&
          std::string_view(item.text).substr(diagnostic.range.begin,
              diagnostic.range.end - diagnostic.range.begin) == item.span) found = true;
    check(found, "runtime error points to the invalid value");
  }
  for (const auto source : {"const int = 1;", "struct struct { int x; };",
                             "enum E { case = 1 };"}) {
    const auto result = language.language.parser->parse(source);
    check(!result.accepted || result.cst().recovered() ||
              !result.diagnostics.empty(), "rpcgen scanner keywords cannot be names");
  }
  {
    const auto result = language.language.parser->parse("const versioned = 1;");
    const auto* spec = cursiv::ast_cast<ast::Specification>(result.value);
    const auto* def = spec && spec->items.size() == 1
        ? cursiv::ast_cast<ast::ConstantDef>(spec->items[0]) : nullptr;
    check(result.accepted && !result.cst().recovered() &&
              result.diagnostics.empty() && def && def->name == "versioned",
          "keyword prefix remains an ordinary identifier");
  }
  return ok;
}
}  // namespace audit_regressions
