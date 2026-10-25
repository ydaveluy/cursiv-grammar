#include "cursiv/bench/LanguageBench.hpp"
#include "objc/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      objc::ObjcSharedServices, objc::register_lsp_services>(argc, argv);
}
