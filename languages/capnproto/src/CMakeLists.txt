# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `capnprotoGenerated` (the compiled library) and `capnproto_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID capnproto
    GRAMMAR    capnproto/core/CapnprotoGrammar.hpp
    NAMESPACE  capnproto::Capnproto
    EXTENSIONS .capnp
    OUT        capnproto/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/capnproto/core/*.cpp)
add_library(capnprotoCore ${_core})
target_link_libraries(capnprotoCore PUBLIC capnprotoGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/capnproto/cli/*.cpp)
add_executable(capnproto_cli ${_cli})
target_link_libraries(capnproto_cli
                      PRIVATE capnprotoCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/capnproto/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/capnproto/lsp/main.cpp)
add_library(capnprotoLsp ${_lsp})
target_link_libraries(capnprotoLsp PUBLIC capnprotoCore cursiv::lsp)
add_executable(capnproto_lsp capnproto/lsp/main.cpp)
target_link_libraries(capnproto_lsp PRIVATE capnprotoLsp)

cursiv_add_benchmark(capnproto SOURCES capnproto/bench/main.cpp)

cursiv_package_language(capnproto)
