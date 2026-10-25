# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `upnpGenerated` (the compiled library) and `upnp_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID upnp
    GRAMMAR    upnp/core/UpnpGrammar.hpp
    NAMESPACE  upnp::Upnp
    EXTENSIONS .upnp
    OUT        upnp/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/upnp/core/*.cpp)
add_library(upnpCore ${_core})
target_link_libraries(upnpCore PUBLIC upnpGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/upnp/cli/*.cpp)
add_executable(upnp_cli ${_cli})
target_link_libraries(upnp_cli
                      PRIVATE upnpCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/upnp/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/upnp/lsp/main.cpp)
add_library(upnpLsp ${_lsp})
target_link_libraries(upnpLsp PUBLIC upnpCore cursiv::lsp)
add_executable(upnp_lsp upnp/lsp/main.cpp)
target_link_libraries(upnp_lsp PRIVATE upnpLsp)

cursiv_add_benchmark(upnp SOURCES upnp/bench/main.cpp)

cursiv_package_language(upnp)
