#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "metamath/core/ast.hpp"
#include <cstdio>
#include <iterator>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <vector>

namespace metamath_audit {
template <class Language> bool modern_cases(const Language &language) {
  namespace ast = metamath::ast;
  const auto &parser = *language.language.parser;
  bool ok = true;
  unsigned checks = 0;
  auto check = [&](bool condition, const char *message) {
    ++checks;
    if (!condition) {
      std::fprintf(stderr, "metamath audit: %s\n", message);
      ok = false;
    }
  };
  struct Case {
    std::string_view name, source;
  };
  constexpr Case valid[] = {
      {"empty", R"MM()MM"},
      {"comments", R"MM($( A comment containing $a $p [ ] ( ) . $)
)MM"},
      {"basic", R"MM($c wff |- $.
$v ph ps $.
wph $f wff ph $.
wps $f wff ps $.
ax $a |- ph $.
)MM"},
      {"vertical-tab", R"MM($cwff|-$.
$vphps$.
wph$fwffph$.
wps$fwffps$.
ax$a|-ph$.
)MM"},
      {"all-whitespace", R"MM($c	
wff
|-$.)MM"},
      {"punctuation",
       R"MM($c ! " # % & ' ( ) * + , - . / : ; < = > ? @ [ \ ] ^ _ ` { | } ~ $.)MM"},
      {"labels", R"MM($c t $.
.-_09 $a t $.
)MM"},
      {"type-only", R"MM($c t $.
a $a t $.
${ e $e t $. $}
)MM"},
      {"block", R"MM($c wff |- $.
$v ph ps $.
wph $f wff ph $.
wps $f wff ps $.
ax $a |- ph $.
${ $d ph ps $. e $e |- ph $. a $a |- ph $. $}
)MM"},
      {"nested-block", R"MM($c wff |- $.
$v ph ps $.
wph $f wff ph $.
wps $f wff ps $.
ax $a |- ph $.
${ ${ $d ph ps $. a $a |- ph $. $} $}
)MM"},
      {"scoped-variable", R"MM($c wff |- $.
$v ph ps $.
wph $f wff ph $.
wps $f wff ps $.
ax $a |- ph $.
${ $v ch $. wc $f wff ch $. a $a |- ch $. $}
)MM"},
      {"plain-proof", R"MM($c wff |- $.
$v ph ps $.
wph $f wff ph $.
wps $f wff ps $.
ax $a |- ph $.
th $p |- ph $= wph ax $.
)MM"},
      {"compressed", R"MM($c wff |- $.
$v ph ps $.
wph $f wff ph $.
wps $f wff ps $.
ax $a |- ph $.
th $p |- ph $= ( ax ) AB $.
)MM"},
      {"compressed-adjacent", R"MM($c wff |- $.
$v ph ps $.
wph $f wff ph $.
wps $f wff ps $.
ax $a |- ph $.
th $p |- ph $= (ax)AB $.
)MM"},
      {"compressed-split", R"MM($c wff |- $.
$v ph ps $.
wph $f wff ph $.
wps $f wff ps $.
ax $a |- ph $.
th $p |- ph $= ( ax ) A $( split $) B $.
)MM"},
      {"unknown-proof", R"MM($c wff |- $.
$v ph ps $.
wph $f wff ph $.
wps $f wff ps $.
ax $a |- ph $.
th $p |- ph $= ? $.
)MM"},
      {"empty-proof", R"MM($c wff |- $.
$v ph ps $.
wph $f wff ph $.
wps $f wff ps $.
ax $a |- ph $.
th $p |- ph $= $.
)MM"},
      {"empty-compressed", R"MM($c wff |- $.
$v ph ps $.
wph $f wff ph $.
wps $f wff ps $.
ax $a |- ph $.
th $p |- ph $= ( ) $.
)MM"},
      {"packed-proof", R"MM($c wff |- $.
$v ph ps $.
wph $f wff ph $.
wps $f wff ps $.
ax $a |- ph $.
${ h1 $e |- ph $. h2 $e |- ph $. combine $a |- ph $. $}
th $p |- ph $= wph wph local:ax local combine $.
)MM"},
      {"explicit-proof", R"MM($c wff |- $.
$v ph ps $.
wph $f wff ph $.
wps $f wff ps $.
ax $a |- ph $.
th $p |- ph $= wph=wph th=ax $.
)MM"},
      {"include-top", R"MM($[ constants.inc $]
)MM"},
      {"include-block", R"MM($c wff |- $.
${ $[ scoped.inc $] $}
)MM"},
      {"include-constant", R"MM($c $[ symbols.inc $] $.
)MM"},
      {"include-variable", R"MM($c wff $.
$v $[ variables.inc $] $.
)MM"},
      {"include-floating", R"MM($c wff $.
$v ph $.
wph $f wff $[ variable.inc $] $.
)MM"},
      {"include-math", R"MM($c wff |- $.
$v ph ps $.
wph $f wff ph $.
wps $f wff ps $.
ax $a |- ph $.
a $a $[ expression.inc $] $.
)MM"},
      {"include-proof", R"MM($c wff |- $.
$v ph ps $.
wph $f wff ph $.
wps $f wff ps $.
ax $a |- ph $.
th $p |- ph $= $[ proof.inc $] $.
)MM"},
      {"include-compressed-labels", R"MM($c wff |- $.
$v ph ps $.
wph $f wff ph $.
wps $f wff ps $.
ax $a |- ph $.
th $p |- ph $= ( $[ labels.inc $] ) AB $.
)MM"},
      {"include-compressed-code", R"MM($c wff |- $.
$v ph ps $.
wph $f wff ph $.
wps $f wff ps $.
ax $a |- ph $.
th $p |- ph $= ( ax ) $[ code.inc $] $.
)MM"},
      {"include-dollar-name", R"MM($[ name$part.inc $]
)MM"},
      {"include-comments", R"MM($[ $( note $) constants.inc $( note $) $]
)MM"},
      {"include-declaration-head", R"MM($[ declaration-head.inc $] wff $.
)MM"},
      {"include-axiom-head", R"MM($c wff |- $.
$v ph ps $.
wph $f wff ph $.
wps $f wff ps $.
ax $a |- ph $.
a $[ axiom-head.inc $] |- ph $.
)MM"},
      {"include-provable-head", R"MM($c wff |- $.
$v ph ps $.
wph $f wff ph $.
wps $f wff ps $.
ax $a |- ph $.
$[ provable-head.inc $] |- ph $= wph ax $.
)MM"},
      {"include-label", R"MM($c t $. $[ label.inc $] $a t $.
)MM"},
      {"include-scope-open", R"MM($c t $. $[ scope-open.inc $] a $a t $. $}
)MM"},
      {"include-scope-close", R"MM($c t $. ${ a $a t $. $[ scope-close.inc $]
)MM"},
      {"include-before-declaration", R"MM($[ constants.inc $] $c u $.
)MM"},
      {"invalid-compressed-label", R"MM($c wff |- $.
$v ph ps $.
wph $f wff ph $.
wps $f wff ps $.
ax $a |- ph $.
th $p |- ph $= ( bad! ) AB $.
)MM"},
      {"invalid-compressed-code", R"MM($c wff |- $.
$v ph ps $.
wph $f wff ph $.
wps $f wff ps $.
ax $a |- ph $.
th $p |- ph $= ( ax ) abc $.
)MM"},
      {"invalid-proof-label", R"MM($c wff |- $.
$v ph ps $.
wph $f wff ph $.
wps $f wff ps $.
ax $a |- ph $.
th $p |- ph $= bad! $.
)MM"},
      {"upstream-demo0", R"MM($( demo0.mm  1-Jan-04 $)

$(
                      PUBLIC DOMAIN DEDICATION

This file is placed in the public domain per the Creative Commons Public
Domain Dedication. http://creativecommons.org/licenses/publicdomain/

Norman Megill
$)

$( This file is the introductory formal system example described
   in Chapter 2 of the Meamath book. $)

$( Declare the constant symbols we will use $)
    $c 0 + = -> ( ) term wff |- $.
$( Declare the metavariables we will use $)
    $v t r s P Q $.
$( Specify properties of the metavariables $)
    tt $f term t $.
    tr $f term r $.
    ts $f term s $.
    wp $f wff P $.
    wq $f wff Q $.
$( Define "term" (part 1) $)
    tze $a term 0 $.
$( Define "term" (part 2) $)
    tpl $a term ( t + r ) $.
$( Define "wff" (part 1) $)
    weq $a wff t = r $.
$( Define "wff" (part 2) $)
    wim $a wff ( P -> Q ) $.
$( State axiom a1 $)
    a1 $a |- ( t = r -> ( t = s -> r = s ) ) $.
$( State axiom a2 $)
    a2 $a |- ( t + 0 ) = t $.
    ${
       min $e |- P $.
       maj $e |- ( P -> Q ) $.
$( Define the modus ponens inference rule $)
       mp  $a |- Q $.
    $}
$( Prove a theorem $)
    th1 $p |- t = t $=
  $( Here is its proof: $)
       tt tze tpl tt weq tt tt weq tt a2 tt tze tpl
       tt weq tt tze tpl tt weq tt tt weq wim tt a2
       tt tze tpl tt tt a1 mp mp
     $.

)MM"},
      {"upstream-disjoint2", R"MM($c formula |- ( ) $.

$v x y z w $.
xf $f formula x $.
yf $f formula y $.
zf $f formula z $.
wf $f formula w $.
combo $a formula ( x y ) $.
${
  $d x y $.
  ax-1 $a |- ( x y ) $.
$}

verybad $p |- ( ( x y ) ( z x ) ) $=
  xf yf combo zf xf combo ax-1 $.

${
  $d x y z $.
  stillbad $p |- ( ( x y ) ( z x ) ) $=
    xf yf combo zf xf combo ax-1 $.
$}

semigood $p |- ( ( x y ) ( z w ) ) $=
  xf yf combo zf wf combo ax-1 $.

${
  $d x y $.
  $d z w $.
  almostgood $p |- ( ( x y ) ( z w ) ) $=
    xf yf combo zf wf combo ax-1 $.
$}

${
  $d x y z w $.
  good $p |- ( ( x y ) ( z w ) ) $=
    xf yf combo zf wf combo ax-1 $.
$}

${
  $d x z $. $d x w $. $d y z $. $d y w $.
  stillgood $p |- ( ( x y ) ( z w ) ) $=
    xf yf combo zf wf combo ax-1 $.
$}
)MM"},
  };
  for (const auto &entry : valid) {
    auto result = parser.parse(entry.source);
    const bool clean = result.accepted && !result.cst().recovered() &&
                       result.diagnostics.empty();
    if (!clean)
      std::fprintf(stderr, "metamath native form: %.*s\n",
                   int(entry.name.size()), entry.name.data());
    check(clean, "READ-positive form parses without repair");
    std::unordered_set<const cursiv::AstNode *> reachable;
    bool parents = true;
    auto walk = [&](auto &&self, const cursiv::AstNode *node,
                    const cursiv::AstNode *parent) -> void {
      if (!node || !reachable.insert(node).second) {
        parents = false;
        return;
      }
      if (node->parent != parent)
        parents = false;
      auto visit = [&](const auto &value) {
        std::visit(
            [&](const auto &part) {
              if constexpr (std::is_pointer_v<std::decay_t<decltype(part)>>)
                self(self, part, node);
            },
            value);
      };
      if (auto x = cursiv::ast_cast<ast::Database>(node))
        for (const auto &value : x->stmts)
          visit(value);
      if (auto x = cursiv::ast_cast<ast::MathStatement>(node))
        for (const auto &value : x->math)
          visit(value);
      if (auto x = cursiv::ast_cast<ast::ProvableStmt>(node))
        visit(x->proof);
      if (auto x = cursiv::ast_cast<ast::DeferredStmt>(node))
        std::visit(
            [&](auto *part) {
              if (part)
                self(self, part, node);
            },
            x->proof);
      if (auto x = cursiv::ast_cast<ast::UncompressedProof>(node))
        for (const auto &value : x->steps)
          visit(value);
      if (auto x = cursiv::ast_cast<ast::CompressedProof>(node)) {
        for (const auto &value : x->labels)
          visit(value);
        for (const auto &value : x->blocks)
          visit(value);
      }
    };
    walk(walk, result.value, nullptr);
    check(parents && reachable.size() == result.nodes.size(),
          "all captured nodes are reachable with the exact field parent");
  }
  auto parse = [&](std::string_view source) {
    auto result = parser.parse(source);
    check(result.accepted && !result.cst().recovered() &&
              result.diagnostics.empty(),
          "AST fixture parses cleanly");
    return result;
  };
  auto root = [](const cursiv::ParseResult &result) {
    return cursiv::ast_cast<ast::Database>(result.value);
  };
  auto statement = [&](const cursiv::ParseResult &result,
                       std::size_t index) -> const cursiv::AstNode * {
    auto database = root(result);
    return database && database->stmts.size() > index
               ? std::visit(
                     [](auto *node) -> const cursiv::AstNode * { return node; },
                     database->stmts[index])
               : nullptr;
  };
  auto symbol = [](const ast::MathItem &item, std::string_view expected) {
    auto text = std::get_if<std::string_view>(&item);
    return text && *text == expected;
  };
  {
    auto result =
        parse("$c wff |- $. $v ph $. wph $f wff ph $. ax $a |- ph $.");
    auto constant = cursiv::ast_cast<ast::ConstantStmt>(statement(result, 0));
    auto floating = cursiv::ast_cast<ast::FloatingStmt>(statement(result, 2));
    auto axiom = cursiv::ast_cast<ast::AxiomStmt>(statement(result, 3));
    check(constant && constant->math.size() == 2 &&
              symbol(constant->math[0], "wff") &&
              symbol(constant->math[1], "|-"),
          "constant symbol order and spelling");
    check(floating && floating->label == "wph" && floating->math.size() == 2 &&
              symbol(floating->math[0], "wff") &&
              symbol(floating->math[1], "ph"),
          "floating label, typecode and variable");
    check(axiom && axiom->label == "ax" && axiom->math.size() == 2 &&
              symbol(axiom->math[0], "|-") && symbol(axiom->math[1], "ph"),
          "assertion label and math string");
    check(axiom && axiom->parent == root(result) && floating &&
              floating->parent == root(result),
          "statement parents");
  }
  {
    auto result = parse("${ $[ local.inc $] $v x $. ${ $d x y $. $} $}");
    auto database = root(result);
    auto include = cursiv::ast_cast<ast::IncludeStmt>(statement(result, 1));
    check(database && database->stmts.size() == 7 &&
              cursiv::ast_cast<ast::ScopeOpen>(statement(result, 0)) &&
              cursiv::ast_cast<ast::ScopeOpen>(statement(result, 3)) &&
              cursiv::ast_cast<ast::ScopeClose>(statement(result, 5)) &&
              cursiv::ast_cast<ast::ScopeClose>(statement(result, 6)),
          "scope markers remain in source order");
    check(include && include->filename == "local.inc" &&
              include->parent == database &&
              statement(result, 4)->parent == database,
          "source statements retain database parents before expansion");
  }
  {
    auto result =
        parse("wph $f wff $[ variable.inc $] $. ax $a $[ expression.inc $] $.");
    auto floating = cursiv::ast_cast<ast::FloatingStmt>(statement(result, 0));
    auto axiom = cursiv::ast_cast<ast::AxiomStmt>(statement(result, 1));
    auto variable = floating && floating->math.size() == 2
                        ? std::get_if<ast::IncludeStmt *>(&floating->math[1])
                        : nullptr;
    auto expression = axiom && axiom->math.size() == 1
                          ? std::get_if<ast::IncludeStmt *>(&axiom->math[0])
                          : nullptr;
    check(floating && symbol(floating->math[0], "wff") && variable &&
              *variable && (*variable)->filename == "variable.inc" &&
              (*variable)->parent == floating,
          "math inclusion order and parent");
    check(expression && *expression &&
              (*expression)->filename == "expression.inc" &&
              (*expression)->parent == axiom,
          "whole math inclusion remains unexpanded");
  }
  {
    auto result = parse("th $p |- ph $= wph wph local:ax local combine $.");
    auto theorem = cursiv::ast_cast<ast::ProvableStmt>(statement(result, 0));
    auto proof = theorem
                     ? std::get_if<ast::UncompressedProof *>(&theorem->proof)
                     : nullptr;
    auto step = proof && *proof && (*proof)->steps.size() == 5
                    ? std::get_if<ast::ProofStep *>(&(*proof)->steps[2])
                    : nullptr;
    check(proof && *proof && (*proof)->steps.size() == 5 &&
              std::get<std::string_view>((*proof)->steps[0]) == "wph" &&
              std::get<std::string_view>((*proof)->steps[3]) == "local",
          "plain proof steps retain their sequence");
    check(step && *step && (*step)->local == "local" &&
              (*step)->label == "ax" && (*step)->target.empty(),
          "packed proof local definition fields");
    check(proof && *proof && (*proof)->parent == theorem && step && *step &&
              (*step)->parent == *proof,
          "proof and local-step parents");
  }
  {
    auto result = parse("th $p |- ph $= wph=wph th=ax $.");
    auto theorem = cursiv::ast_cast<ast::ProvableStmt>(statement(result, 0));
    auto proof = theorem
                     ? std::get_if<ast::UncompressedProof *>(&theorem->proof)
                     : nullptr;
    auto first = proof && *proof && (*proof)->steps.size() == 2
                     ? std::get_if<ast::ProofStep *>(&(*proof)->steps[0])
                     : nullptr;
    auto last = proof && *proof && (*proof)->steps.size() == 2
                    ? std::get_if<ast::ProofStep *>(&(*proof)->steps[1])
                    : nullptr;
    check(first && *first && (*first)->target == "wph" &&
              (*first)->label == "wph" && (*first)->local.empty() && last &&
              *last && (*last)->target == "th" && (*last)->label == "ax",
          "explicit proof target and source labels");
  }
  {
    auto result = parse("th $p |- ph $= ( ax $[ labels.inc $] ) A $( gap $) B "
                        "$[ code.inc $] $.");
    auto theorem = cursiv::ast_cast<ast::ProvableStmt>(statement(result, 0));
    auto proof = theorem ? std::get_if<ast::CompressedProof *>(&theorem->proof)
                         : nullptr;
    auto label = proof && *proof && (*proof)->labels.size() == 2
                     ? std::get_if<ast::IncludeStmt *>(&(*proof)->labels[1])
                     : nullptr;
    auto code = proof && *proof && (*proof)->blocks.size() == 3
                    ? std::get_if<ast::IncludeStmt *>(&(*proof)->blocks[2])
                    : nullptr;
    check(proof && *proof && (*proof)->labels.size() == 2 &&
              symbol((*proof)->labels[0], "ax") &&
              (*proof)->blocks.size() == 3 &&
              symbol((*proof)->blocks[0], "A") &&
              symbol((*proof)->blocks[1], "B"),
          "compressed label and block order");
    check(label && *label && (*label)->filename == "labels.inc" &&
              (*label)->parent == *proof && code && *code &&
              (*code)->filename == "code.inc" && (*code)->parent == *proof,
          "compressed proof inclusions and parents");
  }
  {
    auto result = parse("th $p |- ph $= $[ proof.inc $] $.");
    auto theorem = cursiv::ast_cast<ast::ProvableStmt>(statement(result, 0));
    auto proof = theorem
                     ? std::get_if<ast::UncompressedProof *>(&theorem->proof)
                     : nullptr;
    auto include = proof && *proof && (*proof)->steps.size() == 1
                       ? std::get_if<ast::IncludeStmt *>(&(*proof)->steps[0])
                       : nullptr;
    check(include && *include && (*include)->filename == "proof.inc" &&
              (*include)->parent == *proof,
          "uncompressed proof inclusion remains unexpanded");
  }
  {
    auto result = parse("$[ declaration.inc $] wff $. a $[ axiom.inc $] |- ph "
                        "$. $[ theorem.inc $] |- ph $= wph ax $.");
    auto constant = cursiv::ast_cast<ast::DeferredStmt>(statement(result, 0));
    auto axiom = cursiv::ast_cast<ast::DeferredStmt>(statement(result, 1));
    auto theorem = cursiv::ast_cast<ast::DeferredStmt>(statement(result, 2));
    auto include = constant && constant->math.size() == 2
                       ? std::get_if<ast::IncludeStmt *>(&constant->math[0])
                       : nullptr;
    auto proof = theorem
                     ? std::get_if<ast::UncompressedProof *>(&theorem->proof)
                     : nullptr;
    check(
        include && *include && (*include)->filename == "declaration.inc" &&
            (*include)->parent == constant && symbol(constant->math[1], "wff"),
        "included declaration header stays untyped and preserves source items");
    check(axiom && axiom->math.size() == 4 && symbol(axiom->math[0], "a") &&
              symbol(axiom->math[2], "|-") && symbol(axiom->math[3], "ph"),
          "label before included header stays in source order");
    check(proof && *proof && (*proof)->parent == theorem &&
              (*proof)->steps.size() == 2 &&
              std::get<std::string_view>((*proof)->steps[0]) == "wph",
          "deferred theorem keeps its explicit proof and parent");
  }
  {
    auto result = parse("$[ constants.inc $] $c u $.");
    check(root(result) && root(result)->stmts.size() == 2 &&
              cursiv::ast_cast<ast::IncludeStmt>(statement(result, 0)) &&
              cursiv::ast_cast<ast::ConstantStmt>(statement(result, 1)),
          "inclusion cannot absorb the next explicit declaration");
  }
  auto validate = [&](std::string source, std::string_view code,
                      unsigned expected) {
    cursiv::Document document(cursiv::TextDocument::create(
        "file:///audit.mm", "metamath", 0, std::move(source)));
    document.language = &language.language;
    document.parseResult = parser.parse(document.textDocument().getText());
    check(document.parseSucceeded() && !document.parseRecovered(),
          "invalid form reaches runtime validation");
    if (document.parseResult.value)
      document.parseResult.value->parent = &document;
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(document,
                                                          diagnostics);
    unsigned count = 0;
    for (const auto &diagnostic : diagnostics)
      if (diagnostic.code && diagnostic.code->isString() &&
          diagnostic.code->string() == code) {
        ++count;
        if (code == "metamath.compressed-code")
          check(diagnostic.range.begin ==
                    document.textDocument().getText().find("abc"),
                "compressed-code diagnostic selects the payload");
        if (code == "metamath.proof-label")
          check(diagnostic.range.begin ==
                    document.textDocument().getText().find("bad!"),
                "proof-label diagnostic selects its spelling");
        if (code == "metamath.keyword-space")
          check(diagnostic.range.begin ==
                    document.textDocument().getText().find("$."),
                "keyword-spacing diagnostic selects the keyword");
      }
    if (count != expected)
      std::fprintf(stderr, "metamath validator: %.*s expected %u got %u\n",
                   int(code.size()), code.data(), expected, count);
    check(count == expected, "runtime diagnostic code and count");
  };
  validate("$c $.", "metamath.math-empty", 1);
  validate("$v $.", "metamath.math-empty", 1);
  validate("$d x $.", "metamath.disjoint-size", 1);
  validate("bad $f t $.", "metamath.floating-size", 1);
  validate("bad $f t x y $.", "metamath.floating-size", 1);
  validate("bad $f $.", "metamath.floating-size", 1);
  validate("bad $a $.", "metamath.math-empty", 1);
  validate("${ $c t $. $}", "metamath.constant-scope", 1);
  validate("$a t $.", "metamath.label", 1);
  validate("bad! $a t $.", "metamath.label", 1);
  validate("a $a t $. a $a t $.", "metamath.duplicate-label", 1);
  validate("th $p t $= $.", "metamath.proof-empty", 1);
  validate("th $p t $= ( ) $.", "metamath.proof-empty", 1);
  validate("th $p t $= ? $.", "metamath.proof-empty", 0);
  validate("th $p t $= ( ) abc $.", "metamath.compressed-code", 1);
  validate("th $p t $= ( bad! ) AB $.", "metamath.proof-label", 1);
  validate("th $p t $= bad! $.", "metamath.proof-label", 1);
  validate("$c t$.", "metamath.keyword-space", 1);
  validate("$( outer $( inner $)", "metamath.comment-nesting", 1);
  validate("bad $f $[ fields.inc $] $.", "metamath.floating-size", 0);
  validate("${", "metamath.scope-open", 1);
  validate("$}", "metamath.scope-close", 1);
  validate("${ $[ close.inc $]", "metamath.scope-open", 0);
  validate("$[ open.inc $] $}", "metamath.scope-close", 0);
  validate("${ $[ close.inc $] $c t $.", "metamath.constant-scope", 0);
  validate("$[ label.inc $] $a t $.", "metamath.label", 0);
  validate("$c $[ symbols.inc $] $.", "metamath.math-empty", 0);
  validate("$[ name$part.inc $]", "metamath.keyword-space", 0);
  std::printf("metamath audit: %zu READ-positive forms, %u checks\n",
              std::size(valid), checks);
  return ok;
}
} // namespace metamath_audit
