#include "cursiv/LanguageBench.hpp"
#include "blueprint/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      blueprint::BlueprintSharedServices, blueprint::register_lsp_services>(argc, argv);
}
