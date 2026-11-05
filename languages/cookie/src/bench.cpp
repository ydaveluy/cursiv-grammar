#include "cursiv/LanguageBench.hpp"
#include "cookie/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      cookie::CookieSharedServices, cookie::register_lsp_services>(argc, argv);
}
