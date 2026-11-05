#include "cursiv/LanguageBench.hpp"
#include "trapc/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      trapc::TrapcSharedServices, trapc::register_lsp_services>(argc, argv);
}
