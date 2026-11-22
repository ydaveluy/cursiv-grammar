#include "cursiv/LanguageBench.hpp"
#include "awk/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      awk::AwkSharedServices, awk::register_lsp_services>(argc, argv);
}
