#pragma once
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "native_projection.hpp"
#include <sstream>
#include <unordered_set>
namespace native_regressions {
inline int run(const auto &language) {
  int failures = 0;
  struct Case {
    const char *source;
    const char *expected;
  };
  const Case cases[] = {
      {R"FM((1) STOP . (END))FM", R"FM([["1",[["stop"]]]])FM"},
      {R"FM((1) READ-ITEM A . (END))FM", R"FM([["1",[["read_item","A"]]]])FM"},
      {R"FM((1) WRITE-ITEM B . (END))FM",
       R"FM([["1",[["write_item","B"]]]])FM"},
      {R"FM((1) TRANSFER A TO B . (END))FM",
       R"FM([["1",[["transfer","A","B"]]]])FM"},
      {R"FM((1) COMPARE ITEM (A) WITH PRICE (B) . (END))FM",
       R"FM([["1",[["compare","ITEM","A","PRICE","B"]]]])FM"},
      {R"FM((1) IF EQUAL GO TO OPERATION 5 . (END))FM",
       R"FM([["1",[["if","equal",5]]]])FM"},
      {R"FM((1) IF GREATER GO TO OPERATION 6 . (END))FM",
       R"FM([["1",[["if","greater",6]]]])FM"},
      {R"FM((1) IF END OF DATA GO TO OPERATION 7 . (END))FM",
       R"FM([["1",[["if_eof",7]]]])FM"},
      {R"FM((1) OTHERWISE GO TO OPERATION 8 . (END))FM",
       R"FM([["1",[["goto",8]]]])FM"},
      {R"FM((1) JUMP TO OPERATION 9 . (END))FM", R"FM([["1",[["goto",9]]]])FM"},
      {R"FM((1) MOVE PRICE (A) TO COST (B) . (END))FM",
       R"FM([["1",[["move","PRICE (A)","COST","B"]]]])FM"},
      {R"FM((1) SET OPERATION 4 TO GO TO OPERATION 8 . (END))FM",
       R"FM([["1",[["set_operation",4,8]]]])FM"},
      {R"FM((1) TEST CODE (A) AGAINST ZZZZ . (END))FM",
       R"FM([["1",[["test","CODE","A","ZZZZ"]]]])FM"},
      {R"FM((1) CLOSE-OUT FILES A . (END))FM",
       R"FM([["1",[["close_out",["A"]]]]])FM"},
      {R"FM((1) REWIND A . (END))FM", R"FM([["1",[["rewind","A"]]]])FM"},
      {R"FM((1) IF LESS GO TO OPERATION 9 . (END))FM",
       R"FM([["1",[["if","less",9]]]])FM"},
      {R"FM((1) IF EQUAL GO	 TO OPERATION 5 . (END))FM",
       R"FM([["1",[["if","equal",5]]]])FM"},
      {R"FM((1) IF END
 OF	 DATA GO TO OPERATION 7 .)FM",
       R"FM([["1",[["if_eof",7]]]])FM"},
      {R"FM((1) GO TO OPERATION 8 . (END))FM", R"FM([["1",[["goto",8]]]])FM"},
      {R"FM((1) PRINT-ITEM D . (END))FM",
       R"FM([["1",[["print_item","D"]]]])FM"},
      {R"FM((1) PUNCH-ITEM B . (END))FM",
       R"FM([["1",[["write_item","B"]]]])FM"},
      {R"FM((1) EXECUTE OPERATION 4 . (END))FM",
       R"FM([["1",[["execute",4,4]]]])FM"},
      {R"FM((1) EXECUTE OPERATION 4 THROUGH OPERATION 7 . (END))FM",
       R"FM([["1",[["execute",4,7]]]])FM"},
      {R"FM((1) MOVE 1.25 TO COST2 (B) . (END))FM",
       R"FM([["1",[["move","1.25","COST2","B"]]]])FM"},
      {R"FM((1) MOVE "hello world" TO TEXT (A) . (END))FM",
       R"FM([["1",[["move","\"hello world\"","TEXT","A"]]]])FM"},
      {R"FM((1) MOVE ZEROS TO COST (B) . (END))FM",
       R"FM([["1",[["move","ZEROS","COST","B"]]]])FM"},
      {R"FM((1) ADD 5 TO QTY (A) . (END))FM",
       R"FM([["1",[["add","5",null,"QTY","A"]]]])FM"},
      {R"FM((1) ADD LEFT (A) TO RIGHT (B) GIVING TOTAL (C) . (END))FM",
       R"FM([["1",[["add_giving","LEFT","A","RIGHT","B","TOTAL","C"]]]])FM"},
      {R"FM((1) SUBTRACT QTY (A) FROM TOTAL (B) . (END))FM",
       R"FM([["1",[["subtract","QTY","A","TOTAL","B"]]]])FM"},
      {R"FM((1) MULTIPLY QTY (A) BY 1.05 GIVING TOTAL (C) . (END))FM",
       R"FM([["1",[["multiply","QTY","A","1.05",null,"TOTAL","C"]]]])FM"},
      {R"FM((1) DIVIDE TOTAL (A) BY RATE (B) GIVING QTY (C) . (END))FM",
       R"FM([["1",[["divide","TOTAL","A","RATE","B","QTY","C"]]]])FM"},
      {R"FM((1) TEST NAME (A) AGAINST "bob" . (END))FM",
       R"FM([["1",[["test","NAME","A","\"bob\""]]]])FM"},
      {R"FM((1) CLOSE-OUT FILE A, B . (END))FM",
       R"FM([["1",[["close_out",["A","B"]]]]])FM"},
      {R"FM((1) read-item a . (END))FM", R"FM([["1",[["read_item","a"]]]])FM"},
      {R"FM((0) INPUT SOURCE2 FILE-A ; OUTPUT RESULT FILE-B ; HSP D . (1) STOP .)FM",
       R"FM([["0",[["input","SOURCE2","A"],["output","RESULT","B"],["hsp","HIGH-SPEED-PRINTER","D"]]],["1",[["stop"]]]])FM"},
      {R"FM(* full line comment
(1) STOP . (END))FM",
       R"FM([["1",[["stop"]]]])FM"},
      {R"FM((0001) JUMP TO OPERATION 0002 . (END))FM",
       R"FM([["1",[["goto",2]]]])FM"},
      {R"FM((123456789012345678901234567890) STOP . (END))FM",
       R"FM([["123456789012345678901234567890",[["stop"]]]])FM"},
      {R"FM((1) TEST QTY (A) AGAINST 12 . (END))FM",
       R"FM([["1",[["test","QTY","A","12"]]]])FM"},
      {R"FM((1) STOP ; (2) STOP . (END))FM",
       R"FM([["1",[["stop"]]],["2",[["stop"]]]])FM"},
  };
  for (const auto &test : cases) {
    const std::string source(test.source);
    const auto result = language.language.parser->parse(source);
    const auto *root = cursiv::ast_cast<flowmatic::ast::Program>(result.value);
    std::ostringstream actual;
    Projection(actual).tree(root);
    bool parents = root && root->parent == nullptr;
    std::unordered_set<const cursiv::AstNode *> seen;
    const auto visit = [&](auto &&self, const cursiv::AstNode &node) -> void {
      if (!seen.insert(&node).second) {
        parents = false;
        return;
      }
      language.language.astReflection->visitMembers(
          node, [&](std::string_view, std::uint32_t, cursiv::AstNode &child) {
            parents &= child.parent == &node &&
                       result.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    if (root)
      visit(visit, *root);
    parents &= seen.size() == result.nodes.size();
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || actual.str() != test.expected ||
        !parents ||
        !cursiv::test::validationHelper(language.language, source)
             .diagnostics.empty()) {
      std::fprintf(stderr, "flowmatic: wrong native fields: %s\n", test.source);
      ++failures;
    }
  }
  for (const auto &test :
       {std::pair{"(1) MULTIPLY QTY (A) BY 2 .", "flowmatic.arithmetic-result"},
        std::pair{"(1) SUBTRACT 1 FROM TOTAL (B) .",
                  "flowmatic.arithmetic-left"}}) {
    const auto result = language.language.parser->parse(test.first);
    const auto validation =
        cursiv::test::validationHelper(language.language, test.first);
    bool found = false;
    for (const auto &d : validation.diagnostics)
      found |= d.code && d.code->isString() && d.code->string() == test.second;
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !found)
      ++failures;
  }
  {
    const auto result =
        language.language.parser->parse("(0001) JUMP TO OPERATION 0002 .");
    const auto *program =
        cursiv::ast_cast<flowmatic::ast::Program>(result.value);
    const auto *jump = program && !program->lines.empty() &&
                               !program->lines[0]->statements.empty()
                           ? cursiv::ast_cast<flowmatic::ast::JumpToStmt>(
                                 program->lines[0]->statements[0])
                           : nullptr;
    if (!jump || program->lines[0]->label != "0001" || jump->target != "0002")
      ++failures;
  }
  for (const auto &text : {"(1) WRITE-ITEM B .", "(1) PUNCH-ITEM B ."}) {
    const auto result = language.language.parser->parse(text);
    const auto *program =
        cursiv::ast_cast<flowmatic::ast::Program>(result.value);
    const auto *write = program && !program->lines.empty() &&
                                !program->lines[0]->statements.empty()
                            ? cursiv::ast_cast<flowmatic::ast::WriteItemStmt>(
                                  program->lines[0]->statements[0])
                            : nullptr;
    if (!write || write->punched != (std::string_view(text).find("PUNCH") != std::string_view::npos))
      ++failures;
  }
  return failures;
}
} // namespace native_regressions
