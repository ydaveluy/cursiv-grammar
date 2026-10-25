# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `vmfGenerated` (the compiled library) and `vmf_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID vmf
    GRAMMAR    vmf/core/VmfGrammar.hpp
    NAMESPACE  vmf::Vmf
    EXTENSIONS .vmf
    OUT        vmf/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/vmf/core/*.cpp)
add_library(vmfCore ${_core})
target_link_libraries(vmfCore PUBLIC vmfGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/vmf/cli/*.cpp)
add_executable(vmf_cli ${_cli})
target_link_libraries(vmf_cli
                      PRIVATE vmfCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/vmf/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/vmf/lsp/main.cpp)
add_library(vmfLsp ${_lsp})
target_link_libraries(vmfLsp PUBLIC vmfCore cursiv::lsp)
add_executable(vmf_lsp vmf/lsp/main.cpp)
target_link_libraries(vmf_lsp PRIVATE vmfLsp)

cursiv_add_benchmark(vmf SOURCES vmf/bench/main.cpp)

cursiv_package_language(vmf)
