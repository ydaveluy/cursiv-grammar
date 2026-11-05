#include "cursiv/LanguageBench.hpp"
#include "useragent/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      useragent::UseragentSharedServices, useragent::register_lsp_services>(argc, argv);
}
