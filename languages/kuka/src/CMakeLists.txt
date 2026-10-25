# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `kukaGenerated` (the compiled library) and `kuka_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID kuka
    GRAMMAR    kuka/core/KukaGrammar.hpp
    NAMESPACE  kuka::Kuka
    EXTENSIONS .kuka
    OUT        kuka/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/kuka/core/*.cpp)
add_library(kukaCore ${_core})
target_link_libraries(kukaCore PUBLIC kukaGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/kuka/cli/*.cpp)
add_executable(kuka_cli ${_cli})
target_link_libraries(kuka_cli
                      PRIVATE kukaCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/kuka/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/kuka/lsp/main.cpp)
add_library(kukaLsp ${_lsp})
target_link_libraries(kukaLsp PUBLIC kukaCore cursiv::lsp)
add_executable(kuka_lsp kuka/lsp/main.cpp)
target_link_libraries(kuka_lsp PRIVATE kukaLsp)

cursiv_add_benchmark(kuka SOURCES kuka/bench/main.cpp)

cursiv_package_language(kuka)
