#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
#include "miniscript/core/CoreServices.hpp"
#include "miniscript/core/ast.hpp"
// The smoke test of the language: every file in example/ parses clean and
// builds a Document; a few documents are read back field by field, since a
// converter that silently yields the wrong value passes every structural
// check.

#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

int fail(const std::string& path, const char* what) {
  std::fprintf(stderr, "miniscript: %s: %s\n", path.c_str(), what);
  return 1;
}

std::vector<std::string> split(const std::string& s, char sep) {
  std::vector<std::string> parts;
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, sep))
    if (!item.empty()) parts.push_back(item);
  return parts;
}

bool ends_with(const std::string& s, const char* suffix) {
  const std::string_view sv(s);
  const std::string_view suf(suffix);
  return sv.size() >= suf.size() && sv.substr(sv.size() - suf.size()) == suf;
}

}  // namespace

static int audit_regression() {
  const auto language =
      cursiv::create_language<miniscript::MiniscriptCoreServices,
                              miniscript::MiniscriptSharedCoreServices,
                              miniscript::install_core_module>();
  {
    const auto r = language.language.parser->parse(
        "multi(1,key0,key1,key2,key3,key4,key5,key6,key7,key8,key9,key10,key11,"
        "key12,key13,key14,key15,key16,key17,key18,key19,key20)");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;

    const auto* doc = r.value_as<miniscript::ast::Document>();
    const auto* multi =
        doc ? cursiv::ast_cast<miniscript::ast::MultisigExpression>(
                  doc->expression)
            : nullptr;
    if (!multi || multi->keys.size() != 21) return 1;
  }
  {
    auto doc = language.shared->documentFactory->fromString(
        "multi(1,key0,key1,key2,key3,key4,key5,key6,key7,key8,key9,key10,key11,"
        "key12,key13,key14,key15,key16,key17,key18,key19,key20)",
        "file:///audit.ms");
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
  const auto language = cursiv::create_language<
      miniscript::MiniscriptCoreServices,
      miniscript::MiniscriptSharedCoreServices,
      miniscript::install_core_module>();

  for (const std::string& path : split(CURSIV_SAMPLES, ',')) {
    std::ifstream file(path);
    if (!file) return fail(path, "the sample document is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) return fail(path, "the sample does not parse");
    if (!result.diagnostics.empty())
      return fail(path, "the sample raised a diagnostic");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<miniscript::ast::Document>)
      return fail(path, "the parse built no Document");

    const auto& doc = *static_cast<const miniscript::ast::Document*>(root);
    if (doc.expression == nullptr)
      return fail(path, "the document holds no expression");

    if (ends_with(path, "/multisig.ms")) {
      if (doc.expression->type !=
          &cursiv::ast_type_tag<miniscript::ast::MultisigExpression>)
        return fail(path, "multisig.ms should build a MultisigExpression");
      const auto& multisig =
          *static_cast<const miniscript::ast::MultisigExpression*>(
              doc.expression);
      if (multisig.isMultiA) return fail(path, "multi() read as multi_a()");
      if (multisig.threshold != "2")
        return fail(path, "multi()'s threshold should read '2'");
      if (multisig.keys.size() != 3)
        return fail(path, "multi() should hold 3 keys");
      if (multisig.keys[0].substr(0, 2) != "02")
        return fail(path, "multi()'s first key should start with 02");
    }

    if (ends_with(path, "/taproot.ms")) {
      if (doc.expression->type !=
          &cursiv::ast_type_tag<miniscript::ast::MultisigExpression>)
        return fail(path, "taproot.ms should build a MultisigExpression");
      const auto& multisig =
          *static_cast<const miniscript::ast::MultisigExpression*>(
              doc.expression);
      if (!multisig.isMultiA) return fail(path, "multi_a() read as multi()");
      if (multisig.keys.size() != 3)
        return fail(path, "multi_a() should hold 3 keys");
    }

    if (ends_with(path, "/wrappers.ms")) {
      if (doc.expression->type !=
          &cursiv::ast_type_tag<miniscript::ast::WrappedExpression>)
        return fail(path, "wrappers.ms should build a WrappedExpression");
      const auto& wrapped =
          *static_cast<const miniscript::ast::WrappedExpression*>(
              doc.expression);
      if (wrapped.wrappers != "sdv")
        return fail(path, "the wrapper letters should read 'sdv'");
      if (wrapped.inner == nullptr ||
          wrapped.inner->type !=
              &cursiv::ast_type_tag<miniscript::ast::TimeExpression>)
        return fail(path, "sdv:older(1) should wrap a TimeExpression");
      const auto& time =
          *static_cast<const miniscript::ast::TimeExpression*>(
              wrapped.inner);
      if (time.op != miniscript::ast::TimeOp::older)
        return fail(path, "the wrapped expression should read older()");
      if (time.value != "1")
        return fail(path, "older()'s argument should read '1'");
    }

    std::printf("miniscript: %s parses\n", path.c_str());
  }

  return 0;
}
