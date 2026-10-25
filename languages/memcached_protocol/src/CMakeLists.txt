# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `memcachedprotocolGenerated` (the compiled library) and `memcachedprotocol_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID memcachedprotocol
    GRAMMAR    memcachedprotocol/core/MemcachedProtocolGrammar.hpp
    NAMESPACE  memcachedprotocol::MemcachedProtocol
    EXTENSIONS .memcachedprotocol
    OUT        memcachedprotocol/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/memcachedprotocol/core/*.cpp)
add_library(memcachedprotocolCore ${_core})
target_link_libraries(memcachedprotocolCore PUBLIC memcachedprotocolGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/memcachedprotocol/cli/*.cpp)
add_executable(memcachedprotocol_cli ${_cli})
target_link_libraries(memcachedprotocol_cli
                      PRIVATE memcachedprotocolCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/memcachedprotocol/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/memcachedprotocol/lsp/main.cpp)
add_library(memcachedprotocolLsp ${_lsp})
target_link_libraries(memcachedprotocolLsp PUBLIC memcachedprotocolCore cursiv::lsp)
add_executable(memcachedprotocol_lsp memcachedprotocol/lsp/main.cpp)
target_link_libraries(memcachedprotocol_lsp PRIVATE memcachedprotocolLsp)

cursiv_add_benchmark(memcachedprotocol SOURCES memcachedprotocol/bench/main.cpp)

cursiv_package_language(memcachedprotocol)
