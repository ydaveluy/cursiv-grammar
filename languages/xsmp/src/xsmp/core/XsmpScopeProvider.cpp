#include "xsmp/core/XsmpScopeProvider.hpp"

#include <unordered_set>
#include <vector>

#include "cursiv/core/syntax-tree/DocumentReferences.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "xsmp/core/ast.hpp"

namespace xsmp {
namespace {
using cursiv::AstNode;
using cursiv::AstNodeDescription;
using cursiv::ReferenceInfo;

const cursiv::Document* documentOf(const ReferenceInfo& context) {
  return context.document    ? context.document
         : context.container ? context.container->getDocument()
                             : nullptr;
}

// Declarations establish the expected initializer type; expression wrappers
// preserve it. Arrays establish the type of each nested collection element.
const ast::Type* initializerType(const AstNode* node) {
  const auto* owner = node ? node->parent : nullptr;
  if (const auto* f = cursiv::ast_cast<ast::Field>(owner)) return f->type.get();
  if (const auto* c = cursiv::ast_cast<ast::Constant>(owner))
    return c->type.get();
  if (const auto* p = cursiv::ast_cast<ast::Parameter>(owner))
    return p->type.get();
  if (const auto* a = cursiv::ast_cast<ast::AttributeType>(owner))
    return a->type.get();
  if (const auto* a = cursiv::ast_cast<ast::Attribute>(owner)) {
    const auto* type = cursiv::ast_cast<ast::AttributeType>(a->type.get());
    return type ? type->type.get() : nullptr;
  }
  if (cursiv::ast_cast<ast::CollectionLiteral>(owner)) {
    const auto* type = initializerType(owner);
    if (const auto* array = cursiv::ast_cast<ast::ArrayType>(type))
      return array->itemType.get();
    return nullptr;  // positional structure/static-attribute typing needs the
                     // full type service
  }
  if (cursiv::ast_cast<ast::Expression>(owner)) return initializerType(owner);
  return nullptr;
}

bool memberReference(const ReferenceInfo& context,
                     const cursiv::AstReflection& reflection) {
  const auto accepts = reflection.referenceAccepts(context.reference);
  return reflection.acceptsTag(accepts, &cursiv::ast_type_tag<ast::Constant>) ||
         reflection.acceptsTag(accepts, &cursiv::ast_type_tag<ast::Field>);
}

bool designated(const ReferenceInfo& context) {
  return context.property == "field" &&
         cursiv::ast_cast<ast::DesignatedInitializer>(context.container);
}

void addType(const AstNode* type, std::vector<const AstNode*>& scopes,
             std::unordered_set<const AstNode*>& seen) {
  if (!type || !seen.insert(type).second) return;
  scopes.push_back(type);
  // Resolving a base accepts Type; that query delegates to the default scope
  // and never re-enters this inherited-member walk. Seen also breaks
  // diamonds/cycles.
  if (const auto* c = cursiv::ast_cast<ast::Class>(type))
    addType(c->base.get(), scopes, seen);
  else if (const auto* c = cursiv::ast_cast<ast::Component>(type)) {
    addType(c->base.get(), scopes, seen);
    for (const auto& iface : c->interface_) addType(iface.get(), scopes, seen);
  } else if (const auto* i = cursiv::ast_cast<ast::Interface>(type)) {
    for (const auto& base : i->base) addType(base.get(), scopes, seen);
  }
}
}  // namespace

bool XsmpScopeProvider::visitLocal(
    const ReferenceInfo& context, std::string_view name,
    cursiv::function_ref<bool(const AstNodeDescription&)> visitor) const {
  const auto* document = documentOf(context);
  if (!document) return true;
  const auto& reflection = *services.astReflection;
  const auto accepts = reflection.referenceAccepts(context.reference);
  const auto* narrow =
      reflection.referenceNarrowTarget(context.reference, context.narrow);
  std::vector<const AstNode*> scopes;
  std::unordered_set<const AstNode*> seen;
  if (designated(context)) {
    const auto* type = initializerType(context.container->parent);
    if (cursiv::ast_cast<ast::Structure>(type)) addType(type, scopes, seen);
  } else {
    for (auto* scope = context.container; scope && scope != document;
         scope = scope->parent)
      addType(scope, scopes, seen);
  }
  const auto hash = cursiv::LocalSymbols::hash_name(name);
  const auto take = [&](const AstNodeDescription& entry) {
    return !reflection.acceptsTag(accepts, entry.type) ||
           !reflection.isSubtype(entry.type, narrow) || visitor(entry);
  };
  for (const auto* scope : scopes) {
    const auto* home = scope->getDocument();
    if (!home) continue;
    if (name.empty()) {
      for (const auto& entry : home->localSymbols.forContainer(scope))
        if (!take(entry)) return false;
    } else if (!home->localSymbols.find(scope, name, hash)
                    .for_each([&](const AstNodeDescription& entry) {
                      return entry.name != name || take(entry);
                    }))
      return false;
  }
  return true;
}

const AstNodeDescription* XsmpScopeProvider::getScopeEntry(
    const ReferenceInfo& context, std::string_view name) const {
  if (name.empty() || !documentOf(context)) return nullptr;
  if (!designated(context) &&
      !memberReference(context, *services.astReflection))
    return cursiv::DefaultScopeProvider::getScopeEntry(context, name);
  const AstNodeDescription* found = nullptr;
  (void)visitLocal(context, name, [&](const AstNodeDescription& entry) {
    found = &entry;
    return false;
  });
  if (found || designated(context)) return found;
  return cursiv::DefaultScopeProvider::getScopeEntry(context, name);
}

bool XsmpScopeProvider::visitScopeEntries(
    const ReferenceInfo& context,
    cursiv::function_ref<bool(const AstNodeDescription&)> visitor) const {
  if (!designated(context) &&
      !memberReference(context, *services.astReflection))
    return cursiv::DefaultScopeProvider::visitScopeEntries(context, visitor);
  if (!visitLocal(context, {}, visitor)) return false;
  if (designated(context) || !documentOf(context)) return true;
  const auto& reflection = *services.astReflection;
  return visitGlobalScopeEntries(context,
                                 reflection.referenceAccepts(context.reference),
                                 reflection, visitor);
}

bool XsmpScopeProvider::visitScopeEntries(
    const ReferenceInfo& context, std::string_view name,
    cursiv::function_ref<bool(const AstNodeDescription&)> visitor) const {
  const auto* document = documentOf(context);
  if (name.empty() || !document) return true;
  if (!designated(context) &&
      !memberReference(context, *services.astReflection))
    return cursiv::DefaultScopeProvider::visitScopeEntries(context, name,
                                                           visitor);
  if (!visitLocal(context, name, visitor)) return false;
  if (designated(context)) return true;
  if (auto* refs = document->parseResult.references.get())
    refs->importedNames.record(cursiv::LocalSymbols::hash_name(name));
  const auto& reflection = *services.astReflection;
  return visitGlobalScopeEntries(context, name,
                                 reflection.referenceAccepts(context.reference),
                                 reflection, true, visitor);
}
}  // namespace xsmp
