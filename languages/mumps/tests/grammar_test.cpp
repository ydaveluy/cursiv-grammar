#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
#include "mumps/core/CoreServices.hpp"
#include "mumps/core/ast.hpp"
// The smoke test of the language: every corpus file of `example/` parses,
// clean unless it is named below as expected to fail, and a few real field
// values come back out of the AST -- not merely the shape of the tree.

#include <fstream>
#include <iterator>
#include <string>
#include <string_view>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

int fail(const char* what) {
  std::fprintf(stderr, "mumps: %s\n", what);
  return 1;
}

std::string read(const std::string& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
}

// AST nodes are identified by tag, not RTTI: `dynamic_cast` does not apply.
template <class T, class From>
const T* as(const From* node) {
  return (node != nullptr && node->type == &cursiv::ast_type_tag<T>)
             ? static_cast<const T*>(node)
             : nullptr;
}

// `epic_questions.m` is Caché ObjectScript beyond this grammar's scope:
// `while` loops and `{...}` blocks reach no parser rule at all, and its
// `zr`/`zs` Z-commands are not a command the source language defines. It
// stays in `example/` as the out-of-scope corpus file.
constexpr std::string_view kOutOfScope[] = {"epic_questions.m"};

bool expectedOutOfScope(std::string_view name) {
  for (const std::string_view f : kOutOfScope)
    if (f == name) return true;
  return false;
}

}  // namespace

static int audit_regression() {
  const auto language = cursiv::create_language<mumps::MumpsCoreServices,
                                                mumps::MumpsSharedCoreServices,
                                                mumps::install_core_module>();
  {
    const auto r = language.language.parser->parse("r\n S x=\"a\"_\"b\" W x\n");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr) {
      std::fprintf(stderr,
                   "audit parse: accepted=%d recovered=%d diagnostics=%zu\n",
                   r.accepted, r.cst().recovered(), r.diagnostics.size());
      for (const auto& d : r.diagnostics)
        std::fprintf(stderr, "%s\n", d.message.c_str());
      return 1;
    }
  }
  {
    const auto r = language.language.parser->parse("r\n G label\n");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr) {
      std::fprintf(stderr,
                   "audit parse: accepted=%d recovered=%d diagnostics=%zu\n",
                   r.accepted, r.cst().recovered(), r.diagnostics.size());
      for (const auto& d : r.diagnostics)
        std::fprintf(stderr, "%s\n", d.message.c_str());
      return 1;
    }
  }
  {
    const auto r =
        language.language.parser->parse("r\n F i=1:1:3,5,7:1:9 W i\n");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty()) return 1;
    const auto* doc = r.value_as<mumps::ast::Document>();
    const auto* line =
        doc && doc->lines.size() == 2
            ? cursiv::ast_cast<mumps::ast::CodeLine>(doc->lines[1])
            : nullptr;
    const auto* loop =
        line && line->commands.size() == 1
            ? cursiv::ast_cast<mumps::ast::ForCommand>(line->commands[0])
            : nullptr;
    if (!loop || loop->parameters.size() != 3 || !loop->parameters[0]->step ||
        !loop->parameters[0]->end || loop->parameters[1]->step ||
        !loop->parameters[2]->end || loop->commands.size() != 1)
      return 1;
  }
  {
    const auto r = language.language.parser->parse("r\n E TC\n");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty()) return 1;
    const auto* doc = r.value_as<mumps::ast::Document>();
    const auto* line =
        doc && doc->lines.size() == 2
            ? cursiv::ast_cast<mumps::ast::CodeLine>(doc->lines[1])
            : nullptr;
    if (!line || line->commands.size() != 2) return 1;
  }
  {
    auto doc = language.shared->documentFactory->fromString("r\n G\n",
                                                            "file:///audit.m");
    if (!doc->parseResult.accepted || doc->parseResult.cst().recovered() ||
        !doc->parseResult.diagnostics.empty())
      return 1;
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(*doc, diagnostics);
    if (diagnostics.empty()) return 1;
  }
  {
    const auto r =
        language.language.parser->parse("r\n S x=-.5 S y=+1. W \"a\"\"b\"\n");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty()) return 1;
  }
  return 0;
}

