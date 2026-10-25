#pragma once

#include <memory_resource>
#include <string_view>
#include <vector>

#include "cursiv/core/references/DefaultScopeComputation.hpp"
#include "cursiv/core/utils/Cancellation.hpp"
#include "cursiv/core/workspace/AstNodeDescription.hpp"
#include "cursiv/core/workspace/LocalSymbols.hpp"
#include "xsmp/core/ast.hpp"

// Qualified names, both ways: a declaration is named from outside its
// namespace as `Outer.Inner.Name`, what the grammar's QualifiedName spells.
// The default publishes only the simple name, resolvable from nowhere else.

namespace cursiv {
struct Document;
}  // namespace cursiv

namespace xsmp {

class XsmpScopeComputation : public cursiv::DefaultScopeComputation {
 public:
  using cursiv::DefaultScopeComputation::DefaultScopeComputation;

  [[nodiscard]] std::vector<cursiv::AstNodeDescription> collectExportedSymbols(
      const cursiv::Document& document,
      const cursiv::CancellationToken& cancelToken =
          cursiv::kNoCancel) const override;

  [[nodiscard]] cursiv::LocalSymbols collectLocalSymbols(
      const cursiv::Document& document,
      const cursiv::CancellationToken& cancelToken =
          cursiv::kNoCancel) const override;

 private:
  // The exports of one namespace and everything under it, each already dotted.
  void exportNamespace(ast::Namespace& ns, std::string_view name,
                       std::vector<cursiv::AstNodeDescription>& exported,
                       const cursiv::Document& document,
                       std::pmr::memory_resource* pool,
                       const cursiv::CancellationToken& cancelToken) const;

  // One type, then the members a type publishes under its own name: an
  // enumeration's literals, and the constants a container makes public.
  void exportType(ast::NamedElement& type, std::string_view name,
                  std::vector<cursiv::AstNodeDescription>& exported,
                  const cursiv::Document& document,
                  std::pmr::memory_resource* pool) const;

  // What one namespace contributes to its container's scope, undotted at this
  // level: the caller prefixes with the namespace's own name. The dotted forms
  // are added to `ns` itself on the way through, so a sibling sees them too.
  [[nodiscard]] std::vector<cursiv::AstNodeDescription> qualifyInto(
      ast::Namespace& ns, cursiv::LocalSymbols& symbols,
      const cursiv::Document& document, std::pmr::memory_resource* pool,
      const cursiv::CancellationToken& cancelToken) const;
};

}  // namespace xsmp
