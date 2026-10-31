#pragma once

#include <cstdio>
#include <memory_resource>
#include <string_view>
#include <utility>

#include "clu/core/CluGrammar.hpp"
#include "cursiv/core/parser/Parser.hpp"

namespace clu_converter_cases {

inline bool run(const cursiv::Parser& parser) {
  const auto fail=[](const char* message) {
    std::fprintf(stderr,"clu converter: %s\n",message);return false;
  };
  std::pmr::monotonic_buffer_resource memory;
  for(const std::string_view text:{R"clu("\1")clu",R"clu("\12")clu",
                                  R"clu('\1')clu",R"clu('\12')clu",
                                  R"clu("\12x")clu",R"clu("\400")clu",
                                  R"clu("\q")clu",R"clu("\")clu"}) {
    const auto converted=clu::strip_quotes(text,&memory);
    if(converted || converted.error.kind!=cursiv::DiagnosticKind::conversion_error || converted.error.message.empty())
      return fail("incomplete/invalid escape must return a conversion diagnostic");
  }
  for(const auto [text,expected]:{
      std::pair<std::string_view,std::string_view>{R"clu("")clu",""},
      {R"clu("abc")clu","abc"},{R"clu("\123")clu","S"},
      {R"clu('\123')clu","S"},{R"clu("\377")clu",std::string_view("\xff",1)},
      {R"clu("\000")clu",std::string_view("\0",1)},
      {R"clu("\n\N\t\T\p\P\b\B\r\R\v\V\'\"\\")clu","\n\n\t\t\f\f\b\b\r\r\v\v'\"\\"}}) {
    const auto converted=clu::strip_quotes(text,&memory);
    if(!converted || *converted.value!=expected)
      return fail("native three-digit octal/simple escapes changed value");
  }
  const auto result=parser.parse(R"clu(p = proc () x: string := "\12" end p)clu");
  if(result.accepted && !result.cst().recovered() && result.diagnostics.empty())
    return fail("two-digit octal escape unexpectedly parses without a diagnostic");
  return true;
}

} // namespace clu_converter_cases
