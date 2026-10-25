#include "cursiv/bench/LanguageBench.hpp"
#include "aspectj/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      aspectj::AspectjSharedServices, aspectj::register_lsp_services>(argc, argv);
}
