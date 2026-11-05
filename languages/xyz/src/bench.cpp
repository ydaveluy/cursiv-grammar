#include "cursiv/LanguageBench.hpp"
#include "xyz/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      xyz::XyzSharedServices, xyz::register_lsp_services>(argc, argv);
}
