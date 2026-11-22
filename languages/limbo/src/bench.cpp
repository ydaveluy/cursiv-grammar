#include "cursiv/LanguageBench.hpp"
#include "limbo/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      limbo::LimboSharedServices, limbo::register_lsp_services>(argc, argv);
}
