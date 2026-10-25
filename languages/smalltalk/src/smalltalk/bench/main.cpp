#include "cursiv/bench/LanguageBench.hpp"
#include "smalltalk/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      smalltalk::SmalltalkSharedServices, smalltalk::register_lsp_services>(argc, argv);
}
