#include "cursiv/bench/LanguageBench.hpp"
#include "pike/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      pike::PikeSharedServices, pike::register_lsp_services>(argc, argv);
}
