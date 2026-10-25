#pragma once

#include <cstdio>
#include <string>

#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "srt/core/ast.hpp"

inline bool native_regressions(const auto& services) {
  namespace ast = srt::ast;
  bool ok = true;
  const auto check = [&](bool value, const char* message) {
    if (!value) {std::fprintf(stderr, "srt: %s\n", message);ok = false;}
  };
  const std::string source =
      "\xEF\xBB\xBFmetadata\r\n"
      "000000000000000000000000000000000001\r\n"
      "0:0:1.5\t-->\t0:0:2,67 X1:10 X2:20 Y1:30 Y2:40\r\n"
      "<i>café</i>\r\n\r\n\xEF\xBB\xBFtext\r\n2026\r\nlast\r\n"
      "0:0:3,0-->0:0:4,0\r\nsecond";
  const auto result = services.parser->parse(source);
  check(result.accepted && !result.cst().recovered() && result.diagnostics.empty(), "native input parses cleanly");
  const auto* doc = cursiv::ast_cast<ast::Document>(result.value);
  check(doc && doc->preamble.size() == 1 && doc->preamble[0] == "metadata" &&
        doc->subtitles.size() == 2, "preamble, initial BOM and both cues");
  if (doc && doc->subtitles.size() == 2) {
    const auto* first = doc->subtitles[0];
    const auto* second = doc->subtitles[1];
    check(first->number && *first->number == "000000000000000000000000000000000001", "index spelling cannot overflow");
    check(first->start == "0:0:1.5" && first->end == "\t0:0:2,67" &&
          first->settings == " X1:10 X2:20 Y1:30 Y2:40", "timestamps and settings remain source text");
    check(first->lines.size() == 4 && first->lines[0] == "<i>café</i>" &&
          first->lines[1] == "\xEF\xBB\xBFtext" && first->lines[2] == "2026" &&
          first->lines[3] == "last", "caption markup, internal BOM and numeric line retained");
    check(!second->number && second->start == "0:0:3,0" && second->end == "0:0:4,0" &&
          second->lines.size() == 1 && second->lines[0] == "second", "unnumbered cue without blank separator or final newline");
    check(first->parent == doc && second->parent == doc && result.nodes.size() == 3, "all cue nodes owned by document");
  }
  for (std::string text : {"", "metadata\n", "1\n0:0:1,0-->0:0:2,0\nhello",
                          "0:0:1,0 --> 0:0:2,0\nhello\n",
                          "1\r0:0:1,0 --> 0:0:2,0\rhello\r",
                          "9\n0:99:99,1234 --> 2:0:0,12345\nhello\n",
                          "2 garbage\n0:0:1,0 --> 0:0:2,0\nhello\n",
                          "-1\n-1:+2:-3,+4 --> +1:-2:+3,-4\nhello\n",
                          "\xEF\xBB\xBF" "0:0:1,0 --> 0:0:2,0\nhello\n"}) {
    const auto validated = cursiv::test::validationHelper(services, text);
    check(validated.diagnostics.empty(), "native input has no spurious numbering or time-format error");
  }
  const auto negative = services.parser->parse(
      "1\n0:0:1,0 --> 0:0:2,0\ncaption\n-1\n0:0:3,0 --> 0:0:4,0\nnext\n");
  const auto* captions = cursiv::ast_cast<ast::Document>(negative.value);
  check(!negative.cst().recovered() && captions && captions->subtitles.size() == 2 &&
        captions->subtitles[0]->lines.size() == 2 &&
        captions->subtitles[0]->lines[1] == "-1" && !captions->subtitles[1]->number,
        "negative numeric caption remains in its cue before an unnumbered header");
  for (const char* line : {"2 caption", "2 ", "-000caption"}) {
    const std::string text = std::string("0:0:1,0 --> 0:0:2,0\n") + line +
        "\n0:0:3,0 --> 0:0:4,0\nnext\n";
    const auto parsed = services.parser->parse(text);
    const auto* d = cursiv::ast_cast<ast::Document>(parsed.value);
    check(!parsed.cst().recovered() && d && d->subtitles.size() == 2 &&
          d->subtitles[0]->lines.size() == 1 && d->subtitles[0]->lines[0] == line &&
          !d->subtitles[1]->number, "numeric text before header belongs to otherwise empty caption");
  }
  const auto double_bom = services.parser->parse(
      "\xEF\xBB\xBF\xEF\xBB\xBFmetadata\n1\n0:0:1,0 --> 0:0:2,0\nhello\n");
  const auto* boms = cursiv::ast_cast<ast::Document>(double_bom.value);
  check(!double_bom.cst().recovered() && boms && boms->preamble.size() == 1 &&
        boms->preamble[0] == "\xEF\xBB\xBFmetadata" && boms->subtitles.size() == 1,
        "only the first BOM is a file marker");
  for (std::string text : {"1\n0:0:0,2147483648 --> 0:0:1,0\ntext\n",
                          "1\n0:-2147483649:0,0 --> 0:0:1,0\ntext\n"}) {
    const auto parsed = services.parser->parse(text);
    check(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty(), "unbounded number reaches runtime");
    const auto validated = cursiv::test::validationHelper(services, text);
    check(validated.diagnostics.size() == 1 &&
          validated.diagnostics[0].message == "A timestamp component must fit a signed 32-bit integer." &&
          validated.diagnostics[0].range.begin < validated.diagnostics[0].range.end,
          "precise timestamp component error");
  }
  return ok;
}
