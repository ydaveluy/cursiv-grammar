#include "cursiv/LanguageBench.hpp"
#include "redis/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      redis::RedisSharedServices, redis::register_lsp_services>(argc, argv);
}
