#pragma once
inline void repeated_alignment(const auto& parse, const auto& validate,
                               const auto& check) {
  struct Case {
    std::string_view source, code, badToken;
    bool alloca;
  };
  constexpr Case cases[] = {
      {R"ir(define i32 @f(ptr %p) { %r = load i32, ptr %p, align 4, align 8 ret i32 %r })ir",
       "", "", false},
      {R"ir(define i32 @f(ptr %p) { %r = load atomic i32, ptr %p acquire, align 4, align 8 ret i32 %r })ir",
       "", "", false},
      {R"ir(define void @f(ptr %p) { store i32 1, ptr %p, align 4, align 8 ret void })ir",
       "", "", false},
      {R"ir(define void @f(ptr %p) { store atomic i32 1, ptr %p release, align 4, align 8 ret void })ir",
       "", "", false},
      {R"ir(define i32 @f(ptr %p) { %r = atomicrmw add ptr %p, i32 1 monotonic, align 4, align 8 ret i32 %r })ir",
       "", "", false},
      {R"ir(define { i32, i1 } @f(ptr %p) { %r = cmpxchg ptr %p, i32 0, i32 1 monotonic monotonic, align 4, align 8 ret { i32, i1 } %r })ir",
       "", "", false},
      {R"ir(define ptr addrspace(2) @f() { %r = alloca i32, align 4, addrspace(1), addrspace(2) ret ptr addrspace(2) %r })ir",
       "", "", true},
      {R"ir(define ptr addrspace(2) @f() { %r = alloca i32, i32 3, align 4, addrspace(1), addrspace(2) ret ptr addrspace(2) %r })ir",
       "", "", true},
      {R"ir(define i32 @f(ptr %p) { %r = load i32, ptr %p, align 4, align 8, !tag !0 ret i32 %r }
!0 = !{})ir",
       "", "", false},
      {R"ir(define i32 @f(ptr %p) { %r = load i32, ptr %p, align 3, align 8 ret i32 %r })ir",
       "llvmir.alignment", "3", false},
      {R"ir(define ptr addrspace(2) @f() { %r = alloca i32, align 4, addrspace(16777216), addrspace(2) ret ptr addrspace(2) %r })ir",
       "llvmir.address-space", "16777216", false},
  };
  for (const auto& c : cases) {
    const auto result = parse(c.source);
    const auto diagnostics = validate(c.source);
    if (!c.code.empty()) {
      check(diagnostics.size() == 1, "invalid annotation before valid one");
      if (diagnostics.size() == 1) {
        const auto& d = diagnostics[0];
        check(d.code && *d.code == c.code &&
                  c.source.substr(d.range.begin, d.range.end - d.range.begin) ==
                      c.badToken,
              "invalid annotation code and exact range");
      }
      continue;
    }
    const auto* module = result.template value_as<ast::Module>();
    const auto* function = module && !module->entities.empty()
        ? cursiv::ast_cast<ast::FuncDef>(module->entities[0]) : nullptr;
    const auto* instruction = function && function->body &&
        !function->body->blocks.empty() &&
        !function->body->blocks[0]->instructions.empty()
        ? cursiv::ast_cast<ast::GenericInst>(
              function->body->blocks[0]->instructions[0]) : nullptr;
    check(instruction && instruction->align.size() == (c.alloca ? 1 : 2) &&
              instruction->align[0] == "4" &&
              instruction->align.back() == (c.alloca ? "4" : "8"),
          "all written alignments are preserved in order");
    if (c.alloca)
      check(instruction && instruction->addrSpace.size() == 2 &&
                instruction->addrSpace[0] == "1" &&
                instruction->addrSpace[1] == "2",
            "all address spaces are preserved in order");
    check(diagnostics.empty(), "valid repeated annotations");
  }
}
