#include "audit_regressions.hpp"
// The grammar end to end: every document under example/ parses clean (no
// recovery, no diagnostics, a Module root), and one document's AST is read
// back field by field.

#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "vba/core/CoreServices.hpp"
#include "vba/core/ast.hpp"

namespace {

namespace fs = std::filesystem;

int failures = 0;

void fail(const std::string& what) {
  std::fprintf(stderr, "vba: %s\n", what.c_str());
  ++failures;
}

std::string read_file(const fs::path& path) {
  std::ifstream file(path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
}

using Language = decltype(cursiv::create_language<
                           vba::VbaCoreServices, vba::VbaSharedCoreServices,
                           vba::install_core_module>());

const vba::ast::Module* parse_module(const Language& language,
                                     const fs::path& path,
                                     cursiv::ParseResult* out) {
  const std::string text = read_file(path);
  *out = language.language.parser->parse(text);
  if (out->cst().recovered()) {
    fail(path.string() + ": the document does not parse clean");
    return nullptr;
  }
  if (!out->diagnostics.empty()) {
    fail(path.string() + ": the document has diagnostics");
    return nullptr;
  }
  const cursiv::AstNode* const root = out->value;
  if (root == nullptr || root->type != &cursiv::ast_type_tag<vba::ast::Module>) {
    fail(path.string() + ": the parse built no Module");
    return nullptr;
  }
  return static_cast<const vba::ast::Module*>(root);
}

void corpus(const Language& language) {
  const fs::path dir = CURSIV_EXAMPLE_DIR;
  int count = 0;
  for (const fs::directory_entry& e : fs::recursive_directory_iterator(dir)) {
    if (!e.is_regular_file()) continue;
    const fs::path& path = e.path();
    const std::string ext = path.extension().string();
    if (ext != ".bas" && ext != ".cls" && ext != ".frm") continue;
    ++count;
    cursiv::ParseResult result;
    parse_module(language, path, &result);
  }
  if (count == 0) fail("no corpus document was found under " + dir.string());
}

// The constructs no upstream corpus example exercises: also
// required to parse clean.
void own_examples(const Language& language) {
  const fs::path dir = fs::path(CURSIV_TEST_DIR) / "own";
  int count = 0;
  for (const fs::directory_entry& e : fs::recursive_directory_iterator(dir)) {
    if (!e.is_regular_file()) continue;
    ++count;
    cursiv::ParseResult result;
    parse_module(language, e.path(), &result);
  }
  if (count == 0) fail("no own-written document was found under " + dir.string());
}

// Field-level readback: SQLHelperFunctions.bas declares four public
// functions, the first being `toUnix`, whose body assigns to its own name
// and calls `DateDiff` with three string/name arguments.
void field_readback(const Language& language) {
  const fs::path path =
      fs::path(CURSIV_EXAMPLE_DIR) / "SQLHelperFunctions.bas";
  cursiv::ParseResult result;
  const vba::ast::Module* const module = parse_module(language, path, &result);
  if (module == nullptr) {
    fail("field_readback: the sample itself failed to parse");
    return;
  }
  const vba::ast::Attribute* attr = nullptr;
  const vba::ast::Procedure* firstProc = nullptr;
  for (vba::ast::Item* item : module->body) {
    if (attr == nullptr && item->type == &cursiv::ast_type_tag<vba::ast::Attribute>)
      attr = static_cast<const vba::ast::Attribute*>(item);
    if (firstProc == nullptr &&
        item->type == &cursiv::ast_type_tag<vba::ast::Procedure>)
      firstProc = static_cast<const vba::ast::Procedure*>(item);
  }
  if (attr == nullptr || attr->value != "SQLHelperFunctions")
    fail("field_readback: the module's VB_Name attribute was not read back");
  if (firstProc == nullptr || firstProc->name != "toUnix" ||
      firstProc->kind != "Function")
    fail("field_readback: the first procedure was not read back as toUnix");
  if (firstProc != nullptr) {
    bool sawAssign = false;
    for (vba::ast::Item* stmt : firstProc->body) {
      if (stmt->type != &cursiv::ast_type_tag<vba::ast::AssignStmt>) continue;
      const auto* const assign = static_cast<const vba::ast::AssignStmt*>(stmt);
      const auto* const call =
          assign->value->type == &cursiv::ast_type_tag<vba::ast::IndexExpr>
              ? static_cast<const vba::ast::IndexExpr*>(assign->value)
              : nullptr;
      if (call == nullptr) continue;
      const auto* const callee =
          call->target->type == &cursiv::ast_type_tag<vba::ast::NameExpr>
              ? static_cast<const vba::ast::NameExpr*>(call->target)
              : nullptr;
      if (callee != nullptr && callee->name == "DateDiff" &&
          call->args.size() == 3) {
        sawAssign = true;
      }
    }
    if (!sawAssign)
      fail("field_readback: toUnix's DateDiff(...) call was not read back");
  }
}

template <class T>
const T* as(const cursiv::AstNode* n) {
  return n != nullptr && n->type == &cursiv::ast_type_tag<T> ?
             static_cast<const T*>(n) : nullptr;
}

// Wraps `rhs` as a Sub's sole assignment and returns its value.
const vba::ast::Expr* single_expr(const Language& language,
                                  const std::string& rhs) {
  const std::string text = "Attribute VB_Name = \"T\"\nSub P()\nv = " + rhs +
                           "\nEnd Sub\n";
  const cursiv::ParseResult result = language.language.parser->parse(text);
  if (result.cst().recovered() || !result.diagnostics.empty()) {
    fail("'" + rhs + "': does not parse clean");
    return nullptr;
  }
  const auto* const module =
      as<vba::ast::Module>(static_cast<const cursiv::AstNode*>(result.value));
  if (module == nullptr) {
    fail("'" + rhs + "': expected a Module");
    return nullptr;
  }
  const vba::ast::Procedure* proc = nullptr;
  for (const vba::ast::Item* item : module->body)
    if (proc == nullptr) proc = as<vba::ast::Procedure>(item);
  if (proc == nullptr || proc->body.size() != 1) {
    fail("'" + rhs + "': expected one statement");
    return nullptr;
  }
  const auto* const assign = as<vba::ast::AssignStmt>(proc->body[0]);
  if (assign == nullptr) {
    fail("'" + rhs + "': expected an AssignStmt");
    return nullptr;
  }
  return assign->value;
}

const vba::ast::BinaryExpr* bin(const vba::ast::Expr* e, std::string_view op) {
  const auto* const b = as<vba::ast::BinaryExpr>(e);
  return (b != nullptr && b->op == op) ? b : nullptr;
}

const vba::ast::UnaryExpr* un(const vba::ast::Expr* e, std::string_view op) {
  const auto* const u = as<vba::ast::UnaryExpr>(e);
  return (u != nullptr && u->op == op) ? u : nullptr;
}

// The operator table: `*`/`/`, `\`, `Mod`, `+`/`-` each their
// own level; negation between `^` and `*`/`/`, `^` left-associative and
// still reachable as a negated right operand; `Not` between Comparison and
// `And`; `And`/`Or`/`Xor`/`Eqv`/`Imp` each their own level.
void precedence_probes(const Language& language) {
  const auto* const negPow = un(single_expr(language, "-2 ^ 2"), "-");
  if (negPow == nullptr || bin(negPow->operand, "^") == nullptr)
    fail("'-2^2': expected -(2^2)");

  const auto* const powNeg = bin(single_expr(language, "2 ^ -2"), "^");
  if (powNeg == nullptr || un(powNeg->right, "-") == nullptr)
    fail("'2^-2': expected 2^(-2)");

  const auto* const powChain = bin(single_expr(language, "2 ^ 3 ^ 2"), "^");
  if (powChain == nullptr || bin(powChain->left, "^") == nullptr)
    fail("'2^3^2': expected left-associative (2^3)^2");

  const auto* const divInt = bin(single_expr(language, "10 / 4 \\ 2"), "\\");
  if (divInt == nullptr || bin(divInt->left, "/") == nullptr)
    fail("'10/4\\2': expected (10/4)\\2, / tighter than \\");

  const auto* const intDiv = bin(single_expr(language, "10 \\ 4 / 2"), "\\");
  if (intDiv == nullptr || bin(intDiv->right, "/") == nullptr)
    fail("'10\\4/2': expected 10\\(4/2), / tighter than \\");

  const auto* const notCmp = un(single_expr(language, "Not a = b"), "Not");
  if (notCmp == nullptr || bin(notCmp->operand, "=") == nullptr)
    fail("'Not a = b': expected Not (a = b)");

  const auto* const andNot = bin(single_expr(language, "a And Not b"), "And");
  if (andNot == nullptr || un(andNot->right, "Not") == nullptr)
    fail("'a And Not b': expected a And (Not b)");

  const auto* const andOr = bin(single_expr(language, "a And b Or c"), "Or");
  if (andOr == nullptr || bin(andOr->left, "And") == nullptr)
    fail("'a And b Or c': expected (a And b) Or c");
}

// Parses `lines` as a Sub's body and hands it to `check`; fails with `what`
// when the document needs repair or `check` refuses the body.
template <class Check>
void sub_probe(const Language& language, const std::string& lines,
               const char* what, Check check) {
  const std::string text =
      "Attribute VB_Name = \"T\"\nSub P()\n" + lines + "\nEnd Sub\n";
  const cursiv::ParseResult result = language.language.parser->parse(text);
  const auto* const module =
      result.cst().recovered() || !result.diagnostics.empty()
          ? nullptr
          : as<vba::ast::Module>(
                static_cast<const cursiv::AstNode*>(result.value));
  const vba::ast::Procedure* proc = nullptr;
  if (module != nullptr)
    for (const vba::ast::Item* item : module->body)
      if (proc == nullptr) proc = as<vba::ast::Procedure>(item);
  if (proc == nullptr || !check(proc->body)) fail("'" + lines + "': " + what);
}

using Body = decltype(vba::ast::Procedure::body);

// MS-VBAL 5.4.2.9 `list-or-label`: a `:` may open the statements after
// `Then` or `Else`, and they stay the single-line `If`'s.
void single_line_if_probes(const Language& language) {
  struct Case {
    const char* line;
    std::size_t thenCount;
    std::size_t elseCount;
  };
  for (const Case& c : {Case{"If a < 3 Then: Foo = 3", 1, 0},
                        Case{"If a Then b = 1 Else: c = 2: d = 3", 1, 2}}) {
    sub_probe(
        language, c.line, "the statements after Then/Else are not the If's",
        [&](const Body& body) {
          const auto* const stmt =
              body.size() == 1 ? as<vba::ast::IfStmt>(body[0]) : nullptr;
          return stmt != nullptr && stmt->thenBody.size() == c.thenCount &&
                 stmt->elseBody.size() == c.elseCount;
        });
  }
}

// A reserved word closes or parts its block (MS-VBAL `statement-block`,
// `for-statement`, `select-case-statement`), and a line number is a label
// (`statement-label-definition`), never a call.
void block_probes(const Language& language) {
  sub_probe(language, "For i = 1 To 3: If a Then b = 1: Next",
            "the For does not close on its Next", [](const Body& body) {
              const auto* const loop =
                  body.size() == 1 ? as<vba::ast::ForStmt>(body[0]) : nullptr;
              return loop != nullptr && loop->closed && loop->body.size() == 1;
            });
  sub_probe(language, "Select Case a\nCase 1: c = 1\nCase 2: d = 1\nEnd Select",
            "a Case is read as a call", [](const Body& body) {
              const auto* const select =
                  body.size() == 1 ? as<vba::ast::SelectCaseStmt>(body[0])
                                   : nullptr;
              return select != nullptr && select->cases.size() == 2;
            });
  sub_probe(
      language,
      "Select Case a\nCase 1: If b Then c = 1: Case 2: d = 1\nEnd Select",
      "a Case after a one-line If is read as a call", [](const Body& body) {
        const auto* const select =
            body.size() == 1 ? as<vba::ast::SelectCaseStmt>(body[0]) : nullptr;
        return select != nullptr && select->cases.size() == 2;
      });
  sub_probe(language, "10: b = 1\n20 c = 2",
            "a line number is not read as a label", [](const Body& body) {
              return body.size() == 4 &&
                     as<vba::ast::LabelStmt>(body[0]) != nullptr &&
                     as<vba::ast::LabelStmt>(body[2]) != nullptr;
            });
  // MS-VBAL 5.4.1: a name and its `:` opening a line are always a label;
  // after a `:` the name is a call.
  sub_probe(language, "  Foo: b = 1\nx = 1: Bar: y = 2\n[a b]:\nElse_:",
            "a label opening a line is read as a call", [](const Body& body) {
              const auto* const foo =
                  body.size() == 7 ? as<vba::ast::LabelStmt>(body[0]) : nullptr;
              const auto* const odd =
                  foo != nullptr ? as<vba::ast::LabelStmt>(body[5]) : nullptr;
              return odd != nullptr && foo->name == "Foo" &&
                     as<vba::ast::CallStmt>(body[3]) != nullptr &&
                     odd->name == "[a b]" &&
                     as<vba::ast::LabelStmt>(body[6]) != nullptr;
            });
}

// A word that closes or opens a statement is no name (MS-VBAL 3.3.5.2): it
// closes its block after a one-line `If`, and `Input` opens its statement,
// never a call; the other reserved words are plain names. An attribute is a
// statement of a block too (MS-VBAL 5.4.1).
void reserved_word_probes(const Language& language) {
  sub_probe(language,
            "Do: If a Then b = 1: Loop\nWhile a: If b Then c = 1: Wend",
            "a Loop or a Wend is read as a call", [](const Body& body) {
              const auto* const loop =
                  body.size() == 2 ? as<vba::ast::DoStmt>(body[0]) : nullptr;
              const auto* const wend =
                  loop != nullptr ? as<vba::ast::WhileStmt>(body[1]) : nullptr;
              return wend != nullptr && loop->body.size() == 1 &&
                     wend->body.size() == 1;
            });
  sub_probe(language, "Input #1, a, b\nAttribute P.VB_Description = \"d\"",
            "Input or an attribute is read as a call", [](const Body& body) {
              return body.size() == 2 &&
                     as<vba::ast::InputStmt>(body[0]) != nullptr &&
                     as<vba::ast::Attribute>(body[1]) != nullptr;
            });
  for (const char* word : {"Next", "InputB", "GoTo", "GoSub"})
    if (language.language.parser
            ->parse(std::string("Dim ") + word + " As Long\n")
            .diagnostics.empty())
      fail(std::string("'Dim ") + word + "': a reserved word is a name");
  for (const char* word : {"Int", "LenB", "CDecl", "LINEINPUT"})
    if (!language.language.parser
             ->parse(std::string("Dim ") + word + " As Long\n")
             .diagnostics.empty())
      fail(std::string("'Dim ") + word + "': a plain word is refused");
  sub_probe(language, "Randomize:\nx = 1", "`Randomize:` opens a label",
            [](const Body& body) {
              return body.size() == 2 &&
                     as<vba::ast::CallStmt>(body[0]) != nullptr;
            });
}

// A type suffix is glued to its name (MS-VBAL 3.3.5.3 `TYPED-NAME`): never
// across a blank, and a `!` glued on both sides is a dictionary access.
void type_suffix_probes(const Language& language) {
  sub_probe(language, "rs!Name = \"a\"\nWidth #1, 80\nDebug.Print D & E",
            "a suffix is read across a blank or on a dictionary `!`",
            [](const Body& body) {
              const auto* const assign =
                  body.size() == 3 ? as<vba::ast::AssignStmt>(body[0])
                                   : nullptr;
              const auto* const print =
                  assign != nullptr ? as<vba::ast::PrintStmt>(body[2])
                                    : nullptr;
              return print != nullptr &&
                     as<vba::ast::DictionaryAccessExpr>(assign->target) !=
                         nullptr &&
                     as<vba::ast::WidthStmt>(body[1]) != nullptr &&
                     print->items.size() == 1;
            });
}

// A codepage letter spells a name (MS-VBAL 3.3.5 `codepage-identifier`):
// `ifé` is no `If`, and `Déjà!` keeps its suffix.
void codepage_name_probes(const Language& language) {
  sub_probe(language, "ifé = Déjà!\nÉtape:",
            "an accented name is not read as one", [](const Body& body) {
              const auto* const assign =
                  body.size() == 2 ? as<vba::ast::AssignStmt>(body[0])
                                   : nullptr;
              const auto* const name =
                  assign != nullptr ? as<vba::ast::NameExpr>(assign->value)
                                    : nullptr;
              return name != nullptr && name->name == "Déjà" &&
                     name->typeChar == "!" &&
                     as<vba::ast::LabelStmt>(body[1]) != nullptr;
            });
}

// MS-VBAL 5.6.16.8: `argument-list = [positional-or-named-argument-list]`,
// so `f()` holds no argument; an omitted slot needs a comma beside it.
void argument_list_probes(const Language& language) {
  const auto indexArgs = [&](const std::string& rhs) -> int {
    const auto* const index = as<vba::ast::IndexExpr>(single_expr(language, rhs));
    return index != nullptr ? static_cast<int>(index->args.size()) : -1;
  };
  if (indexArgs("f()") != 0) fail("'f()': expected no argument");
  if (indexArgs("f(1)") != 1) fail("'f(1)': expected one argument");
  if (indexArgs("f(, 1)") != 2) fail("'f(, 1)': expected two arguments");

  sub_probe(language, "Foo", "a bare call holds no argument",
            [](const Body& body) {
              const auto* const call =
                  body.size() == 1 ? as<vba::ast::CallStmt>(body[0]) : nullptr;
              return call != nullptr && call->args.empty();
            });
  sub_probe(language, "Foo()", "a call with empty parentheses holds none",
            [](const Body& body) {
              const auto* const call =
                  body.size() == 1 ? as<vba::ast::CallStmt>(body[0]) : nullptr;
              const auto* const target =
                  call != nullptr ? as<vba::ast::IndexExpr>(call->target)
                                  : nullptr;
              return target != nullptr && target->args.empty() &&
                     call->args.empty();
            });
  sub_probe(language, "Foo , 2", "an omitted first argument is kept",
            [](const Body& body) {
              const auto* const call =
                  body.size() == 1 ? as<vba::ast::CallStmt>(body[0]) : nullptr;
              return call != nullptr && call->args.size() == 2 &&
                     call->args[0]->value == nullptr;
            });
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  const auto language = cursiv::create_language<
      vba::VbaCoreServices, vba::VbaSharedCoreServices,
      vba::install_core_module>();

  corpus(language);
  own_examples(language);
  field_readback(language);
  precedence_probes(language);
  single_line_if_probes(language);
  block_probes(language);
  reserved_word_probes(language);
  type_suffix_probes(language);
  codepage_name_probes(language);
  argument_list_probes(language);

  if (failures != 0) {
    std::fprintf(stderr, "vba: %d check(s) failed\n", failures);
    return 1;
  }
  std::puts("vba: the corpus parses");
  return 0;
}
