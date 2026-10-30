#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "angelscript/core/CoreServices.hpp"
#include "angelscript/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

namespace ast = angelscript::ast;

int fail(const char* what) {
  std::fprintf(stderr, "angelscript: %s\n", what);
  return 1;
}

std::string read(const char* path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
}

// The corpus is one four-line file; the three documents beside this test
// carry everything else the grammar can say.
const char* const kCleanExamples[] = {
    CURSIV_EXAMPLE_DIR "/example1.as",
    CURSIV_TEST_DIR "/declarations.as",
    CURSIV_TEST_DIR "/statements.as",
    CURSIV_TEST_DIR "/expressions.as",
};

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      angelscript::AngelscriptCoreServices,
      angelscript::AngelscriptSharedCoreServices,
      angelscript::install_core_module>();

  // AngelCode's BNF: attributes are zero-or-more; index arguments use
  // the same optional name prefix at every position; empty scripts are legal.
  for (const char* text : {"", "namespace Empty {}", "namespace A::B {}",
                           "import void plain() from 'mod';",
                           "class C { int value { get; set {} } }",
                           "void f() { a[0, 1, named: 2]; }",
                           "uint a=0xAF; uint b=0B101; uint c=0o77;"}) {
    const auto result = language.language.parser->parse(text);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty())
      return fail("native empty declaration, attribute, index or bits form rejected");
  }
  {
    const auto result = language.language.parser->parse(
        "class C { void f() override final; int p { get final property; } } "
        "import void f() explicit property from 'mod';");
    unsigned sequences = 0;
    for (const auto* node : result.nodes) {
      if (const auto* f = cursiv::ast_cast<ast::FuncDecl>(node)) {
        if (f->attr.size() != 2 || f->attr[0] != "override" || f->attr[1] != "final")
          return fail("function attributes lost order or multiplicity");
        ++sequences;
      }
      if (const auto* a = cursiv::ast_cast<ast::AccessorDecl>(node)) {
        if (a->attr.size() != 2 || a->attr[0] != "final" || a->attr[1] != "property")
          return fail("accessor attributes lost order or multiplicity");
        ++sequences;
      }
      if (const auto* i = cursiv::ast_cast<ast::ImportDecl>(node)) {
        if (i->attr.size() != 2 || i->attr[0] != "explicit" || i->attr[1] != "property")
          return fail("import attributes lost order or multiplicity");
        ++sequences;
      }
    }
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() || sequences != 3)
      return fail("native function attribute sequence rejected");
  }
  for (const char* text : {"uint a=0xG;", "uint a=0b2;", "uint a=0o8;", "uint a=0dA;"}) {
    const auto result = language.language.parser->parse(text);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return fail("invalid radix digits accepted");
  }

  // Native strings share their lexical form in expressions and imports.
  // Keep the raw spelling, including escapes and heredoc contents, in the AST.
  for (const char* literal : {R"("a\"b\\c")", R"('a\'b')",
                              R"("\0\n\r\t\xA\x00FF\u00E9\U0001F600")",
                              "\"\"\"\nfirst \"quoted\" line\nsecond\n\"\"\"",
                              "\"first\" /* join */ 'second'"}) {
    const std::string text = std::string("string value = ") + literal + ";";
    const auto result = language.language.parser->parse(text);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty())
      return fail("native escaped, heredoc or adjacent string does not parse");
    unsigned count = 0;
    for (const auto* node : result.nodes)
      if (const auto* value = cursiv::ast_cast<ast::LiteralExpr>(node)) {
        ++count;
        if (std::string_view(literal).find("/*") == std::string_view::npos &&
            value->text != literal)
          return fail("native string literal lost its raw spelling");
        if (std::string_view(literal).find("/*") != std::string_view::npos &&
            value->text != "\"first\"'second'")
          return fail("adjacent strings did not form one literal");
      }
    if (count != 1) return fail("a native string did not form one literal node");
  }
  for (const char* literal : {R"("\q")", R"("\u123")", "\"line\nline\""}) {
    const std::string text = std::string("string value = ") + literal + ";";
    const auto result = language.language.parser->parse(text);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return fail("invalid default string syntax accepted without diagnostics");
  }
  {
    const auto result = language.language.parser->parse("int value = 8 >> 1 >> 1 < 5;");
    const ast::BinExpr* comparison = nullptr;
    for (const auto* node : result.nodes)
      if (const auto* binary = cursiv::ast_cast<ast::BinExpr>(node);
          binary && binary->op == "<") comparison = binary;
    const auto* shift = comparison
        ? cursiv::ast_cast<ast::BinExpr>(comparison->left) : nullptr;
    const auto* left = shift ? cursiv::ast_cast<ast::BinExpr>(shift->left) : nullptr;
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !shift || shift->op != ">>" || !left || left->op != ">>")
      return fail("shift associativity or precedence changed");
  }
  for (const char* op : {">>", ">>>", "<<"}) {
    const std::string text = std::string("int value = 8 ") + op + " 1;";
    const auto result = language.language.parser->parse(text);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty())
      return fail("adjacent shift operator was rejected");
  }
  for (const char* op : {"> >", ">/* gap */>", ">> >", "< <"}) {
    const std::string text = std::string("int value = 8 ") + op + " 1;";
    const auto result = language.language.parser->parse(text);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return fail("separated angle brackets were accepted as a shift");
  }

  for (const char* const path : kCleanExamples) {
    const std::string text = read(path);
    if (text.empty()) return fail(path);

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      std::fprintf(stderr, "angelscript: %s does not parse clean\n", path);
      return 1;
    }
    if (!result.diagnostics.empty()) {
      std::fprintf(stderr, "angelscript: %s carries a diagnostic\n", path);
      return 1;
    }
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<ast::Script>) {
      std::fprintf(stderr, "angelscript: %s built no Script\n", path);
      return 1;
    }
    const auto& script = *static_cast<const ast::Script*>(root);
    if (script.items.empty()) {
      std::fprintf(stderr, "angelscript: %s built an empty Script\n", path);
      return 1;
    }
  }

  // Read real field values back out of declarations.as: a converter that
  // silently yields the wrong value passes every structural check above.
  {
    const std::string text = read(CURSIV_TEST_DIR "/declarations.as");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto& script = *result.value_as<ast::Script>();

    const ast::ImportDecl* import = nullptr;
    const ast::NamespaceDecl* game = nullptr;
    for (const ast::Item* const item : script.items) {
      if (item->type == &cursiv::ast_type_tag<ast::ImportDecl>)
        import = static_cast<const ast::ImportDecl*>(item);
      if (item->type == &cursiv::ast_type_tag<ast::NamespaceDecl>)
        game = static_cast<const ast::NamespaceDecl*>(item);
    }
    if (import == nullptr || import->name != "notify")
      return fail("declarations: import name");
    if (import->source != "\"log.as\"")
      return fail("declarations: import source");
    if (game == nullptr || game->name != "Game")
      return fail("declarations: namespace name");

    const ast::EnumDecl* status = nullptr;
    const ast::ClassDecl* entity = nullptr;
    for (const ast::Item* const item : game->items) {
      if (item->type == &cursiv::ast_type_tag<ast::EnumDecl>) {
        const auto* const e = static_cast<const ast::EnumDecl*>(item);
        if (e->name == "Status") status = e;
      }
      if (item->type == &cursiv::ast_type_tag<ast::ClassDecl>) {
        const auto* const c = static_cast<const ast::ClassDecl*>(item);
        if (c->name == "Entity") entity = c;
      }
    }
    if (status == nullptr || status->values.size() != 3)
      return fail("declarations: enum Status value count");
    if (status->values[1]->name != "Running" ||
        status->values[1]->value == nullptr)
      return fail("declarations: enum Status.Running value");

    if (entity == nullptr)
      return fail("declarations: class Entity not found");
    if (entity->bases.size() != 2 || entity->bases[0] != "Base" ||
        entity->bases[1] != "IUpdatable")
      return fail("declarations: class Entity bases");

    bool sawDamage = false;
    bool sawHealth = false;
    for (const ast::Item* const member : entity->members) {
      if (member->type == &cursiv::ast_type_tag<ast::FuncDecl>) {
        const auto& fn = *static_cast<const ast::FuncDecl*>(member);
        if (fn.name == "damage") {
          sawDamage = true;
          if (fn.params == nullptr || fn.params->params.size() != 2)
            return fail("declarations: damage() parameter count");
          if (fn.params->params[1]->defaultValue == nullptr)
            return fail("declarations: damage() default value");
        }
      }
      if (member->type == &cursiv::ast_type_tag<ast::VirtPropDecl>) {
        const auto& vp = *static_cast<const ast::VirtPropDecl*>(member);
        if (vp.name == "health") {
          sawHealth = true;
          if (vp.accessors.size() != 2)
            return fail("declarations: health accessors");
        }
      }
    }
    if (!sawDamage) return fail("declarations: Entity::damage not found");
    if (!sawHealth) return fail("declarations: Entity::health not found");
  }

  // Read a few values back out of expressions.as too: the shift operator
  // built from adjacent `<`/`>` tokens, and the precedence ladder.
  {
    const std::string text = read(CURSIV_TEST_DIR "/expressions.as");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto& script = *result.value_as<ast::Script>();

    bool sawShift = false;
    const ast::BinExpr* precMulAdd = nullptr;
    const ast::BinExpr* precPow = nullptr;
    const ast::BinExpr* precBitEq = nullptr;
    const ast::BinExpr* precShiftRel = nullptr;
    for (const ast::Item* const item : script.items) {
      if (item->type != &cursiv::ast_type_tag<ast::FuncDecl>) continue;
      const auto& fn = *static_cast<const ast::FuncDecl*>(item);
      if (fn.name != "showcase" || fn.body == nullptr) continue;
      for (const ast::Item* const stmt : fn.body->items) {
        if (stmt->type != &cursiv::ast_type_tag<ast::VarDecl>) continue;
        const auto& decl = *static_cast<const ast::VarDecl*>(stmt);
        for (const ast::VarInit* const init : decl.declarators) {
          if (init->initExpr == nullptr ||
              init->initExpr->type != &cursiv::ast_type_tag<ast::BinExpr>)
            continue;
          const auto* const bin =
              static_cast<const ast::BinExpr*>(init->initExpr);
          if (init->name == "shiftedLeft" && bin->op == "<<") sawShift = true;
          if (init->name == "precMulAdd") precMulAdd = bin;
          if (init->name == "precPow") precPow = bin;
          if (init->name == "precBitEq") precBitEq = bin;
          if (init->name == "precShiftRel") precShiftRel = bin;
        }
      }
    }
    if (!sawShift)
      return fail("expressions: shiftedLeft is not a '<<' BinExpr");

    const auto isBin = [](const ast::Expr* e, const char* op) {
      return e != nullptr && e->type == &cursiv::ast_type_tag<ast::BinExpr> &&
             static_cast<const ast::BinExpr*>(e)->op == op;
    };

    // 1 + 2 * 3 reads as 1 + (2 * 3): the right operand of + is a * BinExpr.
    if (precMulAdd == nullptr || precMulAdd->op != "+" ||
        !isBin(precMulAdd->right, "*"))
      return fail("expressions: 1 + 2 * 3 does not bind * tighter than +");

    // 2 ** 3 ** 2 is left-associative: the left operand of the outer ** is
    // itself a ** BinExpr.
    if (precPow == nullptr || precPow->op != "**" ||
        !isBin(precPow->left, "**"))
      return fail("expressions: 2 ** 3 ** 2 is not left-associative");

    // 1 & 3 == 3 reads as (1 & 3) == 3: AngelScript ranks & tighter than ==,
    // the reverse of C.
    if (precBitEq == nullptr || precBitEq->op != "==" ||
        !isBin(precBitEq->left, "&"))
      return fail("expressions: 1 & 3 == 3 does not bind & tighter than ==");

    // 1 << 2 < 8 reads as (1 << 2) < 8, though << and < open on the same
    // token.
    if (precShiftRel == nullptr || precShiftRel->op != "<" ||
        !isBin(precShiftRel->left, "<<"))
      return fail("expressions: 1 << 2 < 8 does not bind << tighter than <");
  }

  std::puts("angelscript: every example parses clean");
  return 0;
}
