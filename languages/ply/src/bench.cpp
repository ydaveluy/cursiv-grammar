#include "cursiv/LanguageBench.hpp"
#include "ply/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      ply::PlySharedServices, ply::register_lsp_services>(argc, argv);
}
