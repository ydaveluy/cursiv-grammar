static int native_regressions() {
  const auto language = cursiv::create_language<rfc3080::Rfc3080CoreServices,
      rfc3080::Rfc3080SharedCoreServices, rfc3080::install_core_module>();
  std::string source = "ANS\t0002147483647 0001 . 0004294967295 0003 0002147483647\r\n";
  source.append("x\0y", 3);
  source += "END\r\n";
  const auto parsed = language.language.parser->parse(source);
  CHECK(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty());
  const auto* frame = parsed.value_as<rfc3080::ast::Frame>();
  CHECK(frame && frame->channel == "0002147483647" && frame->seqno == "0004294967295");
  CHECK(frame && frame->size == "0003" && frame->ansno == "0002147483647");
  CHECK(frame && frame->payload == std::string_view("x\0y", 3));
  CHECK(cursiv::test::validationHelper(language.language, source).diagnostics.empty());
  struct Invalid { const char* source; const char* code; };
  for (const auto& invalid : {
      Invalid{"MSG 2147483648 0 . 0 0\r\nEND\r\n", "beep.channel"},
      Invalid{"MSG 0 2147483648 . 0 0\r\nEND\r\n", "beep.msgno"},
      Invalid{"MSG 0 0 . 4294967296 0\r\nEND\r\n", "beep.seqno"},
      Invalid{"MSG 0 0 . 0 18446744073709551616\r\nEND\r\n", "beep.size"},
      Invalid{"ANS 0 0 . 0 0 2147483648\r\nEND\r\n", "beep.ansno"},
      Invalid{"MSG -1 0 . 0 0\r\nEND\r\n", "beep.channel"},
      Invalid{"NUL 0 0 * 0 0\r\nEND\r\n", "beep.nul.more"},
      Invalid{"NUL 0 0 . 0 1\r\nxEND\r\n", "beep.nul.size"},
      Invalid{"ANS 0 0 . 0 0\r\nEND\r\n", "beep.ansno.missing"},
      Invalid{"MSG 0 0 . 0 0 7\r\nEND\r\n", "beep.ansno.unexpected"},
      Invalid{"MSG 0 0 . 0 2\r\nxEND\r\n", "beep.payload.size"}}) {
    const auto result = language.language.parser->parse(invalid.source);
    CHECK(result.accepted && !result.cst().recovered() && result.diagnostics.empty());
    const auto validation = cursiv::test::validationHelper(language.language, invalid.source);
    bool found = false;
    for (const auto& diagnostic : validation.diagnostics)
      found |= diagnostic.code && diagnostic.code->isString() && diagnostic.code->string() == invalid.code;
    CHECK(found);
  }
  return cursiv::test::report("rfc3080_native_regressions");
}
