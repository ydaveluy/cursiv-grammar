#include "cursiv/bench/LanguageBench.hpp"
#include "semver/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      semver::SemverSharedServices, semver::register_lsp_services>(argc, argv);
}
