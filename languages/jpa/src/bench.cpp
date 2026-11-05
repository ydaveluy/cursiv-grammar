#include "cursiv/LanguageBench.hpp"
#include "jpa/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      jpa::JpaSharedServices, jpa::register_lsp_services>(argc, argv);
}
