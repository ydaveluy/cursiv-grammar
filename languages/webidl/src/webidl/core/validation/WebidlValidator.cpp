#include "webidl/core/validation/WebidlValidator.hpp"

#include <algorithm>
#include <memory>
#include <string_view>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "webidl/core/CoreServices.hpp"

namespace webidl {
namespace {
template <class T> bool is(const cursiv::AstNode *n) {
  return n && n->type == &cursiv::ast_type_tag<T>;
}
bool keyword(std::string_view name) {
  constexpr std::string_view words[] = {"ArrayBuffer",
                                        "SharedArrayBuffer",
                                        "DataView",
                                        "Int8Array",
                                        "Int16Array",
                                        "Int32Array",
                                        "Uint8Array",
                                        "Uint16Array",
                                        "Uint32Array",
                                        "Uint8ClampedArray",
                                        "BigInt64Array",
                                        "BigUint64Array",
                                        "Float16Array",
                                        "Float32Array",
                                        "Float64Array",
                                        "any",
                                        "object",
                                        "symbol",
                                        "ByteString",
                                        "DOMString",
                                        "USVString",
                                        "-Infinity",
                                        "FrozenArray",
                                        "Infinity",
                                        "NaN",
                                        "ObservableArray",
                                        "Promise",
                                        "async_iterable",
                                        "async_sequence",
                                        "bigint",
                                        "boolean",
                                        "byte",
                                        "double",
                                        "false",
                                        "float",
                                        "long",
                                        "mixin",
                                        "null",
                                        "octet",
                                        "optional",
                                        "or",
                                        "readonly",
                                        "record",
                                        "sequence",
                                        "short",
                                        "true",
                                        "undefined",
                                        "unsigned",
                                        "void",
                                        "async",
                                        "attribute",
                                        "callback",
                                        "const",
                                        "constructor",
                                        "deleter",
                                        "dictionary",
                                        "enum",
                                        "getter",
                                        "includes",
                                        "inherit",
                                        "interface",
                                        "iterable",
                                        "maplike",
                                        "namespace",
                                        "partial",
                                        "required",
                                        "setlike",
                                        "setter",
                                        "static",
                                        "stringifier",
                                        "typedef",
                                        "unrestricted"};
  return std::ranges::find(words, name) != std::end(words);
}
bool argumentKeyword(std::string_view name) {
  constexpr std::string_view words[] = {
      "async",       "attribute",  "callback",    "const",   "constructor",
      "deleter",     "dictionary", "enum",        "getter",  "includes",
      "inherit",     "interface",  "iterable",    "maplike", "namespace",
      "partial",     "required",   "setlike",     "setter",  "static",
      "stringifier", "typedef",    "unrestricted"};
  return std::ranges::find(words, name) != std::end(words);
}
std::string_view rawName(const cursiv::AstNode &n,
                         cursiv::ValidationAcceptor &a,
                         std::string_view field = "name") {
  const auto *document = n.getDocument();
  const auto span = a.featureSpan(n, field, 0);
  if (!document || !span)
    return {};
  const auto source = document->textDocument().getText();
  return span->end <= source.size()
             ? source.substr(span->begin, span->end - span->begin)
             : std::string_view{};
}
bool returnsVoid(const ast::PrimitiveType &n) {
  const auto *p = n.parent;
  if (const auto *op = cursiv::ast_cast<ast::Operation>(p))
    return op->returnType == &n;
  if (const auto *cb = cursiv::ast_cast<ast::Callback>(p))
    return cb->returnType == &n;
  return is<ast::PromiseType>(p);
}
} // namespace

void WebidlValidator::checkName(const cursiv::AstNode &n,
                                cursiv::ValidationAcceptor &a) const {
  const bool named =
      cursiv::ast_cast<ast::Definition>(&n) && !is<ast::IncludesStatement>(&n);
  const bool identifierValue = is<ast::IdentifierValue>(&n);
  const bool includes = is<ast::IncludesStatement>(&n);
  if (const auto *op = cursiv::ast_cast<ast::Operation>(&n))
    if (op->name.empty())
      return;
  if (!named && !includes && !identifierValue && !is<ast::Argument>(&n) &&
      !is<ast::Attribute>(&n) && !is<ast::Operation>(&n) &&
      !is<ast::ConstMember>(&n) && !is<ast::DictionaryMember>(&n) &&
      !is<ast::NamedType>(&n) && !is<ast::ExtendedAttribute>(&n))
    return;
  const auto validate = [&](std::string_view field) {
    const auto raw = rawName(n, a, field);
    if (raw.empty())
      return;
    auto text = raw;
    if (text.front() == '_' || text.front() == '-')
      text.remove_prefix(1);
    if (text.empty() || !((text.front() >= 'A' && text.front() <= 'Z') ||
                          (text.front() >= 'a' && text.front() <= 'z')))
      a.error(n, "An identifier needs a letter after its optional prefix.",
              "webidl.identifier");
    if (raw == "toString" || raw == "_toString" || raw == "_constructor")
      a.error(n, "This identifier is reserved.", "webidl.reserved-name");
    const bool allowed =
        (is<ast::Argument>(&n) && argumentKeyword(raw)) ||
        (is<ast::Attribute>(&n) && (raw == "async" || raw == "required")) ||
        (is<ast::Operation>(&n) && raw == "includes");
    if (keyword(raw) && !allowed)
      a.error(n, "Escape this keyword with a leading underscore.",
              "webidl.keyword-name");
  };
  if (includes) {
    validate("target");
    validate("mixin");
  } else
    validate(identifierValue ? "text" : "name");
  if (const auto *iface = cursiv::ast_cast<ast::Interface>(&n))
    if (!iface->base.empty())
      validate("base");
  if (const auto *dictionary = cursiv::ast_cast<ast::Dictionary>(&n))
    if (!dictionary->base.empty())
      validate("base");
}

void WebidlValidator::checkType(const ast::Type &n,
                                cursiv::ValidationAcceptor &a) const {
  if (n.nullable && (is<ast::AnyType>(&n) || is<ast::PromiseType>(&n)))
    a.error(n, "This type cannot be nullable.", "webidl.nullable-type")
        .keyword("?");
  if (const auto *u = cursiv::ast_cast<ast::UnionType>(&n)) {
    if (u->members.size() < 2)
      a.error(n, "A union needs at least two member types.",
              "webidl.union-size");
    for (const auto *member : u->members)
      if (is<ast::AnyType>(member) || is<ast::PromiseType>(member))
        a.error(*member, "A union cannot contain any or Promise.",
                "webidl.union-type");
  }
  if (const auto *p = cursiv::ast_cast<ast::PrimitiveType>(&n))
    if (p->kind == ast::Primitive::void_ && !returnsVoid(*p))
      a.error(n, "void is only allowed as a return type.", "webidl.void-type");
  if (!n.attrs.empty() &&
      (is<ast::Operation>(n.parent) || is<ast::Callback>(n.parent)))
    a.error(n, "A return type cannot have extended attributes.",
            "webidl.return-attributes");
  if (const auto *p = cursiv::ast_cast<ast::PromiseType>(&n))
    if (p->value && !p->value->attrs.empty())
      a.error(*p->value, "A Promise subtype cannot have extended attributes.",
              "webidl.promise-attributes");
  if (const auto *r = cursiv::ast_cast<ast::RecordType>(&n))
    if (r->keyType && (r->keyType->nullable || !r->keyType->attrs.empty()))
      a.error(*r->keyType, "A record key must be an unannotated string type.",
              "webidl.record-key");
}

void WebidlValidator::checkMember(const ast::Member &n,
                                  cursiv::ValidationAcceptor &a) const {
  if (const auto *iterable = cursiv::ast_cast<ast::AsyncIterable>(&n);
      iterable && iterable->legacySyntax)
    a.error(n, "Use async_iterable instead of async iterable.",
            "webidl.async-iterable-syntax").keyword("async");
  const auto *p = n.parent;
  const bool constant = is<ast::ConstMember>(&n);
  const auto *op = cursiv::ast_cast<ast::Operation>(&n);
  const auto *attr = cursiv::ast_cast<ast::Attribute>(&n);
  const bool regular = op && !op->static_ && !op->stringifier &&
                       op->special == ast::Special::none_;
  if (is<ast::CallbackInterface>(p) && !constant && !regular)
    a.error(n, "A callback interface accepts constants and regular operations.",
            "webidl.callback-member");
  if (is<ast::Namespace>(p) && !constant && !regular &&
      !(attr && attr->readonly_ && !attr->static_ && !attr->stringifier &&
        !attr->inherit_))
    a.error(n,
            "A namespace accepts constants, regular operations and readonly "
            "attributes.",
            "webidl.namespace-member");
  if (is<ast::Mixin>(p) && !constant && !regular && !(op && op->stringifier) &&
      !is<ast::Stringifier>(&n) && !(attr && !attr->inherit_ && !attr->static_))
    a.error(n, "This member is not allowed in an interface mixin.",
            "webidl.mixin-member");
  if (attr && attr->inherit_ && attr->readonly_)
    a.error(n, "An inherited attribute cannot be readonly.",
            "webidl.inherit-readonly");
  if (const auto *c = cursiv::ast_cast<ast::ConstMember>(&n)) {
    if (c->type &&
        (c->type->nullable || !c->type->attrs.empty() ||
         (!is<ast::PrimitiveType>(c->type) && !is<ast::NamedType>(c->type))))
      a.error(*c->type,
              "A constant needs an unannotated primitive or named type.",
              "webidl.constant-type");
  }
}

void WebidlValidator::checkArgument(const ast::Argument &n,
                                    cursiv::ValidationAcceptor &a) const {
  if (n.optional_ && n.variadic)
    a.error(n, "A variadic argument cannot be optional.",
            "webidl.optional-variadic")
        .property<&ast::Argument::variadic>();
  if (n.default_ && !n.optional_)
    a.error(*n.default_, "Only an optional argument can have a default.",
            "webidl.argument-default");
}
void WebidlValidator::checkField(const ast::DictionaryMember &n,
                                 cursiv::ValidationAcceptor &a) const {
  if (n.required_ && n.default_)
    a.error(*n.default_, "A required dictionary member cannot have a default.",
            "webidl.required-default");
}
void WebidlValidator::checkInterface(const ast::Interface &n,
                                     cursiv::ValidationAcceptor &a) const {
  if (n.partial_ && !n.base.empty())
    a.error(n, "A partial interface cannot declare inheritance.",
            "webidl.partial-inheritance")
        .property<&ast::Interface::base>();
}
void WebidlValidator::checkDictionary(const ast::Dictionary &n,
                                      cursiv::ValidationAcceptor &a) const {
  if (n.partial_ && !n.base.empty())
    a.error(n, "A partial dictionary cannot declare inheritance.",
            "webidl.partial-inheritance")
        .property<&ast::Dictionary::base>();
}
void WebidlValidator::checkExtendedAttribute(
    const ast::ExtendedAttribute &n, cursiv::ValidationAcceptor &a) const {
  if (n.hasList && n.values.empty())
    a.error(n, "An extended attribute list cannot be empty.",
            "webidl.attribute-list");
  if (!n.values.empty() && n.values[0])
    for (const auto *value : n.values)
      if (value && value->type != n.values[0]->type) {
        a.error(*value,
                "An extended attribute list must contain one kind of value.",
                "webidl.attribute-list");
        break;
      }
  if (n.hasArguments && (n.hasList || n.wildcard))
    a.error(
        n,
        "Arguments require a scalar value or a bare extended attribute name.",
        "webidl.attribute-arguments");
}

void register_validation_checks(WebidlCoreServices &services) {
  services.validator = std::make_unique<const WebidlValidator>(services);
  using V = WebidlValidator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkName>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkType>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkMember>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkArgument>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkField>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkInterface>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkDictionary>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkExtendedAttribute>(*services.validator)});
}

} // namespace webidl
