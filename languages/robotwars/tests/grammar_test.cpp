// The corpus lane: every document in example/ builds clean -- no repair, no
// diagnostic -- and lands on a Program root; one document's field values are
// read back to catch a converter that silently yields the wrong number, and
// GOSUB/GOTO resolve through the same document pipeline the linker uses.

#include <cstdio>
#include <memory>
#include <span>
#include <string>

#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/services/Diagnostic.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/core/workspace/DocumentBuilder.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
#include "cursiv/core/workspace/Documents.hpp"
#include "cursiv/test/Check.hpp"
#include "robotwars/core/CoreServices.hpp"
#include "robotwars/core/ast.hpp"

namespace {

namespace ast = robotwars::ast;

template <class T>
const T* as(const cursiv::AstNode* node) {
  return node != nullptr && node->type == &cursiv::ast_type_tag<T>
             ? static_cast<const T*>(node)
             : nullptr;
}

struct World {
  cursiv::CoreLanguage<robotwars::RobotwarsCoreServices,
                       robotwars::RobotwarsSharedCoreServices>
      language = cursiv::create_language<robotwars::RobotwarsCoreServices,
                                         robotwars::RobotwarsSharedCoreServices,
                                         robotwars::install_core_module>();

  mutable int opened = 0;

  // A single document, built through the full pipeline: parsed, scoped and
  // linked, which is what makes a `reference<Label>` resolve at all. Each
  // call gets its own URI: the store refuses a name already open.
  std::shared_ptr<cursiv::Document> build(
      const char* name, const char* dir = CURSIV_EXAMPLE_DIR) const {
    const std::string path = std::string(dir) + "/" + name;
    const std::string text = cursiv::test::read_file(path);
    CHECK(!text.empty());
    std::shared_ptr<cursiv::Document> document =
        language.shared->documentFactory->fromString(
            text, "file:///example/" + std::to_string(opened++) + "/" + name);
    language.shared->documents->addDocument(document);
    cursiv::Document* one = document.get();
    language.shared->documentBuilder->build(
        std::span<cursiv::Document* const>(&one, 1));
    return document;
  }
};

// Every file in example/, original names: fails on a repaired parse, a
// diagnostic, or a root that is not a Program.
void every_example_builds_clean(const World& world) {
  static constexpr const char* kFiles[] = {
      "bottom.robotwars",    "bottomkiller.robotwars", "dragon.robotwars",
      "george.robotwars",    "mover.robotwars",        "random.robotwars",
      "scanner.robotwars",   "target.robotwars",       "test.robotwars",
  };
  for (const char* name : kFiles) {
    const std::shared_ptr<cursiv::Document> document = world.build(name);
    for (const cursiv::Diagnostic& d : document->diagnostics)
      std::fprintf(stderr, "FAIL %s: %s\n", name,
                  std::string(d.message).c_str());
    CHECK(document->diagnostics.empty());
    CHECK(!document->parseResult.cst().recovered());
    CHECK(document->parseResult.value != nullptr &&
          document->parseResult.value->type ==
              &cursiv::ast_type_tag<ast::Program>);
  }
}

// bottom.robotwars, item by item: labels, comments, every statement kind but
// the accumulator, a negative number literal, a binary chain, and both
// GOSUB/GOTO resolving to the label they name.
void field_values_of_bottom(const World& world) {
  const std::shared_ptr<cursiv::Document> document =
      world.build("bottom.robotwars");
  CHECK(document->diagnostics.empty());

  const auto* program = document->parseResult.value_as<ast::Program>();
  CHECK(program != nullptr);
  if (program == nullptr) return;

  CHECK(program->items.size() == 22);
  if (program->items.size() != 22) return;

  const auto* label = as<ast::Label>(program->items[2]);
  CHECK(label != nullptr && label->name == "BOTTOM");

  const auto* to1 = as<ast::ToStatement>(program->items[3]);
  CHECK(to1 != nullptr && to1->targets.size() == 1 &&
        to1->targets[0] == ast::Register::speedy);
  const auto* n250 = to1 != nullptr ? as<ast::NumberLit>(to1->value) : nullptr;
  CHECK(n250 != nullptr && !n250->negative && n250->value == 250);

  const auto* if1 = as<ast::IfStatement>(program->items[4]);
  CHECK(if1 != nullptr);
  if (if1 != nullptr) {
    const auto* left = as<ast::RegisterRef>(if1->condition->left);
    CHECK(left != nullptr && left->reg == ast::Register::y);
    CHECK(if1->condition->op == ast::Comparison::less);
    const auto* right = as<ast::NumberLit>(if1->condition->right);
    CHECK(right != nullptr && right->value == 240);
    const auto* body = as<ast::GotoStatement>(if1->body);
    CHECK(body != nullptr && body->target.refText == "BOTTOM");
    CHECK(body != nullptr && body->target.get() == label);
  }

  // A leading sign is the number's own, not a unary operator.
  const auto* to2 = as<ast::ToStatement>(program->items[8]);
  const auto* neg100 =
      to2 != nullptr ? as<ast::NumberLit>(to2->value) : nullptr;
  CHECK(neg100 != nullptr && neg100->negative && neg100->value == 100);

  const auto* gosub = as<ast::GosubStatement>(program->items[9]);
  const auto* scanLabel = as<ast::Label>(program->items[18]);
  CHECK(gosub != nullptr && scanLabel != nullptr &&
        gosub->target.get() == scanLabel);

  const auto* lastGoto = as<ast::GotoStatement>(program->items[16]);
  const auto* moveleftLabel = as<ast::Label>(program->items[7]);
  CHECK(lastGoto != nullptr && moveleftLabel != nullptr &&
        lastGoto->target.get() == moveleftLabel);

  // `IF RADAR < 0, 0 - RADAR TO SHOT`: a COMMA separator, then a binary
  // chain as the TO value.
  const auto* if2 = as<ast::IfStatement>(program->items[20]);
  CHECK(if2 != nullptr);
  if (if2 != nullptr) {
    const auto* to3 = as<ast::ToStatement>(if2->body);
    CHECK(to3 != nullptr && to3->targets.size() == 1 &&
          to3->targets[0] == ast::Register::shot);
    const auto* chain =
        to3 != nullptr ? as<ast::BinaryExpr>(to3->value) : nullptr;
    CHECK(chain != nullptr && chain->op == ast::Operation::subtract);
    if (chain != nullptr) {
      const auto* l = as<ast::NumberLit>(chain->left);
      const auto* r = as<ast::RegisterRef>(chain->right);
      CHECK(l != nullptr && !l->negative && l->value == 0);
      CHECK(r != nullptr && r->reg == ast::Register::radar);
    }
  }

  CHECK(as<ast::EndSubStatement>(program->items[21]) != nullptr);
}

// `unary.robotwars`: the leading-operator expression, all four operators.
void field_values_of_unary(const World& world) {
  const std::shared_ptr<cursiv::Document> document =
      world.build("unary.robotwars", CURSIV_TEST_DIR);
  CHECK(document->diagnostics.empty());
  CHECK(!document->parseResult.cst().recovered());

  const auto* program = document->parseResult.value_as<ast::Program>();
  CHECK(program != nullptr);
  if (program == nullptr) return;

  static constexpr ast::Operation kOps[] = {
      ast::Operation::multiply, ast::Operation::divide, ast::Operation::add,
      ast::Operation::subtract};
  // Two header comments, the START label, then the four TO statements.
  if (program->items.size() < 7) {
    CHECK(program->items.size() >= 7);
    return;
  }
  for (int i = 0; i < 4; ++i) {
    const auto* to = as<ast::ToStatement>(program->items[3 + i]);
    CHECK(to != nullptr);
    const auto* unary =
        to != nullptr ? as<ast::UnaryExpr>(to->value) : nullptr;
    CHECK(unary != nullptr && unary->op == kOps[i]);
  }
}

}  // namespace

int main() {
  const World world;

  every_example_builds_clean(world);
  field_values_of_bottom(world);
  field_values_of_unary(world);

  return cursiv::test::report("robotwars_grammar_test");
}