int main() {
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  const auto language = cursiv::create_language<
      mumps::MumpsCoreServices, mumps::MumpsSharedCoreServices,
      mumps::install_core_module>();

  constexpr std::string_view kFiles[] = {
      "epic_questions.m", "fibonacci.m", "for.m",   "hello.m",
      "hello2.m",         "hello3.m",    "horolog.m", "math1.m",
      "sampleproc.m",     "set.m"};

  for (const std::string_view name : kFiles) {
    const std::string path =
        std::string(CURSIV_EXAMPLE_DIR) + "/" + std::string(name);
    const std::string text = read(path);
    if (text.empty()) return fail("a corpus file is not readable");

    const cursiv::ParseResult result = language.language.parser->parse(text);
    const bool clean =
        !result.cst().recovered() && result.diagnostics.empty();

    if (expectedOutOfScope(name)) {
      if (clean) return fail("epic_questions.m was expected to fail");
      continue;
    }
    if (!clean) {
      std::fprintf(stderr, "mumps: %s does not parse clean\n", name.data());
      return fail("a corpus file does not parse clean");
    }
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<mumps::ast::Document>) {
      std::fprintf(stderr, "mumps: %s built no document\n", name.data());
      return fail("the parse built no document");
    }
  }

  // Real field values, not merely the tree's shape: fibonacci.m.
  {
    const std::string text =
        read(std::string(CURSIV_EXAMPLE_DIR) + "/fibonacci.m");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("fibonacci.m does not parse clean");
    const auto& doc =
        *static_cast<const mumps::ast::Document*>(result.value);
    if (doc.lines.size() != 4) return fail("fibonacci.m: wrong line count");

    const auto* head = as<mumps::ast::CodeLine>(doc.lines[0]);
    if (head == nullptr || head->label != "FIBONACCI" ||
        !head->commands.empty())
      return fail("fibonacci.m: the label line did not read back");

    const auto* setLine =
        as<mumps::ast::CodeLine>(doc.lines[1]);
    if (setLine == nullptr || setLine->commands.size() != 1)
      return fail("fibonacci.m: the SET line did not read back");
    const auto* setCmd =
        as<mumps::ast::SetCommand>(setLine->commands[0]);
    if (setCmd == nullptr || setCmd->assigns.size() != 1 ||
        setCmd->assigns[0]->targets.size() != 2 ||
        setCmd->assigns[0]->targets[0]->type !=
            &cursiv::ast_type_tag<mumps::ast::Variable>)
      return fail("fibonacci.m: SET (A,B)=1 did not read back");
    const auto* one = as<mumps::ast::NumberLiteral>(
        setCmd->assigns[0]->value);
    if (one == nullptr || one->value != 1.0)
      return fail("fibonacci.m: the assigned value did not read back");

    const auto* forLine =
        as<mumps::ast::CodeLine>(doc.lines[2]);
    if (forLine == nullptr || forLine->commands.size() != 1)
      return fail("fibonacci.m: the FOR line did not read back");
    const auto* forCmd =
        as<mumps::ast::ForCommand>(forLine->commands[0]);
    if (forCmd == nullptr || forCmd->commands.size() != 5)
      return fail("fibonacci.m: FOR's inner commands did not read back");
    const auto* quit =
        as<mumps::ast::QuitCommand>(forCmd->commands[4]);
    if (quit == nullptr || quit->postcondition == nullptr)
      return fail("fibonacci.m: QUIT:S>100's postcondition did not read back");
    const auto* cond =
        as<mumps::ast::BinaryExpression>(quit->postcondition);
    if (cond == nullptr || cond->op != mumps::ast::Operator::gt)
      return fail("fibonacci.m: QUIT:S>100's comparison did not read back");
  }

  // Constructs the corpus never exercises, but this language accepts:
  // BREAK/KILL/MERGE/VIEW/XECUTE, the bare keyword commands, a subscripted
  // global, `&`, `'>`/`'<`, string concatenation, a function argument that
  // is a string literal, and dot-nested commands under DO.
  {
    const std::string text =
        read(std::string(CURSIV_TEST_DIR) + "/coverage.m");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("coverage.m does not parse clean");
    const auto& doc = *static_cast<const mumps::ast::Document*>(result.value);

    const auto* decl = as<mumps::ast::RoutineDecl>(doc.lines[0]);
    if (decl == nullptr || !decl->percent || decl->name != "coverage" ||
        decl->params.size() != 3)
      return fail("coverage.m: the routine header did not read back");

    const auto* mergeLine = as<mumps::ast::CodeLine>(doc.lines[3]);
    const auto* merge =
        mergeLine != nullptr && mergeLine->commands.size() == 1
            ? as<mumps::ast::MergeCommand>(mergeLine->commands[0])
            : nullptr;
    if (merge == nullptr || merge->pairs.size() != 2)
      return fail("coverage.m: MERGE a=b,c=d did not read back");

    const auto* closeLine = as<mumps::ast::CodeLine>(doc.lines[9]);
    const auto* other =
        closeLine != nullptr && closeLine->commands.size() == 1
            ? as<mumps::ast::OtherCommand>(closeLine->commands[0])
            : nullptr;
    if (other == nullptr || other->keyword.empty())
      return fail("coverage.m: the bare CLOSE command did not read back");

    const auto* subLine = as<mumps::ast::CodeLine>(doc.lines[21]);
    const auto* glob =
        subLine != nullptr && subLine->commands.size() == 1
            ? as<mumps::ast::SetCommand>(subLine->commands[0])
            : nullptr;
    const auto* target =
        glob != nullptr && glob->assigns.size() == 1
            ? as<mumps::ast::Variable>(glob->assigns[0]->targets[0])
            : nullptr;
    if (target == nullptr || !target->global || target->name != "glb" ||
        target->subscripts.size() != 2)
      return fail("coverage.m: SET ^glb(1,2)=... did not read back");

    const auto* concatLine = as<mumps::ast::CodeLine>(doc.lines[25]);
    const auto* concatSet =
        concatLine != nullptr && concatLine->commands.size() == 1
            ? as<mumps::ast::SetCommand>(concatLine->commands[0])
            : nullptr;
    const auto* concatExpr =
        concatSet != nullptr
            ? as<mumps::ast::BinaryExpression>(concatSet->assigns[0]->value)
            : nullptr;
    if (concatExpr == nullptr ||
        concatExpr->op != mumps::ast::Operator::concat)
      return fail("coverage.m: SET r=a_b_c did not read back");

    const auto* fnLine = as<mumps::ast::CodeLine>(doc.lines[26]);
    const auto* fnSet =
        fnLine != nullptr && fnLine->commands.size() == 1
            ? as<mumps::ast::SetCommand>(fnLine->commands[0])
            : nullptr;
    const auto* fn =
        fnSet != nullptr
            ? as<mumps::ast::FunctionCall>(fnSet->assigns[0]->value)
            : nullptr;
    const auto* strArg =
        fn != nullptr && !fn->args.empty()
            ? as<mumps::ast::StringArg>(fn->args[0])
            : nullptr;
    if (strArg == nullptr || strArg->value != "abcde")
      return fail("coverage.m: $extract(\"abcde\",...) did not read back");

    const auto* doLine = as<mumps::ast::CodeLine>(doc.lines[27]);
    const auto* doCmd =
        doLine != nullptr && doLine->commands.size() == 1
            ? as<mumps::ast::DoCommand>(doLine->commands[0])
            : nullptr;
    if (doCmd == nullptr || doCmd->name != "sub" || doCmd->params.size() != 2)
      return fail("coverage.m: DO sub(a,b) did not read back");

    const auto* nestedLine = as<mumps::ast::CodeLine>(doc.lines[28]);
    if (nestedLine == nullptr || nestedLine->commands.size() != 1 ||
        as<mumps::ast::SetCommand>(nestedLine->commands[0]) == nullptr)
      return fail("coverage.m: the dot-nested SET did not read back");
  }

  // No precedence, strictly left to right (ANSI X11.1-1995): 10-5-2 groups
  // (10-5)-2.
  {
    const cursiv::ParseResult result =
        language.language.parser->parse(" SET X=10-5-2\n");
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("10-5-2 does not parse clean");
    const auto& doc = *static_cast<const mumps::ast::Document*>(result.value);
    const auto* line = as<mumps::ast::CodeLine>(doc.lines[0]);
    const auto* set =
        line != nullptr && line->commands.size() == 1
            ? as<mumps::ast::SetCommand>(line->commands[0])
            : nullptr;
    const auto* outer =
        set != nullptr && set->assigns.size() == 1
            ? as<mumps::ast::BinaryExpression>(set->assigns[0]->value)
            : nullptr;
    const auto* inner =
        outer != nullptr ? as<mumps::ast::BinaryExpression>(outer->left)
                          : nullptr;
    if (outer == nullptr || outer->op != mumps::ast::Operator::subtract ||
        inner == nullptr || inner->op != mumps::ast::Operator::subtract ||
        as<mumps::ast::NumberLiteral>(outer->right) == nullptr)
      return fail("10-5-2 did not group (10-5)-2");
  }

  // `FIBONACCI ; ...` is a line, label then line start (GT.M Programmer's
  // Guide, "Routines"; the .g4's `line` tries `code` before `routinedecl`);
  // only a name with nothing after it, no line start, is a routine header.
  {
    const cursiv::ParseResult result =
        language.language.parser->parse("NAME\n QUIT\nLOOP \n QUIT\n");
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("NAME / LOOP does not parse clean");
    const auto& doc = *static_cast<const mumps::ast::Document*>(result.value);
    if (doc.lines.size() != 4) return fail("NAME / LOOP: wrong line count");
    const auto* header = as<mumps::ast::RoutineDecl>(doc.lines[0]);
    if (header == nullptr || header->name != "NAME")
      return fail("a bare name did not read as a routine header");
    const auto* label = as<mumps::ast::CodeLine>(doc.lines[2]);
    if (label == nullptr || label->label != "LOOP" || !label->commands.empty())
      return fail("a name and a line start did not read as a label line");
  }

  std::puts("mumps: the corpus parses");
  return 0;
}
