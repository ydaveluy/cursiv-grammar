#include "cursiv/bench/LanguageBench.hpp"
#include "trapc/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      trapc::TrapcSharedServices, trapc::register_lsp_services>(argc, argv);
}
