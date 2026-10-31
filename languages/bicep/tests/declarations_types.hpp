#pragma once

#include <cstdio>
#include <string_view>
#include <unordered_set>

#include "bicep/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"

namespace bicep_declarations_types {
inline bool run(const auto& language) {
  namespace a = bicep::ast;
  bool ok = true;
  auto check = [&](bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "bicep declarations/types: %s\n", message); ok = false; }
  };
  constexpr std::string_view source = R"B(import { T, 'public-name' as Local, f as fn, } from './types.bicep'
import * as lib from './types.bicep'
@description('local extension')
extension kubernetes with {namespace: 'ns', context: 'prod'} as k8s
extension 'br:example/extension:v1' as ext
test integration './main.bicep' = {params: {enabled: true}}
type Element = lib.Payload['items'][*].name?
type Extra = lib.Payload.*!
type Wrapped = resource<'Provider/type@v'>[].properties['id']!
type Signed = -SomeType?
type U =
 | string
 | int[]
type Object = {name: string, * : int}
type Tuple = [string, int?,]
var values = [1, 2,]
var obj = {a: 1, b: 2,}
import 'sys@1.0.0' as old
type Neg = -1
)B";
  const auto result = language.parser->parse(source);
  const auto* root = cursiv::ast_cast<a::Program>(result.value);
  const bool clean = result.accepted && !result.cst().recovered() &&
                     result.diagnostics.empty() && root && root->body.size() == 16;
  check(clean, "native imports/extensions/test and type operators parse cleanly");
  if (!clean) {
    std::fprintf(stderr, "bicep declarations/types: accepted=%u recovered=%u statements=%zu\n",
        unsigned(result.accepted), unsigned(result.cst().recovered()), root ? root->body.size() : 0);
    for (const auto& d : result.diagnostics)
      std::fprintf(stderr, "bicep declarations/types: kind=%u bytes=%u..%u element=%u\n",
          unsigned(d.kind), d.begin, d.end, unsigned(d.element));
    return false;
  }
  const auto* imports = cursiv::ast_cast<a::CompileTimeImportDecl>(root->body[0]);
  const auto* wildcard = cursiv::ast_cast<a::CompileTimeImportDecl>(root->body[1]);
  const auto* path = imports ? cursiv::ast_cast<a::InterpString>(imports->path) : nullptr;
  check(imports && !imports->wildcard && imports->symbols.size() == 3 && path &&
        path->quasis.size() == 1 && path->quasis[0] == "./types.bicep" &&
        imports->symbols[0]->name == "T" && imports->symbols[0]->alias.empty() &&
        imports->symbols[1]->name == "public-name" && imports->symbols[1]->quoted &&
        imports->symbols[1]->alias == "Local" && imports->symbols[2]->name == "f" &&
        imports->symbols[2]->alias == "fn" && wildcard && wildcard->wildcard &&
        wildcard->symbols.empty() && wildcard->alias == "lib",
        "named/wildcard imports preserve names, quoting, aliases, paths and source order");
  const auto* extension = cursiv::ast_cast<a::ExtensionDecl>(root->body[2]);
  const auto* specification = extension ? cursiv::ast_cast<a::Ident>(extension->specification) : nullptr;
  const auto* stringExtension = cursiv::ast_cast<a::ExtensionDecl>(root->body[3]);
  check(extension && specification && specification->name == "kubernetes" &&
        extension->decorators.size() == 1 && extension->alias == "k8s" && extension->withClause &&
        extension->withClause->properties.size() == 2 && stringExtension &&
        cursiv::ast_cast<a::InterpString>(stringExtension->specification) &&
        !stringExtension->withClause && stringExtension->alias == "ext",
        "extensions keep identifier/string specification and optional clauses");
  const auto* test = cursiv::ast_cast<a::TestDecl>(root->body[4]);
  check(test && test->name == "integration" && cursiv::ast_cast<a::InterpString>(test->path) &&
        test->body && test->body->properties.size() == 1 &&
        cursiv::ast_cast<a::ObjectExpr>(test->body->properties[0]->value),
        "test declaration retains its path and object body");
  auto type = [&](std::size_t i) -> const a::TypeExpr* {
    const auto* declaration = cursiv::ast_cast<a::TypeDecl>(root->body[i]);
    return declaration ? declaration->type : nullptr;
  };
  const auto* nullable = cursiv::ast_cast<a::NullableType>(type(5));
  const auto* property = nullable ? cursiv::ast_cast<a::TypeProperty>(nullable->inner) : nullptr;
  const auto* items = property ? cursiv::ast_cast<a::TypeItems>(property->object) : nullptr;
  const auto* index = items ? cursiv::ast_cast<a::TypeIndex>(items->object) : nullptr;
  const auto* indexText = index ? cursiv::ast_cast<a::InterpString>(index->index) : nullptr;
  check(nullable && !nullable->nonNullable && property && property->name == "name" &&
        items && indexText && indexText->quasis.size() == 1 && indexText->quasis[0] == "items" &&
        cursiv::ast_cast<a::TypeProperty>(index->object),
        "type property/index/items/nullable postfixes keep their nesting");
  const auto* nonNullable = cursiv::ast_cast<a::NullableType>(type(6));
  check(nonNullable && nonNullable->nonNullable &&
        cursiv::ast_cast<a::TypeAdditionalProperties>(nonNullable->inner),
        "additional-properties access is distinct from non-null type suffix");
  const auto* wrapped = cursiv::ast_cast<a::NullableType>(type(7));
  const auto* wrappedIndex = wrapped ? cursiv::ast_cast<a::TypeIndex>(wrapped->inner) : nullptr;
  const auto* wrappedProperty = wrappedIndex ? cursiv::ast_cast<a::TypeProperty>(wrappedIndex->object) : nullptr;
  const auto* array = wrappedProperty ? cursiv::ast_cast<a::ArrayType>(wrappedProperty->object) : nullptr;
  const auto* generic = array ? cursiv::ast_cast<a::ParameterizedType>(array->element) : nullptr;
  check(wrapped && wrapped->nonNullable && generic && !generic->object &&
        generic->name == "resource" && generic->args.size() == 1 &&
        cursiv::ast_cast<a::StringType>(generic->args[0]),
        "parameterized type arguments survive an array/property/index chain");
  const auto* signedType = cursiv::ast_cast<a::NullableType>(type(8));
  const auto* unary = signedType ? cursiv::ast_cast<a::UnaryType>(signedType->inner) : nullptr;
  const auto* unionType = cursiv::ast_cast<a::UnionType>(type(9));
  check(unary && unary->op == "-" && cursiv::ast_cast<a::TypeRef>(unary->operand) &&
        unionType && unionType->options.size() == 2 &&
        cursiv::ast_cast<a::AmbientType>(unionType->options[0]) &&
        cursiv::ast_cast<a::ArrayType>(unionType->options[1]),
        "unary type precedence and leading/multiline union order are retained");
  const auto* object = cursiv::ast_cast<a::ObjectType>(type(10));
  const auto* tuple = cursiv::ast_cast<a::TupleType>(type(11));
  check(object && object->members.size() == 2 && tuple && tuple->items.size() == 2 &&
        cursiv::ast_cast<a::NullableType>(tuple->items[1]->type),
        "inline object and tuple types keep every member");
  const auto* oldImport = cursiv::ast_cast<a::ImportDecl>(root->body[14]);
  const auto* oldLiteral = cursiv::ast_cast<a::LiteralType>(type(15));
  const auto* oldNumber = oldLiteral ? cursiv::ast_cast<a::NumberLit>(oldLiteral->value) : nullptr;
  check(oldImport && oldImport->type == &cursiv::ast_type_tag<a::ImportDecl> &&
        oldImport->alias == "old" && oldLiteral && oldLiteral->op == "-" &&
        oldNumber && oldNumber->text == "1", "legacy import and signed literal type shapes remain unchanged");
  auto owned = [&](const cursiv::ParseResult& parsed) {
    if (!parsed.value) return false;
    bool valid = true;
    std::unordered_set<const cursiv::AstNode*> seen;
    auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { valid = false; return; }
      language.astReflection->visitMembers(node,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            valid &= child.parent == &node && parsed.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *parsed.value);
    return valid && seen.size() == parsed.nodes.size();
  };
  check(owned(result), "all declaration/type AST children have unique parents and CST ownership");
  for (const auto source : {
      "import {} from './types.bicep'", "import {\nT\nU\n} from './types.bicep'",
      "extension az", "extension az with {}", "extension az as local",
      "test t './main.bicep' = {}", "type T = lib.Map<string, int>",
      "type T = array<string>",
      "type T = Map<\nstring,\nint\n>", "type T = Map<>",
      "type T = (string | int)[]?", "type T = string[]?!?",
      "type T = | string", "type T = {\nx: int, y: string\n}",
      "type T = [\nint, string\n]", "var x = [\n1, 2\n]"}) {
    const auto r = language.parser->parse(source);
    const bool clean = r.accepted && !r.cst().recovered() && r.diagnostics.empty() && owned(r);
    if (!clean) std::fprintf(stderr, "bicep declarations/types: positive failed: %s\n", source);
    check(clean, "native declaration/type/list control rejected");
  }
  for (const auto source : {
      "import {'name'} from './types.bicep'", "import {'${name}' as n} from './types.bicep'",
      "import {T,\n U} from './types.bicep'", "import {T U} from './types.bicep'",
      "import * from './types.bicep'", "import * as from './types.bicep'",
      "import {T} './types.bicep'", "import {T} from path",
      "extension az with as local", "extension az as local with {}",
      "extension az with {} with {}", "extension az as", "extension",
      "test t './main.bicep' = []", "test t './main.bicep' = if (true) {}",
      "test t = {}", "type T = Map<string,>", "type T = Map<string\n,int>",
      "type T = A?[]", "type T = A..name", "type T = A[*1]",
      "type T = string |\n int", "type T = |", "type T = --1",
      "var x = [1,\n2]", "var x = {a: 1,\nb: 2}",
      "type T = [int,\nstring]", "type T = {a: int,\nb: string}"}) {
    const auto r = language.parser->parse(source);
    const bool rejected = !r.accepted || r.cst().recovered() || !r.diagnostics.empty();
    if (!rejected) std::fprintf(stderr, "bicep declarations/types: unexpectedly clean: %s\n", source);
    check(rejected, "malformed declaration/type/list must carry a diagnostic");
  }
  return ok;
}
} // namespace bicep_declarations_types
