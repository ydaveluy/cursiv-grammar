#pragma once

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "xyz/core/CoreServices.hpp"
#include <array>
#include <bit>
#include <cstdio>
#include <string>
#include <vector>

namespace native_regressions {
inline std::string bits(double value) {
  char text[17];
  std::snprintf(text, sizeof text, "%016llx",
                static_cast<unsigned long long>(std::bit_cast<std::uint64_t>(value)));
  return text;
}

inline bool run(const xyz::XyzCoreServices& services) {
  struct Atom {
    std::string element;
    std::array<std::string,3> position;
    std::vector<std::string> extra;
  };
  struct Frame { std::string count, description; std::vector<Atom> atoms; };
  struct Case { const char* source; bool nativeRead; const char* code; std::vector<Frame> frames; };
  const std::array cases = {
    Case{"1\none atom\nH 0 0 0\n",true,"", {Frame{"1","one atom", {Atom{"H", {"0000000000000000","0000000000000000","0000000000000000"}, {}}}}}},
    Case{"0\n\n",true,"", {Frame{"0","", {}}}},
    Case{"0\nempty",true,"", {Frame{"0","empty", {}}}},
    Case{"1\none\nHe 0 1 2",true,"", {Frame{"1","one", {Atom{"He", {"0000000000000000","3ff0000000000000","4000000000000000"}, {}}}}}},
    Case{"2\n  padded title  \nh +.5 -0.0 1e-3\nHE 1. 2E+2 -3\n",true,"", {Frame{"2","  padded title  ", {Atom{"h", {"3fe0000000000000","8000000000000000","3f50624dd2f1a9fc"}, {}},Atom{"HE", {"3ff0000000000000","4069000000000000","c008000000000000"}, {}}}}}},
    Case{"1\nfirst\nC 1 2 3\n2\nsecond\nH 0 0 0\nO 1 1 1\n",true,"", {Frame{"1","first", {Atom{"C", {"3ff0000000000000","4000000000000000","4008000000000000"}, {}}}},Frame{"2","second", {Atom{"H", {"0000000000000000","0000000000000000","0000000000000000"}, {}},Atom{"O", {"3ff0000000000000","3ff0000000000000","3ff0000000000000"}, {}}}}}},
    Case{"0\nempty\n1\nlast\nLi 0 1 2\n",true,"", {Frame{"0","empty", {}},Frame{"1","last", {Atom{"Li", {"0000000000000000","3ff0000000000000","4000000000000000"}, {}}}}}},
    Case{"1\nnonfinite\nX NaN +inf -Infinity\n",true,"", {Frame{"1","nonfinite", {Atom{"X", {"7ff8000000000000","7ff0000000000000","fff0000000000000"}, {}}}}}},
    Case{"1\nextra columns\nH 0 1 2 99 label\n",true,"", {Frame{"1","extra columns", {Atom{"H", {"0000000000000000","3ff0000000000000","4000000000000000"}, {"99","label"}}}}}},
    Case{"1\r\nwindows\r\n\tHe\t-1\t2\t3\r\n",true,"", {Frame{"1","windows", {Atom{"He", {"bff0000000000000","4000000000000000","4008000000000000"}, {}}}}}},
    Case{"0_1\ncount\nH 0 0 0\n",true,"", {Frame{"0_1","count", {Atom{"H", {"0000000000000000","0000000000000000","0000000000000000"}, {}}}}}},
    Case{"",true,"", {}},
    Case{"+1\nsign\nH 0 0 0\n",true,"", {Frame{"+1","sign", {Atom{"H", {"0000000000000000","0000000000000000","0000000000000000"}, {}}}}}},
    Case{"1.5\nbad\nH 0 0 0\n",false,"xyz.count", {}},
    Case{"1e100\nbad\nH 0 0 0\n",false,"xyz.count", {}},
    Case{"18446744073709551616\nbad\nH 0 0 0\n",false,"xyz.count", {}},
    Case{"2\nbad\nH 0 0 0\n",false,"xyz.atom-count", {}},
    Case{"1\nbad\nFake 0 0 0\n",false,"xyz.element", {}},
    Case{"1__0\nbad\nH 0 0 0\n",false,"xyz.count", {}},
    Case{"0.0\nbad\n",false,"xyz.count", {}},
    Case{"-1\ninvalid count\n",true,"xyz.count", {Frame{"-1","invalid count", {}}}},
    Case{"1\nrange\nH 1e999 0 0\n",true,"", {Frame{"1","range", {Atom{"H", {"7ff0000000000000","0000000000000000","0000000000000000"}, {}}}}}},
    Case{"1\nrange\nH -1e-999 0 0\n",true,"", {Frame{"1","range", {Atom{"H", {"8000000000000000","0000000000000000","0000000000000000"}, {}}}}}},
    Case{"1\nrange\nH -0e99999 0 0\n",true,"", {Frame{"1","range", {Atom{"H", {"8000000000000000","0000000000000000","0000000000000000"}, {}}}}}},
  };
  for (const auto& test : cases) {
    const std::string source(test.source);
    const auto result = services.parser->parse(source);
    const auto* doc = result.value_as<xyz::ast::Document>();
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !doc)
      return false;
    if (test.nativeRead) {
      if (doc->frames.size() != test.frames.size())
        return false;
      for (std::size_t i = 0; i < test.frames.size(); ++i) {
        const auto* frame = doc->frames[i];
        const auto& expected = test.frames[i];
        if (frame->parent != doc || frame->count != expected.count ||
            frame->description != expected.description ||
            frame->atoms.size() != expected.atoms.size())
          return false;
        for (std::size_t j = 0; j < expected.atoms.size(); ++j) {
          const auto* atom = frame->atoms[j];
          const auto& want = expected.atoms[j];
          if (atom->parent != frame || atom->element != want.element ||
              bits(atom->x) != want.position[0] || bits(atom->y) != want.position[1] ||
              bits(atom->z) != want.position[2] || atom->extra.size() != want.extra.size())
            return false;
          for (std::size_t k = 0; k < want.extra.size(); ++k)
            if (atom->extra[k] != want.extra[k])
              return false;
        }
      }
    }
    const auto validation = cursiv::test::validationHelper(services,source);
    if (*test.code) {
      bool found = false;
      for (const auto& d : validation.diagnostics)
        found |= d.code && d.code->isString() && d.code->string() == test.code;
      if (!found)
        return false;
    } else if (!validation.diagnostics.empty()) {
      return false;
    }
  }
  return true;
}
}  // namespace native_regressions
