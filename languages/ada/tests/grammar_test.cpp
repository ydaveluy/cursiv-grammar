// The smoke test of the language: every document under example/ parses
// clean -- no repair, no diagnostics -- and builds a Compilation root.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "ada/core/CoreServices.hpp"
#include "ada/core/ast.hpp"
#include "cursiv/test/ValidationHelper.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const fs::path& file, const char* what) {
  std::fprintf(stderr, "ada: %s: %s\n", file.string().c_str(), what);
  return 1;
}

template <class T>
const T* as(const cursiv::AstNode* node) {
  if (node == nullptr || node->type != &cursiv::ast_type_tag<T>) return nullptr;
  return static_cast<const T*>(node);
}

// The one statement of a `procedure P is begin ... end P;` document, or null
// if the shape does not match.
const ada::ast::Stmt* only_stmt(const cursiv::ParseResult& result) {
  const auto* compilation = as<ada::ast::Compilation>(result.value);
  if (compilation == nullptr || compilation->units.size() != 1) return nullptr;
  const auto* body = as<ada::ast::SubprogramBody>(compilation->units[0]->body);
  if (body == nullptr || body->body == nullptr || body->body->body.size() != 1)
    return nullptr;
  return body->body->body[0];
}

// The value expression of that statement, an assignment, or null.
const ada::ast::Expr* assigned_value(const cursiv::ParseResult& result) {
  const auto* assign = as<ada::ast::AssignStmt>(only_stmt(result));
  return assign == nullptr ? nullptr : assign->value;
}

