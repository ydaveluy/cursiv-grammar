#include "cursiv/LanguageBench.hpp"
#include "mumps/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      mumps::MumpsSharedServices, mumps::register_lsp_services>(argc, argv);
}
