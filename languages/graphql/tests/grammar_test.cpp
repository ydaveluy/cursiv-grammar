// The grammar's smoke test: every document under tests/ (the upstream
// corpus is empty) parses clean, real field values read back correctly, and
// context-sensitive syntax violations are retained for validation.

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <variant>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "graphql/core/CoreServices.hpp"
#include "graphql/core/ast.hpp"
#include "resume_regressions.hpp"

namespace {

namespace ast = graphql::ast;

int fail(const char *what) {
  std::fprintf(stderr, "graphql: %s\n", what);
  return 1;
}

std::string read_file(const char *path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

template <class T> const T *as(const cursiv::AstNode *node) {
  if (node == nullptr || node->type != &cursiv::ast_type_tag<T>)
    return nullptr;
  return static_cast<const T *>(node);
}

int literal_values(const cursiv::Parser& parser) {
  const std::string source = R"gql("schema description" schema { query: Query }
directive @custom on | FIELD | QUERY
{f(x:"A\n\u0042\u{1F600}\uD83D\uDE00", y:"""
    alpha
      beta
    \"""gamma
""")}
)gql";
  const auto result = parser.parse(source);
  if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty())
    return fail("literal regression: expected a clean parse");
  const auto* doc = result.value_as<ast::Document>();
  if (doc == nullptr || doc->definitions.size() != 3)
    return fail("literal regression: definitions were lost");
  const auto* schema = as<ast::SchemaDefinition>(doc->definitions[0]);
  const auto* directive = as<ast::DirectiveDefinition>(doc->definitions[1]);
  const auto* operation = as<ast::OperationDefinition>(doc->definitions[2]);
  if (schema == nullptr || schema->description != "schema description" ||
      directive == nullptr || directive->locations.size() != 2 ||
      operation == nullptr || operation->selectionSet == nullptr ||
      operation->selectionSet->selections.size() != 1)
    return fail("literal regression: schema, directive or operation was lost");
  const auto* field = as<ast::Field>(operation->selectionSet->selections[0]);
  if (field == nullptr || field->arguments.size() != 2)
    return fail("literal regression: arguments were lost");
  const auto* x = std::get_if<std::string_view>(&field->arguments[0]->value);
  const auto* y = std::get_if<std::string_view>(&field->arguments[1]->value);
  if (x == nullptr || *x != "A\nB😀😀" ||
      y == nullptr || *y != "alpha\n  beta\n\"\"\"gamma")
    return fail("literal regression: wrong decoded string value");
  const std::string crlf = "{f(x:\"\"\"\r\n\tfoo\r\t  bar\r\n\"\"\")}";
  const auto normalized = parser.parse(crlf);
  if (!normalized.accepted || normalized.cst().recovered() ||
      !normalized.diagnostics.empty())
    return fail("literal regression: CR/LF block string refused");
  bool found = false;
  for (const auto* node : normalized.nodes)
    if (const auto* arg = as<ast::Argument>(node)) {
      const auto* value = std::get_if<std::string_view>(&arg->value);
      found = value != nullptr && *value == "foo\n  bar";
    }
  return found ? 0 : fail("literal regression: wrong normalized block value");
}

} // namespace

