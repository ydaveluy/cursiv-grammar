#include "cursiv/bench/LanguageBench.hpp"
#include "capnproto/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      capnproto::CapnprotoSharedServices, capnproto::register_lsp_services>(argc, argv);
}
