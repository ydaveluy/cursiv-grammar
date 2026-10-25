# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `cppGenerated` (the compiled library) and `cpp_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID cpp
    GRAMMAR    cpp/core/CppGrammar.hpp
    NAMESPACE  cpp::Cpp
    EXTENSIONS .cpp
    OUT        cpp/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cpp/core/*.cpp)
add_library(cppCore ${_core})
target_link_libraries(cppCore PUBLIC cppGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cpp/cli/*.cpp)
add_executable(cpp_cli ${_cli})
target_link_libraries(cpp_cli
                      PRIVATE cppCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cpp/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/cpp/lsp/main.cpp)
add_library(cppLsp ${_lsp})
target_link_libraries(cppLsp PUBLIC cppCore cursiv::lsp)
add_executable(cpp_lsp cpp/lsp/main.cpp)
target_link_libraries(cpp_lsp PRIVATE cppLsp)

cursiv_add_benchmark(cpp SOURCES cpp/bench/main.cpp)

cursiv_package_language(cpp)
