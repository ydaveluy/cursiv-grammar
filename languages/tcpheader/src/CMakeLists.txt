# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `tcpheaderGenerated` (the compiled library) and `tcpheader_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID tcpheader
    GRAMMAR    tcpheader/core/TcpheaderGrammar.hpp
    NAMESPACE  tcpheader::Tcpheader
    EXTENSIONS .tcpheader
    OUT        tcpheader/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tcpheader/core/*.cpp)
add_library(tcpheaderCore ${_core})
target_link_libraries(tcpheaderCore PUBLIC tcpheaderGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tcpheader/cli/*.cpp)
add_executable(tcpheader_cli ${_cli})
target_link_libraries(tcpheader_cli
                      PRIVATE tcpheaderCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tcpheader/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/tcpheader/lsp/main.cpp)
add_library(tcpheaderLsp ${_lsp})
target_link_libraries(tcpheaderLsp PUBLIC tcpheaderCore cursiv::lsp)
add_executable(tcpheader_lsp tcpheader/lsp/main.cpp)
target_link_libraries(tcpheader_lsp PRIVATE tcpheaderLsp)

cursiv_add_benchmark(tcpheader SOURCES tcpheader/bench/main.cpp)

cursiv_package_language(tcpheader)
