#pragma once

#include <cstdio>
#include <string>
#include <string_view>

#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "fasta/core/ast.hpp"

inline bool native_regressions(const auto& services) {
  namespace ast = fasta::ast;
  bool ok = true;
  const auto check = [&](bool value, const char* message) {
    if (!value) {std::fprintf(stderr, "fasta: %s\n", message);ok = false;}
  };
  const auto parsed = services.parser->parse(
      "metadata\r\n;preamble\r\n>α  \t\r\n\r\nAC -.*?12\r\n;retained\r\nTG\r\n>second\rCC");
  check(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty(),
        "records with punctuation, whitespace and mixed line endings parse cleanly");
  const auto* document = cursiv::ast_cast<ast::Document>(parsed.value);
  check(document && document->preamble.size() == 2 && document->records.size() == 2,
        "preamble and both records retained");
  if (document && document->preamble.size() == 2 && document->records.size() == 2) {
    const auto* first = document->records[0];
    const auto* second = document->records[1];
    check(first && first->description && first->description->text == "α  \t" &&
              first->body.size() == 4, "raw header and all body lines retained");
    if (first && first->description && first->body.size() == 4) {
      check(cursiv::ast_cast<ast::BlankLine>(first->body[0]) != nullptr,
            "blank line retained");
      const auto* sequence = cursiv::ast_cast<ast::SequenceLine>(first->body[1]);
      const auto* comment = cursiv::ast_cast<ast::CommentLine>(first->body[2]);
      check(sequence && sequence->text == "AC -.*?12" &&
                comment && comment->text == "retained", "raw sequence and comment");
    }
    const auto* last = second && second->body.size() == 1 ?
        cursiv::ast_cast<ast::SequenceLine>(second->body[0]) : nullptr;
    check(second && second->description && second->description->text == "second" &&
              last && last->text == "CC", "bare CR and final line without newline");
    check(parsed.nodes.size() == 12, "every allocated node belongs to the source tree");
    for (const auto* line : document->preamble) check(line->parent == document, "preamble parent");
    for (const auto* record : document->records) {
      check(record->parent == document && record->description->parent == record, "record parents");
      for (const auto* line : record->body) check(line->parent == record, "body parent");
    }
  }
  {
    const auto source = services.parser->parse(">id|free text\n ;literal\n A>C;D\n");
    const auto* d = source.template value_as<ast::Document>();
    const auto* r = d && d->records.size() == 1 ? d->records[0] : nullptr;
    const auto* first = r && r->body.size() == 2 ? cursiv::ast_cast<ast::SequenceLine>(r->body[0]) : nullptr;
    const auto* second = r && r->body.size() == 2 ? cursiv::ast_cast<ast::SequenceLine>(r->body[1]) : nullptr;
    check(source.accepted && !source.cst().recovered() && source.diagnostics.empty() &&
          r && r->description->text == "id|free text" && first && first->text == " ;literal" &&
          second && second->text == " A>C;D", "only column-one header/comment markers are structural");
  }
  for (std::string text : {"", "metadata\n;comment\n", ">\n>second", ">id\nACD-EF.*?123xyz\n",
                           ">id\nA\tC G\t\n", ">séquence\n;commenté\nACGT\n",
                           "métadonnées\n>id\nACT\n"}) {
    const auto result = cursiv::test::validationHelper(services, text);
    check(result.diagnostics.empty(), "native Pearson input has no spurious diagnostic");
  }
  for (std::string text : {">id\nACéGT\n", ">id\nACαGT\n", ">id\nAC🙂GT\n"}) {
    const auto p = services.parser->parse(text);
    check(p.accepted && !p.cst().recovered() && p.diagnostics.empty(),
          "non-ASCII sequence reaches runtime validation");
    const auto result = cursiv::test::validationHelper(services, text);
    check(result.diagnostics.size() == 1 &&
              result.diagnostics[0].message == "Sequence data must be ASCII." &&
              result.diagnostics[0].range.begin == 4 &&
              result.diagnostics[0].range.end == text.size() - 1,
          "non-ASCII sequence diagnostic selects the source line");
  }
  return ok;
}
