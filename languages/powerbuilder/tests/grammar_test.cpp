// The smoke test of the language: build the container the way a workspace
// does, read every document under example/, and check each tree. One file's
// values are read back too, so a converter that silently yields the wrong
// value cannot pass unnoticed.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "powerbuilder/core/CoreServices.hpp"
#include "powerbuilder/core/ast.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"

namespace {

int fail(const std::string& path, const char* what) {
  std::fprintf(stderr, "powerbuilder: %s: %s\n", path.c_str(), what);
  return 1;
}

bool is_source_export(const std::filesystem::path& p) {
  static const char* const kExts[] = {".sra", ".srf", ".srm", ".srq",
                                      ".srs", ".sru", ".srw"};
  const std::string ext = p.extension().string();
  for (const char* e : kExts)
    if (ext == e) return true;
  return false;
}

template <class T>
const T* as(const cursiv::AstNode* node) {
  return node != nullptr && node->type == &cursiv::ast_type_tag<T>
             ? static_cast<const T*>(node)
             : nullptr;
}

}  // namespace

int main() {
  std::vector<std::filesystem::path> files;
  for (const auto& entry :
       std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR))
    if (is_source_export(entry.path())) files.push_back(entry.path());
  if (files.empty()) return fail(CURSIV_EXAMPLE_DIR, "no example found");

  const auto language = cursiv::create_language<
      powerbuilder::PowerbuilderCoreServices,
      powerbuilder::PowerbuilderSharedCoreServices,
      powerbuilder::install_core_module>();
  for (const std::string_view number : {"1e999", "1e-999"}) {
    const std::string source = "global function double f (); return " +
                               std::string(number) + " end function";
    const auto result = cursiv::test::validationHelper(language.language,source);
    if (result.diagnostics.empty())
      return fail("<numeric-range>", "out-of-range literal needs a diagnostic");
  }

  {
    const std::string source=R"PB(global function double f();
      /* outer /* inner */ outer */
      x=9007199254740993
      x=12345678901234.5678901234
      x=32.
      x="~~" + "next"
      x='~n~t~r~v~f~b~"~'~~~065~h42~o103'
      return
      MessageBox("a","b")
      end function)PB";
    const auto r=language.language.parser->parse(source);
    const auto* p=r.value_as<powerbuilder::ast::Document>();
    const auto* f=p&&p->items.size()==1?as<powerbuilder::ast::FunctionBody>(p->items[0]):nullptr;
    if(!r.accepted||r.cst().recovered()||!r.diagnostics.empty()||!f||f->body.size()!=7)
      return fail("<native>","comments, literals or RETURN statement boundary rejected");
    const char* numbers[]={"9007199254740993","12345678901234.5678901234","32."};
    for(std::size_t i=0;i<3;++i) {
      const auto* a=as<powerbuilder::ast::AssignStmt>(f->body[i]);
      const auto* n=a&&a->rhs.size()==1?as<powerbuilder::ast::NumberLit>(a->rhs[0]):nullptr;
      if(!n||n->value!=numbers[i])return fail("<native>","integer/decimal lexeme was rounded");
    }
    const auto* a=as<powerbuilder::ast::AssignStmt>(f->body[3]);
    const auto* bin=a&&a->rhs.size()==1?as<powerbuilder::ast::BinaryExpr>(a->rhs[0]):nullptr;
    const auto* left=bin?as<powerbuilder::ast::StringLit>(bin->left):nullptr;
    const auto* right=bin?as<powerbuilder::ast::StringLit>(bin->right):nullptr;
    const auto* escaped=as<powerbuilder::ast::AssignStmt>(f->body[4]);
    const auto* text=escaped&&escaped->rhs.size()==1?as<powerbuilder::ast::StringLit>(escaped->rhs[0]):nullptr;
    const auto* ret=as<powerbuilder::ast::ReturnStmt>(f->body[5]);
    if(!left||left->value!="~"||!right||right->value!="next"||!text||
       text->value!="\n\t\r\v\f\b\"'~ABC"||!ret||ret->value||
       !as<powerbuilder::ast::ExprStmt>(f->body[6]))
      return fail("<native>","escape parity, string values or RETURN boundary AST lost");
  }
  {
    const auto r=language.language.parser->parse(
      "global function string continued(); return \"a&\n b\" end function");
    const auto* p=r.value_as<powerbuilder::ast::Document>();
    const auto* f=p&&p->items.size()==1?as<powerbuilder::ast::FunctionBody>(p->items[0]):nullptr;
    const auto* ret=f&&f->body.size()==1?as<powerbuilder::ast::ReturnStmt>(f->body[0]):nullptr;
    const auto* str=ret?as<powerbuilder::ast::StringLit>(ret->value):nullptr;
    if(!r.accepted||r.cst().recovered()||!r.diagnostics.empty()||!str||str->value!="a b")
      return fail("<native>","string continuation value lost");
    const auto invalid=language.language.parser->parse(
      "global function string f(); return \"a\n b\" end function");
    if(invalid.accepted&&!invalid.cst().recovered()&&invalid.diagnostics.empty())
      return fail("<native>","uncontinued string line break accepted");
  }
  {
    const auto r=language.language.parser->parse(
      "function long f() library \"native.dll\" alias for \"RealName;ansi\"\n"
      "function long g() throws exception\n"
      "global subroutine run(); call Parent::create fetch cur using descriptor desc end subroutine");
    const auto* p=r.value_as<powerbuilder::ast::Document>();
    const auto* f=p&&p->items.size()==3?as<powerbuilder::ast::FunctionForwardDecl>(p->items[0]):nullptr;
    const auto* g=p&&p->items.size()==3?as<powerbuilder::ast::FunctionForwardDecl>(p->items[1]):nullptr;
    const auto* run=p&&p->items.size()==3?as<powerbuilder::ast::FunctionBody>(p->items[2]):nullptr;
    const auto* call=run&&run->body.size()==2?as<powerbuilder::ast::Call>(run->body[0]):nullptr;
    const auto* fetch=run&&run->body.size()==2?as<powerbuilder::ast::FetchStmt>(run->body[1]):nullptr;
    if(!r.accepted||r.cst().recovered()||!r.diagnostics.empty()||!f||f->libraryName!="native.dll"||
       f->aliasText!="RealName;ansi"||!g||g->throwsName!="exception"||!call||call->name!="Parent"||
       call->ancestorMethod!="create"||!fetch||!fetch->usingDescriptor)
      return fail("<native>","external declaration, ancestor target or descriptor AST lost");
  }


  int checked = 0;
  for (const std::filesystem::path& path : files) {
    std::ifstream file(path);
    if (!file) return fail(path.string(), "not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    // `recovered()` is set when the parser had to repair the input to keep
    // going: a clean document leaves it false.
    if (result.cst().recovered()) return fail(path.string(), "does not parse");
    if (!result.diagnostics.empty())
      return fail(path.string(), "parses with diagnostics");

    const cursiv::AstNode* const root = result.value;
    const powerbuilder::ast::Document* const doc =
        as<powerbuilder::ast::Document>(root);
    if (doc == nullptr) return fail(path.string(), "built no document");
    if (doc->items.empty())
      return fail(path.string(), "the document holds no item");

    // Read every field a value converter can silently get wrong: a number,
    // an unescaped string, a case-insensitive keyword spelling.
    if (path.filename() == "example4.sra") {
      if (doc->items.size() != 4)
        return fail(path.string(), "expected 4 top-level items");

      const powerbuilder::ast::TypeDecl* const type =
          as<powerbuilder::ast::TypeDecl>(doc->items[0]);
      if (type == nullptr || type->name != "main" ||
          type->super_ == nullptr || type->super_->name != "application")
        return fail(path.string(), "the type declaration reads wrong");

      // `on main.create`: CREATE is also a soft-keyword identifier segment,
      // so the dotted path absorbs it as an ordinary trailing segment rather
      // than leaving it for a separate suffix.
      const powerbuilder::ast::OnBody* const on =
          as<powerbuilder::ast::OnBody>(doc->items[2]);
      if (on == nullptr || on->target == nullptr ||
          on->target->segments.size() != 2 ||
          on->target->segments[0]->name != "main" ||
          on->target->segments[1]->name != "create" || on->body.size() != 1)
        return fail(path.string(), "the on-block reads wrong");

      const powerbuilder::ast::AssignStmt* const assign =
          as<powerbuilder::ast::AssignStmt>(on->body[0]);
      const powerbuilder::ast::Path* const assignTarget =
          assign != nullptr ? as<powerbuilder::ast::Path>(assign->target)
                             : nullptr;
      const powerbuilder::ast::StringLit* const rhs =
          assign != nullptr && assign->rhs.size() == 1
              ? as<powerbuilder::ast::StringLit>(assign->rhs[0])
              : nullptr;
      if (assign == nullptr || assignTarget == nullptr ||
          assignTarget->segments.size() != 1 ||
          assignTarget->segments[0]->name != "appname" || assign->op != "=" ||
          rhs == nullptr || rhs->value != "main")
        return fail(path.string(), "the assignment reads wrong");

      const powerbuilder::ast::EventBody* const event =
          as<powerbuilder::ast::EventBody>(doc->items[3]);
      if (event == nullptr || event->body.size() != 1)
        return fail(path.string(), "the event block reads wrong");

      const powerbuilder::ast::ExprStmt* const exprStmt =
          as<powerbuilder::ast::ExprStmt>(event->body[0]);
      const powerbuilder::ast::Call* const call =
          exprStmt != nullptr ? as<powerbuilder::ast::Call>(exprStmt->value)
                               : nullptr;
      if (call == nullptr || call->name != "MessageBox" ||
          call->args.size() != 2)
        return fail(path.string(), "the event's call reads wrong");
      const powerbuilder::ast::StringLit* const arg0 =
          as<powerbuilder::ast::StringLit>(call->args[0]->value);
      const powerbuilder::ast::StringLit* const arg1 =
          as<powerbuilder::ast::StringLit>(call->args[1]->value);
      if (arg0 == nullptr || !arg0->value.empty() || arg1 == nullptr ||
          arg1->value != "Hello World")
        return fail(path.string(), "the call's arguments read wrong");
    }
    ++checked;
  }

  // Constructs no upstream example reaches: RELEASE, TYPE VARIABLES,
  // prototypes with a LIBRARY alias, an event forward declaration's TYPE
  // form, array bounds and a brace-list initializer, ENUM/DATE/TIME
  // literals, CHOOSE CASE's three case forms, every loop and try/catch,
  // GOTO+label, the embedded SQL statements, an ancestor `::` call, POST and
  // TRIGGER EVENT (either order), DESCRIBE both ways, and the AT-assignment.
  {
    const std::string path = std::string(CURSIV_TEST_DIR) + "/coverage.sra";
    std::ifstream file(path);
    if (!file) return fail(path, "not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) return fail(path, "does not parse");
    if (!result.diagnostics.empty())
      return fail(path, "parses with diagnostics");
    if (as<powerbuilder::ast::Document>(result.value) == nullptr)
      return fail(path, "built no document");
    ++checked;
  }

  // `^` has its own precedence level, tighter than `*`/`/`, left to right
  // within itself (Appeon PowerScript Reference, "Operator precedence in
  // PowerBuilder expressions"): `a * b ^ c` is `a * (b ^ c)`, `a ^ b ^ c`
  // is `(a ^ b) ^ c`.
  {
    const std::string text =
        "$PBExportHeader$p.srf\n"
        "global function integer f1 ();\n"
        "integer a, b, c, x\n"
        "x = a * b ^ c\n"
        "x = a ^ b ^ c\n"
        "return x\n"
        "end function\n";
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("<precedence>", "does not parse clean");
    const powerbuilder::ast::Document* const doc =
        as<powerbuilder::ast::Document>(result.value);
    const powerbuilder::ast::FunctionBody* body = nullptr;
    if (doc != nullptr)
      for (powerbuilder::ast::Stmt* const item : doc->items)
        if ((body = as<powerbuilder::ast::FunctionBody>(item)) != nullptr)
          break;
    if (body == nullptr || body->body.size() != 4)
      return fail("<precedence>", "expected a 4-statement function body");

    const auto* const assign1 = as<powerbuilder::ast::AssignStmt>(body->body[1]);
    const auto* const mul =
        assign1 != nullptr && assign1->rhs.size() == 1
            ? as<powerbuilder::ast::BinaryExpr>(assign1->rhs[0])
            : nullptr;
    const auto* const pow1 =
        mul != nullptr ? as<powerbuilder::ast::BinaryExpr>(mul->right) : nullptr;
    if (mul == nullptr || mul->op != powerbuilder::ast::Operator::multiply ||
        pow1 == nullptr || pow1->op != powerbuilder::ast::Operator::power)
      return fail("<precedence>", "a * b ^ c should be a * (b ^ c)");

    const auto* const assign2 = as<powerbuilder::ast::AssignStmt>(body->body[2]);
    const auto* const pow2 =
        assign2 != nullptr && assign2->rhs.size() == 1
            ? as<powerbuilder::ast::BinaryExpr>(assign2->rhs[0])
            : nullptr;
    const auto* const pow2Left =
        pow2 != nullptr ? as<powerbuilder::ast::BinaryExpr>(pow2->left) : nullptr;
    if (pow2 == nullptr || pow2->op != powerbuilder::ast::Operator::power ||
        pow2Left == nullptr ||
        pow2Left->op != powerbuilder::ast::Operator::power)
      return fail("<precedence>", "a ^ b ^ c should be (a ^ b) ^ c");
  }

  // `event name(args)` in a body is an event call, with or without
  // arguments; in a type it is a forward declaration, with or without
  // parameters (PowerScript Reference, "Calling PowerScript events").
  {
    const std::string text =
        "$PBExportHeader$p.srf\n"
        "global function integer f1 ();\n"
        "event ue_custom()\n"
        "event ue_custom(1)\n"
        "return 1\n"
        "end function\n";
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("<event call>", "does not parse clean");
    const powerbuilder::ast::Document* const doc =
        as<powerbuilder::ast::Document>(result.value);
    const powerbuilder::ast::FunctionBody* body = nullptr;
    if (doc != nullptr)
      for (powerbuilder::ast::Stmt* const item : doc->items)
        if ((body = as<powerbuilder::ast::FunctionBody>(item)) != nullptr)
          break;
    if (body == nullptr || body->body.size() != 3)
      return fail("<event call>", "expected a 3-statement function body");
    for (std::size_t i = 0; i < 2; ++i) {
      const auto* const stmt = as<powerbuilder::ast::ExprStmt>(body->body[i]);
      const auto* const call =
          stmt != nullptr ? as<powerbuilder::ast::Call>(stmt->value) : nullptr;
      if (call == nullptr || call->name != "ue_custom" || !call->isEvent ||
          call->args.size() != i)
        return fail("<event call>", "`event ue_custom(...)` is not a call");
    }
  }
  {
    const std::string text =
        "$PBExportHeader$w.srw\n"
        "global type w from window\n"
        "event ue_a ( )\n"
        "event ue_b ( integer x )\n"
        "end type\n";
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("<event decl>", "does not parse clean");
    const powerbuilder::ast::Document* const doc =
        as<powerbuilder::ast::Document>(result.value);
    const powerbuilder::ast::TypeDecl* type = nullptr;
    if (doc != nullptr)
      for (powerbuilder::ast::Stmt* const item : doc->items)
        if ((type = as<powerbuilder::ast::TypeDecl>(item)) != nullptr) break;
    if (type == nullptr || type->members.size() != 2)
      return fail("<event decl>", "expected a type with 2 members");
    for (std::size_t i = 0; i < 2; ++i) {
      const auto* const event =
          as<powerbuilder::ast::EventForwardDecl>(type->members[i]);
      if (event == nullptr || !event->hasParens || event->params.size() != i)
        return fail("<event decl>", "`event ue_x (...)` is not a declaration");
    }
  }

  std::printf("powerbuilder: %d sample(s) parse\n", checked);
  return 0;
}
