void native_regressions() {
  {
    const std::string source = "GCS a42 a0000000000000000000000000000001";
    const auto result = build(source);
    const auto* file = result.value_as<ast::File>();
    CHECK(file && file->tuples.size() == 2);
    if (file && file->tuples.size() == 2) {
      CHECK(std::get<std::string_view>(file->tuples[0]->kind) == "a");
      CHECK(file->tuples[0]->exactAge == "42");
      CHECK(file->tuples[1]->exactAge == "0000000000000000000000000000001");
    }
    CHECK(!accepts("GCS C42"));
    CHECK(!accepts("GCS a 42"));
  }
  const std::string source = "-----BEGIN GEEK CODE BLOCK-----\nVersion: 3.12\n"
      "GCS UB++ dpu dx r% P! h! s+(-)>--:(--)>--- C()\n"
      "------END GEEK CODE BLOCK------\n";
  const auto parsed = build(source);
  const auto* file = parsed.value_as<ast::File>();
  CHECK(file && file->header == "GCS" && file->version == "3.12");
  CHECK(file && file->tuples.size() == 8);
  if (!file || file->tuples.size() != 8) return;
  const auto* unixKind = std::get<ast::Unix*>(file->tuples[0]->kind);
  CHECK(unixKind && unixKind->variants.size() == 1 && unixKind->variants[0] == 'B');
  CHECK(std::get<std::string_view>(file->tuples[1]->modifiers[0]) == "pu");
  CHECK(std::get<std::string_view>(file->tuples[2]->modifiers[0]) == "x");
  CHECK(std::get<std::string_view>(file->tuples[3]->modifiers[0]) == "%");
  CHECK(std::get<ast::Qualifier>(file->tuples[4]->modifiers[0]) == ast::Qualifier::refused);
  const auto* shape = std::get<ast::Shape*>(file->tuples[6]->kind);
  CHECK(shape && shape->segments.size() == 2);
  CHECK(shape && shape->segments[0]->modifiers.size() == 4);
  CHECK(shape && shape->segments[1]->modifiers.size() == 3);
  const auto* range = std::get<ast::Crossover*>(file->tuples[7]->modifiers[0]);
  CHECK(range && range->run.empty());
  const std::string separate = "GCS d x";
  const auto separated = build(separate);
  CHECK(separated.value_as<ast::File>()->tuples.size() == 2);
  auto language = cursiv::create_language<geekcode::GeekcodeCoreServices,
      geekcode::GeekcodeSharedCoreServices, geekcode::install_core_module>();
  CHECK(cursiv::test::validationHelper(language.language, source).diagnostics.empty());
  for (std::string_view invalid : {"GCS C%", "GCS Ppu", "GCS Lx", "GCS s%"}) {
    CHECK(accepts(invalid));
    CHECK(!cursiv::test::validationHelper(language.language, std::string(invalid)).diagnostics.empty());
  }
}
