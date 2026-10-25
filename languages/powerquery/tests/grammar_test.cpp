#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
#include "powerquery/core/CoreServices.hpp"
#include "powerquery/core/ast.hpp"
// Power Query end to end: the corpus example, a coverage document of our own
// for what it does not reach, and a spot check of field values read back out
// of the AST.

#include <memory>
#include <string>

#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"
#include "powerquery/generated/Powerquery/Module.hpp"

namespace {

namespace ast = powerquery::ast;

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      powerquery::generated::Powerquery::create_parser();
  return *p;
}

template <class T>
T* as(cursiv::AstNode* n) {
  return const_cast<T*>(cursiv::ast_cast<T>(n));
}

cursiv::ParseResult parse_clean(const std::string& path) {
  const std::string text = cursiv::test::read_file(path);
  CHECK(!text.empty());
  cursiv::ParseResult r = engine().parse(text);
  CHECK(static_cast<bool>(r));
  CHECK(!r.cst().recovered());
  CHECK(r.diagnostics.empty());
  return r;
}

// The upstream corpus: one document. Attributes, a function value with an
// optional parameter, a record, a list literal, `type table`, `try`/
// `otherwise`, `let`/`in`.
void corpus() {
  const std::string path =
      std::string(CURSIV_EXAMPLE_DIR) + "/example.pq";
  cursiv::ParseResult r = parse_clean(path);
  ast::Doc* const doc = r.value_as<ast::Doc>();
  CHECK(doc != nullptr && doc->section != nullptr);
  if (doc == nullptr || doc->section == nullptr) return;
  ast::Section& section = *doc->section;
  CHECK(section.name == "testSection");
  CHECK(section.members.size() == 3);
  if (section.members.size() != 3) return;

  CHECK(section.members[0]->name == "myrecord");
  ast::RecordExpr* const rec = as<ast::RecordExpr>(section.members[0]->value);
  CHECK(rec != nullptr && rec->fields.size() == 1);
  ast::FunctionExpr* const func =
      rec != nullptr && !rec->fields.empty()
          ? as<ast::FunctionExpr>(rec->fields[0]->value)
          : nullptr;
  CHECK(func != nullptr && func->params.size() == 2);
  CHECK(func != nullptr && !func->params.empty() &&
        func->params[0]->optional == false);
  CHECK(func != nullptr && func->params.size() == 2 &&
        func->params[1]->optional == true);
  ast::ListExpr* const body =
      func != nullptr ? as<ast::ListExpr>(func->body) : nullptr;
  CHECK(body != nullptr && body->items.size() == 4);

  CHECK(section.members[1]->name == "CustomType");
  ast::TypeValueExpr* const typeValue =
      as<ast::TypeValueExpr>(section.members[1]->value);
  CHECK(typeValue != nullptr);
  ast::TableTypeNode* const table =
      typeValue != nullptr ? as<ast::TableTypeNode>(typeValue->type) : nullptr;
  CHECK(table != nullptr && table->rows.size() == 5);
  CHECK(table != nullptr && !table->rows.empty() &&
        table->rows[0]->name == "Ttype");
  ast::PrimitiveTypeNode* const ttype =
      table != nullptr && !table->rows.empty()
          ? as<ast::PrimitiveTypeNode>(table->rows[0]->type)
          : nullptr;
  CHECK(ttype != nullptr && ttype->name == "text");

  CHECK(section.members[2]->name == "tryCatcher");
  ast::FunctionExpr* const tryFunc =
      as<ast::FunctionExpr>(section.members[2]->value);
  CHECK(tryFunc != nullptr && tryFunc->returnType != nullptr);
  ast::TryExpr* const tryExpr =
      tryFunc != nullptr ? as<ast::TryExpr>(tryFunc->body) : nullptr;
  CHECK(tryExpr != nullptr && tryExpr->otherwiseExpr != nullptr);
  ast::LetExpr* const let =
      tryExpr != nullptr ? as<ast::LetExpr>(tryExpr->otherwiseExpr) : nullptr;
  CHECK(let != nullptr && let->variables.size() == 2);
  ast::NameExpr* const inBody =
      let != nullptr ? as<ast::NameExpr>(let->body) : nullptr;
  CHECK(inBody != nullptr && inBody->name == "nextLine");
}

// Our own coverage document, under tests/ (never counted as corpus): every
// construct the corpus above does not reach, checked below.
void coverage() {
  const std::string path =
      std::string(CURSIV_TEST_DIR) + "/coverage.pq";
  cursiv::ParseResult r = parse_clean(path);
  ast::Doc* const doc = r.value_as<ast::Doc>();
  CHECK(doc != nullptr && doc->section != nullptr);
  if (doc == nullptr || doc->section == nullptr) return;
  ast::Section& section = *doc->section;
  CHECK(section.attributes != nullptr);
  CHECK(section.members.size() == 3);
  if (section.members.size() != 3) return;
  CHECK(section.members[0]->shared == true);
  ast::EachExpr* const each0 = as<ast::EachExpr>(section.members[0]->value);
  CHECK(each0 != nullptr);
  ast::FieldAccess* const implicitField =
      each0 != nullptr ? as<ast::FieldAccess>(each0->body) : nullptr;
  CHECK(implicitField != nullptr && implicitField->target == nullptr &&
        implicitField->optional == true);

  ast::LetExpr* const let = as<ast::LetExpr>(section.members[1]->value);
  CHECK(let != nullptr && let->variables.size() == 20);
  ast::SectionAccessExpr* const access =
      let != nullptr ? as<ast::SectionAccessExpr>(let->body) : nullptr;
  CHECK(access != nullptr && access->section == "Section2" &&
        access->member == "Query2");

  ast::TypeValueExpr* const typeValue =
      as<ast::TypeValueExpr>(section.members[2]->value);
  ast::RecordTypeNode* const recType =
      typeValue != nullptr ? as<ast::RecordTypeNode>(typeValue->type)
                            : nullptr;
  CHECK(recType != nullptr && recType->open == true);
  CHECK(recType != nullptr && recType->fields.size() == 10);
}

