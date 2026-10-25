# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `szfGenerated` (the compiled library) and `szf_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID szf
    GRAMMAR    szf/core/SzfGrammar.hpp
    NAMESPACE  szf::Szf
    EXTENSIONS .sfz .szf
    OUT        szf/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/szf/core/*.cpp)
add_library(szfCore ${_core})
target_link_libraries(szfCore PUBLIC szfGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/szf/cli/*.cpp)
add_executable(szf_cli ${_cli})
target_link_libraries(szf_cli
                      PRIVATE szfCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/szf/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/szf/lsp/main.cpp)
add_library(szfLsp ${_lsp})
target_link_libraries(szfLsp PUBLIC szfCore cursiv::lsp)
add_executable(szf_lsp szf/lsp/main.cpp)
target_link_libraries(szf_lsp PRIVATE szfLsp)

cursiv_add_benchmark(szf SOURCES szf/bench/main.cpp)

cursiv_package_language(szf)
