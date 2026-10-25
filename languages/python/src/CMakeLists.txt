# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `pythonGenerated` (the compiled library) and `python_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID python
    GRAMMAR    python/core/PythonGrammar.hpp
    NAMESPACE  python::Python
    EXTENSIONS .py
    OUT        python/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/python/core/*.cpp)
add_library(pythonCore ${_core})
target_link_libraries(pythonCore PUBLIC pythonGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/python/cli/*.cpp)
add_executable(python_cli ${_cli})
target_link_libraries(python_cli
                      PRIVATE pythonCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/python/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/python/lsp/main.cpp)
add_library(pythonLsp ${_lsp})
target_link_libraries(pythonLsp PUBLIC pythonCore cursiv::lsp)
add_executable(python_lsp python/lsp/main.cpp)
target_link_libraries(python_lsp PRIVATE pythonLsp)

cursiv_add_benchmark(python SOURCES python/bench/main.cpp)

cursiv_package_language(python)
