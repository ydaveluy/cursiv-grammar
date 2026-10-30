#pragma once

namespace objc_followup {
inline bool run(const objc::ObjcCoreServices& services) {
  const auto check = [&](std::string_view source) {
    const auto result = services.parser->parse(source);
    const bool clean = result.accepted && !result.cst().recovered() &&
                       result.diagnostics.empty();
    if (!clean)
      std::fprintf(stderr, "objc follow-up: %.*s\n",
                   static_cast<int>(source.size()), source.data());
    return clean;
  };
  for (const auto* source : {
      "@ interface Root\n@ end\n",
      "@ interface Root\n@ property int x;\n@ end\n",
      "void f(void) { SEL s = @ selector(if:); }",
      "void f(void) { @ try {} @ catch (...) {} @ finally {} }",
      "void f(id x) { @ synchronized(x) {} @ autoreleasepool {} }",
      "int \\u03B1;",
      "int if\\u00E9;",
      "void f(void) { if\\\n (1) {} }",
      "@interface Root\n- (void)if:(int)x;\n@end\n"})
    if (!check(source)) return false;
  const std::string source = "void f(void) { SEL s = @ /*gap*/ selector(if:); }";
  const auto result = services.parser->parse(source);
  const auto selectors = objc_native::nodes<objc::ast::SelectorExpr>(result);
  if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
      selectors.size() != 1 || selectors[0]->parts.size() != 1 ||
      selectors[0]->parts[0]->name != "if" ||
      selectors[0]->parts[0]->parent != selectors[0])
    { std::fputs("objc follow-up: selector field/parent mismatch\n", stderr); return false; }
  const auto storage = services.parser->parse("int register;");
  const auto declarations = objc_native::nodes<objc::ast::Declaration>(storage);
  if (!storage.accepted || storage.cst().recovered() || !storage.diagnostics.empty() ||
      declarations.size() != 1 || !declarations[0]->declarators.empty()) {
    std::fputs("objc follow-up: register must be storage, with no declarator\n", stderr);
    return false;
  }
  for (const auto* invalid : {"int if;", "int \\u0069f;"}) {
    const auto bad = services.parser->parse(invalid);
    if (bad.accepted && !bad.cst().recovered() && bad.diagnostics.empty()) {
      std::fprintf(stderr, "objc follow-up: invalid name accepted: %s\n", invalid);
      return false;
    }
  }
  return true;
}
}  // namespace objc_followup