int main() {
  {
    const auto audit_language =
        cursiv::create_language<graphql::GraphqlCoreServices,
                                graphql::GraphqlSharedCoreServices,
                                graphql::install_core_module>();
    const auto audit_result =
        audit_language.language.parser->parse("{f(x:\"\"\"a\\\"\"\"b\"\"\")}");
    if (!audit_result.accepted || audit_result.cst().recovered() ||
        !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
  }

  {
    const auto audit_language =
        cursiv::create_language<graphql::GraphqlCoreServices,
                                graphql::GraphqlSharedCoreServices,
                                graphql::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse(
        "query Q($x: Int @custom){f(x:$x)}");
    if (!audit_result.accepted || audit_result.cst().recovered() ||
        !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
    bool retained = false;
    for (const auto *node : audit_result.nodes)
      if (const auto *variable =
              cursiv::ast_cast<graphql::ast::VariableDefinition>(node))
        retained = variable->directives.size() == 1 &&
                   variable->directives[0]->name == "custom";
    if (!retained)
      return 1;
  }

  const auto language =
      cursiv::create_language<graphql::GraphqlCoreServices,
                              graphql::GraphqlSharedCoreServices,
                              graphql::install_core_module>();
  const cursiv::Parser &parser = *language.language.parser;
  if (literal_values(parser)) return 1;
  if (!graphql_resume::run(language.language)) return fail("native regressions");

  const char *const docs[] = {
      CURSIV_TEST_DIR "/query.graphql",
      CURSIV_TEST_DIR "/schema.graphql",
  };

  // The texts must outlive every read of the parsed trees below: every
  // string_view in a CST/AST points into them.
  std::string texts[std::size(docs)];
  for (std::size_t i = 0; i < std::size(docs); ++i)
    texts[i] = read_file(docs[i]);

  cursiv::ParseResult results[std::size(docs)];
  for (std::size_t i = 0; i < std::size(docs); ++i) {
    if (texts[i].empty())
      return fail("a document is not readable");
    results[i] = parser.parse(texts[i]);
    if (results[i].cst().recovered())
      return fail("a document does not parse clean");
    if (!results[i].diagnostics.empty())
      return fail("a document carries an unexpected diagnostic");
    if (as<ast::Document>(results[i].value) == nullptr)
      return fail("the parse built no document");
  }

  // query.graphql: the executable language -- operations, fragments,
  // variables, directives, and every shape of `value`.
  {
    const auto &doc = *as<ast::Document>(results[0].value);
    if (doc.definitions.size() != 4)
      return fail("query.graphql: wrong definition count");

    const auto *get_user = as<ast::OperationDefinition>(doc.definitions[0]);
    if (get_user == nullptr ||
        get_user->operation != ast::OperationKind::Query ||
        get_user->name != "GetUser")
      return fail("query.graphql: GetUser did not read back");
    if (get_user->variableDefinitions.size() != 3)
      return fail("query.graphql: wrong variable definition count");

    const auto *tags_def = get_user->variableDefinitions[2];
    if (tags_def == nullptr || tags_def->name != "tags")
      return fail("query.graphql: the $tags variable did not read back");
    const auto *tags_type = as<ast::ListTypeRef>(tags_def->type);
    if (tags_type == nullptr || tags_type->nonNull)
      return fail("query.graphql: $tags's type is not a nullable list");
    const auto *tags_item = as<ast::NamedTypeRef>(tags_type->itemType);
    if (tags_item == nullptr || tags_item->name != "String" ||
        !tags_item->nonNull)
      return fail("query.graphql: $tags's item type did not read back");
    if (!tags_def->defaultValue.has_value())
      return fail("query.graphql: $tags has no default value");
    const auto *tags_default =
        std::get_if<ast::ListValue *>(&*tags_def->defaultValue);
    if (tags_default == nullptr || *tags_default == nullptr ||
        (*tags_default)->items.size() != 2)
      return fail("query.graphql: $tags's default did not read back as a list");
    const auto *tag0 =
        std::get_if<std::string_view>(&(*tags_default)->items[0]);
    if (tag0 == nullptr || *tag0 != "a")
      return fail(
          "query.graphql: $tags's default first item did not read back");

    if (get_user->selectionSet == nullptr ||
        get_user->selectionSet->selections.size() != 1)
      return fail("query.graphql: GetUser's selection set is wrong");
    const auto *user_field =
        as<ast::Field>(get_user->selectionSet->selections[0]);
    if (user_field == nullptr || user_field->name != "user")
      return fail("query.graphql: the user field did not read back");
    if (user_field->arguments.size() != 7)
      return fail("query.graphql: wrong argument count on user()");

    const auto *limit_arg = user_field->arguments[2];
    const auto *limit_val = limit_arg != nullptr
                                ? std::get_if<ast::IntLit *>(&limit_arg->value)
                                : nullptr;
    if (limit_arg == nullptr || limit_arg->name != "limit" ||
        limit_val == nullptr || *limit_val == nullptr || (*limit_val)->value != "10")
      return fail("query.graphql: the limit argument did not read back");

    const auto *ratio_arg = user_field->arguments[3];
    const auto *ratio_val =
        ratio_arg != nullptr ? std::get_if<ast::FloatLit *>(&ratio_arg->value) : nullptr;
    if (ratio_arg == nullptr || ratio_arg->name != "ratio" ||
        ratio_val == nullptr || *ratio_val == nullptr || (*ratio_val)->value != "1.5")
      return fail("query.graphql: the ratio argument did not read back");

    const auto *kind_arg = user_field->arguments[5];
    const auto *kind_val = kind_arg != nullptr
                               ? std::get_if<ast::EnumLit *>(&kind_arg->value)
                               : nullptr;
    if (kind_arg == nullptr || kind_val == nullptr || *kind_val == nullptr ||
        (*kind_val)->name != "ADMIN")
      return fail(
          "query.graphql: the kind argument did not read back as an enum");

    const auto *meta_arg = user_field->arguments[6];
    const auto *meta_val =
        meta_arg != nullptr ? std::get_if<ast::ObjectValue *>(&meta_arg->value)
                            : nullptr;
    if (meta_arg == nullptr || meta_val == nullptr || *meta_val == nullptr ||
        (*meta_val)->fields.size() != 2)
      return fail(
          "query.graphql: the meta argument did not read back as an object");
    const auto *meta_y = (*meta_val)->fields[1];
    const auto *meta_y_val = meta_y != nullptr
                                 ? std::get_if<std::string_view>(&meta_y->value)
                                 : nullptr;
    if (meta_y == nullptr || meta_y->name != "y" || meta_y_val == nullptr ||
        *meta_y_val != "z")
      return fail("query.graphql: meta.y did not read back");

    if (user_field->selectionSet == nullptr ||
        user_field->selectionSet->selections.size() != 4)
      return fail("query.graphql: user's selection set is wrong");
    const auto *spread =
        as<ast::FragmentSpread>(user_field->selectionSet->selections[2]);
    if (spread == nullptr || spread->name != "UserFields")
      return fail("query.graphql: the ...UserFields spread did not read back");
    const auto *inline_frag =
        as<ast::InlineFragment>(user_field->selectionSet->selections[3]);
    if (inline_frag == nullptr || inline_frag->typeCondition != "Admin" ||
        inline_frag->directives.size() != 1)
      return fail("query.graphql: the inline fragment did not read back");
    const auto *include_dir = inline_frag->directives[0];
    const auto *if_val =
        include_dir != nullptr && !include_dir->arguments.empty()
            ? std::get_if<ast::Variable *>(&include_dir->arguments[0]->value)
            : nullptr;
    if (include_dir == nullptr || include_dir->name != "include" ||
        if_val == nullptr || *if_val == nullptr || (*if_val)->name != "active")
      return fail("query.graphql: @include(if: $active) did not read back");

    const auto *anon_mutation =
        as<ast::OperationDefinition>(doc.definitions[1]);
    if (anon_mutation == nullptr ||
        anon_mutation->operation != ast::OperationKind::Mutation ||
        !anon_mutation->name.empty())
      return fail("query.graphql: the anonymous mutation did not read back");

    const auto *shorthand = as<ast::OperationDefinition>(doc.definitions[2]);
    if (shorthand == nullptr ||
        shorthand->operation != ast::OperationKind::Query ||
        shorthand->selectionSet == nullptr ||
        shorthand->selectionSet->selections.size() != 1)
      return fail(
          "query.graphql: the shorthand `{ ping }` did not default to query");

    const auto *frag = as<ast::FragmentDefinition>(doc.definitions[3]);
    if (frag == nullptr || frag->name != "UserFields" ||
        frag->typeCondition != "User" || frag->selectionSet == nullptr ||
        frag->selectionSet->selections.size() != 3)
      return fail("query.graphql: the fragment definition did not read back");
    // `on` and `type` are keywords elsewhere in the grammar, not here: a
    // field position has no competing keyword action, so the per-state
    // lexer reads them as plain names -- no grammar work was needed for it.
    const auto *on_field = as<ast::Field>(frag->selectionSet->selections[1]);
    const auto *type_field = as<ast::Field>(frag->selectionSet->selections[2]);
    if (on_field == nullptr || on_field->name != "on" ||
        type_field == nullptr || type_field->name != "type")
      return fail(
          "query.graphql: `on`/`type` did not read back as field names");
  }

  // schema.graphql: the type-system definition language -- schema, every
  // type kind, a directive definition, and every extension kind.
  {
    const auto &doc = *as<ast::Document>(results[1].value);
    if (doc.definitions.size() != 17)
      return fail("schema.graphql: wrong definition count");

    const auto *schema_def = as<ast::SchemaDefinition>(doc.definitions[0]);
    if (schema_def == nullptr || schema_def->directives.size() != 1 ||
        schema_def->operationTypes.size() != 2)
      return fail("schema.graphql: the schema definition did not read back");
    if (schema_def->operationTypes[1]->operation !=
            ast::OperationKind::Mutation ||
        schema_def->operationTypes[1]->type != "Mutation")
      return fail("schema.graphql: the mutation root did not read back");

    const auto *query_type = as<ast::ObjectTypeDefinition>(doc.definitions[2]);
    if (query_type == nullptr || query_type->name != "Query" ||
        query_type->description != "Root query type" ||
        query_type->interfaces.size() != 2 ||
        query_type->interfaces[1] != "Named" || query_type->fields.size() != 4)
      return fail("schema.graphql: the Query type did not read back");
    if (query_type->fields[2]->name != "type" ||
        query_type->fields[3]->name != "on")
      return fail(
          "schema.graphql: `type`/`on` did not read back as field names");
    const auto *user_arg_type = query_type->fields[1];
    if (user_arg_type == nullptr || user_arg_type->arguments.size() != 1 ||
        user_arg_type->arguments[0]->name != "id")
      return fail("schema.graphql: Query.user's argument did not read back");

    const auto *enum_def = as<ast::EnumTypeDefinition>(doc.definitions[8]);
    if (enum_def == nullptr || enum_def->name != "Role" ||
        enum_def->values.size() != 3)
      return fail("schema.graphql: the Role enum did not read back");

    const auto *input_def =
        as<ast::InputObjectTypeDefinition>(doc.definitions[9]);
    if (input_def == nullptr || input_def->fields.size() != 2)
      return fail("schema.graphql: the UserInput type did not read back");
    const auto *role_field = input_def->fields[1];
    const auto *role_default =
        role_field != nullptr && role_field->defaultValue.has_value()
            ? std::get_if<ast::EnumLit *>(&*role_field->defaultValue)
            : nullptr;
    if (role_field == nullptr || role_field->name != "role" ||
        role_default == nullptr || *role_default == nullptr ||
        (*role_default)->name != "ADMIN")
      return fail("schema.graphql: UserInput.role's default did not read back");

    const auto *directive_def =
        as<ast::DirectiveDefinition>(doc.definitions[10]);
    if (directive_def == nullptr || directive_def->name != "auth" ||
        !directive_def->repeatable || directive_def->locations.size() != 2 ||
        directive_def->locations[0] !=
            ast::DirectiveLocationKind::FieldDefinition ||
        directive_def->locations[1] != ast::DirectiveLocationKind::Object)
      return fail(
          "schema.graphql: the @auth directive definition did not read back");

    const auto *union_ext = as<ast::UnionTypeExtension>(doc.definitions[14]);
    if (union_ext == nullptr || union_ext->name != "SearchResult" ||
        union_ext->members.size() != 1 || union_ext->members[0] != "Query")
      return fail("schema.graphql: the union extension did not read back");

    const auto *enum_ext = as<ast::EnumTypeExtension>(doc.definitions[15]);
    if (enum_ext == nullptr || enum_ext->name != "Role" ||
        enum_ext->values.size() != 1 || enum_ext->values[0]->name != "BANNED")
      return fail("schema.graphql: the enum extension did not read back");

    const auto *input_ext =
        as<ast::InputObjectTypeExtension>(doc.definitions[16]);
    if (input_ext == nullptr || input_ext->name != "UserInput" ||
        input_ext->fields.size() != 1 || input_ext->fields[0]->name != "extra")
      return fail("schema.graphql: the input extension did not read back");
  }

  // A fragment may be named `on`: refusing it is the validator's job.
  {
    const std::string text = "fragment on on Foo { id }";
    const cursiv::ParseResult result = parser.parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("a fragment named `on` was refused");
  }

  std::puts("graphql: every document parses, field values read back correctly");
  return 0;
}
