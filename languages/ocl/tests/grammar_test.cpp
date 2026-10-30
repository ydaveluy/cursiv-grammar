// The grammar's smoke test: the upstream corpus and this port's own
// coverage document both parse clean, a few real field values read back,
// and a malformed invariant carries a diagnostic.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "ocl/core/CoreServices.hpp"
#include "ocl/core/ast.hpp"
#include "native_regressions.hpp"

namespace {

namespace ast = ocl::ast;

int fail(const char* what) {
  std::fprintf(stderr, "ocl: %s\n", what);
  return 1;
}

std::string read(const char* path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

template <class T>
const T* as(const cursiv::AstNode* node) {
  if (node == nullptr || node->type != &cursiv::ast_type_tag<T>)
    return nullptr;
  return static_cast<const T*>(node);
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      ocl::OclCoreServices, ocl::OclSharedCoreServices,
      ocl::install_core_module>();
  const cursiv::Parser& parser = *language.language.parser;
  if (!ocl_native_regressions(parser)) return fail("native OCL regression");

  // contextExamples.ocl: the upstream corpus, 30 context specifications.
  {
    const std::string text = read(CURSIV_EXAMPLE_SAMPLE);
    const cursiv::ParseResult result = parser.parse(text);
    if (result.cst().recovered())
      return fail("contextExamples.ocl does not parse");
    if (!result.diagnostics.empty())
      return fail("contextExamples.ocl carries a diagnostic");
    const auto* model = as<ast::Model>(result.value);
    if (model == nullptr) return fail("contextExamples.ocl built no model");
    if (model->specs.size() != 30)
      return fail("contextExamples.ocl: wrong spec count");

    const auto* first = as<ast::Invariant>(model->specs[0]);
    if (first == nullptr || first->contextName != "ProductionOrder")
      return fail("contextExamples.ocl: the first context did not read back");
    if (!first->name.empty())
      return fail("contextExamples.ocl: an unnamed invariant kept a name");
    const auto* exists = as<ast::Call>(first->body);
    if (exists == nullptr || !exists->arrow || exists->name != "exists")
      return fail("contextExamples.ocl: the top-level ->exists did not read back");

    const auto* derived = as<ast::DerivedAttribute>(model->specs[4]);
    if (derived == nullptr ||
        derived->qualifiedName != "MyClass::derivedAttribute")
      return fail("contextExamples.ocl: the derived attribute name did not read back");
    const auto* namedType = as<ast::NamedType>(derived->type);
    if (namedType == nullptr || namedType->name != "Integer")
      return fail("contextExamples.ocl: the derived attribute type did not read back");
    if (derived->init != nullptr)
      return fail("contextExamples.ocl: an absent init: was not null");
    const auto* sum = as<ast::BinaryExpr>(derived->derive);
    if (sum == nullptr || sum->op != "+")
      return fail("contextExamples.ocl: the derive: expression did not read back");
  }

  // constructs.ocl: this port's own corpus -- one of each construct not
  // reached by the upstream examples.
  {
    const std::string text = read(CURSIV_TEST_DIR "/constructs.ocl");
    const cursiv::ParseResult result = parser.parse(text);
    if (result.cst().recovered()) return fail("constructs.ocl does not parse");
    if (!result.diagnostics.empty())
      return fail("constructs.ocl carries a diagnostic");
    const auto* model = as<ast::Model>(result.value);
    if (model == nullptr) return fail("constructs.ocl built no model");

    const auto* named = as<ast::Invariant>(model->specs[0]);
    if (named == nullptr || named->name != "named" ||
        named->contextName != "Sample")
      return fail("constructs.ocl: the named invariant did not read back");

    const auto* wheel = as<ast::DerivedAttribute>(model->specs[1]);
    if (wheel == nullptr) return fail("constructs.ocl: wheelCount is not a derived attribute");
    const auto* zero = as<ast::IntLit>(wheel->init);
    if (zero == nullptr || zero->value != 0)
      return fail("constructs.ocl: the init: literal did not read back");

    // OrderedSet{1, 2, 3}->union(Bag{4, 5})->size() = 3 -- the collection
    // kind lost its trailing brace, and the union's argument is its own
    // collection literal.
    const auto* warehouseUnion = as<ast::Invariant>(model->specs[8]);
    if (warehouseUnion == nullptr)
      return fail("constructs.ocl: the union invariant moved");
    const auto* sizeEq = as<ast::BinaryExpr>(warehouseUnion->body);
    const auto* sizeCall =
        sizeEq != nullptr ? as<ast::Call>(sizeEq->left) : nullptr;
    const auto* unionCall =
        sizeCall != nullptr ? as<ast::Call>(sizeCall->object) : nullptr;
    const auto* orderedSet =
        unionCall != nullptr ? as<ast::SetLiteral>(unionCall->object) : nullptr;
    if (orderedSet == nullptr || orderedSet->kind != "OrderedSet" ||
        orderedSet->items.size() != 3)
      return fail("constructs.ocl: the OrderedSet literal did not read back");
    if (unionCall->args.size() != 1)
      return fail("constructs.ocl: ->union did not read back one argument");
    const auto* bag = as<ast::SetLiteral>(unionCall->args[0]);
    if (bag == nullptr || bag->kind != "Bag" || bag->items.size() != 2)
      return fail("constructs.ocl: the Bag argument did not read back");

    // 1.5 <> null and "double quoted \n escape" <> '' -- the outer 'and'
    // splits into two '<>' comparisons; the second's left operand is the
    // escaped string.
    const auto* literals = as<ast::Invariant>(model->specs[16]);
    const auto* bothChecks =
        literals != nullptr ? as<ast::BinaryExpr>(literals->body) : nullptr;
    const auto* stringsEq =
        bothChecks != nullptr ? as<ast::BinaryExpr>(bothChecks->right)
                               : nullptr;
    const auto* escaped =
        stringsEq != nullptr ? as<ast::StringLit>(stringsEq->left) : nullptr;
    if (escaped == nullptr || escaped->value.find('\n') == std::string_view::npos)
      return fail("constructs.ocl: the \\n escape did not decode");

    // (x mod 2) = 0 and ... and x <: y -- five 'and'-level terms, so the
    // outermost node is the last 'and', over the rightmost comparison.
    const auto* ops = as<ast::Invariant>(model->specs.back());
    const auto* top = as<ast::BinaryExpr>(ops->body);
    if (top == nullptr || top->op != "and")
      return fail("constructs.ocl: the ops chain lost its outer 'and'");
    const auto* rightmost = as<ast::BinaryExpr>(top->right);
    if (rightmost == nullptr || rightmost->op != "<:")
      return fail("constructs.ocl: the '<:' operator did not read back");
  }

  // Precedence, OMG OCL 2.4 section 7.4.8: relational binds tighter than
  // equality, and `and`/`or`/`xor`/`implies` are four levels, not one.
  {
    const std::string text = "context D inv: true = 1 < 2";
    const cursiv::ParseResult result = parser.parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("precedence/relational: does not parse clean");
    const auto* model = as<ast::Model>(result.value);
    const auto* inv =
        model != nullptr ? as<ast::Invariant>(model->specs[0]) : nullptr;
    const auto* eq = inv != nullptr ? as<ast::BinaryExpr>(inv->body) : nullptr;
    if (eq == nullptr || eq->op != "=")
      return fail("precedence/relational: outer operator is not '='");
    const auto* cmp = as<ast::BinaryExpr>(eq->right);
    if (cmp == nullptr || cmp->op != "<")
      return fail("precedence/relational: 'true = (1 < 2)' did not read back");
  }
  {
    const std::string text = "context D inv: 1 = 1 or 2 = 2 and 3 = 3";
    const cursiv::ParseResult result = parser.parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("precedence/and-or: does not parse clean");
    const auto* model = as<ast::Model>(result.value);
    const auto* inv =
        model != nullptr ? as<ast::Invariant>(model->specs[0]) : nullptr;
    const auto* orExpr =
        inv != nullptr ? as<ast::BinaryExpr>(inv->body) : nullptr;
    if (orExpr == nullptr || orExpr->op != "or")
      return fail("precedence/and-or: outer operator is not 'or'");
    const auto* andExpr = as<ast::BinaryExpr>(orExpr->right);
    if (andExpr == nullptr || andExpr->op != "and")
      return fail("precedence/and-or: 'or' did not close over 'and' on its right");
  }

  // invalid.ocl: `context ID inv` with no ':' must carry a diagnostic.
  {
    const std::string text = read(CURSIV_TEST_DIR "/invalid.ocl");
    const cursiv::ParseResult result = parser.parse(text);
    if (result.diagnostics.empty())
      return fail("invalid.ocl: the missing ':' was accepted");
  }

  std::puts("ocl: the samples parse");
  return 0;
}
