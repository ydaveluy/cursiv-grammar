#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "tcpheader/core/CoreServices.hpp"
#include "tcpheader/core/ast.hpp"
#include "tcpheader/generated/Tcpheader/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = tcpheader::generated::Tcpheader::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = tcpheader::ast;

  std::string text(20, '\0');
  text[0] = ' ';
  text[1] = ' ';
  text[8] = '\t';
  text[9] = '\n';
  auto r = parser->parse(text);
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
        "binary header whitespace bytes");
  auto *d = as<ast::SegmentHeader>(r.value);
  check(d && d->sourcePort == "  " && d->sequenceNumber.size() == 4 &&
            d->ackNumber.size() == 4 && d->ackNumber[0] == '\t' &&
            d->ackNumber[1] == '\n',
        "binary fields keep every byte");

  const auto language = cursiv::create_language<tcpheader::TcpheaderCoreServices,
      tcpheader::TcpheaderSharedCoreServices, tcpheader::install_core_module>();
  for (unsigned words : {5, 6, 8, 15}) {
    std::string header(4 * words, '\0');
    header[12] = static_cast<char>(words << 4);
    for (std::size_t i = 20; i < header.size(); ++i)
      header[i] = static_cast<char>(i * 71);
    const auto parsed = parser->parse(header);
    const auto* value = parsed.value_as<ast::SegmentHeader>();
    check(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty() &&
              value && value->options == std::string_view(header).substr(20),
          "TCP option bytes");
    check(cursiv::test::validationHelper(language.language, header).diagnostics.empty(),
          "valid header data offset");
  }
  for (unsigned words : {0, 4, 6, 15}) {
    std::string header(20, '\0');
    header[12] = static_cast<char>(words << 4);
    const auto parsed = parser->parse(header);
    check(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty(),
          "invalid data offset remains parseable");
    const auto validation = cursiv::test::validationHelper(language.language, header);
    check(validation.diagnostics.size() == 1 && validation.diagnostics[0].code &&
              validation.diagnostics[0].code->isString() &&
              validation.diagnostics[0].code->string() == "tcpheader.length" &&
              validation.diagnostics[0].range.begin == 12 &&
              validation.diagnostics[0].range.end == 14,
          "data offset diagnostic span");
  }

  return ok;
}
} // namespace audit_regressions
