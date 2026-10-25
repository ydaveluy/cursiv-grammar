#include <cstdio>
#include <string>
#include <string_view>
#include <vector>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "cmake/core/CoreServices.hpp"
#include "cmake/core/ast.hpp"

namespace {
bool collect(const cmake::ast::Argument* argument, const cursiv::AstNode* parent,
             std::vector<std::string_view>& out) {
  if (!argument || argument->parent != parent) return false;
  if (const auto* a = cursiv::ast_cast<cmake::ast::CompoundArgument>(argument)) {
    out.push_back("(");
    for (const auto* child : a->arguments)
      if (!collect(child, a, out)) return false;
    out.push_back(")");
  } else if (const auto* a = cursiv::ast_cast<cmake::ast::UnquotedArgument>(argument)) out.push_back(a->text);
  else if (const auto* a = cursiv::ast_cast<cmake::ast::QuotedArgument>(argument)) out.push_back(a->text);
  else if (const auto* a = cursiv::ast_cast<cmake::ast::BracketArgument>(argument)) out.push_back(a->text);
  else return false;
  return true;
}
struct Case { const char* name; const char* text; std::vector<std::string_view> arguments; };
}
int main() {
  const auto language = cursiv::create_language<cmake::CmakeCoreServices,
      cmake::CmakeSharedCoreServices, cmake::install_core_module>();
  const std::vector<Case> cases = {
    {"simple","function(probe)\nendfunction()\nprobe(name _name 123 a-b a;b)\n",{"name","_name","123","a-b","a;b"}},
    {"quoted","function(probe)\nendfunction()\nprobe(\"one two\" \"\" \"a\\\"b\" \"a\\nb\" \"${missing}\" \"$ENV{HOME}\")\n",{"one two","","a\\\"b","a\\nb","${missing}","$ENV{HOME}"}},
    {"brackets","function(probe)\nendfunction()\nprobe([[plain]] [=[${var}]=] [==[body ]=] inside]==])\n",{"plain","${var}","body ]=] inside"}},
    {"bracket-leading-lf","function(probe)\nendfunction()\nprobe([[\nfirst\nsecond]])\n",{"first\nsecond"}},
    {"bracket-two-lf","function(probe)\nendfunction()\nprobe([=[\n\nbody]=])\n",{"\nbody"}},
    {"quoted-continuation","function(probe)\nendfunction()\nprobe(\"a\\\nb\" \"\\\nbody\\\n\")\n",{"ab","body"}},
    {"quoted-literal-lf","function(probe)\nendfunction()\nprobe(\"a\nb\")\n",{"a\nb"}},
    {"legacy-quotes","function(probe)\nendfunction()\nprobe(a\"b\"c a\"b c\"d a\"\"b)\n",{"a\"b\"c","a\"b c\"d","a\"\"b"}},
    {"legacy-makevar","function(probe)\nendfunction()\nprobe($(MAKEVAR) a$(X)b $() $(1_2))\n",{"$(MAKEVAR)","a$(X)b","$()","$(1_2)"}},
    {"legacy-bracket-prefix","function(probe)\nendfunction()\nprobe([\"a b\"c [=abc [ [abc abc[=def)\n",{"[\"a b\"c","[=abc","[","[abc","abc[=def"}},
    {"legacy-escapes","function(probe)\nendfunction()\nprobe(a\\ b a\\(b\\) a\\#b a\\;b)\n",{"a\\ b","a\\(b\\)","a\\#b","a\\;b"}},
    {"line-comment-equals","function(probe)\nendfunction()\nprobe(a #[=ordinary\nb)\n",{"a","b"}},
    {"line-comment-equals-many","function(probe)\nendfunction()\nprobe(a #[==] ordinary\nb)\n",{"a","b"}},
    {"line-comment-bracket","function(probe)\nendfunction()\nprobe(a #[ ordinary\nb)\n",{"a","b"}},
    {"line-comment-empty","function(probe)\nendfunction()\nprobe(a #\nb)\n",{"a","b"}},
    {"bracket-comment","function(probe)\nendfunction()\nprobe(a #[=[\nignored\n]=] b)\n",{"a","b"}},
    {"bracket-comment-adjacent","function(probe)\nendfunction()\nprobe(a #[[ignored]] b)\n",{"a","b"}},
    {"parentheses","function(probe)\nendfunction()\nprobe((a b) c (d (e)))\n",{"(","a","b",")","c","(","d","(","e",")",")"}},
    {"unicode","function(probe)\nendfunction()\nprobe(café \"日本語\" [[λ]])\n",{"café","日本語","λ"}},
    {"quoted-adjacency","function(probe)\nendfunction()\nprobe(\"a\"b c\"d\" \"e\"\"f\")\n",{"a","b","c\"d\"","e","f"}},
    {"bracket-close-runs","function(probe)\nendfunction()\nprobe([==[a ]===] b ]] c]==])\n",{"a ]===] b ]] c"}},
    {"crlf","function(probe)\nendfunction()\nprobe([[\r\na\r\nb]] \"x\r\ny\" \"a\\\r\nb\")\n",{"a\nb","x\ny","ab"}},
    {"bom","﻿function(probe)\nendfunction()\nprobe(a)\n",{"a"}},
  };
  for (const auto& c : cases) {
    const auto result = language.language.parser->parse(c.text);
    const auto* file = result.value_as<cmake::ast::File>();
    const cmake::ast::CommandInvocation* probe = nullptr;
    if (file) for (const auto* cmd : file->commands)
      if (cmd && cmd->name == "probe") probe = cmd;
    std::vector<std::string_view> args;
    bool ok = result.accepted && !result.cst().recovered() &&
              result.diagnostics.empty() && file && probe && probe->parent == file;
    if (probe) for (const auto* arg : probe->arguments) ok &= collect(arg, probe, args);
    const auto validation = cursiv::test::validationHelper(language.language, c.text);
    if (!ok || args != c.arguments || !validation.diagnostics.empty()) {
      std::fprintf(stderr, "cmake: native argument regression: %s\n", c.name);
      for (const auto& d : validation.diagnostics) std::fprintf(stderr, "%s\n", d.message.c_str());
      return 1;
    }
  }
  struct Invalid { const char* text; const char* message; };
  for (const auto c : {
    Invalid{"function(probe)\nendfunction()\nprobe([==[not closed]=])\n","Bracket argument has no matching"},
    Invalid{"function(probe)\nendfunction()\nprobe(a\\qb \"\\1\")\n","Invalid character escape"},
    Invalid{"function(probe)\nendfunction()\nprobe\n(a)\n","Only spaces"},
    Invalid{"function(probe)\nendfunction()\nprobe(a) probe(b)\n","new line"},
    Invalid{"function(probe)\nendfunction()\nif(TRUE)\nprobe(a)\n","Missing endif"},
  }) {
    const auto parse = language.language.parser->parse(c.text);
    if (!parse.accepted || parse.cst().recovered() || !parse.diagnostics.empty()) return 1;
    const auto result = cursiv::test::validationHelper(language.language, c.text);
    bool found = false;
    for (const auto& diagnostic : result.diagnostics)
      if (diagnostic.message.find(c.message) != std::string::npos) found = true;
    if (!found) { std::fprintf(stderr, "cmake: missing runtime error: %s\n", c.message); return 1; }
  }
  std::printf("cmake: %zu native argument cases and 5 runtime diagnostics\n", cases.size());
}
