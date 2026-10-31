#include "typescript/core/validation/TypeScriptValidator.hpp"

#include <charconv>
#include <memory>
#include <string_view>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "typescript/core/CoreServices.hpp"

namespace typescript {
namespace {
// Ident keeps its source spelling; checker-reserved names use decoded text.
bool identifierIs(std::string_view source, std::string_view name) {
  std::size_t offset = 0;
  for (const char expected : name) {
    if (offset >= source.size()) return false;
    if (!source.substr(offset).starts_with("\\u")) {
      if (source[offset++] != expected) return false;
      continue;
    }
    offset += 2;
    const bool braced = source.substr(offset).starts_with("{");
    if (braced) ++offset;
    const auto end = braced ? source.find('}', offset) : offset + 4;
    if (end == source.npos || end > source.size() || end == offset) return false;
    unsigned value = 0;
    const auto parsed = std::from_chars(source.data() + offset, source.data() + end, value, 16);
    if (parsed.ec != std::errc{} || parsed.ptr != source.data() + end ||
        value != static_cast<unsigned char>(expected)) return false;
    offset = end + (braced ? 1 : 0);
  }
  return offset == source.size();
}

// Official scanner FirstReservedWord..LastReservedWord (83..118), not the
// separate future-reserved/contextual range. Compare decoded spelling while
// leaving the public AST's raw source views unchanged.
bool hardKeyword(std::string_view name) {
  static constexpr std::string_view words[] = {
      "break", "case", "catch", "class", "const", "continue", "debugger",
      "default", "delete", "do", "else", "enum", "export", "extends",
      "false", "finally", "for", "function", "if", "import", "in",
      "instanceof", "new", "null", "return", "super", "switch", "this",
      "throw", "true", "try", "typeof", "var", "void", "while", "with"};
  for (const auto word : words) if (identifierIs(name, word)) return true;
  return false;
}
void checkHardName(const cursiv::AstNode& node, std::string_view name,
                   cursiv::ValidationAcceptor& accept) {
  if (hardKeyword(name))
    accept.error(node, "A reserved word cannot be used as an identifier here.",
                 "typescript.reserved-identifier");
}

bool propertyName(const ast::Ident& node) {
  if (const auto* member = cursiv::ast_cast<ast::MemberExpr>(node.parent))
    return !member->computed && member->property == &node;
  if (const auto* property = cursiv::ast_cast<ast::Prop>(node.parent))
    return !property->computed && property->key == &node;
  if (const auto* member = cursiv::ast_cast<ast::ClassMember>(node.parent))
    return !member->computed && member->key == &node;
  if (const auto* member = cursiv::ast_cast<ast::TypeMember>(node.parent))
    return !member->computed && member->key == &node;
  if (const auto* member = cursiv::ast_cast<ast::EnumMember>(node.parent))
    return member->key == &node;
  return false;
}

// The same nearest-function/body boundaries as resource-name checks. These
// callers own statement/binding names, never computed keys or export names.
struct NameContext { bool generator = false; bool await = false; };
NameContext nameContext(const cursiv::AstNode& node) {
  for (const auto* parent = node.parent; parent; parent = parent->parent) {
    if (const auto* function = cursiv::ast_cast<ast::FunctionDecl>(parent))
      return {function->isGenerator, function->isAsync};
    if (const auto* function = cursiv::ast_cast<ast::FunctionExpr>(parent)) {
      const auto* property = cursiv::ast_cast<ast::Prop>(function->parent);
      return {function->isGenerator || (property && property->isMethod && property->isGenerator),
              function->isAsync || (property && property->isMethod && property->isAsync)};
    }
    if (const auto* arrow = cursiv::ast_cast<ast::ArrowFunctionExpr>(parent))
      return {false, arrow->isAsync};
    if (const auto* member = cursiv::ast_cast<ast::ClassMember>(parent); member && member->body) {
      const bool staticBlock = member->isStatic && !member->key && !member->isMethod &&
                               !member->isGetter && !member->isSetter;
      return {!staticBlock && member->isGenerator, staticBlock || member->isAsync};
    }
  }
  return {};
}
void checkContextName(const cursiv::AstNode& node, std::string_view name,
                      cursiv::ValidationAcceptor& accept) {
  checkHardName(node, name, accept);
  const auto context = nameContext(node);
  if ((context.generator && identifierIs(name, "yield")) ||
      (context.await && identifierIs(name, "await")))
    accept.error(node, "This binding or label name is reserved in the enclosing function context.",
                 "typescript.contextual-name");
}

bool memberSeparator(std::string_view gap) {
  if (gap.find_first_of("\r\n") != gap.npos ||
      gap.find("\xE2\x80\xA8") != gap.npos ||
      gap.find("\xE2\x80\xA9") != gap.npos)
    return true;
  for (std::size_t i = 0; i < gap.size(); ++i) {
    if (gap.substr(i).starts_with("/*")) {
      const auto end = gap.find("*/", i + 2);
      if (end == gap.npos) return false;
      i = end + 1;
    } else if (gap.substr(i).starts_with("//")) {
      return false;
    } else if (gap[i] == ';' || gap[i] == ',') {
      return true;
    }
  }
  return false;
}

template <class Members>
void checkMembers(const Members& members, cursiv::ValidationAcceptor& accept) {
  for (std::size_t i = 1; i < members.size(); ++i) {
    const auto* previous = members[i - 1];
    const auto* current = members[i];
    if (!previous || !current) continue;
    const auto* document = current->getDocument();
    if (!document || previous->cstNodeId == cursiv::kNoNode ||
        current->cstNodeId == cursiv::kNoNode)
      continue;
    const auto& cst = document->parseResult.cst();
    const auto begin = cst[previous->cstNodeId].end;
    const auto end = cst[current->cstNodeId].begin;
    if (begin <= end && !memberSeparator(cst.text().substr(begin, end - begin)))
      accept.error(*current, "Expected ';', ',' or a line break between type members.",
                   "typescript.type-member-separator")
          .template property<&ast::TypeMember::key>();
  }
}
}  // namespace

void TypeScriptValidator::checkObjectMembers(
    const ast::ObjectType& object, cursiv::ValidationAcceptor& accept) const {
  checkMembers(object.members, accept);
}

void TypeScriptValidator::checkInterfaceMembers(
    const ast::InterfaceDecl& interface, cursiv::ValidationAcceptor& accept) const {
  checkMembers(interface.members, accept);
}

void TypeScriptValidator::checkIdentifierNames(
    const cursiv::AstNode& node, cursiv::ValidationAcceptor& accept) const {
  std::string_view name;
  if (const auto* identifier = cursiv::ast_cast<ast::Ident>(&node)) {
    if (propertyName(*identifier)) return;
    // TypeScript's special this parameter uses createIdentifier(true),
    // including an escaped spelling; it is not an ordinary binding name.
    const auto* parameter = cursiv::ast_cast<ast::Param>(node.parent);
    if (parameter && parameter->pattern == identifier &&
        identifierIs(identifier->name, "this")) return;
    name = identifier->name;
  } else if (const auto* n = cursiv::ast_cast<ast::FunctionDecl>(&node)) name = n->name;
  else if (const auto* n = cursiv::ast_cast<ast::FunctionExpr>(&node)) name = n->name;
  else if (const auto* n = cursiv::ast_cast<ast::ClassDecl>(&node)) name = n->name;
  else if (const auto* n = cursiv::ast_cast<ast::ClassExpr>(&node)) name = n->name;
  else if (const auto* n = cursiv::ast_cast<ast::InterfaceDecl>(&node)) name = n->name;
  else if (const auto* n = cursiv::ast_cast<ast::TypeAliasDecl>(&node)) name = n->name;
  else if (const auto* n = cursiv::ast_cast<ast::EnumDecl>(&node)) name = n->name;
  else if (const auto* n = cursiv::ast_cast<ast::TypeParam>(&node)) name = n->name;
  else if (const auto* n = cursiv::ast_cast<ast::MappedType>(&node)) name = n->name;
  else if (const auto* n = cursiv::ast_cast<ast::ClassMember>(&node); n && n->isIndex) name = n->indexName;
  else if (const auto* n = cursiv::ast_cast<ast::TypeMember>(&node); n && n->isIndex) name = n->indexName;
  else if (const auto* n = cursiv::ast_cast<ast::NamespaceDecl>(&node)) name = n->name;
  else if (const auto* n = cursiv::ast_cast<ast::NamespaceExportDecl>(&node)) name = n->name;
  else return;
  // TypeRef/TypeQuery and exported/property names deliberately do not pass
  // here: native type references use allowReservedWords=true.
  checkHardName(node, name, accept);
}

void TypeScriptValidator::checkLabel(const ast::LabeledStmt& node, cursiv::ValidationAcceptor& accept) const {
  checkContextName(node, node.label, accept);
}
void TypeScriptValidator::checkBreak(const ast::BreakStmt& node, cursiv::ValidationAcceptor& accept) const {
  checkContextName(node, node.label, accept);
}
void TypeScriptValidator::checkContinue(const ast::ContinueStmt& node, cursiv::ValidationAcceptor& accept) const {
  checkContextName(node, node.label, accept);
}
void TypeScriptValidator::checkImportBinding(const ast::ImportSpecifier& node, cursiv::ValidationAcceptor& accept) const {
  checkContextName(node, node.local.empty() ? node.imported : node.local, accept);
}
void TypeScriptValidator::checkImportEquals(const ast::ImportEqualsDecl& node, cursiv::ValidationAcceptor& accept) const {
  checkContextName(node, node.name, accept);
}

void TypeScriptValidator::checkResources(
    const ast::VarDecl& declaration, cursiv::ValidationAcceptor& accept) const {
  if (declaration.kind != "using") return;
  const auto report = [&](std::string_view message, std::string_view code) {
    accept.error(declaration, message, code).property<&ast::VarDecl::kind>();
  };
  // These are checker diagnostics in TypeScript, not parser restrictions.
  if (declaration.isDeclare || cursiv::ast_cast<ast::ExportStmt>(declaration.parent)) {
    report("A resource declaration cannot have a declare or export modifier.", "typescript.using.modifier");
    return;
  }
  bool ambient = false, contextFound = false, asyncContext = false, staticBlock = false;
  bool generatorContext = false;
  const ast::Program* program = nullptr;
  for (const auto* node = declaration.parent; node; node = node->parent) {
    if (const auto* ns = cursiv::ast_cast<ast::NamespaceDecl>(node)) {
      ambient |= ns->isDeclare;
      if (!contextFound) contextFound = true;
    }
    if (const auto* fn = cursiv::ast_cast<ast::FunctionDecl>(node)) {
      ambient |= fn->isDeclare;
      if (!contextFound) {
        contextFound = true; asyncContext = fn->isAsync; generatorContext = fn->isGenerator;
      }
    }
    if (const auto* fn = cursiv::ast_cast<ast::FunctionExpr>(node); fn && !contextFound) {
      contextFound = true;
      const auto* property = cursiv::ast_cast<ast::Prop>(fn->parent);
      asyncContext = fn->isAsync || (property && property->isMethod && property->isAsync);
      generatorContext = fn->isGenerator || (property && property->isMethod && property->isGenerator);
    }
    if (const auto* fn = cursiv::ast_cast<ast::ArrowFunctionExpr>(node); fn && !contextFound) {
      contextFound = true; asyncContext = fn->isAsync;
    }
    if (const auto* member = cursiv::ast_cast<ast::ClassMember>(node)) {
      ambient |= member->isDeclare;
      if (!contextFound && member->body) {
        contextFound = true;
        staticBlock = member->isStatic && !member->key && !member->isMethod &&
            !member->isGetter && !member->isSetter;
        asyncContext = !staticBlock && member->isAsync;
        generatorContext = !staticBlock && member->isGenerator;
      }
    }
    if (const auto* cls = cursiv::ast_cast<ast::ClassDecl>(node)) ambient |= cls->isDeclare;
    if (const auto* root = cursiv::ast_cast<ast::Program>(node)) program = root;
  }
  if (ambient) {
    report("Resource declarations are not allowed in ambient contexts.", "typescript.using.ambient");
    return;
  }
  if (declaration.declarations.empty()) {
    report("Variable declaration list cannot be empty.", "typescript.using.empty");
    return;
  }
  if (!contextFound && program) {
    for (const auto* statement : program->body) {
      const auto* alias = cursiv::ast_cast<ast::ImportEqualsDecl>(statement);
      asyncContext |= cursiv::ast_cast<ast::ImportStmt>(statement) != nullptr ||
          cursiv::ast_cast<ast::ExportStmt>(statement) != nullptr || (alias && !alias->source.empty());
    }
    if (const auto* document = declaration.getDocument())
      for (const auto* node : document->parseResult.nodes)
        if (const auto* meta = cursiv::ast_cast<ast::MetaProperty>(node))
          asyncContext |= meta->meta == "import" && meta->property == "meta";
  }
  const auto* forOf = cursiv::ast_cast<ast::ForOfStmt>(declaration.parent);
  const auto* forIn = cursiv::ast_cast<ast::ForInStmt>(declaration.parent);
  const auto* forClassic = cursiv::ast_cast<ast::ForStmt>(declaration.parent);
  const bool ofHeader = forOf && forOf->leftDecl == &declaration;
  const bool inHeader = forIn && forIn->leftDecl == &declaration;
  const bool classicHeader = forClassic && forClassic->initDecl == &declaration;
  if (inHeader) report("A for-in initializer cannot be a resource declaration.", "typescript.using.for-in");
  if (ofHeader && declaration.declarations.size() != 1)
    report("A for-of header must declare exactly one variable.", "typescript.using.for-of-count");
  if (declaration.hasTrailingComma)
    report("Trailing comma not allowed in a resource declaration list.", "typescript.using.trailing-comma");
  for (const auto* binding : declaration.declarations) {
    if (!binding) continue;
    if (cursiv::ast_cast<ast::ArrayLit>(binding->id) || cursiv::ast_cast<ast::ObjectLit>(binding->id)) {
      accept.error(*binding, "Resource declarations may not have binding patterns.", "typescript.using.binding-pattern")
          .property<&ast::Declarator::id>();
      continue;
    }
    if (!ofHeader && !inHeader && !binding->init)
      accept.error(*binding, "Resource declarations must be initialized.", "typescript.using.initializer")
          .property<&ast::Declarator::id>();
    if (ofHeader && binding->init)
      accept.error(*binding, "A for-of declaration cannot have an initializer.", "typescript.using.for-of-initializer")
          .property<&ast::Declarator::init>();
    if (ofHeader && binding->typeAnnotation)
      accept.error(*binding, "A for-of declaration cannot have a type annotation.", "typescript.using.for-of-type")
          .property<&ast::Declarator::typeAnnotation>();
    if (binding->definite && (binding->init || !binding->typeAnnotation))
      accept.error(*binding, "Invalid definite assignment assertion on a resource declaration.", "typescript.using.definite")
          .property<&ast::Declarator::definite>();
    if (const auto* name = cursiv::ast_cast<ast::Ident>(binding->id); name &&
        (identifierIs(name->name, "let") ||
         (identifierIs(name->name, "await") && (asyncContext || staticBlock)) ||
         (identifierIs(name->name, "yield") && generatorContext)))
      accept.error(*binding, "Reserved resource binding name in this context.", "typescript.using.binding-name")
          .property<&ast::Declarator::id>();
  }
  if (!ofHeader && !inHeader && !classicHeader) {
    const cursiv::AstNode* statement = &declaration;
    while (cursiv::ast_cast<ast::LabeledStmt>(statement->parent)) statement = statement->parent;
    const auto* parent = statement->parent;
    if (cursiv::ast_cast<ast::IfStmt>(parent) || cursiv::ast_cast<ast::WhileStmt>(parent) ||
        cursiv::ast_cast<ast::DoWhileStmt>(parent) || cursiv::ast_cast<ast::ForStmt>(parent) ||
        cursiv::ast_cast<ast::ForOfStmt>(parent) || cursiv::ast_cast<ast::ForInStmt>(parent) ||
        cursiv::ast_cast<ast::WithStmt>(parent))
      report("A resource statement must be inside a block here.", "typescript.using.block");
  }
  if (!declaration.isAwaitUsing) return;
  if (staticBlock) report("await using is not allowed in a class static block.", "typescript.using.await-static");
  else if (!asyncContext)
    report("await using requires an async function or top-level module context.", "typescript.using.await-context");
}

void register_validation_checks(TypeScriptCoreServices& services) {
  using V = TypeScriptValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkIdentifierNames>(
          *services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkObjectMembers>(
          *services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkInterfaceMembers>(
          *services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkResources>(
          *services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkLabel>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkBreak>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkContinue>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkImportBinding>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkImportEquals>(*services.validator)});
}

}  // namespace typescript
