// The smoke test of the language: parse every file under example/ and check
// each one comes out clean -- no repaired input, no diagnostics, the right
// root type.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "focal/core/CoreServices.hpp"
#include "focal/core/ast.hpp"

namespace {

int fail(const char* what) {
  std::fprintf(stderr, "focal: %s\n", what);
  return 1;
}

}  // namespace

int main() {
  {
    const auto audit_language = cursiv::create_language<
      focal::FocalCoreServices, focal::FocalSharedCoreServices,
      focal::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse("01.01 SET X=-1.5\n");
    if (!audit_result.accepted || audit_result.cst().recovered() || !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
    const auto* program = audit_result.value_as<focal::ast::Program>();
    if (!program || program->statements.size() != 1) return 1;
    const auto* command = cursiv::ast_cast<focal::ast::SetCommand>(program->statements[0]->command);
    const auto* value = command ? cursiv::ast_cast<focal::ast::NumberLiteral>(command->value) : nullptr;
    if (!value || value->value != -1.5) return 1;
  }


  const auto language = cursiv::create_language<
      focal::FocalCoreServices, focal::FocalSharedCoreServices,
      focal::install_core_module>();

  {
    const std::string source =
        "01.01 FOR A=1,2,9;TYPE A\n"
        "01.02 FOR B=1,9;TYPE B\n"
        "01.03 FOR C=1;TYPE C\n"
        "01.04 SET X=X/2*Y+2*3^2\n"
        "01.05 TYPE FSIN(1E2)\n"
        "01.06 IF (X)1.8\n"
        "01.07 IF (X)1.8,1.9\n"
        "01.08 C a native short comment\n"
        "01.09 ASK X,Y,Z\n"
        "01.10 TYPE X!\n"
        "01.11 DO;SET X(2)=FRAN();WRITE ALL;GO\n"
        "01.12 TYPE %,1,%5,2,%5.2,3,$\n";
    const auto result = language.language.parser->parse(source);
    const auto* program = result.value_as<focal::ast::Program>();
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !program || program->statements.size() != 12)
      return fail("DEC manual examples must parse cleanly");
    const auto loop = [&](std::size_t i) {
      return cursiv::ast_cast<focal::ast::ForCommand>(program->statements[i]->command);
    };
    const auto number = [](const focal::ast::Expression* e, double value) {
      const auto* n = cursiv::ast_cast<focal::ast::NumberLiteral>(e);
      return n && n->value == value;
    };
    if (!loop(0) || !number(loop(0)->from, 1) || !number(loop(0)->step, 2) ||
        !number(loop(0)->to, 9) || !loop(1) || loop(1)->step ||
        !number(loop(1)->to, 9) || !loop(2) || loop(2)->step || loop(2)->to)
      return fail("FOR start, increment and limit must retain their native roles");
    const auto* set = cursiv::ast_cast<focal::ast::SetCommand>(program->statements[3]->command);
    const auto* sum = set ? cursiv::ast_cast<focal::ast::BinaryExpression>(set->value) : nullptr;
    const auto* division = sum ? cursiv::ast_cast<focal::ast::BinaryExpression>(sum->left) : nullptr;
    const auto* denominator = division ? cursiv::ast_cast<focal::ast::BinaryExpression>(division->right) : nullptr;
    const auto* multiplication = sum ? cursiv::ast_cast<focal::ast::BinaryExpression>(sum->right) : nullptr;
    const auto* power = multiplication ? cursiv::ast_cast<focal::ast::BinaryExpression>(multiplication->right) : nullptr;
    if (!sum || sum->op != focal::ast::Operator::add || !division ||
        division->op != focal::ast::Operator::divide || !denominator ||
        denominator->op != focal::ast::Operator::multiply || !multiplication ||
        multiplication->op != focal::ast::Operator::multiply || !power ||
        power->op != focal::ast::Operator::power)
      return fail("DEC FOCAL power, multiplication and division priorities");
    const auto* type = cursiv::ast_cast<focal::ast::TypeCommand>(program->statements[4]->command);
    const auto* expr = type && type->items.size() == 1 ?
        std::get_if<focal::ast::Expression*>(&type->items[0]) : nullptr;
    const auto* call = expr ? cursiv::ast_cast<focal::ast::BuiltinCall>(*expr) : nullptr;
    if (!call || call->name != "FSIN" || !number(call->arg, 100))
      return fail("uppercase native functions and E notation");
    const auto* one = cursiv::ast_cast<focal::ast::IfCommand>(program->statements[5]->command);
    const auto* two = cursiv::ast_cast<focal::ast::IfCommand>(program->statements[6]->command);
    if (!one || !one->negative || one->zero || one->positive ||
        !two || !two->zero || two->positive)
      return fail("IF permits one or two destinations");
    const auto* ask = cursiv::ast_cast<focal::ast::AskCommand>(program->statements[8]->command);
    const auto* print = cursiv::ast_cast<focal::ast::TypeCommand>(program->statements[9]->command);
    if (!ask || ask->pairs.size() != 1 || ask->pairs[0]->variables.size() != 3 ||
        !ask->pairs[0]->prompt.empty() || !print || print->items.size() != 2 ||
        !std::holds_alternative<focal::ast::BangRun>(print->items[1]))
      return fail("ASK without prompt and adjacent TYPE punctuation");
    const auto* subroutine = cursiv::ast_cast<focal::ast::DoCommand>(program->statements[10]->command);
    const auto* indexed = subroutine ? cursiv::ast_cast<focal::ast::SetCommand>(subroutine->next) : nullptr;
    const auto* random = indexed ? cursiv::ast_cast<focal::ast::BuiltinCall>(indexed->value) : nullptr;
    const auto* write = indexed ? cursiv::ast_cast<focal::ast::WriteCommand>(indexed->next) : nullptr;
    if (!subroutine || subroutine->target || subroutine->group || !indexed ||
        !number(indexed->index, 2) || !random || random->name != "FRAN" ||
        random->arg || !write || !write->all ||
        !cursiv::ast_cast<focal::ast::GotoCommand>(write->next))
      return fail("native command chaining, indexed SET, bare DO and FRAN()");
    const auto* formats = cursiv::ast_cast<focal::ast::TypeCommand>(program->statements[11]->command);
    if (!formats || formats->items.size() != 7) return fail("TYPE format alternatives");
    const auto* floating = std::get_if<focal::ast::FormatSpec*>(&formats->items[0]);
    const auto* fixed = std::get_if<focal::ast::FormatSpec*>(&formats->items[2]);
    const auto* decimal = std::get_if<focal::ast::FormatSpec*>(&formats->items[4]);
    if (!floating || (*floating)->hasWidth || !fixed || !(*fixed)->hasWidth ||
        (*fixed)->hasDecimals || (*fixed)->width != 5 || !decimal ||
        !(*decimal)->hasDecimals || (*decimal)->decimals != 2 ||
        !std::holds_alternative<focal::ast::DumpVariables*>(formats->items[6]))
      return fail("TYPE format presence and symbol-table marker are retained");
  }

  int checked = 0;
  // extras.focal lives beside this test, not in the corpus: it exercises
  // the abbreviations, builtins and grouping forms example1.focal does not.
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".focal")
        continue;

      std::ifstream file(entry.path());
      if (!file) return fail("an example is not readable");
      const std::string text((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

      const cursiv::ParseResult result =
          language.language.parser->parse(text);
      if (result.cst().recovered())
        return fail("an example does not parse clean");
      if (!result.diagnostics.empty())
        return fail("an example parses with diagnostics");

      const cursiv::AstNode* const root = result.value;
      if (root == nullptr ||
          root->type != &cursiv::ast_type_tag<focal::ast::Program>)
        return fail("the parse built no program");

      ++checked;
    }
  if (checked == 0) return fail("no example was found");

  std::printf("focal: %d examples parse\n", checked);
  return 0;
}
