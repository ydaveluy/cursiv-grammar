#include "cursiv/bench/LanguageBench.hpp"
#include "blueprint/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      blueprint::BlueprintSharedServices, blueprint::register_lsp_services>(argc, argv);
}
