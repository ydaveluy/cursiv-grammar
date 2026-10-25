# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `llvmirGenerated` (the compiled library) and `llvmir_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID llvmir
    GRAMMAR    llvmir/core/LlvmIrGrammar.hpp
    NAMESPACE  llvmir::LlvmIr
    EXTENSIONS .ll
    OUT        llvmir/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/llvmir/core/*.cpp)
add_library(llvmirCore ${_core})
target_link_libraries(llvmirCore PUBLIC llvmirGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/llvmir/cli/*.cpp)
add_executable(llvmir_cli ${_cli})
target_link_libraries(llvmir_cli
                      PRIVATE llvmirCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/llvmir/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/llvmir/lsp/main.cpp)
add_library(llvmirLsp ${_lsp})
target_link_libraries(llvmirLsp PUBLIC llvmirCore cursiv::lsp)
add_executable(llvmir_lsp llvmir/lsp/main.cpp)
target_link_libraries(llvmir_lsp PRIVATE llvmirLsp)

cursiv_add_benchmark(llvmir SOURCES llvmir/bench/main.cpp)

cursiv_package_language(llvmir)
