#include "cursiv/bench/LanguageBench.hpp"
#include "erlang/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      erlang::ErlangSharedServices, erlang::register_lsp_services>(argc, argv);
}
