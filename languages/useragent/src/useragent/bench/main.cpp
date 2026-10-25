#include "cursiv/bench/LanguageBench.hpp"
#include "useragent/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      useragent::UseragentSharedServices, useragent::register_lsp_services>(argc, argv);
}
