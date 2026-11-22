#include "cursiv/LanguageBench.hpp"
#include "semver/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      semver::SemverSharedServices, semver::register_lsp_services>(argc, argv);
}
