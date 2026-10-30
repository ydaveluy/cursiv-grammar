#include "xsmp/core/XsmpScopeComputation.hpp"

#include <cstring>
#include <utility>

#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/workspace/AstNodeDescriptionProvider.hpp"
#include "cursiv/core/workspace/Document.hpp"

namespace xsmp {

namespace {

template <class T>
T* as(cursiv::AstNode* node) {
  return cursiv::ast_cast<T>(node) != nullptr ? static_cast<T*>(node) : nullptr;
}

// The dotted spelling lives where a converter's names live -- the pool the text
// and the tree share -- because a description holds a VIEW and outlives the
// walk that built it.
std::string_view join(std::pmr::memory_resource* pool, std::string_view outer,
                      std::string_view inner) {
  const std::size_t n = outer.size() + 1 + inner.size();
  char* const at = static_cast<char*>(pool->allocate(n, 1));
  std::memcpy(at, outer.data(), outer.size());
  at[outer.size()] = '.';
  std::memcpy(at + outer.size() + 1, inner.data(), inner.size());
  return {at, n};
}

bool has_modifier(const ast::VisibilityElement& element,
                  std::string_view word) {
  for (const std::string_view modifier : element.modifiers)
    if (modifier == word) return true;
  return false;
}

// A member of a STRUCTURE or an INTERFACE is public unless it says otherwise;
// anywhere else the default is private, so only a spelled `public` publishes.
bool is_published(cursiv::AstNode& member, cursiv::AstNode* container) {
  const auto* const visible = cursiv::ast_cast<ast::VisibilityElement>(&member);
  if (visible == nullptr) return true;
  if (has_modifier(*visible, "private") || has_modifier(*visible, "protected")) return false;
  if (has_modifier(*visible, "public")) return true;
  return container && (container->type == &cursiv::ast_type_tag<ast::Structure> ||
                       container->type == &cursiv::ast_type_tag<ast::Interface>);
}

// The member list a container publishes into its own name, whichever of the
// three shapes it is.
const cursiv::vector<ast::NamedElement*>* members_of(cursiv::AstNode& node) {
  if (const auto* const structure = cursiv::ast_cast<ast::Structure>(&node))
    return &structure->elements;
  if (const auto* const iface = cursiv::ast_cast<ast::Interface>(&node))
    return &iface->elements;
  if (const auto* const component = cursiv::ast_cast<ast::Component>(&node))
    return &component->elements;
  return nullptr;
}

// What a type publishes under its own name: an enumeration's literals, and the
// constants a container exposes. Native local aliases include all constants;
// workspace exports apply visibility.
template <class F>
void each_named_value(ast::NamedElement& type, F&& yield, bool local = false) {
  if (auto* const enumeration = as<ast::Enumeration>(&type)) {
    for (ast::EnumerationLiteral* const literal : enumeration->literal)
      if (literal != nullptr && !literal->name.empty())
        yield(static_cast<cursiv::AstNode&>(*literal), literal->name);
    return;
  }
  const cursiv::vector<ast::NamedElement*>* const members = members_of(type);
  if (members == nullptr) return;
  for (ast::NamedElement* const member : *members) {
    if (member == nullptr || member->name.empty()) continue;
    // Only constants travel outside their container: a field is read through
    // the instance that holds it, never by name from another document.
    if (cursiv::ast_cast<ast::Constant>(member) == nullptr) continue;
    if (!local && !is_published(*member, &type)) continue;
    yield(static_cast<cursiv::AstNode&>(*member), member->name);
  }
}

}  // namespace

void XsmpScopeComputation::exportType(
    ast::NamedElement& type, std::string_view name,
    std::vector<cursiv::AstNodeDescription>& exported,
    const cursiv::Document& document, std::pmr::memory_resource* pool) const {
  if (auto description = services.astNodeDescriptionProvider->createDescription(
          type, name, &document))
    exported.push_back(*std::move(description));

  each_named_value(
      type, [&](cursiv::AstNode& member, std::string_view leaf) {
        if (auto description =
                services.astNodeDescriptionProvider->createDescription(
                    member, join(pool, name, leaf), &document))
          exported.push_back(*std::move(description));
      });
}

void XsmpScopeComputation::exportNamespace(
    ast::Namespace& ns, std::string_view name,
    std::vector<cursiv::AstNodeDescription>& exported,
    const cursiv::Document& document, std::pmr::memory_resource* pool,
    const cursiv::CancellationToken& cancelToken) const {
  if (auto description = services.astNodeDescriptionProvider->createDescription(
          ns, name, &document))
    exported.push_back(*std::move(description));

  for (ast::NamedElement* const element : ns.elements) {
    if (element == nullptr || element->name.empty()) continue;
    cursiv::throw_if_cancelled(cancelToken);
    const std::string_view dotted = join(pool, name, element->name);
    if (auto* const nested = as<ast::Namespace>(element))
      exportNamespace(*nested, dotted, exported, document, pool, cancelToken);
    else
      exportType(*element, dotted, exported, document, pool);
  }
}

std::vector<cursiv::AstNodeDescription>
XsmpScopeComputation::collectExportedSymbols(
    const cursiv::Document& document,
    const cursiv::CancellationToken& cancelToken) const {
  auto* const catalogue = as<ast::Catalogue>(document.parseResult.value);
  if (catalogue == nullptr) return {};

  std::vector<cursiv::AstNodeDescription> exported;
  if (auto description = services.astNodeDescriptionProvider->createDescription(
          *catalogue, {}, &document))
    exported.push_back(*std::move(description));

  std::pmr::memory_resource* const pool = document.parseResult.cst().memory();
  for (ast::Namespace* const ns : catalogue->elements) {
    if (ns == nullptr || ns->name.empty()) continue;
    cursiv::throw_if_cancelled(cancelToken);
    exportNamespace(*ns, ns->name, exported, document, pool, cancelToken);
  }
  return exported;
}

std::vector<cursiv::AstNodeDescription> XsmpScopeComputation::qualifyInto(
    ast::Namespace& ns, cursiv::LocalSymbols& symbols,
    const cursiv::Document& document, std::pmr::memory_resource* pool,
    const cursiv::CancellationToken& cancelToken) const {
  std::vector<cursiv::AstNodeDescription> contributed;
  for (ast::NamedElement* const element : ns.elements) {
    if (element == nullptr || element->name.empty()) continue;
    cursiv::throw_if_cancelled(cancelToken);
    if (auto* const nested = as<ast::Namespace>(element)) {
      for (const cursiv::AstNodeDescription& inner :
           qualifyInto(*nested, symbols, document, pool, cancelToken)) {
        cursiv::AstNodeDescription dotted = inner;
        dotted.name = join(pool, nested->name, inner.name);
        // A sibling of the nested namespace names it dotted from here.
        symbols.add(&ns, dotted);
        contributed.push_back(std::move(dotted));
      }
    } else {
      // ...and a sibling of a type names its members the same way, `Type.Leaf`
      // from here and dotted again by every namespace above.
      each_named_value(
          *element, [&](cursiv::AstNode& member, std::string_view leaf) {
            auto description =
                services.astNodeDescriptionProvider->createDescription(
                    member, join(pool, element->name, leaf), &document);
            if (!description) return;
            symbols.add(&ns, *description);
            contributed.push_back(*std::move(description));
          }, true);
    }
    if (auto description =
            services.astNodeDescriptionProvider->createDescription(*element, {},
                                                                   &document))
      contributed.push_back(*std::move(description));
  }
  return contributed;
}

cursiv::LocalSymbols XsmpScopeComputation::collectLocalSymbols(
    const cursiv::Document& document,
    const cursiv::CancellationToken& cancelToken) const {
  cursiv::LocalSymbols symbols;
  symbols.reserve(document.parseResult.nodes.size() / 4);

  // Every named node in its own container, which is the default's answer and
  // stays true here: the dotted spellings are ADDED to it, never instead of it.
  for (cursiv::AstNode* const node : document.parseResult.nodes) {
    cursiv::throw_if_cancelled(cancelToken);
    addLocalSymbol(*node, document, symbols);
  }

  if (auto* const catalogue = as<ast::Catalogue>(document.parseResult.value)) {
    std::pmr::memory_resource* const pool = document.parseResult.cst().memory();
    for (ast::Namespace* const ns : catalogue->elements) {
      if (ns == nullptr || ns->name.empty()) continue;
      for (const cursiv::AstNodeDescription& inner :
           qualifyInto(*ns, symbols, document, pool, cancelToken)) {
        cursiv::AstNodeDescription dotted = inner;
        dotted.name = join(pool, ns->name, inner.name);
        symbols.add(catalogue, dotted);
      }
    }
  }
  symbols.seal();
  return symbols;
}

}  // namespace xsmp