// The `for` specification of that statement, a loop, or null.
const ada::ast::ForSpec* loop_spec(const cursiv::ParseResult& result) {
  const auto* loop = as<ada::ast::LoopStmt>(only_stmt(result));
  return loop == nullptr ? nullptr : loop->forSpec;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      ada::AdaCoreServices,
      ada::AdaSharedCoreServices,
      ada::install_core_module>();

  // RM 2.1/2.2: separator spaces and format effectors are Unicode-aware;
  // every format effector other than HT ends a comment.
  for (const char* ending : {"\v", "\f", "\xc2\x85", "\xe2\x80\xa8",
                             "\xe2\x80\xa9"}) {
    const std::string source =
        std::string("procedure\xc2\xa0P is begin -- comment") + ending +
        "null; end P;";
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !as<ada::ast::NullStmt>(only_stmt(result)))
      return fail("(native separators)", "a comment swallowed the next statement");
  }
  {
    const auto result = language.language.parser->parse(
        "package Payload is "
        "type Ref is access all Integer; "
        "type R is abstract tagged null record; "
        "type Hidden (<>) is abstract tagged limited private; "
        "procedure P(X : not null Ref); "
        "function F return not null Ref; "
        "type Callback is access function return not null Ref; "
        "generic Ptr : not null Ref; type Money is delta <> digits <>; "
        "type Plain is delta <>; "
        "type Sync is synchronized new Root with private; "
        "with package AllArgs is new G(others => <>); "
        "package Gen is end Gen; end Payload;");
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty())
      return fail("(native declarations)", "a native declaration form was refused");
    bool all = false, abstractRecord = false, privateType = false;
    bool unknown = false, parameter = false, returnType = false;
    bool callback = false, formalObject = false, decimal = false;
    bool ordinary = false, synchronized = false, others = false;
    for (const auto* node : result.nodes) {
      if (const auto* value = as<ada::ast::AccessDef>(node)) {
        all |= value->all_ && value->mark == "Integer";
        callback |= value->isFunction && value->returnNullExclusion;
      }
      if (const auto* value = as<ada::ast::RecordTypeDef>(node))
        abstractRecord |= value->abstract_ && value->tagged_;
      if (const auto* value = as<ada::ast::PrivateTypeDef>(node))
        privateType |= value->abstract_ && value->tagged_ && value->limited_;
      if (const auto* value = as<ada::ast::TypeDecl>(node))
        unknown |= value->name == "Hidden" && value->unknownDiscriminants;
      if (const auto* value = as<ada::ast::Param>(node))
        parameter |= value->names.size() == 1 && value->names[0] == "X" &&
                     value->nullExclusion;
      if (const auto* value = as<ada::ast::SubprogramSpec>(node))
        returnType |= value->name == "F" && value->returnNullExclusion;
      if (const auto* value = as<ada::ast::ObjectDecl>(node))
        formalObject |= value->names.size() == 1 && value->names[0] == "Ptr" &&
                        value->nullExclusion;
      if (const auto* value = as<ada::ast::FormalTypeDef>(node)) {
        decimal |= value->kind == "delta" && value->decimalFixed;
        ordinary |= value->kind == "delta" && !value->decimalFixed;
      }
      if (const auto* value = as<ada::ast::FormalDerivedDef>(node))
        synchronized |= value->synchronized_;
      if (const auto* value = as<ada::ast::GenericFormalPackage>(node))
        others |= value->isBox && value->othersBox;
    }
    if (!all || !abstractRecord || !privateType || !unknown || !parameter ||
        !returnType || !callback || !formalObject || !decimal || !ordinary ||
        !synchronized || !others)
      return fail("(native declaration payload)", "an Ada qualifier was lost");
  }
  {
    const auto result = language.language.parser->parse(
        "procedure Payload is "
        "task T is entry E; end T; "
        "task body T is begin accept E do null; end E; end T; "
        "type Pair is record A, B : Integer; end record; "
        "protected type Obj is entry E; procedure M; "
        "function Make return Pair; end Obj; "
        "protected body Obj is entry E when True is begin null; end E; "
        "procedure M is null; function Make return Pair is (A => 1, B => 2); "
        "end Obj; "
        "begin X := (for all I in reverse 1 .. 2 => I > 0); "
        "X := (for some I of reverse Container => True); "
        "L: declare begin null; end L; <<Done>> end Payload;");
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty())
      return fail("(native statement payload)", "native statements did not parse");
    bool inReverse = false, ofReverse = false, block = false, accept = false;
    bool entry = false, task = false, protectedSpec = false, protectedBody = false;
    bool endLabel = false, nullProcedure = false, expressionFunction = false;
    for (const auto* node : result.nodes) {
      if (const auto* value = as<ada::ast::QuantifiedExpr>(node)) {
        inReverse |= value->var && value->var->reverse_ && !value->var->isOf;
        ofReverse |= value->var && value->var->reverse_ && value->var->isOf;
      }
      if (const auto* value = as<ada::ast::BlockStmt>(node))
        block |= value->endName == "L";
      if (const auto* value = as<ada::ast::AcceptStmt>(node))
        accept |= value->endName == "E";
      if (const auto* value = as<ada::ast::EntryBody>(node))
        entry |= value->endName == "E";
      if (const auto* value = as<ada::ast::TaskSpec>(node))
        task |= value->endName == "T";
      if (const auto* value = as<ada::ast::ProtectedSpec>(node))
        protectedSpec |= value->endName == "Obj";
      if (const auto* value = as<ada::ast::ProtectedBody>(node))
        protectedBody |= value->endName == "Obj";
      if (const auto* value = as<ada::ast::LabeledStmt>(node))
        endLabel |= value->labels.size() == 1 && value->labels[0] == "Done" &&
                    value->inner == nullptr;
      if (const auto* value = as<ada::ast::SubprogramDecl>(node)) {
        nullProcedure |= value->isNullProc;
        expressionFunction |= as<ada::ast::Aggregate>(value->exprBody) != nullptr;
      }
    }
    if (!inReverse || !ofReverse || !block || !accept || !entry || !task ||
        !protectedSpec || !protectedBody || !endLabel || !nullProcedure ||
        !expressionFunction)
      return fail("(native statement payload)", "an Ada statement field was lost");
  }

  int failures = 0;
  int checked = 0;

  // hello.ada, pkg1_body.ada and the semtest_* files live beside this test,
  // not in the corpus.
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
  for (const fs::directory_entry& entry :
       fs::recursive_directory_iterator(dir)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".ada")
      continue;
    ++checked;

    std::ifstream file(entry.path());
    if (!file) {
      failures += fail(entry.path(), "the document is not readable");
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      failures += fail(entry.path(), "the document does not parse clean");
      continue;
    }
    if (!result.diagnostics.empty()) {
      failures += fail(entry.path(), "the document carries diagnostics");
      continue;
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<ada::ast::Compilation>) {
      failures += fail(entry.path(), "the parse built no compilation");
      continue;
    }
  }

  if (checked == 0) {
    std::fprintf(stderr, "ada: no .ada file found under %s\n",
                 CURSIV_EXAMPLE_DIR);
    return 1;
  }
  if (failures > 0) {
    std::fprintf(stderr, "ada: %d of %d documents failed\n", failures,
                 checked);
    return 1;
  }

  // Aspects and overriding indicators stay attached to each declaration.
  {
    const auto result = language.language.parser->parse(
        "package Café is "
        "procedure P with Pre => X > 0, Post => X > 1; "
        "overriding procedure A; not overriding procedure B; "
        "not overriding procedure R renames X; "
        "not overriding procedure S is separate; "
        "not overriding procedure I is new G; "
        "task T is not overriding entry E; end T; end Café;");
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty())
      return fail("(declaration payload)", "Unicode or declaration syntax failed");
    bool aspects = false, positive = false, negative = false;
    bool rename = false, stub = false, instance = false, entry = false;
    for (const auto* node : result.nodes) {
      if (const auto* decl = as<ada::ast::SubprogramDecl>(node)) {
        if (decl->aspects.size() == 2 && decl->aspects[0]->name == "Pre" &&
            decl->aspects[1]->name == "Post" && decl->aspects[0]->value &&
            decl->aspects[1]->value)
          aspects = true;
      }
      if (const auto* spec = as<ada::ast::SubprogramSpec>(node)) {
        positive |= spec->name == "A" && spec->overriding_ && !spec->notOverriding;
        negative |= spec->name == "B" && spec->overriding_ && spec->notOverriding;
        rename |= spec->name == "R" && spec->notOverriding;
        stub |= spec->name == "S" && spec->notOverriding;
      }
      if (const auto* decl = as<ada::ast::GenericInstantiation>(node))
        instance |= decl->name == "I" && decl->notOverriding;
      if (const auto* decl = as<ada::ast::EntryDecl>(node))
        entry |= decl->name == "E" && decl->notOverriding;
    }
    if (!aspects || !positive || !negative || !rename || !stub || !instance || !entry)
      return fail("(declaration payload)", "aspect or overriding information was lost");
  }

  // RM 4.4: `and`/`or`/`xor` never mix without parentheses.
  {
    const std::string chain =
        "procedure P is A, B, C, R : Boolean; begin R := A and B and C; "
        "end P;";
    const cursiv::ParseResult result = language.language.parser->parse(chain);
    const auto* outer = as<ada::ast::BinaryExpr>(assigned_value(result));
    if (result.cst().recovered() || !result.diagnostics.empty() ||
        outer == nullptr || outer->op != ada::ast::Operator::and_)
      return fail("(same-operator chain)", "'A and B and C' did not parse as an and-chain");
    const auto* inner = as<ada::ast::BinaryExpr>(outer->left);
    if (inner == nullptr || inner->op != ada::ast::Operator::and_)
      return fail("(same-operator chain)", "'A and B and C' is not left-associative");

    const std::string mixed =
        "procedure P is A, B, C, R : Boolean; begin R := A and B or C; "
        "end P;";
    const cursiv::ParseResult mixedResult =
        language.language.parser->parse(mixed);
    const auto mixedValidation =
        cursiv::test::validationHelper(language.language, mixed);
    if (mixedResult.cst().recovered() || !mixedResult.diagnostics.empty() ||
        mixedValidation.diagnostics.size() != 1)
      return fail("(mixed operators)",
                  "'A and B or C' should parse and receive one runtime diagnostic");

    const std::string parenMixed =
        "procedure P is A, B, C, R : Boolean; begin R := (A and B) or C; "
        "end P;";
    const cursiv::ParseResult parenResult =
        language.language.parser->parse(parenMixed);
    if (parenResult.cst().recovered() || !parenResult.diagnostics.empty() ||
        !cursiv::test::validationHelper(language.language, parenMixed)
             .diagnostics.empty())
      return fail("(parenthesized mix)", "'(A and B) or C' does not parse clean");
    const auto rightGrouped = cursiv::test::validationHelper(language.language,
        "procedure P is begin R := A and (B or C); end P;");
    if (!rightGrouped.diagnostics.empty())
      return fail("(parenthesized mix)", "A and (B or C) received a diagnostic");
  }

  // RM 4.3.1(7), 4.3.3(3): one positional component is a parenthesized
  // expression; a bare name is an expression in a choice, a subtype in a
  // loop, where `A'Range` is a range (RM 4.1.4) and `F (X)` an iterator.
  {
    const auto parse = [&](std::string_view text) {
      cursiv::ParseResult result = language.language.parser->parse(text);
      if (result.cst().recovered() || !result.diagnostics.empty())
        result.value = nullptr;
      return result;
    };

    const auto qualified = parse("procedure P is begin S := T'(42); end P;");
    const auto* q = as<ada::ast::QualifiedExpr>(assigned_value(qualified));
    if (q == nullptr || as<ada::ast::NumericLit>(q->value) == nullptr)
      return fail("(qualified)", "T'(42) did not qualify a parenthesized 42");

    const auto paren = parse("procedure P is begin S := (42); end P;");
    const auto* grouped = as<ada::ast::GroupedExpr>(assigned_value(paren));
    if (grouped == nullptr || as<ada::ast::NumericLit>(grouped->value) == nullptr)
      return fail("(parenthesized)", "(42) did not preserve its grouping");

    const auto record =
        parse("procedure P is begin R := (X => 10, Y => 20); end P;");
    const auto* agg = as<ada::ast::Aggregate>(assigned_value(record));
    if (agg == nullptr || agg->items.size() != 2 ||
        agg->items[0]->tests.size() != 1 ||
        as<ada::ast::Ident>(agg->items[0]->tests[0]) == nullptr)
      return fail("(record aggregate)", "X in (X => 10, Y => 20) is no name");

    const auto choice = parse(
        "procedure P is begin case C is when Red => null; "
        "when others => null; end case; end P;");
    const auto* cs = as<ada::ast::CaseStmt>(only_stmt(choice));
    if (cs == nullptr || cs->alts.size() != 2 ||
        cs->alts[0]->choices.size() != 1 ||
        as<ada::ast::Ident>(cs->alts[0]->choices[0]) == nullptr)
      return fail("(case choice)", "when Red => did not read Red as a name");

    const auto subtype =
        parse("procedure P is begin for I in Integer loop null; end loop; end P;");
    const auto* sub = loop_spec(subtype);
    if (sub == nullptr || sub->isIterator ||
        as<ada::ast::SubtypeIndication>(sub->range) == nullptr)
      return fail("(loop subtype)", "for I in Integer did not read a subtype");

    const auto range =
        parse("procedure P is begin for I in A'Range loop null; end loop; end P;");
    const auto* rs = loop_spec(range);
    if (rs == nullptr || rs->isIterator ||
        as<ada::ast::AttrExpr>(rs->range) == nullptr)
      return fail("(loop range)", "for I in A'Range did not read a range");

    const auto iterator = parse(
        "procedure P is begin for E in Iterate (C) loop null; end loop; end P;");
    const auto* it = loop_spec(iterator);
    if (it == nullptr || !it->isIterator ||
        as<ada::ast::CallExpr>(it->range) == nullptr)
      return fail("(loop iterator)", "for E in Iterate (C) did not read an iterator");

    const auto of_loop = parse(
        "procedure P is begin for E of C loop null; end loop; end P;");
    const auto* of = loop_spec(of_loop);
    if (of == nullptr || !of->isOf || !of->isIterator ||
        as<ada::ast::Ident>(of->range) == nullptr)
      return fail("(loop of)", "for E of C is not an iterator over a container");
  }

  std::printf("ada: %d documents parse clean\n", checked);
  return 0;
}
