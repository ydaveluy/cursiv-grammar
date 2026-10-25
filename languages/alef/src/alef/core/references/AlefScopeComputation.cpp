#include "alef/core/references/AlefScopeComputation.hpp"

#include <utility>

#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/workspace/AstNodeDescriptionProvider.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/core/workspace/LocalSymbols.hpp"
#include "alef/core/ast.hpp"

namespace alef {

namespace {

const cursiv::AstNode* enclosing_function(const cursiv::AstNode* node) {
  while (node != nullptr &&
        cursiv::ast_cast<const ast::FuncDecl>(node) == nullptr)
    node = node->parent;
  return node;
}

}  // namespace

void AlefScopeComputation::addLocalSymbol(cursiv::AstNode& node,
                                          const cursiv::Document& document,
                                          cursiv::LocalSymbols& symbols) const {
  const cursiv::AstNode* container = node.parent;
  if (cursiv::ast_cast<const ast::LabeledStmt>(&node) != nullptr) {
    if (const cursiv::AstNode* const fn = enclosing_function(node.parent);
        fn != nullptr)
      container = fn;
  }
  if (container == nullptr || container == &document) return;
  if (auto description = services.astNodeDescriptionProvider->createDescription(
          node, {}, &document))
    symbols.add(container, *std::move(description));
}

}  // namespace alef
