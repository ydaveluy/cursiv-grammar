#include "cursiv/bench/LanguageBench.hpp"
#include "mdx/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      mdx::MdxSharedServices, mdx::register_lsp_services>(argc, argv);
}
