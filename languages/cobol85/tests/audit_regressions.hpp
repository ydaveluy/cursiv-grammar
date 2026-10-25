#pragma once
#include "cobol85/core/CoreServices.hpp"
#include "cobol85/core/ast.hpp"
#include "cobol85/generated/Cobol85/Module.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = cobol85::generated::Cobol85::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = cobol85::ast;
  auto r = parser->parse(
      "       IDENTIFICATION DIVISION.\n       PROGRAM-ID. A.\n       "
      "PROCEDURE DIVISION.\n           CALL \"P\" USING BY VALUE X Y OMITTED "
      "BY CONTENT "
      "LENGTH OF Y BY REFERENCE Z END-CALL\n           INSPECT X REPLACING ALL "
      "\"A\" BY \"B\" BEFORE INITIAL \"C\"\n           STOP RUN.\n       END "
      "PROGRAM B.\n");
  check(r.accepted && !r.cst().recovered(), "CALL, INSPECT and END PROGRAM");
  auto *d = as<ast::CompilationUnit>(r.value);
  auto *p = d && !d->programs.empty() ? d->programs[0] : nullptr;
  auto *proc = p ? p->procedure : nullptr;
  auto *call = proc && proc->statements.size() > 1
                   ? as<ast::CallStatement>(proc->statements[0])
                   : nullptr;
  auto *inspect = proc && proc->statements.size() > 1
                      ? as<ast::InspectStatement>(proc->statements[1])
                      : nullptr;
  check(call && call->using_.size() == 5 &&
            call->using_[0]->mode == ast::CallMode::value &&
            call->using_[1]->mode == ast::CallMode::value &&
            call->using_[2]->mode == ast::CallMode::value &&
            call->using_[2]->omitted &&
            call->using_[3]->mode == ast::CallMode::content &&
            call->using_[3]->modifier.find("LENGTH") !=
                std::string_view::npos &&
            call->using_[4]->mode == ast::CallMode::reference,
        "CALL modes and modifier retained");
  check(inspect && inspect->replacements.size() == 1 &&
            inspect->replacements[0]->patterns.size() == 1 &&
            inspect->replacements[0]->patterns[0]->from &&
            inspect->replacements[0]->patterns[0]->to &&
            inspect->replacements[0]->patterns[0]->boundaries.size() == 1,
        "INSPECT operands and boundary retained");
  check(p && p->hasEndProgram && p->name == "A" && p->endName == "B",
        "mismatching names remain available for runtime validation");
  {
    const std::string source =
        "       IDENTIFICATION DIVISION.\n       PROGRAM-ID. A.\n       "
        "PROCEDURE DIVISION.\n           INSPECT X CONVERTING \"ab\" TO "
        "\"AB\" AFTER INITIAL \"c\"\n           STOP RUN.\n";
    auto converted = parser->parse(source);
    check(converted.accepted && !converted.cst().recovered(),
          "INSPECT CONVERTING parses");
    auto *unit = as<ast::CompilationUnit>(converted.value);
    auto *program = unit && !unit->programs.empty() ? unit->programs[0] : nullptr;
    auto *body = program ? program->procedure : nullptr;
    auto *statement = body && !body->statements.empty()
                          ? as<ast::InspectStatement>(body->statements[0])
                          : nullptr;
    check(statement && statement->converting && !statement->tallying &&
              !statement->replacing && statement->conversion &&
              statement->conversion->from && statement->conversion->to &&
              statement->conversion->boundaries.size() == 1,
          "INSPECT CONVERTING keeps its operands and boundary");
    check(statement && statement->conversion &&
              converted.cst().text(statement->conversion->cstNodeId) ==
                  "CONVERTING \"ab\" TO \"AB\" AFTER INITIAL \"c\"",
          "the conversion starts at CONVERTING");
  }
  {
    auto language = cursiv::create_language<cobol85::Cobol85CoreServices,
                                            cobol85::Cobol85SharedCoreServices,
                                            cobol85::install_core_module>();
    const std::string input =
        "       IDENTIFICATION DIVISION.\n       PROGRAM-ID. A.\n       "
        "PROCEDURE DIVISION.\n           STOP RUN.\n       END PROGRAM B.\n";
    cursiv::Document doc(
        cursiv::TextDocument::create("memory:test", "cobol85", 0, input));
    doc.parseResult = parser->parse(input);
    check(doc.parseResult.accepted && !doc.parseResult.cst().recovered(),
          "semantic error remains syntactically parseable");
    int errors = 0;
    auto sink = [&](cursiv::Diagnostic &&) { ++errors; };
    cursiv::ValidationAcceptor accept(sink, doc, input);
    auto *unit = as<ast::CompilationUnit>(doc.parseResult.value);
    if (unit && !unit->programs.empty())
      language.language.validator->checkEndName(*unit->programs[0], accept);
    else
      check(false, "runtime validation receives a compilation unit");
    check(errors == 1, "runtime validator reports semantic constraints");
  }
  return ok;
}
} // namespace audit_regressions
