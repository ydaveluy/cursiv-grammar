// The grammar end to end: every document under example/ parses clean
// (recovered() false, no diagnostics, the right root type), and a few real
// field values come back out of one document's AST.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "eiffel/core/ast.hpp"
#include "eiffel/generated/Eiffel/Module.hpp"
#include "eiffel/generated/Eiffel/AstReflection.hpp"
#include "lexical_cases.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const std::string& what) {
  std::fprintf(stderr, "eiffel: %s\n", what.c_str());
  return 1;
}

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      eiffel::generated::Eiffel::create_parser();
  return *p;
}

std::string read_file(const fs::path& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

}  // namespace

int main() {
  const auto reflection = eiffel::generated::Eiffel::create_ast_reflection();
  if (!eiffel_lexical::run(engine(), *reflection)) return 1;
  std::vector<fs::path> docs;
  // `features.e` and `inspect.e` live beside this test, not in the corpus:
  // they are written here for the constructs the corpus never reaches.
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const fs::directory_entry& entry :
        fs::recursive_directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".e")
        docs.push_back(entry.path());
  if (docs.empty()) return fail("no example documents found");

  fs::path readback_doc;
  for (const fs::path& doc : docs) {
    const std::string text = read_file(doc);
    const cursiv::ParseResult result = engine().parse(text);
    if (!result.accepted)
      return fail(doc.string() + ": the parse was refused");
    if (result.cst().recovered())
      return fail(doc.string() + ": the parse needed repair");
    if (!result.diagnostics.empty())
      return fail(doc.string() + ": the parse left diagnostics");
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<eiffel::ast::ClassDecl>)
      return fail(doc.string() + ": the parse built no class declaration");
    if (doc.filename() == "application.e" &&
        doc.parent_path().filename() == "100_doors")
      readback_doc = doc;
  }

  if (readback_doc.empty())
    return fail("100_doors/application.e was not found among the examples");

  // Read a few real field values out of one document's AST: a converter or a
  // link that silently misfires would still leave `recovered()` false and
  // the root type right, so only reading the fields back catches it.
  const std::string text = read_file(readback_doc);
  const cursiv::ParseResult result = engine().parse(text);
  const auto& cls = *static_cast<const eiffel::ast::ClassDecl*>(result.value);
  if (cls.name != "APPLICATION")
    return fail("100_doors/application.e: wrong class name");
  if (cls.creators.size() != 1 || cls.creators[0]->procedures.size() != 1 ||
      cls.creators[0]->procedures[0] != "make")
    return fail("100_doors/application.e: wrong creation clause");
  if (cls.features.empty())
    return fail("100_doors/application.e: no feature clauses");
  bool found_initialize = false;
  for (const eiffel::ast::FeatureClause* const clause : cls.features)
    for (const eiffel::ast::FeatureDeclaration* const decl : clause->features)
      for (const eiffel::ast::NewFeatureName* const name : decl->names)
        if (name->name == "initialize_closed_doors") found_initialize = true;
  if (!found_initialize)
    return fail("100_doors/application.e: missing initialize_closed_doors");


  {
    namespace ast = eiffel::ast;
    const auto native = engine().parse(R"e(class C [G -> BASE rename old_name as new_name end,
                     H -> {FIRST rename a as b end, SECOND rename c as d end}]
feature
  make
    do
      create x.make (1, 2)
      create Result
      from variant remaining: 10 until done loop step variant after_step: 9 end
    end
end)e");
    const auto* type = native.value_as<ast::ClassDecl>();
    if (!native.accepted || native.cst().recovered() || !native.diagnostics.empty() ||
        !type || type->generics.size() != 2 || type->features.size() != 1)
      return fail("native generic constraints and creation: unclean parse");
    const auto* first = type->generics[0];
    const auto* second = type->generics[1];
    if (first->constraints.size() != 1 || first->constraints[0]->renames.size() != 1 ||
        first->constraints[0]->renames[0]->from != "old_name" ||
        first->constraints[0]->renames[0]->to != "new_name" || second->constraints.size() != 2 ||
        second->constraints[0]->renames.size() != 1 || second->constraints[1]->renames.size() != 1 ||
        second->constraints[0]->renames[0]->to != "b" || second->constraints[1]->renames[0]->to != "d")
      return fail("generic constraint renames lost their associated type");
    const auto* declaration = type->features[0]->features.size() == 1
        ? type->features[0]->features[0] : nullptr;
    const auto* body = declaration && declaration->value && declaration->value->body
        ? declaration->value->body->body : nullptr;
    if (!body || body->items.size() != 3) return fail("native instruction count");
    const auto* creation = cursiv::ast_cast<ast::CreationInstruction>(body->items[0]);
    const auto* target = creation ? cursiv::ast_cast<ast::NameExpr>(creation->target) : nullptr;
    if (!creation || !target || target->name != "x" || target->called ||
        creation->member != "make" || !creation->called || creation->args.size() != 2)
      return fail("creation procedure and arguments were absorbed into its target");
    const auto* result_creation = cursiv::ast_cast<ast::CreationInstruction>(body->items[1]);
    if (!result_creation || !cursiv::ast_cast<ast::ResultExpr>(result_creation->target) ||
        !result_creation->member.empty()) return fail("creation target Result");
    const auto* loop = cursiv::ast_cast<ast::LoopInstruction>(body->items[2]);
    if (!loop || loop->variantBeforeTag != "remaining" || loop->variantAfterTag != "after_step" ||
        !loop->variantBefore || !loop->variantAfter) return fail("loop variant tags were lost");
  }

  {
    const auto native = engine().parse(R"e(class C feature
      text: STRING = "tag[
This contains ]" and a quoted "word".
]tag"
    end)e");
    const auto* type = native.value_as<eiffel::ast::ClassDecl>();
    if (!native.accepted || native.cst().recovered() || !native.diagnostics.empty() ||
        !type || type->features.size() != 1 || type->features[0]->features.size() != 1 ||
        type->features[0]->features[0]->value->explicitValue !=
            R"e("tag[
This contains ]" and a quoted "word".
]tag")e")
      return fail("matching labelled verbatim string was truncated or refused");
  }
  {
    const auto parsed = engine().parse("class C feature text: STRING = \"prefix[content\" end");
    if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty())
      return fail("ordinary string containing [ was misread as a labelled verbatim opener");
  }
  std::printf("eiffel: %zu documents parse clean\n", docs.size());
  return 0;
}
