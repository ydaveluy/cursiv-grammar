# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `rpnGenerated` (the compiled library) and `rpn_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID rpn
    GRAMMAR    rpn/core/RpnGrammar.hpp
    NAMESPACE  rpn::Rpn
    EXTENSIONS .rpn
    OUT        rpn/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rpn/core/*.cpp)
add_library(rpnCore ${_core})
target_link_libraries(rpnCore PUBLIC rpnGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rpn/cli/*.cpp)
add_executable(rpn_cli ${_cli})
target_link_libraries(rpn_cli
                      PRIVATE rpnCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rpn/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/rpn/lsp/main.cpp)
add_library(rpnLsp ${_lsp})
target_link_libraries(rpnLsp PUBLIC rpnCore cursiv::lsp)
add_executable(rpn_lsp rpn/lsp/main.cpp)
target_link_libraries(rpn_lsp PRIVATE rpnLsp)

cursiv_add_benchmark(rpn SOURCES rpn/bench/main.cpp)

cursiv_package_language(rpn)