// A hand-built diagnostic: an unterminated string leaves a diagnostic, not a
// crash, and the rest of the document still parses.
void diagnostics() {
  cursiv::ParseResult r = engine().parse("\"unterminated");
  CHECK(static_cast<bool>(r));
  CHECK(!r.diagnostics.empty() || r.cst().recovered());
}

// `<`, `<=`, `>`, `>=`, `=`, `<>`, `and`, `or` are all right-associative in
// the M language specification: `a<b<c` is `a<(b<c)`, `a and b and c` is
// `a and (b and c)`.
void precedence() {
  {
    cursiv::ParseResult r = engine().parse("a<b<c");
    CHECK(static_cast<bool>(r) && !r.cst().recovered());
    ast::Doc* const doc = as<ast::Doc>(r.value);
    ast::BinaryExpr* const top = doc != nullptr ? as<ast::BinaryExpr>(doc->expr) : nullptr;
    CHECK(top != nullptr && top->op == ast::Op::less);
    ast::BinaryExpr* const right = top != nullptr ? as<ast::BinaryExpr>(top->right) : nullptr;
    CHECK(right != nullptr && right->op == ast::Op::less);
  }
  {
    cursiv::ParseResult r = engine().parse("a and b and c");
    CHECK(static_cast<bool>(r) && !r.cst().recovered());
    ast::Doc* const doc = as<ast::Doc>(r.value);
    ast::BinaryExpr* const top = doc != nullptr ? as<ast::BinaryExpr>(doc->expr) : nullptr;
    CHECK(top != nullptr && top->op == ast::Op::logical_and);
    ast::BinaryExpr* const right = top != nullptr ? as<ast::BinaryExpr>(top->right) : nullptr;
    CHECK(right != nullptr && right->op == ast::Op::logical_and);
  }
}

// The specific keyword alternatives (ANY, TABLE, FUNCTION, ...) of a
// primitive type sit beside a literal fallback (`type null`, `type 5`):
// each common keyword shape must still take the keyword reading, not the
// literal one.
void primitive_type_keywords() {
  for (const char* const text : {"type text", "type any", "type table",
                                 "type function", "type number"}) {
    cursiv::ParseResult r = engine().parse(text);
    CHECK(static_cast<bool>(r) && !r.cst().recovered());
    ast::Doc* const doc = as<ast::Doc>(r.value);
    ast::TypeValueExpr* const tv = doc != nullptr ? as<ast::TypeValueExpr>(doc->expr) : nullptr;
    ast::PrimitiveTypeNode* const prim =
        tv != nullptr ? as<ast::PrimitiveTypeNode>(tv->type) : nullptr;
    CHECK(prim != nullptr && prim->literal == nullptr && !prim->name.empty());
  }
}

}  // namespace

static int audit_regression() {
  const auto language =
      cursiv::create_language<powerquery::PowerqueryCoreServices,
                              powerquery::PowerquerySharedCoreServices,
                              powerquery::install_core_module>();
  {
    const auto r = language.language.parser->parse("\"#(lf,0041,0001F600)\"");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
    const auto* doc = r.value_as<powerquery::ast::Doc>();
    const auto* str =
        doc ? cursiv::ast_cast<powerquery::ast::TextLit>(doc->expr) : nullptr;
    if (!str || str->value != "\nA😀") return 1;
  }
  {
    const auto r = language.language.parser->parse("(optional x) => x");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  {
    auto doc = language.shared->documentFactory->fromString(
        "(optional x, y) => y", "file:///audit.pq");
    if (!doc->parseResult.accepted || doc->parseResult.cst().recovered() ||
        !doc->parseResult.diagnostics.empty())
      return 1;
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(*doc, diagnostics);
    if (diagnostics.empty()) return 1;
  }
  {
    const auto r = language.language.parser->parse("#!\"unparsed #(lf) code\"");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty()) return 1;
  }
  {
    auto doc = language.shared->documentFactory->fromString(
        "type function (optional x as number, y as number) as number",
        "file:///audit.pq");
    if (!doc->parseResult.accepted || doc->parseResult.cst().recovered() ||
        !doc->parseResult.diagnostics.empty())
      return 1;
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(*doc, diagnostics);
    if (diagnostics.empty()) return 1;
  }
  return 0;
}

int main() {
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  corpus();
  coverage();
  diagnostics();
  precedence();
  primitive_type_keywords();
  return cursiv::test::report("powerquery_grammar_test");
}
