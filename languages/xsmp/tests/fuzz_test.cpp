// The widest: a whole catalogue, GLR on ReturnParameter.

#include <memory>

#include "cursiv/fuzz/FuzzLane.hpp"
#include "xsmp/core/XsmpGrammar.hpp"
#include "xsmp/generated/Xsmp/Module.hpp"

namespace {
void fuzz_xsmp() {
  namespace g = xsmp::generated::Xsmp;
  const std::unique_ptr<cursiv::Parser> parser = g::create_parser();
  const std::unique_ptr<cursiv::AstReflection> reflection =
      g::create_ast_reflection();
  cursiv::fuzzer::lane<xsmp::Xsmp>("xsmp", *parser, *reflection, 200, 0x5eed0b);
}
}  // namespace

int main() {
  fuzz_xsmp();
  return cursiv::test::report("xsmp_fuzz_test");
}
