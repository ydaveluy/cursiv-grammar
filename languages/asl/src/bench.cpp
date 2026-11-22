#include "cursiv/LanguageBench.hpp"
#include "asl/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      asl::AslSharedServices, asl::register_lsp_services>(argc, argv);
}
