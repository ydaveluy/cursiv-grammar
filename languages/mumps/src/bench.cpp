#include "cursiv/LanguageBench.hpp"
#include "mumps/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      mumps::MumpsSharedServices, mumps::register_lsp_services>(argc, argv);
}
