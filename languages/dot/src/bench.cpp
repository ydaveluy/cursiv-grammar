#include "cursiv/LanguageBench.hpp"
#include "dot/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      dot::DotSharedServices, dot::register_lsp_services>(argc, argv);
}
