#pragma once

#include <functional>
#include <unordered_set>

#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"

namespace code_audit {
namespace ast = llvmir::ast;
#include "repeated_alignment.hpp"
inline bool run() {
  const auto language =
      cursiv::create_language<llvmir::LlvmIrCoreServices,
                              llvmir::LlvmIrSharedCoreServices,
                              llvmir::install_core_module>();
  bool ok = true;
  auto check = [&](bool value, const char* message) {
    if (!value) {
      std::fprintf(stderr, "llvmir code audit: %s\n", message);
      ok = false;
    }
  };
  auto parse = [&](std::string_view source) {
    auto result = language.language.parser->parse(source);
    check(result.accepted && !result.cst().recovered() &&
              result.diagnostics.empty(),
          "clean parse");
    check(result.value_as<ast::Module>() != nullptr, "Module root");
    std::unordered_set<const cursiv::AstNode*> reached;
    std::function<void(const cursiv::AstNode*, const cursiv::AstNode*)> visit =
        [&](const auto* n, const auto* parent) {
          if (!n) return;
          check(n->parent == parent, "AST parents");
          if (!reached.insert(n).second) {
            check(false, "unique ownership");
            return;
          }
          language.language.astReflection->visitMembers(
              *n, [&](auto, auto, auto& child) { visit(&child, n); });
        };
    visit(result.value, nullptr);
    check(reached.size() == result.nodes.size(),
          "every allocated node is reachable");
    return result;
  };
  auto validate = [&](std::string_view source) {
    cursiv::Document document(cursiv::TextDocument::create(
        "memory:llvmir", "llvmir", 0, std::string(source)));
    document.language = &language.language;
    document.parseResult = parse(source);
    if (document.parseResult.value)
      document.parseResult.value->parent = &document;
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(document,
                                                          diagnostics);
    return diagnostics;
  };
  const std::string source =
      read_file(std::filesystem::path(CURSIV_TEST_DIR) / "native_features.ll");
  const auto result = parse(source);
  const auto* module = result.value_as<ast::Module>();
  const ast::FuncDef *features = nullptr, *debug = nullptr, *splat = nullptr;
  const ast::IndirectSymbol *alias = nullptr, *ifunc = nullptr;
  const ast::GlobalVar *raw = nullptr, *auth = nullptr;
  const ast::FuncDecl* target = nullptr;
  bool pair = false, named_metadata = false;
  if (module)
    for (const auto* e : module->entities) {
      if (const auto* n = cursiv::ast_cast<ast::TypeDef>(e);
          n && n->name == "%Pair") {
        const auto* structure = cursiv::ast_cast<ast::StructType>(n->type);
        const auto* byte =
            structure && structure->members.size() == 2
                ? cursiv::ast_cast<ast::ByteType>(structure->members[0])
                : nullptr;
        pair = structure && structure->packed && byte && byte->text == "b8";
      }
      if (const auto* n = cursiv::ast_cast<ast::GlobalVar>(e)) {
        if (n->name == "@raw") raw = n;
        if (n->name == "@auth") auth = n;
      }
      if (const auto* n = cursiv::ast_cast<ast::IndirectSymbol>(e)) {
        if (n->name == "@tls") alias = n;
        if (n->name == "@ifunc") ifunc = n;
      }
      if (const auto* n = cursiv::ast_cast<ast::FuncDecl>(e);
          n && n->header && n->header->name == "@target_arg")
        target = n;
      if (const auto* n = cursiv::ast_cast<ast::FuncDef>(e); n && n->header) {
        if (n->header->name == "@features") features = n;
        if (n->header->name == "@debug") debug = n;
        if (n->header->name == "@splat") splat = n;
      }
      if (const auto* n = cursiv::ast_cast<ast::NamedMetadataDef>(e);
          n && n->name == "!llvm.dbg.cu" && n->nodes.size() == 1) {
        const auto* ref = std::get_if<ast::MdRef*>(&n->nodes[0]);
        named_metadata = ref && *ref && (*ref)->id == "!100";
      }
    }
  check(pair, "packed byte type");
  const auto* bytes =
      raw ? cursiv::ast_cast<ast::ArrayConst>(raw->initializer) : nullptr;
  check(bytes && bytes->cString == R"("a\00\22\5C")",
        "string bytes remain lossless");
  const auto* ptrauth =
      auth ? cursiv::ast_cast<ast::ConstantExpr>(auth->initializer) : nullptr;
  check(ptrauth && ptrauth->op == "ptrauth" && ptrauth->operands.size() == 3,
        "ptrauth operands");
  check(alias && alias->threadLocal && alias->tlsModel == "localdynamic" &&
            alias->kind == "alias",
        "alias TLS model");
  const auto* ifunc_md =
      ifunc && ifunc->metadata.size() == 1
          ? std::get_if<ast::MdRef*>(&ifunc->metadata[0]->node)
          : nullptr;
  check(ifunc && ifunc->kind == "ifunc" && ifunc_md && *ifunc_md &&
            (*ifunc_md)->id == "!0",
        "ifunc metadata is a reference");
  const auto* target_type = target && target->header->params &&
                                    target->header->params->params.size() == 1
                                ? cursiv::ast_cast<ast::TargetType>(
                                      target->header->params->params[0]->type)
                                : nullptr;
  check(target_type && target_type->name == "\"spirv.Event\"",
        "target extension type");
  if (features && features->body && features->body->blocks.size() == 1) {
    const auto& instructions = features->body->blocks[0]->instructions;
    check(instructions.size() == 7, "every feature instruction retained");
    if (instructions.size() == 7) {
      for (std::size_t i = 0; i < 6; ++i)
        check(instructions[i]->result.has_value(),
              "result inherited by every value instruction");
      const auto* zext = cursiv::ast_cast<ast::GenericInst>(instructions[1]);
      const auto* trunc = cursiv::ast_cast<ast::GenericInst>(instructions[2]);
      const auto* cmp = cursiv::ast_cast<ast::GenericInst>(instructions[3]);
      const auto* gep = cursiv::ast_cast<ast::GenericInst>(instructions[4]);
      check(zext && zext->flags.size() == 1 && zext->flags[0] == "nneg" &&
                trunc && trunc->flags.size() == 2 && cmp &&
                cmp->predicate == "eq" && cmp->flags[0] == "samesign" && gep &&
                gep->operands.size() == 2 && gep->flags.size() == 2,
            "modern flags and operands");
      const auto* md =
          instructions[5]->metadata.size() == 1
              ? std::get_if<ast::MdRef*>(&instructions[5]->metadata[0]->node)
              : nullptr;
      check(md && *md && (*md)->id == "!0",
            "freeze attachment retained as a reference");
      const auto* call = cursiv::ast_cast<ast::CallInst>(instructions[6]);
      check(call && call->args.size() == 2 && !call->args[0]->isMetadata &&
                call->args[0]->attrs.size() == 1 &&
                call->args[1]->attrs.size() == 1,
            "call arguments retain their roles and attributes");
      if (call && call->args.size() == 2 && call->args[0]->attrs.size() == 1 &&
          call->args[1]->attrs.size() == 1) {
        const auto& capture = *call->args[0]->attrs[0];
        const auto& range = *call->args[1]->attrs[0];
        check(capture.name == "captures" && capture.entries.size() == 2 &&
                  capture.entries[0]->name == "ret" &&
                  capture.entries[0]->values.size() == 1 &&
                  capture.entries[0]->values[0] == "address" &&
                  capture.entries[1]->values[0] == "provenance",
              "capture scope and components retained");
        check(range.name == "range" && range.typeArg && range.intArg == "0" &&
                  range.intArg2 == "4",
              "range type and both bounds retained");
      }
    }
  } else
    check(false, "feature function body");
  if (debug && debug->body && debug->body->blocks.size() == 1 &&
      debug->body->blocks[0]->terminator) {
    const auto& records = debug->body->blocks[0]->terminator->debugRecords;
    check(records.size() == 1 && records[0]->kind == "#dbg_value" &&
              records[0]->args.size() == 4,
          "debug record belongs to the following instruction");
    if (records.size() == 1 && records[0]->args.size() == 4) {
      const auto* value = std::get_if<ast::TypedValue*>(&records[0]->args[0]);
      const auto* variable = std::get_if<ast::MdRef*>(&records[0]->args[1]);
      const auto* expression =
          std::get_if<ast::SpecializedMdNode*>(&records[0]->args[2]);
      const auto* location = std::get_if<ast::MdRef*>(&records[0]->args[3]);
      check(
          value && *value && variable && *variable &&
              (*variable)->id == "!104" && expression && *expression &&
              (*expression)->kind == "!DIExpression" && location && *location &&
              (*location)->id == "!105",
          "debug value, variable, expression and location have distinct roles");
    }
  } else
    check(false, "debug function body");
  const auto* ret =
      splat && splat->body && !splat->body->blocks.empty()
          ? cursiv::ast_cast<ast::RetInst>(splat->body->blocks[0]->terminator)
          : nullptr;
  const auto* value =
      ret && ret->value
          ? cursiv::ast_cast<ast::ConstantExpr>((*ret->value)->value)
          : nullptr;
  check(value && value->op == "splat" && value->operands.size() == 1,
        "scalable vector splat");
  check(named_metadata, "named metadata contains references, not strings");
  check(validate(source).empty(),
        "native feature fixture has no runtime errors");
  {
    const auto summary_source =
        read_file(std::filesystem::path(CURSIV_TEST_DIR) / "native_summary.ll");
    const auto r = parse(summary_source);
    const auto* m = r.value_as<ast::Module>();
    check(m && m->entities.size() == 4, "every ThinLTO definition retained");
    if (m && m->entities.size() == 4) {
      const auto* gv = cursiv::ast_cast<ast::SummaryEntry>(m->entities[1]);
      const auto* flags = cursiv::ast_cast<ast::SummaryEntry>(m->entities[2]);
      check(gv && gv->id == "^1" && gv->definition &&
                gv->definition->name == "gv" &&
                gv->definition->value->elements.size() == 2 && flags &&
                flags->definition->value->atoms.size() == 1 &&
                flags->definition->value->atoms[0] == "8",
            "ThinLTO fields and scalar values");
    }
    check(validate(summary_source).empty(),
          "native summary has no runtime errors");
  }
  for (std::string_view invalid :
       {"@g = global i0 0", "@g = global b0 0",
        "@g = external global [ -1 x i8 ]", "@g = external global <0 x i32>",
        "declare void @f(ptr addrspace(16777216))",
        "declare void @f(target(\"audit\", 1, i32))",
        "declare void @f(ptr align 3)", "declare void @f(ptr nofpclass(0))",
        "declare void @f() memory(argmem: read, read)",
        "define void @f() { %x = ret void }",
        "define void @f(ptr %p) { %x = load atomic i32, ptr %p acquire ret "
        "void }",
        "define void @f() { fence monotonic ret void }",
        "define void @f(ptr %p) { store atomic i32 0, ptr %p acquire, align 4 "
        "ret void }",
        "define void @f(i32 %x) { %r = and disjoint i32 %x, 1 ret void }",
        "!0 = !DIFile(filename: \"a\", filename: \"b\", directory: \"\")",
        "!0 = !{!99}", "!0 = !{} !0 = !{}"})
    check(!validate(invalid).empty(),
          "invalid semantic form parses and reports a runtime error");
  check(validate("!00 = !{} !named = !{!0}").empty(),
        "metadata numeric identifiers share their numeric value");
  {
    constexpr std::string_view quoted = R"(define i32 @f(i32 %"arg\00n") {
      %result = add i32 %"arg\00n", 1
      ret i32 %result
    })";
    const auto diagnostics = validate(quoted);
    check(diagnostics.size() == 2,
          "NUL names report definitions and references");
    for (const auto& d : diagnostics)
      check(d.code && *d.code == "llvmir.nul-name" &&
                d.range.end > d.range.begin &&
                quoted.substr(d.range.begin, d.range.end - d.range.begin) ==
                    R"(%"arg\00n")",
            "NUL name diagnostic code and exact identifier range");
    check(validate(R"(define i32 @f(i32 %"arg\\00n") { ret i32 %"arg\\00n" })")
              .empty(),
          "an escaped backslash before 00 is not a NUL");
    check(validate(R"(!0 = !{!"bytes\00\FF"})").empty(),
          "NUL remains valid in metadata strings");
  }
  {
    const auto result = parse("declare i32 @g(i32,...) define i32 @f(i32 %x,...){%r=musttail call i32(i32,...) @g(i32 %x,...) ret i32 %r}");
    bool forward = false;
    for (const auto* n : result.nodes) if (const auto* call = cursiv::ast_cast<ast::CallInst>(n))
      forward |= call->forwardsVarargs && call->tailKind && *call->tailKind == "musttail" && call->args.size() == 1;
    check(forward, "musttail variadic forwarding AST");
  }
  check(!validate("declare void @g(...) define void @f(){call void(...) @g(...) ret void}").empty(), "ellipsis requires musttail");
  check(!validate("define i32 @f(i8 %x){%v=zext nnan i8 %x to i32 ret i32 %v}").empty(), "integer cast rejects fast-math flag");
  check(validate("define float @f(i8 %x){%v=uitofp nnan nneg i8 %x to float ret float %v}").empty(), "native uitofp flags remain valid");
  repeated_alignment(parse, validate, check);
  return ok;
}
}  // namespace code_audit
