#include "c/references/CScopeComputation.hpp"

#include <utility>

#include "cursiv/syntax-tree/AstNode.hpp"
#include "cursiv/workspace/AstNodeDescriptionProvider.hpp"
#include "cursiv/workspace/Document.hpp"
#include "cursiv/workspace/LocalSymbols.hpp"
#include "c/ast.hpp"

namespace c {

namespace {

const cursiv::AstNode* enclosing_function(const cursiv::AstNode* node) {
  while (node != nullptr &&
        cursiv::ast_cast<const ast::FunctionDef>(node) == nullptr)
    node = node->parent;
  return node;
}

}  // namespace

void CScopeComputation::addLocalSymbol(cursiv::AstNode& node,
                                       const cursiv::Document& document,
                                       cursiv::LocalSymbols& symbols) const {
  const cursiv::AstNode* container = node.parent;
  if (cursiv::ast_cast<const ast::Label>(&node) != nullptr) {
    if (const cursiv::AstNode* const fn = enclosing_function(node.parent);
        fn != nullptr)
      container = fn;
  }
  if (container == nullptr || container == &document) return;
  if (auto description = services.astNodeDescriptionProvider->createDescription(
          node, {}, &document))
    symbols.add(container, *std::move(description));
}

}  // namespace c
