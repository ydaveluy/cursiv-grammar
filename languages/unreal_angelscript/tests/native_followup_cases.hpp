#pragma once
#include <cstdio>
#include <string_view>
#include <unordered_set>
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "unrealangelscript/core/ast.hpp"
namespace unreal_followup {
template<class Language> bool run(const Language& language) {
  struct Case {const char* name; std::string_view source; bool accepted;};
  const Case cases[] = {
    {"integer", R"UAS(void Native(){ int Value = 123; })UAS", true},
    {"octal", R"UAS(void Native(){ int Value = 0o17; })UAS", true},
    {"binary", R"UAS(void Native(){ int Value = 0b101; })UAS", true},
    {"hex", R"UAS(void Native(){ int Value = 0xFF; })UAS", true},
    {"leading-zero", R"UAS(void Native(){ int Value = 09; })UAS", true},
    {"float32", R"UAS(void Native(){ float32 Value = 1.5f; })UAS", true},
    {"float64", R"UAS(void Native(){ float64 Value = 1.5; })UAS", true},
    {"keyword", R"UAS(void Native(){ int if = 1; })UAS", false},
    {"keyword-prefix", R"UAS(void Native(){ int iffy = 1; })UAS", true},
    {"string", R"UAS(void Native(){ FString Text = "hello"; })UAS", true},
    {"unicode-escape", R"UAS(void Native(){ FString Text = "\u03B1"; })UAS", true},
    {"name", R"UAS(void Native(){ FName Name = n"Game"; })UAS", true},
    {"format", R"UAS(void Native(){ FString Text = f"Value {Value}"; })UAS", true},
    {"character", R"UAS(void Native(){ FString Text = 'ab'; })UAS", true},
    {"triple", R"UAS(void Native(){ FString Text = """line
next"""; })UAS", true},
    {"cpp-literal", R"UAS(void Native(){ int Value = 123_test; })UAS", false},
    {"cpp-integer-suffix", R"UAS(void Native(){ int Value = 123ULL; })UAS", false},
    {"cpp-digit-separator", R"UAS(void Native(){ int Value = 1'000; })UAS", false},
    {"cpp-char-prefix", R"UAS(void Native(){ int Value = L'a'; })UAS", false},
    {"pointer", R"UAS(void Native(){ int Value = *Other; })UAS", false},
    {"address", R"UAS(void Native(){ int Value = &Other; })UAS", false},
    {"unary-bar", R"UAS(void Native(){ int Value = |Other; })UAS", false},
    {"unary-not", R"UAS(void Native(){ bool Value = !Other; })UAS", true},
    {"unary-negative", R"UAS(void Native(){ int Value = -Other; })UAS", true},
    {"numeric-adjacency", R"UAS(void Native(){ int Value = 1 2; })UAS", false},
    {"quote-newline", R"UAS(void Native(){ FString Text = "a
b"; })UAS", false},
    {"generic", R"UAS(void Native(){ TArray<int32> Values; })UAS", true},
    {"shift", R"UAS(void Native(){ int Value = a >> b; })UAS", true},
    {"assign", R"UAS(void Native(){ Value += 1; })UAS", true},
    {"class-no-semicolon", R"UAS(class ACount : AActor {float32 Value;})UAS", true},
    {"enum-no-semicolon", R"UAS(enum EColor {Red, Blue})UAS", true},
    {"mismatched-list-brace", R"UAS(void Native(){TArray<int> x = {1,2];})UAS", false},
    {"mismatched-list-bracket", R"UAS(void Native(){TArray<int> x = [1,2};})UAS", false},
    {"cpp-decltype", R"UAS(void Native(){(a) value=1;})UAS", false},
    {"cpp-operator", R"UAS(class AThing {int operator+(int Other) {return Other;}})UAS", false},
    {"decimal-base", R"UAS(void Native(){int x=0d19;})UAS", true},
    {"delegate", "delegate void FOnValue(float32 Value);", true},
    {"event", "event void FOnValue(float32 Value);", true},
    {"enum-annotation", "UENUM() enum EColor {Red, Blue}", true},
    {"cpp-lambda", "void Test(){auto x = [](int a){return a;};}", false},
    {"cpp-bitfield", "class AThing {int Flags:4;}", false},
    {"cpp-namespace-alias", "namespace Game = Other;", false},
    {"cpp-type-alias", "Name = float;", false},
    {"cpp-label", "void Test(){Loop: return;}", false},
    {"cpp-defaulted", "void Test() = default;", false},
    {"anonymous-namespace", "namespace {}", false},
    {"bare-local-type", "void Test(){int;}", false},
    {"bare-global-type", "int;", false},
    {"bare-property-type", "class AThing {int;}", false},
    {"bare-global-assignment", "Value=1;", false},
    {"elaborated-parameter", "void Test(class AThing Value){}", false},
    {"empty-class", "class AThing {}", true},
    {"cpp-comma-expression", "void Test(){int Value=(a,b);}", false},
    {"for-update-list", "void Test(){for(;;a++,b++) {}}", true},
    {"empty-switch-cases", "void Test(){switch(x){case 1:default:}}", true},
    {"qualified-namespace", "namespace Game::Inner {int Value=1;}", true},
  };
  bool ok = true;
  for (const auto& test : cases) {
    const auto result = language.language.parser->parse(test.source);
    const bool clean = result.accepted && !result.cst().recovered() && result.diagnostics.empty() && result.value;
    if (clean != test.accepted) {
      std::fprintf(stderr, "UnrealAS reference: %s expected=%d accepted=%d recovered=%d diagnostics=%zu\n",test.name,test.accepted,result.accepted,result.cst().recovered(),result.diagnostics.size());
      ok = false;
    }
  }
  namespace ast = unrealangelscript::ast;
  const auto result = language.language.parser->parse(R"UAS(
UCLASS(Abstract)
class ACount : AActor {
 UPROPERTY() float32 Value = 1.0f;
 UFUNCTION() void Tick(float32 Delta, const FName &in Name) { int Count = 0o17; }
}
)UAS");
  const auto* file = cursiv::ast_cast<ast::Script>(result.value);
  const auto* decl = file && file->items.size() == 1 ? cursiv::ast_cast<ast::VarDecl>(file->items[0]) : nullptr;
  const auto* cls = decl && decl->specifiers && decl->specifiers->parts.size() == 1 ? cursiv::ast_cast<ast::ClassDef>(decl->specifiers->parts[0]) : nullptr;
  const auto* prop = cls && cls->members.size() == 2 ? cursiv::ast_cast<ast::PropertyDef>(cls->members[0]) : nullptr;
  const auto* fn = cls && cls->members.size() == 2 ? cursiv::ast_cast<ast::FunctionDef>(cls->members[1]) : nullptr;
  const auto* type = prop && prop->specifiers && prop->specifiers->parts.size() == 1 ? cursiv::ast_cast<ast::BuiltinTypeSpec>(prop->specifiers->parts[0]) : nullptr;
  const auto* params = fn && fn->declarator ? fn->declarator->params : nullptr;
  const bool values = cls && cls->name == "ACount" && cls->utype && cls->utype->annotations.size() == 1 &&
      cls->bases.size() == 1 && cls->bases[0]->name == "AActor" &&
      prop && prop->uproperty && type && type->keyword == "float32" && prop->declarators.size() == 1 &&
      prop->declarators[0]->name == "Value" && prop->declarators[0]->init &&
      fn && fn->ufunction && fn->declarator->name == "Tick" && params && params->params.size() == 2 &&
      params->params[0]->name == "Delta" && params->params[1]->name == "Name" &&
      params->params[0]->specifiers->parts.size() == 1 && params->params[1]->specifiers->parts.size() == 4;
  std::unordered_set<const cursiv::AstNode*> seen;
  bool owned = true;
  auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
    if (!seen.insert(&node).second) { owned = false; return; }
    language.language.astReflection->visitMembers(node,
        [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
          owned &= child.parent == &node && result.cst().contains(node.cstNodeId,child.cstNodeId);
          self(self,child);
        });
  };
  if (result.value) walk(walk,*result.value);
  if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() || !values || !owned || seen.size() != result.nodes.size()) {
    std::fprintf(stderr,"UnrealAS typed AST values=%d owned=%d nodes=%zu/%zu\n",values,owned,seen.size(),result.nodes.size());
    ok = false;
  }
  const auto definitions = language.language.parser->parse(
      "delegate void FOnValue(float32 Value); event void FOnEvent(float32 Delta); UENUM() enum EColor {Red}");
  const auto* defs = cursiv::ast_cast<ast::Script>(definitions.value);
  const auto* callback = defs && defs->items.size() == 3 ? cursiv::ast_cast<ast::DelegateDecl>(defs->items[0]) : nullptr;
  const auto* event = defs && defs->items.size() == 3 ? cursiv::ast_cast<ast::DelegateDecl>(defs->items[1]) : nullptr;
  const auto* enumDecl = defs && defs->items.size() == 3 ? cursiv::ast_cast<ast::VarDecl>(defs->items[2]) : nullptr;
  const auto* enumeration = enumDecl && enumDecl->specifiers && enumDecl->specifiers->parts.size() == 1 ?
      cursiv::ast_cast<ast::EnumDef>(enumDecl->specifiers->parts[0]) : nullptr;
  const bool captures = callback && !callback->isEvent && callback->result && callback->result->parent == callback &&
      callback->declarator && callback->declarator->parent == callback && callback->declarator->name == "FOnValue" &&
      callback->declarator->params->params.size() == 1 && callback->declarator->params->params[0]->name == "Value" &&
      event && event->isEvent && event->declarator && event->declarator->name == "FOnEvent" &&
      enumeration && enumeration->name == "EColor" && enumeration->utype &&
      enumeration->utype->keyword == "UENUM" && enumeration->utype->parent == enumeration;
  if (!definitions.accepted || definitions.cst().recovered() || !definitions.diagnostics.empty() || !captures) {
    std::fprintf(stderr,"UnrealAS delegate/event/enum captures=%d\n",captures);
    ok = false;
  }
  return ok;
}
}
