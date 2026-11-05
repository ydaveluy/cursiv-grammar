#include "cursiv/LanguageBench.hpp"
#include "limbo/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      limbo::LimboSharedServices, limbo::register_lsp_services>(argc, argv);
}
